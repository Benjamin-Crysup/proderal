
#include <math.h>
#include <vector>
#include <algorithm>

#include "gpmatlan.h"
#include "whodun_cigfq.h"

#include "proderal_seqs.h"
#include "proderal_entry.h"
#include "proderal_setup.h"

/**The relevant entries in the reference information.*/
typedef struct{
	/**The entry in the reference sequence map.*/
	std::map< std::string, std::string >::iterator allRefIt;
	/**Whether probRegIt is valid.*/
	bool haveProbRegIt;
	/**The entry in the problem region map.*/
	std::map< std::string , std::vector< std::pair<intptr_t,intptr_t> > >::iterator probRegIt;
	/**The entry in the alignment cost map.*/
	std::map<std::string,PositionDependentAlignCostKDNode*>::iterator costIt;
	/**The quality changes, if any.*/
	std::map<std::string,PositionDependentQualityChangeSet*>::iterator qualmIt;
} ReferenceEntries;

/**
 * Finds the relevant reference entries.
 * @param refNam The name of the refereence.
 * @param curCon The reference information.
 * @param toFill The place to put the entry data.
 * @return Whether there was a problem.
 */
int gatherReferenceInforamtion(std::string* refNam, ProderalPreload* curCon, ReferenceEntries* toFill){
	toFill->allRefIt = curCon->refd->allRefs.find(*refNam);
	if(toFill->allRefIt == curCon->refd->allRefs.end()){
		std::cerr << "Missing reference sequence " << *refNam << std::endl;
		return 1;
	}
	toFill->probRegIt = curCon->refd->probRegMap.find(*refNam);
	toFill->haveProbRegIt = (toFill->probRegIt != curCon->refd->probRegMap.end());
	bool probNeeded = !(curCon->pargs->hitAll);
	if(probNeeded){
		if(!(toFill->haveProbRegIt)){
			std::cerr << "Missing problem region information for reference " << *refNam << std::endl;
			return 1;
		}
	}
	toFill->costIt = curCon->refd->allRegCosts.find(*refNam);
	if(toFill->costIt == curCon->refd->allRegCosts.end()){
		std::cerr << "Missing cost information for reference " << *refNam << std::endl;
		return 1;
	}
	if(curCon->pargs->qualmFile){
		toFill->qualmIt = curCon->refd->allQualMangs.find(*refNam);
		if(toFill->qualmIt == curCon->refd->allQualMangs.end()){
			std::cerr << "Quality mangle information missing " << *refNam << std::endl;
			return 1;
		}
	}
	return 0;
}

/**
 * This will take a region and expand it according to the options.
 * @param expLowIndAddr The location of the low index. If -1, stop.
 * @param expHigIndAddr The location of the high index.
 * @param curCon The reference information.
 * @param refIts The reference data locations.
 * @param cigBndInfo The cigar bound info.
 * @param fullRefSeq The full reference sequence.
 * @param refNam The reference name.
 * @return Whether there was a problem.
 */
