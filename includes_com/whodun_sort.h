#ifndef WHODUN_SORT_H
#define WHODUN_SORT_H 1

#include "whodun_compress.h"

/**Some options for sorting.*/
class SortOptions{
public:
	/**
	 * The method to use for comparison.
	 * @param itemA The first item.
	 * @param itemB The second item.
	 * @return Whther itemA should come before itemB.
	 */
	virtual bool compMeth(void* itemA,void* itemB) = 0;
	/**The size of each item.*/
	uintptr_t itemSize;
	/**The maximum number of bytes to load at any given time.*/
	uintptr_t maxLoad;
	/**The number of threads this should use.*/
	uintptr_t numThread;
	/**The compression method used on the initial input.*/
	CompressionMethod* inCom;
	/**The compression method to use while working.*/
	CompressionMethod* workCom;
	/**The compression method to use on final output.*/
	CompressionMethod* outCom;
};

/**
 * This will performa a mergesort on some data in memory.
 * @param numEnts The number of items to sort.
 * @param inMem The items to sort.
 * @param opts The options for the sort.
 */
void inMemoryMergesort(uintptr_t numEnts, char* inMem, SortOptions* opts);

/**
 * This will perform a mergesort on some data out of memory.
 * @param startFName THe starting file name. Gzip file containing some number of entries.
 * @param tempFolderName The folder to put temporary files in.
 * @param outFileName THe name of the file to produce (also compressed).
 * @param opts The options for the sort.
 * @return Whether there was an exception.
 */
int outOfMemoryMergesort(const char* startFName, const char* tempFolderName, const char* outFileName, SortOptions* opts);

/**
 * This will perform a mergesort on some data out of memory, using multiple threads.
 * @param startFName THe starting file name. Gzip file containing some number of entries.
 * @param tempFolderName The folder to put temporary files in.
 * @param outFileName THe name of the file to produce (also compressed).
 * @param opts The options for the sort.
 * @return Whether there was an exception.
 */
int outOfMemoryMultithreadMergesort(const char* startFName, const char* tempFolderName, const char* outFileName, SortOptions* opts);

#endif
