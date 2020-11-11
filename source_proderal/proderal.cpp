
#include <algorithm>

#include "whodun_args.h"
#include "whodun_oshook.h"
#include "whodun_datread.h"
#include "whodun_compress.h"
#include "whodun_parse_seq.h"
#include "whodun_stringext.h"
#include "whodun_parse_table.h"
#include "whodun_genome_paired.h"
#include "whodun_align_affinepd.h"
#include "whodun_parse_table_genome.h"

#define PRODERAL_HEADERFLAG "@PG\tID:ProDerAl\tVN:0"

/**Parse arguments for proderal.*/
class ProderalArgumentParser : public ArgumentParser{
public:
	ProderalArgumentParser(){
		defOutFN[0] = '-'; defOutFN[1] = 0;
		myMainDoc = "Usage: proderal [OPTION] [FILE]\n"
			"Realigns entries in a SAM file that map to problematic regions.\n"
			"Multiple SAM files can be included: if none supplied, stdin is used.\n"
			"The OPTIONS are:\n";
		myVersionDoc = "ProDerAl 1.0";
		myCopyrightDoc = "Copyright (C) 2019 UNT HSC Center for Human Identification";
		ArgumentParserStrMeta outMeta("Output File");
			outMeta.isFile = true;
			outMeta.fileWrite = true;
			outMeta.fileExts.insert(".sam");
			addStringOption("--out", &outFileN, 0, "    Specify the output file.\n    --out File.sam\n", &outMeta);
		ArgumentParserStrMeta refMeta("Reference File");
			refMeta.isFile = true;
			refMeta.fileExts.insert(".fa");
			refMeta.fileExts.insert(".fa.gz");
			refMeta.fileExts.insert(".fa.gzip");
			addStringOption("--ref", &refFile, 0, "    Specify the reference sequences in a fasta file.\n    --ref File.fa\n", &refMeta);
		ArgumentParserStrMeta probMeta("Realign Region File");
			probMeta.isFile = true;
			probMeta.fileExts.insert(".bed");
			addStringOption("--prob", &probFile, 0, "    Specify the problematic regions in a bed file.\n    --prob File.bed\n", &probMeta);
		ArgumentParserBoolMeta promMeta("Realign Everything");
			addBooleanFlag("--promiscuous", &hitAll, 1, "    Specify that the entire reference is problematic.\n", &promMeta);
		ArgumentParserStrMeta costMeta("Alignment Parameter File");
			costMeta.isFile = true;
			costMeta.fileExts.insert(".pdc");
			addStringOption("--cost", &costFile, 0, "    Specify the position dependent cost function.\n    --cost File.pdc\n", &costMeta);
		ArgumentParserIntMeta overMeta("Realign Region Expansion");
			addIntegerOption("--overrun", &overRun, 0, "    Specify the number of extra bases to get when realigning.\n    --overrun 20\n", &overMeta);
		ArgumentParserIntMeta maxEMeta("Maximum Region Expansion");
			addIntegerOption("--maxexp", &maxExpand, 0, "    Specify the maximum number of extra bases to get to realign.\n    Use if the problematic regions are large.\n    Use negative one for no limit.\n    --maxexp -1\n", &maxEMeta);
		ArgumentParserEnumMeta atLocMeta("Local");
			atLocMeta.enumGroup = "Alignment Type";
			addEnumerationFlag("--atlocal", &numEnds, 0, "    Produce local alignments\n", &atLocMeta);
		ArgumentParserEnumMeta atSLocMeta("Semi-local");
			atSLocMeta.enumGroup = "Alignment Type";
			addEnumerationFlag("--atsemi", &numEnds, 2, "    Produce semi-local alignments (Default mode)\n", &atSLocMeta);
		ArgumentParserEnumMeta atGlobMeta("Global");
			atGlobMeta.hidden = true;
			atGlobMeta.enumGroup = "Alignment Type";
			addEnumerationFlag("--atglobal", &numEnds, 4, "    Produce global alignments: probably not what you want.\n", &atGlobMeta);
		ArgumentParserIntMeta rankMeta("Non-optimal Scores");
			addIntegerOption("--rank", &uptoRank, 0, "    Specify the number of extra scores to report (0 for best only).\n    --rank 0\n", &rankMeta);
		ArgumentParserIntMeta countMeta("Variant Alignment Count");
			addIntegerOption("--count", &uptoCount, 0, "    Specify the maximum number of alignments to report.\n    Best scores given precedence, zero to report all alternatives.\n    --count 1\n", &countMeta);
		ArgumentParserIntMeta recSoftMeta("Reclaim Soft-clip Compensation");
			addIntegerOption("--reclaimsoft", &recSoft, 0, "    Use soft clipped bases when realigning, with overrun per base.\n    --reclaimsofft 0\n", &recSoftMeta);
		ArgumentParserStrMeta qualmMeta("Quality Mangle File");
			qualmMeta.isFile = true;
			qualmMeta.fileExts.insert(".qualm");
			addStringOption("--qualm", &qualmFile, 0, "    Specify how to modify alignment parameters using their quality.\n    --qualm File.qualm\n", &qualmMeta);
		ArgumentParserIntMeta hfuzzMeta("Score Encounter Limit");
			addIntegerOption("--hfuzz", &hotfuzz, 0, "    The maximum number of times to examine a score during score search.\n    Zero for a FULL traversal.\n    --hfuzz 0\n", &hfuzzMeta);
		ArgumentParserIntMeta threadMeta("Threads");
			addIntegerOption("--thread", &numThread, 0, "    The number of threads to use.\n    --thread 1\n", &threadMeta);
		ArgumentParserBoolMeta orderMeta("Preserve Order");
			addBooleanFlag("--order", &preserveOrder, 1, "    Have ProDerAl output items in the original order.\n    Uses more memory if paired entries are far apart in initial file.\n", &orderMeta);
	}
	~ProderalArgumentParser(){}
	int handleUnknownArgument(int argc, char** argv, std::ostream* helpOut){
		allSamIn.push_back(argv[0]);
		return 1;
	}
	void printExtraGUIInformation(std::ostream* toPrint){
		(*toPrint) << "STRINGVEC\t<|>\tNAME\tInput Files\tFILE\tREAD\t3\t.sam\t.sam.gz\t.sam.gzip\tDOCUMENT\t53414D2066696C657320746F207265616C69676E2E" << std::endl;
	}
	int posteriorCheck(){
		if(!refFile || (strlen(refFile)==0)){
			argumentError = "Need to specify a reference.";
			return 1;
		}
		if(!hitAll && (!probFile || (strlen(probFile)==0))){
			argumentError = "Need to specify the problematic regions.";
			return 1;
		}
		if(!costFile || (strlen(costFile)==0)){
			argumentError = "Need to specify a cost file.";
			return 1;
		}
		if(overRun < 0){
			argumentError = "overrun must be non-negative.";
			return 1;
		}
		if(uptoRank < 0){
			argumentError = "rank must be non-negative.";
			return 1;
		}
		if(uptoCount < 0){
			argumentError = "count must be non-negative.";
			return 1;
		}
		if(recSoft < 0){
			argumentError = "reclaimsofft must be non-negative.";
			return 1;
		}
		if(hotfuzz < 0){
			argumentError = "hfuzz must be non-negative.";
			return 1;
		}
		if(numThread <= 0){
			argumentError = "thread must be positive.";
			return 1;
		}
		if(allSamIn.size() == 0){
			allSamIn.push_back("-");
		}
		if(!outFileN || (strlen(outFileN)==0)){
			outFileN = defOutFN;
		}
		return 0;
	}
	/**The output file.*/
	char* outFileN = 0;
	/**The reference file.*/
	char* refFile = 0;
	/**The problem region file.*/
	char* probFile = 0;
	/**Whether all regions should be realigned.*/
	bool hitAll = false;
	/**The cost specification file.*/
	char* costFile = 0;
	/**The amount of overrun for region realign.*/
	intptr_t overRun = 20;
	/**The maximum number of bases to expand a realign region*/
	intptr_t maxExpand = -1;
	/**The number of ends in the alignment.*/
	intptr_t numEnds = 2;
	/**The worst rank to look at.*/
	intptr_t uptoRank = 0;
	/**The number of alignments to report.*/
	intptr_t uptoCount = 1;
	/**Try to reclaim soft clipped bases.*/
	intptr_t recSoft = 0;
	/**The quality mangle file.*/
	char* qualmFile = 0;
	/**The maximum number of times to see a score before stopping.*/
	intptr_t hotfuzz = 0;
	/**The number of threads to use.*/
	intptr_t numThread = 1;
	/**Whether to preserve order.*/
	bool preserveOrder = false;
	/**All input sam files.*/
	std::vector<const char*> allSamIn;
	/**The default output name.*/
	char defOutFN[2];
	
