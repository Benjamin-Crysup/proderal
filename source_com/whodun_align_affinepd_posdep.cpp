#include "whodun_align_affinepd.h"
#include "whodun_align_affinepd_private.h"

#include <string.h>

AlignCostAffine* getPositionDependentAlignmentCosts(PositionDependentAlignCostKDNode* forPDCost, int inA, int inB){
	AlignCostAffine* curWin = 0;
	int winPriority = 0;
	PositionDependentAlignCostKDNode* curFoc = forPDCost;
	while(curFoc){
		for(int i = 0; i<curFoc->numSpan; i++){
			PositionDependentBounds* curBnd = curFoc->allSpans[i];
			bool isAfterAS = (curBnd->startA < 0) || (curBnd->startA <= inA);
			bool isBeforeAE = (curBnd->endA < 0) || (curBnd->endA > inA);
			bool isAfterBS = (curBnd->startB < 0) || (curBnd->startB <= inB);
			bool isBeforeBE = (curBnd->endB < 0) || (curBnd->endB > inB);
			if(isAfterAS && isBeforeAE && isAfterBS && isBeforeBE){
				if(curWin && (winPriority > curBnd->priority)){
					continue;
				}
				curWin = curBnd->regCosts;
				winPriority = curBnd->priority;
			}
		}
		if(curFoc->splitOn == PDAG_AXIS_A){
			curFoc = (PositionDependentAlignCostKDNode*)((inA < curFoc->splitAt) ? curFoc->subNodeLesser : curFoc->subNodeGreatE);
		}
		else if(curFoc->splitOn == PDAG_AXIS_B){
			curFoc = (PositionDependentAlignCostKDNode*)((inB < curFoc->splitAt) ? curFoc->subNodeLesser : curFoc->subNodeGreatE);
		}
	}
	return curWin;
}

PositionDependentAlignCostKDNode* produceUniversalPositionDependent(AlignCostAffine* useEvery){
	PositionDependentBounds* defBnd = (PositionDependentBounds*)malloc(sizeof(PositionDependentBounds));
	defBnd->startA = -1; defBnd->endA = -1;
	defBnd->startB = -1; defBnd->endB = -1;
	defBnd->priority = 0;
	defBnd->regCosts = useEvery;
	PositionDependentAlignCostKDNode* toRet = (PositionDependentAlignCostKDNode*)malloc(sizeof(PositionDependentAlignCostKDNode));
	toRet->splitOn = PDAG_AXIS_A;
	toRet->splitAt = -1;
	toRet->subNodeLesser = 0;
	toRet->subNodeGreatE = 0;
	toRet->numSpan = 1;
	toRet->allSpans = (PositionDependentBounds**)malloc(sizeof(PositionDependentBounds*));
	toRet->allSpans[0] = defBnd;
	return toRet;
}

/**
 * Figures out how bounds get split.
 * @param allBounds The bounds to split.
 * @param onAxis The axis being split.
 * @param onIndex THe index being split on.
 * @param leftVec The place to add bounds that are less than the split index.
 * @param rightVec The place to add bounds that are greater than or equal to the split index.
 * @param spanVec The place to add bounds that span.
 */
void calculateSplit(std::vector<PositionDependentBounds*>* allBounds, int onAxis, int onIndex, std::vector<PositionDependentBounds*>* leftVec, std::vector<PositionDependentBounds*>* rightVec, std::vector<PositionDependentBounds*>* spanVec){
	for(unsigned int i = 0; i<allBounds->size(); i++){
		PositionDependentBounds* curBnd = (*allBounds)[i];
		if(onAxis == PDAG_AXIS_A){
			if((curBnd->startA < 0) && (curBnd->endA < 0)){
				spanVec->push_back(curBnd);
			}
			else if(curBnd->startA < 0){
				if(curBnd->endA <= onIndex){
					leftVec->push_back(curBnd);
				}
				else{
					spanVec->push_back(curBnd);
				}
			}
			else if(curBnd->endA < 0){
				if(curBnd->startA >= onIndex){
					rightVec->push_back(curBnd);
				}
				else{
					spanVec->push_back(curBnd);
				}
			}
			else{
				if(curBnd->endA <= onIndex){
					leftVec->push_back(curBnd);
				}
				else if(curBnd->startA >= onIndex){
					rightVec->push_back(curBnd);
				}
				else{
					spanVec->push_back(curBnd);
				}
			}
		}
		else{
			if((curBnd->startB < 0) && (curBnd->endB < 0)){
				spanVec->push_back(curBnd);
			}
			else if(curBnd->startB < 0){
				if(curBnd->endB <= onIndex){
					leftVec->push_back(curBnd);
				}
				else{
					spanVec->push_back(curBnd);
				}
			}
			else if(curBnd->endB < 0){
				if(curBnd->startB >= onIndex){
					rightVec->push_back(curBnd);
				}
				else{
					spanVec->push_back(curBnd);
				}
			}
			else{
				if(curBnd->endB <= onIndex){
					leftVec->push_back(curBnd);
				}
				else if(curBnd->startB >= onIndex){
					rightVec->push_back(curBnd);
				}
				else{
					spanVec->push_back(curBnd);
				}
			}
		}
	}
}

