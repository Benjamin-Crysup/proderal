#include "whodun_sort.h"
#include "whodun_sort_private.h"

#include <iostream>
#include <stdlib.h>
#include <string.h>

#include "whodun_oshook.h"

#define TEMP_NAME "sort_tmp"

char** generateTemporaryFileNames(const char* forFolder, int numFiles){
	int foldLen = strlen(forFolder);
	int sepLen = strlen(pathElementSep);
	int tmpLen = strlen(TEMP_NAME);
	int numLen = 4*sizeof(int);
	int numNeedBts = numFiles*sizeof(char*) + numFiles*(foldLen+sepLen+tmpLen+numLen+1);
	char** toRet = (char**)malloc(numNeedBts);
	char* curFoc = (char*)(toRet + numFiles);
	for(int i = 0; i<numFiles; i++){
		toRet[i] = curFoc;
		int numCW = sprintf(curFoc, "%s%s%s%x", forFolder, pathElementSep, TEMP_NAME, i);
		curFoc += (numCW + 1);
	}
	return toRet;
}

/**
 * This will sort as much as possible of a file, before working out of memory.
 * @param startFName THe starting file.
 * @param tempAName The file to put half the sorted chunks into.
 * @param tempBName The file to put the other half in.
 * @param numEntries The total number of entries to chunk. Will be set after the first read.
 * @param inMemSort Storage for loaded values.
 * @param opts The options for the sort.
 * @return Whether there was an exception.
 */
int helpOutOfMemoryPresort(const char* startFName, const char* tempAName, const char* tempBName, uintptr_t* numEntries, char* inMemSort, SortOptions* opts){
	uintptr_t itemSize = opts->itemSize;
	uintptr_t maxInMem = itemSize * (opts->maxLoad / itemSize);
	CompressionMethod* inCom = opts->inCom;
	CompressionMethod* workCom = opts->workCom;
	void* startF = inCom->comfopen(startFName, "rb");
	if(startF == 0){
		std::cerr << "Problem reading file." << std::endl;
		return 1;
	}
	void* tempA = workCom->comfopen(tempAName, "wb");
	void* tempB = workCom->comfopen(tempBName, "wb");
	if(tempA == 0 || tempB == 0){
		std::cerr << "Problem with temp file." << std::endl;
		inCom->comfclose(startF);
		workCom->comfclose(tempA); workCom->comfclose(tempB);
		return 1;
	}
	uintptr_t totNumItems = 0;
	bool curWriteA = true;
	uintptr_t numRead = inCom->comfread(inMemSort, 1, maxInMem, startF);
	while(numRead){
		uintptr_t readNumEnt = numRead / itemSize;
		totNumItems += readNumEnt;
		inMemoryMergesort(readNumEnt, inMemSort, opts);
		workCom->comfwrite(inMemSort, 1, readNumEnt*itemSize, curWriteA ? tempA : tempB);
		curWriteA = !curWriteA;
		numRead = inCom->comfread(inMemSort, 1, maxInMem, startF);
	}
	inCom->comfclose(startF);
	workCom->comfclose(tempA); workCom->comfclose(tempB);
	*numEntries = totNumItems;
	return 0;
}

int helpMergeTwo(void* fromA, void* fromB, void* toEnd, char* inMemSort, uintptr_t inMemSize, uintptr_t numMerge, SortOptions* opts, bool* hitEndA, bool* hitEndB, uintptr_t* lastNumWritten){
	bool anyRead = false;
	CompressionMethod* workCom = opts->workCom;
	uintptr_t itemSize = opts->itemSize;
	uintptr_t halfMemSize = inMemSize >> 1;
	char* readAStore = inMemSort;
	char* readBStore = inMemSort + itemSize*halfMemSize;
	char* readAFoc = 0;
	char* readBFoc = 0;
	uintptr_t numInA = numMerge;
	uintptr_t numInB = numMerge;
	uintptr_t numBuffA = 0;
	uintptr_t numBuffB = 0;
#define MERGE_READ_A \
			readAFoc = readAStore;\
			uintptr_t maxRCnt = (numInA < halfMemSize) ? numInA : halfMemSize;\
			numBuffA = workCom->comfread(readAStore, itemSize, maxRCnt, fromA);\
			if(numBuffA == 0){\
				numInA = 0;\
				*hitEndA = true;\
				break;\
			}\
			else{\
				anyRead = true;\
				if(numBuffA < maxRCnt){\
					numInA = numBuffA;\
				}\
			}
#define MERGE_READ_B \
			readBFoc = readBStore;\
			uintptr_t maxRCnt = (numInB < halfMemSize) ? numInB : halfMemSize;\
			numBuffB = workCom->comfread(readBStore, itemSize, maxRCnt, fromB);\
			if(numBuffB == 0){\
				numInB = 0;\
				*hitEndB = true;\
				break;\
			}\
			else{\
				anyRead = true;\
				if(numBuffB < maxRCnt){\
					numInB = numBuffB;\
				}\
			}
	while(numInA && numInB){
		//read A and B
		if(!numBuffA){
			MERGE_READ_A
		}
		if(!numBuffB){
			MERGE_READ_B
		}
		//compare and merge
		uintptr_t totNumBuff = numBuffA + numBuffB;
		for(uintptr_t itNum = totNumBuff; itNum > 0; itNum--){
			if(opts->compMeth(readAFoc, readBFoc)){
				workCom->comfwrite(readAFoc, 1, itemSize, toEnd);
				readAFoc += itemSize;
				numInA--;
				numBuffA--;
				if(!numBuffA){
					break;
				}
			}
			else{
				workCom->comfwrite(readBFoc, 1, itemSize, toEnd);
				readBFoc += itemSize;
				numInB--;
				numBuffB--;
				if(!numBuffB){
					break;
				}
			}
		}
	}
	while(numInA){
		if(!numBuffA){
			MERGE_READ_A
		}
		workCom->comfwrite(readAFoc, itemSize, numBuffA, toEnd);
		numInA = numInA - numBuffA;
		numBuffA = 0;
	}
	while(numInB){
		if(!numBuffB){
			MERGE_READ_B
		}
		workCom->comfwrite(readBFoc, itemSize, numBuffB, toEnd);
		numInB = numInB - numBuffB;
		numBuffB = 0;
	}
	if(anyRead){
		*lastNumWritten = *lastNumWritten + 1;
	}
	return 0;
}

