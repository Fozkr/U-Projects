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
void Halt()
{
	DEBUG('a', "Shutdown, initiated by user program.\n");
	interrupt->Halt();

}	//Halt

// System call #1
// Exit
void Exit()
{
	currentThread->Finish();
}

// System call #4
// Creates a file in the current directory (where the executable is).
void Create()
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
void Open()
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
void Read()
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
				int UnixFileID = currentThread->openedFilesTable->getUnixFileID(NachosFileID);
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
void Write()
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

// Closes an open file
void Close()
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

// The exception handler, it is invoked whenever a system call is used
// and it handles what to do when any of them is used; it invokes each
// one of the previous functions as required.
void ExceptionHandler(ExceptionType whichException)
{
	int type = machine->ReadRegister(2);
	switch(whichException)
	{
		case SyscallException:
			switch(type)
			{
				case SC_Halt:
					Halt();		// System call # 0
					break;
				case SC_Exit:
					Exit();		// System call # 1
					break;
				case SC_Create:
					Create();	// System call # 4
					break;
				case SC_Open:
					Open();		// System call # 5
					break;
				case SC_Read:
					Read();		// System call # 6
					break;
				case SC_Write:
					Write();	// System call # 7
					break;
				case SC_Close:
					Close();	// System call # 8
					break;
				default:
					printf("Unexpected syscall exception %d\n", type);
					//ASSERT(FALSE);
					break;
			}
			returnFromSystemCall(); //Update the pc registers
			break;
		default:
			printf("Unexpected exception %d\n", whichException);
			//ASSERT(FALSE);
			break;
    }
/* Old code
	if((which == SyscallException)&&(type == SC_Halt))
	{
		DEBUG('a', "Shutdown, initiated by user program.\n");
		interrupt->Halt();
	}
	else
	{
		printf("Unexpected user mode exception %d %d\n", which, type);
		ASSERT(false);
	}
*/
}
