#include "whodun_parse_seq.h"

#include <string.h>
#include <algorithm>

#include "whodun_compress.h"
#include "whodun_stringext.h"

#define WHITESPACE " \t\r"
#define NLWHITESPACE " \t\r\n"
#define WHITESPACELEN 3
#define NLWHITESPACELEN 4

double fastaPhredToLog10Prob(unsigned char toConv){
	return (toConv - 33)/-10.0;
}

unsigned char fastaLog10ProbToPhred(double toConv){
	double curConv = (toConv * -10.0) + 33;
	if(curConv < 33){ return 33; }
	if(curConv > 126){ return 126; }
	return (unsigned char)curConv;
}

void fastaPhredsToLog10Prob(uintptr_t numConv, const unsigned char* toConv, double* toStore){
	for(uintptr_t i = 0; i<numConv; i++){
		toStore[i] = fastaPhredToLog10Prob(toConv[i]);
	}
}

void fastaLog10ProbsToPhred(uintptr_t numConv, const double* toConv, unsigned char* toStore){
	for(uintptr_t i = 0; i<numConv; i++){
		toStore[i] = fastaLog10ProbToPhred(toConv[i]);
	}
}

void sequenceReverseCompliment(uintptr_t revLen, char* toRev, double* toRevQ){
	std::reverse(toRev, toRev+revLen);
	if(toRevQ){
		std::reverse(toRevQ, toRevQ+revLen);
	}
	char compArr[256];
	for(uintptr_t i = 0; i<256; i++){ compArr[i] = i; }
		compArr[0x00FF & 'A'] = 'T';
		compArr[0x00FF & 'T'] = 'A';
		compArr[0x00FF & 'C'] = 'G';
		compArr[0x00FF & 'G'] = 'C';
	for(uintptr_t i = 0; i<revLen; i++){
		toRev[i] = compArr[0x00FF & toRev[i]];
	}
}

SequenceReader::SequenceReader(){
	lastReadShortNameLen = 0;
	lastReadNameLen = 0;
	lastReadSeqLen = 0;
	lastReadHaveQual = 0;
	lastReadName = 0;
	lastReadSeq = 0;
	lastReadQual = 0;
	recordCount = 0;
}

SequenceReader::~SequenceReader(){}

FastAQSequenceReader::FastAQSequenceReader(InStream* mainFrom){
	theStr = mainFrom;
	nameStore.push_back(1);
	seqStore.push_back(1);
	qualStore.push_back(1);
	readBuffO = 0;
	readBuffS = 0;
}

FastAQSequenceReader::~FastAQSequenceReader(){
}

