#include "whodun_sort.h"

#include <assert.h>
#include <iostream>
#include <stdexcept>

#include "whodun_thread.h"
#include "whodun_oshook.h"
#include "whodun_stringext.h"

#define MULTMERGE_BUFF_SIZE 65536

/**A simple queue for sort entities.*/
class SortEntitiyQueue{
public:
	/**
	 * Set up a circular array for sort entities.
	 * @param itemSize The size of each item.
	 * @param arrLength The number of items to hold.
	 */
	SortEntitiyQueue(uintptr_t itemSize, uintptr_t arrLength);
	/**Clean up.*/
	~SortEntitiyQueue();
	/**
	 * Get an item.
	 * @param itemI The item index.
	 * @return The address of the item.
	 */
	char* getItem(uintptr_t itemI);
	/**
	 * Get the number of things that can be pushed back in one go and still be contiguous.
	 * @return The number of things that can be pushed back and have all of them contiguous.
	 */
	uintptr_t pushBackSpan();
	/**
	 * Mark some number of spots as occupied and get the address of the first thing.
	 * @param numPush THe number of spots to occupy.
	 * @return The address of the first item. Note that the items may be discontinuous.
	 */
	char* pushBack(uintptr_t numPush);
	/**
	 * Note how many contiguous items can be popped from the front.
	 * @return The number of contiguous items at the front.
	 */
	uintptr_t popFrontSpan();
	/**
	 * Pop some items from the front and get their memory address (it's still good until YOU do something).
	 * @param numPop The number to pop.
	 * @return The address of the first popped thing. Note that the items may be discontinuous.
	 */
	char* popFront(uintptr_t numPop);
	/**
	 * Perform a lower bound search through the queue.
	 * @param lookFor The entity to search for.
	 * @param compMeth The comparison method.
	 * @param compUni The uniform to pass to the comparison.
	 * @param inRange The range to limit to.
	 * @return The found index.
	 */
	uintptr_t lowerBound(char* lookFor, bool (*compMeth)(void*,void*,void*), void* compUni, std::pair<uintptr_t,uintptr_t> inRange);
	/**
	 * Perform an upper bound search through the queue.
	 * @param lookFor The entity to search for.
	 * @param compMeth The comparison method.
	 * @param compUni The uniform to pass to the comparison.
	 * @param inRange The range to limit to.
	 * @return The found index.
	 */
	uintptr_t upperBound(char* lookFor, bool (*compMeth)(void*,void*,void*), void* compUni, std::pair<uintptr_t,uintptr_t> inRange);
	/**The size of the items.*/
	uintptr_t entSize;
	/**The number of items space is allocated for.*/
	uintptr_t arrAlloc;
	/**The index of the first item.*/
	uintptr_t item0;
	/**The filled length of the array.*/
	uintptr_t arrLen;
	/**The array.*/
	char* arr;
};

SortEntitiyQueue::SortEntitiyQueue(uintptr_t itemSize, uintptr_t arrLength){
	entSize = itemSize;
	arrAlloc = arrLength;
	item0 = 0;
	arrLen = 0;
	arr = (char*)malloc(itemSize*arrLength);
}

SortEntitiyQueue::~SortEntitiyQueue(){
	free(arr);
}

char* SortEntitiyQueue::getItem(uintptr_t itemI){
	return arr + ((item0 + itemI)%arrAlloc)*entSize;
}

uintptr_t SortEntitiyQueue::pushBackSpan(){
	uintptr_t sizeMax = arrAlloc - arrLen;
	uintptr_t loopMax = arrAlloc - ((item0 + arrLen)%arrAlloc);
	return std::min(sizeMax, loopMax);
}

char* SortEntitiyQueue::pushBack(uintptr_t numPush){
	char* toRet = getItem(arrLen);
	arrLen += numPush;
	return toRet;
}

uintptr_t SortEntitiyQueue::popFrontSpan(){
	uintptr_t sizeMax = arrLen;
	uintptr_t loopMax = arrAlloc - item0;
	return std::min(sizeMax, loopMax);
}

char* SortEntitiyQueue::popFront(uintptr_t numPop){
	char* toRet = getItem(0);
	item0 = (item0 + numPop) % arrAlloc;
	arrLen -= numPop;
	return toRet;
}

uintptr_t SortEntitiyQueue::lowerBound(char* lookFor, bool (*compMeth)(void*,void*,void*), void* compUni, std::pair<uintptr_t,uintptr_t> inRange){
	uintptr_t lookLow = inRange.first;
	uintptr_t lookHig = inRange.second;
	while(lookHig - lookLow){
		uintptr_t lookMid = lookLow + ((lookHig - lookLow) >> 1);
		char* compIt = getItem(lookMid);
		if(compMeth(compUni, compIt, lookFor)){
			lookLow = lookMid + 1;
		}
		else{
			lookHig = lookMid;
		}
	}
	return lookLow;
}

uintptr_t SortEntitiyQueue::upperBound(char* lookFor, bool (*compMeth)(void*,void*,void*), void* compUni, std::pair<uintptr_t,uintptr_t> inRange){
	uintptr_t lookLow = inRange.first;
	uintptr_t lookHig = inRange.second;
	while(lookHig - lookLow){
		uintptr_t lookMid = lookLow + ((lookHig - lookLow) >> 1);
		char* compIt = getItem(lookMid);
		if(compMeth(compUni, lookFor, compIt)){
			lookHig = lookMid;
		}
		else{
			lookLow = lookMid + 1;
		}
	}
	return lookLow;
}

