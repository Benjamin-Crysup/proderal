#include "whodun_parse_table_genome.h"

#include <iostream>
#include <string.h>
#include <stdexcept>

#include "whodun_parse.h"
#include "whodun_compress.h"
#include "whodun_stringext.h"

BedFileReader::BedFileReader(TabularReader* readFrom){
	std::string tmpStr;
	#define GET_BFSTR(forInd) tmpStr.clear(); tmpStr.insert(tmpStr.end(), readFrom->curEntries[forInd], readFrom->curEntries[forInd] + readFrom->entrySizes[forInd]);
	while(readFrom->readNextEntry()){
		if(readFrom->numEntries < 3){ continue; }
		if(readFrom->entrySizes[0] && (readFrom->curEntries[0][0] == '#')){ continue; }
		GET_BFSTR(0) chromosomes.push_back(tmpStr);
		GET_BFSTR(1) locStarts.push_back(atol(tmpStr.c_str()));
		GET_BFSTR(2) locEnds.push_back(atol(tmpStr.c_str()));
	}
}

BedFileReader::~BedFileReader(){}

std::pair<uintptr_t,uintptr_t> parseUCSCCoordinate(const char* ucsc, std::string* refStore, std::string* errStore){
	std::pair<uintptr_t,uintptr_t> toRet(0,0);
	const char* curUcsc = ucsc;
	//find the reference name
		const char* colChar = strchr(curUcsc, ':');
		if(!colChar){
			errStore->append("Malformed coordinate: no colon found.");
			return toRet;
		}
		refStore->insert(refStore->end(), curUcsc, colChar);
		curUcsc = colChar + 1;
	//look for a -
		const char* minChar = strchr(curUcsc, '-');
		if(minChar){
			//make sure only digits up to minus
			if(strspn(curUcsc, "0123456789") != (uintptr_t)(minChar - curUcsc)){
				errStore->append("Malformed coordinate: illegal character found.");
				return toRet;
			}
			toRet.first = atol(curUcsc);
			curUcsc = minChar + 1;
		}
	//get the last number
		//only digits
		if(strspn(curUcsc, "0123456789") != strlen(curUcsc)){
			errStore->append("Malformed coordinate: illegal character found.");
			return toRet;
		}
		if(minChar){
			toRet.second = atol(curUcsc);
		}
		else{
			toRet.first = atol(curUcsc);
			toRet.second = toRet.first + 1;
		}
	return toRet;
}

CRBSAMFileContents::CRBSAMFileContents(){
	lastReadHead = 0;
	entryFlag = 0;
	entryPos = 0;
	entryMapq = 255;
	nextPos = -1;
	entryTempLen = 0;
}

CRBSAMFileContents::~CRBSAMFileContents(){}

uintptr_t CRBSAMFileContents::getPackedSize(){
	uintptr_t totSize = 0;
	totSize += sizeof(int);
	totSize += (sizeof(uintptr_t) + headerTxt.size());
	totSize += (sizeof(uintptr_t) + entryName.size());
	totSize += sizeof(int);
	totSize += (sizeof(uintptr_t) + entryReference.size());
	totSize += sizeof(intptr_t);
	totSize++;
	totSize += (sizeof(uintptr_t) + entryCigar.size());
	totSize += (sizeof(uintptr_t) + nextReference.size());
	totSize += sizeof(intptr_t);
	totSize += sizeof(intptr_t);
	totSize += (sizeof(uintptr_t) + entrySeq.size());
	totSize += (sizeof(uintptr_t) + entryQual.size());
	totSize += (sizeof(uintptr_t) + entryExtra.size());
	return totSize;
}

#define PACK_SIMPLE(simpleV, simpleT) curP = ((char*)memcpy(curP, &(simpleV), sizeof(simpleT))) + sizeof(simpleT);
#define PACK_STRING(simpleV) packSize = (simpleV).size(); PACK_SIMPLE(packSize, uintptr_t) curP = ((char*)memcpy(curP, &((simpleV)[0]), packSize)) + packSize;

void CRBSAMFileContents::pack(char* packInto){
	uintptr_t packSize;
	char* curP = packInto;
	PACK_SIMPLE(lastReadHead, int)
	PACK_STRING(headerTxt)
	PACK_STRING(entryName)
	PACK_SIMPLE(entryFlag, int)
	PACK_STRING(entryReference)
	PACK_SIMPLE(entryPos, intptr_t)
	PACK_SIMPLE(entryMapq, unsigned char)
	PACK_STRING(entryCigar)
	PACK_STRING(nextReference)
	PACK_SIMPLE(nextPos, intptr_t)
	PACK_SIMPLE(entryTempLen, intptr_t)
	PACK_STRING(entrySeq)
	PACK_STRING(entryQual)
	PACK_STRING(entryExtra)
}

