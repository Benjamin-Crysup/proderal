
#include "proderal_setup.h"

#include <string.h>
#include <algorithm>

#include "whodun_datread.h"

#define WHITESPACE " \t\r\n"

int parseArguments(int argc, char** argv, ProderalCLIArgs* toFill, std::vector<const char*>* fileStore){
	toFill->needRun = true;
	toFill->refFile = 0;
	toFill->probFile = 0;
	toFill->hitAll = false;
	toFill->costFile = 0;
	toFill->overRun = 20;
	toFill->numEnds = 2;
	toFill->uptoRank = 0;
	toFill->uptoCount = 1;
	toFill->recSoft = 0;
	toFill->cachesize = 100000000L;
	toFill->qualmFile = 0;
	toFill->hotfuzz = 0;
	int i = 1;
	while(i < argc){
		bool haveNext = (i+1) < argc;
		if(strcmp(CLARG_REF, argv[i])==0){
			if(!haveNext){ std::cerr << "--ref needs a file to use for reference." << std::endl; return 1; }
			toFill->refFile = argv[i+1];
			i+=2;
		}
		else if(strcmp(CLARG_PROB, argv[i])==0){
			if(!haveNext){ std::cerr << "--prob needs a file to mark problem regions." << std::endl; return 1; }
			toFill->probFile = argv[i+1];
			i+=2;
		}
		else if(strcmp(CLARG_PROMIS, argv[i])==0){
			toFill->hitAll = true;
			i++;
		}
		else if(strcmp(CLARG_COST, argv[i])==0){
			if(!haveNext){ std::cerr << "--cost needs a cost file." << std::endl; return 1; }
			toFill->costFile = argv[i+1];
			i+=2;
		}
		else if(strcmp(CLARG_OVERRUN, argv[i])==0){
			if(!haveNext){ std::cerr << "--overrun needs an over-run size." << std::endl; return 1; }
			toFill->overRun = atoi(argv[i+1]);
			i+=2;
		}
		else if(strcmp(CLARG_LOCAL, argv[i])==0){
			toFill->numEnds = 0;
			i++;
		}
		else if(strcmp(CLARG_SEMI, argv[i])==0){
			toFill->numEnds = 2;
			i++;
		}
		else if(strcmp(CLARG_GLOBAL, argv[i])==0){
			toFill->numEnds = 4;
			i++;
		}
		else if(strcmp(CLARG_RANK, argv[i])==0){
			if(!haveNext){ std::cerr << "--rank needs a rank specification." << std::endl; return 1; }
			toFill->uptoRank = atoi(argv[i+1]);
			i+=2;
		}
		else if(strcmp(CLARG_COUNT, argv[i])==0){
			if(!haveNext){ std::cerr << "--count needs a count specification." << std::endl; return 1; }
			toFill->uptoCount = atoi(argv[i+1]);
			i+=2;
		}
		else if(strcmp(CLARG_SOFT, argv[i])==0){
			if(!haveNext){ std::cerr << "--reclaimsoft needs the extra overrun per soft clip." << std::endl; return 1; }
			toFill->recSoft = atoi(argv[i+1]);
			i+=2;
		}
		else if(strcmp(CLARG_CACHE, argv[i])==0){
			if(!haveNext){ std::cerr << "--cache needs a cache size." << std::endl; return 1; }
			toFill->cachesize = atol(argv[i+1]);
			i+=2;
		}
		else if(strcmp(CLARG_QUALM, argv[i])==0){
			if(!haveNext){ std::cerr << "--qualm needs an alteration file." << std::endl; return 1; }
			toFill->qualmFile = argv[i+1];
			i+=2;
		}
		else if(strcmp(CLARG_HFUZZ, argv[i])==0){
			if(!haveNext){ std::cerr << "--hfuzz needs a maximum score spot parameter." << std::endl; return 1; }
			toFill->hotfuzz = atoi(argv[i+1]);
			i+=2;
		}
		else if((strcmp(CLARG_HELPA, argv[i])==0) || (strcmp(CLARG_HELPB, argv[i])==0) || (strcmp(CLARG_HELPC, argv[i])==0)){
			std::cout << "Usage: proderal [OPTION] [FILE]" << std::endl;
			std::cout << "Realigns entries in a SAM file that map to problematic regions." << std::endl;
			std::cout << "Multiple SAM files can be included: if none supplied, stdin is used." << std::endl;
			std::cout << "The OPTIONS are:" << std::endl;
			std::cout << "--ref FILE" << std::endl;
			std::cout << "        Specify the reference sequences in a fasta file." << std::endl;
			std::cout << "--prob FILE" << std::endl;
			std::cout << "        Specify the problematic regions in a bed file." << std::endl;
			std::cout << "--promiscuous" << std::endl;
			std::cout << "        Specify that all regions are problematic." << std::endl;
			std::cout << "--cost FILE" << std::endl;
			std::cout << "        Specify the position dependent cost function." << std::endl;
			std::cout << "--overrun 20" << std::endl;
			std::cout << "        Specify the number of extra bases to get when realigning." << std::endl;
			std::cout << "--atlocal" << std::endl;
			std::cout << "        Use local alignment." << std::endl;
			std::cout << "--atsemi" << std::endl;
			std::cout << "        Use semi-global alignment." << std::endl;
			std::cout << "--atglobal" << std::endl;
			std::cout << "        Use global alignment." << std::endl;
			std::cout << "--rank 0" << std::endl;
			std::cout << "        Specify what rank of alignment to report. 0 for best alignments." << std::endl;
			std::cout << "--count 1" << std::endl;
			std::cout << "        Specify how many alternate alignments to report." << std::endl;
			std::cout << "--reclaimsoft 0" << std::endl;
			std::cout << "        Use soft clipped bases when realigning, with overrun per base." << std::endl;
			//std::cout << "--cache 1000000000" << std::endl;
			//std::cout << "        The number of bytes to use for cache." << std::endl;
			std::cout << "--qualm FILE" << std::endl;
			std::cout << "        Specify how to modify alignment parameters using their quality." << std::endl;
			std::cout << "--hfuzz 0" << std::endl;
			std::cout << "        The maximum number of times to examine a score during score search." << std::endl;
			toFill->needRun = false;
			return 0;
		}
		else if(strcmp(CLARG_VERSION, argv[i])==0){
			std::cout << "ProDerAl 0.0" << std::endl;
			std::cout << "Copyright (C) 2019 UNT HSC Center for Human Identification" << std::endl;
			std::cout << "License LGPL: GNU LGPL" << std::endl;
			std::cout << "This is free software: you are free to change and redistribute it." << std::endl;
			std::cout << "There is NO WARRANTY, to the extent permitted by law" << std::endl;
			//TODO citeas
			toFill->needRun = false;
			return 0;
		}
		else{
			fileStore->push_back(argv[i]);
			i++;
		}
	}
	if(fileStore->size() == 0){
		fileStore->push_back("-");
	}
	if(toFill->refFile == 0){
		std::cerr << "Need to specify a reference." << std::endl;
		return 1;
	}
	if((toFill->probFile == 0) && !(toFill->hitAll)){
		std::cerr << "Need to specify the problematic regions." << std::endl;
		return 1;
	}
	if(toFill->costFile == 0){
		std::cerr << "Need to specify a cost file." << std::endl;
		return 1;
	}
	return 0;
}

