#include "whodun_align_affinepd.h"

#include <set>
#include <iostream>
#include <string.h>
#include <algorithm>
#include <stdexcept>

#include "whodun_parse.h"
#include "whodun_stringext.h"

#define WHITESPACE " \t\r\n"

AlignCostAffineMangleSet* PositionalQualityMangleSet::getMangleForQuality(unsigned char relMang){
	std::vector<unsigned char>::iterator mangIt = std::upper_bound(allQualHigh.begin(), allQualHigh.end(), relMang);
	if(mangIt == allQualHigh.begin()){ return 0; }
	mangIt--;
	uintptr_t mangInd = mangIt - allQualHigh.begin();
	if((relMang >= allQualLow[mangInd]) && (relMang <= allQualHigh[mangInd])){
		return &(relMangs[mangInd]);
	}
	return 0;
}

void PositionalQualityMangleSet::parseQualityMangleSet(const char* parseS, const char* parseE){
	allQualLow.clear();
	allQualHigh.clear();
	relMangs.clear();
	std::vector< std::pair<unsigned char,unsigned char> > qualRangs;
	std::vector< std::pair<const char*,const char*> > mangRangs;
	const char* curFoc = parseS;
	while(curFoc < parseE){
		if(strchr(" \r\t\n", *curFoc)){ curFoc++; continue; }
		if(*curFoc == 'q'){
			const char* qspecEnd = (const char*)memchr(curFoc, '\n', parseE - curFoc);
			if(qspecEnd == 0){ throw std::runtime_error("Malformed quality mangle specification."); }
			curFoc++;
			curFoc += memspn(curFoc, qspecEnd - curFoc, " \r\t\n", 4); if(curFoc == qspecEnd){ throw std::runtime_error("Malformed quality mangle specification."); }
			unsigned char lowQ = atol(curFoc);
			if((lowQ < 0) || (lowQ > 255)){ throw std::runtime_error("Quality out of range."); }
			curFoc += memcspn(curFoc, qspecEnd - curFoc, " \r\t\n", 4);
			curFoc += memspn(curFoc, qspecEnd - curFoc, " \r\t\n", 4);
			unsigned char higQ = lowQ;
			if(curFoc != qspecEnd){ higQ = atol(curFoc); }
			if((higQ < 0) || (higQ > 255)){ throw std::runtime_error("Quality out of range."); }
			const char* mangEnd = (const char*)memchr(qspecEnd, 'Q', parseE - qspecEnd);
			if(mangEnd == 0){ throw std::runtime_error("Malformed quality mangle specification."); }
			if(higQ < lowQ){ throw std::runtime_error("High quality below low quality."); }
			qualRangs.push_back( std::pair<unsigned char,unsigned char>(lowQ, higQ) );
			mangRangs.push_back(std::pair<const char*,const char*>(qspecEnd, mangEnd));
			curFoc = mangEnd + 1;
			continue;
		}
		throw std::runtime_error("Malformed quality mangle specification.");
	}
	//sort ranges, parse pieces
	std::vector< std::pair<unsigned char,uintptr_t> > lowSortV;
	for(uintptr_t i = 0; i<qualRangs.size(); i++){
		lowSortV.push_back( std::pair<unsigned char, uintptr_t>(qualRangs[i].first, i) );
	}
	std::sort(lowSortV.begin(), lowSortV.end());
	relMangs.resize(qualRangs.size());
	for(uintptr_t i = 0; i<qualRangs.size(); i++){
		uintptr_t ri = lowSortV[i].second;
		std::pair<unsigned char,unsigned char> qrangP = qualRangs[ri];
		allQualLow.push_back(qrangP.first);
		allQualHigh.push_back(qrangP.second);
		std::pair<const char*,const char*> mrangP = mangRangs[ri];
		relMangs[i].parseMangleSet(mrangP.first, mrangP.second);
	}
	//make sure there is no overlap
	for(uintptr_t i = 1; i<qualRangs.size(); i++){
		if(allQualLow[i] <= allQualHigh[i-1]){
			throw std::runtime_error("Mangle quality ranges overlap.");
		}
	}
}

void PositionDependentQualityMangleSet::parsePositionMangleSet(const char* parseS, const char* parseE){
	rangLow.clear();
	rangHig.clear();
	rangMang.clear();
	//find all the Ps
	std::vector<const char*> allPs;
	const char* curFoc = (const char*)memchr(parseS, 'P', parseE-parseS);
	while(curFoc){
		allPs.push_back(curFoc);
		curFoc = (const char*)memchr(curFoc+1, 'P', parseE - (curFoc+1));
	}
	//parse the default
	if(allPs.size()){
		defMangle.parseQualityMangleSet(parseS, allPs[0]);
	}
	else{
		defMangle.parseQualityMangleSet(parseS, parseE);
		return;
	}
	//get the ranges for the positions
	std::vector< std::pair<uintptr_t,uintptr_t> > allRans;
	std::vector<const char*> allSubStarts;
	std::vector<const char*> allSubEnds;
	for(uintptr_t i = 0; i<allPs.size(); i++){
		const char* curConS = allPs[i] + 1;
		const char* fullConE = (i==(allPs.size()-1)) ? parseE : allPs[i+1];
		const char* curConE = (const char*)memchr(curConS, '\n', fullConE - curConS);
		if(curConE == 0){ throw std::runtime_error("Malformed position flag in quality mangle."); }
		allSubStarts.push_back(curConE);
		allSubEnds.push_back(fullConE);
		curConS += memspn(curConS, curConE-curConS, " \r\t\n", 4);
			if(curConS == curConE){ throw std::runtime_error("Malformed position flag in quality mangle."); }
		intptr_t refLow = atol(curConS);
		curConS += memcspn(curConS, curConE-curConS, " \r\t\n", 4);
		curConS += memspn(curConS, curConE-curConS, " \r\t\n", 4);
			if(curConS == curConE){ throw std::runtime_error("Malformed position flag in quality mangle."); }
		intptr_t refHig = atol(curConS);
		if(refLow < 0){
			throw std::runtime_error("There are no negative reference indices.");
		}
		if(refHig < 0){
			throw std::runtime_error("There are no negative reference indices.");
		}
		if(refHig <= refLow){
			throw std::runtime_error("High index below low index.");
		}
		allRans.push_back( std::pair<uintptr_t,uintptr_t>(refLow, refHig) );
	}
	//sort ranges, parse pieces
	std::vector< std::pair<uintptr_t,uintptr_t> > lowSortV;
	for(uintptr_t i = 0; i<allRans.size(); i++){
		lowSortV.push_back( std::pair<unsigned char, uintptr_t>(allRans[i].first, i) );
	}
	std::sort(lowSortV.begin(), lowSortV.end());
	rangMang.resize(allRans.size());
	for(uintptr_t i = 0; i<allRans.size(); i++){
		uintptr_t ri = lowSortV[i].second;
		std::pair<uintptr_t,uintptr_t> qrangP = allRans[ri];
		rangLow.push_back(qrangP.first);
		rangHig.push_back(qrangP.second);
		rangMang[i].parseQualityMangleSet(allSubStarts[ri], allSubEnds[ri]);
	}
	//make sure there is no overlap
	for(uintptr_t i = 1; i<allRans.size(); i++){
		if(rangLow[i] < rangHig[i-1]){
			throw std::runtime_error("Mangle quality reference position ranges overlap.");
		}
	}
}

void PositionDependentQualityMangleSet::rebase(PositionDependentQualityMangleSet* toRebase, uintptr_t newLow, uintptr_t newHig){
	rangLow.clear();
	rangHig.clear();
	rangMang.clear();
	defMangle = toRebase->defMangle;
	std::vector<uintptr_t> survRs;
	for(uintptr_t i = 0; i<toRebase->rangLow.size(); i++){
		if(toRebase->rangLow[i] >= newHig){ continue; }
		if(toRebase->rangHig[i] <= newLow){ continue; }
		survRs.push_back(i);
	}
	rangMang.resize(survRs.size());
	for(uintptr_t si = 0; si<survRs.size(); si++){
		uintptr_t i = survRs[si];
		intptr_t subLow = toRebase->rangLow[i] - newLow;
		subLow = std::max(subLow, (intptr_t)0);
		uintptr_t subHig = toRebase->rangHig[i] - newLow;
		subHig = std::min(subHig, newHig - newLow);
		rangLow.push_back(subLow);
		rangHig.push_back(subHig);
		rangMang[si] = toRebase->rangMang[i];
	}
}

void parseMultiregionPositionQualityMangle(const char* parseS, const char* parseE, std::map<std::string, PositionDependentQualityMangleSet>* toFill){
	std::vector<std::string> allRefNames;
	std::vector<const char*> refSubS;
	std::vector<const char*> refSubE;
	const char* curFoc = parseS;
	while(curFoc < parseE){
		if(strchr(" \r\t\n", *curFoc)){ curFoc++; continue; }
		if(*curFoc == 'r'){
			curFoc++;
			const char* focNL = (const char*)memchr(curFoc, '\n', parseE-curFoc);
			if(focNL == 0){ std::runtime_error("Malformed multiregion quality mangle."); }
			curFoc += strspn(curFoc, " \r\t");
			const char* rnamEnd = curFoc + strcspn(curFoc, " \r\t\n");
			allRefNames.push_back(std::string(curFoc, rnamEnd));
			refSubS.push_back(focNL);
			curFoc = (const char*)memchr(focNL, 'R', parseE - focNL);
			if(curFoc == 0){ std::runtime_error("Malformed multiregion quality mangle."); }
			refSubE.push_back(curFoc);
			curFoc++;
			continue;
		}
		throw std::runtime_error("Malformed multiregion quality mangle.");
	}
	//make all the references
	PositionDependentQualityMangleSet tmpSet;
	for(uintptr_t i = 0; i<allRefNames.size(); i++){
		(*toFill)[allRefNames[i]] = tmpSet;
	}
	//parse
	for(uintptr_t i = 0; i<allRefNames.size(); i++){
		(*toFill)[allRefNames[i]].parsePositionMangleSet(refSubS[i], refSubE[i]);
	}
}

AlignCostAffineMangleSet* PositionalBiQualityMangleSet::getMangleForQualities(unsigned char relMangA, unsigned char relMangB){
	int winPri = -1;
	AlignCostAffineMangleSet* winMang = 0;
	for(uintptr_t i = 0; i<allQualLow.size(); i++){
		int lowA = allQualLow[i].first;
		int lowB = allQualLow[i].second;
		int higA = allQualHigh[i].first;
		int higB = allQualHigh[i].second;
		if((lowA >= 0) && ((unsigned)lowA > relMangA)){ continue; }
		if((lowB >= 0) && ((unsigned)lowB > relMangB)){ continue; }
		if((higA >= 0) && ((unsigned)higA < relMangA)){ continue; }
		if((higB >= 0) && ((unsigned)higB < relMangB)){ continue; }
		int curPri = (lowA >= 0) + (lowB >= 0) + (higA >= 0) + (higB >= 0);
		if(curPri <= winPri){ continue; }
		winPri = curPri;
		winMang = &(relMangs[i]);
	}
	return winMang;
}

