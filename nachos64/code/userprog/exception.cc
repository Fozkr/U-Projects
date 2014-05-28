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

#include "copyright.h"
#include "system.h"
#include "syscall.h"
//Unix stuff
#include <fcntl.h>
#include <unistd.h>

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
// And don't forget to increment the pc before returning.(Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	are in machine.h.
//----------------------------------------------------------------------

// Modify the registers in order for the user program to continue running
// normally after the system call execution.
void returnFromSystemCall()
{
	int pc, npc;

	pc = machine->ReadRegister(PCReg);
	npc = machine->ReadRegister(NextPCReg);
	machine->WriteRegister(PrevPCReg, pc);		// PrevPC <- PC
	machine->WriteRegister(PCReg, npc);			// PC <- NextPC
	machine->WriteRegister(NextPCReg, npc + 4);	// NextPC <- NextPC+4
}	//returnFromSystemCall

// Read of write data (mainly chars) BYTE BY BYTE from/to the virtual
// user memory.
void readOrWriteVirtualMemory(bool read, char* array, int virtualAdress)
{
	int i = 0;
	if(read)
	{
		int temp = -1;
		while(temp != 0)
		{
			machine->ReadMem(virtualAdress++, 1, &temp);
			array[i] = (char) temp;
			++i;
		}
	}
	else
	{
		while(array[i] != '\0')
		{
			machine->WriteMem(virtualAdress++, 1, array[i]);
			++i;
		}
	}
}

// System call #0
// Halts the entire operating system and exits
void Nachos_Halt()
{
	DEBUG('a', "Shutdown, initiated by user program.\n");
	interrupt->Halt();

}	//Halt

// System call #1
// Exit
void Nachos_Exit()
{
	currentThread->Finish();
}

