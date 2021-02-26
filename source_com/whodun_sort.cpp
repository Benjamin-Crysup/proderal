#include "whodun_sort.h"

#include <deque>
#include <assert.h>
#include <iostream>
#include <stdexcept>

#include "whodun_thread.h"
#include "whodun_oshook.h"
#include "whodun_stringext.h"

#define MULTMERGE_BUFF_SIZE 65536

/**A node in a merge cluster.*/
class MultimergeNode{
public:
	/**Sorting options.*/
	SortOptions* opts;
	/**The stuff in the buffer is all that is left.*/
	int entExhaust;
	/**The waiting entities.*/
	std::deque<char> entQ;
	/**The maximum size for the queue (bytes).*/
	uintptr_t queueSize;
	/**
	 * Set up an empty buffer.
	 * @param myOpts The sorting options to use.
	 * @param buffSize The number of entries to store in the buffer.
	 */
	MultimergeNode(SortOptions* myOpts, uintptr_t buffSize);
	/**Clean up.*/
	virtual ~MultimergeNode();
	/**Fill this buffer.*/
	virtual void fillBuffer() = 0;
};

MultimergeNode::MultimergeNode(SortOptions* myOpts, uintptr_t buffSize){
	opts = myOpts;
	entExhaust = 0;
	queueSize = opts->itemSize * buffSize;
}

MultimergeNode::~MultimergeNode(){}

/**Thread function: fill a merge node.*/
void mergeSortFillMergeNode(void* myU){
	MultimergeNode* nodeFill = (MultimergeNode*)myU;
	nodeFill->fillBuffer();
}

/**Merge from in memory buffers.*/
class MemoryBufferMergeNode : public MultimergeNode{
public:
	/**
	 * Merge two memory buffers.
	 * @param myOpts The sorting options to use.
	 * @param buffSize The number of entries to store in the buffer.
	 * @param buffAS The size of the first buffer.
	 * @param buffA The first buffer.
	 */
	MemoryBufferMergeNode(SortOptions* myOpts, uintptr_t buffSize, uintptr_t buffAS, char* buffA);
	/**Clean up.*/
	~MemoryBufferMergeNode();
	/**Fill this buffer.*/
	void fillBuffer();
	/**The number of entities left in the first buffer.*/
	uintptr_t numLeftA;
	/**The next entity in the first buffer.*/
	char* nextEntA;
};

MemoryBufferMergeNode::MemoryBufferMergeNode(SortOptions* myOpts, uintptr_t buffSize, uintptr_t buffAS, char* buffA) : MultimergeNode(myOpts, buffSize){
	numLeftA = buffAS * myOpts->itemSize;
	nextEntA = buffA;
}

MemoryBufferMergeNode::~MemoryBufferMergeNode(){}

void MemoryBufferMergeNode::fillBuffer(){
	uintptr_t numGet = queueSize - entQ.size();
		numGet = std::min(numGet, numLeftA);
	entQ.insert(entQ.end(), nextEntA, nextEntA + numGet);
	nextEntA += numGet;
	numLeftA -= numGet;
	if(numLeftA == 0){
		entExhaust = 1;
	}
}

/**Merge two other merge nodes.*/
class MergeNodeMergeNode : public MultimergeNode{
public:
	/**
	 * Merge two merge nodes.
	 * @param myOpts The sorting options to use.
	 * @param buffSize The number of entries to store in the buffer.
	 * @param sourceA The first node to merge.
	 * @param sourceB The second node to merge.
	 */
	MergeNodeMergeNode(SortOptions* myOpts, uintptr_t buffSize, MultimergeNode* sourceA, MultimergeNode* sourceB);
	/**Clean up.*/
	~MergeNodeMergeNode();
	/**Fill this buffer.*/
	void fillBuffer();
	/**The first node to merge.*/
	MultimergeNode* srcA;
	/**The second node to merge.*/
	MultimergeNode* srcB;
	/**The next entity from A.*/
	std::vector<char> tmpSaveA;
	/**The next entity from B.*/
	std::vector<char> tmpSaveB;
};