	/**the reference sequences: map from reference name to reference sequence*/
	std::map< std::string, std::string > allRefs;
	/**The problematic regions, by reference.*/
	std::map< std::string , std::vector< std::pair<intptr_t,intptr_t> > > probRegMap;
	/**The position dependent cost information.*/
	std::map<std::string,PositionDependentCostKDTree> allRegCosts;
	/**The quality mangle stuff.*/
	std::map<std::string, PositionDependentQualityMangleSet> allQualMangs;
	/**
	 * Load reference information.
	 */
	void loadReferenceData(std::ostream* warnRep){
		InStream* redFile = 0;
		SequenceReader* refRead = 0;
		TabularReader* tabRead = 0;
		std::string fileConts;
		try{
			//reference
				openSequenceFileRead(refFile, &redFile, &refRead);
				while(refRead->readNextEntry()){
					std::string crefNam(refRead->lastReadName, refRead->lastReadName + refRead->lastReadShortNameLen);
					std::string crefSeq(refRead->lastReadSeq, refRead->lastReadSeq + refRead->lastReadSeqLen);
					allRefs[crefNam] = crefSeq;
				}
				delete(refRead); refRead = 0;
				delete(redFile); redFile = 0;
			//load in the costs
				redFile = new FileInStream(costFile);
				fileConts.clear(); readStream(redFile, &fileConts);
				delete(redFile); redFile = 0;
				parseMultiregionPositionDependentCost(fileConts.c_str(), fileConts.c_str() + fileConts.size(), &allRegCosts);
			//load in the quality mangles
				if(qualmFile){
					redFile = new FileInStream(qualmFile);
					fileConts.clear(); readStream(redFile, &fileConts);
					delete(redFile); redFile = 0;
					parseMultiregionPositionQualityMangle(fileConts.c_str(), fileConts.c_str() + fileConts.size(), &allQualMangs);
				}
			//problem regions
				if(probFile){
					redFile = new FileInStream(probFile);
					tabRead = new TSVTabularReader(0, redFile);
					BedFileReader proBed(tabRead);
					delete(tabRead); tabRead = 0;
					delete(redFile); redFile = 0;
					//make easy to search
					std::map< std::string , std::vector< std::pair<intptr_t,intptr_t> > >::iterator probMIt;
					std::vector< std::pair<intptr_t,intptr_t> >::iterator proRanItA;
					std::vector< std::pair<intptr_t,intptr_t> >::iterator proRanItB;
					for(uintptr_t i = 0; i<proBed.chromosomes.size(); i++){
						std::pair<intptr_t,intptr_t> curRange(proBed.locStarts[i], proBed.locEnds[i]);
						probRegMap[proBed.chromosomes[i]].push_back(curRange);
					}
					//sort the vectors
					for(probMIt = probRegMap.begin(); probMIt != probRegMap.end(); probMIt++){
						std::sort(probMIt->second.begin(), probMIt->second.end());
					}
				}
		}catch(...){
			if(refRead){ delete(refRead); }
			if(redFile){ delete(redFile); }
			if(tabRead){ delete(tabRead); }
			throw;
		}
	}
};

