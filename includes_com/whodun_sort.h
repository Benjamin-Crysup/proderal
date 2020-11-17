#ifndef WHODUN_SORT_H
#define WHODUN_SORT_H 1

#include <deque>

#include "whodun_oshelp.h"
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

/**An pipe for collecting data from multiple threads that is intended to be sorted (i.e. order doesn't REALLY matter).*/
class PreSortMultithreadPipe : public InStream{
public:
	/**
	 * Set up the stream.
	 * @param numSave The maximum number of bytes to save in the buffer.
	 */
	PreSortMultithreadPipe(uintptr_t numSave);
	/**
	 * Add a chunk of bytes.
	 * @param toW The data to write.
	 * @param numW The number of bytes.
	 */
	void writeBytes(const char* toW, uintptr_t numW);
	/**Mark that no more bytes will be written.*/
	void closeWrite();
	
	int readByte();
	uintptr_t readBytes(char* toR, uintptr_t numR);
	
	/**Whether all input has been sent.*/
	bool endWrite;
	/**Maximum number of bytes to buffer.*/
	uintptr_t maxBuff;
	/**A place to temporarily store data.*/
	std::deque<char> datBuff;
	/**Used to synchronize writes: know that this will be held during waits.*/
	OSMutex writeMut;
	/**Used to synchronize reads.*/
	OSMutex drainMut;
	/**Used to wait for a clear buffer.*/
	OSCondition drainCon;
};

#endif
