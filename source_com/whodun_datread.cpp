#include "whodun_datread.h"

#include <string.h>
#include <iostream>

#include "whodun_oshook.h"

#define WHITESPACE " \t\r\n"

char* readEntireFile(const char* readFrom, uintptr_t* fillLen){
	intptr_t acostFSize = getFileSize(readFrom);
	if(acostFSize < 0){
		return 0;
	}
	FILE* costF = fopen(readFrom, "rb");
	if(costF == 0){
		return 0;
	}
	char* acostFBts = (char*)malloc(acostFSize);
	intptr_t costRead = fread(acostFBts, 1, acostFSize, costF);
	fclose(costF);
	if(costRead != acostFSize){
		free(acostFBts);
		return 0;
	}
	*fillLen = acostFSize;
	return acostFBts;
}

intptr_t readDatFileLine(FILE* toRead, std::vector<std::string>* toFill){
	//read until something other than space, \t, \r or \n
	unsigned char curRead;
	intptr_t numRead = fread(&curRead, 1, 1,toRead); if(numRead == 0){ return 0; }
	while(strchr(WHITESPACE, curRead)){
		numRead = fread(&curRead, 1, 1,toRead);
		if(numRead == 0){ return 0; }
	}
	//read until \n, splitting on whitespace
	intptr_t numEnt = 0;
	std::string curLine;
	curLine.push_back(curRead);
	numRead = fread(&curRead, 1, 1,toRead);
	while(numRead){
		if(strchr(WHITESPACE, curRead)){
			if(curLine.size()){
				toFill->push_back(curLine);
				numEnt++;
				curLine.clear();
			}
			if(curRead == '\n'){
				break;
			}
		}
		else{
			curLine.push_back(curRead);
		}
		numRead = fread(&curRead, 1, 1,toRead);
	}
	return numEnt;
}