/**A task to do with paired alignments.*/
class PairedAlignmentDoTask{
public:
	/**The ID of the main entry.*/
	uintptr_t mainID;
	/**The main alignment entry.*/
	CRBSAMFileContents* mainEnt;
	/**Whether there is a pair.*/
	bool havePair;
	/**The ID of the pair entry.*/
	uintptr_t pairID;
	/**The pair entry.*/
	CRBSAMFileContents* pairEnt;
};

/**A task to do with paired alignments.*/
class PairedAlignmentResultData{
public:
	/**The ID of these alignments.*/
	uintptr_t mainID;
	/**The alignments for this result.*/
	std::vector<CRBSAMFileContents*> liveAlns;
	/**The high and low bounds of each entry.*/
	std::vector< std::pair<intptr_t,intptr_t> > liveRanges;
	/**The allocated alignments.*/
	std::vector<CRBSAMFileContents> allocAlns;
};

/**Extra information on the results of an alignment (and allocation storage).*/
class PairedEndAlignmentSpecialFlags{
public:
	
	~PairedEndAlignmentSpecialFlags(){
		if(alnIterSave){ delete(alnIterSave); }
	}
	
	/**Whether the thing was aligned at all.*/
	int haveAln;
	/**The low coordinate of the main alignment.*/
	uintptr_t alnLow;
	/**The high coordinate of the high alignment.*/
	uintptr_t alnHig;
	
	/**Storage for a reference name.*/
	std::string mainNameTmp;
	/**Storage for a reference name.*/
	std::string mainRTmp;
	/**Storage for reference locations.*/
	std::vector<intptr_t> mainCigRLs;
	/**Storage for the reference.*/
	std::string mainRefSeq;
	/**Storage for the read.*/
	std::string mainReadSeq;
	/**Storage for quality.*/
	std::vector<char> mainQualStore;
	/**Storage for the scores*/
	std::vector<intptr_t> mainScores;
	/**Storage for how many times each score has been seen*/
	std::vector<uintptr_t> packScoreSeen;
	/**Storage for how many times each score should be taken*/
	std::vector<uintptr_t> packScoreTake;
	/**Used to easily package things.*/
	CRBSAMFileContents packEnt;
	/**Storage for a cigar.*/
	std::string cigarTmp;
	/**Storage for score data.*/
	std::string extraScoreTmp;
	/**Score indices.*/
	std::vector<intptr_t> workPackScores;
	/**Temporary range storage*/
	std::vector< std::pair<intptr_t,intptr_t> > tmpRanges;
	/**Storage for a rebased cost.*/
	PositionDependentCostKDTree rebaseCost;
	/**Storage for a mangled cost.*/
	PositionDependentCostKDTree mangleCost;
	/**Storage for a rebased mangle.*/
	PositionDependentQualityMangleSet rebaseMang;
	/**Storage for the alignment tables.*/
	PositionDependentAffineGapLinearPairwiseAlignment alnTables;
	/**The saved iteration token.*/
	LinearPairwiseAlignmentIteration* alnIterSave = 0;
};

/**
 * Relign a single sequence.
 * @param origSeq The original alignmetn.
 * @param storeAln The place to put the new alignments.
 * @param storeFlag The place to add extra data.
 * @param argsP The arguments to proderal.
 * @param errLock Lock for writign to stderr.
 */
