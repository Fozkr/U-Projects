// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts(which can also cause control to transfer from user
//	code into the Nachos kernel)are handled elsewhere.
//
// For now, this only handles the Halt()system call.
// Everything else core dumps.
//
// Copyright(c)1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

//Nachos stuff
#include "copyright.h"
#include "system.h"
#include "syscall.h"
//Unix stuff
#include <fcntl.h>
#include <unistd.h>
#include <fstream>

// Modify the registers in order for the user program to continue
// running normally after any system call execution.
void returnFromSystemCall()
{
	DEBUG('u', "Modifying registers to simulate return from system call\n");
	int pc, npc;
	pc = machine->ReadRegister(PCReg);
	npc = machine->ReadRegister(NextPCReg);
	machine->WriteRegister(PrevPCReg, pc);		// PrevPC <- PC
	machine->WriteRegister(PCReg, npc);			// PC <- NextPC
	machine->WriteRegister(NextPCReg, npc + 4);	// NextPC <- NextPC+4
}

// Read of write data (mainly chars) BYTE BY BYTE from/to the virtual
// Nachos user memory.
void readOrWriteVirtualMemory(bool read, char* array, long virtualAdress)
{
	int i = 0;
	if(read)
	{
		DEBUG('u', "Reading from virtual memory, starting at virtual adress: %d\n", virtualAdress);
		int temp = -1;
		while(temp != 0)
		{
			while(!machine->ReadMem(virtualAdress, 1, &temp));
			++virtualAdress;
			array[i] = (char) temp;
			++i;
		}
	}
	else
	{
		DEBUG('u', "Writing to virtual memory, starting at virtual adress: %d\n", virtualAdress);
		while(array[i] != '\0')
		{
			while(!machine->WriteMem(virtualAdress, 1, array[i]));
			++virtualAdress;
			++i;
		}
	}
}

// Add a page to the TLB, in case it is full, apply "second chance
// algorithm", keeping an iterator updated.
void updateTLB(unsigned int virtualPageNumber)
{
	DEBUG('u', "Updating the TLB\n");
	// Second chance algorithm (used although not necessary)
	while(machine->tlb[machine->tlbIterator].use) // while it finds pages "in use", set that to false
	{
		machine->tlb[machine->tlbIterator].use = false;
		machine->tlbIterator = (machine->tlbIterator + 1) % TLBSize;
	}
	
	// next, save the changes in that victim tlb page to the exiting pageTable
	// not assuming that all tlb pages are being used by currentThread...
	if(machine->tlb[machine->tlbIterator].physicalPage >= 0) // if it contains a valid page (exception: when starting nachos)
	{
		Thread* thr = (Thread*) machine->invertedTable[machine->tlb[machine->tlbIterator].physicalPage];
		unsigned int VP = machine->invertedTableVP[machine->tlb[machine->tlbIterator].physicalPage];
		thr->space->getPageTable()[VP].physicalPage = machine->tlb[machine->tlbIterator].physicalPage;
		thr->space->getPageTable()[VP].valid = machine->tlb[machine->tlbIterator].valid;
		thr->space->getPageTable()[VP].use = machine->tlb[machine->tlbIterator].use;
		thr->space->getPageTable()[VP].dirty = machine->tlb[machine->tlbIterator].dirty;
	}
	
	// finally, after finding a page "not in use", add there the one that caused the page fault
	DEBUG('u', "Moving page to tlb[%d], virtual page number: %d\n", machine->tlbIterator, virtualPageNumber);
	machine->tlb[machine->tlbIterator].virtualPage = virtualPageNumber;
	machine->tlb[machine->tlbIterator].physicalPage = currentThread->space->getPageTable()[virtualPageNumber].physicalPage;
	machine->tlb[machine->tlbIterator].valid = true; //currentThread->space->getPageTable()[virtualPageNumber].valid;
	machine->tlb[machine->tlbIterator].use = true; //currentThread->space->getPageTable()[virtualPageNumber].use;
	machine->tlb[machine->tlbIterator].dirty = currentThread->space->getPageTable()[virtualPageNumber].dirty;
	machine->tlb[machine->tlbIterator].readOnly = currentThread->space->getPageTable()[virtualPageNumber].readOnly;
}