#define UNPACK_SIMPLE(simpleV, simpleT) memcpy(&(simpleV), curP, sizeof(simpleT)); curP += sizeof(simpleT);
#define UNPACK_STRING(simpleV) UNPACK_SIMPLE(unpackSize, uintptr_t) (simpleV).resize(unpackSize); memcpy(&((simpleV)[0]), curP, unpackSize); curP += unpackSize;

void CRBSAMFileContents::unpack(char* unpackFrom){
	uintptr_t unpackSize;
	char* curP = unpackFrom;
	UNPACK_SIMPLE(lastReadHead, int)
	UNPACK_STRING(headerTxt)
	UNPACK_STRING(entryName)
	UNPACK_SIMPLE(entryFlag, int)
	UNPACK_STRING(entryReference)
	UNPACK_SIMPLE(entryPos, intptr_t)
	UNPACK_SIMPLE(entryMapq, unsigned char)
	UNPACK_STRING(entryCigar)
	UNPACK_STRING(nextReference)
	UNPACK_SIMPLE(nextPos, intptr_t)
	UNPACK_SIMPLE(entryTempLen, intptr_t)
	UNPACK_STRING(entrySeq)
	UNPACK_STRING(entryQual)
	UNPACK_STRING(entryExtra)
}

void CRBSAMFileContents::clear(){
	headerTxt.clear();
	entryName.clear();
	entryReference.clear();
	entryCigar.clear();
	nextReference.clear();
	entrySeq.clear();
	entryQual.clear();
	entryExtra.clear();
}

CRBSAMFileReader::CRBSAMFileReader(){
	curEntry = 0;
}
CRBSAMFileReader::~CRBSAMFileReader(){}
int CRBSAMFileReader::readNextEntry(){
	return readNextEntry(&curEnt);
}
CRBSAMFileWriter::CRBSAMFileWriter(){}
CRBSAMFileWriter::~CRBSAMFileWriter(){}
void CRBSAMFileWriter::writeNextEntry(){
	writeNextEntry(&curEnt);
}

SAMFileReader::SAMFileReader(TabularReader* toParse){
	fromTsv = toParse;
}
SAMFileReader::~SAMFileReader(){}
int SAMFileReader::readNextEntry(CRBSAMFileContents* toFill){
	if(fromTsv->readNextEntry() == 0){ return 0; }
	while(fromTsv->numEntries == 0){
		if(fromTsv->readNextEntry() == 0){ return 0; }
	}
	curEntry++;
	toFill->clear();
	if(fromTsv->numEntries && fromTsv->entrySizes[0] && (fromTsv->curEntries[0][0]=='@')){
		//is a header, expand and quit
		toFill->lastReadHead = 1;
		for(uintptr_t i = 0; i<fromTsv->numEntries; i++){
			if(i){ toFill->headerTxt.push_back('\t'); }
			const char* curEntTxt = fromTsv->curEntries[i];
			toFill->headerTxt.insert(toFill->headerTxt.end(), curEntTxt, curEntTxt + fromTsv->entrySizes[i]);
		}
		return 1;
	}
	if(fromTsv->numEntries < 11){ throw std::runtime_error("Data entry in SAM file too short."); }
	toFill->lastReadHead = 0;
	#define SAM_HANDLE_ELIDABLE_STRING(varName, varInd) \
		toFill->varName.insert(toFill->varName.end(), fromTsv->curEntries[varInd], fromTsv->curEntries[varInd] + fromTsv->entrySizes[varInd]);\
		if((toFill->varName.size() == 1) && (toFill->varName[0] == '*')){ toFill->varName.clear(); }
	#define SAM_HANDLE_INTEGER(varName, varInd) \
		tempStore.clear();\
		tempStore.insert(tempStore.end(), fromTsv->curEntries[varInd], fromTsv->curEntries[varInd] + fromTsv->entrySizes[varInd]);\
		toFill->varName = atol(tempStore.c_str());
	SAM_HANDLE_ELIDABLE_STRING(entryName, 0)
	SAM_HANDLE_INTEGER(entryFlag, 1)
	SAM_HANDLE_ELIDABLE_STRING(entryReference, 2)
	SAM_HANDLE_INTEGER(entryPos, 3) toFill->entryPos--;
	SAM_HANDLE_INTEGER(entryMapq, 4)
	SAM_HANDLE_ELIDABLE_STRING(entryCigar, 5)
	SAM_HANDLE_ELIDABLE_STRING(nextReference, 6)
	SAM_HANDLE_INTEGER(nextPos, 7) toFill->nextPos--;
	SAM_HANDLE_INTEGER(entryTempLen, 8)
	SAM_HANDLE_ELIDABLE_STRING(entrySeq, 9)
	SAM_HANDLE_ELIDABLE_STRING(entryQual, 10)
	if(toFill->entryQual.size() && (toFill->entryQual.size() != toFill->entrySeq.size())){throw std::runtime_error("Quality and sequence must have the same length.");}
	for(uintptr_t i = 11; i<fromTsv->numEntries; i++){
		if(i > 11){ toFill->entryExtra.push_back('\t'); }
		toFill->entryExtra.insert(toFill->entryExtra.end(), fromTsv->curEntries[i], fromTsv->curEntries[i] + fromTsv->entrySizes[i]);
	}
	return 1;
}

