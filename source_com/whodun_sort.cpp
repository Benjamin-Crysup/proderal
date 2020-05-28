#include "whodun_sort.h"

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>

/**
 * This does an in-memory mergesort, using an allocated pool.
 * @param numEnts The number of entities to work over.
 * @param inMem The original data (and the place to put the sorted array.
 * @param opts The sort options.
 * @param allocPoot The pre-allocated pool for "malloc".
 */
void inMemoryMergesortPreA(uintptr_t numEnts, char* inMem, SortOptions* opts, char* allocPoot){
	if(numEnts > 1){
		uintptr_t itemSize = opts->itemSize;
		//get the first half of the list sorted
		uintptr_t numEntA = numEnts >> 1;
		char* subEntA = allocPoot;
		memcpy(subEntA, inMem, numEntA*itemSize);
		inMemoryMergesortPreA(numEntA, subEntA, opts, allocPoot + numEntA*itemSize);
		//get the second half of the list sorted
		uintptr_t numEntB = numEnts - numEntA;
		char* subEntB = allocPoot + numEntA*itemSize;
		memcpy(subEntB, inMem + numEntA*itemSize, numEntB*itemSize);
		inMemoryMergesortPreA(numEntB, subEntB, opts, allocPoot + numEnts*itemSize);
		//merge the two lists
		char* curTgt = inMem;
		char* curSrcA = subEntA;
		char* curSrcB = subEntB;
		while(numEntA && numEntB){
			if(opts->compMeth(curSrcA, curSrcB)){
				memcpy(curTgt, curSrcA, itemSize);
				curTgt += itemSize;
				curSrcA += itemSize;
				numEntA--;
			}
			else{
				memcpy(curTgt, curSrcB, itemSize);
				curTgt += itemSize;
				curSrcB += itemSize;
				numEntB--;
			}
		}
		while(numEntA){
			memcpy(curTgt, curSrcA, itemSize);
			curTgt += itemSize;
			curSrcA += itemSize;
			numEntA--;
		}
		while(numEntB){
			memcpy(curTgt, curSrcB, itemSize);
			curTgt += itemSize;
			curSrcB += itemSize;
			numEntB--;
		}
	}
}

void inMemoryMergesort(uintptr_t numEnts, char* inMem, SortOptions* opts){
	if(numEnts > 1){
		uintptr_t itemSize = opts->itemSize;
		//figure out how much space to allocate
		uintptr_t needMalEnt = 0;
		uintptr_t curME = numEnts;
		while(curME > 1){
			needMalEnt += curME;
			uintptr_t splA = curME / 2;
			uintptr_t splB = curME - splA;
			curME = (splA > splB) ? splA : splB;
		}
		//allocate it
		char* allocPoot = (char*)malloc(needMalEnt*itemSize);
		//do the actual sort
		inMemoryMergesortPreA(numEnts, inMem, opts, allocPoot);
		//clean up
		free(allocPoot);
	}
}