ProblemLoadSetup::ProblemLoadSetup(ProderalCLIArgs* pargs){
	setupValid = true;
	//load the references
		LiveFastqFileReader* refSeqF = openFastqFile(pargs->refFile);
		if(refSeqF == 0){
			setupValid = false;
			setupErr.append("Problem reading reference fasta ");
			setupErr.append(pargs->refFile);
			setupErr.append(".\n");
			return;
		}
		while(readNextFastqEntry(refSeqF)){
			std::string seqNam(refSeqF->lastReadName, refSeqF->lastReadName + strcspn(refSeqF->lastReadName, WHITESPACE));
			std::string seqSeq(refSeqF->lastReadSeq);
			allRefs[seqNam] = seqSeq;
		}
		closeFastqFile(refSeqF);
	//load in the problematic regions
		BedFileContents* proBed = 0;
		if(pargs->probFile){
			LiveTSVFileReader* proBedF = openTSVFile(pargs->probFile);
			if(proBedF == 0){
				setupValid = false;
				setupErr.append("Problem reading problematic bed file ");
				setupErr.append(pargs->probFile);
				setupErr.append(".\n");
				return;
			}
			proBed = readBedFile(proBedF);
		}
	//make the problematic regions easy to search
		std::map< std::string , std::vector< std::pair<intptr_t,intptr_t> > >::iterator probMIt;
		std::vector< std::pair<intptr_t,intptr_t> >::iterator proRanItA;
		std::vector< std::pair<intptr_t,intptr_t> >::iterator proRanItB;
		if(proBed){
			for(int i = 0; i<proBed->numEnts; i++){
				std::string curChrom(proBed->chroms[i]);
				std::pair<intptr_t,intptr_t> curRange(proBed->starts[i], proBed->ends[i]);
				probRegMap[curChrom].push_back(curRange);
			}
			freeBedFile(proBed);
			//sort the vectors
			for(probMIt = probRegMap.begin(); probMIt != probRegMap.end(); probMIt++){
				std::sort(probMIt->second.begin(), probMIt->second.end());
			}
		}
	//load in the cost file
		uintptr_t costFileLen;
		char* costFile = readEntireFile(pargs->costFile, &costFileLen);
		if(costFile == 0){
			setupValid = false;
			setupErr.append("Problem reading position dependent file ");
			setupErr.append(pargs->costFile);
			setupErr.append(".\n");
			return;
		}
		int isProb = parseAsciiMultiregionPositionDependentSpecification(costFileLen, costFile, &allRegCosts);
		free(costFile);
		if(isProb){
			setupValid = false;
			setupErr.append("Problem parsing position dependent file ");
			setupErr.append(pargs->costFile);
			setupErr.append(".\n");
			return;
		}
	//load in the quality mangle
		if(pargs->qualmFile){
			uintptr_t qualmFileLen;
			char* qualmFile = readEntireFile(pargs->qualmFile, &qualmFileLen);
			if(qualmFile == 0){
				setupValid = false;
				setupErr.append("Problem reading auality mangle file ");
				setupErr.append(pargs->qualmFile);
				setupErr.append(".\n");
				return;
			}
			int isProb = parseReadQualityEffectFile(qualmFileLen, qualmFile, &allQualMangs);
			free(qualmFile);
			if(isProb){
				setupValid = false;
				setupErr.append("Problem parsing quality mangle file ");
				setupErr.append(pargs->qualmFile);
				setupErr.append(".\n");
				return;
			}
		}
}