int expandRealignSearchRegion(intptr_t* expLowIndAddr, intptr_t* expHigIndAddr, ProderalPreload* curCon, ReferenceEntries* refIts, CigarBounds* cigBndInfo, std::string* fullRefSeq, std::string* refNam){
	intptr_t expLowInd = *expLowIndAddr;
	intptr_t expHigInd = *expHigIndAddr;
	if(curCon->pargs->recSoft > 0){
		expLowInd -= (curCon->pargs->recSoft * cigBndInfo->preSkip);
		expHigInd += (curCon->pargs->recSoft * cigBndInfo->postSkip);
	}
	bool needRealn = curCon->pargs->hitAll;
	if(refIts->haveProbRegIt){
		std::pair<intptr_t,intptr_t> searchLow(expLowInd,expLowInd);
		std::pair<intptr_t,intptr_t> searchHig(expHigInd,expHigInd);
		std::vector< std::pair<intptr_t,intptr_t> >::iterator proRanItA = std::lower_bound(refIts->probRegIt->second.begin(), refIts->probRegIt->second.end(), searchLow);
		if(proRanItA != refIts->probRegIt->second.begin()){ proRanItA--; }
		std::vector< std::pair<intptr_t,intptr_t> >::iterator proRanItB = std::upper_bound(refIts->probRegIt->second.begin(), refIts->probRegIt->second.end(), searchHig);
		while(proRanItA != proRanItB){
			intptr_t lowCheck = proRanItA->first;
			intptr_t higCheck = proRanItA->second;
			if((lowCheck < expHigInd) && (expLowInd < higCheck)){
				needRealn = true;
				expLowInd = (lowCheck < expLowInd) ? lowCheck : expLowInd;
				expHigInd = (higCheck > expHigInd) ? higCheck : expHigInd;
			}
			proRanItA++;
		}
	}
	if(!needRealn){*expLowIndAddr = -1; *expHigIndAddr = -1; return 0;}
	expHigInd = (expHigInd < 0) ? 0 : expHigInd;
	expHigInd += curCon->pargs->overRun;
	expLowInd -= curCon->pargs->overRun;
	expLowInd = (expLowInd < 0) ? 0 : expLowInd;
	if(expLowInd >= (intptr_t)fullRefSeq->size()){
		std::cerr << "Reference sequence " << *refNam << " too short for SAM alignments." << std::endl;
		return 1;
	}
	if(expHigInd > (intptr_t)fullRefSeq->size()){
		expHigInd = fullRefSeq->size() - 1;
	}
	*expLowIndAddr = expLowInd;
	*expHigIndAddr = expHigInd;
	return 0;
}

/**
 * Takes a sam entry that should not be realigned, and handles it.
 * @param curCon The reference and cache information.
 * @param samFil The sam entry.
 * @return Whether there was a problem.
 */
int passThroughSAMEntry(ProderalPreload* curCon, LiveSAMFileReader* samFil){
	if(samFil->lastAlignFlag & 0x01){
		WaitingForPairSet* samWPS = new WaitingForPairSet(samFil, &(std::cerr));
		std::map< std::string, WaitingForPairSet* >::iterator anyWait = curCon->waitingPairs.find(samWPS->myName);
		if(anyWait == curCon->waitingPairs.end()){
			curCon->waitingPairs[samWPS->myName] = samWPS;
		}
		else{
			WaitingForPairSet* othWPS = anyWait->second;
			samWPS->writeEntryWithPair(&(std::cout), othWPS);
			othWPS->writeEntryWithPair(&(std::cout), samWPS);
			curCon->waitingPairs.erase(anyWait);
			delete(samWPS);
			delete(othWPS);
		}
	}
	else{
		dumpUnpairedAsIs(samFil, &(std::cout));
	}
	return 0;
}

int getRelevantRealignSequence(char** seqToKill, char** qualErrProbs, ProderalPreload* curCon, LiveSAMFileReader* samFil, CigarBounds* cigInfo){
	*seqToKill = 0;
	*qualErrProbs = 0;
	//bool isRC = samFil->lastAlignFlag & 0x0010;
	int seqLen = strlen(samFil->lastAlignSeq);
	//check for reverse complement and soft clipping
	//only check for soft clipping: reverse complement flag is JUST a flag, bases are in line with the reference
	const char* qualStrStart = 0;
	int qualStrLen = -1;
	if(curCon->pargs->recSoft > 0){
		*seqToKill = strdup(samFil->lastAlignSeq);
		qualStrStart = samFil->lastAlignQual;
		qualStrLen = seqLen;
	}
	else{
		int subLen = seqLen - (cigInfo->preSkip + cigInfo->postSkip);
		char* startFrom = samFil->lastAlignSeq + cigInfo->preSkip;
		*seqToKill = (char*)malloc(subLen + 1);
			memcpy(*seqToKill, startFrom, subLen);
			(*seqToKill)[subLen] = 0;
		qualStrStart = samFil->lastAlignQual + cigInfo->preSkip;
		qualStrLen = subLen;
	}
	//get the quality probabilities
		*qualErrProbs = (char*)malloc(qualStrLen);
		memcpy(*qualErrProbs, qualStrStart, qualStrLen);
	return 0;
}