int outOfMemoryMergesort(const char* startFName, const char* tempFolderName, const char* outFileName, SortOptions* opts){
	uintptr_t itemSize = opts->itemSize;
	uintptr_t entMaxLoad = opts->maxLoad / itemSize;
	if(entMaxLoad < 2){
		//Problem
		std::cerr << "Max load is smaller than the size of two items." << std::endl;
		return 1;
	}
	char** tempFileNames = generateTemporaryFileNames(tempFolderName, 4);
	//figure out how many entries are in the file
	uintptr_t numEntries;
	char* inMemSort = (char*)malloc(opts->maxLoad);
	//chunk the start file, sort as much as possible, and dump to tempA and tempB.
	int chunkRet = helpOutOfMemoryPresort(startFName, tempFileNames[0], tempFileNames[1], &numEntries, inMemSort, opts);
	if(chunkRet){
		free(inMemSort); free(tempFileNames);
		return chunkRet;
	}
	//merge until only one chunk is written
	void* tempA; void* tempB;
	void* tempC; void* tempD;
	uintptr_t curMergeSize = entMaxLoad;
	bool curMergeCD = true;
	uintptr_t lastNumWritten;
	do{
		std::cout << "Merge " << curMergeSize << "/" << numEntries << std::endl;
		lastNumWritten = 0;
		//open the files
		tempA = opts->workCom->comfopen(curMergeCD ? tempFileNames[0] : tempFileNames[2], "rb");
		tempB = opts->workCom->comfopen(curMergeCD ? tempFileNames[1] : tempFileNames[3], "rb");
		tempC = opts->workCom->comfopen(curMergeCD ? tempFileNames[2] : tempFileNames[0], "wb");
		tempD = opts->workCom->comfopen(curMergeCD ? tempFileNames[3] : tempFileNames[1], "wb");
		if(tempA == 0 || tempB == 0 || tempC == 0 || tempD == 0){
			std::cerr << "Problem with temp file." << std::endl;
			free(inMemSort); free(tempFileNames);
			opts->workCom->comfclose(tempA); opts->workCom->comfclose(tempB); opts->workCom->comfclose(tempC); opts->workCom->comfclose(tempD);
			return 1;
		}
		bool curWriteA = true;
		bool hitEndA = false;
		bool hitEndB = false;
		while(!hitEndA && !hitEndB){
			int mergeRet = helpMergeTwo(tempA, tempB, curWriteA ? tempC : tempD, inMemSort, entMaxLoad, curMergeSize, opts, &hitEndA, &hitEndB, &lastNumWritten);
			if(mergeRet){
				free(inMemSort); free(tempFileNames);
				opts->workCom->comfclose(tempA); opts->workCom->comfclose(tempB); opts->workCom->comfclose(tempC); opts->workCom->comfclose(tempD);
				return mergeRet;
			}
			curWriteA = !curWriteA;
		}
		curMergeSize = curMergeSize << 1;
		curMergeCD = !curMergeCD;
		opts->workCom->comfclose(tempA); opts->workCom->comfclose(tempB); opts->workCom->comfclose(tempC); opts->workCom->comfclose(tempD);
	} while(lastNumWritten > 1);
	//copy to the destination
	void* realSrc = opts->workCom->comfopen(curMergeCD ? tempFileNames[0] : tempFileNames[2], "rb");
	void* realTgt = opts->outCom->comfopen(outFileName, "wb");
	if(realTgt == 0){
		std::cerr << "Problem writing " << outFileName << std::endl;
		free(inMemSort); free(tempFileNames);
		opts->workCom->comfclose(realSrc);
		return 1;
	}
	uintptr_t realNR = opts->workCom->comfread(inMemSort, 1, opts->maxLoad, realSrc);
	while(realNR){
		opts->outCom->comfwrite(inMemSort, 1, realNR, realTgt);
		realNR = opts->workCom->comfread(inMemSort, 1, opts->maxLoad, realSrc);
	}
	opts->workCom->comfclose(realSrc);
	opts->outCom->comfclose(realTgt);
	//kill
	//for(int i = 0; i<4; i++){ killFile(tempFileNames[i]); }
	free(inMemSort); free(tempFileNames);
	return 0;
}
