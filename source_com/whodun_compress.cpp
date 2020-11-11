#include "whodun_compress.h"

#include <stdlib.h>
#include <stdexcept>
#include <algorithm>

#include "whodun_oshook.h"
#include "whodun_stringext.h"

CompressionMethod::~CompressionMethod(){}

BlockCompOutStream::BlockCompOutStream(int append, uintptr_t blockSize, const char* mainFN, const char* annotFN, CompressionMethod* compMeth){
	chunkSize = blockSize;
	myComp = compMeth;
	myComp->theData.clear();
	if(append && fileExists(annotFN)){
		intptr_t annotLen = getFileSize(annotFN);
		if(annotLen < 0){throw std::runtime_error("Problem examining annotation file.");}
		if(annotLen % BLOCKCOMP_ANNOT_ENTLEN){throw std::runtime_error("Malformed annotation file.");}
		FILE* annotTmp = fopen(annotFN, "rb");
		if(annotTmp == 0){throw std::runtime_error("Problem opening annotation file.");}
		if(fseekPointer(annotTmp, annotLen - BLOCKCOMP_ANNOT_ENTLEN, SEEK_SET)){fclose(annotTmp); throw std::runtime_error("Problem positioning annotation file.");}
		char tmpBuff[BLOCKCOMP_ANNOT_ENTLEN];
		if(fread(tmpBuff, 1, BLOCKCOMP_ANNOT_ENTLEN, annotTmp) != BLOCKCOMP_ANNOT_ENTLEN){fclose(annotTmp); throw std::runtime_error("Problem reading annotation file.");}
		fclose(annotTmp);
		preCompBS = be2nat64(tmpBuff) + be2nat64(tmpBuff + 16);
		postCompBS = be2nat64(tmpBuff+8) + be2nat64(tmpBuff + 24);
		mainF = fopen(mainFN, "ab");
		if(mainF == 0){
			throw std::runtime_error("Problem opening main block file.");
		}
		annotF = fopen(annotFN, "ab");
		if(annotF == 0){
			fclose(mainF);
			throw std::runtime_error("Problem opening annotation block file.");
		}
	}
	else{
		preCompBS = 0;
		postCompBS = 0;
		mainF = fopen(mainFN, "wb");
		if(mainF == 0){
			throw std::runtime_error("Problem opening main block file.");
		}
		annotF = fopen(annotFN, "wb");
		if(annotF == 0){
			fclose(mainF);
			throw std::runtime_error("Problem opening annotation block file.");
		}
	}
}

BlockCompOutStream::~BlockCompOutStream(){
	try{
		if(myComp->theData.size()){
			dumpCompData();
		}
	}
	catch(...){
		fclose(mainF);
		fclose(annotF);
		return;
	}
	fclose(mainF);
	fclose(annotF);
}

void BlockCompOutStream::writeByte(int toW){
	myComp->theData.push_back(toW);
	if(myComp->theData.size() > chunkSize){
		dumpCompData();
	}
}

uintptr_t BlockCompOutStream::tell(){
	return preCompBS + myComp->theData.size();
}

void BlockCompOutStream::dumpCompData(){
	myComp->compressData();
	if(fwrite(&(myComp->compData[0]), 1, myComp->compData.size(), mainF) != myComp->compData.size()){
		throw std::runtime_error("Problem writing data.");
	}
	char annotBuff[BLOCKCOMP_ANNOT_ENTLEN];
	nat2be64(preCompBS, annotBuff);
	nat2be64(postCompBS, annotBuff+8);
	nat2be64(myComp->theData.size(), annotBuff+16);
	nat2be64(myComp->compData.size(), annotBuff+24);
	if(fwrite(annotBuff, 1, BLOCKCOMP_ANNOT_ENTLEN, annotF) != BLOCKCOMP_ANNOT_ENTLEN){
		throw std::runtime_error("Problem writing annotations.");
	}
	preCompBS += myComp->theData.size();
	postCompBS += myComp->compData.size();
	myComp->theData.clear();
	myComp->compData.clear();
}

