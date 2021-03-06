#include "whodun_suffix.h"

#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <algorithm>

#include "whodun_stringext.h"

//many thanks to geeksforgeeks/suffix-array-set-2-a-nlognlogn-algorithm

void buildSuffixArray(const char* onData, uintptr_t* sortStore){
	SortOptions entryCompMeth;
		entryCompMeth.itemSize = 3*SUFFIX_ARRAY_CANON_SIZE;
		entryCompMeth.maxLoad = 4096 * 3 * SUFFIX_ARRAY_CANON_SIZE;
		entryCompMeth.numThread = 1;
		entryCompMeth.compMeth = SingleStringSuffixRankSortOption_compMeth;
		entryCompMeth.useUni = 0;
	uintptr_t datLen = strlen(onData) + 1;
	char* allSuffs = (char*)malloc(datLen * 3 * SUFFIX_ARRAY_CANON_SIZE);
	char* allSuffTmp = allSuffs;
	for(uintptr_t i = 0; i<datLen; i++){
		nat2be64(i, allSuffTmp);
		nat2be64(0x00FF & onData[i], allSuffTmp + SUFFIX_ARRAY_CANON_SIZE);
		nat2be64(((i+1<datLen) ? (0x00FF & onData[i+1]) : 0), allSuffTmp + 2*SUFFIX_ARRAY_CANON_SIZE);
		allSuffTmp = allSuffTmp + 3*SUFFIX_ARRAY_CANON_SIZE;
	}
	inMemoryMergesort(datLen, allSuffs, &entryCompMeth);
	for(uintptr_t k = 4; k < 2*datLen; k = k<<1){
		//rework the ranks
		uintptr_t nextRank = 1;
		allSuffTmp = allSuffs;
		sortStore[be2nat64(allSuffTmp)] = 0;
		uintptr_t prevRank = be2nat64(allSuffTmp + SUFFIX_ARRAY_CANON_SIZE);
		uintptr_t prevComp = be2nat64(allSuffTmp + 2*SUFFIX_ARRAY_CANON_SIZE);
		nat2be64(nextRank, allSuffTmp + SUFFIX_ARRAY_CANON_SIZE);
		for(uintptr_t i = 1; i<datLen; i++){
			allSuffTmp += 3*SUFFIX_ARRAY_CANON_SIZE;
			sortStore[be2nat64(allSuffTmp)] = i;
			uintptr_t focRank = be2nat64(allSuffTmp + SUFFIX_ARRAY_CANON_SIZE);
			uintptr_t focComp = be2nat64(allSuffTmp + 2*SUFFIX_ARRAY_CANON_SIZE);
			if((prevRank != focRank) || (prevComp != focComp)){
				nextRank++;
				prevRank = focRank;
				prevComp = focComp;
			}
			nat2be64(nextRank, allSuffTmp + SUFFIX_ARRAY_CANON_SIZE);
		}
		//figure out the next comparison ranks
		allSuffTmp = allSuffs;
		for(uintptr_t i = 0; i<datLen; i++){
			uintptr_t testInd = be2nat64(allSuffTmp) + (k>>1);
			uintptr_t nextRnk = (testInd < datLen) ? be2nat64(allSuffs + (3*sortStore[testInd]+1)*SUFFIX_ARRAY_CANON_SIZE) : 0;
			nat2be64(nextRnk, allSuffTmp + 2*SUFFIX_ARRAY_CANON_SIZE);
			allSuffTmp += 3*SUFFIX_ARRAY_CANON_SIZE;
		}
		inMemoryMergesort(datLen, allSuffs, &entryCompMeth);
	}
	allSuffTmp = allSuffs;
	for(uintptr_t i = 0; i<datLen; i++){
		sortStore[i] = be2nat64(allSuffTmp);
		allSuffTmp += 3*SUFFIX_ARRAY_CANON_SIZE;
	}
	free(allSuffs);
}