MergeNodeMergeNode::MergeNodeMergeNode(SortOptions* myOpts, uintptr_t buffSize, MultimergeNode* sourceA, MultimergeNode* sourceB) : MultimergeNode(myOpts, buffSize){
	srcA = sourceA;
	srcB = sourceB;
}

MergeNodeMergeNode::~MergeNodeMergeNode(){
}

void MergeNodeMergeNode::fillBuffer(){
	uintptr_t itemSize = opts->itemSize;
	while(entQ.size() < queueSize){
		//get entities for A and B
		if(tmpSaveA.size() == 0){
			if(srcA->entQ.size()){
				tmpSaveA.insert(tmpSaveA.end(), srcA->entQ.begin(), srcA->entQ.begin() + itemSize);
				srcA->entQ.erase(srcA->entQ.begin(), srcA->entQ.begin() + itemSize);
			}
			else{
				if(srcA->entExhaust == 0){ break; }
			}
		}
		if(tmpSaveB.size() == 0){
			if(srcB->entQ.size()){
				tmpSaveB.insert(tmpSaveB.end(), srcB->entQ.begin(), srcB->entQ.begin() + itemSize);
				srcB->entQ.erase(srcB->entQ.begin(), srcB->entQ.begin() + itemSize);
			}
			else{
				if(srcB->entExhaust == 0){ break; }
			}
		}
		//merge
		if(tmpSaveA.size()){
			if(tmpSaveB.size()){
				if(opts->compMeth(opts->useUni, &(tmpSaveA[0]), &(tmpSaveB[0]))){
					entQ.insert(entQ.end(), tmpSaveA.begin(), tmpSaveA.end());
					tmpSaveA.clear();
				}
				else{
					entQ.insert(entQ.end(), tmpSaveB.begin(), tmpSaveB.end());
					tmpSaveB.clear();
				}
			}
			else{
				entQ.insert(entQ.end(), tmpSaveA.begin(), tmpSaveA.end());
				tmpSaveA.clear();
			}
		}
		else{
			if(tmpSaveB.size()){
				entQ.insert(entQ.end(), tmpSaveB.begin(), tmpSaveB.end());
				tmpSaveB.clear();
			}
			else{
				entExhaust = 1;
				break;
			}
		}
	}
}

/**Reads from a file.*/
class FileSourceMergeNode : public MultimergeNode{
public:
	/**
	 * Merge two merge nodes.
	 * @param myOpts The sorting options to use.
	 * @param buffSize The number of entries to store in the buffer.
	 * @param readFrom The file to read from.
	 */
	FileSourceMergeNode(SortOptions* myOpts, uintptr_t buffSize, InStream* readFrom);
	/**Clean up.*/
	~FileSourceMergeNode();
	/**Fill this buffer.*/
	void fillBuffer();
	/**The file to read from.*/
	InStream* datSrc;
	/**Temporary storage for loading.*/
	std::vector<char> tmpLoad;
};

FileSourceMergeNode::FileSourceMergeNode(SortOptions* myOpts, uintptr_t buffSize, InStream* readFrom) : MultimergeNode(myOpts, buffSize){
	datSrc = readFrom;
	tmpLoad.resize(queueSize);
}

FileSourceMergeNode::~FileSourceMergeNode(){
}