/**A node in a merge cluster.*/
class MultimergeNode{
public:
	/**Sorting options.*/
	SortOptions* opts;
	/**The stuff in the buffer is all that is left.*/
	int entExhaust;
	/**The waiting entities.*/
	SortEntitiyQueue entQ;
	/**The pool to use for running.*/
	ThreadPool* usePool;
	/**The ids of the last run threads.*/
	std::vector<uintptr_t> threadIDs;
	/**
	 * Set up an empty buffer.
	 * @param myOpts The sorting options to use.
	 * @param buffSize The number of entries to store in the buffer.
	 * @param dumpPool The pool to use to run things.
	 */
	MultimergeNode(SortOptions* myOpts, uintptr_t buffSize, ThreadPool* dumpPool);
	/**Clean up.*/
	virtual ~MultimergeNode();
	/**Queue up filling this buffer.*/
	virtual void fillBuffer() = 0;
	/**Join on the last fill.*/
	virtual void join();
};

MultimergeNode::MultimergeNode(SortOptions* myOpts, uintptr_t buffSize, ThreadPool* dumpPool) : entQ(myOpts->itemSize, buffSize){
	opts = myOpts;
	entExhaust = 0;
	usePool = dumpPool;
}

MultimergeNode::~MultimergeNode(){}

void MultimergeNode::join(){
	for(uintptr_t i = 0; i<threadIDs.size(); i++){
		usePool->joinTask(threadIDs[i]);
	}
	threadIDs.clear();
}

/**Load from in memory buffers.*/
class MemoryBufferMergeNode : public MultimergeNode{
public:
	/**
	 * Load from in memory buffers.
	 * @param myOpts The sorting options to use.
	 * @param buffSize The number of entries to store in the buffer.
	 * @param dumpPool The pool to use to run things.
	 * @param buffAS The size of the first buffer (entities).
	 * @param buffA The first buffer.
	 */
	MemoryBufferMergeNode(SortOptions* myOpts, uintptr_t buffSize, ThreadPool* dumpPool, uintptr_t buffAS, char* buffA);
	/**Clean up.*/
	~MemoryBufferMergeNode();
	void fillBuffer();
	/**The number of entities left in the first buffer.*/
	uintptr_t numLeftA;
	/**The next entity in the first buffer.*/
	char* nextEntA;
};

MemoryBufferMergeNode::MemoryBufferMergeNode(SortOptions* myOpts, uintptr_t buffSize, ThreadPool* dumpPool, uintptr_t buffAS, char* buffA) : MultimergeNode(myOpts, buffSize, dumpPool){
	numLeftA = buffAS;
	nextEntA = buffA;
}

MemoryBufferMergeNode::~MemoryBufferMergeNode(){}

/**Actually do the fille.*/
void memoryBufferMergeNodeFill(void* myUni){
	MemoryBufferMergeNode* myU = (MemoryBufferMergeNode*)myUni;
	SortEntitiyQueue* entQ = &(myU->entQ);
	while(entQ->arrLen != entQ->arrAlloc){
		uintptr_t toGet = std::min(myU->numLeftA, entQ->pushBackSpan());
		char* dumpTo = entQ->pushBack(toGet);
		uintptr_t toGetB = toGet * myU->opts->itemSize;
		memcpy(dumpTo, myU->nextEntA, toGetB);
		myU->numLeftA -= toGet;
		myU->nextEntA += toGetB;
		if(myU->numLeftA == 0){
			myU->entExhaust = 1;
			break;
		}
	}
}

void MemoryBufferMergeNode::fillBuffer(){
	if(entExhaust){ return; }
	if(entQ.arrLen != entQ.arrAlloc){
		threadIDs.push_back(usePool->addTask(memoryBufferMergeNodeFill, this));
	}
}

/**Reads from a file.*/
class FileSourceMergeNode : public MultimergeNode{
public:
	/**
	 * Merge two merge nodes.
	 * @param myOpts The sorting options to use.
	 * @param buffSize The number of entries to store in the buffer.
	 * @param dumpPool The pool to use to run things.
	 * @param readFrom The file to read from.
	 */
	FileSourceMergeNode(SortOptions* myOpts, uintptr_t buffSize, ThreadPool* dumpPool, InStream* readFrom);
	/**Clean up.*/
	~FileSourceMergeNode();
	void fillBuffer();
	/**The file to read from.*/
	InStream* datSrc;
};

FileSourceMergeNode::FileSourceMergeNode(SortOptions* myOpts, uintptr_t buffSize, ThreadPool* dumpPool, InStream* readFrom) : MultimergeNode(myOpts, buffSize, dumpPool){
	datSrc = readFrom;
}

FileSourceMergeNode::~FileSourceMergeNode(){}

/**Actually do the fill.*/
void fileSourceMergeNodeFill(void* myUni){
	FileSourceMergeNode* myU = (FileSourceMergeNode*)myUni;
	uintptr_t itemSize = myU->opts->itemSize;
	SortEntitiyQueue* entQ = &(myU->entQ);
	while(entQ->arrLen != entQ->arrAlloc){
		uintptr_t toGet = entQ->pushBackSpan();
		char* dumpTo = entQ->pushBack(toGet);
		uintptr_t numRB = myU->datSrc->readBytes(dumpTo, toGet * itemSize);
		if(numRB % itemSize){
			throw std::runtime_error("File truncated.");
		}
		uintptr_t numRE = numRB / itemSize;
		if(numRE != toGet){
			entQ->arrLen -= (toGet - numRE);
			myU->entExhaust = 1;
			break;
		}
	}
}