void PositionalBiQualityMangleSet::parseQualityMangleSet(const char* parseS, const char* parseE){
	allQualLow.clear();
	allQualHigh.clear();
	relMangs.clear();
	std::vector< std::pair<const char*,const char*> > mangRangs;
	const char* curFoc = parseS;
	while(curFoc < parseE){
		if(strchr(" \r\t\n", *curFoc)){ curFoc++; continue; }
		if(*curFoc == 'q'){
			const char* qspecEnd = (const char*)memchr(curFoc, '\n', parseE - curFoc);
			if(qspecEnd == 0){ throw std::runtime_error("Malformed quality mangle specification."); }
			curFoc++;
			curFoc += memspn(curFoc, qspecEnd - curFoc, " \r\t\n", 4); if(curFoc == qspecEnd){ throw std::runtime_error("Malformed quality mangle specification."); }
				int lowQA = atol(curFoc);
				if(lowQA > 255){ throw std::runtime_error("Quality out of range."); }
				curFoc += memcspn(curFoc, qspecEnd - curFoc, " \r\t\n", 4);
			curFoc += memspn(curFoc, qspecEnd - curFoc, " \r\t\n", 4); if(curFoc == qspecEnd){ throw std::runtime_error("Malformed quality mangle specification."); }
				int higQA = atol(curFoc);
				if(higQA > 255){ throw std::runtime_error("Quality out of range."); }
				curFoc += memcspn(curFoc, qspecEnd - curFoc, " \r\t\n", 4);
			curFoc += memspn(curFoc, qspecEnd - curFoc, " \r\t\n", 4); if(curFoc == qspecEnd){ throw std::runtime_error("Malformed quality mangle specification."); }
				int lowQB = atol(curFoc);
				if(lowQB > 255){ throw std::runtime_error("Quality out of range."); }
				curFoc += memcspn(curFoc, qspecEnd - curFoc, " \r\t\n", 4);
			curFoc += memspn(curFoc, qspecEnd - curFoc, " \r\t\n", 4); if(curFoc == qspecEnd){ throw std::runtime_error("Malformed quality mangle specification."); }
				int higQB = atol(curFoc);
				if(higQB > 255){ throw std::runtime_error("Quality out of range."); }
				curFoc += memcspn(curFoc, qspecEnd - curFoc, " \r\t\n", 4);
			const char* mangEnd = (const char*)memchr(qspecEnd, 'Q', parseE - qspecEnd);
			if(mangEnd == 0){ throw std::runtime_error("Malformed quality mangle specification."); }
			if((higQA >= 0) && (lowQA >= 0) && (higQA < lowQA)){ throw std::runtime_error("High quality below low quality."); }
			if((higQB >= 0) && (lowQB >= 0) && (higQB < lowQB)){ throw std::runtime_error("High quality below low quality."); }
			allQualLow.push_back( std::pair<int,int>(lowQA,lowQB) );
			allQualHigh.push_back( std::pair<int,int>(higQA,higQB) );
			mangRangs.push_back(std::pair<const char*,const char*>(qspecEnd, mangEnd));
			curFoc = mangEnd + 1;
			continue;
		}
		throw std::runtime_error("Malformed quality mangle specification.");
	}
	relMangs.resize(mangRangs.size());
	for(uintptr_t i = 0; i<mangRangs.size(); i++){
		std::pair<const char*,const char*> mrangP = mangRangs[i];
		relMangs[i].parseMangleSet(mrangP.first, mrangP.second);
	}
}

PositionDependentCostKDNode::PositionDependentCostKDNode(){
	subNodeLesser = 0;
	subNodeGreatE = 0;
}

PositionDependentCostKDNode::~PositionDependentCostKDNode(){
}

PositionDependentCostKDTree::PositionDependentCostKDTree(){
	allNodes = 0;
}

PositionDependentCostKDTree::~PositionDependentCostKDTree(){
}

/**
 * Clone a kd node (faster than rebuilding).
 * @param toCopy The node to copy.
 * @param origBase The original base address for the bounds array.
 * @param newBase The new base address for the bounds array.
 * @return The cloned node.
 */
PositionDependentCostKDNode* clonePDCostKDNode(PositionDependentCostKDNode* toCopy, const PositionDependentCostRegion* origBase, PositionDependentCostRegion* newBase){
	if(toCopy == 0){ return 0; }
	PositionDependentCostKDNode* toRet = new PositionDependentCostKDNode();
	toRet->splitOn = toCopy->splitOn;
	toRet->splitAt = toCopy->splitAt;
	toRet->allSpans.resize(toCopy->allSpans.size());
	for(uintptr_t i = 0; i<toCopy->allSpans.size(); i++){
		toRet->allSpans[i] = newBase + (toCopy->allSpans[i] - origBase);
	}
	toRet->subNodeLesser = clonePDCostKDNode(toCopy->subNodeLesser, origBase, newBase);
	toRet->subNodeGreatE = clonePDCostKDNode(toCopy->subNodeGreatE, origBase, newBase);
	return toRet;
}

PositionDependentCostKDTree::PositionDependentCostKDTree(const PositionDependentCostKDTree& toCopy){
	allRegions.resize(toCopy.allRegions.size());
	for(uintptr_t i = 0; i<allRegions.size(); i++){
		allRegions[i] = toCopy.allRegions[i];
	}
	allNodes = 0;
	if(toCopy.allNodes){
		if(toCopy.builtInUse > builtTree.size()){
			builtTree.resize(toCopy.builtInUse);
		}
		builtInUse = toCopy.builtInUse;
		PositionDependentCostKDNode* myBaseTNod = &(builtTree[0]);
		const PositionDependentCostKDNode* othBaseTNod = &(toCopy.builtTree[0]);
		PositionDependentCostRegion* myBaseReg = &(allRegions[0]);
		const PositionDependentCostRegion* othBaseReg = &(toCopy.allRegions[0]);
		for(uintptr_t i = 0; i<toCopy.builtInUse; i++){
			PositionDependentCostKDNode* curFill = &(builtTree[i]);
			const PositionDependentCostKDNode* curCopy = &(toCopy.builtTree[i]);
			curFill->splitOn = curCopy->splitOn;
			curFill->splitAt = curCopy->splitAt;
			curFill->subNodeLesser = curCopy->subNodeLesser ? (myBaseTNod + (curCopy->subNodeLesser - othBaseTNod)) : 0;
			curFill->subNodeGreatE = curCopy->subNodeGreatE ? (myBaseTNod + (curCopy->subNodeGreatE - othBaseTNod)) : 0;
			curFill->allSpans.resize(curCopy->allSpans.size());
			for(uintptr_t j = 0; j<curCopy->allSpans.size(); j++){
				curFill->allSpans[j] = myBaseReg + (curCopy->allSpans[j] - othBaseReg);
			}
		}
		allNodes = &(builtTree[0]);
	}
}
PositionDependentCostKDTree& PositionDependentCostKDTree::operator=(const PositionDependentCostKDTree& toClone){
	if(this == &toClone){return *this;}
	allRegions.resize(toClone.allRegions.size());
	for(uintptr_t i = 0; i<allRegions.size(); i++){
		allRegions[i] = toClone.allRegions[i];
	}
	allNodes = 0;
	if(toClone.allNodes){
		if(toClone.builtInUse > builtTree.size()){
			builtTree.resize(toClone.builtInUse);
		}
		builtInUse = toClone.builtInUse;
		PositionDependentCostKDNode* myBaseTNod = &(builtTree[0]);
		const PositionDependentCostKDNode* othBaseTNod = &(toClone.builtTree[0]);
		PositionDependentCostRegion* myBaseReg = &(allRegions[0]);
		const PositionDependentCostRegion* othBaseReg = &(toClone.allRegions[0]);
		for(uintptr_t i = 0; i<toClone.builtInUse; i++){
			PositionDependentCostKDNode* curFill = &(builtTree[i]);
			const PositionDependentCostKDNode* curCopy = &(toClone.builtTree[i]);
			curFill->splitOn = curCopy->splitOn;
			curFill->splitAt = curCopy->splitAt;
			curFill->subNodeLesser = curCopy->subNodeLesser ? (myBaseTNod + (curCopy->subNodeLesser - othBaseTNod)) : 0;
			curFill->subNodeGreatE = curCopy->subNodeGreatE ? (myBaseTNod + (curCopy->subNodeGreatE - othBaseTNod)) : 0;
			curFill->allSpans.resize(curCopy->allSpans.size());
			for(uintptr_t j = 0; j<curCopy->allSpans.size(); j++){
				curFill->allSpans[j] = myBaseReg + (curCopy->allSpans[j] - othBaseReg);
			}
		}
		allNodes = &(builtTree[0]);
	}
	return *this;
}

void PositionDependentCostKDTree::produceFromRegions(){
	if(allNodes){
		return;
	}
	builtInUse = 0;
	saveRegPtrs.resize(allRegions.size());
	for(uintptr_t i = 0; i<allRegions.size(); i++){saveRegPtrs[i] = &(allRegions[i]);}
	buildPDTreeSplits(allRegions.size(), &(saveRegPtrs[0]));
	allNodes = &(builtTree[0]);
	//fix up the pointers
	for(uintptr_t i = 0; i<builtInUse; i++){
		PositionDependentCostKDNode* toRet = &(builtTree[i]);
		if(toRet->subNodeLesser){
			toRet->subNodeLesser = &(builtTree[(uintptr_t)(toRet->subNodeLesser)]);
		}
		if(toRet->subNodeGreatE){
			toRet->subNodeGreatE = &(builtTree[(uintptr_t)(toRet->subNodeGreatE)]);
		}
	}
}

void PositionDependentCostKDTree::buildPDTreeSplits(uintptr_t numRegs, PositionDependentCostRegion** theRegs){
	std::pair<intptr_t,intptr_t> rangeInA(-1,-1);
	std::pair<intptr_t,intptr_t> rangeInB(-1,-1);
	//get the max and min
	for(uintptr_t i = 0; i<numRegs; i++){
		PositionDependentCostRegion* curReg = theRegs[i];
		#define MINMAX_EXAMINE_VALUE(posRep, forVal, compMethod) forVal = (forVal >= 0) ? ((posRep >= 0) ? compMethod(posRep, forVal) : forVal) : posRep;
		MINMAX_EXAMINE_VALUE(rangeInA.first, curReg->startA, std::min)
		MINMAX_EXAMINE_VALUE(rangeInA.first, curReg->endA, std::min)
		MINMAX_EXAMINE_VALUE(rangeInA.second, curReg->startA, std::max)
		MINMAX_EXAMINE_VALUE(rangeInA.second, curReg->endA, std::max)
		MINMAX_EXAMINE_VALUE(rangeInB.first, curReg->startB, std::min)
		MINMAX_EXAMINE_VALUE(rangeInB.first, curReg->endB, std::min)
		MINMAX_EXAMINE_VALUE(rangeInB.second, curReg->startB, std::max)
		MINMAX_EXAMINE_VALUE(rangeInB.second, curReg->endB, std::max)
	}
	//get the split counts for splitting on A
	intptr_t numSpanA = numRegs;
	intptr_t numLowA = 0;
	intptr_t numHighA = 0;
	if(rangeInA.first >= 0){
		intptr_t splVA = (rangeInA.first + rangeInA.second) / 2;
		numSpanA = 0;
		for(uintptr_t i = 0; i<numRegs; i++){
			PositionDependentCostRegion* curReg = theRegs[i];
			if((curReg->endA >= 0) && (curReg->endA <= splVA)){ numLowA++; }
			else if((curReg->startA >= 0) && (curReg->startA >= splVA)){ numHighA++; }
			else{numSpanA++;}
		}
	}
	//and for B
	intptr_t numSpanB = numRegs;
	intptr_t numLowB = 0;
	intptr_t numHighB = 0;
	if(rangeInB.first >= 0){
		intptr_t splVB = (rangeInB.first + rangeInB.second) / 2;
		numSpanB = 0;
		for(uintptr_t i = 0; i<numRegs; i++){
			PositionDependentCostRegion* curReg = theRegs[i];
			if((curReg->endB >= 0) && (curReg->endB <= splVB)){ numLowB++; }
			else if((curReg->startB >= 0) && (curReg->startB >= splVB)){ numHighB++; }
			else{numSpanB++;}
		}
	}
	//get the bin scores and decide how to split
	int binCntA = (numSpanA ? 1 : 0) + (numLowA ? 1 : 0) + (numHighA ? 1 : 0);
	int binCntB = (numSpanB ? 1 : 0) + (numLowB ? 1 : 0) + (numHighB ? 1 : 0);
	intptr_t scoreA = numLowA + numHighA + 2*numSpanA;
	intptr_t scoreB = numLowB + numHighB + 2*numSpanB;
	int goWithA = (binCntA > binCntB) || ((binCntA == binCntB) && (scoreA < scoreB));
	//get the node to build
		if(builtInUse >= builtTree.size()){ builtTree.resize(2*builtTree.size()+1); }
		PositionDependentCostKDNode* toRet = &(builtTree[builtInUse]);
		toRet->subNodeLesser = 0;
		toRet->subNodeGreatE = 0;
		toRet->allSpans.clear();
		builtInUse++;
	//short circuit if everything goes to the same place
	if((goWithA && (binCntA <= 1)) || (!goWithA && (binCntB <= 1))){
		toRet->splitOn = POSITIONDEPENDENT_SPLITAXIS_A;
		toRet->splitAt = -1;
		toRet->allSpans.insert(toRet->allSpans.end(), theRegs, theRegs + numRegs);
		return;
	}
	//rearrange the pointers, filling in spans and removing them
	PositionDependentCostRegion** nextLReg = theRegs;
	PositionDependentCostRegion** nextRReg = theRegs + numRegs;
	PositionDependentCostRegion** curRegP = theRegs;
	if(goWithA){
		toRet->splitOn = POSITIONDEPENDENT_SPLITAXIS_A;
		toRet->splitAt = (rangeInA.first + rangeInA.second) / 2;
		while(curRegP < nextRReg){
			PositionDependentCostRegion* curReg = *curRegP;
			if((curReg->endA >= 0) && (curReg->endA <= toRet->splitAt)){
				*nextLReg = curReg;
				nextLReg++;
				curReg++;
			}
			else if((curReg->startA >= 0) && (curReg->startA >= toRet->splitAt)){
				nextRReg--;
				PositionDependentCostRegion* tmpReg = *nextRReg;
				*nextRReg = curReg;
				*curRegP = tmpReg;
			}
			else{
				toRet->allSpans.push_back(curReg);
				curRegP++;
			}
		}
	}
	else{
		toRet->splitOn = POSITIONDEPENDENT_SPLITAXIS_B;
		toRet->splitAt = (rangeInB.first + rangeInB.second) / 2;
		while(curRegP < nextRReg){
			PositionDependentCostRegion* curReg = *curRegP;
			if((curReg->endB >= 0) && (curReg->endB <= toRet->splitAt)){
				*nextLReg = curReg;
				nextLReg++;
				curReg++;
			}
			else if((curReg->startB >= 0) && (curReg->startB >= toRet->splitAt)){
				nextRReg--;
				PositionDependentCostRegion* tmpReg = *nextRReg;
				*nextRReg = curReg;
				*curRegP = tmpReg;
			}
			else{
				toRet->allSpans.push_back(curReg);
				curRegP++;
			}
		}
	}
	//make the sub nodes
	uintptr_t numSubL = nextLReg - theRegs;
	if(numSubL){
		toRet->subNodeLesser = (PositionDependentCostKDNode*)(builtInUse); //set to index for now, fix after realloc done
		buildPDTreeSplits(numSubL, theRegs);
	}
	uintptr_t numSubR = (theRegs + numRegs) - nextRReg;
	if(numSubR){
		toRet->subNodeGreatE = (PositionDependentCostKDNode*)(builtInUse); //set to index for now, fix after realloc done
		buildPDTreeSplits(numSubR, nextRReg);
	}
}