void printOutCostTree(PositionDependentAlignCostKDNode* toPrint, int indent){
	for(int q = 0; q<indent; q++){
		std::cout << " ";
	}
	std::cout << toPrint->splitOn << " " << toPrint->splitAt << std::endl;
	for(int i = 0; i<toPrint->numSpan; i++){
		for(int q = 0; q<=indent; q++){
			std::cout << " ";
		}
		PositionDependentBounds* curBnd = toPrint->allSpans[i];
		std::cout << "[" << curBnd->startA << "," << curBnd->endA << ")x[" << curBnd->startB << "," << curBnd->endB << ") " << curBnd->priority << std::endl;
	}
	if(toPrint->subNodeLesser){
		printOutCostTree((PositionDependentAlignCostKDNode*)(toPrint->subNodeLesser), indent + 2);
	}
	if(toPrint->subNodeGreatE){
		printOutCostTree((PositionDependentAlignCostKDNode*)(toPrint->subNodeGreatE), indent + 2);
	}
}

/**
 * Realigns a sequence to the reference.
 * @param curCon The reference and cache information.
 * @param samFil The sam entry.
 * @param cigInfo Info on the original alignment.
 * @param refInfoIts Location of the relevant refernce information.
 * @param refLowInd The low index in the reference.
 * @param refHigInd The high index in the reference.
 * @return The resulting alignments.
 */