void FileSourceMergeNode::fillBuffer(){
	if(entExhaust){ return; }
	if(entQ.arrLen != entQ.arrAlloc){
		fileSourceMergeNodeFill(this); //do in the main thread: multithreaded IO can break things
		//threadIDs.push_back(usePool->addTask(fileSourceMergeNodeFill, this));
	}
}

class MergeNodeMergeNode;

/**A uniform for merging subtasks.*/
typedef struct{
	/**The original task.*/
	MergeNodeMergeNode* baseTask;
	/**The range to look at for node A.*/
	std::pair<uintptr_t,uintptr_t> nodeARange;
	/**The range to look at for node B.*/
	std::pair<uintptr_t,uintptr_t> nodeBRange;
	/**The range to put the merges into.*/
	uintptr_t resRange;
} MergeNodeMergeNodeTUni;

/**Merge two other merge nodes.*/
class MergeNodeMergeNode : public MultimergeNode{
public:
	/**
	 * Merge two merge nodes.
	 * @param myOpts The sorting options to use.
	 * @param buffSize The number of entries to store in the buffer.
	 * @param dumpPool The pool to use to run things.
	 * @param numTasks The number of tasks to split into.
	 * @param sourceA The first node to merge.
	 * @param sourceB The second node to merge.
	 */
	MergeNodeMergeNode(SortOptions* myOpts, uintptr_t buffSize, ThreadPool* dumpPool, uintptr_t numTasks, MultimergeNode* sourceA, MultimergeNode* sourceB);
	/**Clean up.*/
	~MergeNodeMergeNode();
	void fillBuffer();
	void join();
	/**The number of tasks to split into.*/
	uintptr_t numTask;
	/**The first node to merge.*/
	MultimergeNode* srcA;
	/**The second node to merge.*/
	MultimergeNode* srcB;
	/**Uniforms for merging.*/
	std::vector<MergeNodeMergeNodeTUni> mrgUnis;
	/**THe number of things in srcA the last thing ate.*/
	uintptr_t lastAteA;
	/**THe number of things in srcB the last thing ate.*/
	uintptr_t lastAteB;
};

MergeNodeMergeNode::MergeNodeMergeNode(SortOptions* myOpts, uintptr_t buffSize, ThreadPool* dumpPool, uintptr_t numTasks, MultimergeNode* sourceA, MultimergeNode* sourceB) : MultimergeNode(myOpts, buffSize, dumpPool){
	srcA = sourceA;
	srcB = sourceB;
	numTask = numTasks;
	mrgUnis.resize(numTask);
	for(uintptr_t i = 0; i<mrgUnis.size(); i++){
		mrgUnis[i].baseTask = this;
	}
}

MergeNodeMergeNode::~MergeNodeMergeNode(){}

/**Actually do a merge.*/
void mergeNodeMergeNodeFill(void* myUni){
	MergeNodeMergeNodeTUni* myBase = (MergeNodeMergeNodeTUni*)myUni;
	MergeNodeMergeNode* myU = myBase->baseTask;
	uintptr_t itemSize = myU->opts->itemSize;
	SortOptions* opts = myU->opts;
	SortEntitiyQueue* entQ = &(myU->entQ);
	MultimergeNode* srcA = myU->srcA;
	SortEntitiyQueue* entA = &(srcA->entQ);
	MultimergeNode* srcB = myU->srcB;
	SortEntitiyQueue* entB = &(srcB->entQ);
	uintptr_t curAI = myBase->nodeARange.first;
	uintptr_t numLeftA = myBase->nodeARange.second - myBase->nodeARange.first;
	uintptr_t curBI = myBase->nodeBRange.first;
	uintptr_t numLeftB = myBase->nodeBRange.second - myBase->nodeBRange.first;
	uintptr_t curMI = myBase->resRange;
	while(numLeftA && numLeftB){
		char* centR = entQ->getItem(curMI);
		char* centA = entA->getItem(curAI);
		char* centB = entB->getItem(curBI);
		if(opts->compMeth(opts->useUni, centA, centB)){
			memcpy(centR, centA, itemSize);
			curAI++;
			numLeftA--;
		}
		else{
			memcpy(centR, centB, itemSize);
			curBI++;
			numLeftB--;
		}
		curMI++;
	}
	while(numLeftA){
		char* centR = entQ->getItem(curMI);
		char* centA = entA->getItem(curAI);
		memcpy(centR, centA, itemSize);
		curAI++;
		numLeftA--;
		curMI++;
	}
	while(numLeftB){
		char* centR = entQ->getItem(curMI);
		char* centB = entB->getItem(curBI);
		memcpy(centR, centB, itemSize);
		curBI++;
		numLeftB--;
		curMI++;
	}
}

