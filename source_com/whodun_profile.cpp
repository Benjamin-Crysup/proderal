#include "whodun_profile.h"

#include <algorithm>

#include "whodun_stringext.h"

GeneticProfile::GeneticProfile(){}

GeneticProfile::~GeneticProfile(){}


GeneticProfileSet::GeneticProfileSet(){
	numLoci = 0;
	alleleText.push_back(0);
}

GeneticProfileSet::GeneticProfileSet(uintptr_t numberOfLoci){
	numLoci = numberOfLoci;
	alleleText.push_back(0);
}

GeneticProfileSet::~GeneticProfileSet(){}

uintptr_t GeneticProfileSet::size(){
	return profileSInds.size();
}

void GeneticProfileSet::getProfile(uintptr_t profInd, GeneticProfile* storeLoc){
	storeLoc->lociStart.resize(numLoci);
	storeLoc->alleleCounts.resize(numLoci);
	storeLoc->alleleLens.clear();
	storeLoc->alleles.clear();
	uintptr_t curLInd = profInd * numLoci;
	uintptr_t curAInd = profileSInds[profInd];
	for(uintptr_t i = 0; i<numLoci; i++){
		uintptr_t curCnt = lociCounts[curLInd];
		storeLoc->lociStart[i] = storeLoc->alleleLens.size();
		storeLoc->alleleCounts[i] = curCnt;
		for(uintptr_t j = 0; j<curCnt; j++){
			storeLoc->alleleLens.push_back(alleleLens[curAInd]);
			storeLoc->alleles.push_back(&(alleleText[0]) + alleleStarts[curAInd]);
			curAInd++;
		}
		curLInd++;
	}
}

void GeneticProfileSet::addProfile(GeneticProfile* toAdd){
	profileSInds.push_back(lociCounts.size());
	lociCounts.insert(lociCounts.end(), toAdd->alleleCounts.begin(), toAdd->alleleCounts.end());
	alleleLens.insert(alleleLens.end(), toAdd->alleleLens.begin(), toAdd->alleleLens.end());
	uintptr_t totNAll = toAdd->alleles.size();
	for(uintptr_t i = 0; i<totNAll; i++){
		uintptr_t curLen = toAdd->alleleLens[i];
		const char* curAl = toAdd->alleles[i];
		alleleStarts.push_back(alleleText.size());
		alleleText.insert(alleleText.end(), curAl, curAl + curLen);
	}
}

void GeneticProfileSet::dumpSet(OutStream* dumpTo){
	std::vector<char> dumpBuff;
	//dump the number of loci
	dumpBuff.resize(8);
	nat2be64(numLoci, &(dumpBuff[0]));
	dumpTo->writeBytes(&(dumpBuff[0]), 8);
	//dump the number of people
	uintptr_t numPeople = profileSInds.size();
	nat2be64(numPeople, &(dumpBuff[0]));
	dumpTo->writeBytes(&(dumpBuff[0]), 8);
	if(numLoci == 0 || numPeople == 0){
		return;
	}
	//dump the integer vectors
	helpPackVector(&profileSInds, &dumpBuff);
		dumpTo->writeBytes(&(dumpBuff[0]), dumpBuff.size());
	helpPackVector(&lociCounts, &dumpBuff);
		dumpTo->writeBytes(&(dumpBuff[0]), dumpBuff.size());
	helpPackVector(&alleleLens, &dumpBuff);
		dumpTo->writeBytes(&(dumpBuff[0]), dumpBuff.size());
	helpPackVector(&alleleStarts, &dumpBuff);
		dumpTo->writeBytes(&(dumpBuff[0]), dumpBuff.size());
	//dump the text
	dumpTo->writeBytes(&(alleleText[0]), alleleText.size());
}

#define FORCE_KNOWN_READ(numBts) if(dumpTo->readBytes(&(dumpBuff[0]), numBts) != numBts){ throw std::runtime_error("File truncated."); }
#define CALC_TOTAL_LEN(inVar, ofVec, vecLen) inVar = 0; for(uintptr_t ijk = 0; ijk < vecLen; ijk++){ inVar += ofVec[ijk]; }

void GeneticProfileSet::loadSet(InStream* dumpTo){
	std::vector<char> dumpBuff;
	//read the number of loci
	dumpBuff.resize(8);
	FORCE_KNOWN_READ(8)
	numLoci = be2nat64(&(dumpBuff[0]));
	//read the number of people
	FORCE_KNOWN_READ(8)
	uintptr_t numPeople = be2nat64(&(dumpBuff[0]));
	//read the stuff
	dumpBuff.resize(8*numLoci*numPeople);
		FORCE_KNOWN_READ(dumpBuff.size())
		helpUnpackVector(&dumpBuff, &lociCounts);
		uintptr_t totNumAllele;
		CALC_TOTAL_LEN(totNumAllele, lociCounts, lociCounts.size())
	dumpBuff.resize(8*totNumAllele);
		FORCE_KNOWN_READ(dumpBuff.size())
		helpUnpackVector(&dumpBuff, &alleleLens);
		uintptr_t totTextSize;
		CALC_TOTAL_LEN(totTextSize, alleleLens, alleleLens.size())
		totTextSize++;
	dumpBuff.resize(8*totNumAllele);
		FORCE_KNOWN_READ(dumpBuff.size())
		helpUnpackVector(&dumpBuff, &alleleStarts);
	alleleText.resize(totTextSize);
		if(dumpTo->readBytes(&(alleleText[0]), totTextSize) != totTextSize){ throw std::runtime_error("File truncated."); }
}