WaitingForPairSet* realignSequence(ProderalPreload* curCon, LiveSAMFileReader* samFil, CigarBounds* cigInfo, ReferenceEntries* refInfoIts, intptr_t refLowInd, intptr_t refHigInd){
	WaitingForPairSet* toRet = 0;
	AGPDAlignProblem alnProb;
	bool haveAllocTabs = false;
	alnProb.numEnds = curCon->pargs->numEnds;
	//things that might need to be free
		PositionDependentAlignCostKDNode* subCF = 0;
		char* qualErrProbs = 0;
		char* seqToKill = 0;
		int* foundScores = 0;
		AlignmentFiller alnArg;
			alnArg.aInds = 0;
			alnArg.bInds = 0;
	//temporary storage
		std::vector< std::vector< std::string > > allAligns;
		std::vector< std::vector<long> > allAlignStarts;
		std::vector< std::vector<long> > allAlignEnds;
		std::vector< std::vector<double> > allAlignProbs;
		char itoabuff[4*sizeof(intmax_t)+4];
	{
		//reference sequence goes as is
			std::string* fullRefSeq = &(refInfoIts->allRefIt->second);
			alnProb.lenA = (refHigInd - refLowInd) + 1;
			alnProb.seqA = fullRefSeq->c_str() + refLowInd;
		//reverse complement if flag & 16
			if(getRelevantRealignSequence(&seqToKill, &qualErrProbs, curCon, samFil, cigInfo)){goto skipForError;}
			alnProb.lenB = strlen(seqToKill);
			alnProb.seqB = seqToKill;
		//rebase the cost function around the indices in question
			subCF = rebasePositionDependentSpecification(refInfoIts->costIt->second, refLowInd, refHigInd, -1, -1);
		//mangle for quality
			if(curCon->pargs->qualmFile){
				PositionDependentAlignCostKDNode* tmpCF = qualifyPositionDependentSpecification(subCF, refInfoIts->qualmIt->second, alnProb.lenB, qualErrProbs, alnProb.lenA);
				if(!tmpCF){
					goto skipForError;
				}
				freePositionDependentCostInformation(subCF);
				subCF = tmpCF;
			}
			alnProb.alnCosts = subCF;
			//printOutCostTree(subCF, 0);
		//realign
			allocateAGPDCostTables(&alnProb); haveAllocTabs = true;
			fillInAGPDCostTables(&alnProb);
		//figure out the ranks
			int numRank = curCon->pargs->uptoRank + 1;
			foundScores = (int*)malloc(numRank * sizeof(int));
			numRank = findAGPDAlignmentScores(&alnProb, numRank, foundScores, curCon->pargs->hotfuzz);
			if(numRank == 0){ goto skipForError; }
		//create the place for the result
			allAligns.resize(numRank);
			allAlignStarts.resize(numRank);
			allAlignEnds.resize(numRank);
		//start iterating
			int maxRepCount = curCon->pargs->uptoCount;
			void* useIter = initializeAGPDFuzzedAlignmentIteration(&alnProb, foundScores[numRank-1], curCon->pargs->hotfuzz);
			alnArg.aInds = (int*)malloc((alnProb.lenA + alnProb.lenB + 2)*sizeof(int));
			alnArg.bInds = (int*)malloc((alnProb.lenA + alnProb.lenB + 2)*sizeof(int));
			int isAln = getNextAGPDAlignment(useIter, &alnArg);
			while(isAln){
				//figure out which rank you are looking at
					int* crankPtr = std::lower_bound(foundScores, foundScores+numRank, alnArg.alnScore, std::greater<int>());
					int crankI = crankPtr - foundScores;
				//add an entry if necessary
					std::vector<std::string>* curAligns = &(allAligns[crankI]);
					if((maxRepCount == 0) || (curAligns->size() < (unsigned)(maxRepCount))){
						std::vector<long>* curAlignSt = &(allAlignStarts[crankI]);
						std::vector<long>* curAlignE = &(allAlignEnds[crankI]);
						long startInd;
						std::string curCig;
						alignmentToCIGAR(alnArg.alnLen, alnArg.aInds, alnArg.bInds, alnProb.lenB, &curCig, &startInd);
						startInd += refLowInd;
						long endInd = alnArg.aInds[alnArg.alnLen-1] + refLowInd;
						//TODO guard against repeats
						curAligns->push_back(curCig);
						curAlignSt->push_back(startInd);
						curAlignE->push_back(endInd);
					}
				//decide whether to continue
					if(crankI != 0){
						isAln = 1;
					}
					else if(maxRepCount == 0){
						isAln = 1;
					}
					else if((int)(curAligns->size()) < maxRepCount){
						isAln = 1;
					}
					else{
						isAln = 0;
					}
				if(isAln){
					isAln = getNextAGPDAlignment(useIter, &alnArg);
				}
			}
			terminateAGPDAlignmentIteration(useIter);
		//package things up for the caller
		int standFlags = samFil->lastAlignFlag & ~0x00EA;
		std::string refNam(samFil->lastAlignRName);
		std::string defDump;
			defDump.append(samFil->lastAlignRNext);
			defDump.push_back('\t');
			sprintf(itoabuff, "%jd", (intmax_t)(samFil->lastAlignPNext+1));
			defDump.append(itoabuff);
			defDump.push_back('\t');
			sprintf(itoabuff, "%jd", (intmax_t)(samFil->lastAlignTLen));
			defDump.append(itoabuff);
		toRet = new WaitingForPairSet(samFil->lastAlignQName, samFil->lastAlignSeq, samFil->lastAlignQual);
		for(int i = 0; i<numRank; i++){
			int rankScore = foundScores[i];
			std::string newExtra;
				newExtra.append("AS:i:");
				sprintf(itoabuff, "%jd", (intmax_t)(rankScore));
				newExtra.append(itoabuff);
			std::vector<std::string>* curAligns = &(allAligns[i]);
			std::vector<long>* curAlignSt = &(allAlignStarts[i]);
			std::vector<long>* curAlignE = &(allAlignEnds[i]);
			int numNeedAdd;
			if(maxRepCount == 0){
				numNeedAdd = curAligns->size();
			}
			else{
				numNeedAdd = maxRepCount - toRet->knownFlags.size();
				if(numNeedAdd == 0){ break; }
				if((unsigned)numNeedAdd > curAligns->size()){
					numNeedAdd = curAligns->size();
				}
			}
			for(int j = 0; j<numNeedAdd; j++){
				int workFlag = standFlags | ((i || j) ? 0x0100 : 0);
				toRet->knownFlags.push_back(workFlag);
				toRet->mapRefs.push_back(refNam);
				std::string pmcAdd;
					sprintf(itoabuff, "%jd", (intmax_t)((*curAlignSt)[j] + 1));
					pmcAdd.append(itoabuff);
					pmcAdd.push_back('\t');
					sprintf(itoabuff, "%jd", (intmax_t)(samFil->lastAlignMapq));
					pmcAdd.append(itoabuff);
					pmcAdd.push_back('\t');
					pmcAdd.append((*curAligns)[j]);
					toRet->mapPosMapCigs.push_back(pmcAdd);
				toRet->defDumps.push_back(defDump);
				//skip the old score, add a new one
				toRet->extras.push_back(newExtra);
				toRet->lowInds.push_back((*curAlignSt)[j]);
				toRet->higInds.push_back((*curAlignE)[j]);
			}
		}
	}
skipForError:
	if(haveAllocTabs){deallocateAGPDCostTables(&alnProb);}
	if(subCF){ freePositionDependentCostInformation(subCF); }
	if(alnArg.aInds){ free(alnArg.aInds); }
	if(alnArg.bInds){ free(alnArg.bInds); }
	if(qualErrProbs){ free(qualErrProbs); }
	if(foundScores){ free(foundScores); }
	if(seqToKill){ free(seqToKill); }
	return toRet;
}

