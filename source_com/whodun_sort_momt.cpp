#include "whodun_sort.h"
#include "whodun_sort_private.h"

#include <iostream>
#include <stdlib.h>
#include <string.h>

#include "whodun_oshook.h"

///////////////////////////////////////////////////////////
//Multithread initial sort
//one thread produces round robin, other threads work on what they are given

/**Information for a presort.*/
typedef struct{
	/**Index of the thread.*/
	int threadInd;
	/**The lock for the callback condition.*/
	void* callBackMut;
	/**The wait condtion for the callback (main will signal this).*/
	void* callBackCond;
	/**The wait condtion for the callback (this will signal main).*/
	void* kickOffCond;
	/**What needs to happen; 0 for wait, 1 for data, 2 for die.*/
	volatile int* callBackInfo;
	/**The number of entries in the presort.*/
	volatile uintptr_t numEnt;
	/**The stuff to sort.*/
	char* inMemSort;
	/**The place to write the results to.*/
	void** writeLoc;
	/**Options for the sort.*/
	SortOptions* opts;
} MTPresortArgs;

/**
 * Actually does a sort.
 * @param sortArgs The MTPresortArgs.
 */
void outOfMemoryPresortMultithreadSubFunction(void* sortArgs){
	MTPresortArgs* sortArg = (MTPresortArgs*)sortArgs;
	bool isMore = true;
	while(isMore){
		lockMutex(sortArg->callBackMut);
			while(!*(sortArg->callBackInfo)){
				waitCondition(sortArg->callBackMut, sortArg->callBackCond);
			}
			int curCom = *(sortArg->callBackInfo);
		unlockMutex(sortArg->callBackMut);
		if(curCom == 1){
			inMemoryMergesort(sortArg->numEnt, sortArg->inMemSort, sortArg->opts);
			sortArg->opts->workCom->comfwrite(sortArg->inMemSort, sortArg->opts->itemSize, sortArg->numEnt, *(sortArg->writeLoc));
		}
		else if(curCom == 2){
			isMore = false;
			break;
		}
		lockMutex(sortArg->callBackMut);
			*(sortArg->callBackInfo) = 0;
			signalCondition(sortArg->callBackMut, sortArg->kickOffCond);
		unlockMutex(sortArg->callBackMut);
	}
}

/**
 * This will sort as much as possible of a file, before working out of memory.
 * @param startFName THe starting file.
 * @param tempAName The files to put half the sorted chunks into.
 * @param tempBName The files to put the other half in.
 * @param numEntries The total number of entries to chunk. Will be set after the first read.
 * @param inMemSort Storage for loaded values.
 * @param opts The options for the sort.
 * @return Whether there was an exception.
 */
