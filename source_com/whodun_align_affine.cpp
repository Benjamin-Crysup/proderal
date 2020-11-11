#include "whodun_align_affine.h"

#include <set>
#include <math.h>
#include <iostream>
#include <string.h>
#include <stdexcept>

#include "whodun_parse.h"
#include "whodun_stringext.h"

#define WHITESPACE " \t\r\n"

AlignCostAffine::AlignCostAffine(){
	allMMCost = 0;
	numLiveChars = 0;
	openCost = 0;
	extendCost = 0;
	closeCost = 0;
}

AlignCostAffine::AlignCostAffine(const AlignCostAffine& toCopy){
	numLiveChars = toCopy.numLiveChars;
	openCost = toCopy.openCost;
	extendCost = toCopy.extendCost;
	closeCost = toCopy.closeCost;
	memcpy(charMap, toCopy.charMap, 256*sizeof(short));
	allMMCost = (int**)malloc(numLiveChars*sizeof(int*) + numLiveChars*numLiveChars*sizeof(int));
	int* othMCFoc = (int*)(toCopy.allMMCost + numLiveChars);
	int* curMCFoc = (int*)(allMMCost + numLiveChars);
	memcpy(curMCFoc, othMCFoc, numLiveChars*numLiveChars*sizeof(int));
	for(int i = 0; i<numLiveChars; i++){
		allMMCost[i] = curMCFoc;
		curMCFoc += numLiveChars;
	}
}
AlignCostAffine& AlignCostAffine::operator=(const AlignCostAffine& toClone){
	if(this == &toClone){return *this;}
	int origNLC = numLiveChars;
	numLiveChars = toClone.numLiveChars;
	openCost = toClone.openCost;
	extendCost = toClone.extendCost;
	closeCost = toClone.closeCost;
	memcpy(charMap, toClone.charMap, 256*sizeof(short));
	int* othMCFoc = (int*)(toClone.allMMCost + numLiveChars);
	if((allMMCost == 0) || (origNLC != toClone.numLiveChars)){
		if(allMMCost){free(allMMCost);}
		allMMCost = (int**)malloc(numLiveChars*sizeof(int*) + numLiveChars*numLiveChars*sizeof(int));
	}
	int* curMCFoc = (int*)(allMMCost + numLiveChars);
	memcpy(curMCFoc, othMCFoc, numLiveChars*numLiveChars*sizeof(int));
	for(int i = 0; i<numLiveChars; i++){
		allMMCost[i] = curMCFoc;
		curMCFoc += numLiveChars;
	}
	return *this;
}

AlignCostAffine::~AlignCostAffine(){
	if(allMMCost){free(allMMCost);}
}

const char* AlignCostAffine::parseSpecification(const char* parseS, const char* parseE){
	free(allMMCost); allMMCost = 0;
	const char* curFoc = parseS;
	const char* nextFoc;
	std::string curEntP;
	int wsLen = strlen(WHITESPACE);
	#define AGCPARSE_SCANNEXT \
		curEntP.clear(); \
		curFoc = curFoc + memspn(curFoc, parseE-curFoc, WHITESPACE, wsLen);\
		nextFoc = curFoc + memcspn(curFoc, parseE-curFoc, WHITESPACE, wsLen);\
		curEntP.insert(curEntP.end(), curFoc, nextFoc);\
		if(curEntP.size() == 0){ throw std::runtime_error("Missing required item for affine gap cost."); }\
		curFoc = nextFoc;
	//basic costs
		AGCPARSE_SCANNEXT openCost = atol(curEntP.c_str());
		AGCPARSE_SCANNEXT extendCost = atol(curEntP.c_str());
		AGCPARSE_SCANNEXT closeCost = atol(curEntP.c_str());
	//character set
		AGCPARSE_SCANNEXT int numChars = atol(curEntP.c_str());
		memset(charMap, 0, 256*sizeof(short));
		for(int i = 0; i<numChars; i++){
			AGCPARSE_SCANNEXT
			charMap[(unsigned char)(atol(curEntP.c_str()))] = i+1;
		}
		numLiveChars = numChars + 1;
	//character costs
		allMMCost = (int**)malloc(numLiveChars*sizeof(int*) + numLiveChars*numLiveChars*sizeof(int));
		int* curMCFoc = (int*)(allMMCost + numLiveChars);
		for(int i = 0; i<numLiveChars; i++){
			allMMCost[i] = curMCFoc;
			curMCFoc += numLiveChars;
		}
		for(int i = 0; i<numLiveChars; i++){
			allMMCost[0][i] = 0;
			allMMCost[i][0] = 0;
		}
		for(int i = 0; i<numChars; i++){
			for(int j = 0; j<numChars; j++){
				AGCPARSE_SCANNEXT
				allMMCost[i+1][j+1] = atol(curEntP.c_str());
			}
		}
	return curFoc;
}

