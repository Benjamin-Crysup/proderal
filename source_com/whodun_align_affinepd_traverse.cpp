#include "whodun_align_affinepd.h"
#include "whodun_align_affinepd_private.h"

#include <algorithm>

int focusStackGuardFlag(int fI, int fJ, int dirFlag){
	int liveDirs = dirFlag;
	//guard here
	bool fI0 = fI == 0; bool fIg0 = fI > 0; bool fI1 = fI == 1; bool fIg1 = fI > 1;
	bool fJ0 = fJ == 0; bool fJg0 = fJ > 0; bool fJ1 = fJ == 1; bool fJg1 = fJ > 1;
	//match match	: (i==1 && j==1) || (i>1 && j>1)
	if(!((fIg1 && fJg1) || (fI1 && fJ1)))
		{ liveDirs = liveDirs & ~ALIGN_NEED_MATCH_MATCH; }
	//match skipa	: j>0 && i>1
	if(!(fJg0 && fIg1))
		{ liveDirs = liveDirs & ~ALIGN_NEED_MATCH_SKIPA; }
	//match skipb	: i>0 && j>1
	if(!(fIg0 && fJg1))
		{ liveDirs = liveDirs & ~ALIGN_NEED_MATCH_SKIPB; }
	//skipa match	: (i>1 && j>0) || (i==1 && j==0)
	if(!((fIg1 && fJg0) || (fI1 && fJ0)))
		{ liveDirs = liveDirs & ~ALIGN_NEED_SKIPA_MATCH; }
	//skipa skipa	: (i>1)
	if(!(fIg1))
		{ liveDirs = liveDirs & ~ALIGN_NEED_SKIPA_SKIPA; }
	//skipa skipb	: i>0 && j>0
	if(!(fIg0 && fJg0))
		{ liveDirs = liveDirs & ~ALIGN_NEED_SKIPA_SKIPB; }
	//skipb match	: (i>0 && j>1) || (i==0 && j==1)
	if(!((fIg0 && fJg1) || (fI0 && fJ1)))
		{ liveDirs = liveDirs & ~ALIGN_NEED_SKIPB_MATCH; }
	//skipb skipa	: i>0 && j>0
	if(!(fIg0 && fJg0))
		{ liveDirs = liveDirs & ~ALIGN_NEED_SKIPB_SKIPA; }
	//skipb skipb	: (j>1)
	if(!(fJg1))
		{ liveDirs = liveDirs & ~ALIGN_NEED_SKIPB_SKIPB; }
	return liveDirs;
}

FocusStackStarts::FocusStackStarts(){
}

FocusStackStarts::~FocusStackStarts(){
	for(unsigned int i = 0; i<startingLocs.size(); i++){
		delete(startingLocs[i]);
	}
}

void FocusStackStarts::addStartingLocation(int atI, int atJ, int inDirs, int atCost){
	std::greater<int> compMeth;
	FocusStackEntry addStack = NEW_FOCUS_STACK_ENTRY(atI, atJ, inDirs, atCost, 0);
	//find the cost in the list of costs
	std::vector<int>::iterator insLoc = std::lower_bound(startingLocCosts.begin(), startingLocCosts.end(), atCost, compMeth);
	unsigned int insIndex = insLoc - startingLocCosts.begin();
	if(insIndex >= startingLocCosts.size()){
		std::vector<FocusStackEntry>* addVec = new std::vector<FocusStackEntry>();
		addVec->push_back(addStack);
		startingLocs.push_back(addVec);
		startingLocCosts.push_back(atCost);
	}
	else{
		if(atCost != startingLocCosts[insIndex]){
			std::vector<FocusStackEntry>* addVec = new std::vector<FocusStackEntry>();
			addVec->push_back(addStack);
			startingLocs.insert(startingLocs.begin() + insIndex, addVec);
			startingLocCosts.insert(startingLocCosts.begin() + insIndex, atCost);
		}
		else{
			startingLocs[insIndex]->push_back(addStack);
		}
	}
}

void FocusStackStarts::limitStartingLocationScores(int toNumber){
	while(startingLocs.size() > (unsigned int)toNumber){
		std::vector<FocusStackEntry>* toKill = startingLocs[startingLocs.size()-1];
		delete(toKill);
		startingLocs.pop_back();
		startingLocCosts.pop_back();
	}
}

void FocusStackStarts::limitStartingLocationsByScore(int minScore){
	while(startingLocs.size() && (startingLocCosts[startingLocs.size()-1] < minScore)){
		std::vector<FocusStackEntry>* toKill = startingLocs[startingLocs.size()-1];
		delete(toKill);
		startingLocs.pop_back();
		startingLocCosts.pop_back();
	}
}