int helpOutOfMemoryPresortMultithread(const char* startFName, char** tempAName, char** tempBName, uintptr_t* numEntries, char** inMemSort, SortOptions* opts){
	int numThread = opts->numThread;
	uintptr_t itemSize = opts->itemSize;
	uintptr_t maxInMem = itemSize * (opts->maxLoad / itemSize);
	//open up the main file
	void* startF = opts->inCom->comfopen(startFName, "rb");
	if(startF == 0){
		std::cerr << "Problem reading file." << std::endl;
		return 1;
	}
	//open up the temporary files
	void** tempsA = (void**)malloc(numThread*sizeof(void*));
	void** tempsB = (void**)malloc(numThread*sizeof(void*));
	for(int i = 0; i<numThread; i++){
		tempsA[i] = opts->workCom->comfopen(tempAName[i], "wb");
		tempsB[i] = opts->workCom->comfopen(tempBName[i], "wb");
	}
	//make some thread state (and a mutex)
	bool* threadTgts = (bool*)calloc(numThread, sizeof(bool));
	void** threadHands = (void**)malloc(numThread*sizeof(void*));
	int* threadWait = (int*)calloc(numThread, sizeof(int));
	MTPresortArgs* threadArgs = (MTPresortArgs*)malloc(numThread * sizeof(MTPresortArgs));
	for(int i = 0; i<numThread; i++){
		MTPresortArgs* curArg = threadArgs+i;
		curArg->threadInd = i;
		curArg->callBackMut = makeMutex();
		curArg->callBackCond = makeCondition(curArg->callBackMut);
		curArg->kickOffCond = makeCondition(curArg->callBackMut);
		curArg->callBackInfo = threadWait + i;
		curArg->inMemSort = inMemSort[i];
		curArg->opts = opts;
		threadHands[i] = startThread(outOfMemoryPresortMultithreadSubFunction, curArg);
	}
	//read and sort
	uintptr_t totNumItems = 0;
	int curThread = 0;
	bool isMore = true;
	while(isMore){
		MTPresortArgs* curArg = threadArgs+curThread;
		lockMutex(curArg->callBackMut);
		while(*(curArg->callBackInfo)){
			waitCondition(curArg->callBackMut, curArg->kickOffCond);
		}
		uintptr_t numRead = opts->inCom->comfread(inMemSort[curThread], 1, maxInMem, startF);
		if(numRead){
			uintptr_t readNumEnt = numRead / itemSize;
			totNumItems += readNumEnt;
			curArg->numEnt = readNumEnt;
			curArg->writeLoc = (threadTgts[curThread] ? (tempsA + curThread) : (tempsB + curThread));
			*(curArg->callBackInfo) = 1;
			signalCondition(curArg->callBackMut, curArg->callBackCond);
			curThread = (curThread + 1) % numThread;
			threadTgts[curThread] = !(threadTgts[curThread]);
		}
		else{
			isMore = false;
		}
		unlockMutex(curArg->callBackMut);
	}
	//wait for all to finish
	for(int i = 0; i<numThread; i++){
		MTPresortArgs* curArg = threadArgs+i;
		lockMutex(curArg->callBackMut);
		while(*(curArg->callBackInfo)){
			waitCondition(curArg->callBackMut, curArg->kickOffCond);
		}
		*(curArg->callBackInfo) = 2;
		signalCondition(curArg->callBackMut, curArg->callBackCond);
		unlockMutex(curArg->callBackMut);
		joinThread(threadHands[i]);
		killCondition(curArg->callBackCond);
		killCondition(curArg->kickOffCond);
		killMutex(curArg->callBackMut);
		opts->workCom->comfclose(tempsA[i]);
		opts->workCom->comfclose(tempsB[i]);
	}
	free(tempsA); free(tempsB);
	free(threadHands);
	free(threadTgts);
	free(threadArgs);
	free(threadWait);
	opts->inCom->comfclose(startF);
	*numEntries = totNumItems;
	return 0;
}

///////////////////////////////////////////////////////////
//Multithread unsupervised merge
//each thread keeps merging until in only one file

/**Information for a merge pass.*/
typedef struct{
	/**Index of the thread.*/
	int threadInd;
	/**One of the starting files, will be trashed.*/
	char* tempAName;
	/**One of the starting files, will be trashed.*/
	char* tempBName;
	/**Temporary file.*/
	char* tempCName;
	/**Temporary file.*/
	char* tempDName;
	/**The starting number of items to merge.*/
	uintptr_t curMergeSize;
	/**One of the four given names, which specifies where the result is. Null for problem.*/
	char* winnerName;
	/**Options for the sort.*/
	SortOptions* opts;
} MTMergeArgs;

/**
 * Actually does a merge.
 * @param sortArgs The MTMergeArgs.
 */