AlignCostAffineMangleSet::AlignCostAffineMangleSet(){
	openAction = 0;
	extAction = 0;
	closeAction = 0;
}

AlignCostAffineMangleSet::AlignCostAffineMangleSet(const AlignCostAffineMangleSet& toCopy){
	if(toCopy.openAction){ openAction = toCopy.openAction->cloneMe(); }else{ openAction = 0; }
	if(toCopy.extAction){ extAction = toCopy.extAction->cloneMe(); }else{ extAction = 0; }
	if(toCopy.closeAction){ closeAction = toCopy.closeAction->cloneMe(); }else{ closeAction = 0; }
	fromChars = toCopy.fromChars;
	toChars = toCopy.toChars;
	mmActions.resize(toCopy.mmActions.size());
	for(uintptr_t i = 0; i<mmActions.size(); i++){ mmActions[i] = toCopy.mmActions[i]->cloneMe(); }
}

AlignCostAffineMangleSet& AlignCostAffineMangleSet::operator=(const AlignCostAffineMangleSet& toClone){
	if(this == &toClone){return *this;}
	if(openAction){ delete(openAction); }
	if(extAction){ delete(extAction); }
	if(closeAction){ delete(closeAction); }
	for(uintptr_t i = 0; i<mmActions.size(); i++){ delete(mmActions[i]); }
	
	if(toClone.openAction){ openAction = toClone.openAction->cloneMe(); }else{ openAction = 0; }
	if(toClone.extAction){ extAction = toClone.extAction->cloneMe(); }else{ extAction = 0; }
	if(toClone.closeAction){ closeAction = toClone.closeAction->cloneMe(); }else{ closeAction = 0; }
	fromChars = toClone.fromChars;
	toChars = toClone.toChars;
	mmActions.resize(toClone.mmActions.size());
	for(uintptr_t i = 0; i<mmActions.size(); i++){ mmActions[i] = toClone.mmActions[i]->cloneMe(); }
	return *this;
}

std::ostream& operator<<(std::ostream& os, const AlignCostAffine& toOut){
	os << toOut.openCost << " " << toOut.extendCost << " " << toOut.closeCost;
	os << " " << toOut.numLiveChars;
	for(int i = 0; i<256; i++){
		if(toOut.charMap[i]){
			os << " " << i;
		}
	}
	for(int i = 1; i<toOut.numLiveChars; i++){
		for(int j = 1; j<toOut.numLiveChars; j++){
			os << " " << toOut.allMMCost[i][j];
		}
	}
	return os;
}

AlignCostAffineMangleSet::~AlignCostAffineMangleSet(){
	if(openAction){ delete(openAction); }
	if(extAction){ delete(extAction); }
	if(closeAction){ delete(closeAction); }
	for(uintptr_t i = 0; i<mmActions.size(); i++){ delete(mmActions[i]); }
}

void AlignCostAffineMangleSet::performActions(AlignCostAffine* onCost){
	for(uintptr_t i = 0; i<fromChars.size(); i++){
		int mapCF = onCost->charMap[0x00FF & fromChars[i]];
		int mapCT = onCost->charMap[0x00FF & toChars[i]];
		onCost->allMMCost[mapCF][mapCT] = mmActions[i]->performAction(onCost->allMMCost[mapCF][mapCT]);
	}
	if(openAction){
		onCost->openCost = openAction->performAction(onCost->openCost);
	}
	if(extAction){
		onCost->extendCost = extAction->performAction(onCost->extendCost);
	}
	if(closeAction){
		onCost->closeCost = closeAction->performAction(onCost->closeCost);
	}
}

