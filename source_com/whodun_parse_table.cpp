#include "whodun_parse_table.h"

#include <string.h>
#include <iostream>
#include <stdexcept>

#include "whodun_stringext.h"

TabularReader::TabularReader(){
	numEntries = 0;
	entrySizes = 0;
	curEntries = 0;
	recordCount = 0;
}

TabularReader::~TabularReader(){}

#define TSV_EVENT_CHARS "\n\r\t\\"
#define NUM_TSV_EVENT_CHARS 4

TSVTabularReader::TSVTabularReader(int escapes, InStream* mainFrom){
	theStr = mainFrom;
	readBuffO = 0;
	readBuffS = 0;
	escEnable = escapes;
}
TSVTabularReader::~TSVTabularReader(){}
int TSVTabularReader::readNextEntry(){
	//clear and prepare
		entryLens.clear();
		entryFlat.clear();
		entryHeads.clear();
		headTmp.clear();
		fillBuffer();
		if(readBuffS == 0){ return 0; }
		recordCount++;
		headTmp.push_back(0);
	//read down the line
		while(true){
			int evtcspn = memcspn(readBuff + readBuffO, readBuffS, TSV_EVENT_CHARS, NUM_TSV_EVENT_CHARS);
			entryFlat.insert(entryFlat.end(), readBuff + readBuffO, readBuff + readBuffO + evtcspn);
			readBuffO += evtcspn; readBuffS -= evtcspn;
			fillBuffer();
			if(readBuffS == 0){ headTmp.push_back(entryFlat.size()); goto hitEndOfLine; }
			switch(readBuff[readBuffO]){
				case '\\':
					if(escEnable){
						readBuffO++; readBuffS--;
						fillBuffer();
						if(readBuffS == 0){ std::runtime_error("Incomplete escape sequence."); }
						switch(readBuff[readBuffO]){
							case 'n':
								entryFlat.push_back('\n');
								break;
							case 't':
								entryFlat.push_back('\t');
								break;
							case 'r':
								entryFlat.push_back('\r');
								break;
							case '\\':
								entryFlat.push_back('\\');
								break;
							default:
							{
								std::string errMess("Invalid escape sequence: \\");
								errMess.push_back(readBuff[readBuffO]);
								throw std::runtime_error(errMess);
							}
						}
					}
					else{
						entryFlat.push_back('\\');
					}
					readBuffO++; readBuffS--;
					break;
				case '\r':
					//skip it
					readBuffO++; readBuffS--;
					break;
				case '\t':
					readBuffO++; readBuffS--;
					headTmp.push_back(entryFlat.size());
					break;
				case '\n':
					readBuffO++; readBuffS--;
					headTmp.push_back(entryFlat.size());
					goto hitEndOfLine;
				default:
					;
					//do nothing
			}
		}
		hitEndOfLine:
	//set up the pointers
		const char* baseSPtr = entryFlat.c_str();
		for(uintptr_t i = 1; i<headTmp.size(); i++){
			entryHeads.push_back(baseSPtr + headTmp[i-1]);
		}
	//set up the lengths
		for(uintptr_t i = 1; i<headTmp.size(); i++){
			entryLens.push_back(headTmp[i] - headTmp[i-1]);
		}
	//report
		numEntries = headTmp.size() - 1;
		if(numEntries){
			entrySizes = &(entryLens[0]);
			curEntries = &(entryHeads[0]);
		}
	return 1;
}
void TSVTabularReader::fillBuffer(){
	if(readBuffS){ return; }
	readBuffO = 0;
	readBuffS = theStr->readBytes(readBuff, TSVTABLEREAD_BUFFER_SIZE);
}

TabularWriter::TabularWriter(){
	numEntries = 0;
	entrySizes = 0;
	curEntries = 0;
}

TabularWriter::~TabularWriter(){}