void MergeNodeMergeNode::fillBuffer(){
	lastAteA = 0;
	lastAteB = 0;
	if(entExhaust){ return; }
	//wait on the pieces
	srcA->join();
	srcB->join();
	std::pair<uintptr_t,uintptr_t> rangeA(0, srcA->entQ.arrLen);
	std::pair<uintptr_t,uintptr_t> rangeB(0, srcB->entQ.arrLen);
	uintptr_t nextRes = entQ.arrLen;
	//limit the range in each to the last element in the other (if the other has not finished)
	if(!srcA->entExhaust && srcA->entQ.arrLen){
		rangeB.second = srcB->entQ.upperBound(srcA->entQ.getItem(srcA->entQ.arrLen-1), opts->compMeth, opts->useUni, rangeB);
	}
	if(!srcB->entExhaust && srcB->entQ.arrLen){
		rangeA.second = srcA->entQ.upperBound(srcB->entQ.getItem(srcB->entQ.arrLen-1), opts->compMeth, opts->useUni, rangeA);
	}
	//run down the quantiles
	uintptr_t quantLeft = std::min((uintptr_t)(entQ.arrAlloc - entQ.arrLen), (uintptr_t)((rangeA.second - rangeA.first) + (rangeB.second - rangeB.first)));
	entQ.pushBack(quantLeft);
	uintptr_t quantPT = quantLeft / numTask;
	uintptr_t quantET = quantLeft % numTask;
	for(uintptr_t ti = 0; ti<numTask; ti++){
		uintptr_t quantGet = quantPT;	if(quantET){ quantGet++; quantET--; }
		//figure out how many to get
			uintptr_t curFromA;
			uintptr_t curFromB;
			if(rangeA.first != rangeA.second){
				//find the split element
				uintptr_t quantALow = rangeA.first;
				uintptr_t quantAHig = rangeA.second;
				while(quantAHig - quantALow){
					uintptr_t quantAMid = quantALow + ((quantAHig - quantALow)>>1);
					char* midEA = srcA->entQ.getItem(quantAMid);
					uintptr_t midEBI = srcB->entQ.lowerBound(midEA, opts->compMeth, opts->useUni, rangeB);
					uintptr_t midEats = (quantAMid - rangeA.first) + (midEBI - rangeB.first);
					if(midEats < quantGet){
						quantALow = quantAMid + 1;
					}
					else{
						quantAHig = quantAMid;
					}
				}
				curFromA = quantALow - rangeA.first;
				curFromB = quantGet - curFromA; //...is this right?
			}
			else{
				curFromA = 0;
				curFromB = quantGet;
			}
		//add a run
			MergeNodeMergeNodeTUni* curUni = &(mrgUnis[ti]);
			curUni->nodeARange = std::pair<uintptr_t,uintptr_t>(rangeA.first, rangeA.first + curFromA);
			curUni->nodeBRange = std::pair<uintptr_t,uintptr_t>(rangeB.first, rangeB.first + curFromB);
			curUni->resRange = nextRes;
			threadIDs.push_back(usePool->addTask(mergeNodeMergeNodeFill, curUni));
		//prepare for the next
			rangeA.first += curFromA;
			rangeB.first += curFromB;
			nextRes += quantGet;
			lastAteA += curFromA;
			lastAteB += curFromB;
	}
}

void MergeNodeMergeNode::join(){
	//actually join
	for(uintptr_t i = 0; i<threadIDs.size(); i++){
		usePool->joinTask(threadIDs[i]);
	}
	threadIDs.clear();
	//pop from A and B
	srcA->entQ.popFront(lastAteA);
	srcB->entQ.popFront(lastAteB);
	//and note whether it is exhausted
	if((srcA->entQ.arrLen == 0) && (srcB->entQ.arrLen == 0) && srcA->entExhaust && srcB->entExhaust){
		entExhaust = 1;
	}
	lastAteA = 0;
	lastAteB = 0;
}

/**
 * Performs a small mergesort in memory in a single thread..
 * @param numEnts The entries to merge.
 * @param inMem The data to sort.
 * @param opts Sort options.
 * @param tmpStore Temporary storage (same size as inMem).
 * @return Whether the end result is in tmpStore.
 */
int inMemoryMergesortSmall(uintptr_t numEnts, char* inMem, SortOptions* opts, char* tmpStore){
	uintptr_t itemSize = opts->itemSize;
	char* curStore = inMem;
	char* nxtStore = tmpStore;
	uintptr_t size = 1;
	while(size < numEnts){
		uintptr_t curBase = 0;
		while(curBase < numEnts){
			char* curFromA = curStore + (curBase*itemSize);
			char* curTo = nxtStore + (curBase*itemSize);
			uintptr_t nxtBase = curBase + size;
			if(nxtBase > numEnts){
				memcpy(curTo, curFromA, (numEnts - curBase)*itemSize);
				break;
			}
			char* curFromB = curStore + (nxtBase*itemSize);
			uintptr_t finBase = std::min(nxtBase + size, numEnts);
			uintptr_t numLeftA = nxtBase - curBase;
			uintptr_t numLeftB = finBase - nxtBase;
			while(numLeftA && numLeftB){
				if(opts->compMeth(opts->useUni, curFromA, curFromB)){
					memcpy(curTo, curFromA, itemSize);
					curTo += itemSize;
					curFromA += itemSize;
					numLeftA--;
				}
				else{
					memcpy(curTo, curFromB, itemSize);
					curTo += itemSize;
					curFromB += itemSize;
					numLeftB--;
				}
			}
			if(numLeftA){ memcpy(curTo, curFromA, numLeftA*itemSize); }
			if(numLeftB){ memcpy(curTo, curFromB, numLeftB*itemSize); }
			curBase = finBase;
		}
		size = (size << 1);
		char* swapSave = curStore;
		curStore = nxtStore;
		nxtStore = swapSave;
	}
	return curStore != inMem;
}

