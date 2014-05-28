#ifndef OPEN_FILES_TABLE_H
#define OPEN_FILES_TABLE_H

#include "bitmap.h"
//#include "syscall.h"
//#define bool int
//#define true 1
//#define false 0
#define SIZE_OF_TABLE 128

class openFilesTable
{
  public:
    openFilesTable();       				// Initialize
    ~openFilesTable();      				// De-allocate
    
    int Open(long UnixFileID); 				// Register the file ID
    int Close(int NachosFileID);    		// Unregister the file ID
    inline int getUsage(){return usage;};	// Return the number of threads using the table
    long getUnixFileID(int NachosFileID);	// Get the file ID
    bool isOpen(int NachosFileID);			// Check by ID if it is open
    void addThread();						// If a user thread is using this table, add it
    void delThread();						// If a user thread is using this table, delete it
    void Print();               			// Print contents of table
    void initializeBoolTable();				// Initialize the bool table
    void copyTable(openFilesTable* otherTable);		//Copy table of father process
    
  private:
    long* openFiles;						// A vector with user opened Unix file IDs. Uses long in order to work with semaphore pointers
    int usage;								// How many threads are using this table
    BitMap* openFilesMap;					// A bitmap to control our vector
    bool* openedByCurrentThread;			// Bool table that indicates wether each file has been opened by the current thread or not
};

#endif //OPEN_FILES_TABLE_H