void AlignCostAffineMangleSet::parseMangleSet(const char* parseS, const char* parseE){
	if(openAction){ delete(openAction); }
	if(extAction){ delete(extAction); }
	if(closeAction){ delete(closeAction); }
	for(uintptr_t i = 0; i<mmActions.size(); i++){ delete(mmActions[i]); }
	fromChars.clear();
	toChars.clear();
	mmActions.clear();
	std::string tmpParse;
	std::set< std::pair<char,char> > seenMMs;
	#define CHECK_OUT_OF_ROAD if(curFoc >= nextFoc){ throw new std::runtime_error("Malformed specification for affine mangle."); }
	const char* curFoc = parseS;
	while(curFoc < parseE){
		if(strchr(" \r\t\n", *curFoc)){ curFoc++; continue; }
		const char* nextFoc = (const char*)memchr(curFoc, '\n', parseE - curFoc);
			nextFoc = nextFoc ? (nextFoc + 1) : parseE;
		if(*curFoc == '~'){
			curFoc++; CHECK_OUT_OF_ROAD
			while(strchr(" \r\t\n", *curFoc)){ curFoc++; CHECK_OUT_OF_ROAD }
			tmpParse.clear(); while(strchr(" \r\t\n", *curFoc)==0){ tmpParse.push_back(*curFoc); curFoc++; if(curFoc >= nextFoc){break;}}
			char mangF = atol(tmpParse.c_str());
			CHECK_OUT_OF_ROAD
			while(strchr(" \r\t\n", *curFoc)){ curFoc++; CHECK_OUT_OF_ROAD }
			tmpParse.clear(); while(strchr(" \r\t\n", *curFoc)==0){ tmpParse.push_back(*curFoc); curFoc++; if(curFoc >= nextFoc){break;}}
			char mangT = atol(tmpParse.c_str());
			fromChars.push_back(mangF);
			toChars.push_back(mangT);
			if(seenMMs.count( std::pair<char,char>(mangF,mangT) ) ){
				throw new std::runtime_error("Duplicate mismatch entry in affine mangle.");
			}
			seenMMs.insert(std::pair<char,char>(mangF,mangT));
			tmpParse.clear(); tmpParse.insert(tmpParse.end(), curFoc, nextFoc);
			mmActions.push_back(new ScriptAlignCostMangleAction(&tmpParse));
		}
		else if(*curFoc == 'G'){
			curFoc++; CHECK_OUT_OF_ROAD
			char specG = *curFoc;
			curFoc++;
			tmpParse.clear(); tmpParse.insert(tmpParse.end(), curFoc, nextFoc);
			if(specG == 'o'){
				if(openAction){ delete(openAction); openAction = 0; }
				openAction = new ScriptAlignCostMangleAction(&tmpParse);
			}
			else if(specG == 'x'){
				if(extAction){ delete(extAction); extAction = 0; }
				extAction = new ScriptAlignCostMangleAction(&tmpParse);
			}
			else if(specG == 'c'){
				if(closeAction){ delete(closeAction); closeAction = 0; }
				closeAction = new ScriptAlignCostMangleAction(&tmpParse);
			}
			else{
				throw new std::runtime_error("Malformed specification for affine mangle.");
			}
		}
		curFoc = nextFoc;
	}
}

double linearReferenceAlignProbabilityAffine(LinearPairwiseSequenceAlignment* alnPro, double* readQuals, LinearPairwiseAlignmentIteration* forAln, double lproGapOpen, double lproGapExtend){
	double lten3 = log10(1.0/3.0);
	uintptr_t alnLen = forAln->aInds.size();
	//uintptr_t lenA = alnPro->seqAs->size();
	uintptr_t lenB = alnPro->seqBs->size();
	std::string* seqAs = alnPro->seqAs;
	std::string* seqBs = alnPro->seqBs;
	std::vector<intptr_t>* aInds = &(forAln->aInds);
	std::vector<intptr_t>* bInds = &(forAln->bInds);
	//get the span and probability
	double curLPro = 0.0;
	if(alnLen <= 1){
		curLPro = lproGapOpen + (lenB*lproGapExtend);
	}
	else{
		if(forAln->bInds[0]){
			curLPro += (lproGapOpen + ((*bInds)[0]*lproGapExtend));
		}
		if((uintptr_t)((*bInds)[alnLen-1]) != lenB){
			curLPro += (lproGapOpen + ((lenB - (*bInds)[alnLen-1])*lproGapExtend));
		}
		double curOpenA = lproGapOpen;
		double curOpenB = lproGapOpen;
		for(uintptr_t i = 1; i<alnLen; i++){
			if((*aInds)[i-1] != (*aInds)[i]){
				if((*bInds)[i-1] != (*bInds)[i]){
					double curLQP = readQuals[(*bInds)[i-1]];
					if((*seqBs)[(*bInds)[i-1]] != (*seqAs)[(*aInds)[i-1]]){
						curLPro += (curLQP + lten3);
					}
					else{
						curLPro += log10(1.0 - pow(10.0, curLQP));
					}
					curOpenA = lproGapOpen;
					curOpenB = lproGapOpen;
				}
				else{
					curLPro += (curOpenB + lproGapExtend);
					curOpenA = lproGapOpen;
					curOpenB = 0;
				}
			}
			else{
				curLPro += (curOpenA + lproGapExtend);
				curOpenA = 0;
				curOpenB = lproGapOpen;
			}
		}
	}
	return curLPro;
}