bool AlignmentIterationState::hasPath(){
	return alnStackSize != 0;
}

bool AlignmentIterationState::pathTerminal(){
	FocusStackEntry* curFoc = (alnStack + alnStackSize - 1);
	int pi = curFoc->focI;
	int pj = curFoc->focJ;
	switch(forProb->numEnds){
		case 0:
			if(curFoc->liveDirs & ALIGN_NEED_SKIPA){
				return (forProb->skipATable[pi][pj] == 0);
			}
			else if(curFoc->liveDirs & ALIGN_NEED_MATCH){
				return (forProb->matchTable[pi][pj] == 0);
			}
			else if(curFoc->liveDirs & ALIGN_NEED_SKIPB){
				return (forProb->skipBTable[pi][pj] == 0);
			}
			else{
				return (forProb->costTable[pi][pj] == 0);
			}
		case 2:
			return (pi == 0) || (pj == 0);
		case 4:
			return (pi == 0) && (pj == 0);
	}
	return 0;
}

int AlignmentIterationState::currentPathScore(){
	return (alnStack + alnStackSize - 1)->pathScore;
}

int AlignmentIterationState::currentPathEndScore(){
	return dieHereScore;
}

void AlignmentIterationState::iterateNextPathStep(){
	bool negToZero = forProb->numEnds == 0;
	//if it hit an end, draw back (do not go forward)
	if(alnStackSize && pathTerminal()){
		alnStackSize--;
	}
	//might need to do the following multiple times.
	tailRecursionTgt:
	lastIterChange = false;
	while(alnStackSize && ((alnStack + alnStackSize - 1)->liveDirs == 0)){
		alnStackSize--;
	}
	if(alnStackSize == 0){
		if(waitingStart.size()){
			*alnStack = waitingStart[waitingStart.size()-1];
			alnStackSize = 1;
			waitingStart.pop_back();
			dieHereScore = 0;
			lastIterChange = true;
		}
		else{
			return;
		}
	}
	else{
		FocusStackEntry* curFoc = (alnStack + alnStackSize - 1);
		int li = curFoc->focI;
		int lj = curFoc->focJ;
		#define DO_PUSH(curDirFlag, lookTable, checkTable, offI, offJ, nextDir) \
			curFoc->liveDirs = curFoc->liveDirs & (~curDirFlag);\
			int** lookTable = forProb->lookTable;\
			int** checkTable = forProb->checkTable;\
			if(negToZero && (checkTable[li][lj] == 0)){ goto tailRecursionTgt; }\
			int newScore = curFoc->pathScore + lookTable[li][lj] - checkTable[li][lj];\
			if(newScore != curFoc->pathScore){ lastIterChange = true; }\
				else{ lastIterChange = curFoc->seenDef; curFoc->seenDef = 1; }\
			*(alnStack + alnStackSize) = NEW_FOCUS_STACK_ENTRY(li - offI, lj - offJ, nextDir, newScore, curDirFlag);\
			alnStackSize++;
		#define POST_PUSH_ZCHECK(nextTable, offI, offJ, taction, faction) \
			if(negToZero){\
				int ptabVal = forProb->nextTable[li-offI][lj-offJ];\
				if(ptabVal > 0){\
					taction\
				}\
				else{\
					faction\
				}\
			}
		#define POST_PUSH_TACTION_MATCH dieHereScore = newScore - ptabVal;
		#define POST_PUSH_FACTION_MATCH dieHereScore = newScore;
		#define POST_PUSH_FACTION_SKIP(offI, offJ) \
			if((li-offI) && (lj-offJ)){\
				alnStackSize--;\
				goto tailRecursionTgt;\
			}\
			else{\
				dieHereScore = newScore;\
			}
		#define POST_PUSH_TACTION_SKIP_OPEN \
			AlignCostAffine* curCost = getPositionDependentAlignmentCosts(forProb->alnCosts, li, lj);\
			dieHereScore = newScore - ptabVal + curCost->openCost;
		#define POST_PUSH_TACTION_SKIP_CLOSE(offI, offJ) \
			AlignCostAffine* othCost = getPositionDependentAlignmentCosts(forProb->alnCosts, li-offI, lj-offJ);\
			dieHereScore = newScore - ptabVal + -(othCost->closeCost);
		if(curFoc->liveDirs & ALIGN_NEED_SKIPA){
			if(curFoc->liveDirs & ALIGN_NEED_SKIPA_SKIPA){
				DO_PUSH(ALIGN_NEED_SKIPA_SKIPA, skipASkipATable, skipATable, 1, 0, ALIGN_NEED_SKIPA)
				POST_PUSH_ZCHECK(skipATable, 1, 0, POST_PUSH_TACTION_SKIP_OPEN, POST_PUSH_FACTION_SKIP(1,0))
			}
			else if(curFoc->liveDirs & ALIGN_NEED_SKIPA_MATCH){
				DO_PUSH(ALIGN_NEED_SKIPA_MATCH, skipAMatchTable, skipATable, 1, 0, ALIGN_NEED_MATCH)
				POST_PUSH_ZCHECK(matchTable, 1, 0, POST_PUSH_TACTION_MATCH, POST_PUSH_FACTION_MATCH)
			}
			else if(curFoc->liveDirs & ALIGN_NEED_SKIPA_SKIPB){
				DO_PUSH(ALIGN_NEED_SKIPA_SKIPB, skipASkipBTable, skipATable, 1, 0, ALIGN_NEED_SKIPB)
				POST_PUSH_ZCHECK(skipBTable, 1, 0, POST_PUSH_TACTION_SKIP_CLOSE(1,0), POST_PUSH_FACTION_SKIP(1,0))
			}
		}
		else if(curFoc->liveDirs & ALIGN_NEED_MATCH){
			if(curFoc->liveDirs & ALIGN_NEED_MATCH_SKIPA){
				DO_PUSH(ALIGN_NEED_MATCH_SKIPA, matchSkipATable, matchTable, 1, 1, ALIGN_NEED_SKIPA)
				POST_PUSH_ZCHECK(skipATable, 1, 1, POST_PUSH_TACTION_SKIP_CLOSE(1,1), POST_PUSH_FACTION_SKIP(1,1))
			}
			else if(curFoc->liveDirs & ALIGN_NEED_MATCH_MATCH){
				DO_PUSH(ALIGN_NEED_MATCH_MATCH, matchMatchTable, matchTable, 1, 1, ALIGN_NEED_MATCH)
				POST_PUSH_ZCHECK(matchTable, 1, 1, POST_PUSH_TACTION_MATCH, POST_PUSH_FACTION_MATCH)
			}
			else if(curFoc->liveDirs & ALIGN_NEED_MATCH_SKIPB){
				DO_PUSH(ALIGN_NEED_MATCH_SKIPB, matchSkipBTable, matchTable, 1, 1, ALIGN_NEED_SKIPB)
				POST_PUSH_ZCHECK(skipBTable, 1, 1, POST_PUSH_TACTION_SKIP_CLOSE(1,1), POST_PUSH_FACTION_SKIP(1,1))
			}
		}
		else if(curFoc->liveDirs & ALIGN_NEED_SKIPB){
			if(curFoc->liveDirs & ALIGN_NEED_SKIPB_SKIPA){
				DO_PUSH(ALIGN_NEED_SKIPB_SKIPA, skipBSkipATable, skipBTable, 0, 1, ALIGN_NEED_SKIPA)
				POST_PUSH_ZCHECK(skipATable, 0, 1, POST_PUSH_TACTION_SKIP_CLOSE(0,1), POST_PUSH_FACTION_SKIP(0,1))
			}
			else if(curFoc->liveDirs & ALIGN_NEED_SKIPB_MATCH){
				DO_PUSH(ALIGN_NEED_SKIPB_MATCH, skipBMatchTable, skipBTable, 0, 1, ALIGN_NEED_MATCH)
				POST_PUSH_ZCHECK(matchTable, 0, 1, POST_PUSH_TACTION_MATCH, POST_PUSH_FACTION_MATCH)
			}
			else if(curFoc->liveDirs & ALIGN_NEED_SKIPB_SKIPB){
				DO_PUSH(ALIGN_NEED_SKIPB_SKIPB, skipBSkipBTable, skipBTable, 0, 1, ALIGN_NEED_SKIPB)
				POST_PUSH_ZCHECK(skipBTable, 0, 1, POST_PUSH_TACTION_SKIP_OPEN, POST_PUSH_FACTION_SKIP(0,1))
			}
		}
	}
	//std::cout << curStack[curStack.size()-1].pathScore << " " << dieHereScore << " ";
	//for(unsigned i = 0; i<curStack.size(); i++){
	//	std::cout << "(" << curStack[i].focI << "," << curStack[i].focJ << ")";
	//}
	//std::cout << std::endl;
}