void alignSingleSequence(CRBSAMFileContents* origSeq, PairedAlignmentResultData* storeAln, PairedEndAlignmentSpecialFlags* storeFlag, ProderalArgumentParser* argsP, void* errLock){
	std::greater<intptr_t> compMeth;
	intptr_t worstScore = ((intptr_t)-1) << (8*sizeof(intptr_t) - 1);
	uintptr_t maxCount = argsP->uptoCount;
	char numBuffer[4*sizeof(intptr_t)+4];
	std::pair<intptr_t,intptr_t> mainBnd = std::pair<intptr_t,intptr_t>(-1,-1);
	std::string* mainNameTmp = &(storeFlag->mainNameTmp);
	mainNameTmp->clear(); mainNameTmp->insert(mainNameTmp->end(), origSeq->entryName.begin(), origSeq->entryName.end());
	#define UNTOUCHED_FLAGS (SAM_FLAG_SEQREVCOMP | SAM_FLAG_FILTFAIL | SAM_FLAG_DUPLICATE | SAM_FLAG_FIRST | SAM_FLAG_LAST)
	#define DUMP_AS_IS \
		storeAln->liveAlns.resize(1);\
		if(storeAln->allocAlns.size() < 1){ storeAln->allocAlns.resize(1); }\
		storeAln->liveAlns[0] = &(storeAln->allocAlns[0]);\
		storeAln->allocAlns[0] = *origSeq;\
		storeAln->allocAlns[0].entryFlag = storeAln->allocAlns[0].entryFlag & UNTOUCHED_FLAGS;\
		storeAln->liveRanges.clear(); storeAln->liveRanges.push_back(mainBnd);\
		return;
	//if it was not aligned, don't try to align, just dump
		if(!samEntryIsAlign(origSeq)){
			storeFlag->haveAln = 0;
			DUMP_AS_IS
		}
	//get the cigar info
		storeFlag->haveAln = 1;
		std::vector<intptr_t>* mainCigRLs = &(storeFlag->mainCigRLs); mainCigRLs->clear();
		std::pair<uintptr_t,uintptr_t> mainSClip;
		try{
			mainSClip = cigarStringToReferencePositions(origSeq->entryPos, &(origSeq->entryCigar), mainCigRLs);
		}catch(std::exception& err){
			lockMutex(errLock);
				std::cerr << *mainNameTmp << ": " << err.what() << std::endl;
			unlockMutex(errLock);
			storeFlag->haveAln = 0;
			DUMP_AS_IS
		}
		mainBnd = getCigarReferenceBounds(mainCigRLs);
		storeFlag->alnLow = mainBnd.first; storeFlag->alnHig = mainBnd.second + 1;
	//get the reference information
		std::string* mainRTmp = &(storeFlag->mainRTmp);
		mainRTmp->clear(); mainRTmp->insert(mainRTmp->end(), origSeq->entryReference.begin(), origSeq->entryReference.end());
		std::string* mainRef = (argsP->allRefs.find(*mainRTmp) != argsP->allRefs.end()) ? &(argsP->allRefs[*mainRTmp]) : 0;
		std::vector< std::pair<intptr_t,intptr_t> >* mainProbs = (argsP->probRegMap.find(*mainRTmp) != argsP->probRegMap.end()) ? &(argsP->probRegMap[*mainRTmp]) : 0;
		PositionDependentCostKDTree* mainCosts = (argsP->allRegCosts.find(*mainRTmp) != argsP->allRegCosts.end()) ? &(argsP->allRegCosts[*mainRTmp]) : 0;
		PositionDependentQualityMangleSet* mainMang = (argsP->allQualMangs.find(*mainRTmp) != argsP->allQualMangs.end()) ? &(argsP->allQualMangs[*mainRTmp]) : 0;
	//complain and short circuit if no reference
		if(mainRef == 0){
			lockMutex(errLock);
				std::cerr << "Cannot find reference " << *mainRTmp << std::endl;
			unlockMutex(errLock);
			DUMP_AS_IS
		}
	//complain about wonky cigars
		if((uintptr_t)(mainBnd.second) >= mainRef->size()){
			lockMutex(errLock);
				std::cerr << "Entry " << *mainNameTmp << " cigar extends beyond reference." << std::endl;
			unlockMutex(errLock);
		}
		if((mainSClip.first + mainSClip.second + mainCigRLs->size()) != origSeq->entrySeq.size()){
			lockMutex(errLock);
				std::cerr << "Entry " << *mainNameTmp << " cigar claims more sequence than is present." << std::endl;
			unlockMutex(errLock);
		}
	//expand
		std::pair<intptr_t,intptr_t> mainExp = mainBnd;
		mainExp.first -= (argsP->overRun + mainSClip.first*argsP->recSoft);
			if(mainExp.first < 0){ mainExp.first = 0; }
		mainExp.second += (argsP->overRun + mainSClip.second*argsP->recSoft);
			if((uintptr_t)(mainExp.second) >= mainRef->size()){ mainExp.second = mainRef->size()-1; }
			mainExp.second++;
	//see if it needs realignment
		bool mainNeed = argsP->hitAll;
		if(mainProbs){
			std::vector< std::pair<intptr_t,intptr_t> >::iterator proRanItA = std::lower_bound(mainProbs->begin(), mainProbs->end(), std::pair<intptr_t,intptr_t>(mainExp.first,mainExp.first));
			if(proRanItA != mainProbs->begin()){ proRanItA--; }
			std::vector< std::pair<intptr_t,intptr_t> >::iterator proRanItB = std::upper_bound(mainProbs->begin(), mainProbs->end(), std::pair<intptr_t,intptr_t>(mainExp.second,mainExp.second));
			while(proRanItA != proRanItB){
				if((proRanItA->first <= mainExp.second) && (proRanItA->second > mainExp.first)){
					mainNeed = true;
					mainExp.first = std::min(mainExp.first, proRanItA->first);
					mainExp.second = std::max(mainExp.second, proRanItA->second);
				}
				proRanItA++;
			}
		}
		if(!mainNeed){
			DUMP_AS_IS
		}
	//upper bound the expansion
		uintptr_t maxExpand = argsP->maxExpand;
		if((uintptr_t)(mainExp.second - (mainBnd.second + 1)) > maxExpand){
			mainExp.second = (mainBnd.second + 1 + maxExpand);
		}
		if((uintptr_t)(mainBnd.first - mainExp.first) > maxExpand){
			mainExp.first = mainBnd.first - maxExpand;
		}
	//various idiot checks
		if(mainCosts == 0){
			lockMutex(errLock);
				std::cerr << "Missing cost info for reference " << *mainRTmp << std::endl;
			unlockMutex(errLock);
			DUMP_AS_IS
		}
		if((mainMang == 0) && (argsP->qualmFile)){
			lockMutex(errLock);
				std::cerr << "Missing quality mangle info for reference " << *mainRTmp << std::endl;
			unlockMutex(errLock);
			DUMP_AS_IS
		}
		if((origSeq->entrySeq.size() == 0) || (origSeq->entryQual.size() == 0)){
			lockMutex(errLock);
				std::cerr << "Entry " << *mainNameTmp << " missing sequence data." << std::endl;
			unlockMutex(errLock);
			DUMP_AS_IS
		}
	//setup the alignment
		std::string* mainRefSeq = &(storeFlag->mainRefSeq);
		std::string* mainReadSeq = &(storeFlag->mainReadSeq);
		mainRefSeq->clear(); mainRefSeq->insert(mainRefSeq->end(), mainRef->begin() + mainExp.first, mainRef->begin() + mainExp.second);
		mainReadSeq->clear(); mainReadSeq->insert(mainReadSeq->end(), origSeq->entrySeq.begin() + (argsP->recSoft ? 0 : mainSClip.first), origSeq->entrySeq.end() - (argsP->recSoft ? 0 : mainSClip.second));
		PositionDependentCostKDTree* mainUseCost = &(storeFlag->rebaseCost);
			mainUseCost->regionsRebased(mainCosts, mainExp.first, mainExp.second, -1, -1);
		if(mainMang){
			std::vector<char>* mainQualStore = &(storeFlag->mainQualStore);
			mainQualStore->clear(); mainQualStore->insert(mainQualStore->end(), origSeq->entryQual.begin() + (argsP->recSoft ? 0 : mainSClip.first), origSeq->entryQual.end() - (argsP->recSoft ? 0 : mainSClip.second));
			PositionDependentQualityMangleSet* subMang = &(storeFlag->rebaseMang);
				subMang->rebase(mainMang, mainExp.first, mainExp.second);
			PositionDependentCostKDTree* tmpUse = &(storeFlag->mangleCost);
				tmpUse->regionsQualityMangled(mainUseCost, subMang, mainQualStore);
			mainUseCost = tmpUse;
		}
		mainUseCost->produceFromRegions();
		PositionDependentAffineGapLinearPairwiseAlignment* mainAln = &(storeFlag->alnTables);
		mainAln->changeProblem(argsP->numEnds, mainRefSeq, mainReadSeq, mainUseCost);
		mainAln->prepareAlignmentStructure();
		if(storeFlag->alnIterSave == 0){
			storeFlag->alnIterSave = mainAln->getIteratorToken();
		}
		LinearPairwiseAlignmentIteration* mainIter = storeFlag->alnIterSave;
	//get scores
		std::vector<intptr_t>* mainScores = &(storeFlag->mainScores);
		int mainNumScore = mainAln->findAlignmentScores(mainIter, mainScores->size(), &((*mainScores)[0]), worstScore, argsP->hotfuzz);
	//set up the defaults
		CRBSAMFileContents* packEnt = &(storeFlag->packEnt);
		packEnt->lastReadHead = 0;
		packEnt->headerTxt.clear();
		packEnt->entryName = origSeq->entryName;
		packEnt->entryReference = origSeq->entryReference;
		packEnt->entryMapq = origSeq->entryMapq;
		packEnt->nextReference.clear();
		packEnt->nextPos = -1;
		packEnt->entryTempLen = 0;
		packEnt->entrySeq = origSeq->entrySeq;
		packEnt->entryQual = origSeq->entryQual;
		//open items are: -entryFlag -entryPos -entryCigar -entryExtra
		//open flags are: SAM_FLAG_SECONDARY
	//loop through: the first optimal is the main (note its bounds)
		int foundMain = 0;
		std::vector<intptr_t>* workPackScores = &(storeFlag->workPackScores);
			workPackScores->clear();
		std::vector< std::pair<intptr_t,intptr_t> >* tmpRanges = &(storeFlag->tmpRanges);
			tmpRanges->clear();
		std::vector<uintptr_t>* packScoreSeen = &(storeFlag->packScoreSeen);
		packScoreSeen->resize(mainNumScore);
		for(int i = 0; i<mainNumScore; i++){ (*packScoreSeen)[i] = 0; }
		uintptr_t nextSaveEntI = 0;
		mainAln->startFuzzyIteration(mainIter, (*mainScores)[mainNumScore-1], argsP->hotfuzz, mainNumScore);
		while(true){
			if(!(mainIter->getNextAlignment())){break;}
			uintptr_t scoreInd = std::lower_bound(mainScores->begin(), mainScores->begin() + mainNumScore, mainIter->alnScore, compMeth) - mainScores->begin();
			if(maxCount && ((*packScoreSeen)[scoreInd] >= maxCount)){ continue; }
			std::string* cigarTmp = &(storeFlag->cigarTmp);
				cigarTmp->clear();
				if((argsP->recSoft == 0) && (mainSClip.first)){
					sprintf(numBuffer, "%jd", (intmax_t)mainSClip.first);
					cigarTmp->append(numBuffer);
					cigarTmp->push_back('S');
				}
				uintptr_t subOff;
				mainIter->toCigar(cigarTmp, &subOff);
				if((argsP->recSoft == 0) && (mainSClip.second)){
					sprintf(numBuffer, "%jd", (intmax_t)mainSClip.second);
					cigarTmp->append(numBuffer);
					cigarTmp->push_back('S');
				}
				packEnt->entryCigar.clear(); packEnt->entryCigar.insert(packEnt->entryCigar.end(), cigarTmp->begin(), cigarTmp->end());
			intptr_t entryStart = mainExp.first + mainIter->aInds[0];
				intptr_t entryEnd = mainExp.first + mainIter->aInds[mainIter->aInds.size()-1];
				packEnt->entryPos = entryStart;
			std::string* extraScoreTmp = &(storeFlag->extraScoreTmp);
				extraScoreTmp->clear();
				extraScoreTmp->append("AS:i:");
				sprintf(numBuffer, "%jd", (intmax_t)(mainIter->alnScore));
				extraScoreTmp->append(numBuffer);
				packEnt->entryExtra.clear(); packEnt->entryExtra.insert(packEnt->entryExtra.end(), extraScoreTmp->begin(), extraScoreTmp->end());
			tmpRanges->push_back(std::pair<intptr_t,intptr_t>(entryStart,entryEnd));
			packEnt->entryFlag = origSeq->entryFlag & UNTOUCHED_FLAGS;
			int isMainAln = (foundMain == 0) && (scoreInd == 0);
			if(isMainAln){
				foundMain = 1;
				if(!samEntryIsAlign(packEnt)){
					storeFlag->haveAln = 0;
				}
				else{
					storeFlag->alnLow = entryStart;
					storeFlag->alnHig = entryEnd;
				}
			}
			else{
				packEnt->entryFlag = packEnt->entryFlag | SAM_FLAG_SECONDARY;
			}
			if(nextSaveEntI >= storeAln->allocAlns.size()){
				storeAln->allocAlns.push_back(*packEnt);
			}
			else{
				storeAln->allocAlns[nextSaveEntI] = *packEnt;
			}
			workPackScores->push_back(scoreInd);
			nextSaveEntI++;
			(*packScoreSeen)[scoreInd]++;
			if((scoreInd == 0) && maxCount && ((*packScoreSeen)[scoreInd] >= maxCount)){ break; }
		}
	//note which ones should be reported
		storeAln->liveAlns.clear();
		if(maxCount){
			std::vector<uintptr_t>* packScoreTake = &(storeFlag->packScoreTake);
			packScoreTake->resize(mainNumScore);
			uintptr_t numCountLeft = maxCount;
			for(int i = 0; i<mainNumScore; i++){
				if((*packScoreSeen)[i] > numCountLeft){
					(*packScoreTake)[i] = numCountLeft;
					numCountLeft = 0;
				}
				else{
					(*packScoreTake)[i] = (*packScoreSeen)[i];
					numCountLeft -= (*packScoreSeen)[i];
				}
			}
			for(uintptr_t i = 0; i<nextSaveEntI; i++){
				if((*packScoreTake)[(*workPackScores)[i]]){
					storeAln->liveAlns.push_back(&(storeAln->allocAlns[i]));
					storeAln->liveRanges.push_back((*tmpRanges)[i]);
					(*packScoreTake)[(*workPackScores)[i]]--;
				}
			}
		}
		else{
			for(uintptr_t i = 0; i<nextSaveEntI; i++){
				storeAln->liveAlns.push_back(&(storeAln->allocAlns[i]));
				storeAln->liveRanges.push_back((*tmpRanges)[i]);
			}
		}
}