void PositionDependentCostKDTree::regionsUpdated(){
	allNodes = 0;
}

void PositionDependentCostKDTree::regionsUniform(AlignCostAffine* baseCost){
	allNodes = 0;
	allRegions.clear();
	PositionDependentCostRegion addReg;
		addReg.startA = -1;
		addReg.endA = -1;
		addReg.startB = -1;
		addReg.endB = -1;
		addReg.priority = 0;
		addReg.regCosts = *baseCost;
	allRegions.push_back(addReg);
}

void PositionDependentCostKDTree::regionsRebased(PositionDependentCostKDTree* toRebase, intptr_t newLowA, intptr_t newHighA, intptr_t newLowB, intptr_t newHighB){
	allNodes = 0;
	allRegions.clear();
	//get the regions that overlap that region
	std::vector<PositionDependentCostKDNode*> nodeStack;
	std::vector<PositionDependentCostRegion*> winRegs;
	nodeStack.push_back(toRebase->allNodes);
	while(nodeStack.size()){
		PositionDependentCostKDNode* curNode = nodeStack[nodeStack.size()-1];
		nodeStack.pop_back();
		while(curNode){
			//look through the spanners
			for(uintptr_t i = 0; i<curNode->allSpans.size(); i++){
				PositionDependentCostRegion* curReg = curNode->allSpans[i];
				if((curReg->startA >= 0) && (newHighA >= 0) && (curReg->startA >= newHighA)){ continue; }
				if((curReg->endA >= 0) && (newLowA >= 0) && (curReg->endA <= newLowA)){ continue; }
				if((curReg->startB >= 0) && (newHighB >= 0) && (curReg->startB >= newHighB)){ continue; }
				if((curReg->endB >= 0) && (newLowB >= 0) && (curReg->endB <= newLowB)){ continue; }
				winRegs.push_back(curReg);
			}
			//figure out which sub node to look at
			if(curNode->splitOn == POSITIONDEPENDENT_SPLITAXIS_A){
				if((newHighA >= 0) && (newHighA <= curNode->splitAt)){
					curNode = curNode->subNodeLesser;
				}
				else if((newLowA >= 0) && (newLowA >= curNode->splitAt)){
					curNode = curNode->subNodeGreatE;
				}
				else{
					curNode = curNode->subNodeLesser;
					nodeStack.push_back(curNode->subNodeGreatE);
				}
			}
			else{
				if((newHighB >= 0) && (newHighB <= curNode->splitAt)){
					curNode = curNode->subNodeLesser;
				}
				else if((newLowB >= 0) && (newLowB >= curNode->splitAt)){
					curNode = curNode->subNodeGreatE;
				}
				else{
					curNode = curNode->subNodeLesser;
					nodeStack.push_back(curNode->subNodeGreatE);
				}
			}
		}
	}
	//rebase everything
	allRegions.resize(winRegs.size());
	for(uintptr_t i = 0; i<winRegs.size(); i++){
		PositionDependentCostRegion* curReg = winRegs[i];
		PositionDependentCostRegion* setReg = &(allRegions[i]);
		//change the coordinates
		if((curReg->startA < 0) || (newLowA < 0)){ setReg->startA = curReg->startA; }
			else{ setReg->startA = std::max(curReg->startA - newLowA, (intptr_t)0); }
		if((curReg->endA < 0) || ((newLowA < 0) && (newHighA < 0))){ setReg->endA = curReg->startA; }
			else if(newLowA < 0){ setReg->endA = std::min(curReg->endA, newHighA); }
			else if(newHighA < 0){ setReg->endA = std::max(curReg->endA - newLowA, (intptr_t)0); }
			else{ setReg->endA = std::max(std::min(curReg->endA, newHighA) - newLowA, (intptr_t)0);}
		if((curReg->startB < 0) || (newLowB < 0)){ setReg->startB = curReg->startB; }
			else{ setReg->startB = std::max(curReg->startB - newLowB, (intptr_t)0); }
		if((curReg->endB < 0) || ((newLowB < 0) && (newHighB < 0))){ setReg->endB = curReg->startB; }
			else if(newLowB < 0){ setReg->endB = std::min(curReg->endB, newHighB); }
			else if(newHighB < 0){ setReg->endB = std::max(curReg->endB - newLowB, (intptr_t)0); }
			else{ setReg->endB = std::max(std::min(curReg->endB, newHighB) - newLowB, (intptr_t)0);}
		//extra stuff
		setReg->priority = curReg->priority;
		setReg->regCosts = curReg->regCosts;
	}
}

void PositionDependentCostKDTree::regionsQualityMangled(PositionDependentCostKDTree* toMangle, PositionDependentQualityMangleSet* useMangle, std::vector<char>* readQuals){
	allNodes = 0;
	allRegions.clear();
	PositionDependentCostRegion newReg;
	//figure out which mangle sets to use for each part of the reference.
		std::vector<PositionalQualityMangleSet*> allMangs;
		std::vector<intptr_t> allLows;
		std::vector<intptr_t> allHigs;
		if(useMangle->rangLow.size()){
			uintptr_t numRang = useMangle->rangLow.size();
			allMangs.push_back(&(useMangle->defMangle));
			allLows.push_back(-1);
			allHigs.push_back(useMangle->rangLow[0]);
			allMangs.push_back(&(useMangle->defMangle));
			allLows.push_back(useMangle->rangHig[numRang-1]);
			allHigs.push_back(-1);
			for(uintptr_t i = 1; i<numRang; i++){
				uintptr_t prevHig = useMangle->rangHig[i-1];
				uintptr_t curLow = useMangle->rangLow[i];
				if(prevHig != curLow){
					allMangs.push_back(&(useMangle->defMangle));
					allLows.push_back(prevHig);
					allHigs.push_back(curLow);
				}
			}
			for(uintptr_t i = 0; i<numRang; i++){
				allMangs.push_back(&(useMangle->rangMang[i]));
				allLows.push_back(useMangle->rangLow[i]);
				allHigs.push_back(useMangle->rangHig[i]);
			}
		}
		else{
			allMangs.push_back(&(useMangle->defMangle));
			allLows.push_back(-1);
			allHigs.push_back(-1);
		}
	//look for runs of quality
		uintptr_t readLen = readQuals->size();
		std::vector<unsigned char> qualRuns;
		std::vector<uintptr_t> readLows;
		std::vector<uintptr_t> readHigs;
		uintptr_t curReadL = 0;
		while(curReadL < readLen){
			unsigned char curQual = (*readQuals)[curReadL];
			uintptr_t curReadH = curReadL + 1;
			while(curReadH < readLen){
				if((*readQuals)[curReadH] != curQual){
					break;
				}
				curReadH++;
			}
			qualRuns.push_back(curQual);
			readLows.push_back(curReadL);
			readHigs.push_back(curReadH);
			curReadL = curReadH;
		}
	//handle stuff before and after the read
		for(uintptr_t i = 0; i<toMangle->allRegions.size(); i++){
			PositionDependentCostRegion* cpyReg = &(toMangle->allRegions[i]);
			if(cpyReg->startB >= 0){ continue; }
			newReg = *cpyReg;
			newReg.endB = 0;
			allRegions.push_back(newReg);
		}
		for(uintptr_t i = 0; i<toMangle->allRegions.size(); i++){
			PositionDependentCostRegion* cpyReg = &(toMangle->allRegions[i]);
			if((cpyReg->endB >= 0) && ((uintptr_t)(cpyReg->endB) <= readLen)){ continue; }
			newReg = *cpyReg;
			newReg.startB = readLen;
			allRegions.push_back(newReg);
		}
	//mangle along the read
		for(uintptr_t readI = 0; readI < qualRuns.size(); readI++){
			for(uintptr_t refI = 0; refI < allMangs.size(); refI++){
				PositionalQualityMangleSet* curMang = allMangs[refI];
				std::vector<unsigned char>::iterator qualIt = std::lower_bound(curMang->allQualHigh.begin(), curMang->allQualHigh.end(), qualRuns[readI]);
				AlignCostAffineMangleSet* selMang = 0;
				if(qualIt != curMang->allQualHigh.end()){
					selMang = &(curMang->relMangs[qualIt - curMang->allQualHigh.begin()]);
				}
				for(uintptr_t i = 0; i<toMangle->allRegions.size(); i++){
					PositionDependentCostRegion* cpyReg = &(toMangle->allRegions[i]);
					newReg.startA = cpyReg->startA;
					if(newReg.startA >= 0){
						if((allHigs[refI] >= 0) && (newReg.startA >= allHigs[refI])){continue;}
						if((allLows[refI] >= 0) && (newReg.startA < allLows[refI])){ newReg.startA = allLows[refI]; }
					}
					else if(allLows[refI] >= 0){ newReg.startA = allLows[refI]; }
					newReg.endA = cpyReg->endA;
					if(newReg.endA >= 0){
						if((allLows[refI] >= 0) && (newReg.endA <= allLows[refI])){ continue; }
						if((allHigs[refI] >= 0) && (newReg.endA > allHigs[refI])){ newReg.endA = allHigs[refI]; }
					}
					else if(allHigs[refI] >= 0){ newReg.endA = allHigs[refI]; }
					newReg.startB = cpyReg->startB;
					if(newReg.startB >= 0){
						if((uintptr_t)(newReg.startB) >= readHigs[readI]){ continue; }
						if((uintptr_t)(newReg.startB) < readLows[readI]){ newReg.startB = readLows[readI]; }
					}
					else{ newReg.startB = readLows[readI]; }
					newReg.endB = cpyReg->endB;
					if(newReg.endB >= 0){
						if((uintptr_t)(newReg.endB) <= readLows[readI]){ continue; }
						if((uintptr_t)(newReg.endB) > readHigs[readI]){ newReg.endB = readHigs[readI]; }
					}
					else{ newReg.endB = readHigs[readI]; }
					newReg.priority = cpyReg->priority;
					newReg.regCosts = cpyReg->regCosts;
					if(selMang){ selMang->performActions(&(newReg.regCosts)); }
					allRegions.push_back(newReg);
				}
				
			}
		}
}

