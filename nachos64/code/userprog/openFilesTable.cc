#include "openFilesTable.h"
    
/*
    int* openFiles		// A vector with user opened files
    BitMap* openFilesMap	// A bitmap to control our vector
    int usage			// How many threads are using this table
*/

// Initialize
openFilesTable::openFilesTable()
	: usage(1)
{
	openFiles = new int[SIZE_OF_TABLE];
	openFilesMap = new BitMap(SIZE_OF_TABLE*sizeof(int));
	openFilesMap->Mark(0); //stdin
	openFilesMap->Mark(1); //stdout
	openFilesMap->Mark(2); //stderr
	openedByCurrentThread = new bool[SIZE_OF_TABLE];
}

// De-allocate, ALSO close all the files that are currently open and associated to this thread
openFilesTable::~openFilesTable()
{
	//for(unsigned int pos=0; pos<SIZE_OF_TABLE; ++pos) //Check if there are opened files, close them using the system call
	//{
	//	if(openFilesMap->Test(pos))
	//		Close(openFiles[pos]);
	//}
	delete openFiles;
	delete openFilesMap;
	delete openedByCurrentThread;
}

// Register the file handle
int openFilesTable::Open(int UnixHandle)
{
	int nextFreePos = openFilesMap->Find();
	openFiles[nextFreePos] = UnixHandle;
	//machine->WriteRegister(2, nextFreePos); //receive the Unix fileID and return the Nachos fileID
	return nextFreePos;
}

// Remove from the table
int openFilesTable::Close(int NachosHandle)      // Unregister the file handle
{
	openFilesMap->Clear(NachosHandle);
	return 0;
}

// Returns wether the file pointed by the NachosHandle is opened or not
bool openFilesTable::isOpened(int NachosHandle)
{
	return openFilesMap->Test(NachosHandle);
}

// Returns the Unix file ID of the file using the Nachos file ID
int openFilesTable::getUnixHandle(int NachosHandle)
{
	return openFiles[NachosHandle];
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

// Print stats and info
void openFilesTable::Print()               // Print contents
{
	for(unsigned int pos=0; pos<SIZE_OF_TABLE; ++pos)
	{
		printf("%d\t%d\t%d", pos, openFilesMap->Test(pos), openFiles[pos]);
	}
}

// Initialize the bool table that indicates wether the file has been opened by the current thread or not
void openFilesTable::initializeBoolTable()
{
	for(unsigned int i=0; i<SIZE_OF_TABLE; ++i)
		openedByCurrentThread[i] = false;
}