TSVTabularWriter::TSVTabularWriter(int escapes, OutStream* mainTo){
	theStr = mainTo;
	escEnable = escapes;
}
TSVTabularWriter::~TSVTabularWriter(){}
void TSVTabularWriter::writeNextEntry(){
	outTmp.clear();
	for(uintptr_t i = 0; i<numEntries; i++){
		if(i){ outTmp.push_back('\t'); }
		uintptr_t curEntS = entrySizes[i];
		const char* curEntT = curEntries[i];
		const char* curEntE = curEntT + curEntS;
		if(escEnable){
			while(curEntT < curEntE){
				uintptr_t numSimp = memcspn(curEntT, curEntE - curEntT, TSV_EVENT_CHARS, NUM_TSV_EVENT_CHARS);
				outTmp.insert(outTmp.end(), curEntT, curEntT + numSimp);
				curEntT += numSimp;
				if(curEntT < curEntE){
					outTmp.push_back('\\');
					switch(*curEntT){
						case '\t':
							outTmp.push_back('t');
							break;
						case '\r':
							outTmp.push_back('r');
							break;
						case '\n':
							outTmp.push_back('n');
							break;
						case '\\':
							outTmp.push_back('\\');
							break;
						default:
							std::cerr << "Da fuq?!" << std::endl;
					}
					curEntT++;
				}
			}
		}
		else{
			outTmp.insert(outTmp.end(), curEntT, curEntE);
		}
	}
	outTmp.push_back('\n');
	theStr->writeBytes(&(outTmp[0]), outTmp.size());
}

TabularData::TabularData(){}

TabularData::TabularData(TabularReader* toLoad){
	std::vector<std::string> curEntry;
	while(toLoad->readNextEntry() > 0){
		curEntry.clear();
		for(uintptr_t i = 0; i<toLoad->numEntries; i++){
			uintptr_t curEntS = toLoad->entrySizes[i];
			const char* curEntTmp = toLoad->curEntries[i];
			curEntry.push_back(std::string(curEntTmp, curEntTmp+curEntS));
		}
		allEntries.push_back(curEntry);
	}
}

TabularData::~TabularData(){}

void TabularData::dumpData(TabularWriter* dumpTo){
	std::vector<uintptr_t> lenStore;
	std::vector<const char*> txtStore;
	for(uintptr_t i = 0; i<allEntries.size(); i++){
		lenStore.clear();
		txtStore.clear();
		std::vector<std::string>* curEntry = &(allEntries[i]);
		for(uintptr_t j = 0; j<curEntry->size(); j++){
			std::string* curTxt = &((*curEntry)[j]);
			lenStore.push_back(curTxt->size());
			txtStore.push_back(curTxt->c_str());
		}
		dumpTo->numEntries = curEntry->size();
		dumpTo->entrySizes = &(lenStore[0]);
		dumpTo->curEntries = &(txtStore[0]);
		dumpTo->writeNextEntry();
	}
}

#define BCOMPTAB_INDEX_ENTLEN 8

BCompTabularReader::BCompTabularReader(BlockCompInStream* toFlit, const char* indFName){
	theStr = toFlit;
	resetInd = -1;
	focusInd = 0;
	intptr_t annotLen = getFileSize(indFName);
	if(annotLen < 0){throw std::runtime_error("Problem examining index file.");}
	if(annotLen % BCOMPTAB_INDEX_ENTLEN){throw std::runtime_error("Malformed index file.");}
	rowCount = annotLen / BCOMPTAB_INDEX_ENTLEN;
	indF = fopen(indFName, "rb");
	if(indF == 0){ throw std::runtime_error("Could not open index file."); }
}

BCompTabularReader::~BCompTabularReader(){
	fclose(indF);
}

/**
 * Load a tsv entry given all seeking has been done.
 * @param loadFor The thing to load for.
 */
void loadBCompTSVEntry(BCompTabularReader* loadFor){
	char loadBuff[16];
	//get the total entry size and number of entries
		if(loadFor->theStr->readBytes(loadBuff,16) != 16){ throw std::runtime_error("Problem reading data."); }
		uintptr_t totTxtSize = be2nat64(loadBuff);
		uintptr_t numEnts = be2nat64(loadBuff + 8);
	//load the text
		loadFor->entryFlat.resize(totTxtSize+1);
		char* initFoc = &(loadFor->entryFlat[0]);
		if(loadFor->theStr->readBytes(initFoc, totTxtSize)!=totTxtSize){ throw std::runtime_error("Problem reading data."); }
	//and entry lengths
		char* curFoc = initFoc;
		loadFor->entryLens.resize(numEnts+1);
		loadFor->entryHeads.resize(numEnts+1);
		for(uintptr_t i = 0; i<numEnts; i++){
			loadFor->entryHeads[i] = curFoc;
			if(loadFor->theStr->readBytes(loadBuff,8) != 8){ throw std::runtime_error("Problem reading data."); }
			uintptr_t curEntLen = be2nat64(loadBuff);
			loadFor->entryLens[i] = curEntLen;
			curFoc += curEntLen;
			if((curFoc - initFoc) > (intptr_t)totTxtSize){ throw std::runtime_error("Malformed table data."); }
		}
	//update the tsv pointer stuff
		loadFor->numEntries = numEnts;
		loadFor->entrySizes = &(loadFor->entryLens[0]);
		loadFor->curEntries = &(loadFor->entryHeads[0]);
}