// Store page in SWAP file
void storeInSWAP(Thread* otherThread, unsigned int physicalPageReplaced, unsigned int virtualPageReplaced)
{
	DEBUG('u', "Storing in SWAP, physical page: %d, virtual page: %d\n", physicalPageReplaced, virtualPageReplaced);
	unsigned int nextFreeSWAPFrame = machine->SWAPmap->Find(); // For now we will assume the SWAP never gets full
	machine->SWAP->WriteAt(&(machine->mainMemory[physicalPageReplaced*PageSize]), PageSize, nextFreeSWAPFrame*PageSize);
	otherThread->space->getPageTable()[virtualPageReplaced].physicalPage = nextFreeSWAPFrame; // save it for later, when reloading from SWAP
}

// Retrieve page from SWAP file
void readFromSWAP(int nextFreeFrame, unsigned int virtualPageNumber, int oldSWAPframe)
{
	DEBUG('u', "Reading from SWAP, to physical page: %d, virtual page: %d, oldSWAPframe: %d\n", nextFreeFrame, virtualPageNumber, oldSWAPframe);
	machine->SWAP->ReadAt(&(machine->mainMemory[nextFreeFrame * PageSize]), PageSize, oldSWAPframe * PageSize);
	machine->SWAPmap->Clear(oldSWAPframe);
}

// Find a frame in memory using an iterator and the second chance algorithm, to release it and then
// use it to store the new page that caused a page fault
void replaceMainMemoryFrame()
{
	DEBUG('u', "REPLACING FRAME: ");
	// Replace a frame, first find one with the "use bit" on false (second chance)
	Thread* thr = (Thread*) machine->invertedTable[machine->mainMemoryIterator]; // to iterate through the pages
	unsigned int VP = machine->invertedTableVP[machine->mainMemoryIterator];
	// Second chance replacement algorithm
	while(thr->space->getPageTable()[VP].use)
	{
		// while it finds pages "in use", set that use bit to false
		thr->space->getPageTable()[VP].use = false;
		machine->mainMemoryIterator = (machine->mainMemoryIterator + 1) % NumPhysPages;
		thr = (Thread*) machine->invertedTable[machine->mainMemoryIterator];
		VP = machine->invertedTableVP[machine->mainMemoryIterator];
	}
	DEBUG('u', "%d\n", machine->mainMemoryIterator);
	
	// Then tell the thread using that physical page to make that virtual page not valid
	thr->space->getPageTable()[VP].valid = false;
	thr->space->getPageTable()[VP].physicalPage = -1;

	// If that page was in the tlb, remove it from there, but also update its dirty bit before removing!
	unsigned short posInTLB = 0;
	while(posInTLB < TLBSize && machine->tlb[posInTLB].physicalPage != machine->mainMemoryIterator)
		++posInTLB;
	if(posInTLB < 4) // it found it in the tlb, update dirty bit
		thr->space->getPageTable()[VP].dirty = machine->tlb[posInTLB].dirty;
	
	// If that page is dirty (bit), then save it in the SWAP
	if(thr->space->getPageTable()[VP].dirty)
		storeInSWAP(thr, machine->mainMemoryIterator, VP);
	
	// Then, clear the position and update mainMemoryIterator
	mainMemoryMap->Clear(machine->mainMemoryIterator);
	//bzero(&(machine->mainMemory[machine->mainMemoryIterator*PageSize]), PageSize); // clean new frame
	machine->mainMemoryIterator = (machine->mainMemoryIterator + 1) % NumPhysPages;
}

// System call #0
// Halt
// Halts the entire operating system and exits.
void Nachos_Halt()
{
	DEBUG('u', "Shutdown, initiated by user program.\n");
	interrupt->Halt();

}