#define SAM_DUMP_ELIDABLE_STRING(varName) \
	tmpFlatS.push_back(tmpFlat.size());\
	if(toFill->varName.size()){\
		tmpFlat.insert(tmpFlat.end(), toFill->varName.begin(), toFill->varName.end());\
	}\
	else{\
		tmpFlat.push_back('*');\
	}\
	tmpFlatE.push_back(tmpFlat.size());
#define SAM_DUMP_INTEGER(varName, intOffset) \
	tmpFlatS.push_back(tmpFlat.size());\
	sprintf(numBuffer, "%ju", (uintmax_t)(toFill->varName + intOffset));\
	tmpFlat.insert(tmpFlat.end(), numBuffer, numBuffer + strlen(numBuffer));\
	tmpFlatE.push_back(tmpFlat.size());
#define SAM_DUMP_SINTEGER(varName, intOffset) \
	tmpFlatS.push_back(tmpFlat.size());\
	sprintf(numBuffer, "%jd", (intmax_t)(toFill->varName + intOffset));\
	tmpFlat.insert(tmpFlat.end(), numBuffer, numBuffer + strlen(numBuffer));\
	tmpFlatE.push_back(tmpFlat.size());

SAMFileWriter::SAMFileWriter(TabularWriter* toParse){
	fromTsv = toParse;
}
SAMFileWriter::~SAMFileWriter(){}
void SAMFileWriter::writeNextEntry(CRBSAMFileContents* toFill){
	tmpS.clear(); tmpE.clear(); tmpL.clear();
	if(toFill->lastReadHead){
		splitOnCharacter(&(toFill->headerTxt[0]), &(toFill->headerTxt[0]) + toFill->headerTxt.size(), '\t', &tmpS, &tmpE);
		fromTsv->numEntries = tmpS.size();
		for(uintptr_t i = 0; i<tmpS.size(); i++){ tmpL.push_back(tmpE[i] - tmpS[i]); }
		fromTsv->entrySizes = &(tmpL[0]);
		fromTsv->curEntries = &(tmpS[0]);
	}
	else{
		char numBuffer[4*sizeof(uintmax_t) + 4];
		tmpFlat.clear(); tmpFlatS.clear(); tmpFlatE.clear();
		SAM_DUMP_ELIDABLE_STRING(entryName)
		SAM_DUMP_INTEGER(entryFlag, 0)
		SAM_DUMP_ELIDABLE_STRING(entryReference)
		SAM_DUMP_INTEGER(entryPos, 1)
		SAM_DUMP_INTEGER(entryMapq, 0)
		SAM_DUMP_ELIDABLE_STRING(entryCigar)
		SAM_DUMP_ELIDABLE_STRING(nextReference)
		SAM_DUMP_INTEGER(nextPos, 1)
		SAM_DUMP_SINTEGER(entryTempLen, 0)
		SAM_DUMP_ELIDABLE_STRING(entrySeq)
		SAM_DUMP_ELIDABLE_STRING(entryQual)
		char* flatInit = &(tmpFlat[0]);
		for(uintptr_t i = 0; i<tmpFlatS.size(); i++){
			tmpS.push_back(flatInit + tmpFlatS[i]);
			tmpE.push_back(flatInit + tmpFlatE[i]);
		}
		splitOnCharacter(&(toFill->entryExtra[0]), &(toFill->entryExtra[0]) + toFill->entryExtra.size(), '\t', &tmpS, &tmpE);
		fromTsv->numEntries = tmpS.size();
		for(uintptr_t i = 0; i<tmpS.size(); i++){ tmpL.push_back(tmpE[i] - tmpS[i]); }
		fromTsv->entrySizes = &(tmpL[0]);
		fromTsv->curEntries = &(tmpS[0]);
	}
	fromTsv->writeNextEntry();
}