/**Arguments to the threads that do the work.*/
typedef struct{
	/**Lock for stderr*/
	void* errLock;
	/**Arguments to ProDerAl.*/
	ProderalArgumentParser* argsP;
	/**Return entry data.*/
	ThreadsafeReusableContainerCache<CRBSAMFileContents>* entC;
	/**Get tasks to do.*/
	ThreadProdComCollector<PairedAlignmentDoTask>* taskPCC;
	/**Output results to write.*/
	ThreadProdComCollector<PairedAlignmentResultData>* resPCC;
} PairedEndThreadArgs;

/**
 * Actually perform alignments.
 * @param myUni Actually a PairedEndThreadArgs.
 */
void pairAlignDoThing(void* myUni){
	PairedEndThreadArgs* myArgs = (PairedEndThreadArgs*)myUni;
	
	PairedEndAlignmentSpecialFlags mainFlag;
		mainFlag.mainScores.resize(myArgs->argsP->uptoRank+1);
	PairedEndAlignmentSpecialFlags pairFlag;
		pairFlag.mainScores.resize(myArgs->argsP->uptoRank+1);
	
	PairedAlignmentDoTask* curDo = myArgs->taskPCC->getThing();
	while(curDo){
		PairedAlignmentResultData* mainRes = myArgs->resPCC->taskCache.alloc();
		mainRes->mainID = curDo->mainID;
		alignSingleSequence(curDo->mainEnt, mainRes, &mainFlag, myArgs->argsP, myArgs->errLock);
		//check unmap
		for(uintptr_t i = 0; i<mainRes->liveAlns.size(); i++){
			CRBSAMFileContents* curItem = mainRes->liveAlns[i];
			if(!samEntryIsAlign(curItem) || ((mainRes->liveAlns.size()==1) && (mainFlag.haveAln == 0))){ curItem->entryFlag = curItem->entryFlag | SAM_FLAG_SEGUNMAP; }
		}
		if(curDo->havePair){
			PairedAlignmentResultData* pairRes = myArgs->resPCC->taskCache.alloc();
			pairRes->mainID = curDo->pairID;
			alignSingleSequence(curDo->pairEnt, pairRes, &pairFlag, myArgs->argsP, myArgs->errLock);
			//check unmap
			for(uintptr_t i = 0; i<pairRes->liveAlns.size(); i++){
				CRBSAMFileContents* curItem = pairRes->liveAlns[i];
				if(!samEntryIsAlign(curItem) || ((pairRes->liveAlns.size()==1) && (pairFlag.haveAln == 0))){ curItem->entryFlag = curItem->entryFlag | SAM_FLAG_SEGUNMAP; }
			}
			//handle pair flags
			for(uintptr_t i = 0; i<mainRes->liveAlns.size(); i++){
				CRBSAMFileContents* curItem = mainRes->liveAlns[i];
				curItem->nextReference.insert(curItem->nextReference.end(), curDo->pairEnt->entryReference.begin(), curDo->pairEnt->entryReference.end());
				curItem->entryFlag |= SAM_FLAG_MULTSEG;
				if(pairFlag.haveAln){
					curItem->nextPos = pairFlag.alnLow;
					if((curItem->entryFlag & SAM_FLAG_SEGUNMAP) == 0){
						curItem->entryFlag |= SAM_FLAG_ALLALN;
						std::pair<intptr_t,intptr_t> curItemR = mainRes->liveRanges[i];
						intptr_t tempLen = std::max((uintptr_t)(curItemR.second), pairFlag.alnHig) - std::min((uintptr_t)(curItemR.first), pairFlag.alnLow);
						if(((uintptr_t)(curItemR.first) < pairFlag.alnLow) || (((uintptr_t)(curItemR.first) == pairFlag.alnLow) && ((uintptr_t)(curItemR.second) < pairFlag.alnHig))){
						}
						else{
							tempLen = -tempLen;
						}
						curItem->entryTempLen = tempLen;
					}
				}
				else{
					curItem->entryFlag |= SAM_FLAG_NEXTSEGUNMAP;
				}
				if(curDo->pairEnt->entryFlag & SAM_FLAG_SEQREVCOMP){ curItem->entryFlag |= SAM_FLAG_NEXTSEQREVCOMP; }
			}
			for(uintptr_t i = 0; i<pairRes->liveAlns.size(); i++){
				CRBSAMFileContents* curItem = pairRes->liveAlns[i];
				curItem->nextReference.insert(curItem->nextReference.end(), curDo->mainEnt->entryReference.begin(), curDo->mainEnt->entryReference.end());
				curItem->entryFlag |= SAM_FLAG_MULTSEG;
				if(mainFlag.haveAln){
					curItem->nextPos = mainFlag.alnLow;
					if((curItem->entryFlag & SAM_FLAG_SEGUNMAP) == 0){
						curItem->entryFlag |= SAM_FLAG_ALLALN;
						std::pair<intptr_t,intptr_t> curItemR = pairRes->liveRanges[i];
						intptr_t tempLen = std::max((uintptr_t)(curItemR.second), mainFlag.alnHig) - std::min((uintptr_t)(curItemR.first), mainFlag.alnLow);
						if(((uintptr_t)(curItemR.first) < mainFlag.alnLow) || (((uintptr_t)(curItemR.first) == mainFlag.alnLow) && ((uintptr_t)(curItemR.second) < mainFlag.alnHig))){
						}
						else{
							tempLen = -tempLen;
						}
						curItem->entryTempLen = tempLen;
					}
				}
				else{
					curItem->entryFlag |= SAM_FLAG_NEXTSEGUNMAP;
				}
				if(curDo->mainEnt->entryFlag & SAM_FLAG_SEQREVCOMP){ curItem->entryFlag |= SAM_FLAG_NEXTSEQREVCOMP; }
			}
			//what a mess
			myArgs->resPCC->addThing(pairRes);
			//return the pair entry to the allocator
			myArgs->entC->dealloc(curDo->pairEnt);
		}
		myArgs->resPCC->addThing(mainRes);
		//return the main thing to the allocator
		myArgs->entC->dealloc(curDo->mainEnt);
		myArgs->taskPCC->taskCache.dealloc(curDo);
		curDo = myArgs->taskPCC->getThing();
	}
}

