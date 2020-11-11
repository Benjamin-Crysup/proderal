#include "whodun_align.h"

#include <set>
#include <queue>
#include <string.h>
#include <iostream>
#include <stdexcept>
#include <algorithm>

#include "whodun_stringext.h"

AlignCostMangleAction::~AlignCostMangleAction(){}

#define SCRIPT_ACTION_ADD 0
#define SCRIPT_ACTION_SUB 1
#define SCRIPT_ACTION_MUL 2
#define SCRIPT_ACTION_DIV 3
#define SCRIPT_ACTION_SET 4
ScriptAlignCostMangleAction::ScriptAlignCostMangleAction(const ScriptAlignCostMangleAction& toCopy){
	myActs = toCopy.myActs;
	actArguments = toCopy.actArguments;
}
ScriptAlignCostMangleAction& ScriptAlignCostMangleAction::operator=(const ScriptAlignCostMangleAction& toClone){
	if(this == &toClone){return *this;}
	myActs = toClone.myActs;
	actArguments = toClone.actArguments;
	return *this;
}
ScriptAlignCostMangleAction::ScriptAlignCostMangleAction(std::string* myActions){
	const char* curFoc = myActions->c_str();
	while(*curFoc){
		if(strchr(" \t\r\n", *curFoc)){
			curFoc++; continue;
		}
		int opAct;
		switch(*curFoc){
			case '+': opAct = SCRIPT_ACTION_ADD; break;
			case '-': opAct = SCRIPT_ACTION_SUB; break;
			case '*': opAct = SCRIPT_ACTION_MUL; break;
			case '/': opAct = SCRIPT_ACTION_DIV; break;
			case '=': opAct = SCRIPT_ACTION_SET; break;
			default: throw std::runtime_error("Uknown action in mangle.");
		}
		curFoc++;
		while(strchr(" \t\r\n", *curFoc)){
			curFoc++;
		}
		if(strchr("+-0123456789", *curFoc)==0){ throw std::runtime_error("Missing argument to operation in mangle."); }
		intptr_t actArg = atol(curFoc);
		curFoc += strspn(curFoc, "+-0123456789");
		myActs.push_back(opAct);
		actArguments.push_back(actArg);
	}
}
ScriptAlignCostMangleAction::~ScriptAlignCostMangleAction(){}
int ScriptAlignCostMangleAction::performAction(int onCost){
	int toRet = onCost;
	for(uintptr_t i = 0; i<myActs.size(); i++){
		switch(myActs[i]){
			case SCRIPT_ACTION_ADD:
				toRet += actArguments[i];
				break;
			case SCRIPT_ACTION_SUB:
				toRet -= actArguments[i];
				break;
			case SCRIPT_ACTION_MUL:
				toRet *= actArguments[i];
				break;
			case SCRIPT_ACTION_DIV:
				toRet /= actArguments[i];
				break;
			case SCRIPT_ACTION_SET:
				toRet = actArguments[i];
				break;
			default:
				std::cerr << "Da fuq?" << std::endl;
		}
	}
	return toRet;
}
AlignCostMangleAction* ScriptAlignCostMangleAction::cloneMe(){
	return new ScriptAlignCostMangleAction(*this);
}

LinearPairwiseAlignmentIteration::LinearPairwiseAlignmentIteration(LinearPairwiseSequenceAlignment* forAln){
	baseAln = forAln;
}
LinearPairwiseAlignmentIteration::~LinearPairwiseAlignmentIteration(){}

void LinearPairwiseAlignmentIteration::toCigar(std::string* fillStr, uintptr_t* startAddr){
	char numBuff[4*sizeof(uintmax_t)+4];
	//handle the inital
		intptr_t lastRI = aInds[0];
		intptr_t lastSI = bInds[0];
		*startAddr = lastRI;
		if(lastSI){ sprintf(numBuff, "%ju", (uintmax_t)(lastSI)); fillStr->append(numBuff); fillStr->push_back('S'); }
	//look for runs of operations
		std::string fullRun;
		for(uintptr_t ci = 1; ci<aInds.size(); ci++){
			intptr_t curRI = aInds[ci];
			intptr_t curSI = bInds[ci];
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
	//collapse runs
		const char* curFoc = fullRun.c_str();
		while(*curFoc){
			const char* origFoc = curFoc;
			char curLook = *curFoc;
			curFoc++;
			while(*curFoc){
				if(*curFoc != curLook){ break; }
				curFoc++;
			}
			sprintf(numBuff, "%ju", (uintmax_t)(curFoc - origFoc));
			fillStr->append(numBuff);
			fillStr->push_back(curLook);
		}
	//soft clipping at the end
		uintptr_t numSoftEnd = baseAln->seqBs->size() - bInds[bInds.size()-1];
		if(numSoftEnd){ sprintf(numBuff, "%ju", (uintmax_t)(numSoftEnd)); fillStr->append(numBuff); fillStr->push_back('S'); }
}

LinearPairwiseSequenceAlignment::LinearPairwiseSequenceAlignment(){}
LinearPairwiseSequenceAlignment::~LinearPairwiseSequenceAlignment(){}

int LinearPairwiseSequenceAlignment::findAlignmentScores(LinearPairwiseAlignmentIteration* theIter, int numFind, intptr_t* storeCost, intptr_t minScore, intptr_t maxDupDeg){
	std::greater<intptr_t> compMeth;
	unsigned numFindU = numFind;
	uintptr_t numFound = 0;
	intptr_t curMin = minScore;
	intptr_t realLim = minScore;
	LinearPairwiseAlignmentIteration* curIter = theIter;
	startFuzzyIteration(curIter, realLim, maxDupDeg, numFind);
	while(curIter->getNextAlignment()){
		intptr_t* tsIt = std::lower_bound(storeCost, storeCost + numFound, curIter->alnScore, compMeth);
		uintptr_t tsInd = tsIt - storeCost;
		if((tsInd < numFound) && (*tsIt == curIter->alnScore)){ continue; }
		if(tsInd == numFindU){ continue; }
		if(numFound == numFindU){ numFound--; }
		memmove(tsIt + 1, tsIt, (numFound - tsInd)*sizeof(intptr_t));
		*tsIt = curIter->alnScore;
		numFound++;
		if((numFound == numFindU) && (storeCost[numFindU-1] > curMin)){
			curMin = storeCost[numFindU-1];
			realLim = curMin + 1;
			curIter->updateMinimumScore(realLim);
		}
	}
	return numFound;
}