// System call #1
// Exit
// The current thread exits, it must wait for its child processes to
// finish, close open files, delete semaphores, update father's usage,
// signal the father if it is waiting in a Join, remove itself from the
// threadsTable and delete its adress space from the memory.
void Nachos_Exit()
{
	DEBUG('u', "Exiting through system call, pid: %d\n", currentThread->pid);
	DEBUG('x', "Exit status: %d\n", machine->ReadRegister(4));
	
	// First wait for the child processes to finish
	DEBUG('u', "Waiting for child processes\n");
	while(currentThread->openedFilesTable->getUsage()>1 && currentThread->associatedSemaphores->getUsage()>1) //while there are active child processes
		currentThread->Yield(); //just wait
	
	// Close the open files (that were opened by the current thread)
	DEBUG('u', "Closing files\n");
	for(unsigned int file=3; file<SIZE_OF_TABLE; ++file) // Start with 3, becuase 0, 1, 2 are reserved
	{
		if(currentThread->openedFilesTable->isOpen(file) && currentThread->openedFilesTable->openedByCurrentThread[file])
		{
			// Simulate Nachos_Close call
			// Get the UnixFileID from our table of open files
			int UnixFileID = currentThread->openedFilesTable->getUnixFileID(file);
			currentThread->openedFilesTable->Close(file);
			// Use the Unix close system call
			close(UnixFileID);
		}
	}
	
	// Delete the associated semaphores (created by the current thread)
	DEBUG('u', "Deleting semaphores\n");
	for(unsigned int sem=0; sem<SIZE_OF_TABLE; ++sem)
	{
		if(currentThread->associatedSemaphores->isOpen(sem) && currentThread->associatedSemaphores->openedByCurrentThread[sem])
		{
			// Simulate Nachos_Close call
			// Get the UnixsemID from our table of open sems
			Semaphore* semaph = (Semaphore*) currentThread->associatedSemaphores->getUnixFileID(sem);
			currentThread->associatedSemaphores->Close(sem);
			// Delete the dynamically created
			delete semaph;
		}
	}
	
	// If this is a child process...
	if(currentThread->fatherProcess != NULL)
	{
		// Substract 1 from the father process tables' usage
		DEBUG('u', "Substracting 1 from father's usage\n");
		currentThread->fatherProcess->openedFilesTable->delThread();
		currentThread->fatherProcess->associatedSemaphores->delThread();
	
		// Signal the father if he used JOIN
		DEBUG('u', "Check if the thread is being waited for in a Join, content in table: %d\n", threadsTable->getUnixFileID(currentThread->pid));
		if(threadsTable->getUnixFileID(currentThread->pid) > 0)
		{ //it means that it contains a Semaphore adress (pointer) and that indeed JOIN was used
			DEBUG('u', "Signal the father (pid = %d) %d\n", currentThread->pid, threadsTable->getUnixFileID(currentThread->pid));
			Semaphore* sem = (Semaphore*) threadsTable->getUnixFileID(currentThread->pid);
			sem->V();
		}
	}
	
	// Remove myself from the global threads table
	DEBUG('u', "Removing from global threads table\n");
	if(currentThread->pid >= 0 && currentThread->pid <= SIZE_OF_TABLE) //if I am a child process
		threadsTable->Close(currentThread->pid);
	
	// Finally, finish the current thread
	DEBUG('u', "Exiting exit\n");
	currentThread->Finish();
	
	// Free the adress space, done in the adress space destructor, invoked by the thread destructor
}

// Secondary function of Nachos_Exec
void nachosExecThread(void* virtualAdressOfParameter)
{
	char filename[128] = {'\0'};
	readOrWriteVirtualMemory(true, filename, (long) virtualAdressOfParameter);
    OpenFile* executable = fileSystem->Open((const char*)filename);
    AddrSpace* space;

    if(executable == NULL)
    {
		printf("Unable to open file %s at Exec_aux\n", (char*) filename);
		return;
    }
    
    space = new AddrSpace(executable);
    strcpy(space->getFilename(), filename);
    currentThread->space = space;

    delete executable;			// close file

    space->InitRegisters();		// set the initial register values
    space->RestoreState();		// load page table register

    machine->Run();			// jump to the user progam
    ASSERT(false);	
}

// System call #2
// Run the executable, stored in the Nachos file "name", and return the 
// address space identifier
void Nachos_Exec()
{
	// Read the file name from the user virtual memory
	long virtualAdressOfParameter = (long) machine->ReadRegister(4);
	
	// Create new thread and make it invoke kernel_Fork
	Thread* newThread = new Thread("Child", true, currentThread);
	newThread->Fork(nachosExecThread, (void*) virtualAdressOfParameter);
	
	// Return the pid of the child process
	machine->WriteRegister(2, newThread->pid);
}