int FastAQSequenceReader::readNextEntry(){
	//get the first character and interpret
		fillBuffer();
		if(readBuffS == 0){ return 0; }
		int curC = readBuff[readBuffO]; readBuffO++; readBuffS--;
		recordCount++;
		if(curC == '@'){
			lastReadHaveQual = 1;
		}
		else if(curC == '>'){
			lastReadHaveQual = 0;
		}
		else{
			throw std::runtime_error("Bad character to start the sequence.");
		}
		nameStore.clear();
		seqStore.clear();
		qualStore.clear();
		tmpQualS.clear();
	//read to the end of the line: everything goes
		while(true){
			const char* eolLoc = (const char*)memchr(readBuff + readBuffO, '\n', readBuffS);
			int numAdd = readBuffS; int numEat = readBuffS;
			if(eolLoc){
				numAdd = eolLoc - (readBuff + readBuffO);
				numEat = numAdd + 1;
			}
			nameStore.insert(nameStore.end(), readBuff + readBuffO, readBuff + readBuffO + numAdd);
			readBuffO += numEat;
			readBuffS -= numEat;
			if(eolLoc){break;}
			fillBuffer();
			if(readBuffS == 0){
				throw std::runtime_error("Unexpected end of file in name.");
			}
		}
		//skip the initial whitespace
		const char* nameBase = &(nameStore[0]);
		const char* nameEnd = nameBase + nameStore.size();
		const char* curNameS = nameBase + memspn(nameBase, nameStore.size(), WHITESPACE, WHITESPACELEN);
		if((uintptr_t)(curNameS - nameBase) == nameStore.size()){
			throw std::runtime_error("Entry missing name.");
		}
		//note the first whitespace in the range
		const char* curNameSE = curNameS + memcspn(curNameS, nameEnd - curNameS, WHITESPACE, WHITESPACELEN);
		//skip any final whitespace
		const char* curNameE = curNameSE;
		const char* namePWSE = curNameE + memspn(curNameE, nameEnd - curNameE, WHITESPACE, WHITESPACELEN);
		while(namePWSE != nameEnd){
			curNameE = namePWSE + memcspn(namePWSE, nameEnd - namePWSE, WHITESPACE, WHITESPACELEN);
			namePWSE = curNameE + memspn(curNameE, nameEnd - curNameE, WHITESPACE, WHITESPACELEN);
		}
		//note the results
		lastReadShortNameLen = curNameSE - curNameS;
		lastReadNameLen = curNameE - curNameS;
		lastReadName = curNameS;
	//what next depends on type
	if(lastReadHaveQual){
		//read sequence to the line
		while(true){
			int wscSpn = memcspn(readBuff + readBuffO, readBuffS, NLWHITESPACE, NLWHITESPACELEN);
			seqStore.insert(seqStore.end(), readBuff + readBuffO, readBuff + readBuffO + wscSpn);
			readBuffO += wscSpn; readBuffS -= wscSpn;
			if(readBuffS && (readBuff[readBuffO] == '\n')){ readBuffO++; readBuffS--; break; }
			int wsSpn = memspn(readBuff + readBuffO, readBuffS, WHITESPACE, WHITESPACELEN);
			readBuffO += wsSpn; readBuffS -= wsSpn;
			fillBuffer();
			if(readBuffS == 0){
				throw std::runtime_error("Unexpected end of file in quality record.");
			}
		}
		//this line better start with a +
		fillBuffer();
		if(readBuffS == 0){
			throw std::runtime_error("Unexpected end of file in quality record.");
		}
		if(readBuff[readBuffO] != '+'){
			throw std::runtime_error("Missing sequence/quality separator (+) in quality record.");
		}
		while(true){
			const char* eolLoc = (const char*)memchr(readBuff + readBuffO, '\n', readBuffS);
			int numEat = eolLoc ? (1+(eolLoc - (readBuff + readBuffO))) : readBuffS;
			readBuffO += numEat;
			readBuffS -= numEat;
			if(eolLoc){break;}
			fillBuffer();
			if(readBuffS == 0){
				throw std::runtime_error("Unexpected end of file in quality record.");
			}
		}
		//read quality to line
		while(true){
			int wscSpn = memcspn(readBuff + readBuffO, readBuffS, NLWHITESPACE, NLWHITESPACELEN);
			tmpQualS.insert(tmpQualS.end(), readBuff + readBuffO, readBuff + readBuffO + wscSpn);
			readBuffO += wscSpn; readBuffS -= wscSpn;
			if(readBuffS && (readBuff[readBuffO] == '\n')){ readBuffO++; readBuffS--; break; }
			int wsSpn = memspn(readBuff + readBuffO, readBuffS, WHITESPACE, WHITESPACELEN);
			readBuffO += wsSpn; readBuffS -= wsSpn;
			fillBuffer();
			if(readBuffS == 0){
				throw std::runtime_error("Unexpected end of file in quality record.");
			}
		}
		if(tmpQualS.size() != seqStore.size()){
			throw std::runtime_error("Length of quality does not match length of sequence.");
		}
		//prepare report
		qualStore.resize(tmpQualS.size());
		fastaPhredsToLog10Prob(tmpQualS.size(), &(tmpQualS[0]), &(qualStore[0]));
		lastReadSeqLen = seqStore.size();
		lastReadSeq = seqStore.c_str();
		lastReadQual = &(qualStore[0]);
	}
	else{
		//read until @,> or eof; skip all whitespace
		bool lastNL = true;
		while(true){
			fillBuffer();
			if(readBuffS == 0){ break; }
			if(lastNL && ((readBuff[readBuffO] == '@') || (readBuff[readBuffO] == '>'))){ break; }
			int wscSpn = memcspn(readBuff + readBuffO, readBuffS, NLWHITESPACE, NLWHITESPACELEN);
			seqStore.insert(seqStore.end(), readBuff + readBuffO, readBuff + readBuffO + wscSpn);
			readBuffO += wscSpn; readBuffS -= wscSpn;
			if(wscSpn){ lastNL = false; }
			int wsSpn = memspn(readBuff + readBuffO, readBuffS, WHITESPACE, WHITESPACELEN);
			readBuffO += wsSpn; readBuffS -= wsSpn;
			if(wsSpn){ lastNL = false; }
			fillBuffer();
			if(readBuffS && (readBuff[readBuffO] == '\n')){ lastNL = true; readBuffO++; readBuffS--; }
		}
		lastReadSeqLen = seqStore.size();
		lastReadSeq = seqStore.c_str();
	}
	return 1;
}
void FastAQSequenceReader::fillBuffer(){
	if(readBuffS){ return; }
	readBuffO = 0;
	readBuffS = theStr->readBytes(readBuff, FASTAQREAD_BUFFER_SIZE);
}

