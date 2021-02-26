#include "whodun_stringext.h"

/**A uniform for a memcpy piece.*/
typedef struct{
	/**The place to copy to.*/
	void* myTo;
	/**The place to copy from.*/
	const void* myFrom;
	/**The number of bytes to copy.*/
	size_t myNumB;
	/**The ID of this task.*/
	uintptr_t threadID;
} MTMemcpyUniform;

/**
 * The action for each subthread.
 * @param myUni The uniform.
 */
void memcpymt_sub(void* myUni){
	MTMemcpyUniform* rUni = (MTMemcpyUniform*)myUni;
	memcpy(rUni->myTo, rUni->myFrom, rUni->myNumB);
}

void* memcpymt(void* cpyTo, const void* cpyFrom, size_t numBts, unsigned numThread, ThreadPool* mainPool){
	std::vector<MTMemcpyUniform> memcpyUnis;
	memcpyUnis.resize(numThread);
	size_t numPerT = numBts / numThread;
	size_t numExtT = numBts % numThread;
	size_t curOff = 0;
	for(unsigned i = 0; i<numThread; i++){
		void* curTo = ((char*)cpyTo) + curOff;
		const void* curFrom = ((const char*)cpyFrom) + curOff;
		size_t curNum = numPerT + (i<numExtT);
		MTMemcpyUniform* curUniD = &(memcpyUnis[i]);
			curUniD->myTo = curTo;
			curUniD->myFrom = curFrom;
			curUniD->myNumB = curNum;
		curUniD->threadID = mainPool->addTask(memcpymt_sub, curUniD);
		curOff += curNum;
	}
	for(uintptr_t i = 0; i<numThread; i++){
		mainPool->joinTask(memcpyUnis[i].threadID);
	}
	return cpyTo;
}

/**A uniform for a memset piece.*/
typedef struct{
	/**The place to copy to.*/
	void* myTo;
	/**The value to set.*/
	int myVal;
	/**The number of bytes.*/
	size_t myNumB;
	/**The ID of this task.*/
	uintptr_t threadID;
} MTMemsetUniform;

/**
 * The action for each subthread.
 * @param myUni The uniform.
 */
void memsetmt_sub(void* myUni){
	MTMemsetUniform* rUni = (MTMemsetUniform*)myUni;
	memset(rUni->myTo, rUni->myVal, rUni->myNumB);
}

void* memsetmt(void* setP, int value, size_t numBts, unsigned numThread, ThreadPool* mainPool){
	std::vector<MTMemsetUniform> memcpyUnis;
	memcpyUnis.resize(numThread);
	size_t numPerT = numBts / numThread;
	size_t numExtT = numBts % numThread;
	size_t curOff = 0;
	for(unsigned i = 0; i<numThread; i++){
		void* curTo = ((char*)setP) + curOff;
		size_t curNum = numPerT + (i<numExtT);
		MTMemsetUniform* curUniD = &(memcpyUnis[i]);
			curUniD->myTo = curTo;
			curUniD->myVal = value;
			curUniD->myNumB = curNum;
		curUniD->threadID = mainPool->addTask(memsetmt_sub, curUniD);
		curOff += curNum;
	}
	for(uintptr_t i = 0; i<numThread; i++){
		mainPool->joinTask(memcpyUnis[i].threadID);
	}
	return setP;
}
