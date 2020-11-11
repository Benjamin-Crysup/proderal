#include "whodun_setting.h"

#include <string.h>
#include <stdexcept>

#include "whodun_stringext.h"

#define LOAD_CHUNK 1024

#define WHITESPACE " \t\r\n"
#define WHITESPACE_LEN 4

LoadedSettings::LoadedSettings(InStream* loadFrom){
	std::vector<uintptr_t> entryOffsets;
	std::vector<char> curLine;
	int isMore = 1;
	while(isMore || curLine.size()){
		//parse a line if it is present
		const char* lineStart = &(curLine[0]);
		const char* lineEnd = lineStart + curLine.size();
		const char* nlOffset = (const char*)memchr(lineStart, '\n', curLine.size());
		if(nlOffset){ lineEnd = nlOffset + 1; }
		if(!isMore && (nlOffset==0)){ nlOffset = lineEnd; }
		if(nlOffset){
			uintptr_t curLineLen = lineEnd - lineStart;
			if((curLine[0] == ';') || (curLine[0] == '#')){
				curLine.erase(curLine.begin(), curLine.begin() + curLineLen);
				continue;
			}
			//skip any leading whitespace
			lineStart += memspn(lineStart, lineEnd - lineStart, WHITESPACE, WHITESPACE_LEN);
			if(lineStart == lineEnd){
				curLine.erase(curLine.begin(), curLine.begin() + curLineLen);
				continue;
			}
			//if no =, complain
			const char* eqOffset = (const char*)memchr(lineStart, '=', lineEnd - lineStart);
			if(eqOffset == 0){
				throw std::runtime_error("INI setting missing value");
			}
			//add the name
			const char* setNameEnd = lineStart + memcspn(lineStart, eqOffset - lineStart, WHITESPACE, WHITESPACE_LEN);
			entryOffsets.push_back(byteStore.size());
			byteStore.insert(byteStore.end(), lineStart, setNameEnd);
			byteStore.push_back(0);
			//add all the text for the setting
			entryOffsets.push_back(byteStore.size());
			byteStore.insert(byteStore.end(), eqOffset + 1, lineEnd);
			byteStore.push_back(0);
			//next line
			curLine.erase(curLine.begin(), curLine.begin() + curLineLen);
			continue;
		}
		//load some more data
		uintptr_t origSize = curLine.size();
		curLine.resize(origSize + LOAD_CHUNK);
		uintptr_t numLoad = loadFrom->readBytes(&(curLine[origSize]), LOAD_CHUNK);
		if(numLoad < LOAD_CHUNK){
			isMore = 0;
			curLine.resize(origSize + numLoad);
		}
	}
	//make some pointers
	char* byteStoreS = &(byteStore[0]);
	entryStore.resize(entryOffsets.size());
	for(uintptr_t i = 0; i<entryOffsets.size(); i++){
		entryStore[i] = byteStoreS + entryOffsets[i];
	}
}