ProblemLoadSetup::~ProblemLoadSetup(){
	std::map<std::string,PositionDependentAlignCostKDNode*>::iterator allRCIt;
	for(allRCIt = allRegCosts.begin(); allRCIt != allRegCosts.end(); allRCIt++){
		freePositionDependentCostInformation(allRCIt->second);
	}
	std::map<std::string,PositionDependentQualityChangeSet*>::iterator allQMIt;
	for(allQMIt = allQualMangs.begin(); allQMIt != allQualMangs.end(); allQMIt++){
		freePositionDependentQualityChangeSet(allQMIt->second);
	}
}

SAMFileSet::SAMFileSet(std::vector<const char*>* fileNames, std::ostream* toDump){
	//open the files
	for(unsigned fi = 0; fi<fileNames->size(); fi++){
		const char* curFN = (*fileNames)[fi];
		FILE* readFil = (strcmp(curFN, "-")==0) ? stdin : fopen(curFN, "rb");
		if(readFil == 0){
			std::cerr << "Problem reading SAM file " << curFN << std::endl;
			continue;
		}
		LiveSAMFileReader* samFil = wrapSAMFile(readFil);
		openFiles.push_back(samFil);
	}
	//dump the header information of the first file, skip it for the others
	bool isFirstF = true;
	unsigned fi = 0;
	while(fi < openFiles.size()){
		entryWaiting.push_back(true);
		LiveSAMFileReader* samFil = openFiles[fi];
		
		moreHeaderTarget:
			if(readNextSAMEntry(samFil)){
				if(samFil->lastReadHeader){
					if(isFirstF){
						(*toDump) << samFil->lastReadHeader << std::endl;
					}
					goto moreHeaderTarget;
				}
				else{
					isFirstF = false;
					fi++;
				}
			}
			else{
				entryWaiting.erase(entryWaiting.begin() + fi);
				closeSAMFile(openFiles[fi]);
				openFiles.erase(openFiles.begin() + fi);
			}
	}
	//add this program's identifier as a header
		(*toDump) << "@PG\tID:ProDerAl\tVN:0" << std::endl;
}

SAMFileSet::~SAMFileSet(){
	for(unsigned fi = 0; fi<openFiles.size(); fi++){
		closeSAMFile(openFiles[fi]);
	}
}

LiveSAMFileReader* SAMFileSet::getNextEntry(){
	tailRecursionTarget:
	
	if(entryWaiting.size() == 0){
		return 0;
	}
	if(entryWaiting[0]){
		entryWaiting[0] = 0;
		return openFiles[0];
	}
	if(readNextSAMEntry(openFiles[0])){
		return openFiles[0];
	}
	entryWaiting.erase(entryWaiting.begin());
	closeSAMFile(openFiles[0]);
	openFiles.erase(openFiles.begin());
	goto tailRecursionTarget;
}

ProderalPreload::ProderalPreload(int argc, char** argv){
	pargs = 0;
	refd = 0;
	allFs = 0;
	isValid = false;
	//parse the arguments
		pargs = (ProderalCLIArgs*)malloc(sizeof(ProderalCLIArgs));
		std::vector<const char*> allInFiles;
		if(parseArguments(argc, argv, pargs, &allInFiles)){
			return;
		}
		if(!pargs->needRun){
			return;
		}
	//load in the basic information
		refd = new ProblemLoadSetup(pargs);
		if(!refd->setupValid){
			std::cerr << refd->setupErr << std::endl;
			return;
		}
	//set up the cache: (ref, remap_spos, remap_epos, seq)->(score[], spoint[], ainds[][], binds[][])
		//TODO
	//open all the files
		allFs = new SAMFileSet(&allInFiles, &(std::cout));
	isValid = true;
}

ProderalPreload::~ProderalPreload(){
	if(pargs){
		free(pargs);
	}
	if(refd){
		delete(refd);
	}
	if(allFs){
		delete(allFs);
	}
	for(std::map< std::string, WaitingForPairSet* >::iterator curIt = waitingPairs.begin(); curIt != waitingPairs.end(); curIt++){
		delete(curIt->second);
	}
}
