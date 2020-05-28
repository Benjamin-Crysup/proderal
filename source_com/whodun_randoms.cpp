#include "whodun_randoms.h"

#include <math.h>

RandomGenerator::~RandomGenerator(){}

void RandomGenerator::getBytes(int numByte, char* toFill){
	for(int i = 0; i<numByte; i++){
		toFill[i] = getByte();
	}
}

void RandomGenerator::getDoubles(int numByte, double* toFill){
	int64_t* fillAsI = (int64_t*)toFill;
	//fill all with random
		getBytes(numByte*sizeof(double), (char*)toFill);
	//convert all to within range [1,2).
		for(int i = 0; i<numByte; i++){
			fillAsI[i] = 0x3FF0000000000000LL | (0x000FFFFFFFFFFFFFLL & fillAsI[i]);
		}
	//subtract 1.0 from each
		for(int i = 0; i<numByte; i++){
			toFill[i] = (toFill[i] - 1.0);
		}
}

#define CON_PI 3.14159265358979

void RandomGenerator::getStandardNormal(int numByte, double* toFill){
	//the minimum double
	union{
		int64_t asI;
		double asD;
	} minNZDblU;
	minNZDblU.asI = 0x3FF0000000000001LL;
	double minNZDbl = minNZDblU.asD - 1.0;
	
	int numLeft = numByte;
	double* curFill = toFill;
	while(numLeft){
		//generate uniforms
			int topD = (numLeft < DOUBLE_STANNORM_TEMPS ? numLeft : DOUBLE_STANNORM_TEMPS);
			getDoubles(topD, dblBufferA);
			getDoubles(topD, dblBufferB);
		//nudge any zeros up one
			for(int i = 0; i<topD; i++){
				if(dblBufferA[i] == 0.0){
					dblBufferA[i] = minNZDbl;
				}
			}
		//do box muller to get the randoms
			for(int i = 0; i<topD; i++){
				curFill[i] = sqrt(-2*log(dblBufferA[i]))*cos(2*CON_PI*dblBufferB[i]);
			}
		numLeft -= topD;
		curFill += topD;
	}
}

MersenneTwisterGenerator::MersenneTwisterGenerator(){
	numPG = 0;
	nextEnt = MERSENNE_TWIST_N;
	haveSeed = false;
	//TODO
}

MersenneTwisterGenerator::~MersenneTwisterGenerator(){
	//TODO
}

int MersenneTwisterGenerator::seedSize(){
	return sizeof(uint32_t)*MERSENNE_TWIST_N;
}

void MersenneTwisterGenerator::seed(char* seedV){
	haveSeed = true;
	//initialize the array
		mtarr[0] = 19650218UL;
		for(int i = 1; i<MERSENNE_TWIST_N; i++){
			mtarr[i] = (1812433253UL * (mtarr[i-1] ^ (mtarr[i-1] >> 30)) + i);
		}
	//mangle with the seed
		int i = 1;
		int j = 0;
		char* focSeed = seedV;
		for(int k = MERSENNE_TWIST_N; k; k--){
			uint32_t curKey = 0;
			for(unsigned j = 0; j<sizeof(uint32_t); j++){
				curKey = (curKey << 8) + (0x00FF & *focSeed);
				focSeed++;
			}
			mtarr[i] = (mtarr[i] ^ ((mtarr[i-1] ^ (mtarr[i-1] >> 30)) * 1664525UL)) + curKey + j;
			i++; j++;
			if(i >= MERSENNE_TWIST_N){
				mtarr[0] = mtarr[MERSENNE_TWIST_N-1];
				i = 1;
			}
		}
	//another post-mangle
		for(int k = MERSENNE_TWIST_N-1; k; k--){
			mtarr[i] = (mtarr[i] ^ ((mtarr[i-1] ^ (mtarr[i-1] >> 30)) * 1566083941UL)) - i;
			i++;
			if(i >= MERSENNE_TWIST_N){
				mtarr[0] = mtarr[MERSENNE_TWIST_N-1];
				i = 1;
			}
		}
	//clamp the first entry
		mtarr[0] = 0x80000000UL;
}

char MersenneTwisterGenerator::getByte(){
	if(numPG){
		char toRet = prevGen & 0x00FF;
		prevGen = prevGen >> 8;
		numPG--;
		return toRet;
	}
	if(nextEnt < MERSENNE_TWIST_N){
		numPG = sizeof(uint32_t);
		prevGen = mtarr[nextEnt];
		prevGen ^= (prevGen >> 11);
		prevGen ^= (prevGen << 7) & 0x9D2C5680UL;
		prevGen ^= (prevGen << 15) & 0xEFC60000UL;
		prevGen ^= (prevGen >> 18);
		nextEnt++;
		return getByte();
	}
	//generate a new batch
	int kk = 0;
	for(; kk<(MERSENNE_TWIST_N - MERSENNE_TWIST_M); kk++){
		uint32_t y = (mtarr[kk]&MERSENNE_TWIST_UPPERMASK) | (mtarr[kk+1]&MERSENNE_TWIST_LOWERMASK);
		mtarr[kk] = mtarr[kk+MERSENNE_TWIST_M] ^ (y >> 1) ^ ((y&0x01) ? MERSENNE_TWIST_MATRIXA : 0);
	}
	for(; kk<(MERSENNE_TWIST_N-1); kk++){
		uint32_t y = (mtarr[kk]&MERSENNE_TWIST_UPPERMASK) | (mtarr[kk+1]&MERSENNE_TWIST_LOWERMASK);
		mtarr[kk] = mtarr[kk+MERSENNE_TWIST_M-MERSENNE_TWIST_N] ^ (y >> 1) ^ ((y&0x01) ? MERSENNE_TWIST_MATRIXA : 0);
	}
	uint32_t y = (mtarr[MERSENNE_TWIST_N-1]&MERSENNE_TWIST_UPPERMASK) | (mtarr[0]&MERSENNE_TWIST_LOWERMASK);
	mtarr[MERSENNE_TWIST_N-1] = mtarr[MERSENNE_TWIST_M-1] ^ (y >> 1) ^ ((y&0x01) ? MERSENNE_TWIST_MATRIXA : 0);
	nextEnt = 0;
	return getByte();
}