// System call #3
// Join causes the father process to create a semaphore, store the
// semaphore's adress in the global threads table and goes to sleep
// using Wait()
void Nachos_Join()
{
	// Obtain child pid as parameter
	int childPID = machine->ReadRegister(4);
	
	if(threadsTable->isOpen(childPID)) //if it "is open", it means the process exists
	{
		// Create a semaphore that will be used to synch the father and child processes
		Semaphore* fatherJoinsChild = new Semaphore("Join semaphore", 0);
		// Store the semaphore's adress in the threadsTable
		threadsTable->storeSemAdress(childPID, (long) fatherJoinsChild);
		
		// Wait for the child process to finish
		fatherJoinsChild->P();
		
		// Return 0 on success
		machine->WriteRegister(2, 0);
	}
	else
		// Return -1 on failure
		machine->WriteRegister(2, -1);
}

// System call #4
// Creates a file in the current directory (where the executable is).
void Nachos_Create()
{
	// Read the file name from the user virtual memory
	int virtualAdressOfParameter = machine->ReadRegister(4);
	char filename[128] = {'\0'};
	readOrWriteVirtualMemory(true, filename, virtualAdressOfParameter);
	DEBUG('u', "Creating a file with filename: %s\n", filename);
	
	// Unix creat
	creat(filename, S_IRWXU | S_IRWXG | S_IRWXO);
	//int UnixFileID = creat(filename, S_IRWXU | S_IRWXG | S_IRWXO);
	// Verify for errors
	//if(UnixFileID < 0)
}

// System call #5
// Opens an already created file (if it does not exist, it should create
// it) in order to be able to "read from"/"write to" it.
void Nachos_Open()
{
	// Read the file name from the user virtual memory
	int virtualAdressOfParameter = machine->ReadRegister(4);
	char filename[128] = {'\0'};
	readOrWriteVirtualMemory(true, filename, virtualAdressOfParameter);
	DEBUG('u', "Opening a file with filename: %s\n", filename);
	
	// Open the file using the Linux system call
	//filesystem->Open(filename); //find out if this is necessary
	int UnixFileID = open(filename, O_RDWR);
	
	// Use openFilesTable class to link nachos fileID with Unix fileID
	int NachosFileID = currentThread->openedFilesTable->Open(UnixFileID);
	
	// Verify for errors
	//if(UnixFileID < 0)

	// Return the NachosFileID
	machine->WriteRegister(2, NachosFileID);
}

// System call #6
// Reads from either the console (stdin) or a file, given that the file
// is previously opened.
void Nachos_Read()
{
	char* buffer;
	// Obtain parameters
	int virtualAdressOfBuffer = machine->ReadRegister(4); //COPY WHAT IS READ INTO THIS BUFFER IN THE END
	int bufferSize = machine->ReadRegister(5);	// Read size to write
    OpenFileId NachosFileID = machine->ReadRegister(6);	// Read fileID
	// Create buffer with the given size
	buffer = new char[bufferSize+1]; //+1 to add a '\0' at the end
	DEBUG('u', "Reading from a file with fileID: %d\n", NachosFileID);
	
	// Determine where to read from
	switch(NachosFileID)
	{
		case ConsoleInput:	// User can read from standard input, always open
			// Need a semaphore to control access to console
			consoleMutexSem->P(); // Wait
			for(int i=0; i<bufferSize; ++i) // to control the amount of chars read
				scanf("%c", buffer+i); // apparently ok to use scanf
			buffer[bufferSize] = '\0'; // just in case
			// Update simulation stats, see details in Statistics class in machine/stats.cc
			consoleMutexSem->V(); // Signal
			machine->WriteRegister(2, 0); // return 0
			break;
		case ConsoleOutput: //User can not read from stdout
			machine->WriteRegister(2, -1);
			break;
		case ConsoleError:
			// Should not read
			break;
		default: // All other opened files
			// Verify if file is open, if not, return -1 in register2
			if(currentThread->openedFilesTable->isOpen(NachosFileID))
			{
				// Get the UnixFileID from our table for open files
				int UnixFileID = (int)(currentThread->openedFilesTable->getUnixFileID(NachosFileID));
				// Do the Unix read to the already opened Unix file
				int result = read(UnixFileID, buffer, bufferSize);
				// Verify for errors with result
				// Return the result
				machine->WriteRegister(2, result);
			}
			else
				machine->WriteRegister(2, -1);
			break;
    }
    // Now that it has read into the new buffer, copy to original buffer
	readOrWriteVirtualMemory(false, buffer, virtualAdressOfBuffer);
    
    // Delete the dynamic buffer
    delete buffer;
}