/**A uniform for sorting a range in memory.*/
class MemoryRangeMergesortUni{
public:
	/**The number of entities.*/
	uintptr_t numEnts;
	/**The memory to sort.*/
	char* inMem;
	/**Options for the sort.*/
	SortOptions* opts;
	/**Temporary storage.*/
	char* tmpStore;
	/**Where teh end result was saved.*/
	int endSaveL;
	/**The "thread" id of this thing.*/
	uintptr_t myID;
};
/**Patch function for threads.*/
void memRangeMergesortTPFunc(void* myU){
	MemoryRangeMergesortUni* argHelp = (MemoryRangeMergesortUni*)myU;
	argHelp->endSaveL = inMemoryMergesortSmall(argHelp->numEnts, argHelp->inMem, argHelp->opts, argHelp->tmpStore);
}

/**
 * Set up the data cascade.
 * @param numP1 The 
 * @param allMerge The nodes to run down.
 */
void mergesortCascadeFill(std::vector<MultimergeNode*>* allMerge){
	//queue
	for(uintptr_t i = 0; i<allMerge->size(); i++){
		(*allMerge)[i]->fillBuffer();
	}
	//wait
	for(uintptr_t i = 0; i<allMerge->size(); i++){
		(*allMerge)[i]->join();
	}
}

void inMemoryMergesort(uintptr_t numEnts, char* inMem, SortOptions* opts){
	uintptr_t itemSize = opts->itemSize;
	std::vector<char> tmpSave; tmpSave.resize(itemSize*numEnts + 1);
	if(opts->numThread == 1){
		if(inMemoryMergesortSmall(numEnts, inMem, opts, &(tmpSave[0]))){
			memcpy(inMem, &(tmpSave[0]), numEnts*itemSize);
		}
	}
	else{
		//make a pool if needbe
			int killPool = 0;
			ThreadPool* usePool = opts->usePool;
			if(usePool == 0){
				killPool = 1;
				usePool = new ThreadPool(opts->numThread);
			}
		//split it amongst threads, sort each piece
			std::vector<MemoryRangeMergesortUni> initSUs;
			initSUs.resize(opts->numThread);
			uintptr_t numPerT = numEnts / opts->numThread;
			uintptr_t numExtT = numEnts % opts->numThread;
			uintptr_t curT0 = 0;
			for(uintptr_t i = 0; i<opts->numThread; i++){
				MemoryRangeMergesortUni* curU = &(initSUs[i]);
				curU->numEnts = numPerT + (i<numExtT);
				curU->inMem = inMem + (curT0*itemSize);
				curU->opts = opts;
				curU->tmpStore = &(tmpSave[curT0*itemSize]);
				curT0 += curU->numEnts;
				curU->myID = usePool->addTask(memRangeMergesortTPFunc, curU);
			}
		//put everything in the temporary storage
			for(uintptr_t i = 0; i<opts->numThread; i++){
				usePool->joinTask(initSUs[i].myID);
				MemoryRangeMergesortUni* curU = &(initSUs[i]);
				if(curU->endSaveL == 0){
					memcpymt(curU->tmpStore, curU->inMem, curU->numEnts*itemSize, opts->numThread, usePool);
				}
			}
		//set up the bulk merge
			uintptr_t cnumTask = 1;
			uintptr_t cbuffSize = MULTMERGE_BUFF_SIZE;
			//initial memory
			std::vector<MultimergeNode*> allMerge;
			for(uintptr_t i = 0; i<opts->numThread; i++){
				MemoryRangeMergesortUni* curU = &(initSUs[i]);
				allMerge.push_back(new MemoryBufferMergeNode(opts, cbuffSize, usePool, curU->numEnts, curU->tmpStore));
			}
			std::vector<MultimergeNode*> curHor;
				curHor.insert(curHor.end(), allMerge.begin(), allMerge.end());
			//build up the merge between nodes
			std::vector<MultimergeNode*> nxtHor;
			while(curHor.size() > 1){
				cnumTask = 2*cnumTask;
				cbuffSize = 2*cbuffSize;
				uintptr_t i = 0;
				uintptr_t curPS = 0;
				while(i < curHor.size()){
					MultimergeNode* nodA = curHor[i];
					i++;
					if(i >= curHor.size()){
						nxtHor.push_back(nodA);
					}
					else{
						MultimergeNode* nodB = curHor[i];
						MultimergeNode* mrgNod = new MergeNodeMergeNode(opts, cbuffSize, usePool, cnumTask, nodA, nodB);
						allMerge.push_back(mrgNod);
						nxtHor.push_back(mrgNod);
						i++;
						curPS++;
					}
				}
				curHor.clear();
				std::swap(curHor, nxtHor);
			}
		//mergeit
			MultimergeNode* endNod = curHor[0];
			char* curDump = inMem;
			while(!(endNod->entExhaust) || endNod->entQ.arrLen){
				while(endNod->entQ.arrLen){
					uintptr_t curNG = endNod->entQ.popFrontSpan();
					char* cpyFrom = endNod->entQ.popFront(curNG);
					memcpymt(curDump, cpyFrom, curNG * itemSize, opts->numThread, usePool);
					curDump += (curNG * itemSize);
				}
				mergesortCascadeFill(&allMerge);
			}
		//clean up
			if(killPool){ delete(usePool); }
			for(uintptr_t i = 0; i<allMerge.size(); i++){ delete(allMerge[i]); }
	}
}