SequenceWriter::SequenceWriter(){
	nextNameLen = 0;
	nextName = 0;
	nextSeqLen = 0;
	nextSeq = 0;
	nextHaveQual = 0;
	nextQual = 0;
}

SequenceWriter::~SequenceWriter(){}

FastAQSequenceWriter::FastAQSequenceWriter(OutStream* mainTo){
	theStr = mainTo;
}
FastAQSequenceWriter::~FastAQSequenceWriter(){
	//not much to do
}
void FastAQSequenceWriter::writeNextEntry(){
	theStr->writeByte(nextHaveQual ? '@' : '>');
	theStr->writeBytes(nextName, nextNameLen);
	theStr->writeByte('\n');
	theStr->writeBytes(nextSeq, nextSeqLen);
	theStr->writeByte('\n');
	if(nextHaveQual){
		theStr->writeByte('+');
		theStr->writeByte('\n');
		tmpQualS.resize(nextSeqLen);
		fastaLog10ProbsToPhred(nextSeqLen, nextQual, &(tmpQualS[0]));
		theStr->writeBytes((char*)(&(tmpQualS[0])), nextSeqLen);
		theStr->writeByte('\n');
	}
}

#define GAIL_INDEX_ENTLEN 40

GailAQSequenceReader::GailAQSequenceReader(BlockCompInStream* toFlit, const char* indFName){
	theStr = toFlit;
	resetInd = -1;
	focusInd = 0;
	intptr_t annotLen = getFileSize(indFName);
	if(annotLen < 0){throw std::runtime_error("Problem examining index file.");}
	if(annotLen % GAIL_INDEX_ENTLEN){throw std::runtime_error("Malformed index file.");}
	numEntries = annotLen / GAIL_INDEX_ENTLEN;
	indF = fopen(indFName, "rb");
	if(indF == 0){ throw std::runtime_error("Could not open index file."); }
}