void GeneticProfileSet::helpPackVector(std::vector<uintptr_t>* toPack, std::vector<char>* packTo){
	uintptr_t numP = toPack->size();
	packTo->resize(8*numP);
	if(numP == 0){ return; }
	uintptr_t* packInt = &((*toPack)[0]);
	char* packBuff = &((*packTo)[0]);
	for(uintptr_t i = 0; i<numP; i++){
		nat2be64(packInt[i], packBuff);
		packBuff += 8;
	}
}

void GeneticProfileSet::helpUnpackVector(std::vector<char>* toPack, std::vector<uintptr_t>* packTo){
	uintptr_t numP = toPack->size() / 8;
	packTo->resize(numP);
	if(numP == 0){ return; }
	char* packBuff = &((*toPack)[0]);
	uintptr_t* packInt = &((*packTo)[0]);
	for(uintptr_t i = 0; i<numP; i++){
		packInt[i] = be2nat64(packBuff);
		packBuff += 8;
	}
}

Pedigree::Pedigree(){}

Pedigree::~Pedigree(){}

void Pedigree::prepareChildren(){
	allChildren.clear();
	allChildren.reserve(allParents.size());
	for(std::set< std::pair<uintptr_t,uintptr_t> >::iterator parIt = allParents.begin(); parIt != allParents.end(); parIt++){
		std::pair<uintptr_t,uintptr_t> tmpStore = *parIt;
		allChildren.push_back(std::pair<uintptr_t,uintptr_t>(tmpStore.second, tmpStore.first));
	}
	std::sort(allChildren.begin(), allChildren.end());
}

void Pedigree::getAllParents(uintptr_t ofPerson, std::vector<uintptr_t>* addTo){
	std::set< std::pair<uintptr_t,uintptr_t> >::iterator curIt = allParents.lower_bound(std::pair<uintptr_t,uintptr_t>(ofPerson,0));
	while(curIt != allParents.end()){
		if(curIt->first != ofPerson){ break; }
		addTo->push_back(curIt->second);
		curIt++;
	}
}

void Pedigree::getAllChildren(uintptr_t ofPerson, std::vector<uintptr_t>* addTo){
	std::vector< std::pair<uintptr_t,uintptr_t> >::iterator curIt = std::lower_bound(allChildren.begin(), allChildren.end(), std::pair<uintptr_t,uintptr_t>(ofPerson,0));
	while(curIt != allChildren.end()){
		if(curIt->first != ofPerson){ break; }
		addTo->push_back(curIt->second);
		curIt++;
	}
}

#define DUMP_OUT_INT(toDump) nat2be64(toDump, dumpBuff); dumpTo->writeBytes(dumpBuff, 8);

void Pedigree::dumpPedigree(OutStream* dumpTo){
	char dumpBuff[8];
	//dump out the people
	DUMP_OUT_INT(allPeople.size())
	for(std::set<uintptr_t>::iterator curIt = allPeople.begin(); curIt != allPeople.end(); curIt++){
		DUMP_OUT_INT(*curIt)
	}
	//dump out the parentage
	DUMP_OUT_INT(allParents.size())
	for(std::set< std::pair<uintptr_t,uintptr_t> >::iterator curIt = allParents.begin(); curIt != allParents.end(); curIt++){
		DUMP_OUT_INT(curIt->first)
		DUMP_OUT_INT(curIt->second)
	}
}

#define PULL_IN_INT(toDump) if(dumpTo->readBytes(dumpBuff,8)!=8){ throw std::runtime_error("File truncated."); } toDump = be2nat64(dumpBuff);

void Pedigree::loadPedigree(InStream* dumpTo){
	char dumpBuff[8];
	allPeople.clear();
	allParents.clear();
	//load in the people
	uintptr_t numPeople; PULL_IN_INT(numPeople)
	for(uintptr_t i = 0; i<numPeople; i++){
		uintptr_t curPerson; PULL_IN_INT(curPerson)
		allPeople.insert(curPerson);
	}
	//load in the parentage
	uintptr_t numParent; PULL_IN_INT(numParent)
	for(uintptr_t i = 0; i<numParent; i++){
		uintptr_t curChild; PULL_IN_INT(curChild)
		uintptr_t curParen; PULL_IN_INT(curParen)
		allParents.insert( std::pair<uintptr_t,uintptr_t>(curChild,curParen) );
	}
	prepareChildren();
}
