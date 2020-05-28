
#include "whodun_bamfa.h"

#include <string>
#include <vector>
#include <string.h>
#include <stdlib.h>
#include <iostream>

#define EXTENSION_GZIP ".gzip"
#define EXTENSION_GZ ".gz"
#define INITMALLOC 256
#define WHITESPACE " \t\r\n"

LiveFastqFileReader* openFastqFile(const char* faqFileName){
	int ffn = strlen(faqFileName);
	int gzipn = strlen(EXTENSION_GZIP);
	int gzn = strlen(EXTENSION_GZ);
	bool isGzip = ((ffn > gzipn) && (strcmp(faqFileName + (ffn - gzipn), EXTENSION_GZIP)==0));
	bool isGz = ((ffn > gzn) && (strcmp(faqFileName + (ffn - gzn), EXTENSION_GZ)==0));
	LiveFastqFileReader* waitRet;
	if(isGzip || isGz){
		gzFile readF = gzopen(faqFileName, "rb");
		if(readF == 0){
			return 0;
		}
		waitRet = (LiveFastqFileReader*)malloc(sizeof(LiveFastqFileReader));
		waitRet->srcFileC = readF;
		waitRet->isComp = true;
	}
	else{
		FILE* readF = fopen(faqFileName, "rb");
		if(readF == 0){
			return 0;
		}
		waitRet = (LiveFastqFileReader*)malloc(sizeof(LiveFastqFileReader));
		waitRet->srcFile = readF;
		waitRet->isComp = false;
	}
	waitRet->lastReadName = (char*)malloc(INITMALLOC);
	waitRet->lastReadSeq = (char*)malloc(INITMALLOC);
	waitRet->lastReadQual = (char*)malloc(INITMALLOC);
	waitRet->nameAlloc = INITMALLOC;
	waitRet->seqAlloc = INITMALLOC;
	waitRet->qualAlloc = INITMALLOC;
	waitRet->lastC = 0;
	return waitRet;
}

LiveFastqFileReader* wrapFastqFile(FILE* faqFile){
	LiveFastqFileReader* waitRet;
	waitRet = (LiveFastqFileReader*)malloc(sizeof(LiveFastqFileReader));
	waitRet->srcFile = faqFile;
	waitRet->isComp = false;
	waitRet->lastReadName = (char*)malloc(INITMALLOC);
	waitRet->lastReadSeq = (char*)malloc(INITMALLOC);
	waitRet->lastReadQual = (char*)malloc(INITMALLOC);
	waitRet->nameAlloc = INITMALLOC;
	waitRet->seqAlloc = INITMALLOC;
	waitRet->qualAlloc = INITMALLOC;
	waitRet->lastC = 0;
	return waitRet;
}

/**
 * Gets the next character from a file (-1 for eof).
 * @param forFile The file to read from.
 * @return The next character.
 */
int readNextFastqChar(LiveFastqFileReader* forFile){
	if(forFile->lastC){
		char toRet = forFile->lastC;
		forFile->lastC = 0;
		return 0x00FF & toRet;
	}
	if(forFile->isComp){
		return gzgetc(forFile->srcFileC);
	}
	else{
		return fgetc(forFile->srcFile);
	}
}

/**
 * Reads the next line from a fastq file.
 * @param forFile The file in question.
 * @param toFill Pointer to the buffer to fill (and reallocate if necessary).
 * @param curAlloc Poihter to the maximum size of the buffer.
 * @param endChar The characters that will end the line.
 * @param eatChar Things that will be eaten.
 * @return The terminating character (-1 for eof).
 */