/**
 * This will do anlaysis on a SAM entry.
 * @param curCon The reference and cache information.
 * @param samFil The sam entry.
 * @return Whether there was a problem.
 */
int runSAMEntry(ProderalPreload* curCon, LiveSAMFileReader* samFil){
	//quick testable cases (pass through header, skip supplemental and secondary)
		if(!(samFil->lastAlignQName)){
			return 0;
		}
		if(samFil->lastAlignFlag & 0x0904){
			return 0;
		}
	//get the bounds of the entry (expand the cigar)
		int curSLen = strlen(samFil->lastAlignSeq);
		CigarBounds cigInfo;
		if(findCigarBounds(curSLen, samFil->lastAlignCIGAR, samFil->lastAlignPos, &cigInfo, &(std::cerr))){return 1;}
		if(cigInfo.lowInd < 0){ passThroughSAMEntry(curCon, samFil); return 0; }
	//get the reference stuff
		std::string refNam(samFil->lastAlignRName);
		ReferenceEntries refInfoIts;
		if(gatherReferenceInforamtion(&refNam, curCon, &refInfoIts)){passThroughSAMEntry(curCon, samFil); return 0;}
	//reclaim soft clipped bases, scan against problematic regions, add overlap
		std::string* fullRefSeq = &(refInfoIts.allRefIt->second);
		intptr_t expLowInd = cigInfo.lowInd;
		intptr_t expHigInd = cigInfo.higInd;
		if(expandRealignSearchRegion(&expLowInd, &expHigInd, curCon, &refInfoIts, &cigInfo, fullRefSeq, &refNam)){return 1;}
		if(expLowInd < 0){passThroughSAMEntry(curCon, samFil); return 0;}
	//realign
		WaitingForPairSet* samWPS = realignSequence(curCon, samFil, &cigInfo, &refInfoIts, expLowInd, expHigInd);
		if(samWPS == 0){ return 1; }
	//dump
		if(samFil->lastAlignFlag & 0x01){
			std::map< std::string, WaitingForPairSet* >::iterator anyWait = curCon->waitingPairs.find(samWPS->myName);
			if(anyWait == curCon->waitingPairs.end()){
				curCon->waitingPairs[samWPS->myName] = samWPS;
			}
			else{
				WaitingForPairSet* othWPS = anyWait->second;
				samWPS->writeEntryWithPair(&(std::cout), othWPS);
				othWPS->writeEntryWithPair(&(std::cout), samWPS);
				curCon->waitingPairs.erase(anyWait);
				delete(samWPS);
				delete(othWPS);
			}
		}
		else{
			samWPS->writeEntryWithoutPair(&(std::cout));
			delete(samWPS);
		}
	return 0;
}

int main(int argc, char** argv){
	//basic setup
		ProderalPreload curCon(argc, argv);
		if(!curCon.isValid){
			return 1;
		}
	//run down the entries
		LiveSAMFileReader* curSFE = curCon.allFs->getNextEntry();
		while(curSFE){
			if(runSAMEntry(&curCon, curSFE)){
				return 1;
			}
			curSFE = curCon.allFs->getNextEntry();
		}
	//dump out any waiting pairs
		for(std::map< std::string, WaitingForPairSet* >::iterator wpit = curCon.waitingPairs.begin(); wpit != curCon.waitingPairs.end(); wpit++){
			WaitingForPairSet* curWait = wpit->second;
			curWait->writeEntryWithoutPair(&(std::cout));
		}
	return 0;
}