#define SORT_BLOCK_SIZE 0x010000

void outOfMemoryMergesort(InStream* startF, const char* tempFolderName, OutStream* outF, SortOptions* opts){
	//common storage
		uintptr_t itemSize = opts->itemSize;
		uintptr_t maxLoadEnt = opts->maxLoad / itemSize;
			maxLoadEnt = maxLoadEnt / 2;
			if(maxLoadEnt < 2){ maxLoadEnt = 2; }
		std::vector<char> tempFileName;
			tempFileName.insert(tempFileName.end(), tempFolderName, tempFolderName + strlen(tempFolderName));
			tempFileName.insert(tempFileName.end(), pathElementSep, pathElementSep + strlen(pathElementSep));
			uintptr_t fnameBuffI0 = tempFileName.size();
			tempFileName.resize(fnameBuffI0 + 20+4*sizeof(uintmax_t)+4);
		std::vector<char> tempBlokName = tempFileName;
		char* fnameBuff = &(tempFileName[fnameBuffI0]);
		char* fpathBuff = &(tempFileName[0]);
		char* fnameBBuff = &(tempBlokName[fnameBuffI0]);
		char* fpathBBuff = &(tempBlokName[0]);
	//make a pool if needbe
		int killPool = 0;
		ThreadPool* usePool = opts->usePool;
		if(usePool == 0){
			killPool = 1;
			usePool = new ThreadPool(opts->numThread);
		}
	//sort in chunks
		GZipCompressionMethod baseComp;
		SortOptions subOpts = *opts;
			subOpts.usePool = usePool;
		uintptr_t numOutBase = 0;
		uintptr_t numOutFiles = 0;
		char* inMemSortArena = (char*)malloc(itemSize*maxLoadEnt);
		while(true){
			//load
			uintptr_t numRead = startF->readBytes(inMemSortArena, maxLoadEnt*itemSize);
			uintptr_t numLoadEnt = numRead / itemSize;
			if(numLoadEnt == 0){ break; }
			//sort
			inMemoryMergesort(numLoadEnt, inMemSortArena, &subOpts);
			//dump to new file
			sprintf(fnameBuff, "%s%ju", "sortspl_", (uintmax_t)numOutFiles);
			sprintf(fnameBBuff, "%s%ju", "sortblk_", (uintmax_t)numOutFiles);
			MultithreadBlockCompOutStream curDumpOut(0, SORT_BLOCK_SIZE, fpathBuff, fpathBBuff, &baseComp, opts->numThread, usePool);
			curDumpOut.writeBytes(inMemSortArena, numRead);
			numOutFiles++;
		}
		free(inMemSortArena);
	//merge
		uintptr_t mergeThread = std::max(opts->numThread, (uintptr_t)64);
		while(numOutBase != numOutFiles){
			int lastLine = (numOutFiles - numOutBase) < mergeThread;
			uintptr_t nxtOutBase = numOutFiles;
			uintptr_t nxtOutFiles = numOutFiles;
			uintptr_t baseI = numOutBase;
			while(baseI < numOutFiles){
				uintptr_t nextI = std::min(baseI + mergeThread, numOutFiles);
				//open up the current crop of files
				uintptr_t cnumTask = 1;
				uintptr_t cbuffSize = MULTMERGE_BUFF_SIZE;
				//std::vector<GZipCompressionMethod> subComps; subComps.resize(nextI - baseI);
				std::vector<InStream*> saveFiles;
				std::vector<MultimergeNode*> allMerge;
				for(uintptr_t i = baseI; i<nextI; i++){
					sprintf(fnameBuff, "%s%ju", "sortspl_", (uintmax_t)i);
					sprintf(fnameBBuff, "%s%ju", "sortblk_", (uintmax_t)i);
					InStream* curSrcS = new MultithreadBlockCompInStream(fpathBuff, fpathBBuff, &baseComp, opts->numThread, usePool);
					MultimergeNode* curMrg = new FileSourceMergeNode(opts, cbuffSize, usePool, curSrcS);
					saveFiles.push_back(curSrcS);
					allMerge.push_back(curMrg);
				}
				std::vector<MultimergeNode*> curHor;
					curHor.insert(curHor.end(), allMerge.begin(), allMerge.end());
				//build up the merge between nodes
				std::vector<MultimergeNode*> nxtHor;
				while(curHor.size() > 1){
					cnumTask = 2*cnumTask;
					cbuffSize = 2*cbuffSize;
					uintptr_t i = 0;
					uintptr_t curPS = 0;
					while(i < curHor.size()){
						MultimergeNode* nodA = curHor[i];
						i++;
						if(i >= curHor.size()){
							nxtHor.push_back(nodA);
						}
						else{
							MultimergeNode* nodB = curHor[i];
							MultimergeNode* mrgNod = new MergeNodeMergeNode(opts, cbuffSize, usePool, cnumTask, nodA, nodB);
							allMerge.push_back(mrgNod);
							nxtHor.push_back(mrgNod);
							i++;
							curPS++;
						}
					}
					curHor.clear();
					std::swap(curHor, nxtHor);
				}
				//figure out where to output
				int killOut;
				OutStream* curOut;
				if(lastLine){
					killOut = 0;
					curOut = outF;
				}
				else{
					killOut = 1;
					sprintf(fnameBuff, "%s%ju", "sortspl_", (uintmax_t)nxtOutFiles);
					sprintf(fnameBBuff, "%s%ju", "sortblk_", (uintmax_t)nxtOutFiles);
					curOut = new MultithreadBlockCompOutStream(0, SORT_BLOCK_SIZE, fpathBuff, fpathBBuff, &baseComp, opts->numThread, usePool);
					nxtOutFiles++;
				}
				//output
				std::vector<uintptr_t> threadIDTmp;
				MultimergeNode* endNod = curHor[0];
				std::vector<char> tmpDump;
				while(!(endNod->entExhaust) || endNod->entQ.arrLen){
					while(endNod->entQ.arrLen){
						uintptr_t curNG = endNod->entQ.popFrontSpan();
						char* cpyFrom = endNod->entQ.popFront(curNG);
						curOut->writeBytes(cpyFrom, curNG*itemSize);
					}
					mergesortCascadeFill(&allMerge);
				}
				//clean up and prepare for the next round
				if(killOut){ delete(curOut); }
				for(uintptr_t i = 0; i<allMerge.size(); i++){ delete(allMerge[i]); }
				for(uintptr_t i = 0; i<saveFiles.size(); i++){ delete(saveFiles[i]); }
				baseI = nextI;
			}
			//kill the old files and prepare for the next round
			for(uintptr_t i = numOutBase; i<numOutFiles; i++){
				sprintf(fnameBuff, "%s%ju", "sortspl_", (uintmax_t)i);
				sprintf(fnameBBuff, "%s%ju", "sortblk_", (uintmax_t)i);
				killFile(fpathBuff);
				killFile(fpathBBuff);
			}
			numOutBase = nxtOutBase;
			numOutFiles = nxtOutFiles;
		}
	//clean up
		if(killPool){ delete(usePool); }
}