void outOfMemoryMergeMultithreadSubFunction(void* sortArgs){
	MTMergeArgs* sortArg = (MTMergeArgs*)sortArgs;
	sortArg->winnerName = 0;
	char* inMemSort = (char*)malloc(sortArg->opts->maxLoad);
	uintptr_t itemSize = sortArg->opts->itemSize;
	uintptr_t entMaxLoad = sortArg->opts->maxLoad / itemSize;
	CompressionMethod* workCom = sortArg->opts->workCom;
	//merge until only one chunk is written
	void* tempA; void* tempB;
	void* tempC; void* tempD;
	uintptr_t curMergeSize = sortArg->curMergeSize;
	bool curMergeCD = true;
	uintptr_t lastNumWritten;
	do{
		if(sortArg->threadInd == 0){
			std::cout << "Merge " << curMergeSize << std::endl;
		}
		lastNumWritten = 0;
		//open the files
		tempA = workCom->comfopen(curMergeCD ? sortArg->tempAName : sortArg->tempCName, "rb");
		tempB = workCom->comfopen(curMergeCD ? sortArg->tempBName : sortArg->tempDName, "rb");
		tempC = workCom->comfopen(curMergeCD ? sortArg->tempCName : sortArg->tempAName, "wb");
		tempD = workCom->comfopen(curMergeCD ? sortArg->tempDName : sortArg->tempBName, "wb");
		if(tempA == 0 || tempB == 0 || tempC == 0 || tempD == 0){
			std::cerr << "Problem with temp file." << std::endl;
			free(inMemSort);
			workCom->comfclose(tempA); workCom->comfclose(tempB); workCom->comfclose(tempC); workCom->comfclose(tempD);
			return;
		}
		bool curWriteA = true;
		bool hitEndA = false;
		bool hitEndB = false;
		while(!hitEndA && !hitEndB){
			int mergeRet = helpMergeTwo(tempA, tempB, curWriteA ? tempC : tempD, inMemSort, entMaxLoad, curMergeSize, sortArg->opts, &hitEndA, &hitEndB, &lastNumWritten);
			if(mergeRet){
				free(inMemSort);
				workCom->comfclose(tempA); workCom->comfclose(tempB); workCom->comfclose(tempC); workCom->comfclose(tempD);
				return;
			}
			curWriteA = !curWriteA;
		}
		curMergeSize = curMergeSize << 1;
		curMergeCD = !curMergeCD;
		workCom->comfclose(tempA); workCom->comfclose(tempB); workCom->comfclose(tempC); workCom->comfclose(tempD);
	} while(lastNumWritten > 1);
	sortArg->winnerName = curMergeCD ? sortArg->tempAName : sortArg->tempCName;
	free(inMemSort);
}

/**
 * This will do merges among single threads.
 * @param tempAName The names of the starting files.
 * @param tempBName The names of the starting files.
 * @param tempCName The names of the temporary files.
 * @param tempDName The names of the temporary files.
 * @param winnerName The place to store the file that holds the result.
 * @param opts The options for the sort.
 * @return Whether there was an exception.
 */
int helpOutOfMemoryMergeFullMultithread(char** tempAName, char** tempBName, char** tempCName, char** tempDName, char** winnerName, SortOptions* opts){
	//build info for the threads
	int numThread = opts->numThread;
	MTMergeArgs* threadArgs = (MTMergeArgs*)malloc(numThread * sizeof(MTMergeArgs));
	for(int i = 0; i<numThread; i++){
		MTMergeArgs* curArg = threadArgs+i;
		curArg->threadInd = i;
		curArg->tempAName = tempAName[i];
		curArg->tempBName = tempBName[i];
		curArg->tempCName = tempCName[i];
		curArg->tempDName = tempDName[i];
		curArg->curMergeSize = opts->maxLoad / opts->itemSize;
		curArg->opts = opts;
	}
	//start the threads
	void** threadHands = (void**)malloc(numThread*sizeof(void*));
	for(int i = 0; i<numThread; i++){
		MTMergeArgs* curArg = threadArgs+i;
		threadHands[i] = startThread(outOfMemoryMergeMultithreadSubFunction, curArg);
	}
	//join on the threads
	for(int i = 0; i<numThread; i++){
		joinThread(threadHands[i]);
	}
	//clean up and report
	int isRet = 0;
	for(int i = 0; i<numThread; i++){
		MTMergeArgs* curArg = threadArgs+i;
		winnerName[i] = curArg->winnerName;
		isRet = isRet || (curArg->winnerName == 0);
	}
	free(threadArgs);
	free(threadHands);
	return isRet;
}

///////////////////////////////////////////////////////////
//Multithread collect merge
//spawn threads to merge two until all in one file

/**Information for a merge pass.*/
typedef struct{
	/**Index of the thread.*/
	int threadInd;
	/**One of the starting files, will be trashed.*/
	char* tempAName;
	/**One of the starting files, will be trashed.*/
	char* tempBName;
	/**The result file.*/
	char* tempCName;
	/**Options for the sort.*/
	SortOptions* opts;
} MTMergeEndArgs;

/**
 * Actually does a consumption merge.
 * @param sortArgs The MTMergeEndArgs.
 */