void PositionDependentCostKDTree::regionsBiQualityMangled(PositionDependentCostKDTree* toMangle, PositionalBiQualityMangleSet* useMangle, std::vector<char>* readQualsA, std::vector<char>* readQualsB){
	allNodes = 0;
	allRegions.clear();
	PositionDependentCostRegion newReg;
	//look for runs of quality
		std::deque< std::pair<uintptr_t,uintptr_t> > lowQueue;
		std::deque< std::pair<uintptr_t,uintptr_t> > highQueue;
		std::vector< std::pair<uintptr_t,uintptr_t> > runsLow;
		std::vector< std::pair<uintptr_t,uintptr_t> > runsHigh;
		std::vector<AlignCostAffineMangleSet*> runsQual;
		lowQueue.push_back( std::pair<uintptr_t,uintptr_t>(0,0) ); highQueue.push_back( std::pair<uintptr_t,uintptr_t>(readQualsA->size(), readQualsB->size()) );
		while(lowQueue.size()){
			std::pair<uintptr_t,uintptr_t> curL = lowQueue[0];
			std::pair<uintptr_t,uintptr_t> curH = highQueue[0];
			lowQueue.pop_front(); highQueue.pop_front();
			//look for a square
			uintptr_t curIA = curL.first; uintptr_t curIB = curL.second;
			AlignCostAffineMangleSet* curMang = useMangle->getMangleForQualities((*readQualsA)[curIA], (*readQualsB)[curIB]);
			curIA++; curIB++;
			while((curIA < curH.first) && (curIB < curH.second)){
				bool canExp = true;
				for(uintptr_t ia = curL.first; ia<curIA; ia++){
					AlignCostAffineMangleSet* testMang = useMangle->getMangleForQualities((*readQualsA)[ia], (*readQualsB)[curIB]);
					if(testMang != curMang){ canExp = false; break; }
				}
				if(!canExp){ break; }
				for(uintptr_t ib = curL.second; ib<curIB; ib++){
					AlignCostAffineMangleSet* testMang = useMangle->getMangleForQualities((*readQualsA)[curIA], (*readQualsB)[ib]);
					if(testMang != curMang){ canExp = false; break; }
				}
				if(!canExp){ break; }
				AlignCostAffineMangleSet* testMang = useMangle->getMangleForQualities((*readQualsA)[curIA], (*readQualsB)[curIB]);
				if(testMang != curMang){ break; }
			}
			//add the square
			runsLow.push_back( std::pair<uintptr_t,uintptr_t>(curL.first, curL.second) );
			runsHigh.push_back( std::pair<uintptr_t,uintptr_t>(curIA,curIB) );
			runsQual.push_back(curMang);
			//add anything that was missed back to the queue (keep the larger area together)
			if(curIA < curH.first){
				if(curIB < curH.second){
					if((curH.first-curIA)*(curH.second-curL.second) > (curH.second-curIB)*(curH.first-curL.first)){
						lowQueue.push_back( std::pair<uintptr_t,uintptr_t>(curIA, curL.second) );
						highQueue.push_back( std::pair<uintptr_t,uintptr_t>(curH.first, curH.second) );
						lowQueue.push_back( std::pair<uintptr_t,uintptr_t>(curL.first, curIB) );
						highQueue.push_back( std::pair<uintptr_t,uintptr_t>(curIA, curH.second) );
					}
					else{
						lowQueue.push_back( std::pair<uintptr_t,uintptr_t>(curL.first, curIB) );
						highQueue.push_back( std::pair<uintptr_t,uintptr_t>(curH.first, curH.second) );
						lowQueue.push_back( std::pair<uintptr_t,uintptr_t>(curIA, curL.second) );
						highQueue.push_back( std::pair<uintptr_t,uintptr_t>(curH.first, curIB) );
					}
				}
				else{
					lowQueue.push_back( std::pair<uintptr_t,uintptr_t>(curIA, curL.second) );
					highQueue.push_back( std::pair<uintptr_t,uintptr_t>(curH.first, curH.second) );
				}
			}
			else if(curIB < curH.second){
				lowQueue.push_back( std::pair<uintptr_t,uintptr_t>(curL.first, curIB) );
				highQueue.push_back( std::pair<uintptr_t,uintptr_t>(curH.first, curH.second) );
			}
			else{
				//all done
			}
		}
	//handle stuff before and after the reference and read (8 possibilities)
		uintptr_t readALen = readQualsA->size();
		uintptr_t readBLen = readQualsB->size();
		#define BQM_REG_LIMIT(testA, testB, doA, doB) \
			for(uintptr_t i = 0; i<toMangle->allRegions.size(); i++){\
				PositionDependentCostRegion* cpyReg = &(toMangle->allRegions[i]);\
				testA \
				testB \
				newReg = *cpyReg;\
				doA \
				doB \
				allRegions.push_back(newReg);\
			}
		#define BQM_TEST_ALOW if(cpyReg->startA >= 0){ continue; }
		#define BQM_TEST_BLOW if(cpyReg->startB >= 0){ continue; }
		#define BQM_TEST_AHIG if((cpyReg->endA >= 0) && ((uintptr_t)(cpyReg->endA) <= readALen)){ continue; }
		#define BQM_TEST_BHIG if((cpyReg->endB >= 0) && ((uintptr_t)(cpyReg->endB) <= readBLen)){ continue; }
		#define BQM_TEST_AMID if((cpyReg->startA >= 0) && ((uintptr_t)(cpyReg->startA) > readALen)){ continue; }
		#define BQM_TEST_BMID if((cpyReg->startB >= 0) && ((uintptr_t)(cpyReg->startB) > readBLen)){ continue; }
		#define BQM_DO_ALOW newReg.endA = 0;
		#define BQM_DO_BLOW newReg.endB = 0;
		#define BQM_DO_AHIG newReg.endA = readALen;
		#define BQM_DO_BHIG newReg.endB = readBLen;
		#define BQM_DO_AMID
		#define BQM_DO_BMID
		BQM_REG_LIMIT(BQM_TEST_ALOW, BQM_TEST_BLOW, BQM_DO_ALOW, BQM_DO_BLOW)
		BQM_REG_LIMIT(BQM_TEST_AMID, BQM_TEST_BLOW, BQM_DO_AMID, BQM_DO_BLOW)
		BQM_REG_LIMIT(BQM_TEST_AHIG, BQM_TEST_BLOW, BQM_DO_AHIG, BQM_DO_BLOW)
		BQM_REG_LIMIT(BQM_TEST_ALOW, BQM_TEST_BMID, BQM_DO_ALOW, BQM_DO_BMID)
		BQM_REG_LIMIT(BQM_TEST_AHIG, BQM_TEST_BMID, BQM_DO_AHIG, BQM_DO_BMID)
		BQM_REG_LIMIT(BQM_TEST_ALOW, BQM_TEST_BHIG, BQM_DO_ALOW, BQM_DO_BHIG)
		BQM_REG_LIMIT(BQM_TEST_AMID, BQM_TEST_BHIG, BQM_DO_AMID, BQM_DO_BHIG)
		BQM_REG_LIMIT(BQM_TEST_AHIG, BQM_TEST_BHIG, BQM_DO_AHIG, BQM_DO_BHIG)
	//and handle the interior
		for(uintptr_t ri = 0; ri<runsLow.size(); ri++){
			std::pair<uintptr_t,uintptr_t> curL = runsLow[ri];
			std::pair<uintptr_t,uintptr_t> curH = runsHigh[ri];
			AlignCostAffineMangleSet* curMang = runsQual[ri];
			for(uintptr_t i = 0; i<toMangle->allRegions.size(); i++){
				PositionDependentCostRegion* cpyReg = &(toMangle->allRegions[i]);
				if((cpyReg->endA >= 0) && ((uintptr_t)(cpyReg->endA) < curL.first)){ continue; }
				if((cpyReg->startA >= 0) && ((uintptr_t)(cpyReg->startA) >= curH.first)){ continue; }
				if((cpyReg->endB >= 0) && ((uintptr_t)(cpyReg->endB) < curL.second)){ continue; }
				if((cpyReg->startB >= 0) && ((uintptr_t)(cpyReg->startB) >= curH.second)){ continue; }
				
				if((cpyReg->startA < 0) || ((uintptr_t)(cpyReg->startA) < curL.first)){ newReg.startA = curL.first; }
					else{ newReg.startA = cpyReg->startA; }
				if((cpyReg->endA < 0) || ((uintptr_t)(cpyReg->endA) > curH.first)){ newReg.endA = curH.first; }
					else{ newReg.endA = cpyReg->endA; }
				if((cpyReg->startB < 0) || ((uintptr_t)(cpyReg->startB) < curL.second)){ newReg.startB = curL.second; }
					else{ newReg.startB = cpyReg->startB; }
				if((cpyReg->endB < 0) || ((uintptr_t)(cpyReg->endB) > curH.second)){ newReg.endB = curH.second; }
					else{ newReg.endB = cpyReg->endB; }
				
				newReg.priority = cpyReg->priority;
				newReg.regCosts = cpyReg->regCosts;
				if(curMang){ curMang->performActions(&(newReg.regCosts)); }
				allRegions.push_back(newReg);
			}
		}
}

const char* PositionDependentCostKDTree::regionsParse(const char* parseS, const char* parseE){
	allNodes = 0;
	allRegions.clear();
	std::vector<const char*> tokS;
	std::vector<const char*> tokE;
	splitOnCharacters(parseS, parseE, strlen(WHITESPACE), WHITESPACE, &tokS, &tokE);
	uintptr_t curI = 0;
	std::string curEntP;
	std::vector<const char*>::iterator nextTokIt;
	#define SPDPARSE_SCANNEXT curEntP.clear(); while(curI < tokS.size()){ if(tokE[curI]-tokS[curI]){break;} curI++; } if(curI >= tokS.size()){ throw std::runtime_error("Missing required item for position dependent affine gap cost."); } curEntP.insert(curEntP.end(), tokS[curI], tokE[curI]); curI++;
	#define SPDFIGURE_SKIP(fromPos) nextTokIt = std::lower_bound(tokS.begin(), tokS.end(), fromPos); curI = nextTokIt - tokS.begin();
	//parse the default
		AlignCostAffine defSpec;
		const char* defEnd = defSpec.parseSpecification(parseS, parseE);
		SPDFIGURE_SKIP(defEnd);
	//get the number of (extra) regions
		SPDPARSE_SCANNEXT uintptr_t numSpec = atol(curEntP.c_str());
	//add the default
		allRegions.resize(numSpec+1);
		PositionDependentCostRegion* defReg = &(allRegions[0]);
			defReg->startA = -1; defReg->endA = -1;
			defReg->startB = -1; defReg->endB = -1;
			defReg->priority = 0;
			defReg->regCosts = defSpec;
	//parse the extras
		for(uintptr_t i = 0; i<numSpec; i++){
			PositionDependentCostRegion* curReg = &(allRegions[i+1]);
			SPDPARSE_SCANNEXT curReg->startA = atol(curEntP.c_str());
			SPDPARSE_SCANNEXT curReg->endA = atol(curEntP.c_str());
			SPDPARSE_SCANNEXT curReg->startB = atol(curEntP.c_str());
			SPDPARSE_SCANNEXT curReg->endB = atol(curEntP.c_str());
			curReg->priority = (curReg->startA >= 0) && (curReg->endA >= 0) && (curReg->startB >= 0) && (curReg->endB >= 0);
			const char* curEnd = curReg->regCosts.parseSpecification(tokS[curI], parseE);
			SPDFIGURE_SKIP(curEnd)
		}
	return (curI < tokS.size()) ? tokS[curI] : parseE;
}

#define CHECK_PDB_APPLICABLE(aind, bind, limCheck) \
	for(uintptr_t i = 0; i<curFoc->allSpans.size(); i++){\
		PositionDependentCostRegion* curBnd = curFoc->allSpans[i];\
		bool isAfterAS = (curBnd->startA < 0) || (curBnd->startA <= aind);\
		bool isBeforeAE = (curBnd->endA < 0) || (curBnd->endA > aind);\
		bool isAfterBS = (curBnd->startB < 0) || (curBnd->startB <= bind);\
		bool isBeforeBE = (curBnd->endB < 0) || (curBnd->endB > bind);\
		limCheck \
		if(isAfterAS && isBeforeAE && isAfterBS && isBeforeBE){\
			if(curWin && (winPriority > curBnd->priority)){\
				continue;\
			}\
			curWin = curBnd;\
			winPriority = curBnd->priority;\
		}\
	}

