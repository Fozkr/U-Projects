// addrspace.cc 
//	Routines to manage address spaces (executing user programs).
//
//	In order to run a user program, you must:
//
//	1. link with the -N -T 0 option 
//	2. run coff2noff to convert the object file to Nachos format
//		(Nachos object code format is essentially just a simpler
//		version of the UNIX executable object code format)
//	3. load the NOFF file into the Nachos file system
//		(if you haven't implemented the file system yet, you
//		don't need to do this last step)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "addrspace.h"
#include "noff.h"

//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the 
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

static void 
SwapHeader (NoffHeader *noffH)
{
	noffH->noffMagic = WordToHost(noffH->noffMagic);
	noffH->code.size = WordToHost(noffH->code.size);
	noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
	noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
	noffH->initData.size = WordToHost(noffH->initData.size);
	noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
	noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
	noffH->uninitData.size = WordToHost(noffH->uninitData.size);
	noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
	noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
// 	Create an address space to run a user program.
//	Load the program from a file "executable", and set everything
//	up so that we can start executing user instructions.
//
//	Assumes that the object code file is in NOFF format.
//
//	First, set up the translation from program memory to physical 
//	memory.
//
//	"executable" is the file containing the object code to load into memory
//----------------------------------------------------------------------

AddrSpace::AddrSpace(OpenFile* executable)
{
	//For lab 7 testing
	//for(int itr=0; itr<=10; itr+=2)
	//	mainMemoryMap->Mark(itr);
	
    NoffHeader noffH;
    unsigned int i, sizeOfCode, sizeOfDataStack;

    executable->ReadAt((char*) &noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) && (WordToHost(noffH.noffMagic) == NOFFMAGIC))
    	SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

	// First, the code, asign separately from the data and stack
    sizeOfCode = noffH.code.size;
    sizeOfDataStack = noffH.initData.size + noffH.uninitData.size + UserStackSize;
	// we need to increase the size to leave room for the stack
    numPagesCode = divRoundUp(sizeOfCode, PageSize);
    numPagesDataStack = divRoundUp(sizeOfDataStack, PageSize);
    numPages = numPagesCode + numPagesDataStack;
    sizeOfCode = numPagesCode * PageSize;
    sizeOfDataStack = numPagesDataStack * PageSize;
    // store the number of initData pages for future reference
    numPagesInitData = divRoundUp(noffH.initData.size,PageSize);
			DEBUG('a', "numpagesinitdata: %d\n", numPagesInitData);

    ASSERT(numPages <= NumPhysPages);		// check we are not trying to
											// run anything too big
											// (at least until we have
											// virtual memory)

	executableFilename = new char[32]; // 32 for now

    DEBUG('a', "Initializing address space, num pages %d, size %d\n", numPages, sizeOfCode + sizeOfDataStack);
	// first, set up the translation
    pageTable = new TranslationEntry[numPages];
/*
    // First the code
    for(i=0; i<numPagesCode; ++i) // reserves pages for uninitData and stack too
    {
		pageTable[i].virtualPage = i;
		// Added on lab 9
#ifndef VM
		int nextFreePhysicalPage = mainMemoryMap->Find();
		DEBUG('a', "nextFreePhysicalPage: %d\n", nextFreePhysicalPage);
		pageTable[i].physicalPage = nextFreePhysicalPage;
		pageTable[i].valid = true;
#else
		pageTable[i].physicalPage = -1;
		pageTable[i].valid = false;
#endif
		pageTable[i].use = false;
		pageTable[i].dirty = false;
		pageTable[i].readOnly = true;  // if the code segment was entirely on 
										// a separate page, we can set its 
										// pages to be read-only
    }
    // Second, the data and stack
    for(; i<numPages; ++i) // reserves pages for uninitData and stack too
    {
		pageTable[i].virtualPage = i;
		// Added on lab 9
#ifndef VM
		int nextFreePhysicalPage = mainMemoryMap->Find();
		DEBUG('a', "nextFreePhysicalPage: %d\n", nextFreePhysicalPage);
		pageTable[i].physicalPage = nextFreePhysicalPage;
		pageTable[i].valid = true;
#else
		pageTable[i].physicalPage = -1;
		pageTable[i].valid = false;
#endif
		pageTable[i].use = false;
		pageTable[i].dirty = false;
		pageTable[i].readOnly = false;
    }
*/
	// Reserve pages
    for(i=0; i<numPages; ++i)
    {
		pageTable[i].virtualPage = i;
		// Added on lab 9
#ifndef VM
		int nextFreePhysicalPage = mainMemoryMap->Find();
		DEBUG('a', "nextFreePhysicalPage: %d\n", nextFreePhysicalPage);
		pageTable[i].physicalPage = nextFreePhysicalPage;
		pageTable[i].valid = true;
#else
		pageTable[i].physicalPage = -1;
		pageTable[i].valid = false;
#endif
		pageTable[i].use = false;
		pageTable[i].dirty = false;
		if(i < numPagesCode)
			pageTable[i].readOnly = true;	// if the code segment is entirely on a separate page, we can set its pages to be read-only
		else
			pageTable[i].readOnly = false;
    }
    
	// zero out the entire address space, to zero the unitialized data
	// segment and the stack segment
    //bzero(machine->mainMemory, size); // MODIFY THIS so it only zeros-out the necessary memory segment *****
#ifndef VM
	// then, copy the code and data segments into memory
    if(noffH.code.size > 0)
    {
        DEBUG('a', "Initializing code segment, at 0x%x, size %d\n", noffH.code.virtualAddr, noffH.code.size);
        //executable->ReadAt(&(machine->mainMemory[noffH.code.virtualAddr]), noffH.code.size, noffH.code.inFileAddr);
		//numPagesCode = divRoundUp(noffH.code.size, PageSize);
		for(i=0; i<numPagesCode; ++i)
		{
			int physicalPage = pageTable[i].physicalPage;
			executable->ReadAt(&(machine->mainMemory[physicalPage * PageSize]), PageSize, noffH.code.inFileAddr + (i * PageSize));
		}
    }
    if(noffH.initData.size > 0)
    {
        DEBUG('a', "Initializing data segment, at 0x%x, size %d\n", noffH.initData.virtualAddr, noffH.initData.size);
        //executable->ReadAt(&(machine->mainMemory[noffH.initData.virtualAddr]),noffH.initData.size, noffH.initData.inFileAddr);
        //numPagesInitData = divRoundUp(noffH.initData.size, PageSize);
		for(; i<numPagesInitData; ++i)
		{
			int physicalPage = pageTable[i].physicalPage;
			executable->ReadAt(&(machine->mainMemory[physicalPage * PageSize]), PageSize, noffH.code.inFileAddr + (i * PageSize)); //modified "noffH.initData.inFileAddr" to "noffH.code.inFileAddr" on 2/24/6
		}
    }
#endif
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
// 	Create an address space to run a user function.
//	Load the adress space from another adress space, as a copy from the
//	father process's adress space.
//----------------------------------------------------------------------

AddrSpace::AddrSpace(AddrSpace* otherSpace)
{
	numPagesCode = otherSpace->numPagesCode;
	numPagesDataStack = otherSpace->numPagesDataStack;
    numPagesInitData = otherSpace->numPagesInitData;
    numPages = numPagesCode + numPagesDataStack;
	DEBUG('a', "Copying address space, num pages: %d\n", numPages);
	unsigned int i;
    pageTable = new TranslationEntry[numPages];
    executableFilename = new char[32]; // 32 for now
    strcpy(executableFilename, otherSpace->getFilename());
/*
    // First the code
    for(i=0; i<numPagesCode; ++i)
    {
		DEBUG('a', "Copying code, page: %d\n", i);
		pageTable[i].virtualPage = otherSpace->pageTable[i].virtualPage;
		// Added on lab 9
#ifndef VM
		pageTable[i].physicalPage = otherSpace->pageTable[i].physicalPage;
		pageTable[i].valid = true;
#else
		pageTable[i].physicalPage = -1;
		pageTable[i].valid = false;
#endif
		pageTable[i].use = false;
		pageTable[i].dirty = false;
		pageTable[i].readOnly = true;  // if the code segment was entirely on 
										// a separate page, we could set its 
										// pages to be read-only
    }
    for(; i<(numPages - UserStackSize/PageSize); ++i) // reserves pages for uninitData and stack too
    {
		DEBUG('a', "Copying  inidata and uninitdata, page: %d\n", i);
		pageTable[i].virtualPage = otherSpace->pageTable[i].virtualPage;
		// Added on lab 9
#ifndef VM
		pageTable[i].physicalPage = otherSpace->pageTable[i].physicalPage;
		pageTable[i].valid = true;
#else
		pageTable[i].physicalPage = -1;
		pageTable[i].valid = false;
#endif
		pageTable[i].use = false;
		pageTable[i].dirty = false;
		pageTable[i].readOnly = false;  // if the code segment was entirely on 
										// a separate page, we could set its 
										// pages to be read-only
    }
*/
	// Reserve pages
	for(i=0; i<(numPages - UserStackSize/PageSize); ++i)
    {
		DEBUG('a', "Copying code, initdata and uninitdata, page: %d\n", i);
		pageTable[i].virtualPage = otherSpace->pageTable[i].virtualPage;
		// Added on lab 9
#ifndef VM
		pageTable[i].physicalPage = otherSpace->pageTable[i].physicalPage;
		pageTable[i].valid = true;
#else
		pageTable[i].physicalPage = -1;
		pageTable[i].valid = false;
#endif
		pageTable[i].use = false;
		pageTable[i].dirty = false;
		if(i < numPagesCode)
			pageTable[i].readOnly = true;	// if the code segment is entirely on a separate page, we can set its pages to be read-only
		else
			pageTable[i].readOnly = false;
    }
    // Use the same "i" from before, but run the for structure 8 times (the stack uses 8 pages)
    for(unsigned int k=0; k<(UserStackSize/PageSize); k++) // reserves pages for uninitData and stack too
    {
		DEBUG('a', "Reserving stack, page: %d\n", i);
		pageTable[i].virtualPage = i;
		// Added on lab 9
#ifndef VM
		int nextFreePhysicalPage = mainMemoryMap->Find();
		pageTable[i].physicalPage = nextFreePhysicalPage;
		pageTable[i].valid = true;
#else
		pageTable[i].physicalPage = -1;
		pageTable[i].valid = false;
#endif
		pageTable[i].use = false;
		pageTable[i].dirty = false;
		pageTable[i].readOnly = false;
		++i;
    }
}

//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
// 	Deallocate an address space.
//----------------------------------------------------------------------

AddrSpace::~AddrSpace()
{
	// If there are no child threads associated to this thread, delete
	// the whole adress space. Otherwise, delete only the stack.
	if(currentThread->openedFilesTable->getUsage() > 1) //there are child threads, delete the stack only
	{
		for(unsigned int page=(numPages - UserStackSize/PageSize); page<numPages; ++page)
			if(pageTable[page].physicalPage >=0 && mainMemoryMap->Test(pageTable[page].physicalPage))
				mainMemoryMap->Clear(pageTable[page].physicalPage);
	}
	else
	{
		for(unsigned int page=0; page<numPages; ++page)
			if(pageTable[page].physicalPage >=0 && mainMemoryMap->Test(pageTable[page].physicalPage))
				mainMemoryMap->Clear(pageTable[page].physicalPage);
	}
	delete pageTable;
	delete[] executableFilename;
}

//----------------------------------------------------------------------
// AddrSpace::InitRegisters
// 	Set the initial values for the user-level register set.
//
// 	We write these directly into the "machine" registers, so
//	that we can immediately jump to user code.  Note that these
//	will be saved/restored into the currentThread->userRegisters
//	when this thread is context switched out.
//----------------------------------------------------------------------

void AddrSpace::InitRegisters()
{
    int i;

    for (i = 0; i < NumTotalRegs; ++i)
	machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of "Start"
    machine->WriteRegister(PCReg, 0);	

    // Need to also tell MIPS where next instruction is, because
    // of branch delay possibility
    machine->WriteRegister(NextPCReg, 4);

   // Set the stack register to the end of the address space, where we
   // allocated the stack; but subtract off a bit, to make sure we don't
   // accidentally reference off the end!
    machine->WriteRegister(StackReg, numPages * PageSize - 16);
    DEBUG('a', "Initializing stack register to %d\n", numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//----------------------------------------------------------------------

void AddrSpace::SaveState() 
{
#ifdef VM
	for(unsigned int i=0; i<TLBSize; ++i)
	{
		pageTable[machine->tlb[i].virtualPage].physicalPage = machine->tlb[i].physicalPage;
		pageTable[machine->tlb[i].virtualPage].valid = machine->tlb[i].valid;
		pageTable[machine->tlb[i].virtualPage].use = machine->tlb[i].use;
		pageTable[machine->tlb[i].virtualPage].dirty = machine->tlb[i].dirty;
	}
#endif
}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState() 
{
#ifndef VM
    machine->pageTable = pageTable;
    machine->pageTableSize = numPages;
#else
	for(unsigned int i=0; i<TLBSize; i++)
	{
		machine->tlb[i].virtualPage = -1;
		machine->tlb[i].physicalPage = -1;
		machine->tlb[i].valid = false;
		machine->tlb[i].use = false;
		machine->tlb[i].dirty = false;
		machine->tlb[i].readOnly = false;
	}
#endif
}

TranslationEntry* AddrSpace::getPageTable()		{return pageTable;}
unsigned int AddrSpace::getNumPagesCode()		{return numPagesCode;}
unsigned int AddrSpace::getNumPagesInitData()	{return numPagesInitData;}
char* AddrSpace::getFilename()					{return executableFilename;}