int readNextFastqLine(LiveFastqFileReader* forFile, char** toFill, uintptr_t* curAlloc, const char* endChar, const char* eatChar){
	uintptr_t curInd = 0;
	uintptr_t curAl = *curAlloc;
	char* curFill = *toFill;
	curFill[curInd] = 0;
	int curRd = readNextFastqChar(forFile);
	while(true){
		if(curInd >= curAl){
			char* fillCpy = (char*)malloc(curAl*2);
			memcpy(fillCpy, curFill, curAl);
			free(curFill);
			*toFill = fillCpy;
			curFill = fillCpy;
			curAl = 2*curAl;
			*curAlloc = curAl;
		}
		curFill[curInd] = 0;
		if(curRd < 0){
			return curRd;
		}
		if(strchr(endChar, curRd)!=0){
			return curRd;
		}
		if(strchr(eatChar, curRd)==0){
			curFill[curInd] = (char)curRd;
			curInd++;
		}
		curRd = readNextFastqChar(forFile);
	}
	return -1;
}

bool readNextFastqEntry(LiveFastqFileReader* forFile){
	int moreRead;
	//read until > or @ found
	int nameMark;
	while(true){
		nameMark = readNextFastqChar(forFile);
		if(nameMark < 0){
			return false;
		}
		if((nameMark == '>') || (nameMark == '@')){
			break;
		}
	}
	//> means fasta, @ means fastq
	if(nameMark == '>'){
		forFile->lastReadSeq[0] = 0;
		forFile->lastReadQual[0] = 0;
		//read the name
		moreRead = readNextFastqLine(forFile, &(forFile->lastReadName), &(forFile->nameAlloc), "\n", "\r");
		if(moreRead < 0){
			return false;
		}
		//read the sequence
		moreRead = readNextFastqLine(forFile, &(forFile->lastReadSeq), &(forFile->seqAlloc), ">", " \t\n\v\f\r");
		if(moreRead >= 0){
			forFile->lastC = (char)moreRead;
		}
		return true;
	}
	else{
		forFile->lastReadSeq[0] = 0;
		forFile->lastReadQual[0] = 0;
		//read the name
		moreRead = readNextFastqLine(forFile, &(forFile->lastReadName), &(forFile->nameAlloc), "\n", "\r");
		if(moreRead < 0){
			return false;
		}
		//read the sequence
		moreRead = readNextFastqLine(forFile, &(forFile->lastReadSeq), &(forFile->seqAlloc), "\n", " \t\v\f\r");
		if(moreRead < 0){
			return false;
		}
		//read the + (dump it into quality)
		moreRead = readNextFastqLine(forFile, &(forFile->lastReadQual), &(forFile->qualAlloc), "\n", " \t\v\f\r");
		if(moreRead < 0){
			return false;
		}
		//read the quality
		moreRead = readNextFastqLine(forFile, &(forFile->lastReadQual), &(forFile->qualAlloc), "\n", " \t\v\f\r");
		if(moreRead < 0){
			return false;
		}
		return true;
	}
}

void closeFastqFile(LiveFastqFileReader* toClose){
	free(toClose->lastReadName);
	free(toClose->lastReadSeq);
	free(toClose->lastReadQual);
	if(toClose->isComp){
		gzclose(toClose->srcFileC);
	}
	else{
		fclose(toClose->srcFile);
	}
	free(toClose);
}

LiveSAMFileReader* openSAMFile(const char* faqFileName){
	int ffn = strlen(faqFileName);
	int gzipn = strlen(EXTENSION_GZIP);
	int gzn = strlen(EXTENSION_GZ);
	bool isGzip = ((ffn > gzipn) && (strcmp(faqFileName + (ffn - gzipn), EXTENSION_GZIP)==0));
	bool isGz = ((ffn > gzn) && (strcmp(faqFileName + (ffn - gzn), EXTENSION_GZ)==0));
	LiveSAMFileReader* waitRet;
	if(isGzip || isGz){
		gzFile readF = gzopen(faqFileName, "rb");
		if(readF == 0){
			return 0;
		}
		waitRet = (LiveSAMFileReader*)malloc(sizeof(LiveSAMFileReader));
		waitRet->srcFileC = readF;
		waitRet->isComp = true;
	}
	else{
		FILE* readF = fopen(faqFileName, "rb");
		if(readF == 0){
			return 0;
		}
		waitRet = (LiveSAMFileReader*)malloc(sizeof(LiveSAMFileReader));
		waitRet->srcFile = readF;
		waitRet->isComp = false;
	}
	waitRet->lastC = 0;
	waitRet->allocSize = 256;
	waitRet->lastAlloc = (char*)malloc(waitRet->allocSize);
	return waitRet;
}