AlignCostAffine* PositionDependentCostKDTree::getCostsAt(intptr_t inA, intptr_t inB){
	PositionDependentCostRegion* curWin = 0;
	int winPriority = 0;
	PositionDependentCostKDNode* curFoc = allNodes;
	while(curFoc){
		CHECK_PDB_APPLICABLE(inA, inB, ;)
		if(curFoc->splitOn == POSITIONDEPENDENT_SPLITAXIS_A){
			curFoc = (PositionDependentCostKDNode*)((inA < curFoc->splitAt) ? curFoc->subNodeLesser : curFoc->subNodeGreatE);
		}
		else if(curFoc->splitOn == POSITIONDEPENDENT_SPLITAXIS_B){
			curFoc = (PositionDependentCostKDNode*)((inB < curFoc->splitAt) ? curFoc->subNodeLesser : curFoc->subNodeGreatE);
		}
	}
	return curWin ? &(curWin->regCosts) : 0;
}

void PositionDependentCostKDTree::getCostsForA(intptr_t inA, intptr_t fromB, intptr_t toB, std::vector<AlignCostAffine*>* saveRes){
	saveRes->clear();
	int curLB = fromB;
	while(curLB < toB){
		//loop down the tree, get winner PDB
		int lastBLim = toB;
		PositionDependentCostRegion* curWin = 0;
		int winPriority = 0;
		PositionDependentCostKDNode* curFoc = allNodes;
		while(curFoc){
			CHECK_PDB_APPLICABLE(inA, curLB, if((curBnd->startB >= 0) && (curBnd->startB > curLB) && (curBnd->startB < lastBLim)){lastBLim = curBnd->startB;} if((curBnd->endB >= 0) && (curBnd->endB > curLB) && (curBnd->endB < lastBLim)){lastBLim = curBnd->endB;} )
			if(curFoc->splitOn == POSITIONDEPENDENT_SPLITAXIS_A){
				curFoc = (PositionDependentCostKDNode*)((inA < curFoc->splitAt) ? curFoc->subNodeLesser : curFoc->subNodeGreatE);
			}
			else if(curFoc->splitOn == POSITIONDEPENDENT_SPLITAXIS_B){
				if(curLB < curFoc->splitAt){
					lastBLim = (lastBLim < curFoc->splitAt) ? lastBLim : curFoc->splitAt;
				}
				curFoc = (PositionDependentCostKDNode*)((curLB < curFoc->splitAt) ? curFoc->subNodeLesser : curFoc->subNodeGreatE);
			}
		}
		if(curWin){
			if((curWin->endB >= 0) && (curWin->endB < lastBLim)){
				lastBLim = curWin->endB;
			}
			saveRes->insert(saveRes->end(), lastBLim-curLB, &(curWin->regCosts));
			curLB = lastBLim;
		}
		else{
			saveRes->push_back(0);
			curLB++;
		}
	}
}

void PositionDependentCostKDTree::getCostsForB(intptr_t fromA, intptr_t toA, intptr_t inB, std::vector<AlignCostAffine*>* saveRes){
	saveRes->clear();
	int curLA = fromA;
	while(curLA < toA){
		//loop down the tree, get winner PDB
		int lastALim = toA;
		PositionDependentCostRegion* curWin = 0;
		int winPriority = 0;
		PositionDependentCostKDNode* curFoc = allNodes;
		while(curFoc){
			CHECK_PDB_APPLICABLE(curLA, inB, if((curBnd->startA >= 0) && (curBnd->startA > curLA) && (curBnd->startA < lastALim)){lastALim = curBnd->startA;} if((curBnd->endA >= 0) && (curBnd->endA > curLA) && (curBnd->endA < lastALim)){lastALim = curBnd->endA;} )
			if(curFoc->splitOn == POSITIONDEPENDENT_SPLITAXIS_A){
				if(curLA < curFoc->splitAt){
					lastALim = (lastALim < curFoc->splitAt) ? lastALim : curFoc->splitAt;
				}
				curFoc = (PositionDependentCostKDNode*)((curLA < curFoc->splitAt) ? curFoc->subNodeLesser : curFoc->subNodeGreatE);
			}
			else if(curFoc->splitOn == POSITIONDEPENDENT_SPLITAXIS_B){
				curFoc = (PositionDependentCostKDNode*)((inB < curFoc->splitAt) ? curFoc->subNodeLesser : curFoc->subNodeGreatE);
			}
		}
		if(curWin){
			if((curWin->endA >= 0) && (curWin->endA < lastALim)){
				lastALim = curWin->endA;
			}
			saveRes->insert(saveRes->end(), lastALim-curLA, &(curWin->regCosts));
			curLA = lastALim;
		}
		else{
			saveRes->push_back(0);
			curLA++;
		}
	}
}

/**Debug output a kd tree node.*/
void outputPDKDTreeNode(std::ostream& os, const PositionDependentCostRegion* baseReg, const PositionDependentCostKDNode* curNode){
	os << "<" << curNode->splitOn << "|" << curNode->splitAt << ">";
	for(uintptr_t i = 0; i<curNode->allSpans.size(); i++){
		os << " " << (curNode->allSpans[i] - baseReg);
	}
	os << "(";
	if(curNode->subNodeLesser){
		outputPDKDTreeNode(os, baseReg, curNode->subNodeLesser);
	}
	os << ")";
	os << "(";
	if(curNode->subNodeGreatE){
		outputPDKDTreeNode(os, baseReg, curNode->subNodeGreatE);
	}
	os << ")";
}

std::ostream& operator<<(std::ostream& os, const PositionDependentCostKDTree& toOut){
	for(uintptr_t i = 0; i<toOut.allRegions.size(); i++){
		const PositionDependentCostRegion* curReg = &(toOut.allRegions[i]);
		os << i << " " << curReg->startA << " " << curReg->endA << " " << curReg->startB << " " << curReg->endB << " " << curReg->regCosts << std::endl;
	}
	if(toOut.allNodes){
		os << "(";
		outputPDKDTreeNode(os, &(toOut.allRegions[0]), toOut.allNodes);
		os << ")" << std::endl;
	}
	return os;
}

void parseMultiregionPositionDependentCost(const char* parseS, const char* parseE, std::map<std::string,PositionDependentCostKDTree>* toFill){
	std::vector<std::string> subNames;
	std::vector<const char*> subStarts;
	std::vector<const char*> subEnds;
	//look for pipes
		const char* curS = parseS;
		const char* curE = (const char*)memchr(curS, '|', parseE - curS);
		while(curE){
			curS = curS + strspn(curS, WHITESPACE);
			const char* namE = curS + strcspn(curS, " \t\r\n|");
			subNames.push_back(std::string(curS, namE));
			curS = namE;
			subStarts.push_back(curS);
			subEnds.push_back(curE);
			curS = curE+1;
			curE = (const char*)memchr(curS, '|', parseE - curS);
		}
	//pre-make the things (less copying)
		PositionDependentCostKDTree tmpSet;
		for(uintptr_t i = 0; i<subNames.size(); i++){
			(*toFill)[subNames[i]] = tmpSet;
		}
	//parse the things
		for(uintptr_t i = 0; i<subNames.size(); i++){
			(*toFill)[subNames[i]].regionsParse(subStarts[i], subEnds[i]);
		}
	//build trees
		for(std::map<std::string,PositionDependentCostKDTree>::iterator costIt = toFill->begin(); costIt != toFill->end(); costIt++){
			costIt->second.produceFromRegions();
		}
}

PositionDependentAffineGapLinearPairwiseAlignment::PositionDependentAffineGapLinearPairwiseAlignment(){
	numEnds = 4;
	alnCosts = 0;
	seqAs = 0;
	seqBs = 0;
	costTable = 0;
	saveAlloc = (intptr_t**)malloc(8*sizeof(intptr_t*));
	saveSize = 8;
}

PositionDependentAffineGapLinearPairwiseAlignment::PositionDependentAffineGapLinearPairwiseAlignment(int numSeqEnds, std::string* refSeq, std::string* readSeq, PositionDependentCostKDTree* alnCost){
	numEnds = numSeqEnds;
	alnCosts = alnCost;
	seqAs = refSeq;
	seqBs = readSeq;
	costTable = 0;
	saveAlloc = (intptr_t**)malloc(8*sizeof(intptr_t*));
	saveSize = 8;
}

PositionDependentAffineGapLinearPairwiseAlignment::~PositionDependentAffineGapLinearPairwiseAlignment(){
	free(saveAlloc);
}

void PositionDependentAffineGapLinearPairwiseAlignment::changeProblem(int numSeqEnds, std::string* refSeq, std::string* readSeq, PositionDependentCostKDTree* alnCost){
	numEnds = numSeqEnds;
	alnCosts = alnCost;
	seqAs = refSeq;
	seqBs = readSeq;
	costTable = 0;
}