uintptr_t suffixArrayLowerBound(const char* toFind, const char* onData, uintptr_t* sortStore, uintptr_t numInSort){
	uintptr_t findLen = strlen(toFind);
	uintptr_t lowInd = 0;
	uintptr_t higInd = numInSort;
	while(higInd-lowInd > 0){
		uintptr_t midInd = (higInd + lowInd) >> 1;
		uintptr_t midEnt = sortStore[midInd];
		const char* midStr = onData + midEnt;
		int compVal = strncmp(midStr, toFind, findLen);
		if(compVal < 0){
			lowInd = midInd + 1;
		}
		else{
			higInd = midInd;
		}
	}
	return lowInd;
}

uintptr_t suffixArrayUpperBound(const char* toFind, const char* onData, uintptr_t* sortStore, uintptr_t numInSort){
	uintptr_t findLen = strlen(toFind);
	uintptr_t lowInd = 0;
	uintptr_t higInd = numInSort;
	while(higInd-lowInd > 0){
		uintptr_t midInd = (higInd + lowInd) >> 1;
		uintptr_t midEnt = sortStore[midInd];
		const char* midStr = onData + midEnt;
		int compVal = strncmp(midStr, toFind, findLen);
		if(compVal <= 0){
			lowInd = midInd + 1;
		}
		else{
			higInd = midInd;
		}
	}
	return lowInd;
}

void buildMultistringSuffixArray(size_t numStrings, const char** onData, uintptr_t* strIStore, uintptr_t* sortStore){
	SortOptions entryCompMeth;
		entryCompMeth.itemSize = 4*SUFFIX_ARRAY_CANON_SIZE;
		entryCompMeth.maxLoad = 4096 * 4 * SUFFIX_ARRAY_CANON_SIZE;
		entryCompMeth.numThread = 1;
		entryCompMeth.compMeth = MultiStringSuffixRankSortOption_compMeth;
		entryCompMeth.useUni = 0;
	//figure out the individual lengths
	uintptr_t* curSLens = (uintptr_t*)malloc(numStrings*sizeof(uintptr_t));
	uintptr_t** stringTmps = (uintptr_t**)malloc(numStrings*sizeof(uintptr_t*));
	uintptr_t maxLen = 0;
	uintptr_t datLen = 0;
	for(uintptr_t i = 0; i<numStrings; i++){
		stringTmps[i] = sortStore + datLen;
		uintptr_t curSLen = strlen(onData[i]) + 1;
		if(curSLen > maxLen){
			maxLen = curSLen;
		}
		curSLens[i] = curSLen;
		datLen += curSLen;
	}
	//get the initial two characters out of the way
	char* allSuffs = (char*)malloc(datLen * 4 * SUFFIX_ARRAY_CANON_SIZE);
	char* allSuffTmp = allSuffs;
	for(uintptr_t i = 0; i<numStrings; i++){
		const char* curStrDat = onData[i];
		uintptr_t curSLen = curSLens[i];
		for(uintptr_t j = 0; j<curSLen; j++){
			nat2be64(i, allSuffTmp);
			nat2be64(j, allSuffTmp + SUFFIX_ARRAY_CANON_SIZE);
			nat2be64(0x00FF & curStrDat[j], allSuffTmp + 2*SUFFIX_ARRAY_CANON_SIZE);
			nat2be64(((j+1<curSLen) ? (0x00FF & curStrDat[j+1]) : 0), allSuffTmp + 3*SUFFIX_ARRAY_CANON_SIZE);
			allSuffTmp = allSuffTmp + 4*SUFFIX_ARRAY_CANON_SIZE;
		}
	}
	inMemoryMergesort(datLen, allSuffs, &entryCompMeth);
	//do futher comparisons
	for(uintptr_t k = 4; k < 2*maxLen; k = k<<1){
		uintptr_t focStrI;
		uintptr_t focIndI;
		//rework the ranks
		uintptr_t nextRank = 1;
		allSuffTmp = allSuffs;
		focStrI = be2nat64(allSuffTmp);
		focIndI = be2nat64(allSuffTmp + SUFFIX_ARRAY_CANON_SIZE);
		stringTmps[focStrI][focIndI] = 0;
		uintptr_t prevRank = be2nat64(allSuffTmp + 2*SUFFIX_ARRAY_CANON_SIZE);
		uintptr_t prevComp = be2nat64(allSuffTmp + 3*SUFFIX_ARRAY_CANON_SIZE);
		nat2be64(nextRank, allSuffTmp + 2*SUFFIX_ARRAY_CANON_SIZE);
		for(uintptr_t i = 1; i<datLen; i++){
			allSuffTmp += 4*SUFFIX_ARRAY_CANON_SIZE;
			focStrI = be2nat64(allSuffTmp);
			focIndI = be2nat64(allSuffTmp + SUFFIX_ARRAY_CANON_SIZE);
			stringTmps[focStrI][focIndI] = i;
			uintptr_t focRank = be2nat64(allSuffTmp + 2*SUFFIX_ARRAY_CANON_SIZE);
			uintptr_t focComp = be2nat64(allSuffTmp + 3*SUFFIX_ARRAY_CANON_SIZE);
			if((prevRank != focRank) || (prevComp != focComp)){
				nextRank++;
				prevRank = focRank;
				prevComp = focComp;
			}
			nat2be64(nextRank, allSuffTmp + 2*SUFFIX_ARRAY_CANON_SIZE);
		}
		//figure out the next comparison ranks
		allSuffTmp = allSuffs;
		for(uintptr_t i = 0; i<datLen; i++){
			focStrI = be2nat64(allSuffTmp);
			focIndI = be2nat64(allSuffTmp + SUFFIX_ARRAY_CANON_SIZE);
			uintptr_t testInd = focIndI + (k>>1);
			uintptr_t nextRnk;
			if(testInd < curSLens[focStrI]){
				nextRnk = be2nat64(allSuffs + (4*stringTmps[focStrI][testInd]+2)*SUFFIX_ARRAY_CANON_SIZE);
			}
			else{
				nextRnk = 0;
			}
			nat2be64(nextRnk, allSuffTmp + 3*SUFFIX_ARRAY_CANON_SIZE);
			allSuffTmp += 4*SUFFIX_ARRAY_CANON_SIZE;
		}
		inMemoryMergesort(datLen, allSuffs, &entryCompMeth);
	}
	//flatten out to the result
	allSuffTmp = allSuffs;
	for(uintptr_t i = 0; i<datLen; i++){
		strIStore[i] = be2nat64(allSuffTmp);
		sortStore[i] = be2nat64(allSuffTmp + SUFFIX_ARRAY_CANON_SIZE);
		allSuffTmp += 4*SUFFIX_ARRAY_CANON_SIZE;
	}
	free(allSuffs);
	free(curSLens);
	free(stringTmps);
}

