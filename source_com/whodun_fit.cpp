#include "whodun_fit.h"

#include "whodun_stringext.h"

DatapointDescription::DatapointDescription(){}

DatapointDescription::~DatapointDescription(){}

void DatapointDescription::figureColumnsFromHeader(TabularReader* parseF){
	uintptr_t noAssn = -1;
	colIndices.clear(); colIndices.insert(colIndices.end(), colTypes.size(), noAssn);
	for(uintptr_t i = 0; i<parseF->numEntries; i++){
		tmpName.clear(); tmpName.insert(tmpName.end(), parseF->curEntries[i], parseF->curEntries[i] + parseF->entrySizes[i]);
		for(uintptr_t j = 0; j<colIndices.size(); j++){
			if(colNames[j] == tmpName){
				if(colIndices[j] != noAssn){
					throw std::runtime_error("Multiple columns with the same name.");
				}
				break;
			}
		}
	}
	maxCIndex = 0;
	for(uintptr_t j = 0; j<colIndices.size(); j++){
		if(colIndices[j] == noAssn){
			throw std::runtime_error("Missing column in table.");
		}
		maxCIndex = std::max(maxCIndex, colIndices[j]);
	}
	factorColMap.clear();
		factorColMap.resize(colTypes.size());
	factorMaxLevel.clear();
		factorMaxLevel.insert(factorMaxLevel.end(), colTypes.size(), 0);
	colFactorMap.clear();
		colFactorMap.resize(colTypes.size());
	ioStage.resize(8*colTypes.size());
}

void DatapointDescription::figureColumnsIdentity(){
	for(uintptr_t j = 0; j<colTypes.size(); j++){
		colIndices.push_back(j);
	}
	maxCIndex = colTypes.size() - 1;
	factorColMap.clear();
		factorColMap.resize(colTypes.size());
	factorMaxLevel.clear();
		factorMaxLevel.insert(factorMaxLevel.end(), colTypes.size(), 0);
	colFactorMap.clear();
		colFactorMap.resize(colTypes.size());
	ioStage.resize(8*colTypes.size());
}

void DatapointDescription::dumpColumnHeader(TabularWriter* dumpT){
	tmpDumpS.clear();
	tmpDumpCp.clear();
	tmpDumpC.clear();
	for(uintptr_t j = 0; j<colIndices.size(); j++){
		std::string* ccolN = &(colNames[j]);
		tmpDumpS.push_back(ccolN->size());
		tmpDumpC.insert(tmpDumpC.end(), ccolN->begin(), ccolN->end());
	}
	tmpDumpS.push_back(0);
	tmpDumpC.push_back(0);
	char* curP = &(tmpDumpC[0]);
	for(uintptr_t j = 0; j<colIndices.size(); j++){
		tmpDumpCp.push_back(curP);
		curP += tmpDumpS[j];
	}
	dumpT->numEntries = colIndices.size();
	dumpT->entrySizes = &(tmpDumpS[0]);
	dumpT->curEntries = &(tmpDumpCp[0]);
	dumpT->writeNextEntry();
}

#define DD_LOAD_INT if(parseF->readBytes(tmpLdB,8)!=8){ throw std::runtime_error("Unexpected end of file."); } curLdV = be2nat64(tmpLdB);