// System call #7
// Writes on either the console (stdout) or a file, given that the file
// is previously opened.
void Nachos_Write()
{
	char* buffer;
	// Obtain parameters
	int bufferSize = machine->ReadRegister(5);	// Read size to write
    OpenFileId NachosFileID = machine->ReadRegister(6);	// Read fileID
	int virtualAdressOfBuffer = machine->ReadRegister(4);
	// Create buffer with the given size
	buffer = new char[bufferSize+1]; //+1 to add a '\0' at the end
	DEBUG('u', "Writing to a file with fileID: %d\n", NachosFileID);
	// Read from virtual memory into that buffer
	readOrWriteVirtualMemory(true, buffer, virtualAdressOfBuffer);
	DEBUG('u', "Read: %s\n", buffer);
	
	// Determine where to write
	switch(NachosFileID)
	{
		case ConsoleInput:	// User can not write to standard input
			machine->WriteRegister(2, -1);
			break;
		case ConsoleOutput: // User can write to stdout, always open
			// Need a semaphore to control access to console
			consoleMutexSem->P(); // Wait
			buffer[bufferSize] = '\0'; // apparently necessary
			printf("%s", buffer); // apparently ok to use printf
			// Update simulation stats, see details in Statistics class in machine/stats.cc
			consoleMutexSem->V(); // Signal
			machine->WriteRegister(2, 0); // return 0
			break;
		case ConsoleError: // This trick permits to write ints to console
			printf("%d\n", machine->ReadRegister(4));
			break;
		default: // All other opened files
			// Verify if file is open, if not, return -1 in register2
			if(currentThread->openedFilesTable->isOpen(NachosFileID))
			{
				// Get the UnixFileID from our table for open files
				int UnixFileID = currentThread->openedFilesTable->getUnixFileID(NachosFileID);
				// Do the Unix write to the already opened Unix file
				int result = write(UnixFileID, buffer, bufferSize);
				// Verify for errors with result
				// Return the result
				machine->WriteRegister(2, result);
			}
			else
				machine->WriteRegister(2, -1);
			break;
    }
    // Delete the dynamic buffer
    delete buffer;
}

//System call #8
// Closes an open file
void Nachos_Close()
{
	// Read the nachos file ID of the file from register4
	int NachosFileID = machine->ReadRegister(4);
	DEBUG('u', "Closing a file with fileID: %d\n", NachosFileID);
	
	// Check if this file is indeed open. If is not, do nothing.
	if(currentThread->openedFilesTable->isOpen(NachosFileID))
	{
		// If it is open, close it. Get the UnixFileID from the table
		int UnixFileID = currentThread->openedFilesTable->getUnixFileID(NachosFileID);
		currentThread->openedFilesTable->Close(NachosFileID);
		// Use the Unix close system call
		close(UnixFileID);
		//int result = close(UnixFileID);
		// Verify for errors with result
		//if(result < 0)
	}
}

// Secondary function for Nachos_Fork
void NachosForkThread(void* pointerToFunction)
{
	DEBUG('u', "Starting NachosForkThread, the auxiliar function of Nachos_Fork\n");
    AddrSpace *space;

    space = currentThread->space;
    space->InitRegisters();             // set the initial register values
    space->RestoreState();              // load page table register

	// Set the return address for this thread to the same as the main thread
	// This will lead this thread to call the exit system call and finish
	unsigned long pointer = (unsigned long) pointerToFunction;
    machine->WriteRegister(RetAddrReg, 4);
    machine->WriteRegister(PCReg, pointer);
    machine->WriteRegister(NextPCReg, pointer+4);
    machine->Run();                     // jump to the user progam
    ASSERT(false);
}

