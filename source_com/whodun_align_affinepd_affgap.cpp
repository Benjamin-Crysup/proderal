#include "whodun_align_affinepd.h"
#include "whodun_align_affinepd_private.h"

#include <string.h>

const char* parseAffineGapCostSpecification(int numPChars, const char* toParse, AlignCostAffine** toFill){
	*toFill = 0;
	const char* curFoc = toParse;
#define READ_NEXT_ITEM_AG(newName) \
	curFoc = curFoc + strspn(curFoc, WHITESPACE); if(!*curFoc){ return 0; }\
	int newName = atoi(curFoc);\
	curFoc = strpbrk(curFoc, WHITESPACE); if(!curFoc){ return 0; }
	//read the gap cost
	READ_NEXT_ITEM_AG(openCost)
	READ_NEXT_ITEM_AG(extendCost)
	READ_NEXT_ITEM_AG(closeCost)
	//read the number of characters
	READ_NEXT_ITEM_AG(numChars)
	//read the characters
	std::vector<unsigned char> useChars;
	for(int i = 0; i<numChars; i++){
		READ_NEXT_ITEM_AG(curChar)
		useChars.push_back(curChar);
	}
	//read the pairwise costs
	std::vector<int> allCost;
	for(int i = 0; i<numChars; i++){
		for(int j = 0; j<numChars; j++){
			READ_NEXT_ITEM_AG(curCost)
			allCost.push_back(curCost);
		}
	}
	//build up the thing
	AlignCostAffine* toRet = (AlignCostAffine*)malloc(sizeof(AlignCostAffine));
	toRet->openCost = openCost;
	toRet->extendCost = extendCost;
	toRet->closeCost = closeCost;
	//character map
	toRet->charMap = (int*)calloc(256, sizeof(int));
	for(int i = 0; i<numChars; i++){
		toRet->charMap[useChars[i]] = i+1;
	}
	//empty mm cost table
	toRet->numLiveChars = numChars + 1;
	int costTabSize = (numChars+1)*sizeof(int*) + (numChars+1)*(numChars+1)*sizeof(int);
	toRet->allMMCost = (int**)malloc(costTabSize);
	memset(toRet->allMMCost, 0, costTabSize);
	//fill in the pointer array
	int* curAFoc = (int*)(toRet->allMMCost + (numChars+1));
	for(int i = 0; i<=numChars; i++){
		toRet->allMMCost[i] = curAFoc;
		curAFoc += (numChars + 1);
	}
	//and the characters
	int curCI = 0;
	for(int i = 0; i<numChars; i++){
		for(int j = 0; j<numChars; j++){
			toRet->allMMCost[i+1][j+1] = allCost[curCI];
			curCI++;
		}
	}
	*toFill = toRet;
	return curFoc;
}

AlignCostAffine* cloneAlignCostAffine(AlignCostAffine* toClone){
	AlignCostAffine* toRet = (AlignCostAffine*)malloc(sizeof(AlignCostAffine));
	toRet->openCost = toClone->openCost;
	toRet->extendCost = toClone->extendCost;
	toRet->closeCost = toClone->closeCost;
	toRet->charMap = (int*)malloc(256*sizeof(int));
	memcpy(toRet->charMap, toClone->charMap, 256*sizeof(int));
	toRet->numLiveChars = toClone->numLiveChars;
	//set up mm table pointers
	int numChar = toRet->numLiveChars;
	toRet->allMMCost = (int**)malloc(numChar*sizeof(int*) + numChar*numChar*sizeof(int));
	int* curAFoc = (int*)(toRet->allMMCost + numChar);
	for(int i = 0; i<numChar; i++){
		toRet->allMMCost[i] = curAFoc;
		curAFoc += numChar;
	}
	//copy mismatch costs
	memcpy(toRet->allMMCost + numChar, toClone->allMMCost + numChar, numChar*numChar*sizeof(int));
	return toRet;
}

void freeAlignCostAffine(AlignCostAffine* toKill){
	free(toKill->charMap);
	free(toKill->allMMCost);
	free(toKill);
}

void freeAlignCosts(std::vector<AlignCostAffine*>* toKill){
	for(unsigned int i=0; i<toKill->size(); i++){
		freeAlignCostAffine((*toKill)[i]);
	}
}
