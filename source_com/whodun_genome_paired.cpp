#include "whodun_genome_paired.h"

#include <string.h>

bool samEntryIsPrimary(CRBSAMFileContents* toExamine){
	return (toExamine->entryFlag & (SAM_FLAG_SECONDARY | SAM_FLAG_SUPPLEMENT)) == 0;
}

bool samEntryIsAlign(CRBSAMFileContents* toExamine){
	if(toExamine->entryFlag & SAM_FLAG_SEGUNMAP){
		return false;
	}
	//will only handle up to pairs: triples and more not entertained
	if((toExamine->entryFlag & (SAM_FLAG_FIRST | SAM_FLAG_LAST)) == (SAM_FLAG_FIRST | SAM_FLAG_LAST)){
		return false;
	}
	if(toExamine->entryPos < 0){
		return false;
	}
	uintptr_t cigSize = toExamine->entryCigar.size();
	if(cigSize == 0){
		return false;
	}
	//if M=X in the thing
	char* curLook = &(toExamine->entryCigar[0]);
	if(memchr(curLook, 'M', cigSize) || memchr(curLook, '=', cigSize) || memchr(curLook, 'X', cigSize)){
		return true;
	}
	return false;
}

bool samEntryNeedPair(CRBSAMFileContents* toExamine){
	return toExamine->entryFlag & SAM_FLAG_MULTSEG;
}

PairedEndCache::PairedEndCache(){}
PairedEndCache::~PairedEndCache(){
	//if you fail to drain the outstanding, I'm not cleaning up after you
}

bool PairedEndCache::havePair(CRBSAMFileContents* toExamine){
	nameTmp.clear(); nameTmp.insert(nameTmp.end(), toExamine->entryName.begin(), toExamine->entryName.end());
	return (waitingPair.find(nameTmp) != waitingPair.end());
}

std::pair<uintptr_t,CRBSAMFileContents*> PairedEndCache::getPair(CRBSAMFileContents* forAln){
	nameTmp.clear(); nameTmp.insert(nameTmp.end(), forAln->entryName.begin(), forAln->entryName.end());
	std::map<std::string, uintptr_t>::iterator idIt = waitingPair.find(nameTmp);
	uintptr_t pairID = idIt->second;
	std::map<uintptr_t, CRBSAMFileContents* >::iterator datIt = pairData.find(pairID);
	CRBSAMFileContents* pairDat = datIt->second;
	waitingPair.erase(idIt);
	pairData.erase(datIt);
	return std::pair<uintptr_t,CRBSAMFileContents*>(pairID,pairDat);
}

void PairedEndCache::waitForPair(uintptr_t alnID, CRBSAMFileContents* forAln){
	nameTmp.clear(); nameTmp.insert(nameTmp.end(), forAln->entryName.begin(), forAln->entryName.end());
	waitingPair[nameTmp] = alnID;
	pairData[alnID] = forAln;
}

bool PairedEndCache::haveOutstanding(){
	return waitingPair.size();
}

std::pair<uintptr_t,CRBSAMFileContents*> PairedEndCache::getOutstanding(){
	std::map<std::string, uintptr_t>::iterator idIt = waitingPair.begin();
	uintptr_t pairID = idIt->second;
	std::map<uintptr_t, CRBSAMFileContents* >::iterator datIt = pairData.find(pairID);
	CRBSAMFileContents* pairDat = datIt->second;
	waitingPair.erase(idIt);
	pairData.erase(datIt);
	return std::pair<uintptr_t,CRBSAMFileContents*>(pairID,pairDat);
}
