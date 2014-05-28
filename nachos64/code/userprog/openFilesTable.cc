#include "openFilesTable.h"

// Initialize
openFilesTable::openFilesTable()
	: usage(1)
{
	openFiles = new long[SIZE_OF_TABLE];
	openFilesMap = new BitMap(SIZE_OF_TABLE*sizeof(long));
	openFilesMap->Mark(0); //stdin
	openFilesMap->Mark(1); //stdout
	openFilesMap->Mark(2); //stderr
	openedByCurrentThread = new bool[SIZE_OF_TABLE];
}

// De-allocate, ALSO close all the files that are currently open and associated to this thread
openFilesTable::~openFilesTable()
{
	// Check if there are open files, close them using the system call
	//for(unsigned int pos=0; pos<SIZE_OF_TABLE; ++pos)
	//{
	//	if(openFilesMap->Test(pos))
	//		Close(openFiles[pos]);
	//}
	delete openFiles;
	delete openFilesMap;
	delete openedByCurrentThread;
}

// Register the file ID in the table
int openFilesTable::Open(long UnixFileID)
{
	int nextFreePos = openFilesMap->Find();
	openFiles[nextFreePos] = UnixFileID;
	//machine->WriteRegister(2, nextFreePos); //receive the Unix fileID and return the Nachos fileID
	return nextFreePos;
}

// Unregister the file ID from the table
int openFilesTable::Close(int NachosFileID)
{
	openFilesMap->Clear(NachosFileID);
	return 0;
}

// Returns wether the file pointed by the NachosFileID is open or not
bool openFilesTable::isOpen(int NachosFileID)
{
	return openFilesMap->Test(NachosFileID);
}

// Returns the Unix file ID of the file using the Nachos file ID
long openFilesTable::getUnixFileID(int NachosFileID)
{
	return openFiles[NachosFileID];
}

// Simply add 1 to the counter of threads
void openFilesTable::addThread()		// If a user thread is using this table, add it
{
	++usage;
}

// Simply substract 1 to the counter of threads
void openFilesTable::delThread()		// If a user thread is using this table, delete it
{
	--usage;
}

// Print stats and info, the contents of the table
void openFilesTable::Print()
{
	for(unsigned int pos=0; pos<SIZE_OF_TABLE; ++pos)
	{
		printf("%d\t%d\t%li", pos, openFilesMap->Test(pos), openFiles[pos]);
	}
}

// Initialize the bool table that indicates wether each file has been opened by the current thread or not
void openFilesTable::initializeBoolTable()
{
	for(unsigned int i=0; i<SIZE_OF_TABLE; ++i)
		openedByCurrentThread[i] = false;
}