void AlignmentIterationState::abandonPath(){
	//std::cout << "ABANDON" << std::endl;
	alnStackSize--;
	iterateNextPathStep();
}

int AlignmentIterationState::dumpPath(int* aInds, int* bInds){
	int getInd = alnStackSize-1;
	for(int i = 0; i<alnStackSize; i++){
		FocusStackEntry* curEnt = alnStack + getInd;
		aInds[i] = curEnt->focI;
		bInds[i] = curEnt->focJ;
		getInd--;
	}
	//std::cout << "DUMP" << std::endl;
	return alnStackSize;
}

bool AlignmentIterationState::lastIterationChangedPath(){
	return lastIterChange;
}

AlignmentIterationState::AlignmentIterationState(FocusStackStarts* toStart, AGPDAlignProblem* forPro){
	forProb = forPro;
	unsigned i = toStart->startingLocs.size();
	while(i){
		i--;
		std::vector<FocusStackEntry>* curSLs = toStart->startingLocs[i];
		waitingStart.insert(waitingStart.begin(), curSLs->begin(), curSLs->end());
	}
	alnStack = (FocusStackEntry*)malloc(sizeof(FocusStackEntry)*(forPro->lenA + forPro->lenB + 2));
	iterateNextPathStep();
}

AlignmentIterationState::~AlignmentIterationState(){
	free(alnStack);
}

