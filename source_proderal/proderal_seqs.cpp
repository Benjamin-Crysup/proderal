
#include "proderal_seqs.h"

#include <string.h>
#include <algorithm>

#include "gpmatlan.h"
#include "whodun_cigfq.h"

int findCigarBounds(int seqLen, const char* cigarStr, intptr_t fromLoc, CigarBounds* toFill, std::ostream* errDump){
	const char* cigErr = 0;
	GPML_ByteArray cigAsBA = allocateByteArray(strlen(cigarStr));
		memcpy(cigAsBA->arrConts, cigarStr, cigAsBA->arrLen);
	GPML_IntArray cigLocRet = cigarStringToReferenceLocations(cigAsBA, fromLoc, &cigErr);
	free(cigAsBA->arrConts); free(cigAsBA);
	if(cigErr){
		*errDump << cigErr << std::endl;
		return 1;
	}
	bool haveCIGARBound = false;
	intptr_t tLowIndex = -1;
	intptr_t tHigIndex = -1;
	intptr_t retLen = cigLocRet->arrConts[cigLocRet->arrLen - 1];
	intptr_t retSkp = cigLocRet->arrConts[cigLocRet->arrLen - 2];
	intptr_t postSkip = seqLen - (retLen + retSkp);
	for(intptr_t i = 0; i<retLen; i++){
		int curLoc = cigLocRet->arrConts[i];
		if(curLoc < 0){continue;}
		if(haveCIGARBound){
			tLowIndex = (curLoc < tLowIndex) ? curLoc : tLowIndex;
			tHigIndex = (curLoc > tHigIndex) ? curLoc : tHigIndex;
		}
		else{
			tLowIndex = curLoc;
			tHigIndex = curLoc;
		}
		haveCIGARBound = true;
	}
	free(cigLocRet->arrConts); free(cigLocRet);
	toFill->lowInd = tLowIndex;
	toFill->higInd = tHigIndex;
	toFill->preSkip = retSkp;
	toFill->postSkip = postSkip;
	return 0;
}

char* reverseComplement(const char* startC, const char* endC){
	const char* revCompSArr = "ACGTacgt";
	const char* revCompEArr = "TGCAtgca";
	//do it
	int seqLen = endC - startC;
	char* toRet = (char*)malloc(seqLen + 1);
	for(int i = 0; i<seqLen; i++){
		char curC = startC[i];
		const char* curSC = strchr(revCompSArr, curC);
		if(curSC){
			toRet[i] = revCompEArr[curSC - revCompSArr];
		}
		else{
			toRet[i] = curC;
		}
	}
	std::reverse(toRet, toRet+seqLen);
	toRet[seqLen] = 0;
	return toRet;
}