BAMFileReader::BAMFileReader(InStream* toParse){
	fromSrc = toParse;
	doneHead = 0;
	tempO = 0;
	//FIND THE MAGIC
		tempStore.resize(4);
		if(fromSrc->readBytes(&(tempStore[0]), 4) != 4){ throw std::runtime_error("No magic in BAM file."); }
		if(memcmp(&(tempStore[0]), "BAM\1", 4)!=0){ throw std::runtime_error("Bad magic in BAM file."); }
	//load in the header
		if(fromSrc->readBytes(&(tempStore[0]), 4) != 4){ throw std::runtime_error("BAM file missing header length."); }
		uint32_t headLen = le2nat32(&(tempStore[0]));
		tempStore.resize(headLen);
		if(fromSrc->readBytes(&(tempStore[0]), headLen) != headLen){ throw std::runtime_error("BAM file missing header data."); }
	//load in the references
		char tmpBuff[4];
		std::vector<char> nameTmp;
		if(fromSrc->readBytes(tmpBuff, 4) != 4){ throw std::runtime_error("BAM file missing reference data."); }
		uint32_t numRef = le2nat32(tmpBuff);
		refNames.resize(numRef);
		for(uint32_t i = 0; i<numRef; i++){
			if(fromSrc->readBytes(tmpBuff, 4) != 4){ throw std::runtime_error("BAM file missing reference name length."); }
			uint32_t rnameLen = le2nat32(tmpBuff);
			nameTmp.resize(rnameLen);
			if(fromSrc->readBytes(&(nameTmp[0]), rnameLen) != rnameLen){ throw std::runtime_error("BAM file missing reference name."); }
			if(rnameLen){ nameTmp.pop_back(); } //get rid of the null at the end
			refNames[i].insert(refNames[i].end(), nameTmp.begin(), nameTmp.end());
			if(fromSrc->readBytes(tmpBuff, 4) != 4){ throw std::runtime_error("BAM file missing reference length."); }
			//don't really care about the reference length: not my problem
		}
}
BAMFileReader::~BAMFileReader(){}
#define BAM_WS_CHARS " \r\t\n"
#define BAM_WS_COUNT 4
int BAMFileReader::readNextEntry(CRBSAMFileContents* toFill){
	if(doneHead){
		char tmpBuff[4];
		//length of the next entry
			int numRead = fromSrc->readBytes(tmpBuff,4);
			if(numRead == 0){ return 0; }
			if(numRead != 4){ throw std::runtime_error("BAM file mangled at end."); }
			uint32_t entLen = le2nat32(tmpBuff);
			toFill->lastReadHead = 0;
		//read the next entry bytes
			tempStore.resize(entLen);
			if(fromSrc->readBytes(&(tempStore[0]), entLen) != entLen){ throw std::runtime_error("BAM entry missing data."); }
		//get the things
			uintptr_t curI = 0;
			uintptr_t nxtI;
			#define BAM_READ_BYTE(toVar, errMess) \
				if((curI + 1) > tempStore.size()){ throw std::runtime_error(errMess); }\
				toVar = 0x00FF & tempStore[curI];\
				curI++;
			#define BAM_READ_I16(toVar, errMess) \
				if((curI + 2) > tempStore.size()){ throw std::runtime_error(errMess); }\
				toVar = le2nat16(&(tempStore[curI]));\
				curI += 2;
			#define BAM_READ_I32(toVar, errMess) \
				if((curI + 4) > tempStore.size()){ throw std::runtime_error(errMess); }\
				toVar = le2nat32(&(tempStore[curI]));\
				curI += 4;
			#define BAM_READ_STRING(toVar, reqLen, errMess) \
				if((curI + reqLen) > tempStore.size()){ throw std::runtime_error(errMess); }\
				toVar.clear();\
				toVar.insert(toVar.end(), tempStore.begin() + curI, tempStore.begin() + curI + reqLen);\
				curI += reqLen;
			#define BAM_SIGN_SAVE_BYTE(toVar) if(toVar & 0x0080){ toVar = toVar | (((uint64_t)-1) << 8); }
			#define BAM_SIGN_SAVE_SHORT(toVar) if(toVar & 0x008000){ toVar = toVar | (((uint64_t)-1) << 16); }
			#define BAM_SIGN_SAVE_INT(toVar) if(toVar & 0x80000000){ toVar = toVar | (((uint64_t)-1) << 32); }
			//get the constant crap
			int32_t refInd; BAM_READ_I32(refInd, "Reference index missing.") BAM_SIGN_SAVE_INT(refInd)
			int32_t entryPos; BAM_READ_I32(entryPos, "Position missing.") BAM_SIGN_SAVE_INT(entryPos) toFill->entryPos = entryPos;
			uintptr_t namLen; BAM_READ_BYTE(namLen, "Name length missing."); namLen = 0x00FF & namLen;
			BAM_READ_BYTE(toFill->entryMapq, "Map quality missing.");
			curI+=2; //I don't care about the index bin
			uintptr_t numCigOp; BAM_READ_I16(numCigOp, "Number of cigar operations missing.")
			BAM_READ_I16(toFill->entryFlag, "SAM flag missing.")
			uintptr_t seqLen; BAM_READ_I32(seqLen, "Sequence length missing.")
			int32_t nrefInd; BAM_READ_I32(nrefInd, "Next reference index missing.") BAM_SIGN_SAVE_INT(nrefInd)
			intptr_t nextPos; BAM_READ_I32(nextPos, "Next position missing.") BAM_SIGN_SAVE_INT(nextPos) toFill->nextPos = nextPos;
			int32_t entryTempLen; BAM_READ_I32(entryTempLen, "Template length missing.") BAM_SIGN_SAVE_INT(entryTempLen) toFill->entryTempLen = entryTempLen;
			//reference
			toFill->entryReference.clear();
			if(refInd >= 0){
				if((uintptr_t)refInd >= refNames.size()){ throw std::runtime_error("BAM entry has bad reference index."); }
				toFill->entryReference.insert(toFill->entryReference.end(), refNames[refInd].begin(), refNames[refInd].end());
			}
			//next reference
			toFill->nextReference.clear();
			if(nrefInd >= 0){
				if((uintptr_t)nrefInd >= refNames.size()){ throw std::runtime_error("BAM entry has bad next reference index."); }
				toFill->nextReference.insert(toFill->nextReference.end(), refNames[nrefInd].begin(), refNames[nrefInd].end());
			}
			//name
			BAM_READ_STRING(toFill->entryName, namLen, "Missing name.")
			toFill->entryName.pop_back(); //terminating null
			//raw cigar
			char strBuff[4*sizeof(uint32_t)+4];
			int firstCOp = 1;
			int cigNeedOver = 0;
			const char* cigOpMap = "MIDNSHP=X*******";
			toFill->entryCigar.clear();
			for(uintptr_t i = 0; i<numCigOp; i++){
				uintmax_t cigOpPack; BAM_READ_I32(cigOpPack, "Missing cigar operation.")
				int istrlen = sprintf(strBuff, "%ju", (cigOpPack >> 4));
				toFill->entryCigar.insert(toFill->entryCigar.end(), strBuff, strBuff + istrlen);
				toFill->entryCigar.push_back(cigOpMap[cigOpPack & 0x0F]);
				if(firstCOp){
					cigNeedOver = ((cigOpMap[cigOpPack & 0x0F] == 'S') && ((cigOpPack >> 4) == seqLen));
					firstCOp = 0;
				}
			}
			//sequence
			const char* seqBaseMap = "=ACMGRSVTWYHKDBN";
			nxtI = curI + ((seqLen + 1)/2);
			if(nxtI > tempStore.size()){ throw std::runtime_error("BAM entry sequence incomplete."); }
			toFill->entrySeq.resize(seqLen);
			for(uintptr_t i = 0; i<seqLen; i+=2){
				char curBt = tempStore[curI + (i>>1)];
				toFill->entrySeq[i] = seqBaseMap[0x0F & (curBt >> 4)];
			}
			for(uintptr_t i = 1; i<seqLen; i+=2){
				char curBt = tempStore[curI + (i>>1)];
				toFill->entrySeq[i] = seqBaseMap[0x0F & curBt];
			}
			curI = nxtI;
			//quality
			nxtI = curI + seqLen;
			if(nxtI > tempStore.size()){ throw std::runtime_error("BAM entry quality incomplete."); }
			if(tempStore[curI] == (char)0x00FF){ toFill->entryQual.clear(); }
			else{
				toFill->entryQual.resize(seqLen);
				char* srcLoc = &(tempStore[curI]);
				char* dumpLoc = &(toFill->entryQual[0]);
				for(uintptr_t i = 0; i<seqLen; i++){
					dumpLoc[i] = srcLoc[i] + 33;
				}
			}
			curI = nxtI;
			//extra crap
			char numConvBuffer[64+4*sizeof(uintmax_t)];
			int numT = 0;
			std::vector<char>* entryExtra = &(toFill->entryExtra);
			entryExtra->clear();
			while(curI < tempStore.size()){
				char tagA; BAM_READ_BYTE(tagA, "Missing tag name.")
				char tagB; BAM_READ_BYTE(tagB, "Missing tag name.")
				char tagTp; BAM_READ_BYTE(tagTp, "Missing tag type.");
				#define TAG_NMTPOUT(overTp) if(numT){entryExtra->push_back('\t');} entryExtra->push_back(tagA); entryExtra->push_back(tagB); entryExtra->push_back(':'); entryExtra->push_back(overTp); entryExtra->push_back(':');
				switch(tagTp){
					case 'A':
						{
							TAG_NMTPOUT(tagTp)
							char curGet; BAM_READ_BYTE(curGet, "Missing tag data (char).")
							entryExtra->push_back(curGet);
							numT = 1;
						}
						break;
					case 'c':
						{
							TAG_NMTPOUT('i')
							char curGet; BAM_READ_BYTE(curGet, "Missing tag data (byte).") BAM_SIGN_SAVE_BYTE(curGet)
							int numC = sprintf(numConvBuffer, "%jd", (intmax_t)curGet);
							entryExtra->insert(entryExtra->end(), numConvBuffer, numConvBuffer+numC);
							numT = 1;
						}
						break;
					case 'C':
						{
							TAG_NMTPOUT('i')
							unsigned char curGet; BAM_READ_BYTE(curGet, "Missing tag data (byte).")
							int numC = sprintf(numConvBuffer, "%ju", (uintmax_t)curGet);
							entryExtra->insert(entryExtra->end(), numConvBuffer, numConvBuffer+numC);
							numT = 1;
						}
						break;
					case 's':
						{
							TAG_NMTPOUT('i')
							int16_t curGet; BAM_READ_I16(curGet, "Missing tag data (short).") BAM_SIGN_SAVE_SHORT(curGet)
							int numC = sprintf(numConvBuffer, "%jd", (intmax_t)curGet);
							entryExtra->insert(entryExtra->end(), numConvBuffer, numConvBuffer+numC);
							numT = 1;
						}
						break;
					case 'S':
						{
							TAG_NMTPOUT('i')
							uint16_t curGet; BAM_READ_I16(curGet, "Missing tag data (short).")
							int numC = sprintf(numConvBuffer, "%jd", (intmax_t)curGet);
							entryExtra->insert(entryExtra->end(), numConvBuffer, numConvBuffer+numC);
							numT = 1;
						}
						break;
					case 'i':
						{
							TAG_NMTPOUT('i')
							int32_t curGet; BAM_READ_I32(curGet, "Missing tag data (int).") BAM_SIGN_SAVE_INT(curGet)
							int numC = sprintf(numConvBuffer, "%jd", (intmax_t)curGet);
							entryExtra->insert(entryExtra->end(), numConvBuffer, numConvBuffer+numC);
							numT = 1;
						}
						break;
					case 'I':
						{
							TAG_NMTPOUT('i')
							uint32_t curGet; BAM_READ_I32(curGet, "Missing tag data (int).")
							int numC = sprintf(numConvBuffer, "%jd", (intmax_t)curGet);
							entryExtra->insert(entryExtra->end(), numConvBuffer, numConvBuffer+numC);
							numT = 1;
						}
						break;
					case 'f':
						{
							TAG_NMTPOUT(tagTp)
							uint32_t curGet; BAM_READ_I32(curGet, "Missing tag data (float).")
							int numC = sprintf(numConvBuffer, "%f", (double)sbitsflt(curGet));
							entryExtra->insert(entryExtra->end(), numConvBuffer, numConvBuffer+numC);
							numT = 1;
						}
						break;
					case 'Z':
						{
							TAG_NMTPOUT(tagTp)
							char curGet; BAM_READ_BYTE(curGet, "Missing tag text.")
							while(curGet){
								entryExtra->push_back(curGet);
								BAM_READ_BYTE(curGet, "Missing tag text.")
							}
							numT = 1;
						}
						break;
					case 'H':
						{
							TAG_NMTPOUT(tagTp)
							char curGet; BAM_READ_BYTE(curGet, "Missing tag hex data.")
							while(curGet){
								entryExtra->push_back(curGet);
								BAM_READ_BYTE(curGet, "Missing tag hex data.")
								entryExtra->push_back(curGet);
								BAM_READ_BYTE(curGet, "Missing tag hex data.")
							}
							numT = 1;
						}
						break;
					case 'B':
						{
							char subTp; BAM_READ_BYTE(subTp, "Missing array type.")
							#define TAG_ARR_NMTPOUT if(numT){entryExtra->push_back('\t');} entryExtra->push_back(tagA); entryExtra->push_back(tagB); entryExtra->push_back(':'); entryExtra->push_back(tagTp); entryExtra->push_back(':'); entryExtra->push_back(subTp);
							uint32_t arrLen; BAM_READ_I32(arrLen, "Missing array length.")
							switch(subTp){
								case 'c':
									{
										TAG_ARR_NMTPOUT
										for(uintptr_t i = 0; i<arrLen; i++){
											entryExtra->push_back(',');
											char curGet; BAM_READ_BYTE(curGet, "Missing array data (byte).") BAM_SIGN_SAVE_BYTE(curGet)
											int numC = sprintf(numConvBuffer, "%jd", (intmax_t)curGet);
											entryExtra->insert(entryExtra->end(), numConvBuffer, numConvBuffer+numC);
										}
										numT = 1;
									}
									break;
								case 'C':
									{
										TAG_ARR_NMTPOUT
										for(uintptr_t i = 0; i<arrLen; i++){
											entryExtra->push_back(',');
											unsigned char curGet; BAM_READ_BYTE(curGet, "Missing array data (byte).")
											int numC = sprintf(numConvBuffer, "%ju", (uintmax_t)curGet);
											entryExtra->insert(entryExtra->end(), numConvBuffer, numConvBuffer+numC);
										}
										numT = 1;
									}
									break;
								case 's':
									{
										TAG_ARR_NMTPOUT
										for(uintptr_t i = 0; i<arrLen; i++){
											entryExtra->push_back(',');
											int16_t curGet; BAM_READ_I16(curGet, "Missing array data (short).") BAM_SIGN_SAVE_SHORT(curGet)
											int numC = sprintf(numConvBuffer, "%jd", (intmax_t)curGet);
											entryExtra->insert(entryExtra->end(), numConvBuffer, numConvBuffer+numC);
										}
										numT = 1;
									}
									break;
								case 'S':
									{
										TAG_ARR_NMTPOUT
										for(uintptr_t i = 0; i<arrLen; i++){
											entryExtra->push_back(',');
											uint16_t curGet; BAM_READ_I16(curGet, "Missing array data (short).")
											int numC = sprintf(numConvBuffer, "%ju", (uintmax_t)curGet);
											entryExtra->insert(entryExtra->end(), numConvBuffer, numConvBuffer+numC);
										}
										numT = 1;
									}
									break;
								case 'i':
									{
										TAG_ARR_NMTPOUT
										for(uintptr_t i = 0; i<arrLen; i++){
											entryExtra->push_back(',');
											int32_t curGet; BAM_READ_I32(curGet, "Missing array data (int).") BAM_SIGN_SAVE_INT(curGet)
											int numC = sprintf(numConvBuffer, "%jd", (intmax_t)curGet);
											entryExtra->insert(entryExtra->end(), numConvBuffer, numConvBuffer+numC);
										}
										numT = 1;
									}
									break;
								case 'I':
									{
										if((tagA == 'C') && (tagB == 'G') && cigNeedOver){
											toFill->entryCigar.clear();
											for(uintptr_t i = 0; i<arrLen; i++){
												uintmax_t cigOpPack; BAM_READ_I32(cigOpPack, "Missing extra cigar.")
												int istrlen = sprintf(strBuff, "%ju", (cigOpPack >> 4));
												toFill->entryCigar.insert(toFill->entryCigar.end(), strBuff, strBuff + istrlen);
												toFill->entryCigar.push_back(cigOpMap[cigOpPack & 0x0F]);
											}
											cigNeedOver = 0;
										}
										else{
											TAG_ARR_NMTPOUT
											for(uintptr_t i = 0; i<arrLen; i++){
												entryExtra->push_back(',');
												uint32_t curGet; BAM_READ_I32(curGet, "Missing array data (int).")
												int numC = sprintf(numConvBuffer, "%ju", (uintmax_t)curGet);
												entryExtra->insert(entryExtra->end(), numConvBuffer, numConvBuffer+numC);
											}
											numT = 1;
										}
									}
									break;
								case 'f':
									{
										TAG_ARR_NMTPOUT
										for(uintptr_t i = 0; i<arrLen; i++){
											entryExtra->push_back(',');
											uint32_t curGet; BAM_READ_I32(curGet, "Missing array data (float).")
											int numC = sprintf(numConvBuffer, "%f", (double)sbitsflt(curGet));
											entryExtra->insert(entryExtra->end(), numConvBuffer, numConvBuffer+numC);
										}
										numT = 1;
									}
									break;
								default:
									throw std::runtime_error("Unknown array type code.");
							}
						}
						break;
					default:
						throw std::runtime_error("Unknown extra field type code.");
				}
			}
		return 1;
	}
	else{
		//skip any whitespace
		uintptr_t skipWS = memspn(&(tempStore[tempO]), tempStore.size()-tempO, BAM_WS_CHARS, BAM_WS_COUNT);
		tempO += skipWS;
		//if nothing, end
		if(tempO >= tempStore.size()){ goto finishHead; }
		toFill->lastReadHead = 1;
		//current thing had better be an @
		if(tempStore[tempO] != '@'){ throw std::runtime_error("BAM header entry malformed."); }
		//get the header
		const char* headEntS = &(tempStore[tempO]);
		const char* headEntE = (const char*)memchr(headEntS, '\n', tempStore.size()-tempO);
		headEntE = headEntE ? headEntE : (&(tempStore[0]) + tempStore.size());
		toFill->headerTxt.clear();
		toFill->headerTxt.insert(toFill->headerTxt.end(), headEntS, headEntE);
		tempO += (headEntE - headEntS);
		return 1;
	}
	//got through the header, retry with actual data
	finishHead:
	doneHead = 1;
	return readNextEntry(toFill);
}