PreSortMultithreadPipe::PreSortMultithreadPipe(uintptr_t numSave) : drainCon(&datMut), fillCon(&datMut){
	maxBuff = numSave;
	endWrite = false;
	usePool = 0;
	totDat = 0;
}

PreSortMultithreadPipe::PreSortMultithreadPipe(uintptr_t numSave, ThreadPool* bigPool) : drainCon(&datMut), fillCon(&datMut){
	maxBuff = numSave;
	endWrite = false;
	usePool = bigPool;
	liveReadTasks.resize(bigPool->numThr);
	totDat = 0;
}

PreSortMultithreadPipe::~PreSortMultithreadPipe(){
	for(uintptr_t i = 0; i<datBuff.size(); i++){
		if(datBuff[i].buff){ free(datBuff[i].buff); }
	}
	for(uintptr_t i = 0; i<allocBuff.size(); i++){
		if(allocBuff[i].buff){ free(allocBuff[i].buff); }
	}
}

void PreSortMultithreadPipe::writeBytes(const char* toW, uintptr_t numW){
	//get a buffer
		PreSortMultithreadPipeBufferEntry myBuff = {0,0,0,0};
		datMut.lock();
			//wait for space to open up
			while(totDat >= maxBuff){ drainCon.wait(); }
			if(endWrite){ throw std::runtime_error("Cannot write to a closed sort pipe."); }
			totDat += numW;
			//get a buffer, if ready
			if(allocBuff.size()){
				myBuff = allocBuff[0];
				allocBuff.pop_front();
			}
		datMut.unlock();
	//if necessary, allocate a buffer
		if(myBuff.buffAlloc < numW){
			if(myBuff.buff){ free(myBuff.buff); }
			myBuff.buff = (char*)malloc(numW);
			myBuff.buffAlloc = numW;
		}
		myBuff.buffSize = numW;
		myBuff.nextI = 0;
	//copy
		memcpy(myBuff.buff, toW, numW);
	//and spank it on the queue
		datMut.lock();
			datBuff.push_back(myBuff);
			fillCon.signal();
		datMut.unlock();
}

void PreSortMultithreadPipe::closeWrite(){
	datMut.lock();
		endWrite = true;
		drainCon.broadcast();
		fillCon.signal();
	datMut.unlock();
}

int PreSortMultithreadPipe::readByte(){
	int toRet = -1;
	datMut.lock();
		head_git_gud_loop:
		while(!endWrite && (datBuff.size()==0)){ fillCon.wait(); }
		if(datBuff.size()){
			PreSortMultithreadPipeBufferEntry curEnt = datBuff[0];
			if(curEnt.nextI >= curEnt.buffSize){
				allocBuff.push_back(curEnt);
				datBuff.pop_front();
				goto head_git_gud_loop;
			}
			toRet = curEnt.buff[curEnt.nextI];
			curEnt.nextI++;
			totDat--;
			drainCon.signal();
		}
	datMut.unlock();
	return toRet;
}