void PositionDependentAffineGapLinearPairwiseAlignment::prepareAlignmentStructure(){
	if(costTable){ return; }
	//allocate the stupid thing
		uintptr_t numTabEnts = (seqAs->size()+1)*(seqBs->size()+1);
		uintptr_t numLineEnts = (seqBs->size()+1);
		uintptr_t numPtrEnts = (seqAs->size()+1);
		uintptr_t numAlSing = numTabEnts*sizeof(intptr_t) + numPtrEnts*sizeof(intptr_t*);
		uintptr_t totNumAlloc = numAlSing*13;
		if(totNumAlloc > saveSize){
			free(saveAlloc);
			saveAlloc = (intptr_t**)malloc(totNumAlloc);
			saveSize = totNumAlloc;
		}
		costTable = saveAlloc;
		matchTable = costTable + numPtrEnts;
		matchMatchTable = matchTable + numPtrEnts;
		matchSkipATable = matchMatchTable + numPtrEnts;
		matchSkipBTable = matchSkipATable + numPtrEnts;
		skipATable = matchSkipBTable + numPtrEnts;
		skipAMatchTable = skipATable + numPtrEnts;
		skipASkipATable = skipAMatchTable + numPtrEnts;
		skipASkipBTable = skipASkipATable + numPtrEnts;
		skipBTable = skipASkipBTable + numPtrEnts;
		skipBMatchTable = skipBTable + numPtrEnts;
		skipBSkipATable = skipBMatchTable + numPtrEnts;
		skipBSkipBTable = skipBSkipATable + numPtrEnts;
		intptr_t* curFoc = (intptr_t*)(skipBSkipBTable + numPtrEnts);
		uintptr_t numSetLin = numPtrEnts * 13;
		for(uintptr_t i = 0; i<numSetLin; i++){
			costTable[i] = curFoc;
			curFoc += numLineEnts;
		}
	//get some commons
		intptr_t lenA = seqAs->size();
		intptr_t lenB = seqBs->size();
		const char* seqA = seqAs->c_str();
		const char* seqB = seqBs->c_str();
		intptr_t worstScore = -1; worstScore = worstScore << (8*sizeof(intptr_t)-1);
		intptr_t negToZero = (numEnds == 0)-1;
		intptr_t startIJ;
		switch(numEnds){
			case 0: startIJ = 0; break;
			case 2: startIJ = 0; break;
			case 4: startIJ = -1; break;
			default:
				return;
		};
	//helpful space for variables
		AlignCostAffine* curCost;
		AlignCostAffine* matCost;
		AlignCostAffine* skaCost;
		AlignCostAffine* skbCost;
		curCosts.clear();
		matCosts.clear();
		skaCosts.clear();
		skbCosts.clear();
		intptr_t scoreDiff;
		intptr_t diffSign;
		intptr_t scoreMax;
	//some helpful code pieces
	#define GET_COST_SCAN \
		alnCosts->getCostsForA(i-1, -1, lenB, &curCosts);\
		alnCosts->getCostsForA(i-2, -2, lenB-1, &matCosts);\
		alnCosts->getCostsForA(i-2, -1, lenB, &skaCosts);\
		alnCosts->getCostsForA(i-1, -2, lenB-1, &skbCosts);
	#define NEGATIVE_GUARD(forVal) forVal = (negToZero | ((forVal < 0)-1)) & forVal;
	#define GET_MAX_THREE(itemA,itemB,itemC) \
		scoreMax = itemA;\
		scoreDiff = (scoreMax - itemB);\
		diffSign = scoreDiff >> (8*sizeof(intptr_t)-1);\
		scoreMax = scoreMax - (scoreDiff & diffSign);\
		scoreDiff = (scoreMax - itemC);\
		diffSign = scoreDiff >> (8*sizeof(intptr_t)-1);\
		scoreMax = scoreMax - (scoreDiff & diffSign);
	//fill in the stupid thing
	intptr_t i = 0;
	{
		GET_COST_SCAN
		intptr_t j = 0;
		{
			intptr_t skipSets = startIJ & worstScore;
			matchMatchTable[i][j] = 0;
			matchSkipATable[i][j] = worstScore;
			matchSkipBTable[i][j] = worstScore;
			matchTable[i][j] = 0;
			skipAMatchTable[i][j] = skipSets;
			skipASkipATable[i][j] = skipSets;
			skipASkipBTable[i][j] = skipSets;
			skipATable[i][j] = skipSets;
			skipBMatchTable[i][j] = skipSets;
			skipBSkipATable[i][j] = skipSets;
			skipBSkipBTable[i][j] = skipSets;
			skipBTable[i][j] = skipSets;
			costTable[i][j] = 0;
		}
		j = 1;
		{
			curCost = curCosts[j];
			intptr_t skipSets = startIJ & worstScore;
			matchMatchTable[i][j] = worstScore;
			matchSkipATable[i][j] = worstScore;
			matchSkipBTable[i][j] = worstScore;
			matchTable[i][j] = worstScore;
			skipAMatchTable[i][j] = skipSets;
			skipASkipATable[i][j] = skipSets;
			skipASkipBTable[i][j] = skipSets;
			skipATable[i][j] = skipSets;
			intptr_t winBM = startIJ & (matchTable[i][j-1] + curCost->openCost + curCost->extendCost);
			skipBMatchTable[i][j] = winBM;
			skipBSkipATable[i][j] = skipSets;
			skipBSkipBTable[i][j] = skipSets;
			skipBTable[i][j] = winBM;
			costTable[i][j] = winBM;
		}
		for(j = 2; j<=lenB; j++){
			curCost = curCosts[j];
			intptr_t skipSets = startIJ & worstScore;
			matchMatchTable[i][j] = worstScore;
			matchSkipATable[i][j] = worstScore;
			matchSkipBTable[i][j] = worstScore;
			matchTable[i][j] = worstScore;
			skipAMatchTable[i][j] = skipSets;
			skipASkipATable[i][j] = skipSets;
			skipASkipBTable[i][j] = skipSets;
			skipATable[i][j] = skipSets;
			skipBMatchTable[i][j] = skipSets;
			skipBSkipATable[i][j] = skipSets;
			intptr_t winBB = startIJ & (skipBTable[i][j-1] + curCost->extendCost);
			skipBSkipBTable[i][j] = winBB;
			skipBTable[i][j] = winBB;
			costTable[i][j] = winBB;
		}
	}
	i = 1;
	{
		GET_COST_SCAN
		intptr_t j = 0;
		{
			curCost = curCosts[j];
			intptr_t skipSets = startIJ & worstScore;
			matchMatchTable[i][j] = worstScore;
			matchSkipATable[i][j] = worstScore;
			matchSkipBTable[i][j] = worstScore;
			matchTable[i][j] = worstScore;
			intptr_t winAM = startIJ & (matchTable[i-1][j] + curCost->openCost + curCost->extendCost);
			skipAMatchTable[i][j] = winAM;
			skipASkipATable[i][j] = skipSets;
			skipASkipBTable[i][j] = skipSets;
			skipATable[i][j] = winAM;
			skipBMatchTable[i][j] = skipSets;
			skipBSkipATable[i][j] = skipSets;
			skipBSkipBTable[i][j] = skipSets;
			skipBTable[i][j] = skipSets;
			costTable[i][j] = winAM;
		}
		j = 1;
		{
			curCost = curCosts[j];
			skaCost = skaCosts[j];
			skbCost = skbCosts[j];
			intptr_t matchCost = curCost->allMMCost[curCost->charMap[0x00FF&seqA[i-1]]][curCost->charMap[0x00FF&seqB[j-1]]];
			intptr_t winMM = matchTable[i-1][j-1] + matchCost;
			NEGATIVE_GUARD(winMM)
			matchMatchTable[i][j] = winMM;
			matchSkipATable[i][j] = worstScore;
			matchSkipBTable[i][j] = worstScore;
			matchTable[i][j] = winMM;
			skipAMatchTable[i][j] = worstScore;
			skipASkipATable[i][j] = worstScore;
			intptr_t winAB = skipBTable[i-1][j] + (startIJ & skaCost->closeCost) + curCost->openCost + curCost->extendCost;
			NEGATIVE_GUARD(winAB)
			skipASkipBTable[i][j] = winAB;
			skipATable[i][j] = winAB;
			skipBMatchTable[i][j] = worstScore;
			intptr_t winBA = skipATable[i][j-1] + (startIJ & skbCost->closeCost) + curCost->openCost + curCost->extendCost;
			NEGATIVE_GUARD(winBA)
			skipBSkipATable[i][j] = winBA;
			skipBSkipBTable[i][j] = worstScore;
			skipBTable[i][j] = winBA;
			GET_MAX_THREE(winMM, winAB, winBA)
			costTable[i][j] = scoreMax;
		}
		for(j = 2; j<=lenB; j++){
			curCost = curCosts[j];
			skaCost = skaCosts[j];
			skbCost = skbCosts[j];
			matCost = matCosts[j];
			intptr_t matchCost = curCost->allMMCost[curCost->charMap[0x00FF&seqA[i-1]]][curCost->charMap[0x00FF&seqB[j-1]]];
			matchMatchTable[i][j] = worstScore;
			matchSkipATable[i][j] = worstScore;
			intptr_t winMB = skipBTable[i-1][j-1] + (startIJ & matCost->closeCost) + matchCost;
			NEGATIVE_GUARD(winMB)
			matchSkipBTable[i][j] = winMB;
			matchTable[i][j] = winMB;
			skipAMatchTable[i][j] = worstScore;
			skipASkipATable[i][j] = worstScore;
			intptr_t winAB = skipBTable[i-1][j] + (startIJ & skaCost->closeCost) + curCost->openCost + curCost->extendCost;
			NEGATIVE_GUARD(winAB)
			skipASkipBTable[i][j] = winAB;
			skipATable[i][j] = winAB;
			intptr_t winBM = matchTable[i][j-1] + curCost->openCost + curCost->extendCost;
			NEGATIVE_GUARD(winBM)
			skipBMatchTable[i][j] = winBM;
			intptr_t winBA = skipATable[i][j-1] + (startIJ & skbCost->closeCost) + curCost->openCost + curCost->extendCost;
			NEGATIVE_GUARD(winBA)
			skipBSkipATable[i][j] = winBA;
			intptr_t winBB = skipBTable[i][j-1] + curCost->extendCost;
			NEGATIVE_GUARD(winBB)
			skipBSkipBTable[i][j] = winBB;
			GET_MAX_THREE(winBM,winBA,winBB)
			skipBTable[i][j] = scoreMax;
			GET_MAX_THREE(matchTable[i][j], skipATable[i][j], skipBTable[i][j])
			costTable[i][j] = scoreMax;
		}
	}
	for(i = 2; i<=lenA; i++){
		GET_COST_SCAN
		intptr_t j = 0;
		{
			curCost = curCosts[j];
			intptr_t skipSets = startIJ & worstScore;
			matchMatchTable[i][j] = worstScore;
			matchSkipATable[i][j] = worstScore;
			matchSkipBTable[i][j] = worstScore;
			matchTable[i][j] = worstScore;
			skipAMatchTable[i][j] = skipSets;
			intptr_t winAA = startIJ & (skipATable[i-1][j] + curCost->extendCost);
			skipASkipATable[i][j] = winAA;
			skipASkipBTable[i][j] = skipSets;
			skipATable[i][j] = winAA;
			skipBMatchTable[i][j] = skipSets;
			skipBSkipATable[i][j] = skipSets;
			skipBSkipBTable[i][j] = skipSets;
			skipBTable[i][j] = skipSets;
			costTable[i][j] = winAA;
		}
		j = 1;
		{
			curCost = curCosts[j];
			skaCost = skaCosts[j];
			skbCost = skbCosts[j];
			matCost = matCosts[j];
			intptr_t matchCost = curCost->allMMCost[curCost->charMap[0x00FF&seqA[i-1]]][curCost->charMap[0x00FF&seqB[j-1]]];
			matchMatchTable[i][j] = worstScore;
			intptr_t winMA = skipATable[i-1][j-1] + (startIJ & matCost->closeCost) + matchCost;
			NEGATIVE_GUARD(winMA)
			matchSkipATable[i][j] = winMA;
			matchSkipBTable[i][j] = worstScore;
			matchTable[i][j] = winMA;
			intptr_t winAM = matchTable[i-1][j] + curCost->openCost + curCost->extendCost;
			NEGATIVE_GUARD(winAM)
			skipAMatchTable[i][j] = winAM;
			intptr_t winAA = skipATable[i-1][j] + curCost->extendCost;
			NEGATIVE_GUARD(winAA)
			skipASkipATable[i][j] = winAA;
			intptr_t winAB = skipBTable[i-1][j] + (startIJ & skaCost->closeCost) + curCost->openCost + curCost->extendCost;
			NEGATIVE_GUARD(winAB)
			skipASkipBTable[i][j] = winAB;
			GET_MAX_THREE(winAM,winAA,winAB)
			skipATable[i][j] = scoreMax;
			skipBMatchTable[i][j] = worstScore;
			intptr_t winBA = skipATable[i][j-1] + (startIJ & skbCost->closeCost) + curCost->openCost + curCost->extendCost;
			NEGATIVE_GUARD(winBA)
			skipBSkipATable[i][j] = winBA;
			skipBSkipBTable[i][j] = worstScore;
			skipBTable[i][j] = winBA;
			GET_MAX_THREE(matchTable[i][j], skipATable[i][j], skipBTable[i][j])
			costTable[i][j] = scoreMax;
		}
		for(j = 2; j<=lenB; j++){
			curCost = curCosts[j];
			skaCost = skaCosts[j];
			skbCost = skbCosts[j];
			matCost = matCosts[j];
			intptr_t matchCost = curCost->allMMCost[curCost->charMap[0x00FF&seqA[i-1]]][curCost->charMap[0x00FF&seqB[j-1]]];
			intptr_t winMM = matchTable[i-1][j-1] + matchCost;
			NEGATIVE_GUARD(winMM)
			matchMatchTable[i][j] = winMM;
			intptr_t winMA = skipATable[i-1][j-1] + matCost->closeCost + matchCost;
			NEGATIVE_GUARD(winMA)
			matchSkipATable[i][j] = winMA;
			intptr_t winMB = skipBTable[i-1][j-1] + matCost->closeCost + matchCost;
			NEGATIVE_GUARD(winMB)
			matchSkipBTable[i][j] = winMB;
			GET_MAX_THREE(winMM, winMA, winMB)
			matchTable[i][j] = scoreMax;
			intptr_t winAM = matchTable[i-1][j] + curCost->openCost + curCost->extendCost;
			NEGATIVE_GUARD(winAM)
			skipAMatchTable[i][j] = winAM;
			intptr_t winAA = skipATable[i-1][j] + curCost->extendCost;
			NEGATIVE_GUARD(winAA)
			skipASkipATable[i][j] = winAA;
			intptr_t winAB = skipBTable[i-1][j] + skaCost->closeCost + curCost->openCost + curCost->extendCost;
			NEGATIVE_GUARD(winAB)
			skipASkipBTable[i][j] = winAB;
			GET_MAX_THREE(winAM, winAA, winAB)
			skipATable[i][j] = scoreMax;
			intptr_t winBM = matchTable[i][j-1] + curCost->openCost + curCost->extendCost;
			NEGATIVE_GUARD(winBM)
			skipBMatchTable[i][j] = winBM;
			intptr_t winBA = skipATable[i][j-1] + skbCost->closeCost + curCost->openCost + curCost->extendCost;
			NEGATIVE_GUARD(winBA)
			skipBSkipATable[i][j] = winBA;
			intptr_t winBB = skipBTable[i][j-1] + curCost->extendCost;
			NEGATIVE_GUARD(winBB)
			skipBSkipBTable[i][j] = winBB;
			GET_MAX_THREE(winBM, winBA, winBB)
			skipBTable[i][j] = scoreMax;
			GET_MAX_THREE(matchTable[i][j], skipATable[i][j], skipBTable[i][j])
			costTable[i][j] = scoreMax;
		}
	}
}