std::pair<uintptr_t,uintptr_t> cigarStringToReferencePositions(uintptr_t refPos0, std::vector<char>* cigStr, std::vector<intptr_t>* fillPos){
	uintptr_t curRef = refPos0;
	int seenAction = 0;
	uintptr_t softClipStart = 0;
	uintptr_t softClipEnd = 0;
	int seenHardEnd = 0;
	char* cigC = &((*cigStr)[0]);
	char* cigEnd = cigC + cigStr->size();
	while(cigC < cigEnd){
		char* cigDE = cigC;
		while(cigDE < cigEnd){
			if(strchr("+-0123456789", *cigDE)){
				cigDE++;
			}
			else{ break; }
		}
		if(cigDE == cigC){ throw std::runtime_error("Cigar operation missing count."); }
		if(cigDE >= cigEnd){ throw std::runtime_error("Cigar operation count missing operation."); }
		intptr_t opCount = atol(cigC);
		if(opCount < 0){ throw std::runtime_error("Negative operation count in cigar string."); }
		cigC = cigDE;
		switch(*cigC){
			case 'M':
			case '=':
			case 'X':
				if(softClipEnd || seenHardEnd){ throw std::runtime_error("Match operation in cigar string after clipping."); }
				seenAction = 1;
				for(intptr_t i = 0; i<opCount; i++){
					fillPos->push_back(curRef); curRef++;
				}
				break;
			case 'I':
				if(softClipEnd || seenHardEnd){ throw std::runtime_error("Insertion operation in cigar string after clipping."); }
				seenAction = 1;
				for(intptr_t i = 0; i<opCount; i++){
					fillPos->push_back(-1);
				}
				break;
			case 'D':
			case 'N':
				if(softClipEnd || seenHardEnd){ throw std::runtime_error("Deletion operation in cigar string after clipping."); }
				seenAction = 1;
				curRef += opCount;
				break;
			case 'S':
				if(seenHardEnd){ throw std::runtime_error("Cannot soft clip after hard clip."); }
				if(seenAction){ softClipEnd += opCount; } else{ softClipStart += opCount; }
				break;
			case 'H':
				if(seenAction || softClipStart){ seenHardEnd = 1; }
				break;
			case 'P':
				//I do not care
				break;
			default:
				throw std::runtime_error("Unknown cigar operation.");
		}
		cigC++;
	}
	return std::pair<uintptr_t,uintptr_t>(softClipStart, softClipEnd);
}