uintptr_t multiSuffixArrayLowerBound(const char* toFind, int numStrings, const char** onData, uintptr_t* strIStore, uintptr_t* sortStore, uintptr_t numInSort){
	uintptr_t findLen = strlen(toFind);
	uintptr_t lowInd = 0;
	uintptr_t higInd = numInSort;
	while(higInd-lowInd > 0){
		uintptr_t midInd = (higInd + lowInd) >> 1;
		uintptr_t midStI = strIStore[midInd];
		uintptr_t midEnt = sortStore[midInd];
		const char* midStr = onData[midStI] + midEnt;
		int compVal = strncmp(midStr, toFind, findLen);
		if(compVal < 0){
			lowInd = midInd + 1;
		}
		else{
			higInd = midInd;
		}
	}
	return lowInd;
}

uintptr_t multiSuffixArrayUpperBound(const char* toFind, int numStrings, const char** onData, uintptr_t* strIStore, uintptr_t* sortStore, uintptr_t numInSort){
	uintptr_t findLen = strlen(toFind);
	uintptr_t lowInd = 0;
	uintptr_t higInd = numInSort;
	while(higInd-lowInd > 0){
		uintptr_t midInd = (higInd + lowInd) >> 1;
		uintptr_t midStI = strIStore[midInd];
		uintptr_t midEnt = sortStore[midInd];
		const char* midStr = onData[midStI] + midEnt;
		int compVal = strncmp(midStr, toFind, findLen);
		if(compVal <= 0){
			lowInd = midInd + 1;
		}
		else{
			higInd = midInd;
		}
	}
	return lowInd;
}