PositionDependentAlignCostKDNode* buildPositionDependentKDTree(std::vector<PositionDependentBounds*>* allBounds){
	//run through all possible splits
	int winSplitAxis = PDAG_AXIS_A;
	int winSplitInd = -1;
	int winSplitScore = allBounds->size() * allBounds->size();
	int winSplitBinCount = 1;
	//common storage
	std::vector<PositionDependentBounds*> leftBnd;
	std::vector<PositionDependentBounds*> rightBnd;
	std::vector<PositionDependentBounds*> spanBnd;
	//run through all possible splits
	for(unsigned int i = 0; i<allBounds->size(); i++){
		PositionDependentBounds* curBnd = (*allBounds)[i];
		int curSplitCost; int leftCost; int rightCost; int curSplitBinCount;
#define PD_KDBUILD_CHECKONE(onAxis, onVar) \
		leftBnd.clear(); rightBnd.clear(); spanBnd.clear();\
			calculateSplit(allBounds, onAxis, curBnd->onVar, &leftBnd, &rightBnd, &spanBnd);\
			leftCost = leftBnd.size() + spanBnd.size();\
			rightCost = rightBnd.size() + spanBnd.size();\
			curSplitCost = leftCost*leftCost + rightCost*rightCost;\
			curSplitBinCount = ((leftBnd.size()) ? 1 : 0) + ((rightBnd.size()) ? 1 : 0) + ((spanBnd.size()) ? 1 : 0);\
			if((curSplitCost < winSplitScore) || ((curSplitCost == winSplitScore) && (curSplitBinCount > winSplitBinCount))){\
				winSplitAxis = onAxis;\
				winSplitInd = curBnd->onVar;\
				winSplitScore = curSplitCost;\
				winSplitBinCount = curSplitBinCount;\
			}
		PD_KDBUILD_CHECKONE(PDAG_AXIS_A, startA)
		PD_KDBUILD_CHECKONE(PDAG_AXIS_A, endA)
		PD_KDBUILD_CHECKONE(PDAG_AXIS_B, startB)
		PD_KDBUILD_CHECKONE(PDAG_AXIS_B, endB)
	}
	//if only one bin, go back to default, otherwise respect and recurse
	PositionDependentAlignCostKDNode* toRet = (PositionDependentAlignCostKDNode*)malloc(sizeof(PositionDependentAlignCostKDNode));
	if(winSplitBinCount == 1){
		toRet->splitOn = PDAG_AXIS_A;
		toRet->splitAt = -1;
		toRet->subNodeLesser = 0;
		toRet->subNodeGreatE = 0;
		toRet->numSpan = allBounds->size();
		toRet->allSpans = (PositionDependentBounds**)malloc(allBounds->size() * sizeof(PositionDependentBounds*));
		std::copy(allBounds->begin(), allBounds->end(), toRet->allSpans);
	}
	else{
		leftBnd.clear(); rightBnd.clear(); spanBnd.clear();
			calculateSplit(allBounds, winSplitAxis, winSplitInd, &leftBnd, &rightBnd, &spanBnd);
		toRet->splitOn = winSplitAxis;
		toRet->splitAt = winSplitInd;
		toRet->subNodeLesser = leftBnd.size() ? buildPositionDependentKDTree(&leftBnd) : 0;
		toRet->subNodeGreatE = rightBnd.size() ? buildPositionDependentKDTree(&rightBnd) : 0;
		toRet->numSpan = spanBnd.size();
		toRet->allSpans = (PositionDependentBounds**)malloc(spanBnd.size() * sizeof(PositionDependentBounds*));
		std::copy(spanBnd.begin(), spanBnd.end(), toRet->allSpans);
	}
	return toRet;
}