/**A uniform for a running task.*/
class PreSortMultithreadPipeTaskEntry{
public:
	/**Set up an empty entry.*/
	PreSortMultithreadPipeTaskEntry();
	/**The buffer to copy from.*/
	PreSortMultithreadPipeBufferEntry copyFrom;
	/**Whether the entry needs to be thrown on the alloc stack (ie it was exhausted); the alternative is to update the queue state.*/
	int wasExhaust;
	/**The place to copy to.*/
	char* copyTo;
	/**The number to copy.*/
	uintptr_t numCopy;
	/**The task identifier.*/
	uintptr_t threadID;
};

PreSortMultithreadPipeTaskEntry::PreSortMultithreadPipeTaskEntry(){}

/**Actually copy read data.*/
void runPreSortMultithreadPipeReadTask(void* uniPass){
	PreSortMultithreadPipeTaskEntry* myUni = (PreSortMultithreadPipeTaskEntry*)uniPass;
	memcpy(myUni->copyTo, myUni->copyFrom.buff + myUni->copyFrom.nextI, myUni->numCopy);
}

uintptr_t PreSortMultithreadPipe::readBytes(char* toR, uintptr_t numR){
	char* nextR = toR;
	uintptr_t leftR = numR;
	if(usePool){
		uintptr_t nextUniSI = 0;
		uintptr_t nextUniEI = 0;
		#define PRESORTMTPIPE_READ_DRAINRUN \
			datMut.unlock();\
			usePool->joinTask(liveReadTasks[nextUniEI].threadID);\
			datMut.lock();\
			if(liveReadTasks[nextUniEI].wasExhaust){\
				allocBuff.push_back(liveReadTasks[nextUniEI].copyFrom);\
			}\
			else{\
				datBuff[0].nextI += liveReadTasks[nextUniEI].numCopy;\
			}\
			nextUniEI = (nextUniEI + 1) % liveReadTasks.size();
		datMut.lock();
		while(leftR){
			PreSortMultithreadPipeBufferEntry curEnt;
			//get an entry
				while(!endWrite && (datBuff.size()==0)){ fillCon.wait(); }
				if(datBuff.size() == 0){
					break;
				}
				curEnt = datBuff[0];
			//figure out how much to copy
				uintptr_t numCopy = std::min(leftR, curEnt.buffSize - curEnt.nextI);
				totDat -= numCopy;
				drainCon.signal();
			//pop if hit the end
				int willExhaust = (numCopy == (curEnt.buffSize - curEnt.nextI));
				if(willExhaust){
					datBuff.pop_front();
				}
			//start a new run
				PreSortMultithreadPipeTaskEntry* nextUni = &(liveReadTasks[nextUniSI]);
				nextUni->copyFrom = curEnt;
				nextUni->wasExhaust = willExhaust;
				nextUni->copyTo = nextR;
				nextUni->numCopy = numCopy;
				nextUni->threadID = usePool->addTask(runPreSortMultithreadPipeReadTask, nextUni);
				nextR += numCopy;
				leftR -= numCopy;
				nextUniSI = (nextUniSI + 1) % liveReadTasks.size();
			//if loop, drain a run
			if(nextUniSI == nextUniEI){
				PRESORTMTPIPE_READ_DRAINRUN
			}
		}
		//drain the remainder
		while(nextUniEI != nextUniSI){
			PRESORTMTPIPE_READ_DRAINRUN
		}
		datMut.unlock();
		return numR - leftR;
	}
	else{
		//reading is single threaded
		datMut.lock();
		while(leftR){
			PreSortMultithreadPipeBufferEntry curEnt;
			//get an entry
				while(!endWrite && (datBuff.size()==0)){ fillCon.wait(); }
				if(datBuff.size() == 0){
					datMut.unlock();
					return numR - leftR;
				}
				curEnt = datBuff[0];
				if(curEnt.nextI >= curEnt.buffSize){
					allocBuff.push_back(curEnt);
					datBuff.pop_front();
					continue;
				}
			//figure out how much to copy
				uintptr_t numCopy = std::min(leftR, curEnt.buffSize - curEnt.nextI);
				totDat -= numCopy;
				drainCon.signal();
			//copy to the target
				datMut.unlock();
					memcpy(nextR, curEnt.buff + curEnt.nextI, numCopy);
				datMut.lock();
			//update entity
				datBuff[0].nextI += numCopy;
				nextR += numCopy;
				leftR -= numCopy;
		}
		datMut.unlock();
		return numR;
	}
}

char* whodunSortLowerBound(uintptr_t numEnts, char* inMem, char* lookFor, SortOptions* opts){
	uintptr_t itemSize = opts->itemSize;
	char* first = inMem;
	uintptr_t count = numEnts;
	while(count){
		uintptr_t step = count / 2;
		char* it = first + (step * itemSize);
		if(opts->compMeth(opts->useUni, it, lookFor)){
			first = it + itemSize;
			count = count - (step + 1);
		}
		else{
			count = step;
		}
	}
	return first;
}

char* whodunSortUpperBound(uintptr_t numEnts, char* inMem, char* lookFor, SortOptions* opts){
	uintptr_t itemSize = opts->itemSize;
	char* first = inMem;
	uintptr_t count = numEnts;
	while(count){
		uintptr_t step = count / 2;
		char* it = first + (step * itemSize);
		if(!(opts->compMeth(opts->useUni, lookFor, it))){
			first = it + itemSize;
			count = count - (step + 1);
		}
		else{
			count = step;
		}
	}
	return first;
}