void DatapointDescription::loadHeader(InStream* parseF){
	char tmpLdB[8];
	uintptr_t curLdV;
	//clear
	colTypes.clear();
	colNames.clear();
	factorColMap.clear();
	factorMaxLevel.clear();
	colIndices.clear();
	colFactorMap.clear();
	//load the number of types
	DD_LOAD_INT uintptr_t numCol = curLdV;
	colTypes.resize(numCol);
	colNames.resize(numCol);
	//load for each
	for(uintptr_t i = 0; i<numCol; i++){
		//type
		DD_LOAD_INT int curTp = curLdV;
		if((curTp != DATAPOINT_CELL_INT) && (curTp != DATAPOINT_CELL_FLT) && (curTp != DATAPOINT_CELL_CAT)){ throw std::runtime_error("Bad type specification for data."); }
		colTypes.push_back(curTp);
		//name
		DD_LOAD_INT uintptr_t namLen = curLdV;
		tmpDumpC.resize(namLen);
		if(parseF->readBytes(&(tmpDumpC[0]), namLen) != namLen){ throw std::runtime_error("Name cut short."); }
		colNames.push_back(std::string(tmpDumpC.begin(), tmpDumpC.end()));
	}
	//update any extra junk
	figureColumnsIdentity();
	//load factor map
	for(uintptr_t i = 0; i<numCol; i++){
		if(colTypes[i] != DATAPOINT_CELL_CAT){ continue; }
		int maxLevel = 0;
		std::map<std::string,int>* loadM = &(factorColMap[i]);
		DD_LOAD_INT uintptr_t numFacs = curLdV;
		for(uintptr_t j = 0; j<numFacs; j++){
			DD_LOAD_INT int facVal = curLdV;
			if(facVal >= maxLevel){ maxLevel = facVal + 1; }
			DD_LOAD_INT uintptr_t namLen = curLdV;
			tmpDumpC.resize(namLen);
			if(parseF->readBytes(&(tmpDumpC[0]), namLen) != namLen){ throw std::runtime_error("Factor name cut short."); }
			(*loadM)[std::string(tmpDumpC.begin(), tmpDumpC.end())] = facVal;
		}
		factorMaxLevel[i] = maxLevel;
	}
}

#define DD_WRITE_INT(toConv) nat2be64(toConv, tmpDmB); parseF->writeBytes(tmpDmB, 8);

void DatapointDescription::writeHeader(OutStream* parseF){
	char tmpDmB[8];
	DD_WRITE_INT(colTypes.size())
	for(uintptr_t i = 0; i<colTypes.size(); i++){
		DD_WRITE_INT(colTypes[i])
		DD_WRITE_INT(colNames[i].size())
		parseF->writeBytes(colNames[i].c_str(), colNames[i].size());
	}
	for(uintptr_t i = 0; i<colTypes.size(); i++){
		if(colTypes[i] != DATAPOINT_CELL_CAT){ continue; }
		std::map<std::string,int>* loadM = &(factorColMap[i]);
		DD_WRITE_INT(loadM->size())
		for(std::map<std::string,int>::iterator lmIt = loadM->begin(); lmIt != loadM->end(); lmIt++){
			DD_WRITE_INT(lmIt->second)
			DD_WRITE_INT(lmIt->first.size())
			parseF->writeBytes(lmIt->first.c_str(), lmIt->first.size());
		}
	}
}

void DatapointDescription::parseRow(TabularReader* parseF, DatapointCell* saveL){
	if(parseF->numEntries <= maxCIndex){
		throw std::runtime_error("Row in table too short.");
	}
	for(uintptr_t j = 0; j<colIndices.size(); j++){
		uintptr_t rj = colIndices[j];
		tmpName.clear();
		tmpName.insert(tmpName.end(), parseF->curEntries[rj], parseF->curEntries[rj] + parseF->entrySizes[rj]);
		switch(colTypes[j]){
			case DATAPOINT_CELL_INT:
				saveL[j].idat = atol(tmpName.c_str());
				break;
			case DATAPOINT_CELL_FLT:
				saveL[j].ddat = atof(tmpName.c_str());
				break;
			case DATAPOINT_CELL_CAT:
			{
				std::map<std::string,int>* curFM = &(factorColMap[j]);
				std::map<std::string,int>::iterator facIt = curFM->find(tmpName);
				if(facIt == curFM->end()){
					(*curFM)[tmpName] = factorMaxLevel[j];
					factorMaxLevel[j]++;
					colFactorMap[j].clear();
					facIt = curFM->find(tmpName);
				}
				saveL[j].cdat = facIt->second;
			}
				break;
			default:
				throw std::runtime_error("...da faq?");
		}
	}
}

