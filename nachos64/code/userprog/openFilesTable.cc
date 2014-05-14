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
}

// Register the file handle
int openFilesTable::Open(int UnixHandle)
{
	int nextFreePos = openFilesMap->Find();
	openFiles[nextFreePos] = UnixHandle;
	//machine->WriteRegister(2, nextFreePos); //receive the Unix fileID and return the Nachos fileID
	return nextFreePos;
}

int openFilesTable::Close(int NachosHandle)      // Unregister the file handle
{
	openFilesMap->Clear(NachosHandle);
	return 0;
}

bool openFilesTable::isOpened(int NachosHandle)
{
	return openFilesMap->Test(NachosHandle);
}

int openFilesTable::getUnixHandle(int NachosHandle)
{
	return openFiles[NachosHandle];
}

void openFilesTable::addThread()		// If a user thread is using this table, add it
{
	++usage;
}

void openFilesTable::delThread()		// If a user thread is using this table, delete it
{
	--usage;
}

void openFilesTable::Print()               // Print contents
{
	for(unsigned int pos=0; pos<SIZE_OF_TABLE; ++pos)
	{
		printf("%d\t%d\t%d", pos, openFilesMap->Test(pos), openFiles[pos]);
	}
}