/**Arguments to the threads that do the work.*/
typedef struct{
	/**Lock for stderr*/
	void* errLock;
	/**The place to output.*/
	CRBSAMFileWriter* curOut;
	/**Arguments to ProDerAl.*/
	ProderalArgumentParser* argsP;
	/**Output results to write.*/
	ThreadProdComCollector<PairedAlignmentResultData>* resPCC;
} FinalOutputThreadArgs;

void outputFinalResults(void* tmpArg){
	FinalOutputThreadArgs* myArgs = (FinalOutputThreadArgs*)tmpArg;
	bool presOrder = myArgs->argsP->preserveOrder;
	std::map<uintptr_t,PairedAlignmentResultData*> waitResWrite;
	uintptr_t numWriteOut = 0;
	PairedAlignmentResultData* anyRes = myArgs->resPCC->getThing();
	while(anyRes){
		if(presOrder){
			waitResWrite[anyRes->mainID] = anyRes;
			std::map<uintptr_t,PairedAlignmentResultData*>::iterator cdIt = waitResWrite.find(numWriteOut);
			while(cdIt != waitResWrite.end()){
				PairedAlignmentResultData* curDump = cdIt->second;
				try{
					for(uintptr_t i = 0; i<curDump->liveAlns.size(); i++){
						myArgs->curOut->writeNextEntry(curDump->liveAlns[i]);
					}
				}catch(std::exception& err){
					lockMutex(myArgs->errLock);
						std::cerr << "Exception thrown in output: " << err.what() << std::endl;
					unlockMutex(myArgs->errLock);
				}
				myArgs->resPCC->taskCache.dealloc(curDump);
				waitResWrite.erase(cdIt);
				numWriteOut++;
				cdIt = waitResWrite.find(numWriteOut);
			}
		}
		else{
			try{
				for(uintptr_t i = 0; i<anyRes->liveAlns.size(); i++){
						myArgs->curOut->writeNextEntry(anyRes->liveAlns[i]);
				}
			}catch(std::exception& err){
				lockMutex(myArgs->errLock);
					std::cerr << "Exception thrown in output: " << err.what() << std::endl;
				unlockMutex(myArgs->errLock);
			}
			myArgs->resPCC->taskCache.dealloc(anyRes);
			numWriteOut++;
		}
		anyRes = myArgs->resPCC->getThing();
	}
}

