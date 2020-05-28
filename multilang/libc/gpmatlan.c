
#include "gpmatlan.h"

GPML_ByteArray allocateByteArray(int arrSize){
	GPML_ByteArray toRet = (GPML_ByteArray)malloc(sizeof(GPML_ByteArrayBase));
	if(toRet == 0){
		return 0;
	}
	toRet->arrLen = arrSize;
	toRet->curAlloc = arrSize;
	toRet->arrConts = (char*)calloc(arrSize, sizeof(char));
	if(toRet->arrConts == 0){
		free(toRet);
		return 0;
	}
	return toRet;
}

_Bool addToByteArray(GPML_ByteArray toAddTo, char toAdd){
	if(toAddTo->curAlloc == toAddTo->arrLen){
		int newSize = 2*toAddTo->curAlloc + 1;
		char* newMal = (char*)realloc(toAddTo->arrConts, newSize*sizeof(char));
		if(newMal == 0){
			return 1;
		}
		toAddTo->arrConts = newMal;
		toAddTo->curAlloc = newSize;
	}
	toAddTo->arrConts[toAddTo->arrLen] = toAdd;
	(toAddTo->arrLen)++;
	return 0;
}

GPML_IntArray allocateIntArray(int arrSize){
	GPML_IntArray toRet = (GPML_IntArray)malloc(sizeof(GPML_IntArrayBase));
	if(toRet == 0){
		return 0;
	}
	toRet->arrLen = arrSize;
	toRet->curAlloc = arrSize;
	toRet->arrConts = (int*)calloc(arrSize, sizeof(int));
	if(toRet->arrConts == 0){
		free(toRet);
		return 0;
	}
	return toRet;
}

GPML_FloatArray allocateFloatArray(int arrSize){
	GPML_FloatArray toRet = (GPML_FloatArray)malloc(sizeof(GPML_FloatArrayBase));
	if(toRet == 0){
		return 0;
	}
	toRet->arrLen = arrSize;
	toRet->curAlloc = arrSize;
	toRet->arrConts = (double*)calloc(arrSize, sizeof(double));
	if(toRet->arrConts == 0){
		free(toRet);
		return 0;
	}
	return toRet;
}

_Bool addToIntArray(GPML_IntArray toAddTo, int toAdd){
	if(toAddTo->curAlloc == toAddTo->arrLen){
		int newSize = 2*toAddTo->curAlloc + 1;
		int* newMal = (int*)realloc(toAddTo->arrConts, newSize*sizeof(int));
		if(newMal == 0){
			return 1;
		}
		toAddTo->arrConts = newMal;
		toAddTo->curAlloc = newSize;
	}
	toAddTo->arrConts[toAddTo->arrLen] = toAdd;
	(toAddTo->arrLen)++;
	return 0;
}

GPML_FloatArrayND allocateFloatArrayND(int dim, int* dimLens){
	GPML_FloatArrayND toRet = (GPML_FloatArrayND)malloc(sizeof(GPML_FloatArrayNDBase));
	if(toRet == 0){
		return 0;
	}
	int totAlloc = 1;
	for(int i = 0; i<dim; i++){
		totAlloc = totAlloc * dimLens[i];
	}
	int* dimLenCpy = (int*)malloc(dim*sizeof(int));
	if(dimLenCpy == 0){
		free(toRet);
		return 0;
	}
	double* dataAl = (double*)calloc(totAlloc, sizeof(double));
	if(dataAl == 0){
		free(toRet);
		free(dimLenCpy);
		return 0;
	}
	toRet->dim = dim;
	toRet->arrLen = dimLenCpy;
	toRet->arrConts = dataAl;
	memcpy(dimLenCpy, dimLens, dim*sizeof(int));
	return toRet;
}

double getFromFloatArrayND(GPML_FloatArrayND getFrom, int* dimInds){
	int curInd = dimInds[0];
	for(int i = 1; i<getFrom->dim; i++){
		curInd = (curInd * getFrom->arrLen[i]) + dimInds[i];
	}
	return getFrom->arrConts[curInd];
}

void setInFloatArrayND(GPML_FloatArrayND getFrom, int* dimInds, double newVal){
	int curInd = dimInds[0];
	for(int i = 1; i<getFrom->dim; i++){
		curInd = (curInd * getFrom->arrLen[i]) + dimInds[i];
	}
	getFrom->arrConts[curInd] = newVal;
}

_Bool addToPointerArray(PointerArray* toAddTo, void* toAdd){
	if(toAddTo->maxLen == toAddTo->curLen){
		int newSize = 2*toAddTo->maxLen + 1;
		void** newMal = (void**)realloc(toAddTo->allEnts, newSize*sizeof(void*));
		if(newMal == 0){
			return 1;
		}
		toAddTo->allEnts = newMal;
		toAddTo->maxLen = newSize;
	}
	toAddTo->allEnts[toAddTo->curLen] = toAdd;
	(toAddTo->curLen)++;
	return 0;
}

_Bool ensurePointerArrayCapacity(PointerArray* toAddTo, int byNum){
	int nextLen = toAddTo->curLen + byNum;
	if(nextLen > toAddTo->maxLen){
		int newSize = 2*toAddTo->maxLen + 1;
		if(newSize < nextLen){
			newSize = nextLen;
		}
		void** newMal = (void**)realloc(toAddTo->allEnts, newSize*sizeof(void*));
		if(newMal == 0){
			return 1;
		}
		toAddTo->allEnts = newMal;
		toAddTo->maxLen = newSize;
	}
	return 0;
}

void freeAllPointers(int numKill, void** toKill){
	for(int i = 0; i<numKill; i++){
		free(toKill[i]);
	}
}

void freeAllPointersExcept(int numKill, void** toKill, int numSave, void** toSave){
	for(int i = 0; i<numKill; i++){
		void* curFoc = toKill[i];
		_Bool needKill = 1;
		for(int j = 0; j<numSave; j++){
			if(toSave[j] == curFoc){
				needKill = 0;
				break;
			}
		}
		if(needKill){
			free(curFoc);
		}
	}
	
}

void removePointerFromKillArray(int* numKill, void** toKill, void* toSpare){
	int numKillS = *numKill;
	int startCpyInd = numKillS;
	for(int i = 0; i<numKillS; i++){
		if(toKill[i] == toSpare){
			startCpyInd = i;
			*numKill = (numKillS-1);
			break;
		}
	}
	for(int i = startCpyInd + 1; i<numKillS; i++){
		toKill[i-1] = toKill[i];
	}
}