GailAQSequenceReader::~GailAQSequenceReader(){
	fclose(indF);
}
//TODO quality as double
int GailAQSequenceReader::readNextEntry(){
	bool wasSeek = false;
	if(resetInd >= 0){
		focusInd = resetInd;
		if(focusInd < numEntries){
			if(fseekPointer(indF, GAIL_INDEX_ENTLEN*focusInd, SEEK_SET)){ throw std::runtime_error("Problem seeking index file."); }
			wasSeek = true;
		}
		resetInd = -1;
	}
	if(focusInd >= numEntries){
		return 0;
	}
	char loadBuff[GAIL_INDEX_ENTLEN];
	if(fread(loadBuff, 1, GAIL_INDEX_ENTLEN, indF)!=GAIL_INDEX_ENTLEN){ throw std::runtime_error("Problem reading index file."); }
	uintptr_t nameLoc = be2nat64(loadBuff);
	uintptr_t shortNameLen = be2nat64(loadBuff+8);
	uintptr_t seqLoc = be2nat64(loadBuff+16);
	uintptr_t qualLoc = be2nat64(loadBuff+24);
	uintptr_t haveQual = be2nat64(loadBuff+32);
	if(wasSeek){
		theStr->seek(nameLoc);
	}
	InStream* focStr = theStr;
	nameStore.resize(seqLoc - nameLoc);
		if(focStr->readBytes(&(nameStore[0]), nameStore.size()) != nameStore.size()){ throw std::runtime_error("Problem reading sequence name."); }
		lastReadShortNameLen = shortNameLen;
		if(shortNameLen > nameStore.size()){ throw std::runtime_error("Short name longer than full name."); }
		lastReadNameLen = nameStore.size();
		lastReadName = &(nameStore[0]);
	seqStore.resize(qualLoc - seqLoc);
		if(focStr->readBytes(&(seqStore[0]), seqStore.size()) != seqStore.size()){ throw std::runtime_error("Problem reading sequence."); }
		lastReadSeqLen = seqStore.size();
		lastReadSeq = &(seqStore[0]);
	lastReadHaveQual = haveQual;
	if(haveQual){
		tmpQualS.resize(qualLoc - seqLoc);
		if(focStr->readBytes((char*)&(tmpQualS[0]), tmpQualS.size()) != tmpQualS.size()){ throw std::runtime_error("Problem reading quality."); }
		qualStore.resize(tmpQualS.size());
		fastaPhredsToLog10Prob(tmpQualS.size(), &(tmpQualS[0]), &(qualStore[0]));
		lastReadQual = &(qualStore[0]);
	}
	else{
		qualStore.clear();
	}
	focusInd++;
	return 1;
}

uintptr_t GailAQSequenceReader::getNumEntries(){
	return numEntries;
}

uintptr_t GailAQSequenceReader::getEntryLength(uintptr_t entInd){
	if(!theStr){ throw std::runtime_error("If using a multithreaded gail reader, cannot random access."); }
	if(entInd >= numEntries){ throw std::runtime_error("Bad entry index."); }
	resetInd = focusInd;
	if(fseekPointer(indF, GAIL_INDEX_ENTLEN*entInd, SEEK_SET)){ throw std::runtime_error("Problem seeking index file."); }
	char loadBuff[GAIL_INDEX_ENTLEN];
	if(fread(loadBuff, 1, GAIL_INDEX_ENTLEN, indF)!=GAIL_INDEX_ENTLEN){ throw std::runtime_error("Problem reading index file."); }
	//uintptr_t nameLoc = be2nat64(loadBuff);
	//uintptr_t shortNameLen = be2nat64(loadBuff+8);
	uintptr_t seqLoc = be2nat64(loadBuff+16);
	uintptr_t qualLoc = be2nat64(loadBuff+24);
	//uintptr_t haveQual = be2nat64(loadBuff+32);
	return qualLoc - seqLoc;
}

void GailAQSequenceReader::getEntrySubsequence(uintptr_t entInd, uintptr_t fromBase, uintptr_t toBase){
	if(!theStr){ throw std::runtime_error("If using a multithreaded gail reader, cannot random access."); }
	if(entInd >= numEntries){ throw std::runtime_error("Bad entry index."); }
	resetInd = focusInd;
	if(fseekPointer(indF, GAIL_INDEX_ENTLEN*entInd, SEEK_SET)){ throw std::runtime_error("Problem seeking index file."); }
	char loadBuff[GAIL_INDEX_ENTLEN];
	if(fread(loadBuff, 1, GAIL_INDEX_ENTLEN, indF)!=GAIL_INDEX_ENTLEN){ throw std::runtime_error("Problem reading index file."); }
	uintptr_t nameLoc = be2nat64(loadBuff);
	uintptr_t shortNameLen = be2nat64(loadBuff+8);
	uintptr_t seqLoc = be2nat64(loadBuff+16);
	uintptr_t qualLoc = be2nat64(loadBuff+24);
	uintptr_t haveQual = be2nat64(loadBuff+32);
	if((toBase < fromBase) || (fromBase > (qualLoc - seqLoc))){ throw std::runtime_error("Invalid sequence range."); }
	//read the name
	theStr->seek(nameLoc);
		nameStore.resize(seqLoc - nameLoc);
		if(theStr->readBytes(&(nameStore[0]), nameStore.size()) != nameStore.size()){ throw std::runtime_error("Problem reading sequence name."); }
		lastReadShortNameLen = shortNameLen;
		if(shortNameLen > nameStore.size()){ throw std::runtime_error("Short name longer than full name."); }
		lastReadNameLen = nameStore.size();
		lastReadName = &(nameStore[0]);
	//sequence
	theStr->seek(seqLoc + fromBase);
		seqStore.resize(toBase - fromBase);
		if(theStr->readBytes(&(seqStore[0]), seqStore.size()) != seqStore.size()){ throw std::runtime_error("Problem reading sequence."); }
		lastReadSeqLen = seqStore.size();
		lastReadSeq = &(seqStore[0]);
	//quality
	lastReadHaveQual = haveQual;
	if(haveQual){
		theStr->seek(qualLoc + fromBase);
			tmpQualS.resize(toBase - fromBase);
			if(theStr->readBytes((char*)&(tmpQualS[0]), tmpQualS.size()) != tmpQualS.size()){ throw std::runtime_error("Problem reading quality."); }
			qualStore.resize(tmpQualS.size());
			fastaPhredsToLog10Prob(tmpQualS.size(), &(tmpQualS[0]), &(qualStore[0]));
			lastReadQual = &(qualStore[0]);
	}
	else{
		qualStore.clear();
	}
}