LiveSAMFileReader* wrapSAMFile(FILE* faqFile){
	LiveSAMFileReader* waitRet;
	waitRet = (LiveSAMFileReader*)malloc(sizeof(LiveSAMFileReader));
	waitRet->srcFile = faqFile;
	waitRet->isComp = false;
	waitRet->lastC = 0;
	waitRet->allocSize = 256;
	waitRet->lastAlloc = (char*)malloc(waitRet->allocSize);
	return waitRet;
}

/**
 * Gets the next character from a file (-1 for eof).
 * @param forFile The file to read from.
 * @return The next character.
 */
int readNextSAMChar(LiveSAMFileReader* forFile){
	if(forFile->lastC){
		char toRet = forFile->lastC;
		forFile->lastC = 0;
		return 0x00FF & toRet;
	}
	if(forFile->isComp){
		return gzgetc(forFile->srcFileC);
	}
	else{
		return fgetc(forFile->srcFile);
	}
}

bool readNextSAMEntry(LiveSAMFileReader* forFile){
	//read to a character
		int curChar;
		curChar = readNextSAMChar(forFile);
			if(curChar < 0){ return false; }
		while(strchr(WHITESPACE, curChar)){
			curChar = readNextSAMChar(forFile);
			if(curChar < 0){ return false; }
		}
		int firstChar = curChar;
	//read to newline
		std::string curStr;
		curStr.push_back(curChar);
		//read to newline,
		curChar = readNextSAMChar(forFile);
		while(curChar >= 0){
			if((curChar == '\r') || (curChar == '\n')){
				break;
			}
			else{
				curStr.push_back(curChar);
				curChar = readNextSAMChar(forFile);
			}
		}
	//manage allocation
		if(forFile->allocSize < (curStr.size()+1)){
			forFile->allocSize = (curStr.size()+1);
			free(forFile->lastAlloc);
			forFile->lastAlloc = (char*)malloc(forFile->allocSize);
		}
		strcpy(forFile->lastAlloc, curStr.c_str());
	//if it is an apetail, is header, otherwise alignment
		if(firstChar == '@'){
			forFile->lastReadHeader = forFile->lastAlloc;
			forFile->lastAlignQName = 0;
		}
		else{
			forFile->lastReadHeader = 0;
			char* nextFoc;
			char* curFoc = forFile->lastAlloc;
			bool hitEOS = false;
			#define SAMSEEKSTRING(resName) \
				if(hitEOS){return false;}\
				curFoc = curFoc + strspn(curFoc, WHITESPACE);\
				if(!*curFoc){ return false; }\
				forFile->resName = curFoc;\
				nextFoc = curFoc + strcspn(curFoc, WHITESPACE);\
				if(*nextFoc == 0){ hitEOS = true; }\
				*nextFoc = 0; curFoc = nextFoc + 1;
			#define SAMSEEKINT(resName) \
				if(hitEOS){return false;}\
				curFoc = curFoc + strspn(curFoc, WHITESPACE);\
				if(!*curFoc){ return false; }\
				forFile->resName = atol(curFoc);\
				nextFoc = curFoc + strcspn(curFoc, WHITESPACE);\
				if(*nextFoc == 0){ hitEOS = true; }\
				*nextFoc = 0; curFoc = nextFoc + 1;
			SAMSEEKSTRING(lastAlignQName)
			SAMSEEKINT(lastAlignFlag)
			SAMSEEKSTRING(lastAlignRName)
			SAMSEEKINT(lastAlignPos)
				forFile->lastAlignPos--;
			SAMSEEKINT(lastAlignMapq)
			SAMSEEKSTRING(lastAlignCIGAR)
			SAMSEEKSTRING(lastAlignRNext)
			SAMSEEKINT(lastAlignPNext)
				forFile->lastAlignPNext--;
			SAMSEEKINT(lastAlignTLen)
			SAMSEEKSTRING(lastAlignSeq)
			SAMSEEKSTRING(lastAlignQual)
			if(hitEOS){ curFoc--; }
			forFile->lastAlignExtra = curFoc;
		}
	return true;
}