void FileSourceMergeNode::fillBuffer(){
	uintptr_t itemSize = opts->itemSize;
	char* tmpB = &(tmpLoad[0]);
	uintptr_t numGet = queueSize - entQ.size();
	uintptr_t numRB = datSrc->readBytes(tmpB, numGet);
	if(numRB % itemSize){
		throw std::runtime_error("File truncated.");
	}
	if(numRB != numGet){
		entExhaust = 1;
	}
	entQ.insert(entQ.end(), tmpB, tmpB + numRB);
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
 * @param allMerge The nodes to run down.
 * @param phaseSize THe size of each phase of the topological sort.
 * @param usePool The threads to use.
 * @param tmpStore Storage for thread IDs.
 */
void mergesortCascadeFill(std::vector<MultimergeNode*>* allMerge, std::vector<uintptr_t>* phaseSize, ThreadPool* usePool, std::vector<uintptr_t>* tmpStore){
	uintptr_t mn0 = 0;
	for(uintptr_t pi = 0; pi<phaseSize->size(); pi++){
		uintptr_t psize = (*phaseSize)[pi];
		tmpStore->clear();
		for(uintptr_t ni = 0; ni < psize; ni++){
			MultimergeNode* curNode = (*allMerge)[mn0];
			if(!(curNode->entExhaust) && (curNode->entQ.size() != curNode->queueSize)){
				tmpStore->push_back(usePool->addTask(mergeSortFillMergeNode, curNode));
			}
			mn0++;
		}
		for(uintptr_t ti = 0; ti<tmpStore->size(); ti++){
			usePool->joinTask((*tmpStore)[ti]);
		}
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
			uintptr_t buffSize = 4096 * itemSize;
			//initial memory
			std::vector<MultimergeNode*> allMerge;
			for(uintptr_t i = 0; i<opts->numThread; i++){
				MemoryRangeMergesortUni* curU = &(initSUs[i]);
				allMerge.push_back(new MemoryBufferMergeNode(opts, buffSize, curU->numEnts, curU->tmpStore));
			}
			std::vector<uintptr_t> phaseSize;
				phaseSize.push_back(opts->numThread);
			std::vector<MultimergeNode*> curHor;
				curHor.insert(curHor.end(), allMerge.begin(), allMerge.end());
			//build up the merge between nodes
			std::vector<MultimergeNode*> nxtHor;
			while(curHor.size() > 1){
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
						MultimergeNode* mrgNod = new MergeNodeMergeNode(opts, buffSize, nodA, nodB);
						allMerge.push_back(mrgNod);
						nxtHor.push_back(mrgNod);
						i++;
						curPS++;
					}
				}
				phaseSize.push_back(curPS);
				curHor.clear();
				std::swap(curHor, nxtHor);
			}
		//mergeit
			std::vector<uintptr_t> threadIDTmp;
			MultimergeNode* endNod = curHor[0];
			char* curDump = inMem;
			while(!(endNod->entExhaust) || endNod->entQ.size()){
				std::copy(endNod->entQ.begin(), endNod->entQ.end(), curDump);
				curDump += endNod->entQ.size();
				endNod->entQ.clear();
				mergesortCascadeFill(&allMerge, &phaseSize, usePool, &threadIDTmp);
			}
		//clean up
			if(killPool){ delete(usePool); }
			for(uintptr_t i = 0; i<allMerge.size(); i++){ delete(allMerge[i]); }
	}
}

void outOfMemoryMergesort(InStream* startF, const char* tempFolderName, OutStream* outF, SortOptions* opts){
	//common storage
		uintptr_t itemSize = opts->itemSize;
		uintptr_t maxLoadEnt = opts->maxLoad / itemSize;
			maxLoadEnt = maxLoadEnt / 2;
			if(maxLoadEnt < 2){ maxLoadEnt = 2; }
		uintptr_t buffSize = 4096 * itemSize;
		std::vector<char> tempFileName;
			tempFileName.insert(tempFileName.end(), tempFolderName, tempFolderName + strlen(tempFolderName));
			tempFileName.insert(tempFileName.end(), pathElementSep, pathElementSep + strlen(pathElementSep));
			uintptr_t fnameBuffI0 = tempFileName.size();
			tempFileName.resize(fnameBuffI0 + 10+4*sizeof(uintmax_t)+4);
		char* fnameBuff = &(tempFileName[fnameBuffI0]);
		char* fpathBuff = &(tempFileName[0]);
	//make a pool if needbe
		int killPool = 0;
		ThreadPool* usePool = opts->usePool;
		if(usePool == 0){
			killPool = 1;
			usePool = new ThreadPool(opts->numThread);
		}
	//sort in chunks
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
			MultithreadGZipOutStream curDumpOut(0, fpathBuff, opts->numThread, usePool);
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
				std::vector<InStream*> saveFiles;
				std::vector<MultimergeNode*> allMerge;
				for(uintptr_t i = baseI; i<nextI; i++){
					sprintf(fnameBuff, "%s%ju", "sortspl_", (uintmax_t)i);
					InStream* curSrcS = new GZipInStream(fpathBuff);
					MultimergeNode* curMrg = new FileSourceMergeNode(opts, buffSize, curSrcS);
					saveFiles.push_back(curSrcS);
					allMerge.push_back(curMrg);
				}
				std::vector<uintptr_t> phaseSize;
					phaseSize.push_back(allMerge.size());
				std::vector<MultimergeNode*> curHor;
					curHor.insert(curHor.end(), allMerge.begin(), allMerge.end());
				//build up the merge between nodes
				std::vector<MultimergeNode*> nxtHor;
				while(curHor.size() > 1){
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
							MultimergeNode* mrgNod = new MergeNodeMergeNode(opts, buffSize, nodA, nodB);
							allMerge.push_back(mrgNod);
							nxtHor.push_back(mrgNod);
							i++;
							curPS++;
						}
					}
					phaseSize.push_back(curPS);
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
					curOut = new MultithreadGZipOutStream(0, fpathBuff, opts->numThread, usePool);
					nxtOutFiles++;
				}
				//output
				std::vector<uintptr_t> threadIDTmp;
				MultimergeNode* endNod = curHor[0];
				std::vector<char> tmpDump;
				while(!(endNod->entExhaust) || endNod->entQ.size()){
					if(endNod->entQ.size()){
						tmpDump.insert(tmpDump.end(), endNod->entQ.begin(), endNod->entQ.end());
						curOut->writeBytes(&(tmpDump[0]), tmpDump.size());
						endNod->entQ.clear();
						tmpDump.clear();
					}
					mergesortCascadeFill(&allMerge, &phaseSize, usePool, &threadIDTmp);
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
				killFile(fpathBuff);
			}
			numOutBase = nxtOutBase;
			numOutFiles = nxtOutFiles;
		}
	//clean up
		if(killPool){ delete(usePool); }
}