void outOfMemoryMergeEndMultithreadSubFunction(void* sortArgs){
	MTMergeEndArgs* sortArg = (MTMergeEndArgs*)sortArgs;
	char* inMemSort = (char*)malloc(sortArg->opts->maxLoad);
	uintptr_t itemSize = sortArg->opts->itemSize;
	uintptr_t entMaxLoad = sortArg->opts->maxLoad / itemSize;
	CompressionMethod* workCom = sortArg->opts->workCom;
	SortOptions* compM = sortArg->opts;
	//useful values
	uintptr_t halfMemSize = entMaxLoad >> 1;
	char* readAStore = inMemSort;
	char* readBStore = inMemSort + itemSize*halfMemSize;
	char* readAFoc;
	char* readBFoc;
	//open the files
	void* fromA = workCom->comfopen(sortArg->tempAName, "rb");
	void* fromB = workCom->comfopen(sortArg->tempBName, "rb");
	void* toEnd = workCom->comfopen(sortArg->tempCName, "wb1");
	if((fromA == 0) || (fromB == 0) || (toEnd == 0)){
		workCom->comfclose(fromA); workCom->comfclose(fromB); workCom->comfclose(toEnd);
		free(inMemSort);
		return;
	}
	//merge until all written
	uintptr_t numBuffA = workCom->comfread(readAStore, itemSize, halfMemSize, fromA);
		readAFoc = readAStore;
	uintptr_t numBuffB = workCom->comfread(readBStore, itemSize, halfMemSize, fromB);
		readBFoc = readBStore;
	while(numBuffA | numBuffB){
		if(!numBuffA){
			workCom->comfwrite(readBFoc, itemSize, numBuffB, toEnd);
			numBuffB = workCom->comfread(readBStore, itemSize, halfMemSize, fromB);
				readBFoc = readBStore;
		}
		else if(!numBuffB){
			workCom->comfwrite(readAFoc, itemSize, numBuffA, toEnd);
			numBuffA = workCom->comfread(readAStore, itemSize, halfMemSize, fromA);
				readAFoc = readAStore;
		}
		else{
			uintptr_t totNumBuff = numBuffA + numBuffB;
			for(uintptr_t itNum = totNumBuff; itNum > 0; itNum--){
				if(compM->compMeth(readAFoc, readBFoc)){
					workCom->comfwrite(readAFoc, 1, itemSize, toEnd);
					readAFoc += itemSize;
					numBuffA--;
					if(!numBuffA){
						numBuffA = workCom->comfread(readAStore, itemSize, halfMemSize, fromA);
							readAFoc = readAStore;
						break;
					}
				}
				else{
					workCom->comfwrite(readBFoc, 1, itemSize, toEnd);
					readBFoc += itemSize;
					numBuffB--;
					if(!numBuffB){
						numBuffB = workCom->comfread(readBStore, itemSize, halfMemSize, fromB);
							readBFoc = readBStore;
						break;
					}
				}
			}
		}
	}
	workCom->comfclose(fromA); workCom->comfclose(fromB); workCom->comfclose(toEnd);
	free(inMemSort);
}

///////////////////////////////////////////////////////////
//pull it all together

