// addrspace.h 
//	Data structures to keep track of executing user programs 
//	(address spaces).
//
//	For now, we don't keep any information about address spaces.
//	The user level CPU state is saved and restored in the thread
//	executing the user program (see thread.h).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef ADDRSPACE_H
#define ADDRSPACE_H

#include "copyright.h"
#include "filesys.h"

#define UserStackSize		1024 		// increase this as necessary!

class AddrSpace
{
  public:
    AddrSpace(OpenFile *executable);	// Create an address space,
										// initializing it with the program
										// stored in the file "executable"
	AddrSpace(AddrSpace* otherSpace);	// Create an address space as copy 
										// of another adress space (father process's)
    ~AddrSpace();						// De-allocate an address space
    void InitRegisters();				// Initialize user-level CPU registers,
										// before jumping to user code
    void SaveState();					// Save/restore address space-specific
    void RestoreState();				// info on a context switch 
    TranslationEntry* getPageTable();	// Quickly retrieve the pageTable
    unsigned int getNumPagesCode();		// Quickly retrieve the numPagesCode
    unsigned int getNumPagesInitData();	// Quickly retrieve the numPagesInitData
	char* getFilename();				// Quickly retrieve the Filename
    
  public:

  private:
    TranslationEntry* pageTable;		// Assume linear page table translation
										// for now!
	char* executableFilename;			// To read when it is necessary
	
	unsigned int numPages;				// Number of pages in the virtual 
										// address space
    unsigned int numPagesCode;			// Number of pages of code in the virtual 
										// address space
    unsigned int numPagesInitData;		// Used to know when to read from the executable file or not
    
	unsigned int numPagesDataStack;		// Number of pages of data and stack in the virtual 
										// address space
};

#endif // ADDRSPACE_H