std::pair<intptr_t,intptr_t> getCigarReferenceBounds(std::vector<intptr_t>* lookPos){
	intptr_t lowRI = -1;
	uintptr_t i = 0;
	while(i < lookPos->size()){
		intptr_t curRI = (*lookPos)[i];
		if(curRI >= 0){
			lowRI = curRI;
			break;
		}
		i++;
	}
	if(lowRI < 0){ return std::pair<intptr_t,intptr_t>(-1,-1); }
	i = lookPos->size();
	while(i){
		i--;
		intptr_t curRI = (*lookPos)[i];
		if(curRI >= 0){
			return std::pair<intptr_t,intptr_t>(lowRI,curRI);
		}
	}
	return std::pair<intptr_t,intptr_t>(-1,-1);
}

void openCRBSamFileRead(const char* fileName, InStream** saveIS, TabularReader** saveTS, CRBSAMFileReader** saveSS){
	if(strcmp(fileName, "-")==0){
		*saveIS = new ConsoleInStream();
		*saveTS = new TSVTabularReader(0, *saveIS);
		*saveSS = new SAMFileReader(*saveTS);
		return;
	}
	if(strendswith(fileName, ".sam.gz") || strendswith(fileName, ".sam.gzip")){
		*saveIS = new GZipInStream(fileName);
		*saveTS = new TSVTabularReader(0, *saveIS);
		*saveSS = new SAMFileReader(*saveTS);
		return;
	}
	if(strendswith(fileName, ".bam")){
		*saveIS = new GZipInStream(fileName);
		*saveTS = 0;
		*saveSS = new BAMFileReader(*saveIS);
		return;
	}
	//sam is the default
	*saveIS = new FileInStream(fileName);
	*saveTS = new TSVTabularReader(0, *saveIS);
	*saveSS = new SAMFileReader(*saveTS);
}

void openCRBSamFileWrite(const char* fileName, OutStream** saveIS, TabularWriter** saveTS, CRBSAMFileWriter** saveSS){
	if(strcmp(fileName, "-")==0){
		*saveIS = new ConsoleOutStream();
		*saveTS = new TSVTabularWriter(0, *saveIS);
		*saveSS = new SAMFileWriter(*saveTS);
		return;
	}
	if(strendswith(fileName, ".sam.gz") || strendswith(fileName, ".sam.gzip")){
		//TODO BGZip
		/*
		*saveIS = new GZipOutStream(0, fileName);
		*saveTS = new TSVTabularWriter(0, *saveIS);
		*saveSS = new SAMFileWriter(*saveTS);
		return;
		*/
	}
	//TODO write BAM
	//sam is the default
	*saveIS = new FileOutStream(0, fileName);
	*saveTS = new TSVTabularWriter(0, *saveIS);
	*saveSS = new SAMFileWriter(*saveTS);
}