BlockCompInStream::BlockCompInStream(const char* mainFN, const char* annotFN, CompressionMethod* compMeth){
	myComp = compMeth;
	intptr_t numBlocksT = getFileSize(annotFN);
	if(numBlocksT < 0){throw std::runtime_error("Problem examining annotation file.");}
	numBlocks = numBlocksT / BLOCKCOMP_ANNOT_ENTLEN;
	numLastLine = 0;
	mainF = fopen(mainFN, "rb");
	if(mainF == 0){
		throw std::runtime_error("Problem opening main block file.");
	}
	annotF = fopen(annotFN, "rb");
	if(annotF == 0){
		fclose(mainF);
		throw std::runtime_error("Problem opening annotation block file.");
	}
	nextReadI = 0;
	myComp->theData.clear();
	lastLineBuff = (char*)malloc(BLOCKCOMP_ANNOT_ENTLEN*BLOCKCOMPIN_LASTLINESEEK);
	lastLineAddrs = (uintptr_t*)malloc(sizeof(uintptr_t)*BLOCKCOMP_ANNOT_ENTLEN*BLOCKCOMPIN_LASTLINESEEK);
}

BlockCompInStream::~BlockCompInStream(){
	fclose(mainF);
	fclose(annotF);
	free(lastLineBuff);
	free(lastLineAddrs);
}

int BlockCompInStream::readByte(){
	while(nextReadI >= myComp->theData.size()){
		char annotBuff[BLOCKCOMP_ANNOT_ENTLEN];
		if(fread(annotBuff, 1, BLOCKCOMP_ANNOT_ENTLEN, annotF) != BLOCKCOMP_ANNOT_ENTLEN){
			if(ferror(annotF)){throw std::runtime_error("Problem reading annotations.");}
			return -1;
		}
		uintptr_t numPost = be2nat64(annotBuff+24);
		if(numPost){
			myComp->compData.resize(numPost);
			if(fread(&(myComp->compData[0]), 1, numPost, mainF) != numPost){throw std::runtime_error("Problem reading data.");}
			myComp->decompressData();
			nextReadI = 0;
		}
	}
	int toRet = 0x00FF & myComp->theData[nextReadI];
	nextReadI++;
	return toRet;
}

void BlockCompInStream::seek(uintptr_t toAddr){
	retryWithCachedArena:
	if(numLastLine){
		uintptr_t* winBlk = std::upper_bound(lastLineAddrs, lastLineAddrs + numLastLine, toAddr) - 1;
		intptr_t winBI = winBlk - lastLineAddrs;
		if(winBI < 0){ goto arenaNotCached; }
		char* focEnt = lastLineBuff + BLOCKCOMP_ANNOT_ENTLEN*winBI;
		uintptr_t focLI = be2nat64(focEnt);
		uintptr_t focHI = focLI + be2nat64(focEnt + 16);
		if(toAddr >= focHI){ goto arenaNotCached; }
		uintptr_t blockSAddr = be2nat64(focEnt + 8);
		uintptr_t blockCLen = be2nat64(focEnt + 24);
		if(fseekPointer(mainF, blockSAddr, SEEK_SET)){throw std::runtime_error("Problem seeking data file.");}
		myComp->compData.resize(blockCLen);
		if(fread(&(myComp->compData[0]), 1, blockCLen, mainF) != blockCLen){throw std::runtime_error("Problem reading data.");}
		myComp->decompressData();
		nextReadI = toAddr - focLI;
		return;
	}
	arenaNotCached:
	uintptr_t fromBlock = 0;
	uintptr_t toBlock = numBlocks;
	while((toBlock - fromBlock) > BLOCKCOMPIN_LASTLINESEEK){
		char annotBuff[BLOCKCOMP_ANNOT_ENTLEN];
		uintptr_t midBlock = (fromBlock + toBlock)/2;
		if(fseekPointer(annotF, BLOCKCOMP_ANNOT_ENTLEN*midBlock, SEEK_SET)){throw std::runtime_error("Problem seeking annotation file.");}
		if(fread(annotBuff, 1, BLOCKCOMP_ANNOT_ENTLEN, annotF)!=BLOCKCOMP_ANNOT_ENTLEN){throw std::runtime_error("Problem reading annotation file.");}
		uintptr_t precomLI = be2nat64(annotBuff);
		//uintptr_t precomHI = precomLI + be2nat64(annotBuff+16);
		if(toAddr < precomLI){
			toBlock = midBlock;
		}
		else{
			fromBlock = midBlock;
		}
	}
	uintptr_t numRBlock = toBlock - fromBlock;
	numLastLine = numRBlock;
	if(fseekPointer(annotF, BLOCKCOMP_ANNOT_ENTLEN*fromBlock, SEEK_SET)){throw std::runtime_error("Problem seeking annotation file.");}
	if(fread(lastLineBuff, 1, numRBlock*BLOCKCOMP_ANNOT_ENTLEN, annotF) != (numRBlock*BLOCKCOMP_ANNOT_ENTLEN)){throw std::runtime_error("Problem reading data.");}
	for(uintptr_t i = 0; i<numRBlock; i++){
		lastLineAddrs[i] = be2nat64(lastLineBuff + i*BLOCKCOMP_ANNOT_ENTLEN);
	}
	goto retryWithCachedArena;
}