LinearPairwiseAlignmentIteration* PositionDependentAffineGapLinearPairwiseAlignment::getIteratorToken(){
	return new PositionDependentAffineGapLinearPairwiseAlignmentIteration(this);
}

void PositionDependentAffineGapLinearPairwiseAlignment::startOptimalIteration(LinearPairwiseAlignmentIteration* theIter){
	PositionDependentAffineGapLinearPairwiseAlignmentIteration* realIter = (PositionDependentAffineGapLinearPairwiseAlignmentIteration*)theIter;
	realIter->changeProblem(this);
}
void PositionDependentAffineGapLinearPairwiseAlignment::startFuzzyIteration(LinearPairwiseAlignmentIteration* theIter, intptr_t minScore, intptr_t maxDupDeg, intptr_t maxNumScore){
	PositionDependentAffineGapLinearPairwiseAlignmentIteration* realIter = (PositionDependentAffineGapLinearPairwiseAlignmentIteration*)theIter;
	realIter->changeProblem(this, minScore, maxDupDeg, maxNumScore);
}

#define ALIGN_NEED_MATCH_MATCH 1
#define ALIGN_NEED_MATCH_SKIPA 2
#define ALIGN_NEED_MATCH_SKIPB 4
#define ALIGN_NEED_SKIPA_MATCH 8
#define ALIGN_NEED_SKIPA_SKIPA 16
#define ALIGN_NEED_SKIPA_SKIPB 32
#define ALIGN_NEED_SKIPB_MATCH 64
#define ALIGN_NEED_SKIPB_SKIPA 128
#define ALIGN_NEED_SKIPB_SKIPB 256
#define ALIGN_NEED_MATCH (ALIGN_NEED_MATCH_MATCH | ALIGN_NEED_MATCH_SKIPA | ALIGN_NEED_MATCH_SKIPB)
#define ALIGN_NEED_SKIPA (ALIGN_NEED_SKIPA_MATCH | ALIGN_NEED_SKIPA_SKIPA | ALIGN_NEED_SKIPA_SKIPB)
#define ALIGN_NEED_SKIPB (ALIGN_NEED_SKIPB_MATCH | ALIGN_NEED_SKIPB_SKIPA | ALIGN_NEED_SKIPB_SKIPB)

/**
 * Guards a direction entry.
 * @param fI The i index.
 * @param fJ The j index.
 * @param dirFlag The paths to go down.
 * @return The guarded paths to go down.
 */