// System call #9
void Nachos_Fork()
{
	DEBUG('u', "Entering Fork System call\n");
	// We need to create a new kernel thread to execute the user thread
	Thread* newThread = new Thread("Child-to-execute-Fork-code", true, currentThread);
	
	// We need to copy the Table structures with this new child
	newThread->openedFilesTable->copyTable(currentThread->openedFilesTable);
	newThread->associatedSemaphores->copyTable(currentThread->associatedSemaphores);
	newThread->openedFilesTable->initializeBoolTable();
	newThread->associatedSemaphores->initializeBoolTable();
	
	// Add this thread to the father tables
	currentThread->openedFilesTable->addThread();
	currentThread->associatedSemaphores->addThread();
	
	// Child and father will also share the same address space, except for the stack
	// Text, init data and uninit data are shared, a new stack area must be created
	// for the new child
	newThread->space = new AddrSpace(currentThread->space);

	// We (kernel)-Fork to a new method to execute the child code
	// Pass the user routine address, now in register 4, as a parameter
	newThread->Fork(NachosForkThread, (void*) (long) machine->ReadRegister(4));

	DEBUG('u', "Exiting Fork System call\n");
}

// System call #10
void Nachos_Yield()
{
	DEBUG('u', "Yielding\n");
	currentThread->Yield();
}

// System call #11
void Nachos_SemCreate()
{
	DEBUG('u', "Creating a semaphore\n");
	// Get initial value for semaphore
	int initialValueOfSemaphore = machine->ReadRegister(4);
	Semaphore* sem = new Semaphore("Sempahore created in system call",initialValueOfSemaphore);
	
	// Get the next free position in the openFilesTable which is equivalent to this semaphoreID
	int semID = currentThread->associatedSemaphores->Open((long)sem);
	
	// Return semID
	machine->WriteRegister(2, semID);
	DEBUG('u', "Created a semaphore with id: %d\n", semID);
}

// System call #12
void Nachos_SemDestroy()
{
	// Get identification of sempahore
	int semID = machine->ReadRegister(4);
	DEBUG('u', "Destroying semaphore with id: %d\n", semID);
	
	// Obtain the semaphoreID from openFilesTable  
	Semaphore* sem = (Semaphore*)currentThread->associatedSemaphores->getUnixFileID(semID);
	currentThread->associatedSemaphores->Close(semID);
	
	// Invokes destroy so the threads in the semaphore queue do not stay asleep 
	sem->Destroy();
	delete sem;
}

// System call #13
void Nachos_SemSignal()
{
	// Get identification of sempahore
	int semID = machine->ReadRegister(4);
	DEBUG('u', "Signaling semaphore with id: %d\n", semID);
	
	// Obtain the semaphoreID from openFilesTable  
	Semaphore* sem = (Semaphore*)currentThread->associatedSemaphores->getUnixFileID(semID);
	sem->V();
}

// System call #14
void Nachos_SemWait()
{
	// Get identification of sempahore
	int semID = machine->ReadRegister(4);
	DEBUG('u', "Waiting semaphore with id: %d\n", semID);
	
	// Obtain the semaphoreID from openFilesTable  
	Semaphore* sem = (Semaphore*)currentThread->associatedSemaphores->getUnixFileID(semID);
	sem->P();
}

