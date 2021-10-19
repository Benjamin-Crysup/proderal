#ifndef WHODUN_SORT_H
#define WHODUN_SORT_H 1

#include <deque>

#include "whodun_nmcy.h"
#include "whodun_oshelp.h"
#include "whodun_compress.h"

/**Some options for sorting.*/
class SortOptions{
public:
	/**
	 * The method to use for comparison (i.e. less than).
	 * @param unif A uniform for the comparison.
	 * @param itemA The first item.
	 * @param itemB The second item.
	 * @return Whther itemA should come before itemB (false if equal).
	 */
	bool (*compMeth)(void* unif, void* itemA,void* itemB);
	/**The size of each item.*/
	uintptr_t itemSize;
	/**The maximum number of bytes to load at any given time.*/
	uintptr_t maxLoad;
	/**The number of threads this should use.*/
	uintptr_t numThread;
	/**The uniform to use.*/
	void* useUni;
	/**The thread pool to use, if any.*/
	ThreadPool* usePool;
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

/**Bytes that have been "written" but not yet read from the pipe.*/
typedef struct{
	/**The size of the data in the buffer.*/
	uintptr_t buffSize;
	/**The next index to spit out.*/
	uintptr_t nextI;
	/**The size of the allocation.*/
	uintptr_t buffAlloc;
	/**The buffer itself.*/
	char* buff;
} PreSortMultithreadPipeBufferEntry;

class PreSortMultithreadPipeTaskEntry;

/**An pipe for collecting data from multiple threads that is intended to be sorted (i.e. input order doesn't REALLY matter).*/
class PreSortMultithreadPipe : public InStream{
public:
	/**
	 * Set up the stream.
	 * @param numSave The maximum number of bytes to save in the buffer.
	 */
	PreSortMultithreadPipe(uintptr_t numSave);
	/**
	 * Set up the stream.
	 * @param numSave The maximum number of bytes to save in the buffer.
	 * @param bigPool The pool to use.
	 */
	PreSortMultithreadPipe(uintptr_t numSave, ThreadPool* bigPool);
	/**Clean up.*/
	~PreSortMultithreadPipe();
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
	/**Queued data.*/
	std::deque< PreSortMultithreadPipeBufferEntry > datBuff;
	/**Allocated buffers.*/
	std::deque< PreSortMultithreadPipeBufferEntry > allocBuff;
	/**The amount of data waiting to fly.*/
	uintptr_t totDat;
	/**Used to synchronize writes: know that this will be held during waits.*/
	OSMutex datMut;
	/**Used to wait for a clear buffer.*/
	OSCondition drainCon;
	/**Used to wait for a full buffer.*/
	OSCondition fillCon;
	/**The thread pool to use, if any.*/
	ThreadPool* usePool;
	/**The currently running tasks.*/
	std::vector<PreSortMultithreadPipeTaskEntry> liveReadTasks;
};

/**
 * Perform a lower_bound-type search.
 * @param numEnts The number of items to search through.
 * @param inMem The items to search through.
 * @param lookFor The thing to look for.
 * @param opts The options for the sort.
 * @return The lower_bound insertion index.
 */
char* whodunSortLowerBound(uintptr_t numEnts, char* inMem, char* lookFor, SortOptions* opts);

/**
 * Perform a upper_bound-type search.
 * @param numEnts The number of items to search through.
 * @param inMem The items to search through.
 * @param lookFor The thing to look for.
 * @param opts The options for the sort.
 * @return The upper_bound insertion index.
 */
char* whodunSortUpperBound(uintptr_t numEnts, char* inMem, char* lookFor, SortOptions* opts);

#endif
