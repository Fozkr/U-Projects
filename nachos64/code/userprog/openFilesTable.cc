#include "openFilesTable.h"

// Initialize, marking the first three positions
openFilesTable::openFilesTable()
	: usage(1)
{
	openFiles = new long[SIZE_OF_TABLE];
	openFilesMap = new BitMap(SIZE_OF_TABLE);
	// Mark positions 0,1,2, these are reserved
	openFilesMap->Mark(0); //stdin
	openFilesMap->Mark(1); //stdout
	openFilesMap->Mark(2); //stderr
	openedByCurrentThread = new bool[SIZE_OF_TABLE];
}

// Initialize, without marking any position, and with -1 everywhere if it is for the threadsTable
openFilesTable::openFilesTable(bool isThreadsTable)
	: usage(1)
{
	openFiles = new long[SIZE_OF_TABLE];
	openFilesMap = new BitMap(SIZE_OF_TABLE);
	openedByCurrentThread = new bool[SIZE_OF_TABLE];
	
	// Put -1 everywhere if it is the threadsTable
	if(isThreadsTable)
		for(unsigned int i=0; i<SIZE_OF_TABLE; ++i)
			openFiles[i] = -1;
}

// De-allocate
openFilesTable::~openFilesTable()
{
	delete openFiles;
	delete openFilesMap;
	delete openedByCurrentThread;
}

// Register the file ID in the table
// Register the semaphore adress in the table
int openFilesTable::Open(long UnixFileID)
{
	int nextFreePos = openFilesMap->Find();
	openFiles[nextFreePos] = UnixFileID;
	openedByCurrentThread[nextFreePos] = true;
	return nextFreePos;
}

// Unregister the file ID from the table (simply clear, no cleaning)
// Unregister the semaphore adress from the table (simply clear, no cleaning)
int openFilesTable::Close(int NachosFileID)
{
	openFilesMap->Clear(NachosFileID);
	return 0;
}

// Returns wether the file pointed by the NachosFileID is open or not
// Returns wether the semaphore identified by the NachosFileID exists or not
bool openFilesTable::isOpen(int NachosFileID)
{
	return openFilesMap->Test(NachosFileID);
}

// Returns the Unix file ID of the file using the Nachos file ID
// Returns the adress of the semaphore using the Nachos semaphore ID
long openFilesTable::getUnixFileID(int NachosFileID)
{
	return openFiles[NachosFileID];
}

// Register the semaphore ID in the table, invoked by Nachos_Join only
void openFilesTable::storeSemAdress(int NachosSemID, long adress)
{
	openFiles[NachosSemID] = adress;
}

// Simply add 1 to the counter of threads
void openFilesTable::addThread()
{
	++usage;
}

// Simply substract 1 from the counter of threads
void openFilesTable::delThread()
{
	--usage;
}

// Print stats and info, the contents of the table
void openFilesTable::Print()
{
	for(unsigned int pos=0; pos<SIZE_OF_TABLE; ++pos)
		printf("%d\t%d\t%li", pos, openFilesMap->Test(pos), openFiles[pos]);
}

// Initialize the bool table that indicates wether each file/semaphore
// has been opened/created by the current thread or not
void openFilesTable::initializeBoolTable()
{
	for(unsigned int i=0; i<SIZE_OF_TABLE; ++i)
		openedByCurrentThread[i] = false;
}

// Copy the table from the father process, the whole class (usage, vector, bitmap and bool vector)
void openFilesTable::copyTable(openFilesTable* otherTable)
{
	//usage = otherTable->usage; // Decided to remove this for Nachos_Exit ease
	for(int i=0; i<SIZE_OF_TABLE; ++i)
	{
		openFiles[i] = otherTable->openFiles[i];
		if(otherTable->openFilesMap->Test(i))
			openFilesMap->Mark(i);
		openedByCurrentThread[i] = false;
	}
}
