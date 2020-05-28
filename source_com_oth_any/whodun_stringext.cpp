#include "whodun_stringext.h"

void memswap(char* arrA, char* arrB, size_t numBts){
	for(int i = 0; i<numBts; i++){
		char tmp = arrA[i];
		arrA[i] = arrB[i];
		arrB[i] = tmp;
	}
}

void canonPrepareInt64(uint_least64_t toPrep, char* toBuffer){
	uint_least64_t tmpPrep = toPrep;
	for(int i = 0; i<8; i++){
		toBuffer[7-i] = (char)(tmpPrep & 0x00FF);
		tmpPrep = tmpPrep >> 8;
	}
}

uint_least64_t canonParseInt64(const char* toDebuffer){
	uint_least64_t toRet = 0;
	for(int i = 0; i<8; i++){
		toRet = (toRet << 8) | (0x00FF & toDebuffer[i]);
	}
	return toRet;
}