uintptr_t BlockCompInStream::getUncompressedSize(){
	intptr_t retLoc = ftellPointer(annotF);
	if(fseekPointer(annotF, BLOCKCOMP_ANNOT_ENTLEN * (numBlocks - 1), SEEK_SET)){ throw std::runtime_error("Problem positioning annotation file."); }
	char tmpBuff[BLOCKCOMP_ANNOT_ENTLEN];
	if(fread(tmpBuff, 1, BLOCKCOMP_ANNOT_ENTLEN, annotF) != BLOCKCOMP_ANNOT_ENTLEN){ throw std::runtime_error("Problem reading annotation file.");}
	if(fseekPointer(annotF, retLoc, SEEK_SET)){ throw std::runtime_error("Problem positioning annotation file."); }
	return be2nat64(tmpBuff) + be2nat64(tmpBuff+16);
}

GZipOutStream::GZipOutStream(int append, const char* fileName){
	myName = fileName;
	if(append){
		baseFile = gzopen(fileName, "ab");
	}
	else{
		baseFile = gzopen(fileName, "wb");
	}
	if(baseFile == 0){
		throw std::runtime_error("Could not open file " + myName);
	}
}
GZipOutStream::~GZipOutStream(){
	gzclose(baseFile);
}
void GZipOutStream::writeByte(int toW){
	if(gzputc(baseFile, toW) < 0){
		throw std::runtime_error("Problem writing file " + myName);
	}
}

GZipInStream::GZipInStream(const char* fileName){
	myName = fileName;
	baseFile = gzopen(fileName, "rb");
	if(baseFile == 0){
		throw std::runtime_error("Could not open file " + myName);
	}
}
GZipInStream::~GZipInStream(){
	gzclose(baseFile);
}
int GZipInStream::readByte(){
	int toR = gzgetc(baseFile);
	if(toR < 0){
		int gzerrcode;
		const char* errMess = gzerror(baseFile, &gzerrcode);
		if(gzerrcode && (gzerrcode != Z_STREAM_END)){
			std::string errRep = "Problem reading file ";
				errRep.append(myName);
				errRep.append(" : ");
				errRep.append(errMess);
			throw std::runtime_error(errRep);
		}
	}
	return toR;
}

RawCompressionMethod::~RawCompressionMethod(){}

void RawCompressionMethod::decompressData(){
	theData = compData;
}

void RawCompressionMethod::compressData(){
	compData = theData;
}

GZipCompressionMethod::~GZipCompressionMethod(){}

void GZipCompressionMethod::decompressData(){
	uintptr_t curBuffLen = theData.capacity();
	if(compData.size() > curBuffLen){ curBuffLen = compData.size(); }
	if(1024 > curBuffLen){ curBuffLen = 1024; }
	theData.resize(curBuffLen);
	unsigned long bufEndSStore = curBuffLen;
	int compRes = 0;
	while((compRes = uncompress(((unsigned char*)(&(theData[0]))), &bufEndSStore, ((const unsigned char*)(&(compData[0]))), compData.size())) != Z_OK){
		if((compRes == Z_MEM_ERROR) || (compRes == Z_DATA_ERROR)){
			throw std::runtime_error("Error decompressing gzip data.");
		}
		curBuffLen = curBuffLen << 1;
		theData.resize(curBuffLen);
		bufEndSStore = curBuffLen;
	}
	theData.resize(bufEndSStore);
}

void GZipCompressionMethod::compressData(){
	uintptr_t curBuffLen = compData.capacity();
	if(theData.size() > curBuffLen){ curBuffLen = theData.size(); }
	if(1024 > curBuffLen){ curBuffLen = 1024; }
	compData.resize(curBuffLen);
	unsigned long bufEndSStore = curBuffLen;
	int compRes = 0;
	while((compRes = compress(((unsigned char*)(&(compData[0]))), &bufEndSStore, ((const unsigned char*)(&(theData[0]))), theData.size())) != Z_OK){
		if((compRes == Z_MEM_ERROR) || (compRes == Z_DATA_ERROR)){
			throw std::runtime_error("Error compressing gzip data.");
		}
		curBuffLen = curBuffLen << 1;
		compData.resize(curBuffLen);
		bufEndSStore = curBuffLen;
	}
	compData.resize(bufEndSStore);
}