int BCompTabularReader::readNextEntry(){
	bool wasSeek = false;
	if(resetInd >= 0){
		focusInd = resetInd;
		if(focusInd < rowCount){
			if(fseekPointer(indF, BCOMPTAB_INDEX_ENTLEN*focusInd, SEEK_SET)){ throw std::runtime_error("Problem seeking index file."); }
			wasSeek = true;
		}
		resetInd = -1;
	}
	if(focusInd >= rowCount){
		return 0;
	}
	char loadBuff[BCOMPTAB_INDEX_ENTLEN];
	if(fread(loadBuff, 1, BCOMPTAB_INDEX_ENTLEN, indF)!=BCOMPTAB_INDEX_ENTLEN){ throw std::runtime_error("Problem reading index file."); }
	uintptr_t nameLoc = be2nat64(loadBuff);
	if(wasSeek){
		theStr->seek(nameLoc);
	}
	loadBCompTSVEntry(this);
	focusInd++;
	return 1;
}

uintptr_t BCompTabularReader::getNumEntries(){
	return rowCount;
}

void BCompTabularReader::readSpecificEntry(uintptr_t entInd){
	if(entInd >= rowCount){ throw std::runtime_error("Bad entry index."); }
	resetInd = focusInd;
	if(fseekPointer(indF, BCOMPTAB_INDEX_ENTLEN*entInd, SEEK_SET)){ throw std::runtime_error("Problem seeking index file."); }
	char loadBuff[BCOMPTAB_INDEX_ENTLEN];
	if(fread(loadBuff, 1, BCOMPTAB_INDEX_ENTLEN, indF)!=BCOMPTAB_INDEX_ENTLEN){ throw std::runtime_error("Problem reading index file."); }
	uintptr_t nameLoc = be2nat64(loadBuff);
	theStr->seek(nameLoc);
	loadBCompTSVEntry(this);
}

BCompTabularWriter::BCompTabularWriter(int append, BlockCompOutStream* toFlit, const char* indFName){
	theStr = toFlit;
	intptr_t annotLen = getFileSize(indFName);
	if((annotLen >= 0) && (annotLen % BCOMPTAB_INDEX_ENTLEN)){throw std::runtime_error("Malformed index file.");}
	indF = fopen(indFName, append ? "ab" : "wb");
	if(indF == 0){ throw std::runtime_error("Could not open index file."); }
}

BCompTabularWriter::~BCompTabularWriter(){
	fclose(indF);
}

void BCompTabularWriter::writeNextEntry(){
	//note the location in the index
		char indOutBuff[BCOMPTAB_INDEX_ENTLEN];
		nat2be64(theStr->tell(), indOutBuff);
		if(fwrite(indOutBuff, 1, BCOMPTAB_INDEX_ENTLEN, indF)!=BCOMPTAB_INDEX_ENTLEN){throw std::runtime_error("Problem writing index file.");}
	//write the total size and the number of entries
		uintptr_t totEntSize = 0;
		for(uintptr_t i = 0; i<numEntries; i++){ totEntSize += entrySizes[i]; }
		char dumpBuff[16];
		nat2be64(totEntSize, dumpBuff);
		nat2be64(numEntries, dumpBuff + 8);
		theStr->writeBytes(dumpBuff,16);
	//write all the entries
		for(uintptr_t i = 0; i<numEntries; i++){
			theStr->writeBytes(curEntries[i], entrySizes[i]);
		}
	//and write the lengths
		for(uintptr_t i = 0; i<numEntries; i++){
			nat2be64(entrySizes[i], dumpBuff);
			theStr->writeBytes(dumpBuff, 8);
		}
	theStr->flush();
}