void closeSAMFile(LiveSAMFileReader* toClose){
	free(toClose->lastAlloc);
	if(toClose->isComp){
		gzclose(toClose->srcFileC);
	}
	else{
		fclose(toClose->srcFile);
	}
	free(toClose);
}

LiveTSVFileReader* openTSVFile(const char* faqFileName){
	int ffn = strlen(faqFileName);
	int gzipn = strlen(EXTENSION_GZIP);
	int gzn = strlen(EXTENSION_GZ);
	bool isGzip = ((ffn > gzipn) && (strcmp(faqFileName + (ffn - gzipn), EXTENSION_GZIP)==0));
	bool isGz = ((ffn > gzn) && (strcmp(faqFileName + (ffn - gzn), EXTENSION_GZ)==0));
	LiveTSVFileReader* waitRet;
	if(isGzip || isGz){
		gzFile readF = gzopen(faqFileName, "rb");
		if(readF == 0){
			return 0;
		}
		waitRet = (LiveTSVFileReader*)malloc(sizeof(LiveTSVFileReader));
		waitRet->srcFileC = readF;
		waitRet->isComp = true;
	}
	else{
		FILE* readF = fopen(faqFileName, "rb");
		if(readF == 0){
			return 0;
		}
		waitRet = (LiveTSVFileReader*)malloc(sizeof(LiveTSVFileReader));
		waitRet->srcFile = readF;
		waitRet->isComp = false;
	}
	waitRet->lastLine = (char*)malloc(INITMALLOC);
	waitRet->lineAlloc = INITMALLOC;
	waitRet->colStarts = (char**)malloc(INITMALLOC * sizeof(char*));
	waitRet->colEnds = (char**)malloc(INITMALLOC * sizeof(char*));
	waitRet->colAlloc = INITMALLOC;
	return waitRet;
}

LiveTSVFileReader* wrapTSVFile(FILE* faqFile){
	LiveTSVFileReader* waitRet;
	waitRet = (LiveTSVFileReader*)malloc(sizeof(LiveTSVFileReader));
	waitRet->srcFile = faqFile;
	waitRet->isComp = false;
	waitRet->lastLine = (char*)malloc(INITMALLOC);
	waitRet->lineAlloc = INITMALLOC;
	waitRet->colStarts = (char**)malloc(INITMALLOC * sizeof(char*));
	waitRet->colEnds = (char**)malloc(INITMALLOC * sizeof(char*));
	waitRet->colAlloc = INITMALLOC;
	return waitRet;
}