int outOfMemoryMultithreadMergesort(const char* startFName, const char* tempFolderName, const char* outFileName, SortOptions* opts){
	int numThread = opts->numThread;
	uintptr_t itemSize = opts->itemSize;
	uintptr_t entMaxLoad = opts->maxLoad / itemSize;
	if(entMaxLoad < 2){
		//Problem
		std::cerr << "Max load is smaller than the size of two items." << std::endl;
		return 1;
	}
	char** tempFileNames = generateTemporaryFileNames(tempFolderName, 4*numThread);
	//figure out how many entries are in the file
	uintptr_t numEntries;
	char** inMemSort = (char**)malloc(numThread * sizeof(char*));
	for(int i = 0; i<numThread; i++){
		inMemSort[i] = (char*)malloc(opts->maxLoad);
	}
	//chunk the start file, sort as much as possible, and dump to tempA and tempB.
	int chunkRet = helpOutOfMemoryPresortMultithread(startFName, tempFileNames, tempFileNames + numThread, &numEntries, inMemSort, opts);
	for(int i = 0; i<numThread; i++){free(inMemSort[i]);}
	free(inMemSort);
	if(chunkRet){
		free(tempFileNames);
		return chunkRet;
	}
	std::cout << "Merging " << numEntries << std::endl;
	//run the merges for each thread
	char** winnerNames = (char**)malloc(numThread*sizeof(char*));
	int mergeARet = helpOutOfMemoryMergeFullMultithread(tempFileNames, tempFileNames + numThread, tempFileNames + 2*numThread, tempFileNames + 3*numThread, winnerNames, opts);
	if(mergeARet){
		free(winnerNames);
		free(tempFileNames);
		return mergeARet;
	}
	//merge the threads together
	bool curB = true;//whether to store a merge in B or D (winnerNames is full of A and C)
	char** leftNames = (char**)malloc(numThread*sizeof(char*));
	void** threadHands = (void**)malloc(numThread*sizeof(void*));
	MTMergeEndArgs* threadArgs = (MTMergeEndArgs*)malloc(numThread * sizeof(MTMergeEndArgs));
	int numLeftMrg = numThread;
	while(numLeftMrg > 1){
		std::cout << "Merge* " << numLeftMrg << std::endl;
		int numStartThread = 0;
		int nextLeftMrg = 0;
		MTMergeEndArgs* nextTA = threadArgs;
		for(int i = 0; i<numLeftMrg; i+=2){
			nextLeftMrg++;
			if(i+1 >= numLeftMrg){
				leftNames[i>>1] = winnerNames[i];
			}
			else{
				//start the thread
				nextTA->threadInd = numStartThread;
				nextTA->tempAName = winnerNames[i];
				nextTA->tempBName = winnerNames[i+1];
				nextTA->tempCName = (tempFileNames + (curB ? numThread : (3*numThread)))[i];
				leftNames[i>>1] = nextTA->tempCName;
				std::cout << nextTA->tempAName << " ";
				std::cout << nextTA->tempBName << " ";
				std::cout << nextTA->tempCName << std::endl;;
				nextTA->opts = opts;
				threadHands[numStartThread] = startThread(outOfMemoryMergeEndMultithreadSubFunction, nextTA);
				numStartThread++;
				nextTA++;
			}
		}
		//join on the threads
		for(int i = 0; i<numStartThread; i++){
			joinThread(threadHands[i]);
		}
		//move to the next
		curB = !curB;
		memcpy(winnerNames, leftNames, nextLeftMrg*sizeof(char*));
		numLeftMrg = nextLeftMrg;
	}
	free(leftNames);
	free(threadHands);
	free(threadArgs);
	//copy final result to the end
	char* tmpDump = (char*)malloc(opts->maxLoad);
	if(opts->workCom == opts->outCom){
		FILE* realSrc = fopen(winnerNames[0], "rb");
		FILE* realTgt = fopen(outFileName, "wb");
		if(realTgt == 0){
			std::cerr << "Problem writing " << outFileName << std::endl;
			free(winnerNames); fclose(realSrc);
			free(tempFileNames);
			free(tmpDump);
			return 1;
		}
		uintptr_t realNR = fread(tmpDump, 1, opts->maxLoad, realSrc);
		while(realNR){
			fwrite(tmpDump, 1, realNR, realTgt);
			realNR = fread(tmpDump, 1, opts->maxLoad, realSrc);
		}
		fclose(realSrc); fclose(realTgt);
	}
	else{
		void* realSrc = opts->workCom->comfopen(winnerNames[0], "rb");
		void* realTgt = opts->outCom->comfopen(outFileName, "wb");
		if(realTgt == 0){
			std::cerr << "Problem writing " << outFileName << std::endl;
			free(winnerNames); opts->workCom->comfclose(realSrc);
			free(tempFileNames);
			free(tmpDump);
			return 1;
		}
		uintptr_t realNR = opts->workCom->comfread(tmpDump, 1, opts->maxLoad, realSrc);
		while(realNR){
			opts->outCom->comfwrite(tmpDump, 1, realNR, realTgt);
			realNR = opts->workCom->comfread(tmpDump, 1, opts->maxLoad, realSrc);
		}
		opts->workCom->comfclose(realSrc);
		opts->outCom->comfclose(realTgt);
	}
	free(tmpDump);
	//clean up
	free(winnerNames);
	free(tempFileNames);
	return 0;
}