//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	are in machine.h.
//----------------------------------------------------------------------
void ExceptionHandler(ExceptionType whichException)
{
    //DEBUG('u', "Entering ExceptionHandler with exception: %d\n", whichException);
	int type = machine->ReadRegister(2);
	
	bool validBit, dirtyBit, readOnlyBit;	// Used for handling page fault exceptions
	unsigned int virtualPageNumber;			// Used for handling page fault exceptions
	
	switch(whichException)
	{
		case SyscallException:
			DEBUG('u', "Syscall: %d\n", type);
			switch(type)
			{
				case SC_Halt:
					Nachos_Halt();		// System call # 0
					break;
				case SC_Exit:
					Nachos_Exit();		// System call # 1
					break;
				case SC_Exec:
					Nachos_Exec();		// System call # 2
					break;
				case SC_Join:
					Nachos_Join();		// System call # 3
					break;
				case SC_Create:
					Nachos_Create();	// System call # 4
					break;
				case SC_Open:
					Nachos_Open();		// System call # 5
					break;
				case SC_Read:
					Nachos_Read();		// System call # 6
					break;
				case SC_Write:
					Nachos_Write();		// System call # 7
					break;
				case SC_Close:
					Nachos_Close();		// System call # 8
					break;
				case SC_Fork:	
					Nachos_Fork();		// System call # 9
					break;
				case SC_Yield:
					Nachos_Yield();		// System call # 10
					break;
				case SC_SemCreate:
				    Nachos_SemCreate();		// System call # 11
				    break;
				case SC_SemDestroy:
					Nachos_SemDestroy();	// System call # 12
					break;
				case SC_SemSignal:
					Nachos_SemSignal();		// System call # 13
					break;
				case SC_SemWait:
					Nachos_SemWait();		// System call # 14
					break;
				default:
					printf("Unexpected syscall exception %d\n", type);
					ASSERT(false);
					break;
			}
			returnFromSystemCall(); //Update the pc registers
			break;
		case PageFaultException:
			DEBUG('u', "---Page Fault Exception detected---\nVirtual adress: %d", machine->ReadRegister(39));
			virtualPageNumber = machine->ReadRegister(39)/PageSize;
			DEBUG('u', ", Page number: %d", virtualPageNumber);
			validBit = currentThread->space->getPageTable()[virtualPageNumber].valid;
			DEBUG('u', ", Bit validez: %d", validBit);
			dirtyBit = currentThread->space->getPageTable()[virtualPageNumber].dirty;
			DEBUG('u', ", Bit suciedad: %d", dirtyBit);
			readOnlyBit = currentThread->space->getPageTable()[virtualPageNumber].readOnly;
			DEBUG('u', ", Bit readOnly: %d\n", readOnlyBit);
			/* Suggested code (by website and pagefault.jpg) that works */
			/* Page is not in TLB */
			/* Is the page in the page table? (assuming that means "is it in main memory?"...) */
			if(!validBit) // the page is not in main memory
			{
				/* Is there a free frame in main memory? */
				DEBUG('u', "Reserving space in main memory...\n");
				if(mainMemoryMap->NumClear() == 0) // if there are no free frames
					replaceMainMemoryFrame();
				int nextFreeFrame = mainMemoryMap->Find(); // to read into from SWAP, or from executable file
				
				/* Is the faulting page dirty? */
				if(dirtyBit) // the page is dirty
				{
					/* Read from SWAP, update pageTable, update TLB and exit */
					int oldSWAPframe = currentThread->space->getPageTable()[virtualPageNumber].physicalPage; // to read from SWAP
					readFromSWAP(nextFreeFrame, virtualPageNumber, oldSWAPframe);
				}
				else // the page is not dirty
				{
					/* Is the page of code or initialized data? */
					if(readOnlyBit || virtualPageNumber < (currentThread->space->getNumPagesCode() + currentThread->space->getNumPagesInitData())) // the page is either code or initData
					{
						/* Read from executable file, update pageTable, update TLB and exit */
						DEBUG('u', "The page is either code or initData, loading from disk to main memory...\n");
						OpenFile* executable = fileSystem->Open(currentThread->space->getFilename());
						NoffHeader noffH;
						executable->ReadAt((char*) &noffH, sizeof(noffH), 0);
						ASSERT(noffH.noffMagic == NOFFMAGIC);
						executable->ReadAt(&(machine->mainMemory[nextFreeFrame * PageSize]), PageSize, noffH.code.inFileAddr + (virtualPageNumber * PageSize));
						//DEBUG('u', "Reading vpage %d to ppage %d. Result: %d bytes read\n", virtualPageNumber, nextFreeFrame, executable->ReadAt(&(machine->mainMemory[nextFreeFrame * PageSize]), PageSize, noffH.code.inFileAddr + (virtualPageNumber * PageSize)));
						delete executable;
					}
					else // it is either uninitData or stack
						bzero(&(machine->mainMemory[nextFreeFrame*PageSize]), PageSize); /* Clean frame, update pageTable, update TLB and exit */
				}
				// Update pageTable
				currentThread->space->getPageTable()[virtualPageNumber].physicalPage = nextFreeFrame;
				currentThread->space->getPageTable()[virtualPageNumber].valid = true; // ***also done in updateTLB()
				
				// Store the data in the inverted table (because we just added a page to the main memory)
				DEBUG('u', "Storing Thread* %li in inverted table[%d] with virtual page: %d\n", (long) currentThread, nextFreeFrame, virtualPageNumber);
				machine->invertedTable[nextFreeFrame] = (long) currentThread;
				machine->invertedTableVP[nextFreeFrame] = virtualPageNumber;
				DEBUG('u', "Ok\n");
			}
			/* Is the TLB full? if it is, replace a page, then add the faulting page and exit */
			// If the page was indeed in main memory, simply add to the tlb... always, add to the tlb
			updateTLB(virtualPageNumber);
			break;
			
			/* end of suggested code */
			/* ********************************************************************************************************************************* */
			/* Original code (that did not work)
			if(!validBit) // if the page is not valid (IS NOT IN MAIN MEMORY/PAGE TABLE)
			{
				// It is code, initData, uninitData or stack, not in memory, all of them need to reserve space
				DEBUG('u', "Reserving space...\n");
				if(mainMemoryMap->NumClear() == 0) // if there are no free frames
					replaceMainMemoryFrame();
				// just in case the page is in SWAP, got to save that SWAP frame before overwriting it with nextFreeFrame
				int oldSWAPframe = currentThread->space->getPageTable()[virtualPageNumber].physicalPage;
				
				// Now store in the main memory
				int nextFreeFrame = mainMemoryMap->Find();
				bzero(&(machine->mainMemory[nextFreeFrame*PageSize]), PageSize); // clean new frame
				currentThread->space->getPageTable()[virtualPageNumber].physicalPage = nextFreeFrame;
				currentThread->space->getPageTable()[virtualPageNumber].valid = true;
				DEBUG('u', "Reserved page: %d\n", nextFreeFrame);
				
				// Next add to the tlb
				updateTLB(virtualPageNumber);
				
				// If it is readOnly, it is code, if the pagenumber is < than numPagesCode+numPagesInitData, then it is initData
				// If it is either code or initData, the page has not been loaded to memory, it must be loaded from disk
				if(readOnlyBit || virtualPageNumber < (currentThread->space->getNumPagesCode() + currentThread->space->getNumPagesInitData())) // if the page is not dirty
				{
					DEBUG('u', "It is code or initData, loading from disk to main memory...\n");
					OpenFile* executable = fileSystem->Open(currentThread->space->getFilename());
					NoffHeader noffH;
					executable->ReadAt((char*) &noffH, sizeof(noffH), 0);
					//if ((noffH.noffMagic != NOFFMAGIC) && (WordToHost(noffH.noffMagic) == NOFFMAGIC))
					//	SwapHeader(&noffH);
					ASSERT(noffH.noffMagic == NOFFMAGIC);
					//executable->ReadAt(&(machine->mainMemory[nextFreeFrame * PageSize]), PageSize, noffH.code.inFileAddr + (virtualPageNumber * PageSize));
					DEBUG('u', "Reading page %d. Result: %d bytes read\n", virtualPageNumber, executable->ReadAt(&(machine->mainMemory[nextFreeFrame * PageSize]), PageSize, noffH.code.inFileAddr + (virtualPageNumber * PageSize)));
					delete executable;
				}
				else // it is uninitData or stack
				{
					if(dirtyBit) // if it is dirty, must read from SWAP
					{
						DEBUG('u', "dirty bit true!!!\n");
						DEBUG('u', "It is uninitData or stack, loading from SWAP to main memory...\n");
						readFromSWAP(nextFreeFrame, virtualPageNumber, oldSWAPframe);
					}
				}
				
				// Store the data in the inverted table
				DEBUG('u', "Storing Thread* %li in inverted table with virtual page: %d\n", (long) currentThread, virtualPageNumber);
				machine->invertedTable[nextFreeFrame] = (long) currentThread;
				machine->invertedTableVP[nextFreeFrame] = virtualPageNumber;
				DEBUG('u', "Ok\n");
			}
			else // the page is valid (IS IN THE MAIN MEMORY/PAGE TABLE)
			{
				// Simply add to the tlb
				updateTLB(virtualPageNumber);
			}
			*/
			/* end of original code */
			
			//ASSERT(false);
			break;
		default:
			printf("Unexpected exception %d\n", whichException);
			ASSERT(false);
			break;
    }
}
