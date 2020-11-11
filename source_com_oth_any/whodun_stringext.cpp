#include "whodun_stringext.h"

int strendswith(const char* str1, const char* str2){
	size_t str1L = strlen(str1);
	size_t str2L = strlen(str2);
	if(str1L < str2L){ return 0; }
	return strcmp(str1 + (str1L - str2L), str2) == 0;
}

size_t memcspn(const char* str1, size_t numB1, const char* str2, size_t numB2){
	for(size_t curCS = 0; curCS < numB1; curCS++){
		for(size_t i = 0; i<numB2; i++){
			if(str1[curCS] == str2[i]){
				return curCS;
			}
		}
	}
	return numB1;
}

size_t memspn(const char* str1, size_t numB1, const char* str2, size_t numB2){
	for(size_t curCS = 0; curCS < numB1; curCS++){
		for(size_t i = 0; i<numB2; i++){
			if(str1[curCS] == str2[i]){
				goto wasGut;
			}
		}
		return curCS;
		wasGut:
	}
	return numB1;
}

char* memmem(const char* str1, size_t numB1, const char* str2, size_t numB2){
	if(numB2 > numB1){
		return 0;
	}
	size_t maxCheck = (numB1 - numB2) + 1;
	for(size_t curCS = 0; curCS < maxCheck; curCS++){
		if(memcmp(str1+curCS, str2, numB2) == 0){
			return str1+curCS;
		}
	}
	return 0;
}

void memswap(char* arrA, char* arrB, size_t numBts){
	for(int i = 0; i<numBts; i++){
		char tmp = arrA[i];
		arrA[i] = arrB[i];
		arrB[i] = tmp;
	}
}

size_t memdiff(const char* str1, size_t numB1, const char* str2, size_t numB2){
	size_t toRet = 0;
	while(numB1 && numB2){
		if(*str1 != *str2){
			return toRet;
		}
		toRet++;
		str1++;
		numB1--;
		str2++;
		numB2--;
	}
	return toRet;
}

void nat2be64(uint_least64_t toPrep, char* toBuffer){
	uint_least64_t tmpPrep = toPrep;
	for(int i = 0; i<8; i++){
		toBuffer[7-i] = (char)(tmpPrep & 0x00FF);
		tmpPrep = tmpPrep >> 8;
	}
}

uint_least64_t be2nat64(const char* toDebuffer){
	uint_least64_t toRet = 0;
	for(int i = 0; i<8; i++){
		toRet = (toRet << 8) | (0x00FF & toDebuffer[i]);
	}
	return toRet;
}

void nat2le64(uint_least64_t toPrep, char* toBuffer){
	uint_least64_t tmpPrep = toPrep;
	for(int i = 0; i<8; i++){
		toBuffer[i] = (char)(tmpPrep & 0x00FF);
		tmpPrep = tmpPrep >> 8;
	}
}

uint_least64_t le2nat64(const char* toDebuffer){
	uint_least64_t toRet = 0;
	for(int i = 0; i<8; i++){
		toRet = toRet | ((0x00FF & toDebuffer[i]) << (8*i));
	}
	return toRet;
}


void nat2be32(uint_least32_t toPrep, char* toBuffer){
	uint_least32_t tmpPrep = toPrep;
	for(int i = 0; i<4; i++){
		toBuffer[3-i] = (char)(tmpPrep & 0x00FF);
		tmpPrep = tmpPrep >> 8;
	}
}

uint_least32_t be2nat32(const char* toDebuffer){
	uint_least32_t toRet = 0;
	for(int i = 0; i<4; i++){
		toRet = (toRet << 8) | (0x00FF & toDebuffer[i]);
	}
	return toRet;
}

void nat2le32(uint_least32_t toPrep, char* toBuffer){
	uint_least32_t tmpPrep = toPrep;
	for(int i = 0; i<4; i++){
		toBuffer[i] = (char)(tmpPrep & 0x00FF);
		tmpPrep = tmpPrep >> 8;
	}
}

uint_least32_t le2nat32(const char* toDebuffer){
	uint_least32_t toRet = 0;
	for(int i = 0; i<4; i++){
		toRet = toRet | ((0x00FF & toDebuffer[i]) << (8*i));
	}
	return toRet;
}

void nat2be16(uint_least16_t toPrep, char* toBuffer){
	toDebuffer[0] = (toPrep>>8);
	toDebuffer[1] = toPrep;
}

uint_least16_t be2nat16(const char* toDebuffer){
	return (0x00FF00 & (toDebuffer[0] << 8)) + (0x00FF & toDebuffer[1]);
}

void nat2le16(uint_least16_t toPrep, char* toBuffer){
	toDebuffer[0] = toPrep;
	toDebuffer[1] = (toPrep >> 8);
}

uint_least16_t le2nat16(const char* toDebuffer){
	return (0x00FF00 & (toDebuffer[1] << 8)) + (0x00FF & toDebuffer[0]);
}

uint_least32_t sfltbits(float toConv){
	union {
		uint32_t saveI;
		float saveF;
	} tmpSave;
	tmpSave.saveF = toConv;
	return tmpSave.saveI;
}

float sbitsflt(uint_least32_t toConv){
	union {
		uint32_t saveI;
		float saveF;
	} tmpSave;
	tmpSave.saveI = toConv;
	return tmpSave.saveF;
}

uint_least64_t sdblbits(double toConv){
	union {
		uint64_t saveI;
		double saveF;
	} tmpSave;
	tmpSave.saveF = toConv;
	return tmpSave.saveI;
}

double sbitsdbl(uint_least64_t toConv){
	union {
		uint64_t saveI;
		double saveF;
	} tmpSave;
	tmpSave.saveI = toConv;
	return tmpSave.saveF;
}
