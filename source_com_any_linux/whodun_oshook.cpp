
#include "whodun_oshook.h"

#include <vector>
#include <stdio.h>
#include <iostream>
#include <string.h>
#include <stdlib.h>

#include <dlfcn.h>
#include <dirent.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>

const char* pathElementSep = "/";

bool fileExists(const char* fileName){
	struct stat dirFo;
	if((stat(fileName, &dirFo)==0) && (S_ISREG(dirFo.st_mode))){
		return 1;
	}
	return 0;
}

void killFile(const char* fileName){
	remove(fileName);
}

intptr_t getFileSize(const char* fileName){
	struct stat fdatBuff;
	if(stat(fileName, &fdatBuff)){ return -1; }
	return fdatBuff.st_size;
}

int fseekPointer(FILE* stream, intptr_t offset, int whence){
	return fseek(stream, offset, whence);
}

intptr_t ftellPointer(FILE* stream){
	return ftell(stream);
}

/**Passable info for a thread.*/
typedef struct{
	/**The function.*/
	void(*callFun)(void*);
	/**The argument to the function.*/
	void* callUniform;
} ThreadWorker;

/**
 * Go between for threads.
 * @param threadParam The ThreadWorker info.
 * return The thread result.
 */
void* mainThreadFunc(void* threadParam){
	ThreadWorker* curInfo = (ThreadWorker*)threadParam;
	void(*callFun)(void*) = curInfo->callFun;
	void* callUni = curInfo->callUniform;
	free(curInfo);
	callFun(callUni);
	return 0;
}

void* startThread(void(*callFun)(void*), void* callUniform){
	ThreadWorker* curInfo = (ThreadWorker*)malloc(sizeof(ThreadWorker));
	curInfo->callFun = callFun;
	curInfo->callUniform = callUniform;
	pthread_t* curHand = (pthread_t*)malloc(sizeof(pthread_t));
	pthread_create(curHand, 0, mainThreadFunc, curInfo);
	return curHand;
}

void joinThread(void* tHandle){
	pthread_t* tHand = (pthread_t*)tHandle;
	pthread_join(*tHand, 0);
	free(tHand);
}

void* makeMutex(){
	pthread_mutex_t* curSect = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(curSect, 0);
	return curSect;
}

void lockMutex(void* toLock){
	pthread_mutex_t* curSect = (pthread_mutex_t*)toLock;
	pthread_mutex_lock(curSect);
}

void unlockMutex(void* toUnlock){
	pthread_mutex_t* curSect = (pthread_mutex_t*)toUnlock;
	pthread_mutex_unlock(curSect);
}

void killMutex(void* toKill){
	pthread_mutex_t* curSect = (pthread_mutex_t*)toKill;
	pthread_mutex_destroy(curSect);
	free(curSect);
}

void* makeCondition(void* forMutex){
	pthread_cond_t* curCond = (pthread_cond_t*)malloc(sizeof(pthread_cond_t));
	pthread_cond_init(curCond, 0);
	return curCond;
}

void waitCondition(void* forMutex, void* forCondition){
	pthread_cond_t* curCond = (pthread_cond_t*)forCondition;
	pthread_mutex_t* curSect = (pthread_mutex_t*)forMutex;
    pthread_cond_wait(curCond, curSect);
}

void signalCondition(void* forMutex, void* forCondition){
	pthread_cond_t* curCond = (pthread_cond_t*)forCondition;
    pthread_cond_signal(curCond);
}

void broadcastCondition(void* forMutex, void* forCondition){
	pthread_cond_t* curCond = (pthread_cond_t*)forCondition;
    pthread_cond_broadcast(curCond);
}

void killCondition(void* forCondition){
	pthread_cond_t* curCond = (pthread_cond_t*)forCondition;
    pthread_cond_destroy(curCond);
    free(curCond);
}

typedef struct{
	void* dllMod;
} DLLibStruct;

void* loadInDLL(const char* loadFrom){
	void* dllMod = dlopen(loadFrom, RTLD_LAZY);
	if(dllMod){
		DLLibStruct* toRet = (DLLibStruct*)malloc(sizeof(DLLibStruct));
		toRet->dllMod = dllMod;
		return toRet;
	}
	return 0;
}

void* getDLLLocation(void* fromDLL, const char* locName){
	DLLibStruct* toRet = (DLLibStruct*)fromDLL;
	return dlsym(toRet->dllMod, locName);
}

void unloadDLL(void* toKill){
	DLLibStruct* toRet = (DLLibStruct*)toKill;
	dlclose(toRet->dllMod);
	free(toRet);
}

bool directoryExists(const char* dirName){
	struct stat dirFo;
	if((stat(dirName, &dirFo)==0) && (S_ISDIR(dirFo.st_mode))){
		return 1;
	}
	return 0;
}

int makeDirectory(const char* dirName){
	return mkdir(dirName, 0777);
}

void killDirectory(const char* dirName){
	rmdir(dirName);
}

uintptr_t* openDirectory(const char* dirName){
	DIR* dirFo = opendir(dirName);
	if(dirFo == 0){ return 0; }
	std::vector<bool> allFolds;
	std::vector<std::string> allNames;
	uintptr_t totNameLen = 0;
	struct stat dirSt;
	struct dirent* curDFo = readdir(dirFo);
	while(curDFo){
		totNameLen += strlen(curDFo->d_name) + 2;
		allNames.push_back(std::string(curDFo->d_name));
		std::string tmpName(dirName);
		tmpName.push_back('/');
		tmpName.append(curDFo->d_name);
		allFolds.push_back((stat(tmpName.c_str(), &dirSt)==0) && (S_ISDIR(dirSt.st_mode)));
	}
	uintptr_t* toRet = (uintptr_t*)malloc(totNameLen + sizeof(uintptr_t) + allFolds.size()*sizeof(char*));
	char** curPP = (char**)(toRet+1);
	char* curP = (char*)(curPP + allFolds.size());
	*toRet = allFolds.size();
	for(uintptr_t i = 0; i<allFolds.size(); i++){
		*curPP = curP;
		curPP++;
		*curP = allFolds[i];
		strcpy(curP+1, allNames[i].c_str());
		curP += (2+allNames[i].size());
	}
	return toRet;
}

void closeDirectory(uintptr_t* theDir){
	free(theDir);
}