SequentialGailAQSequenceReader::SequentialGailAQSequenceReader(InStream* toFlit, const char* indFName, uintptr_t loadSize, uintptr_t useTCount, ThreadPool* useThreads){
	intptr_t annotLen = getFileSize(indFName);
	if(annotLen < 0){throw std::runtime_error("Problem examining index file.");}
	if(annotLen % GAIL_INDEX_ENTLEN){throw std::runtime_error("Malformed index file.");}
	numEntries = annotLen / GAIL_INDEX_ENTLEN;
	indF = fopen(indFName, "rb");
	if(indF == 0){ throw std::runtime_error("Could not open index file."); }
	bufferSize = loadSize;
	cacheSize = bufferSize;
	cache = (char*)malloc(cacheSize);
	theStr = toFlit;
	handEntries = 0;
	nextReport = 0;
	//TODO
}

SequentialGailAQSequenceReader::~SequentialGailAQSequenceReader(){
	fclose(indF);
	free(cache);
}

int SequentialGailAQSequenceReader::readNextEntry(){
	char loadBuff[GAIL_INDEX_ENTLEN];
	if(5*nextReport >= cacheEntries.size()){
		handEntries += nextReport;
		nextReport = 0;
		cacheEntries.clear();
		if(handEntries >= numEntries){ return 0; }
		//refill the buffer
		uintptr_t loadSize = 0;
		uintptr_t loadEnts = 0;
		while(loadSize < bufferSize){
			if((handEntries + loadEnts) == numEntries){ break; }
			if(fread(loadBuff, 1, GAIL_INDEX_ENTLEN, indF)!=GAIL_INDEX_ENTLEN){ throw std::runtime_error("Problem reading index file."); }
			uintptr_t nameLoc = be2nat64(loadBuff);
			uintptr_t shortNameLen = be2nat64(loadBuff+8);
			uintptr_t seqLoc = be2nat64(loadBuff+16);
			uintptr_t qualLoc = be2nat64(loadBuff+24);
			uintptr_t haveQual = be2nat64(loadBuff+32);
			cacheEntries.push_back(nameLoc);
			cacheEntries.push_back(shortNameLen);
			cacheEntries.push_back(seqLoc);
			cacheEntries.push_back(qualLoc);
			cacheEntries.push_back(haveQual);
			loadSize += ((seqLoc - nameLoc) + (qualLoc - seqLoc) + (haveQual ? (qualLoc - seqLoc) : 0));
			loadEnts++;
		}
		if(loadSize > cacheSize){
			free(cache);
			cacheSize = loadSize;
			cache = (char*)malloc(cacheSize);
		}
		if(theStr->readBytes(cache, loadSize) != loadSize){ throw std::runtime_error("Truncated gail sequence file."); }
		//convert qualities
		//TODO
	}
	uintptr_t baseI = 5*nextReport;
	uintptr_t entOffset = cacheEntries[baseI] - cacheEntries[0];
	lastReadShortNameLen = cacheEntries[baseI + 1];
	lastReadNameLen = cacheEntries[baseI + 2] - cacheEntries[baseI];
	lastReadName = cache + entOffset;
	lastReadSeqLen = cacheEntries[baseI + 3] - cacheEntries[baseI + 2];
	lastReadSeq = lastReadName + lastReadNameLen;
	//TODO fix this
	lastReadHaveQual = 0; //= cacheEntries[baseI + 4];
	nextReport++;
	return 1;
}