PositionDependentAlignCostKDNode* parseAsciiPositionDependentSpecification(int numChars, const char* toParse){
	int numLeft = numChars;
	const char* curFoc = toParse;
	const char* nextParse;
	std::vector<AlignCostAffine*> freeOnErr;
#define READ_NEXT_AC_PD(newName) \
	AlignCostAffine* newName;\
	nextParse = parseAffineGapCostSpecification(numLeft, curFoc, &(newName));\
	if(!nextParse){ freeAlignCosts(&freeOnErr); return 0; }\
	freeOnErr.push_back(newName);\
	numLeft = numLeft - (nextParse - curFoc);\
	curFoc = nextParse;
#define READ_NEXT_ITEM_PD(newName) \
	nextParse = curFoc + strspn(curFoc, WHITESPACE); if(!*nextParse){ freeAlignCosts(&freeOnErr); return 0; }\
	int newName = atoi(nextParse);\
	nextParse = strpbrk(nextParse, WHITESPACE); if(!nextParse){ freeAlignCosts(&freeOnErr); return 0; }\
	numLeft = numLeft - (nextParse - curFoc);\
	curFoc = nextParse;
	//read the default spec first
	READ_NEXT_AC_PD(defSpec)
	//get the number of specifications
	READ_NEXT_ITEM_PD(numSpec)
	std::vector<int> astart;
	std::vector<int> aend;
	std::vector<int> bstart;
	std::vector<int> bend;
	std::vector<AlignCostAffine*> allSpecs;
	for(int i = 0; i<numSpec; i++){
		READ_NEXT_ITEM_PD(cas)
		READ_NEXT_ITEM_PD(cae)
		READ_NEXT_ITEM_PD(cbs)
		READ_NEXT_ITEM_PD(cbe)
		READ_NEXT_AC_PD(cspec)
		astart.push_back(cas);
		aend.push_back(cae);
		bstart.push_back(cbs);
		bend.push_back(cbe);
		allSpecs.push_back(cspec);
	}
	//build the bounds
	std::vector<PositionDependentBounds*> allBounds;
	PositionDependentBounds* defBnd = (PositionDependentBounds*)malloc(sizeof(PositionDependentBounds));
	defBnd->startA = -1; defBnd->endA = -1;
	defBnd->startB = -1; defBnd->endB = -1;
	defBnd->priority = 0;
	defBnd->regCosts = defSpec;
	allBounds.push_back(defBnd);
	for(unsigned int i = 0; i<allSpecs.size(); i++){
		PositionDependentBounds* newBnd = (PositionDependentBounds*)malloc(sizeof(PositionDependentBounds));
		newBnd->startA = astart[i]; newBnd->endA = aend[i];
		newBnd->startB = bstart[i]; newBnd->endB = bend[i];
		newBnd->priority = ((astart[i] >= 0) ? 1 : 0) + ((aend[i] >= 0) ? 1 : 0) + ((bstart[i] >= 0) ? 1 : 0) + ((bend[i] >= 0) ? 1 : 0);
		newBnd->regCosts = allSpecs[i];
		allBounds.push_back(newBnd);
	}
	//set up the return
	return buildPositionDependentKDTree(&allBounds);
}

int parseAsciiMultiregionPositionDependentSpecification(int numChars, const char* toParse, std::map<std::string,PositionDependentAlignCostKDNode*>* toFill){
	int curNC = numChars;
	const char* curFoc = toParse;
	while(curNC){
		//get the current name
		if(strchr(WHITESPACE, *curFoc)){
			curFoc++;
			curNC--;
			continue;
		}
		const char* nameEnd = strpbrk(curFoc, WHITESPACE);
		if(!nameEnd){
			return 1;
		}
		//get the characters for the individual region
		const char* aspecEnd = strchr(nameEnd, '|');
		if(!aspecEnd){
			return 1;
		}
		//parse
		PositionDependentAlignCostKDNode* curCost = parseAsciiPositionDependentSpecification(aspecEnd - nameEnd, nameEnd);
		if(!curCost){
			return 1;
		}
		int nameLen = nameEnd - curFoc;
		char* nameCpy = (char*)malloc(nameLen+1);
		memcpy(nameCpy, curFoc, nameLen);
		nameCpy[nameLen] = 0;
		(*toFill)[nameCpy] = curCost;
		free(nameCpy);
		//prepare for the next
		curFoc = aspecEnd + 1;
		curNC = numChars - (curFoc - toParse);
	}
	return 0;
}

