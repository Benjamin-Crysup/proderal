#include "whodun_parse.h"

#include "whodun_stringext.h"

void splitOnCharacter(const char* splitF, const char* splitT, int splitC, std::vector<const char*>* splitS, std::vector<const char*>* splitE){
	splitS->push_back(splitF);
	const char* curSt = splitF;
	const char* curEn = (const char*)memchr(curSt, splitC, splitT - curSt);
	while(curEn){
		splitE->push_back(curEn);
		curSt = curEn + 1;
		splitS->push_back(curSt);
		curEn = (const char*)memchr(curSt, splitC, splitT - curSt);
	}
	splitE->push_back(splitT);
}

void splitOnCharacters(const char* splitF, const char* splitT, int splitCN, const char* splitCs, std::vector<const char*>* splitS, std::vector<const char*>* splitE){
	std::vector<const char*> curTokS; curTokS.push_back(splitF);
	std::vector<const char*> curTokE; curTokE.push_back(splitT);
	std::vector<const char*> nextTokS;
	std::vector<const char*> nextTokE;
	for(int i = 0; i<splitCN; i++){
		nextTokS.clear(); nextTokE.clear();
		for(uintptr_t j = 0; j<curTokS.size(); j++){
			splitOnCharacter(curTokS[j], curTokE[j], splitCs[i], &nextTokS, &nextTokE);
		}
		std::swap(curTokS, nextTokS);
		std::swap(curTokE, nextTokE);
	}
	splitS->insert(splitS->end(), curTokS.begin(), curTokS.end());
	splitE->insert(splitE->end(), curTokE.begin(), curTokE.end());
}

short parseHexNibbleArr[] = {
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, -1, -1, -1, -1, -1, -1,
	-1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};

int parseHexNibble(char cval){
	return parseHexNibbleArr[0x00FF&cval];
}

int parseHexCode(char cvalA, char cvalB){
	int ivalA = parseHexNibble(cvalA);
	int ivalB = parseHexNibble(cvalB);
	int toRet = (ivalA << 4) + ivalB;
	if(toRet < 0){ return -1; }
	return toRet;
}