PreSortMultithreadPipe::PreSortMultithreadPipe(uintptr_t numSave) : drainCon(&drainMut), fillCon(&drainMut){
	maxBuff = numSave;
	endWrite = false;
}

void PreSortMultithreadPipe::writeBytes(const char* toW, uintptr_t numW){
	const char* curFoc = toW;
	uintptr_t numLeft = numW;
	
	writeMut.lock();
	drainMut.lock();
		while(numLeft){
			while(datBuff.size() >= maxBuff){
				if(endWrite){ drainMut.unlock(); writeMut.unlock(); throw std::runtime_error("Cannot write to closed pipe."); }
				drainCon.wait();
			}
			uintptr_t numAdd = maxBuff - datBuff.size(); numAdd = std::min(numAdd, numLeft);
			datBuff.insert(datBuff.end(), curFoc, curFoc+numAdd);
			curFoc += numAdd;
			numLeft -= numAdd;
			fillCon.signal();
		}
	drainMut.unlock();
	writeMut.unlock();
}

void PreSortMultithreadPipe::closeWrite(){
	writeMut.lock();
	drainMut.lock();
		endWrite = true;
		drainCon.broadcast();
		fillCon.broadcast();
	drainMut.unlock();
	writeMut.unlock();
}

int PreSortMultithreadPipe::readByte(){
	int toRet = -1;
	drainMut.lock();
		while(!endWrite && (datBuff.size()==0)){ fillCon.wait(); }
		if(datBuff.size()){
			toRet = datBuff[0];
			datBuff.pop_front();
		}
		drainCon.signal();
	drainMut.unlock();
	return toRet;
}

uintptr_t PreSortMultithreadPipe::readBytes(char* toR, uintptr_t numR){
	char* curFoc = toR;
	uintptr_t numLeft = numR;
	uintptr_t totRead = 0;
	
	drainMut.lock();
		while(numLeft){
			while(!endWrite && (datBuff.size()==0)){ fillCon.wait(); }
			if(datBuff.size()){
				uintptr_t numCpy = std::min(numLeft, datBuff.size());
				std::copy(datBuff.begin(), datBuff.begin() + numCpy, curFoc);
				datBuff.erase(datBuff.begin(), datBuff.begin() + numCpy);
				curFoc += numCpy;
				numLeft -= numCpy;
				totRead += numCpy;
			}
			else{
				numLeft = 0;
			}
			drainCon.signal();
		}
	drainMut.unlock();
	return totRead;
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