intptr_t focusStackGuardFlag(intptr_t fI, intptr_t fJ, intptr_t dirFlag){
	intptr_t liveDirs = dirFlag;
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

/**
 * Make an initializer for a PositionDependentAGLPFocusStackEntry.
 * @param fI The i index.
 * @param fJ The j index.
 * @param dirFlag The paths to go down.
 * @param pathSc The score of the path this is on.
 * @param howG How this node was arrived at. 0 for start.
 */
#define NEW_FOCUS_STACK_ENTRY(fI, fJ, dirFlag, pathSc, howG) (PositionDependentAGLPFocusStackEntry){fI, fJ, focusStackGuardFlag(fI,fJ,dirFlag), pathSc, howG, 0}

/**
 * Sorting method for stack starting locations.
 * @param itemA The first item.
 * @param itemB The second item.
 * @return Whether itemA's score should come before itemB. (greater than).
 */
bool stackStartPositionSortFunction(const std::pair<intptr_t,PositionDependentAGLPFocusStackEntry>& itemA, const std::pair<intptr_t,PositionDependentAGLPFocusStackEntry>& itemB){
	return itemA.first > itemB.first;
}

/**A collection of starting locations.*/
class PositionDependentAGLPFocusStackStarts{
public:
	/**Sets up an empty set of stack starts.*/
	PositionDependentAGLPFocusStackStarts(){
		curSorted = false;
	}
	/**Delete memory.*/
	~PositionDependentAGLPFocusStackStarts(){
	}
	/**
	 * This will add a starting location.
	 * @param atI The i coordinate to work from.
	 * @param atJ The j coordinate to work from.
	 * @param inDirs The future directions to consider.
	 * @param atCost The score of this location.
	 */
	void addStartingLocation(intptr_t atI, intptr_t atJ, intptr_t inDirs, intptr_t atCost){
		PositionDependentAGLPFocusStackEntry addStack = NEW_FOCUS_STACK_ENTRY(atI, atJ, inDirs, atCost, 0);
		startingLocs.push_back( std::pair<intptr_t,PositionDependentAGLPFocusStackEntry>(atCost, addStack) );
		curSorted = false;
	}
	/**
	 * Limits the number of starting location scores.
	 * @param toNumber The number of scores to limit to.
	 */
	void limitStartingLocationScores(intptr_t toNumber){
		if(startingLocs.size() == 0){ return; }
		if(toNumber <= 0){ startingLocs.clear(); return; }
		//get all the scores
		hotScores.clear();
		for(uintptr_t i = 0; i<startingLocs.size(); i++){
			hotScores.insert(startingLocs[i].first);
			if(hotScores.size() > (uintptr_t)toNumber){
				std::set<intptr_t, std::greater<intptr_t> >::iterator killIt = hotScores.end();
				killIt--;
				hotScores.erase(killIt);
			}
		}
		//remove anything not in the set
		uintptr_t setI = 0;
		for(uintptr_t i = 0; i<startingLocs.size(); i++){
			if(hotScores.count(startingLocs[i].first)){
				startingLocs[setI] = startingLocs[i];
				setI++;
			}
		}
		startingLocs.erase(startingLocs.begin() + setI, startingLocs.end());
	}
	/**
	 * This will limit starting locations to scores above some threshold.
	 * @param minScore The threshold.
	 */
	void limitStartingLocationsByScore(intptr_t minScore){
		//remove anything low
		uintptr_t setI = 0;
		for(uintptr_t i = 0; i<startingLocs.size(); i++){
			if(startingLocs[i].first >= minScore){
				startingLocs[setI] = startingLocs[i];
				setI++;
			}
		}
		startingLocs.erase(startingLocs.begin() + setI, startingLocs.end());
	}
	/**Sort starting positions by score.*/
	void sortStartingLocations(){
		if(!curSorted){
			curSorted = true;
			std::stable_sort(startingLocs.begin(), startingLocs.end(), stackStartPositionSortFunction);
		}
	}
	/**Whether the thing is currently sorted.*/
	bool curSorted;
	/**The starting locations of interest, with their scores as a key.*/
	std::vector< std::pair<intptr_t,PositionDependentAGLPFocusStackEntry> > startingLocs;
	/**Save a set for use*/
	std::set<intptr_t, std::greater<intptr_t> > hotScores;
	/**Save a vector for use*/
	std::vector<AlignCostAffine*> costSave;
};

/**
 * This will find alignment starting positions for a score search.
 * @param forProb The problem in question.
 * @param maxNumScore The maximum number of scores to keep track of.
 * @param toFill The place to put the found starting positions.
 */
void findStartingPositions(PositionDependentAffineGapLinearPairwiseAlignment* forProb, uintptr_t maxNumScore, PositionDependentAGLPFocusStackStarts* toFill){
	std::set<intptr_t, std::greater<intptr_t> >* hotScores = &(toFill->hotScores);
	std::vector<AlignCostAffine*>* costSave = &(toFill->costSave);
	hotScores->clear();
	costSave->clear();
	//save some info
		//int** costTable = forProb->costTable;
		intptr_t** matchTable = forProb->matchTable;
		intptr_t** skipATable = forProb->skipATable;
		intptr_t** skipBTable = forProb->skipBTable;
		intptr_t lenA = forProb->seqAs->size();
		intptr_t lenB = forProb->seqBs->size();
	AlignCostAffine* curCost;
	toFill->curSorted = false;
	toFill->startingLocs.clear();
	PositionDependentAGLPFocusStackStarts* toRet = toFill;
	int haveBreak = 0;
	intptr_t breakScore = 0;
	intptr_t startScore;
	std::set<intptr_t, std::greater<intptr_t> >::iterator killIt;
	#define CONSIDER_ADD_START(atI, atJ, needDir, scoreExpr) \
		startScore = scoreExpr;\
		if(!haveBreak || (startScore >= breakScore)){\
			hotScores->insert(startScore);\
			if(hotScores->size() > maxNumScore){\
				killIt = hotScores->end();\
				killIt--;\
				hotScores->erase(killIt);\
				killIt = hotScores->end();\
				killIt--;\
				breakScore = *killIt;\
				haveBreak = 1;\
			}\
		}\
		if(hotScores->count(startScore)){\
			toRet->addStartingLocation(atI, atJ, needDir, startScore);\
		}
	//add a starting location for each direction: add close penalty if applicable
	switch(forProb->numEnds){
		case 4:
			curCost = forProb->alnCosts->getCostsAt(lenA-1, lenB-1);
			CONSIDER_ADD_START(lenA, lenB, ALIGN_NEED_MATCH, matchTable[lenA][lenB]);
			CONSIDER_ADD_START(lenA, lenB, ALIGN_NEED_SKIPA, skipATable[lenA][lenB] + curCost->closeCost);
			CONSIDER_ADD_START(lenA, lenB, ALIGN_NEED_SKIPB, skipBTable[lenA][lenB] + curCost->closeCost);
			break;
		case 2:
			//add all the right and bottom as starting points
			forProb->alnCosts->getCostsForA(lenA-1, -1, lenB, costSave);
			for(int j = 0; j<=lenB; j++){
				curCost = (*costSave)[j];
				if(j){ CONSIDER_ADD_START(lenA, j, ALIGN_NEED_MATCH, matchTable[lenA][j]) }
				CONSIDER_ADD_START(lenA, j, ALIGN_NEED_SKIPA, skipATable[lenA][j] + (j ? curCost->closeCost : 0))
				//if(j){ toRet->addStartingLocation(lenA, j, ALIGN_NEED_SKIPB, skipBTable[lenA][j] + curCost->closeCost); }
			}
			forProb->alnCosts->getCostsForB(-1, lenA, lenB-1, costSave);
			for(int i = 0; i<lenA; i++){
				curCost = (*costSave)[i];
				if(i){ CONSIDER_ADD_START(i, lenB, ALIGN_NEED_MATCH, matchTable[i][lenB]); }
				//if(i){ toRet->addStartingLocation(i, lenB, ALIGN_NEED_SKIPA, skipATable[i][lenB] + curCost->closeCost); }
				CONSIDER_ADD_START(i, lenB, ALIGN_NEED_SKIPB, skipBTable[i][lenB] + (i ? curCost->closeCost : 0));
			}
			break;
		case 0:
			for(int i = 0; i<=lenA; i++){
				forProb->alnCosts->getCostsForA(i-1, -1, lenB, costSave);
				for(int j = 0; j<=lenB; j++){
					curCost = (*costSave)[j];
					if(i&&j){ CONSIDER_ADD_START(i, j, ALIGN_NEED_MATCH, matchTable[i][j]); }
					if(i && j && (j<lenB)){ CONSIDER_ADD_START(i, j, ALIGN_NEED_SKIPA, skipATable[i][j] + curCost->closeCost); }
					if(j && i && (i<lenA)){ CONSIDER_ADD_START(i, j, ALIGN_NEED_SKIPB, skipBTable[i][j] + curCost->closeCost); }
				}
			}
			break;
		default:
			throw std::runtime_error("Da fuq?");
	}
	return;
}

PositionDependentAffineGapLinearPairwiseAlignmentIteration::PositionDependentAffineGapLinearPairwiseAlignmentIteration(PositionDependentAffineGapLinearPairwiseAlignment* forAln) : LinearPairwiseAlignmentIteration(forAln){
	alnStackSize = 0;
	dieHereScore = 0;
	alnStackAlloc = 16 * sizeof(PositionDependentAGLPFocusStackEntry);
	alnStack = (PositionDependentAGLPFocusStackEntry*)malloc(alnStackAlloc);
	saveExtra = new PositionDependentAGLPFocusStackStarts();
	changeProblem(forAln);
}

void PositionDependentAffineGapLinearPairwiseAlignmentIteration::changeProblem(PositionDependentAffineGapLinearPairwiseAlignment* forAln){
	PositionDependentAGLPFocusStackStarts* saveStarts = (PositionDependentAGLPFocusStackStarts*)saveExtra;
	baseAln = forAln;
	allScore.clear();
	allScoreSeen.clear();
	maxDupDeg = 0;
	maxNumScore = 1;
	findStartingPositions(forAln, maxNumScore, saveStarts);
	saveStarts->limitStartingLocationScores(1);
	minScore = saveStarts->startingLocs[0].first;
	waitingStart.resize(saveStarts->startingLocs.size());
	uintptr_t i = saveStarts->startingLocs.size();
	while(i){
		i--;
		waitingStart[i] = saveStarts->startingLocs[i].second;
	}
	uintptr_t needAlloc = sizeof(PositionDependentAGLPFocusStackEntry)*(forAln->seqAs->size() + forAln->seqBs->size() + 2);
	if(needAlloc > alnStackAlloc){
		free(alnStack);
		alnStackAlloc = needAlloc;
		alnStack = (PositionDependentAGLPFocusStackEntry*)malloc(alnStackAlloc);
	}
	alnStackSize = 0;
	iterateNextPathStep();
}

void PositionDependentAffineGapLinearPairwiseAlignmentIteration::changeProblem(PositionDependentAffineGapLinearPairwiseAlignment* forAln, intptr_t minimumScore, intptr_t maximumDupDeg, intptr_t maximumNumScore){
	PositionDependentAGLPFocusStackStarts* saveStarts = (PositionDependentAGLPFocusStackStarts*)saveExtra;
	baseAln = forAln;
	allScore.clear();
	allScoreSeen.clear();
	minScore = minimumScore;
	maxDupDeg = maximumDupDeg;
	maxNumScore = maximumNumScore;
	findStartingPositions(forAln, maxNumScore, saveStarts);
	saveStarts->limitStartingLocationsByScore(minScore);
	saveStarts->sortStartingLocations();
	waitingStart.resize(saveStarts->startingLocs.size());
	uintptr_t i = saveStarts->startingLocs.size();
	while(i){
		i--;
		waitingStart[i] = saveStarts->startingLocs[i].second;
	}
	uintptr_t needAlloc = sizeof(PositionDependentAGLPFocusStackEntry)*(forAln->seqAs->size() + forAln->seqBs->size() + 2);
	if(needAlloc > alnStackAlloc){
		free(alnStack);
		alnStackAlloc = needAlloc;
		alnStack = (PositionDependentAGLPFocusStackEntry*)malloc(alnStackAlloc);
	}
	alnStackSize = 0;
	iterateNextPathStep();
}

PositionDependentAffineGapLinearPairwiseAlignmentIteration::~PositionDependentAffineGapLinearPairwiseAlignmentIteration(){
	free(alnStack);
	PositionDependentAGLPFocusStackStarts* saveStarts = (PositionDependentAGLPFocusStackStarts*)saveExtra;
	delete(saveStarts);
}

void PositionDependentAffineGapLinearPairwiseAlignmentIteration::updateMinimumScore(intptr_t newMin){
	minScore = newMin;
}

int PositionDependentAffineGapLinearPairwiseAlignmentIteration::getNextAlignment(){
	PositionDependentAffineGapLinearPairwiseAlignment* baseAlnPD = (PositionDependentAffineGapLinearPairwiseAlignment*)baseAln;
	std::greater<intptr_t> compMeth;
	std::vector<intptr_t>::iterator insLoc;
	bool needPreClose = baseAlnPD->numEnds == 0;
	bool gotThing = false;
	while(!gotThing){
		if(hasPath()){
			//check getting the thing
			if(needPreClose){
				if(currentPathEndScore() >= minScore){
					gotThing = true;
					dumpPath();
					alnScore = currentPathEndScore();
				}
			}
			else if(pathTerminal()){
				if(currentPathScore() >= minScore){
					gotThing = true;
					dumpPath();
					alnScore = currentPathScore();
				}
			}
			//abandon if score too low
			if(currentPathScore() < minScore){
				abandonPath();
				continue;
			}
			//check for abandon given it's not sufficiently new
			if(maxDupDeg && lastIterationChangedPath()){
				intptr_t curPS = currentPathScore();
				//try to add
				insLoc = std::lower_bound(allScore.begin(), allScore.end(), curPS, compMeth);
				uintptr_t insInd = insLoc - allScore.begin();
				if(insInd >= allScore.size()){
					allScore.push_back(curPS);
					allScoreSeen.push_back(0);
				}
				else if(allScore[insInd] != curPS){
					allScore.insert(allScore.begin() + insInd, curPS);
					allScoreSeen.insert(allScoreSeen.begin() + insInd, 0);
				}
				if(allScore.size() > (uintptr_t)maxNumScore){
					minScore = allScore[maxNumScore-1];
				}
				//check for abandon
				if(allScoreSeen[insInd] >= maxDupDeg){
					abandonPath();
					continue;
				}
				allScoreSeen[insInd]++;
			}
			//move to next
			iterateNextPathStep();
		}
		else{
			return 0;
		}
	}
	return 1;
}

bool PositionDependentAffineGapLinearPairwiseAlignmentIteration::hasPath(){
	return alnStackSize != 0;
}

bool PositionDependentAffineGapLinearPairwiseAlignmentIteration::pathTerminal(){
	PositionDependentAffineGapLinearPairwiseAlignment* baseAlnPD = (PositionDependentAffineGapLinearPairwiseAlignment*)baseAln;
	PositionDependentAGLPFocusStackEntry* curFoc = (alnStack + alnStackSize - 1);
	intptr_t pi = curFoc->focI;
	intptr_t pj = curFoc->focJ;
	switch(baseAlnPD->numEnds){
		case 0:
			if(curFoc->liveDirs & ALIGN_NEED_SKIPA){
				return (baseAlnPD->skipATable[pi][pj] == 0);
			}
			else if(curFoc->liveDirs & ALIGN_NEED_MATCH){
				return (baseAlnPD->matchTable[pi][pj] == 0);
			}
			else if(curFoc->liveDirs & ALIGN_NEED_SKIPB){
				return (baseAlnPD->skipBTable[pi][pj] == 0);
			}
			else{
				return (baseAlnPD->costTable[pi][pj] == 0);
			}
		case 2:
			return (pi == 0) || (pj == 0);
		case 4:
			return (pi == 0) && (pj == 0);
	}
	return 0;
}

intptr_t PositionDependentAffineGapLinearPairwiseAlignmentIteration::currentPathScore(){
	return (alnStack + alnStackSize - 1)->pathScore;
}

intptr_t PositionDependentAffineGapLinearPairwiseAlignmentIteration::currentPathEndScore(){
	return dieHereScore;
}

void PositionDependentAffineGapLinearPairwiseAlignmentIteration::iterateNextPathStep(){
	PositionDependentAffineGapLinearPairwiseAlignment* baseAlnPD = (PositionDependentAffineGapLinearPairwiseAlignment*)baseAln;
	bool negToZero = baseAlnPD->numEnds == 0;
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
		PositionDependentAGLPFocusStackEntry* curFoc = (alnStack + alnStackSize - 1);
		intptr_t li = curFoc->focI;
		intptr_t lj = curFoc->focJ;
		#define DO_PUSH(curDirFlag, lookTable, checkTable, offI, offJ, nextDir) \
			curFoc->liveDirs = curFoc->liveDirs & (~curDirFlag);\
			intptr_t** lookTable = baseAlnPD->lookTable;\
			intptr_t** checkTable = baseAlnPD->checkTable;\
			if(negToZero && (lookTable[li][lj] == 0)){ goto tailRecursionTgt; }\
			intptr_t newScore = curFoc->pathScore + lookTable[li][lj] - checkTable[li][lj];\
			if(newScore != curFoc->pathScore){ lastIterChange = true; }\
				else{ lastIterChange = curFoc->seenDef; curFoc->seenDef = 1; }\
			*(alnStack + alnStackSize) = NEW_FOCUS_STACK_ENTRY(li - offI, lj - offJ, nextDir, newScore, curDirFlag);\
			alnStackSize++;
		#define POST_PUSH_ZCHECK(nextTable, offI, offJ, taction, faction) \
			if(negToZero){\
				intptr_t ptabVal = baseAlnPD->nextTable[li-offI][lj-offJ];\
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
			AlignCostAffine* curCost = baseAlnPD->alnCosts->getCostsAt(li, lj);\
			dieHereScore = newScore - ptabVal + curCost->openCost;
		#define POST_PUSH_TACTION_SKIP_CLOSE(offI, offJ) \
			AlignCostAffine* othCost = baseAlnPD->alnCosts->getCostsAt(li-offI, lj-offJ);\
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
}

void PositionDependentAffineGapLinearPairwiseAlignmentIteration::abandonPath(){
	alnStackSize--;
	iterateNextPathStep();
}

void PositionDependentAffineGapLinearPairwiseAlignmentIteration::dumpPath(){
	aInds.resize(alnStackSize);
	bInds.resize(alnStackSize);
	int getInd = alnStackSize-1;
	for(int i = 0; i<alnStackSize; i++){
		PositionDependentAGLPFocusStackEntry* curEnt = alnStack + getInd;
		aInds[i] = curEnt->focI;
		bInds[i] = curEnt->focJ;
		getInd--;
	}
}

bool PositionDependentAffineGapLinearPairwiseAlignmentIteration::lastIterationChangedPath(){
	return lastIterChange;
}

/**
 * Print out a table.
 * @param os The place to print.
 * @param seqAs The reference sequence.
 * @param seqBs The read sequence.
 * @param theTab The table to print.
 */
void printPositionDependentCostTable(std::ostream& os, std::string* seqAs, std::string* seqBs, intptr_t** theTab){
	uintptr_t lenA = seqAs->size();
	uintptr_t lenB = seqBs->size();
	//print out the first row
	os << "\t_";
	for(uintptr_t j = 0; j<lenB; j++){
		os << "\t" << (*seqBs)[j];
	}
	os << std::endl;
	//print out the first row of costs
	os << "_";
	for(uintptr_t j = 0; j<=lenB; j++){
		os << "\t" << theTab[0][j];
	}
	os << std::endl;
	//run down the rows
	for(uintptr_t i = 0; i<lenA; i++){
		os << (*seqAs)[i];
		for(uintptr_t j = 0; j<=lenB; j++){
			os << "\t" << theTab[i+1][j];
		}
		os << std::endl;
	}
}

std::ostream& operator<<(std::ostream& os, const PositionDependentAffineGapLinearPairwiseAlignment& toOut){
	//print out the tables
	os << "Cost" << std::endl;
	printPositionDependentCostTable(os, toOut.seqAs, toOut.seqBs, toOut.costTable);
	os << "Match" << std::endl;
	printPositionDependentCostTable(os, toOut.seqAs, toOut.seqBs, toOut.matchTable);
	os << "Skip A" << std::endl;
	printPositionDependentCostTable(os, toOut.seqAs, toOut.seqBs, toOut.skipATable);
	os << "Skip B" << std::endl;
	printPositionDependentCostTable(os, toOut.seqAs, toOut.seqBs, toOut.skipBTable);
	os << "Match Match" << std::endl;
	printPositionDependentCostTable(os, toOut.seqAs, toOut.seqBs, toOut.matchMatchTable);
	os << "Match Skip A" << std::endl;
	printPositionDependentCostTable(os, toOut.seqAs, toOut.seqBs, toOut.matchSkipATable);
	os << "Match Skip B" << std::endl;
	printPositionDependentCostTable(os, toOut.seqAs, toOut.seqBs, toOut.matchSkipBTable);
	os << "Skip A Match" << std::endl;
	printPositionDependentCostTable(os, toOut.seqAs, toOut.seqBs, toOut.skipAMatchTable);
	os << "Skip A Skip A" << std::endl;
	printPositionDependentCostTable(os, toOut.seqAs, toOut.seqBs, toOut.skipASkipATable);
	os << "Skip A Skip B" << std::endl;
	printPositionDependentCostTable(os, toOut.seqAs, toOut.seqBs, toOut.skipASkipBTable);
	os << "Skip B Match" << std::endl;
	printPositionDependentCostTable(os, toOut.seqAs, toOut.seqBs, toOut.skipBMatchTable);
	os << "Skip B Skip A" << std::endl;
	printPositionDependentCostTable(os, toOut.seqAs, toOut.seqBs, toOut.skipBSkipATable);
	os << "Skip B Skip B" << std::endl;
	printPositionDependentCostTable(os, toOut.seqAs, toOut.seqBs, toOut.skipBSkipBTable);
	return os;
}


