#include "whodun_align_affinepd.h"
#include "whodun_align_affinepd_private.h"

#include <vector>
#include <iostream>
#include <string.h>
#include <algorithm>

int findAGPDAlignmentScores(AGPDAlignProblem* forProb, int numFind, int* storeCost, int maxDupDeg){
	std::greater<int> compMeth;
	std::vector<int>::iterator insLoc;
	bool needPreClose = forProb->numEnds == 0;
	//get the starting locations
	FocusStackStarts* startLocs = findStartingPositions(forProb);
	startLocs->limitStartingLocationScores(numFind);
	//end quickly if only optimal wanted
	if(numFind == 1){
		*storeCost = startLocs->startingLocCosts[0];
		delete(startLocs);
		return 1;
	}
	//start an iteration
	AlignmentIterationState curIter(startLocs, forProb);
	delete(startLocs);
	//note all scores along the way
	std::vector<int> allScore;
	std::vector<int> allScoreSeen;
	while(curIter.hasPath()){
		//see whether the current score needs adding
			bool needConsiderAdd = false;
			int considerScore = 0;
			if(needPreClose){
				considerScore = curIter.currentPathEndScore();
				needConsiderAdd = considerScore > 0;
			}
			else if(curIter.pathTerminal()){
				needConsiderAdd = true;
				considerScore = curIter.currentPathScore();
			}
			if(needConsiderAdd){
				insLoc = std::lower_bound(allScore.begin(), allScore.end(), considerScore, compMeth);
				if(insLoc == allScore.end()){
					if(allScore.size() < (unsigned)numFind){ allScore.push_back(considerScore); allScoreSeen.push_back(0); }
				}
				else if(*insLoc != considerScore){
					int insInd = insLoc - allScore.begin();
					allScore.insert(insLoc, considerScore);
					allScoreSeen.insert(allScoreSeen.begin() + insInd, 0);
					if(allScore.size() > (unsigned int)numFind){ allScore.pop_back(); allScoreSeen.pop_back(); }
				}
			}
		//if the current path is off the end of the list, abandon
		//otherwise, continue (and add if not already in)
			bool abandoned = false;
			int pathScore = curIter.currentPathScore();
			insLoc = std::lower_bound(allScore.begin(), allScore.end(), pathScore, compMeth);
			int insInd = insLoc - allScore.begin();
			if(insLoc == allScore.end()){
				if(allScore.size() >= (unsigned)numFind){
					curIter.abandonPath();
					abandoned = true;
				}
				else{
					allScore.push_back(pathScore);
					allScoreSeen.push_back(0);
				}
			}
			else if(*insLoc != pathScore){
				allScore.insert(insLoc, pathScore);
				allScoreSeen.insert(allScoreSeen.begin() + insInd, 0);
				if(allScore.size() > (unsigned int)numFind){ allScore.pop_back(); allScoreSeen.pop_back(); }
			}
		//check whether to abandon due to seeing it too many times
			if(maxDupDeg && !abandoned && curIter.lastIterationChangedPath()){
				if(allScoreSeen[insInd] >= maxDupDeg){
					curIter.abandonPath();
					abandoned = true;
				}
				else{
					allScoreSeen[insInd]++;
				}
			}
		//continue
			if(!abandoned){
				curIter.iterateNextPathStep();
			}
	}
	//if local, remove the zero score, if present
	if(forProb->numEnds == 0){
		if(allScore.size()){
			if(allScore[allScore.size()-1] == 0){
				allScore.pop_back();
			}
		}
	}
	//copy to result
	std::copy(allScore.begin(), allScore.end(), storeCost);
	return allScore.size();
}

/**An alignment iteration.*/
typedef struct{
	/**THe problem in question.*/
	AGPDAlignProblem* forProb;
	/**The minimum score to look at.*/
	int minScore;
	/**The generic iterator.*/
	AlignmentIterationState* mainState;
	/**The maximum number of times to hit a degraded score.*/
	int maxDupDeg;
	/**The scores.*/
	std::vector<int>* allScore;
	/**The number of times each has been seen.*/
	std::vector<int>* allScoreSeen;
} AlignmentIterator;

void* initializeAGPDOptimalAlignmentIteration(AGPDAlignProblem* forProb){
	//find the starting locations
	FocusStackStarts* startLocs = findStartingPositions(forProb);
	startLocs->limitStartingLocationsByScore(startLocs->startingLocCosts[0]);
	//set up the iterator
	AlignmentIterator* newIter = (AlignmentIterator*)malloc(sizeof(AlignmentIterator));
	newIter->forProb = forProb;
	newIter->minScore = startLocs->startingLocCosts[0];
	newIter->mainState = new AlignmentIterationState(startLocs, forProb);
	delete(startLocs);
	//no limit on degrade hits
	newIter->maxDupDeg = 0;
	newIter->allScore = new std::vector<int>();
	newIter->allScoreSeen = new std::vector<int>();
	return newIter;
}

