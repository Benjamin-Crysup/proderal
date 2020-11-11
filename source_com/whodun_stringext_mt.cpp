#include "whodun_stringext.h"

/**A uniform for a memcpy piece.*/
typedef struct{
	/**The place to copy to.*/
	void* myTo;
	/**The place to copy from.*/
	const void* myFrom;
	/**The number of bytes to copy.*/
	size_t myNumB;
	/**The sync point.*/
	ThreadMultiWait* myReport;
} MTMemcpyUniform;

/**
 * The action for each subthread.
 * @param myUni The uniform.
 */
void memcpymt_sub(void* myUni){
	MTMemcpyUniform* rUni = (MTMemcpyUniform*)myUni;
	memcpy(rUni->myTo, rUni->myFrom, rUni->myNumB);
	rUni->myReport->unwaitOne();
}

void* memcpymt(void* cpyTo, const void* cpyFrom, size_t numBts, unsigned numThread, ThreadPool* mainPool){
	ThreadMultiWait mainWait;
	std::vector<MTMemcpyUniform> memcpyUnis;
	memcpyUnis.resize(numThread);
	size_t numPerT = numBts / numThread;
	size_t numExtT = numBts % numThread;
	size_t curOff = 0;
	for(unsigned i = 0; i<numThread; i++){
		void* curTo = ((char*)cpyTo) + curOff;
		const void* curFrom = ((const char*)cpyFrom) + curOff;
		size_t curNum = numPerT + (i<numExtT);
		MTMemcpyUniform curUniD = {curTo, curFrom, curNum, &mainWait};
		memcpyUnis[i] = curUniD;
		mainPool->addTask(memcpymt_sub, &(memcpyUnis[i]));
		curOff += curNum;
	}
	mainWait.waitOn(numThread);
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
	/**The sync point.*/
	ThreadMultiWait* myReport;
} MTMemsetUniform;

/**
 * The action for each subthread.
 * @param myUni The uniform.
 */
void memsetmt_sub(void* myUni){
	MTMemsetUniform* rUni = (MTMemsetUniform*)myUni;
	memset(rUni->myTo, rUni->myVal, rUni->myNumB);
	rUni->myReport->unwaitOne();
}

void* memsetmt(void* setP, int value, size_t numBts, unsigned numThread, ThreadPool* mainPool){
	ThreadMultiWait mainWait;
	std::vector<MTMemsetUniform> memcpyUnis;
	memcpyUnis.resize(numThread);
	size_t numPerT = numBts / numThread;
	size_t numExtT = numBts % numThread;
	size_t curOff = 0;
	for(unsigned i = 0; i<numThread; i++){
		void* curTo = ((char*)setP) + curOff;
		size_t curNum = numPerT + (i<numExtT);
		MTMemsetUniform curUniD = {curTo, value, curNum, &mainWait};
		memcpyUnis[i] = curUniD;
		mainPool->addTask(memsetmt_sub, &(memcpyUnis[i]));
		curOff += curNum;
	}
	mainWait.waitOn(numThread);
	return setP;
}