void DatapointDescription::dumpRow(TabularWriter* dumpT, DatapointCell* saveL){
	if(colFactorMap.size() == 0){
		//set up the reverse for factors if need be
		colFactorMap.resize(factorColMap.size());
		for(uintptr_t j = 0; j<factorColMap.size(); j++){
			std::map<std::string,int>* curFM = &(factorColMap[j]);
			std::map<int,std::string>* curBM = &(colFactorMap[j]);
			for(std::map<std::string,int>::iterator facIt = curFM->begin(); facIt != curFM->end(); facIt++){
				(*curBM)[facIt->second] = facIt->first;
			}
		}
	}
	
	char numBuff[8*sizeof(uintptr_t)+32];
	uintptr_t tmpLen;
	tmpDumpS.clear();
	tmpDumpCp.clear();
	tmpDumpC.clear();
	for(uintptr_t j = 0; j<colIndices.size(); j++){
		switch(colTypes[j]){
			case DATAPOINT_CELL_INT:
				sprintf(numBuff, "%ju", (uintmax_t)(saveL[j].idat));
				tmpLen = strlen(numBuff);
				tmpDumpS.push_back(tmpLen);
				tmpDumpC.insert(tmpDumpC.end(), numBuff, numBuff + tmpLen);
				break;
			case DATAPOINT_CELL_FLT:
				sprintf(numBuff, "%e", (saveL[j].ddat));
				tmpLen = strlen(numBuff);
				tmpDumpS.push_back(tmpLen);
				tmpDumpC.insert(tmpDumpC.end(), numBuff, numBuff + tmpLen);
				break;
			case DATAPOINT_CELL_CAT:
			{
				std::map<int,std::string>* curBM = &(colFactorMap[j]);
				if(curBM->size() == 0){
					//set up the reverse if need be
					std::map<std::string,int>* curFM = &(factorColMap[j]);
					for(std::map<std::string,int>::iterator facIt = curFM->begin(); facIt != curFM->end(); facIt++){
						(*curBM)[facIt->second] = facIt->first;
					}
				}
				std::string* repS = &((*curBM)[saveL[j].cdat]);
				tmpDumpS.push_back(repS->size());
				tmpDumpC.insert(tmpDumpC.end(), repS->begin(), repS->end());
			}
				break;
			default:
				throw std::runtime_error("...da faq?");
		}
	}
	tmpDumpS.push_back(0);
	tmpDumpC.push_back(0);
	char* curP = &(tmpDumpC[0]);
	for(uintptr_t j = 0; j<colIndices.size(); j++){
		tmpDumpCp.push_back(curP);
		curP += tmpDumpS[j];
	}
	dumpT->numEntries = colIndices.size();
	dumpT->entrySizes = &(tmpDumpS[0]);
	dumpT->curEntries = &(tmpDumpCp[0]);
	dumpT->writeNextEntry();
}

int DatapointDescription::loadRow(InStream* parseF, DatapointCell* saveL){
	char* curFoc = &(ioStage[0]);
	uintptr_t numLoad = parseF->readBytes(curFoc, ioStage.size());
	if(numLoad == 0){ return 0; }
	if(numLoad != ioStage.size()){ throw std::runtime_error("Table file truncated."); }
	for(uintptr_t j = 0; j<colTypes.size(); j++){
		int_least64_t curV = be2nat64(curFoc);
		switch(colTypes[j]){
			case DATAPOINT_CELL_INT:
				saveL[j].idat = curV;
				break;
			case DATAPOINT_CELL_FLT:
				saveL[j].ddat = sbitsdbl(curV);
				break;
			case DATAPOINT_CELL_CAT:
				saveL[j].cdat = curV;
				break;
			default:
				throw std::runtime_error("...da faq?");
		}
		curFoc += 8;
	}
	return 1;
}

void DatapointDescription::writeRow(OutStream* parseF, DatapointCell* saveL){
	char* curFoc = &(ioStage[0]);
	for(uintptr_t j = 0; j<colTypes.size(); j++){
		switch(colTypes[j]){
			case DATAPOINT_CELL_INT:
				nat2be64(saveL[j].idat, curFoc);
				break;
			case DATAPOINT_CELL_FLT:
				nat2be64(sdblbits(saveL[j].ddat), curFoc);
				break;
			case DATAPOINT_CELL_CAT:
				nat2be64(saveL[j].cdat, curFoc);
				break;
			default:
				throw std::runtime_error("...da faq?");
		}
		curFoc += 8;
	}
	parseF->writeBytes(&(ioStage[0]), ioStage.size());
}