bool readNextTSVEntry(LiveTSVFileReader* forFile){
	//read in a line
	bool hitNL = false;
	int curRCI = 0;
	while(!hitNL){
		if((curRCI+1) >= forFile->lineAlloc){
			forFile->lineAlloc = 2*(forFile->lineAlloc);
			forFile->lastLine = (char*)realloc(forFile->lastLine, forFile->lineAlloc);
		}
		int curC = (forFile->isComp ? gzgetc(forFile->srcFileC) : fgetc(forFile->srcFile));
		if(curC < 0){
			hitNL = true;
			forFile->lastLine[curRCI] = 0;
			if(curRCI == 0){
				return 0;
			}
		}
		else{
			forFile->lastLine[curRCI] = curC;
			curRCI++;
			if(curC == '\n'){
				hitNL = true;
				forFile->lastLine[curRCI] = 0;
			}
		}
	}
	//find the tabs in that line
	int curTI = 0;
	char* curFoc = forFile->lastLine;
	char* lineEnd = curFoc + strlen(curFoc);
	while(curFoc != lineEnd){
		if(curTI >= forFile->colAlloc){
			forFile->colAlloc = 2*(forFile->colAlloc);
			forFile->colStarts = (char**)realloc(forFile->colStarts, forFile->colAlloc * sizeof(char*));
			forFile->colEnds = (char**)realloc(forFile->colEnds, forFile->colAlloc * sizeof(char*));
		}
		forFile->colStarts[curTI] = curFoc;
		char* toTab = strchr(curFoc, '\t');
		int addNext = 1;
		if(toTab == 0){
			toTab = curFoc + strlen(curFoc);
			addNext = 0;
		}
		forFile->colEnds[curTI] = toTab;
		curFoc = toTab + addNext;
		curTI++;
	}
	forFile->numColumns = curTI;
	return 1;
}

void closeTSVFile(LiveTSVFileReader* toClose){
	free(toClose->lastLine);
	free(toClose->colStarts);
	free(toClose->colEnds);
	if(toClose->isComp){
		gzclose(toClose->srcFileC);
	}
	else{
		fclose(toClose->srcFile);
	}
	free(toClose);
}

BedFileContents* readBedFile(LiveTSVFileReader* toWork){
	std::vector<uintptr_t> chromStarts;
	std::string allChrom;
	std::vector<uintptr_t> starts;
	std::vector<uintptr_t> ends;
	std::vector<uintptr_t> extStarts;
	std::string allExt;
	while(readNextTSVEntry(toWork)){
		if(toWork->lastLine[0] == '#'){
			continue;
		}
		if(toWork->numColumns < 3){
			continue;
		}
		std::string curChr(toWork->colStarts[0], toWork->colEnds[0]);
		uintptr_t curStart = atol(toWork->colStarts[1]);
		uintptr_t curEnd = atol(toWork->colStarts[2]);
		std::string curExt(toWork->colEnds[2]);
		chromStarts.push_back(allChrom.size());
		allChrom.append(curChr);
		allChrom.push_back(0);
		starts.push_back(curStart);
		ends.push_back(curEnd);
		extStarts.push_back(allExt.size());
		allExt.append(curExt);
		allExt.push_back(0);
	}
	closeTSVFile(toWork);
	BedFileContents* toRet = (BedFileContents*)malloc(sizeof(BedFileContents));
	uintptr_t numEnt = chromStarts.size();
	toRet->numEnts = numEnt;
	toRet->chroms = (char**)malloc(numEnt*sizeof(char*) + allChrom.size());
	toRet->starts = (uintptr_t*)malloc(numEnt * sizeof(uintptr_t));
	toRet->ends = (uintptr_t*)malloc(numEnt * sizeof(uintptr_t));
	toRet->extras = (char**)malloc(numEnt*sizeof(char*) + allExt.size());
	char* chromStrS = (char*)(toRet->chroms + numEnt);
	char* extStrS = (char*)(toRet->extras + numEnt);
	for(uintptr_t i = 0; i<numEnt; i++){
		toRet->chroms[i] = chromStrS + chromStarts[i];
		toRet->extras[i] = extStrS + extStarts[i];
		toRet->starts[i] = starts[i];
		toRet->ends[i] = ends[i];
	}
	memcpy(chromStrS, allChrom.c_str(), allChrom.size());
	memcpy(extStrS, allExt.c_str(), allExt.size());
	return toRet;
}

void freeBedFile(BedFileContents* toKill){
	free(toKill->chroms);
	free(toKill->starts);
	free(toKill->ends);
	free(toKill->extras);
	free(toKill);
}
