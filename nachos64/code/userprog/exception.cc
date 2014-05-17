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

//#include "../threads/system.h"
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

void returnFromSystemCall()	// returnFromSystemCall
{
	int pc, npc;

	pc = machine->ReadRegister(PCReg);
	npc = machine->ReadRegister(NextPCReg);
	machine->WriteRegister(PrevPCReg, pc);	// PrevPC <- PC
	machine->WriteRegister(PCReg, npc);	   // PC <- NextPC
	machine->WriteRegister(NextPCReg, npc + 4);   // NextPC <- NextPC + 4
}

void Halt()	// System call 0
{
	DEBUG('a', "Shutdown, initiated by user program.\n");
	interrupt->Halt();

}	// Nachos_Halt


void readFromVirtualMemory(char* array, int virtualAdress)
{
	int temp = -1;
	int i = 0;
	while(temp != 0)
	{
		machine->ReadMem(virtualAdress++, 1, &temp);
		array[i] = (char) temp;
		++i;
	}
}

void Create()
{
	// Read the file name from the user virtual memory, see 4 below
	int virtualAdressOfParameter = machine->ReadRegister(4);
	//int physicalAdressOfParameter;
	//machine->Translate(virtualAdressOfParameter, &physicalAdressOfParameter, 4, false);
	char filename[100] = {0};
	readFromVirtualMemory(filename, virtualAdressOfParameter);
	
	int result = creat(filename, S_IRWXU | S_IRWXG | S_IRWXO);
	
	// Verify for errors
	//if(result < 0)
	
	machine->WriteRegister(2, result);
}

void Open()	// System call 5
{
	// Read the file name from the user virtual memory, see 4 below
	int virtualAdressOfParameter = machine->ReadRegister(4);
	//int physicalAdressOfParameter;
	//machine->Translate(virtualAdressOfParameter, &physicalAdressOfParameter, 4, false);
	char filename[100] = {0};
	readFromVirtualMemory(filename, virtualAdressOfParameter);
	
	//TODO: create a global linked list that contains the names of all opened files
	//Check if the file is already open
	
	//Open the file using the Linux system call
	//filesystem->Open(filename);
	int UnixFileID = open(filename, O_RDWR);
	
	// Use openFilesTable class to create a relationship between user file and unix file
	currentThread->openedFilesTable->Open(UnixFileID);
	
	// Verify for errors

}       // Nachos_Open

void Write()	// System call 7
{
	char* buffer;
	int bufferSize = machine->ReadRegister(5);	// Read size to write
    OpenFileId fileID = machine->ReadRegister(6);	// Read file descriptor
	//printf("HELLOOO %d", fileID);
    // buffer = Read data from address given by user;
	int virtualAdressOfBuffer = machine->ReadRegister(4);
	// create buffer with the given size
	buffer = new char[bufferSize];
	// read from virtual memory
	readFromVirtualMemory(buffer, virtualAdressOfBuffer);
	switch(fileID)
	{
		case ConsoleInput:	// User could not write to standard input
			machine->WriteRegister(2, -1);
			break;
		case ConsoleOutput:
			// Need a semaphore to synchronize access to console
			// Console->P();
			//machine->consoleMutexSem->P();
			buffer[bufferSize] = 0;
			printf("%s", buffer);
			// Update simulation stats, see details in Statistics class in machine/stats.cc
			// Console->V();
			//machine->consoleMutexSem->V();
			break;
		case ConsoleError: // This trick permits to write integers to console
			printf("%d\n", machine->ReadRegister(4));
			break;
		default: // All other opened files
			if(currentThread->openedFilesTable->isOpened(fileID)) // Verify if the file is opened, if not return -1 in r2
			{
				// Get the unix handle from our table for open files
				// Do the write to the already opened Unix file
				int result = write(currentThread->openedFilesTable->getUnixHandle(fileID), buffer, bufferSize);
				// Verify for errors with result
				machine->WriteRegister(2, result);
			}
			else
				machine->WriteRegister(2, -1);
    }
    delete buffer;
    returnFromSystemCall(); // Update the PC registers

} // Nachos_Write
    
void ExceptionHandler(ExceptionType which)
{
	int type = machine->ReadRegister(2);
	switch(which)
	{
       case SyscallException:
		switch(type)
		{
	     case SC_Halt:
			Halt();	    // System call # 0
			break;
		 case SC_Create:
			Create();	// System call # 1
			break;
	     case SC_Open:
			Open();	    // System call # 2
			break;
	     case SC_Write:
			Write();	// System call # 3
			break;
	     default:
			printf("Unexpected syscall exception %d\n", type);
			//ASSERT(FALSE);
			break;
		}
		returnFromSystemCall();
		break;
      default:
		printf("Unexpected exception %d\n", which);
		//ASSERT(FALSE);
		break;
    }
/*
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
