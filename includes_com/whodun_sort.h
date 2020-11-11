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
	bool (*compMeth)(void* itemA,void* itemB);
	/**The size of each item.*/
	uintptr_t itemSize;
	/**The maximum number of bytes to load at any given time.*/
	uintptr_t maxLoad;
	/**The number of threads this should use.*/
	uintptr_t numThread;
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
 * @param startF THe starting file.
 * @param tempFolderName The folder to put temporary files in.
 * @param outF The file to write to.
 * @param opts The options for the sort.
 */
void outOfMemoryMergesort(InStream* startF, const char* tempFolderName, OutStream* outF, SortOptions* opts);

#endif