// System call #4
// Creates a file in the current directory (where the executable is).
void Nachos_Create()
{
	// Read the file name from the user virtual memory
	int virtualAdressOfParameter = machine->ReadRegister(4);
	char filename[128] = {'\0'};
	readOrWriteVirtualMemory(true, filename, virtualAdressOfParameter);
	
	// Unix creat
	int UnixFileID = creat(filename, S_IRWXU | S_IRWXG | S_IRWXO);
	
	// Verify for errors
	//if(UnixFileID < 0)
	
	// Return the UnixFileID
	machine->WriteRegister(2, UnixFileID);
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
	
	//TODO: create a global linked list that contains the names of all opened files
	// Check if the file is already open
	
	// Open the file using the Linux system call
	//filesystem->Open(filename); //find out if this is necessary
	int UnixFileID = open(filename, O_RDWR);
	
	// Use openFilesTable class to link nachos fileID with Unix fileID
	int NachosFileID = currentThread->openedFilesTable->Open(UnixFileID);
	
	// Verify for errors

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
	
	// Determine where to read from
	switch(NachosFileID)
	{
		case ConsoleInput:	//User can read from standard input, always open
			// Need a semaphore to control access to console
			// Console->P();
			// machine->consoleMutexSem->P(); //attempt to do that
			for(int i=0; i<bufferSize; ++i) //to control the amount of chars read
				scanf("%c", buffer+i); //apparently ok to use scanf
			buffer[bufferSize] = '\0'; //just in case
			// Update simulation stats, see details in Statistics class in machine/stats.cc
			// Console->V();
			// machine->consoleMutexSem->V(); //attempt to do that
			break;
		case ConsoleOutput: //User can not read from stdout
			machine->WriteRegister(2, -1);
			break;
		case ConsoleError: //...not sure what to do here, find out later***
			//This trick permits to write ints to console
			//printf("%d\n", machine->ReadRegister(4));
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
	// Read from virtual memory into that buffer
	readOrWriteVirtualMemory(true, buffer, virtualAdressOfBuffer);
	
	// Determine where to write
	switch(NachosFileID)
	{
		case ConsoleInput:	//User can not write to standard input
			machine->WriteRegister(2, -1);
			break;
		case ConsoleOutput: //User can write to stdout, always open
			// Need a semaphore to control access to console
			// Console->P();
			// machine->consoleMutexSem->P(); //attempt to do that
			buffer[bufferSize] = '\0'; //apparently necessary
			printf("%s", buffer); //apparently ok to use printf
			// Update simulation stats, see details in Statistics class in machine/stats.cc
			// Console->V();
			// machine->consoleMutexSem->V(); //attempt to do that
			break;
		case ConsoleError: //This trick permits to write ints to console
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
	
	// Check in the global linked list if this file is indeed open
	// If is not close, do nothing; if it is open, close it
	// Get the UnixFileID from our table for open files
	int UnixFileID = currentThread->openedFilesTable->getUnixFileID(NachosFileID);
	// Use the Unix close system call
	int result = close(UnixFileID);
	
	// Verify for errors with result
	
	// Return the result
	machine->WriteRegister(2, result);
}

// Secondary function for Nachos_Fork
void NachosForkThread(void* v)
{
	//return;
    AddrSpace *space;

    space = currentThread->space;
    space->InitRegisters();             // set the initial register values
    space->RestoreState();              // load page table register

	// Set the return address for this thread to the same as the main thread
	// This will lead this thread to call the exit system call and finish
	unsigned long p = (unsigned long) v;
    machine->WriteRegister(RetAddrReg, 4);
    machine->WriteRegister(PCReg, p);
    machine->WriteRegister(NextPCReg, p+4);
    machine->Run();                     // jump to the user progam
    ASSERT(false);
}

// System call #9
void Nachos_Fork()
{
	DEBUG( 'u', "Entering Fork System call\n" );
	// We need to create a new kernel thread to execute the user thread
	Thread* newThread = new Thread( "Child to execute Fork code" );
	currentThread->openedFilesTable->addThread();
	// We need to share the Open File Table structure with this new child
	newThread->openedFilesTable->copyTable(currentThread->openedFilesTable);
	
	// Child and father will also share the same address space, except for the stack
	// Text, init data and uninit data are shared, a new stack area must be created
	// for the new child
	// We suggest the use of a new constructor in AddrSpace class,
	// This new constructor will copy the shared segments (space variable) from currentThread, passed
	// as a parameter, and create a new stack for the new child
	newThread->space = new AddrSpace(currentThread->space);

	// We (kernel)-Fork to a new method to execute the child code
	// Pass the user routine address, now in register 4, as a parameter
	newThread->Fork(NachosForkThread, (void*) machine->ReadRegister(4));

	DEBUG( 'u', "Exiting Fork System call\n" );
}

// System call #10
void Nachos_Yield()
{
	currentThread->Yield();
}

// System call #11
void Nachos_SemCreate()
{
	// Get initial value for semaphore
	int initialValueOfSemaphore = machine->ReadRegister(4);
	Semaphore* sem = new Semaphore("Sempahore created in system call",initialValueOfSemaphore);
	
	// Get the next free position in the openFilesTable which is equivalent to this semaphoreID
	// The semaphores are interpreted as files too
	int semID = currentThread->openedFilesTable->Open((long)sem);
	
	// Return semID
	machine->WriteRegister(2, semID);
}

// System call #12
void Nachos_SemDestroy()
{
	// Get identification of sempahore
	int semID = machine->ReadRegister(4);
	// Obtain the semaphoreID from openFilesTable  
	Semaphore* sem = (Semaphore*)currentThread->openedFilesTable->getUnixFileID(semID);
	currentThread->openedFilesTable->Close(semID);
	// Invokes destroy so the threads in the semaphore queue don't stay asleep 
	sem->Destroy();
	delete sem;
}

// System call #13
void Nachos_SemSignal()
{
	// Get identification of sempahore
	int semID = machine->ReadRegister(4);
	// Obtain the semaphoreID from openFilesTable  
	Semaphore* sem = (Semaphore*)currentThread->openedFilesTable->getUnixFileID(semID);
	sem->V();
}

// System call #14
void Nachos_SemWait()
{
	// Get identification of sempahore
	int semID = machine->ReadRegister(4);
	// Obtain the semaphoreID from openFilesTable  
	Semaphore* sem = (Semaphore*)currentThread->openedFilesTable->getUnixFileID(semID);
	sem->P();
}

// The exception handler, it is invoked whenever a system call is used
// and it handles what to do when any of them is used; it invokes each
// one of the previous functions as required.
void ExceptionHandler(ExceptionType whichException)
{
    DEBUG('a', "Entering ExceptionHandler\n");
	int type = machine->ReadRegister(2);
	switch(whichException)
	{
		case SyscallException:
			switch(type)
			{
				case SC_Halt:
					Nachos_Halt();		// System call # 0
					break;
				case SC_Exit:
					Nachos_Exit();		// System call # 1
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
		default:
			printf("Unexpected exception %d\n", whichException);
			ASSERT(false);
			break;
    }
}