GailAQSequenceWriter::GailAQSequenceWriter(int append, BlockCompOutStream* toFlit, const char* indFName){
	theStr = toFlit;
	intptr_t annotLen = getFileSize(indFName);
	if((annotLen >= 0) && (annotLen % GAIL_INDEX_ENTLEN)){throw std::runtime_error("Malformed index file.");}
	indF = fopen(indFName, append ? "ab" : "wb");
	if(indF == 0){ throw std::runtime_error("Could not open index file."); }
}

GailAQSequenceWriter::~GailAQSequenceWriter(){
	fclose(indF);
}

void GailAQSequenceWriter::writeNextEntry(){
	char indOutBuff[GAIL_INDEX_ENTLEN];
	nat2be64(theStr->tell(), indOutBuff);
	nat2be64(nextShortNameLen, indOutBuff+8);
	theStr->writeBytes(nextName, nextNameLen);
	nat2be64(theStr->tell(), indOutBuff+16);
	theStr->writeBytes(nextSeq, nextSeqLen);
	nat2be64(theStr->tell(), indOutBuff+24);
	nat2be64(nextHaveQual, indOutBuff+32);
	if(nextHaveQual){
		tmpQualS.resize(nextSeqLen);
		fastaLog10ProbsToPhred(nextSeqLen, nextQual, &(tmpQualS[0]));
		theStr->writeBytes((char*)&(tmpQualS[0]), nextSeqLen);
	}
	theStr->flush();
	if(fwrite(indOutBuff, 1, GAIL_INDEX_ENTLEN, indF)!=GAIL_INDEX_ENTLEN){throw std::runtime_error("Problem writing index file.");}
}

void openSequenceFileRead(const char* fileName, InStream** saveIS, SequenceReader** saveSS){
	if(strcmp(fileName, "-")==0){
		*saveIS = new ConsoleInStream();
		*saveSS = new FastAQSequenceReader(*saveIS);
		return;
	}
	if(strendswith(fileName, ".fasta") || strendswith(fileName, ".fa") || strendswith(fileName, ".fastq") || strendswith(fileName, ".fq")){
		*saveIS = new FileInStream(fileName);
		*saveSS = new FastAQSequenceReader(*saveIS);
		return;
	}
	if(strendswith(fileName, ".fasta.gz") || strendswith(fileName, ".fa.gz") || strendswith(fileName, ".fastq.gz") || strendswith(fileName, ".fq.gz")){
		*saveIS = new GZipInStream(fileName);
		*saveSS = new FastAQSequenceReader(*saveIS);
		return;
	}
	if(strendswith(fileName, ".fasta.gzip") || strendswith(fileName, ".fa.gzip") || strendswith(fileName, ".fastq.gzip") || strendswith(fileName, ".fq.gzip")){
		*saveIS = new GZipInStream(fileName);
		*saveSS = new FastAQSequenceReader(*saveIS);
		return;
	}
	//fasta is the default
	*saveIS = new FileInStream(fileName);
	*saveSS = new FastAQSequenceReader(*saveIS);
}

void openSequenceFileWrite(const char* fileName, OutStream** saveIS, SequenceWriter** saveSS){
	if(strcmp(fileName, "-")==0){
		*saveIS = new ConsoleOutStream();
		*saveSS = new FastAQSequenceWriter(*saveIS);
		return;
	}
	if(strendswith(fileName, ".fasta") || strendswith(fileName, ".fa") || strendswith(fileName, ".fastq") || strendswith(fileName, ".fq")){
		*saveIS = new FileOutStream(0, fileName);
		*saveSS = new FastAQSequenceWriter(*saveIS);
		return;
	}
	//TODO
	//fasta is the default
	*saveIS = new FileOutStream(0, fileName);
	*saveSS = new FastAQSequenceWriter(*saveIS);
}