#define MAX_QUEUE_SIZE 16

/**Run the stupid thing.*/
int main(int argc, char** argv){
	int retCode = 0;
	//parse arguments
		ProderalArgumentParser argsP;
			if(argsP.parseArguments(argc-1, argv+1, &std::cout) < 0){
				std::cerr << argsP.argumentError << std::endl;
				return 1;
			}
			if(argsP.needRun == 0){ return 0; }
	//common things to save
		void* errLock = makeMutex();
		InStream* curInpF = 0;
		TabularReader* curInpT = 0;
		CRBSAMFileReader* curInp = 0;
		OutStream* curOutF = 0;
		TabularWriter* curOutT = 0;
		CRBSAMFileWriter* curOut = 0;
	//prepare caches and work queues
		ThreadsafeReusableContainerCache<CRBSAMFileContents> entCache;
		PairedEndCache proCache;
		ThreadProdComCollector<PairedAlignmentDoTask> taskPCC(MAX_QUEUE_SIZE*argsP.numThread);
		ThreadProdComCollector<PairedAlignmentResultData> resPCC(MAX_QUEUE_SIZE*argsP.numThread);
		std::vector<PairedEndThreadArgs> threadArgs;
		std::vector<void*> liveThread;
		FinalOutputThreadArgs finOutA;
		void* finOutT = 0;
try{
	//load the reference data
		argsP.loadReferenceData(&std::cerr);
	//start up the work threads
		threadArgs.resize(argsP.numThread);
		for(intptr_t i = 0; i<argsP.numThread; i++){
			threadArgs[i] = {errLock, &argsP, &entCache, &taskPCC, &resPCC};
			liveThread.push_back(startThread(pairAlignDoThing, &(threadArgs[i])));
		}
	//start up the output thread
		openCRBSamFileWrite(argsP.outFileN, &curOutF, &curOutT, &curOut);
		finOutA = {errLock, curOut, &argsP, &resPCC};
		finOutT = startThread(outputFinalResults, &finOutA);
	//run down the files
		uintptr_t numTaskIn = 0;
		CRBSAMFileContents* curEnt = entCache.alloc();
		bool haveEndHead = false;
		for(uintptr_t si = 0; si < argsP.allSamIn.size(); si++){
			//open
			openCRBSamFileRead(argsP.allSamIn[si], &curInpF, &curInpT, &curInp);
			//run down the file looking for unpaired and paired
			while(curInp->readNextEntry(curEnt)){
				//manage the entry
					if(curEnt->lastReadHead){
						if(!haveEndHead){
							curOut->writeNextEntry(curEnt);
						}
					}
					else{
						if(!haveEndHead){
							const char* progNVS = PRODERAL_HEADERFLAG;
							curOut->curEnt.clear();
							curOut->curEnt.lastReadHead = 1;
							curOut->curEnt.headerTxt.insert(curOut->curEnt.headerTxt.end(), progNVS, progNVS + strlen(progNVS));
							curOut->writeNextEntry();
							haveEndHead = true;
						}
						//skip secondary/supplementary stuff
						if(!samEntryIsPrimary(curEnt)){ continue; }
						//if paired, handle special
						if(samEntryNeedPair(curEnt)){
							if(proCache.havePair(curEnt)){
								PairedAlignmentDoTask* curPush = taskPCC.taskCache.alloc();
								curPush->mainID = numTaskIn;
								curPush->mainEnt = curEnt;
								curPush->havePair = true;
								std::pair<uintptr_t,CRBSAMFileContents*> origEnt = proCache.getPair(curEnt);
								curPush->pairID = origEnt.first;
								curPush->pairEnt = origEnt.second;
								taskPCC.addThing(curPush);
							}
							else{
								proCache.waitForPair(numTaskIn, curEnt);
							}
						}
						else{
							PairedAlignmentDoTask* curPush = taskPCC.taskCache.alloc();
							curPush->mainID = numTaskIn;
							curPush->mainEnt = curEnt;
							curPush->havePair = false;
							taskPCC.addThing(curPush);
						}
						curEnt = entCache.alloc();
						numTaskIn++;
					}
			}
			//close
			delete(curInp); curInp = 0;
			delete(curInpT); curInpT = 0;
			delete(curInpF); curInpF = 0;
		}
		entCache.dealloc(curEnt); //got one too many
	//drain the pairs
		std::string badPairName;
		while(proCache.haveOutstanding()){
			PairedAlignmentDoTask* curPush = taskPCC.taskCache.alloc();
			std::pair<uintptr_t,CRBSAMFileContents*> origEnt = proCache.getOutstanding();
			curPush->mainID = origEnt.first;
			curPush->mainEnt = origEnt.second;
			curPush->havePair = false;
			badPairName.clear(); badPairName.insert(badPairName.end(), curPush->mainEnt->entryName.begin(), curPush->mainEnt->entryName.end());
			lockMutex(errLock);
				std::cerr << "Entry " << badPairName << " claims to be paired, but no pair is in file." << std::endl;
			unlockMutex(errLock);
			taskPCC.addThing(curPush);
		}
}catch(std::exception& err){
	std::cerr << err.what() << std::endl;
	retCode = 1;
}
	//cleanUp:
	//end the task cache, join the threads
		taskPCC.end();
		for(uintptr_t i = 0; i<liveThread.size(); i++){
			joinThread(liveThread[i]);
		}
		liveThread.clear();
	//end the result cache, join the finale
		resPCC.end();
		if(finOutT){ joinThread(finOutT); }
	//close any open files
		if(curInp){ delete(curInp); }
		if(curInpT){ delete(curInpT); }
		if(curInpF){ delete(curInpF); }
		if(curOut){ delete(curOut); }
		if(curOutT){ delete(curOutT); }
		if(curOutF){ delete(curOutF); }
	//and the error lock
		killMutex(errLock);
	return retCode;
}