void freePositionDependentCostInformation(PositionDependentAlignCostKDNode* toKill){
	//free the sub nodes
	if(toKill->subNodeLesser){
		freePositionDependentCostInformation((PositionDependentAlignCostKDNode*)(toKill->subNodeLesser));
	}
	if(toKill->subNodeGreatE){
		freePositionDependentCostInformation((PositionDependentAlignCostKDNode*)(toKill->subNodeGreatE));
	}
	//free the bounds
	for(int i = 0; i<toKill->numSpan; i++){
		PositionDependentBounds* curBnd = toKill->allSpans[i];
		freeAlignCostAffine(curBnd->regCosts);
		free(curBnd);
	}
	free(toKill->allSpans);
	//free the thing
	free(toKill);
}

PositionDependentAlignCostKDNode* rebasePositionDependentSpecification(PositionDependentAlignCostKDNode* startPoint, int newLowA, int newHigA, int newLowB, int newHigB){
	bool needFiltA = newLowA >= 0;
	bool needFiltB = newLowB >= 0;
	//flatten the tree
		std::vector<PositionDependentBounds*> allFlat;
		flattenPositionDependentKD(startPoint, &allFlat);
	//filter
		PositionDependentBounds* tmpAlloc = (PositionDependentBounds*)malloc(sizeof(PositionDependentBounds));
		std::vector<PositionDependentBounds*> allFilt;
		#define REBASE_ENTRY(entName, relTo) \
			if(curFoc->entName >= 0){\
				tmpAlloc->entName = curFoc->entName - relTo;\
				if(tmpAlloc->entName < 0){\
					tmpAlloc->entName = 0;\
				}\
			}\
			else{\
				tmpAlloc->entName = curFoc->entName;\
			}
		for(unsigned i = 0; i<allFlat.size(); i++){
			PositionDependentBounds* curFoc = allFlat[i];
			if(needFiltA){
				if((curFoc->startA >= 0) && (curFoc->startA >= newHigA)){
					continue;
				}
				if((curFoc->endA >= 0) && (curFoc->endA <= newLowA)){
					continue;
				}
				REBASE_ENTRY(startA, newLowA)
				REBASE_ENTRY(endA, newLowA)
			}
			else{
				tmpAlloc->startA = curFoc->startA;
				tmpAlloc->endA = curFoc->endA;
			}
			if(needFiltB){
				if((curFoc->startB >= 0) && (curFoc->startB >= newHigB)){
					continue;
				}
				if((curFoc->endB >= 0) && (curFoc->endB <= newLowB)){
					continue;
				}
				REBASE_ENTRY(startB, newLowB)
				REBASE_ENTRY(endB, newLowB)
			}
			else{
				tmpAlloc->startB = curFoc->startB;
				tmpAlloc->endB = curFoc->endB;
			}
			tmpAlloc->priority = curFoc->priority;
			tmpAlloc->regCosts = cloneAlignCostAffine(curFoc->regCosts);
			allFilt.push_back(tmpAlloc);
			tmpAlloc = (PositionDependentBounds*)malloc(sizeof(PositionDependentBounds));;
		}
		free(tmpAlloc);
	//build a new tree
		return buildPositionDependentKDTree(&allFilt);
}

void flattenPositionDependentKD(PositionDependentAlignCostKDNode* toFlatten, std::vector<PositionDependentBounds*>* toFill){
	if(toFlatten->subNodeLesser){
		flattenPositionDependentKD((PositionDependentAlignCostKDNode*)(toFlatten->subNodeLesser), toFill);
	}
	if(toFlatten->subNodeGreatE){
		flattenPositionDependentKD((PositionDependentAlignCostKDNode*)(toFlatten->subNodeGreatE), toFill);
	}
	toFill->insert(toFill->end(), toFlatten->allSpans, toFlatten->allSpans + toFlatten->numSpan);
}
