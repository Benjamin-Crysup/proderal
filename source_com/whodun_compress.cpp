#include "whodun_compress.h"

#include <string>
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
	myComp->compData.clear();
	if(append && fileExists(annotFN)){
		intptr_t annotLen = getFileSize(annotFN);
		if(annotLen < 0){std::string errMess("Problem examining annotation file "); errMess.append(annotFN); throw std::runtime_error(errMess);}
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

void BlockCompOutStream::flush(){
	if(myComp->theData.size()){
		dumpCompData();
	}
}

void BlockCompOutStream::writeByte(int toW){
	myComp->theData.push_back(toW);
	if(myComp->theData.size() > chunkSize){
		dumpCompData();
	}
}

void BlockCompOutStream::writeBytes(const char* toW, uintptr_t numW){
	const char* curFoc = toW;
	uintptr_t addOutstand = numW;
	while((myComp->theData.size() + addOutstand) > chunkSize){
		uintptr_t numNeedA = chunkSize - myComp->theData.size();
		myComp->theData.insert(myComp->theData.end(), curFoc, curFoc + numNeedA);
		dumpCompData();
		curFoc += numNeedA;
		addOutstand -= numNeedA;
	}
	myComp->theData.insert(myComp->theData.end(), curFoc, curFoc + addOutstand);
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
	if(numBlocksT < 0){std::string errMess("Problem examining annotation file "); errMess.append(annotFN); throw std::runtime_error(errMess);}
	numBlocks = numBlocksT / BLOCKCOMP_ANNOT_ENTLEN;
	numLastLine = 0;
	lastLineBI0 = 0;
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
	myComp->compData.clear();
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

uintptr_t BlockCompInStream::readBytes(char* toR, uintptr_t numR){
	uintptr_t totRead = 0;
	char* nextR = toR;
	uintptr_t leftR = numR;
	
	tailRecurTgt:
	uintptr_t numBuff = myComp->theData.size() - nextReadI;
	//get the remainder and return
	if(numBuff >= leftR){
		memcpy(nextR, &(myComp->theData[nextReadI]), leftR);
		totRead += leftR;
		nextReadI += leftR;
		return totRead;
	}
	//get as much as you can
	memcpy(nextR, &(myComp->theData[nextReadI]), numBuff);
	nextReadI += numBuff;
	totRead += numBuff;
	nextR += numBuff;
	leftR -= numBuff;
	//and read a single byte
	int nextBt = readByte();
	if(nextBt < 0){ return totRead; }
	//and add to the thing
	*nextR = nextBt;
	totRead++;
	nextR++;
	leftR--;
	//and retry
	goto tailRecurTgt;
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
		if(fseekPointer(annotF, BLOCKCOMP_ANNOT_ENTLEN*(lastLineBI0 + winBI + 1), SEEK_SET)){throw std::runtime_error("Problem seeking annotation file.");}
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
	lastLineBI0 = fromBlock;
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

/**Multithread stuff for block compression.*/
class MultithreadBlockCompOutStreamUniform{
public:
	/**Set up an empty uniform.*/
	MultithreadBlockCompOutStreamUniform();
	/**Clean up any data.*/
	~MultithreadBlockCompOutStreamUniform();
	/**The compression method.*/
	CompressionMethod* compMeth;
	/**If it has, the ID to wait on.*/
	uintptr_t threadID;
	/**Whether this task has been waited on.*/
	int hasWait;
	/**The threads to use for compression.*/
	ThreadPool* compThreads;
	/**The place to get data from (used for fill tasks).*/
	const char* insertFrom;
	/**The amount of data to fill with.*/
	uintptr_t insertNum;
};

MultithreadBlockCompOutStreamUniform::MultithreadBlockCompOutStreamUniform(){
	compMeth = 0;
	hasWait = 1;
	compThreads = 0;
}

MultithreadBlockCompOutStreamUniform::~MultithreadBlockCompOutStreamUniform(){
	if(!hasWait){
		compThreads->joinTask(threadID);
		hasWait = 1;
	}
	if(compMeth){ delete(compMeth); }
}

/**Fill thing with data.*/
void multithreadBlockCompOutFill(void* theUni){
	MultithreadBlockCompOutStreamUniform* myU = (MultithreadBlockCompOutStreamUniform*)theUni;
	myU->compMeth->theData.insert(myU->compMeth->theData.end(), myU->insertFrom, myU->insertFrom + myU->insertNum);
}

/**Compress a thing.*/
void multithreadBlockCompOutCompress(void* theUni){
	MultithreadBlockCompOutStreamUniform* myU = (MultithreadBlockCompOutStreamUniform*)theUni;
	myU->compMeth->compressData();
}

MultithreadBlockCompOutStream::MultithreadBlockCompOutStream(int append, uintptr_t blockSize, const char* mainFN, const char* annotFN, CompressionMethod* compMeth, int numThreads, ThreadPool* useThreads){
	chunkSize = blockSize;
	if(append && fileExists(annotFN)){
		intptr_t annotLen = getFileSize(annotFN);
		if(annotLen < 0){std::string errMess("Problem examining annotation file "); errMess.append(annotFN); throw std::runtime_error(errMess);}
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
	totalWrite = preCompBS;
	compThreads = useThreads;
	threadUnis.resize(numThreads);
	for(uintptr_t i = 0; i<threadUnis.size(); i++){
		threadUnis[i].compMeth = compMeth->clone();
		threadUnis[i].compThreads = compThreads;
		waitingUnis.push_back(&(threadUnis[i]));
	}
	fillingTmp = 0;
}

MultithreadBlockCompOutStream::~MultithreadBlockCompOutStream(){
	flush();
	while(compressingUnis.size()){
		getOpenUniform();
	}
	fclose(mainF);
	fclose(annotF);
}

void MultithreadBlockCompOutStream::writeByte(int toW){
	totalWrite++;
	MultithreadBlockCompOutStreamUniform* curUni = getOpenUniform();
	curUni->compMeth->theData.push_back(toW);
	if(curUni->compMeth->theData.size() >= chunkSize){
		curUni->threadID = compThreads->addTask(multithreadBlockCompOutCompress, curUni);
		curUni->hasWait = 0;
		compressingUnis.push_back(curUni);
	}
	else{
		fillingTmp = curUni;
	}
}

void MultithreadBlockCompOutStream::writeBytes(const char* toW, uintptr_t numW){
	const char* leftW = toW;
	uintptr_t leftN = numW;
	//set up the fills
	while(leftN){
		MultithreadBlockCompOutStreamUniform* curUni = getOpenUniform();
		uintptr_t numPosAdd = chunkSize - curUni->compMeth->theData.size();
		if(numPosAdd > leftN){ numPosAdd = leftN; }
		uintptr_t endSize = curUni->compMeth->theData.size() + numPosAdd;
		curUni->insertFrom = leftW;
		curUni->insertNum = numPosAdd;
		curUni->threadID = compThreads->addTask(multithreadBlockCompOutFill, curUni);
		curUni->hasWait = 0;
		if(endSize == chunkSize){
			fillingFull.push_back(curUni);
		}
		else{
			//leftN will be zero, so this is fine
			fillingTmp = curUni;
		}
		leftW += numPosAdd;
		leftN -= numPosAdd;
	}
	//wait on the fills (pushing fulls to compression)
	while(fillingFull.size()){
		MultithreadBlockCompOutStreamUniform* curUni = fillingFull[0];
		fillingFull.pop_front();
		compThreads->joinTask(curUni->threadID);
		curUni->threadID = compThreads->addTask(multithreadBlockCompOutCompress, curUni);
		curUni->hasWait = 0;
		compressingUnis.push_back(curUni);
	}
	if(fillingTmp){
		compThreads->joinTask(fillingTmp->threadID);
		fillingTmp->hasWait = 1;
	}
	//note the change
	totalWrite += numW;
}

void MultithreadBlockCompOutStream::flush(){
	if(fillingTmp){
		fillingTmp->threadID = compThreads->addTask(multithreadBlockCompOutCompress, fillingTmp);
		fillingTmp->hasWait = 0;
		compressingUnis.push_back(fillingTmp);
		fillingTmp = 0;
	}
}

uintptr_t MultithreadBlockCompOutStream::tell(){
	return totalWrite;
}

MultithreadBlockCompOutStreamUniform* MultithreadBlockCompOutStream::getOpenUniform(){
	tailRecurTgt:
	MultithreadBlockCompOutStreamUniform* curUni;
	if(fillingTmp){
		curUni = fillingTmp;
		fillingTmp = 0;
		return curUni;
	}
	if(waitingUnis.size()){
		curUni = waitingUnis[waitingUnis.size()-1];
		waitingUnis.pop_back();
		return curUni;
	}
	if(compressingUnis.size()){
		curUni = compressingUnis[0];
		compressingUnis.pop_front();
		compThreads->joinTask(curUni->threadID);
		curUni->hasWait = 1;
		//dump to file
		//write the data
		uintptr_t origLen = curUni->compMeth->theData.size();
		uintptr_t compLen = curUni->compMeth->compData.size();
		if(compLen){
			if(fwrite(&(curUni->compMeth->compData[0]), 1, compLen, mainF) != compLen){
				throw std::runtime_error("Problem writing compressed data.");
			}
		}
		//write out an annotation
		char annotBuff[BLOCKCOMP_ANNOT_ENTLEN];
		nat2be64(preCompBS, annotBuff);
		nat2be64(postCompBS, annotBuff+8);
		nat2be64(origLen, annotBuff+16);
		nat2be64(compLen, annotBuff+24);
		if(fwrite(annotBuff, 1, BLOCKCOMP_ANNOT_ENTLEN, annotF) != BLOCKCOMP_ANNOT_ENTLEN){
			throw std::runtime_error("Problem writing annotations.");
		}
		//and prepare for the next round
		preCompBS += origLen;
		postCompBS += compLen;
		curUni->compMeth->theData.clear();
		return curUni;
	}
	//everything is filling: wait on all and start compressing
	while(fillingFull.size()){
		curUni = fillingFull[0];
		fillingFull.pop_front();
		compThreads->joinTask(curUni->threadID);
		curUni->threadID = compThreads->addTask(multithreadBlockCompOutCompress, curUni);
		curUni->hasWait = 0;
		compressingUnis.push_back(curUni);
	}
	goto tailRecurTgt;
	return 0;
}

/**Multithread stuff for block decompression.*/
class MultithreadBlockCompInStreamUniform{
public:
	/**Set up an empty uniform.*/
	MultithreadBlockCompInStreamUniform();
	/**Clean up any data.*/
	~MultithreadBlockCompInStreamUniform();
	/**The compression method.*/
	CompressionMethod* compMeth;
	/**The ID to wait on.*/
	uintptr_t threadID;
	
	/**The place to put the prefix of the data.*/
	char* dumpToA;
	/**The amount of prefix data to dump.*/
	uintptr_t numDumpA;
	/**The place to put the suffix of the data.*/
	char* dumpToB;
	/**The amount of suffix data to dump.*/
	uintptr_t numDumpB;
	
	/**The place to copy from: for memcpy jobs.*/
	const char* copyFrom;
};

MultithreadBlockCompInStreamUniform::MultithreadBlockCompInStreamUniform(){
	compMeth = 0;
}

MultithreadBlockCompInStreamUniform::~MultithreadBlockCompInStreamUniform(){
	if(compMeth){ delete(compMeth); }
}

/**Actually perform decompression.*/
void multithreadBlockCompInDecompress(void* myUni){
	MultithreadBlockCompInStreamUniform* rUni = (MultithreadBlockCompInStreamUniform*)myUni;
	rUni->compMeth->decompressData();
	const char* curCpyF = &(rUni->compMeth->theData[0]);
	memcpy(rUni->dumpToA, curCpyF, rUni->numDumpA);
	memcpy(rUni->dumpToB, curCpyF + rUni->numDumpA, rUni->numDumpB);
}

/**Actually perform decompression.*/
void multithreadBlockCompInFill(void* myUni){
	MultithreadBlockCompInStreamUniform* rUni = (MultithreadBlockCompInStreamUniform*)myUni;
	const char* curCpyF = rUni->copyFrom;
	memcpy(rUni->dumpToA, curCpyF, rUni->numDumpA);
	memcpy(rUni->dumpToB, curCpyF + rUni->numDumpA, rUni->numDumpB);
}

MultithreadBlockCompInStream::MultithreadBlockCompInStream(const char* mainFN, const char* annotFN, CompressionMethod* compMeth, int numThreads, ThreadPool* useThreads){
	//open up the file proper
	intptr_t numBlocksT = getFileSize(annotFN);
	if(numBlocksT < 0){std::string errMess("Problem examining annotation file "); errMess.append(annotFN); throw std::runtime_error(errMess);}
	numBlocks = numBlocksT / BLOCKCOMP_ANNOT_ENTLEN;
	mainF = fopen(mainFN, "rb");
	if(mainF == 0){
		throw std::runtime_error("Problem opening main block file.");
	}
	annotF = fopen(annotFN, "rb");
	if(annotF == 0){
		fclose(mainF);
		throw std::runtime_error("Problem opening annotation block file.");
	}
	//set up the initial reads
	numReadBlocks = 0;
	compThreads = useThreads;
	threadUnis.resize(numThreads);
	for(uintptr_t i = 0; i<threadUnis.size(); i++){
		MultithreadBlockCompInStreamUniform* curUni = &(threadUnis[i]);
		curUni->compMeth = compMeth->clone();
	}
	leftover = &leftoverA;
	nextLeftover = 0;
}

MultithreadBlockCompInStream::~MultithreadBlockCompInStream(){
	fclose(mainF);
	fclose(annotF);
}

int MultithreadBlockCompInStream::readByte(){
	if(nextLeftover < leftover->size()){
		int toRet = (*leftover)[nextLeftover];
		nextLeftover++;
		return toRet;
	}
	char rBuff;
	uintptr_t numRead = readBytes(&rBuff, 1);
	if(numRead == 1){
		return rBuff;
	}
	return -1;
}

uintptr_t MultithreadBlockCompInStream::readBytes(char* toR, uintptr_t numR){
	//figure out the prelude/postlude storage (short circuit if enough left over)
	std::vector<char>* prelude = leftover;
	std::vector<char>* postlude = &leftoverA;
	if(postlude == prelude){ postlude = &leftoverB; }
	if(nextLeftover >= prelude->size()){ prelude = 0;}
	else if((prelude->size() - nextLeftover) >= numR){
		memcpy(toR, &((*leftover)[nextLeftover]), numR);
		nextLeftover += numR;
		return numR;
	}
	postlude->clear();
	//start reading
	char* nextR = toR;
	uintptr_t leftR = numR;
	uintptr_t nextOpenI = 0;
	uintptr_t nextHotI = 0;
	#define PREPARE_NEXT_JOB \
		nextOpenI = (nextOpenI + 1) % threadUnis.size();\
		if(nextOpenI == nextHotI){\
			compThreads->joinTask(threadUnis[nextOpenI].threadID);\
			nextHotI = (nextHotI + 1) % threadUnis.size();\
		}
	while(leftR){
		MultithreadBlockCompInStreamUniform* cUni = &(threadUnis[nextOpenI]);
		cUni->dumpToA = nextR;
		if(prelude){
			//setup a memcpy job
			cUni->numDumpA = prelude->size() - nextLeftover;
			cUni->copyFrom = &((*prelude)[nextLeftover]);
			cUni->dumpToB = 0;
			cUni->numDumpB = 0;
			cUni->threadID = compThreads->addTask(multithreadBlockCompInFill, cUni);
			leftR -= cUni->numDumpA;
			nextR += cUni->numDumpA;
			prelude = 0;
			PREPARE_NEXT_JOB
			continue;
		}
		//if no more blocks, stop
		if(numReadBlocks == numBlocks){ break; }
		//load in the annotation for the next block
		char annotBuff[BLOCKCOMP_ANNOT_ENTLEN];
		if(fread(annotBuff, 1, BLOCKCOMP_ANNOT_ENTLEN, annotF) != BLOCKCOMP_ANNOT_ENTLEN){
			throw std::runtime_error("Problem reading annotations.");
		}
		uintptr_t numPre = be2nat64(annotBuff+16);
		uintptr_t numPost = be2nat64(annotBuff+24);
		numReadBlocks++;
		if(numPost==0){ continue; }
		cUni->compMeth->compData.resize(numPost);
		if(fread(&(cUni->compMeth->compData[0]), 1, numPost, mainF) != numPost){throw std::runtime_error("Problem reading data.");}
		//figure out how it splits
		if(numPre > leftR){
			postlude->resize(numPre - leftR);
			cUni->numDumpA = leftR;
			cUni->dumpToB = &((*postlude)[0]);
			cUni->numDumpB = postlude->size();
		}
		else{
			cUni->numDumpA = numPre;
			cUni->dumpToB = 0;
			cUni->numDumpB = 0;
		}
		cUni->threadID = compThreads->addTask(multithreadBlockCompInDecompress, cUni);
		leftR -= cUni->numDumpA;
		nextR += cUni->numDumpA;
		PREPARE_NEXT_JOB
	}
	//run out any open clocks
	while(nextHotI != nextOpenI){
		compThreads->joinTask(threadUnis[nextHotI].threadID);
		nextHotI = (nextHotI + 1) % threadUnis.size();
	}
	//and prepare for the next round
	nextLeftover = 0;
	leftover = postlude;
	return (numR - leftR);
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
void GZipOutStream::writeBytes(const char* toW, uintptr_t numW){
	if(gzwrite(baseFile, toW, numW)!=((int)numW)){
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
uintptr_t GZipInStream::readBytes(char* toR, uintptr_t numR){
	int toRet = gzread(baseFile, toR, numR);
	if(toRet < 0){
		int gzerrcode;
		const char* errMess = gzerror(baseFile, &gzerrcode);
		std::string errRep = "Problem reading file ";
			errRep.append(myName);
			errRep.append(" : ");
			errRep.append(errMess);
		throw std::runtime_error(errRep);
	}
	return toRet;
}

/**The uniform used by threads.*/
class MultithreadGZipOutStreamUniform{
public:
	/**Basic setup.*/
	MultithreadGZipOutStreamUniform();
	/**Basic teardown.*/
	~MultithreadGZipOutStreamUniform();
	/**If it has, the ID to wait on.*/
	uintptr_t threadID;
	/**The data to compress.*/
	std::vector<char> toCompress;
	/**The place to put the compressed bytes.*/
	std::vector<char> compressTo;
	/**Compression stream*/
	z_stream zs;
};

/**Compress a thing.*/
void multithreadGZipOutThreadFunc(void* theUni){
	MultithreadGZipOutStreamUniform* myU = (MultithreadGZipOutStreamUniform*)theUni;
	std::vector<char>* toFill = &(myU->compressTo);
	uintptr_t compLen = myU->toCompress.size();
	toFill->resize(std::max((uintptr_t)64, 2*compLen));
	z_stream* zs = &(myU->zs);
	zs->avail_in = compLen;
	zs->next_in = (Bytef*)(compLen ? &(myU->toCompress[0]) : 0);
	zs->avail_out = toFill->size();
	zs->next_out = (Bytef*)&((*toFill)[0]);
	deflateInit2(zs, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15 | 16, 8, Z_DEFAULT_STRATEGY);
	deflate(zs, Z_FINISH);
	deflateEnd(zs);
	toFill->resize(zs->total_out);
}

MultithreadGZipOutStreamUniform::MultithreadGZipOutStreamUniform(){
	zs.zalloc = Z_NULL;
	zs.zfree = Z_NULL;
	zs.opaque = Z_NULL;
}

MultithreadGZipOutStreamUniform::~MultithreadGZipOutStreamUniform(){
}

#define MTGZIP_BYTES_PER_THREAD 65536

#define MTGZIP_COMMON_SETUP \
	numThread = numThreads;\
	myName = fileName;\
	if(append){\
		baseFile = fopen(fileName, "ab");\
	}\
	else{\
		baseFile = fopen(fileName, "wb");\
	}\
	if(baseFile == 0){\
		throw std::runtime_error("Could not open file " + myName);\
	}\
	threadUnis.resize(numThread);\
	nextTUni = 0;\
	nextOUni = 0;

MultithreadGZipOutStream::MultithreadGZipOutStream(int append, const char* fileName, int numThreads){
	MTGZIP_COMMON_SETUP
	compThreads = new ThreadPool(numThreads);
	killPool = true;
}

MultithreadGZipOutStream::MultithreadGZipOutStream(int append, const char* fileName, int numThreads, ThreadPool* useThreads){
	MTGZIP_COMMON_SETUP
	compThreads = useThreads;
	killPool = false;
}

MultithreadGZipOutStream::~MultithreadGZipOutStream(){
	if(threadUnis[nextTUni].toCompress.size()){
		startCompressing();
	}
	while(nextOUni != nextTUni){
		startDumping();
	}
	fclose(baseFile);
	if(killPool){ delete(compThreads); }
}

void MultithreadGZipOutStream::writeByte(int toW){
	MultithreadGZipOutStreamUniform* curUni = &(threadUnis[nextTUni]);
	curUni->toCompress.push_back(toW);
	if(curUni->toCompress.size() >= MTGZIP_BYTES_PER_THREAD){
		startCompressing();
	}
}

void MultithreadGZipOutStream::writeBytes(const char* toW, uintptr_t numW){
	const char* leftW = toW;
	uintptr_t leftN = numW;
	while(leftN){
		MultithreadGZipOutStreamUniform* curUni = &(threadUnis[nextTUni]);
		uintptr_t numPosAdd = MTGZIP_BYTES_PER_THREAD - curUni->toCompress.size();
		if(numPosAdd > leftN){ numPosAdd = leftN; }
		curUni->toCompress.insert(curUni->toCompress.end(), leftW, leftW + numPosAdd);
		if(curUni->toCompress.size() >= MTGZIP_BYTES_PER_THREAD){
			startCompressing();
		}
		leftW += numPosAdd;
		leftN -= numPosAdd;
	}
}

void MultithreadGZipOutStream::startCompressing(){
	MultithreadGZipOutStreamUniform* curUni = &(threadUnis[nextTUni]);
	curUni->threadID = compThreads->addTask(multithreadGZipOutThreadFunc, curUni);
	nextTUni = (nextTUni + 1) % numThread;
	if(nextTUni == nextOUni){
		startDumping();
	}
}

void MultithreadGZipOutStream::startDumping(){
	MultithreadGZipOutStreamUniform* curUni = &(threadUnis[nextOUni]);
	compThreads->joinTask(curUni->threadID);
	if(curUni->compressTo.size()){
		uintptr_t numW = fwrite(&(curUni->compressTo[0]), 1, curUni->compressTo.size(), baseFile);
		if(numW != curUni->compressTo.size()){
			throw std::runtime_error("Problem writing compressed data.");
		}
	}
	curUni->toCompress.clear();
	nextOUni = (nextOUni + 1) % numThread;
}

RawCompressionMethod::~RawCompressionMethod(){}

void RawCompressionMethod::decompressData(){
	theData = compData;
}

void RawCompressionMethod::compressData(){
	compData = theData;
}

CompressionMethod* RawCompressionMethod::clone(){
	RawCompressionMethod* toRet = new RawCompressionMethod();
	toRet->theData = theData;
	toRet->compData = compData;
	return toRet;
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

CompressionMethod* GZipCompressionMethod::clone(){
	GZipCompressionMethod* toRet = new GZipCompressionMethod();
	toRet->theData = theData;
	toRet->compData = compData;
	return toRet;
}

