
#include "proderal_entry.h"

#include "proderal_seqs.h"

void fillEntryFromSAM(LiveSAMFileReader* toNote, WaitPairEntry* toFill){
	toFill->lastAlignFlag = toNote->lastAlignFlag;
	toFill->lastAlignRName = toNote->lastAlignRName;
	toFill->lastAlignPos = toNote->lastAlignPos;
	toFill->lastAlignMapq = toNote->lastAlignMapq;
	toFill->lastAlignCIGAR = toNote->lastAlignCIGAR;
	toFill->lastAlignRNext = toNote->lastAlignRNext;
	toFill->lastAlignPNext = toNote->lastAlignPNext;
	toFill->lastAlignTLen = toNote->lastAlignTLen;
	toFill->lastAlignExtra = toNote->lastAlignExtra;
}

WaitingForPairSet::WaitingForPairSet(LiveSAMFileReader* toNote, std::ostream* errDump){
	myName.append(toNote->lastAlignQName);
	sequen.append(toNote->lastAlignSeq);
	qualstr.append(toNote->lastAlignQual);
	WaitPairEntry tmpEnt;
		fillEntryFromSAM(toNote, &tmpEnt);
	addEntry(&tmpEnt, errDump);
}
	
WaitingForPairSet::WaitingForPairSet(const char* queryName, const char* sequence, const char* qualString){
	myName.append(queryName);
	sequen.append(sequence);
	qualstr.append(qualString);
}

void WaitingForPairSet::addEntry(WaitPairEntry* infoGet, std::ostream* errDump){
	char itoabuff[4*sizeof(intmax_t)+4];
	knownFlags.push_back(infoGet->lastAlignFlag);
	std::string tnref(infoGet->lastAlignRName);
		mapRefs.push_back(tnref);
	std::string mpmtmp;
		sprintf(itoabuff, "%jd", (intmax_t)(infoGet->lastAlignPos+1));
		mpmtmp.append(itoabuff);
		mpmtmp.push_back('\t');
		sprintf(itoabuff, "%jd", (intmax_t)(infoGet->lastAlignMapq & 0x00FF));
		mpmtmp.append(itoabuff);
		mpmtmp.push_back('\t');
		mpmtmp.append(infoGet->lastAlignCIGAR);
		mapPosMapCigs.push_back(mpmtmp);
	std::string defdmptmp;
		defdmptmp.append(infoGet->lastAlignRNext);
		defdmptmp.push_back('\t');
		sprintf(itoabuff, "%jd", (intmax_t)(infoGet->lastAlignPNext+1));
		defdmptmp.append(itoabuff);
		defdmptmp.push_back('\t');
		sprintf(itoabuff, "%jd", (intmax_t)(infoGet->lastAlignTLen));
		defdmptmp.append(itoabuff);
		defDumps.push_back(defdmptmp);
	std::string extratmp(infoGet->lastAlignExtra);
		extras.push_back(extratmp);
	CigarBounds tmpCB;
	if(findCigarBounds(sequen.size(), infoGet->lastAlignCIGAR, infoGet->lastAlignPos, &tmpCB, errDump)){
		lowInds.push_back(-1);
		higInds.push_back(-1);
	}
	else{
		lowInds.push_back(tmpCB.lowInd);
		higInds.push_back(tmpCB.higInd);
	}
}

void WaitingForPairSet::writeEntryWithoutPair(std::ostream* toDump){
	for(unsigned int i = 0; i<knownFlags.size(); i++){
		int winFlag = knownFlags[i] & ~0x02;
			if(winFlag & 0x01){ winFlag = winFlag | 0x08; }
			if(i){ winFlag = winFlag | 0x0100; }
		*toDump << myName << '\t';
		*toDump << winFlag << '\t';
		*toDump << mapRefs[i] << '\t';
		*toDump << mapPosMapCigs[i] << '\t';
		*toDump << "*" << "\t";
		*toDump << "0" << "\t";
		*toDump << "0" << "\t";
		*toDump << sequen << "\t";
		*toDump << qualstr << "\t";
		*toDump << extras[i] << std::endl;
	}
}

void WaitingForPairSet::writeEntryWithPair(std::ostream* toDump, WaitingForPairSet* pairInfo){
	if(pairInfo->knownFlags.size() == 0){
		writeEntryWithoutPair(toDump);
	}
	intptr_t othLow = pairInfo->lowInds[0];
	intptr_t othHig = pairInfo->higInds[0];
	std::string* othRef = &(pairInfo->mapRefs[0]);
	int othFlags = pairInfo->knownFlags[0];
	for(unsigned int i = 0; i<knownFlags.size(); i++){
		intptr_t curLow = lowInds[0];
		intptr_t curHig = higInds[0];
		std::string* curRef = &(mapRefs[0]);
		bool sameRef = (*curRef == *othRef);
		int winFlag = knownFlags[i] & ~0x00EA;
			if(((othFlags & 0x04)==0) && ((winFlag & 0x04)==0) && sameRef && (knownFlags[i]&0x02)){winFlag = winFlag | 0x02;}
			if(othFlags & 0x04){ winFlag = winFlag | 0x08; }
			if(othFlags & 0x0010){ winFlag = winFlag | 0x0020; }
			if(sameRef && (curLow < othLow)){ winFlag = winFlag | 0x0040; }
			if(sameRef && (curLow >= othLow)){ winFlag = winFlag | 0x0080; }
			if(i){ winFlag = winFlag | 0x0100; }
		*toDump << myName << '\t';
		*toDump << winFlag << '\t';
		*toDump << mapRefs[i] << '\t';
		*toDump << mapPosMapCigs[i] << '\t';
		*toDump << *othRef << '\t';
		*toDump << (othLow + 1) << '\t';
		if(sameRef){
			if(curLow >= othLow){ *toDump << '-'; }
			intptr_t biggest = (othHig > curHig) ? othHig : curHig;
			intptr_t smallest = (othLow < curLow) ? othLow : curLow;
			*toDump << (biggest - smallest) << '\t';
		}
		else{
			*toDump << "0" << "\t";
		}
		*toDump << sequen << "\t";
		*toDump << qualstr << "\t";
		*toDump << extras[i] << std::endl;
	}
}

void dumpUnpairedAsIs(LiveSAMFileReader* unpEnt, std::ostream* toDump){
	*(toDump) << unpEnt->lastAlignQName << "\t";
	*(toDump) << unpEnt->lastAlignFlag << "\t";
	*(toDump) << unpEnt->lastAlignRName << "\t";
	*(toDump) << (unpEnt->lastAlignPos + 1) << "\t";
	*(toDump) << (unsigned)(unpEnt->lastAlignMapq & 0x00FF) << "\t";
	*(toDump) << unpEnt->lastAlignCIGAR << "\t";
	*(toDump) << "*" << "\t";
	*(toDump) << "0" << "\t";
	*(toDump) << "0" << "\t";
	*(toDump) << unpEnt->lastAlignSeq << "\t";
	*(toDump) << unpEnt->lastAlignQual << "\t";
	*(toDump) << unpEnt->lastAlignExtra << std::endl;
}