void* initializeAGPDFuzzedAlignmentIteration(AGPDAlignProblem* forProb, int minScore, int maxDupDeg){
	//find the starting locations
	FocusStackStarts* startLocs = findStartingPositions(forProb);
	startLocs->limitStartingLocationsByScore(minScore);
	//set up the iterator
	AlignmentIterator* newIter = (AlignmentIterator*)malloc(sizeof(AlignmentIterator));
	newIter->forProb = forProb;
	newIter->minScore = minScore;
	newIter->mainState = new AlignmentIterationState(startLocs, forProb);
	delete(startLocs);
	//set limit on degrade hits
	newIter->maxDupDeg = maxDupDeg;
	newIter->allScore = new std::vector<int>();
	newIter->allScoreSeen = new std::vector<int>();
	return newIter;
}

int getNextAGPDAlignment(void* iterator, AlignmentFiller* storeLoc){
	std::greater<int> compMeth;
	std::vector<int>::iterator insLoc;
	AlignmentIterator* curIter = (AlignmentIterator*)iterator;
	int minScore = curIter->minScore;
	AlignmentIterationState* mainState = curIter->mainState;
	bool needPreClose = curIter->forProb->numEnds == 0;
	bool gotThing = false;
	while(!gotThing){
		if(mainState->hasPath()){
			//check getting the thing
			if(needPreClose){
				if(mainState->currentPathEndScore() >= minScore){
					gotThing = true;
					storeLoc->alnLen = mainState->dumpPath(storeLoc->aInds, storeLoc->bInds);
					storeLoc->alnScore = mainState->currentPathEndScore();
				}
			}
			else if(mainState->pathTerminal()){
				if(mainState->currentPathScore() >= minScore){
					gotThing = true;
					storeLoc->alnLen = mainState->dumpPath(storeLoc->aInds, storeLoc->bInds);
					storeLoc->alnScore = mainState->currentPathScore();
				}
			}
			//abandon if score too low
			if(mainState->currentPathScore() < minScore){
				mainState->abandonPath();
				continue;
			}
			//check for abandon given it's not sufficiently new
			if(curIter->maxDupDeg && mainState->lastIterationChangedPath()){
				int curPS = mainState->currentPathScore();
				//try to add
				insLoc = std::lower_bound(curIter->allScore->begin(), curIter->allScore->end(), curPS, compMeth);
				unsigned insInd = insLoc - curIter->allScore->begin();
				if(insInd >= curIter->allScore->size()){
					curIter->allScore->push_back(curPS);
					curIter->allScoreSeen->push_back(0);
				}
				else if((*(curIter->allScore))[insInd] != curPS){
					curIter->allScore->insert(curIter->allScore->begin() + insInd, curPS);
					curIter->allScoreSeen->insert(curIter->allScoreSeen->begin() + insInd, 0);
				}
				//check for abandon
				if((*(curIter->allScoreSeen))[insInd] >= curIter->maxDupDeg){
					mainState->abandonPath();
					continue;
				}
				(*(curIter->allScoreSeen))[insInd]++;
			}
			//move to next
			mainState->iterateNextPathStep();
		}
		else{
			return 0;
		}
	}
	return 1;
}

void terminateAGPDAlignmentIteration(void* toTerm){
	AlignmentIterator* curIter = (AlignmentIterator*)toTerm;
	delete(curIter->mainState);
	delete(curIter->allScore);
	delete(curIter->allScoreSeen);
	free(curIter);
}

void alignmentToCIGAR(int numEnt, int* refInds, int* seqInds, int seqLen, std::string* fillStr, long int* startAddr){
	std::string fullRun;
	*startAddr = *refInds;
	//handle soft clipping on the front end
	for(int i = 0; i<*seqInds; i++){
		fullRun.push_back('S');
	}
	//handle the middle
	int lastRI = refInds[0];
	int lastSI = seqInds[0];
	for(int ci = 1; ci<numEnt; ci++){
		int curRI = refInds[ci];
		int curSI = seqInds[ci];
		if(curRI != lastRI){
			if(curSI != lastSI){
				fullRun.push_back('M');
			}
			else{
				fullRun.push_back('D');
			}
		}
		else{
			fullRun.push_back('I');
		}
		lastRI = curRI;
		lastSI = curSI;
	}
	//handle soft clipping on the back end
	for(int i = seqInds[numEnt-1]; i<seqLen; i++){
		fullRun.push_back('S');
	}
	//collapse things
	char curOp[2];
	curOp[1] = 0;
	const char* curFoc = fullRun.c_str();
	while(*curFoc){
		curOp[0] = *curFoc;
		int numOp = strspn(curFoc, curOp);
		std::string numOpS = std::to_string(numOp);
		fillStr->append(numOpS);
		fillStr->push_back(*curFoc);
		curFoc += numOp;
	}
}