FocusStackStarts* findStartingPositions(AGPDAlignProblem* forProb){
	//save some info
		//int** costTable = forProb->costTable;
		int** matchTable = forProb->matchTable;
		int** skipATable = forProb->skipATable;
		int** skipBTable = forProb->skipBTable;
		int lenA = forProb->lenA;
		int lenB = forProb->lenB;
	AlignCostAffine* curCost;
	FocusStackStarts* toRet = new FocusStackStarts();
	//add a starting location for each direction: add close penalty if applicable
	switch(forProb->numEnds){
		case 4:
			curCost = getPositionDependentAlignmentCosts(forProb->alnCosts, lenA-1, lenB-1);
			toRet->addStartingLocation(lenA, lenB, ALIGN_NEED_MATCH, matchTable[lenA][lenB]);
			toRet->addStartingLocation(lenA, lenB, ALIGN_NEED_SKIPA, skipATable[lenA][lenB] + curCost->closeCost);
			toRet->addStartingLocation(lenA, lenB, ALIGN_NEED_SKIPB, skipBTable[lenA][lenB] + curCost->closeCost);
			break;
		case 2:
			//add all the right and bottom as starting points
			for(int j = 0; j<=lenB; j++){
				curCost = getPositionDependentAlignmentCosts(forProb->alnCosts, lenA-1, j-1);
				if(j){ toRet->addStartingLocation(lenA, j, ALIGN_NEED_MATCH, matchTable[lenA][j]); }
				toRet->addStartingLocation(lenA, j, ALIGN_NEED_SKIPA, skipATable[lenA][j] + (j ? curCost->closeCost : 0));
				//if(j){ toRet->addStartingLocation(lenA, j, ALIGN_NEED_SKIPB, skipBTable[lenA][j] + curCost->closeCost); }
			}
			for(int i = 0; i<lenA; i++){
				curCost = getPositionDependentAlignmentCosts(forProb->alnCosts, i-1, lenB-1);
				if(i){ toRet->addStartingLocation(i, lenB, ALIGN_NEED_MATCH, matchTable[i][lenB]); }
				//if(i){ toRet->addStartingLocation(i, lenB, ALIGN_NEED_SKIPA, skipATable[i][lenB] + curCost->closeCost); }
				toRet->addStartingLocation(i, lenB, ALIGN_NEED_SKIPB, skipBTable[i][lenB] + (i ? curCost->closeCost : 0));
			}
			break;
		case 0:
			for(int i = 0; i<=lenA; i++){
				for(int j = 0; j<=lenB; j++){
					curCost = getPositionDependentAlignmentCosts(forProb->alnCosts, i-1, j-1);
					if(i&&j){ toRet->addStartingLocation(i, j, ALIGN_NEED_MATCH, matchTable[i][j]); }
					if(i && j && (j<lenB)){ toRet->addStartingLocation(i, j, ALIGN_NEED_SKIPA, skipATable[i][j] + curCost->closeCost); }
					if(j && i && (i<lenA)){ toRet->addStartingLocation(i, j, ALIGN_NEED_SKIPB, skipBTable[i][j] + curCost->closeCost); }
				}
			}
			break;
		default:
			delete(toRet);
			return 0;
	}
	return toRet;
}
