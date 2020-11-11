#define _WIN32_WINNT 0x0600
#include "whodun_oshook.h"

#include <vector>
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <windows.h>

const char* pathElementSep = "\\";

bool fileExists(const char* fileName){
	DWORD dwAttrib = GetFileAttributes(fileName);
	return ((dwAttrib != INVALID_FILE_ATTRIBUTES) && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

void killFile(const char* fileName){
	DeleteFile(fileName);
}

intptr_t getFileSize(const char* fileName){
	WIN32_FILE_ATTRIBUTE_DATA storeAtts;
	if(GetFileAttributesEx(fileName, GetFileExInfoStandard, &storeAtts)){
		long long int lowDW = storeAtts.nFileSizeLow;
		long long int higDW = storeAtts.nFileSizeHigh;
		return (higDW << (8*sizeof(DWORD))) + lowDW;
	}
	else{
		return -1;
	}
}

int fseekPointer(FILE* stream, intptr_t offset, int whence){
	return _fseeki64(stream, offset, whence);
}

intptr_t ftellPointer(FILE* stream){
	return _ftelli64(stream);
}

/**Passable info for a thread.*/
typedef struct{
	/**The function.*/
	void(*callFun)(void*);
	/**The argument to the function.*/
	void* callUniform;
} ThreadWorker;

/**
 * Go between for windows threads.
 * @param threadParam The ThreadWorker info.
 * return The thread result.
 */
DWORD WINAPI mainThreadFunc(LPVOID threadParam){
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
	HANDLE* curHand = (HANDLE*)malloc(sizeof(HANDLE));
	*curHand = CreateThread(NULL, 0, mainThreadFunc, curInfo, 0, 0);
	return curHand;
}

void joinThread(void* tHandle){
	HANDLE* tHand = (HANDLE*)tHandle;
	WaitForSingleObject(*tHand, INFINITE);
	CloseHandle(*tHand);
	free(tHand);
}

void* makeMutex(){
	CRITICAL_SECTION* curSect = (CRITICAL_SECTION*)malloc(sizeof(CRITICAL_SECTION));
	InitializeCriticalSectionAndSpinCount(curSect, 3);
	return curSect;
}

void lockMutex(void* toLock){
	CRITICAL_SECTION* curSect = (CRITICAL_SECTION*)toLock;
	EnterCriticalSection(curSect);
}

void unlockMutex(void* toUnlock){
	CRITICAL_SECTION* curSect = (CRITICAL_SECTION*)toUnlock;
	LeaveCriticalSection(curSect);
}

void killMutex(void* toKill){
	CRITICAL_SECTION* curSect = (CRITICAL_SECTION*)toKill;
	DeleteCriticalSection(curSect);
	free(curSect);
}

void* makeCondition(void* forMutex){
	CONDITION_VARIABLE* curCond = (CONDITION_VARIABLE*)malloc(sizeof(CONDITION_VARIABLE));
	InitializeConditionVariable(curCond);
	return curCond;
}

void waitCondition(void* forMutex, void* forCondition){
	CRITICAL_SECTION* curSect = (CRITICAL_SECTION*)forMutex;
	CONDITION_VARIABLE* curCond = (CONDITION_VARIABLE*)forCondition;
	SleepConditionVariableCS(curCond, curSect, INFINITE);
}

void signalCondition(void* forMutex, void* forCondition){
	//CRITICAL_SECTION* curSect = (CRITICAL_SECTION*)forMutex;
	CONDITION_VARIABLE* curCond = (CONDITION_VARIABLE*)forCondition;
	WakeConditionVariable(curCond);
}

void broadcastCondition(void* forMutex, void* forCondition){
	//CRITICAL_SECTION* curSect = (CRITICAL_SECTION*)forMutex;
	CONDITION_VARIABLE* curCond = (CONDITION_VARIABLE*)forCondition;
	WakeAllConditionVariable(curCond);
}

void killCondition(void* forCondition){
	CONDITION_VARIABLE* curCond = (CONDITION_VARIABLE*)forCondition;
	free(curCond);
}

typedef struct{
	HMODULE dllMod;
} DLLibStruct;

void* loadInDLL(const char* loadFrom){
	HMODULE dllMod = LoadLibrary(loadFrom);
	if(dllMod){
		DLLibStruct* toRet = (DLLibStruct*)malloc(sizeof(DLLibStruct));
		toRet->dllMod = dllMod;
		return toRet;
	}
	return 0;
}

void* getDLLLocation(void* fromDLL, const char* locName){
	DLLibStruct* toRet = (DLLibStruct*)fromDLL;
	return (void*)(GetProcAddress(toRet->dllMod, locName));
}

void unloadDLL(void* toKill){
	DLLibStruct* toRet = (DLLibStruct*)toKill;
	FreeLibrary(toRet->dllMod);
	free(toRet);
}

bool directoryExists(const char* dirName){
	DWORD dwAttrib = GetFileAttributes(dirName);
	return ((dwAttrib != INVALID_FILE_ATTRIBUTES) && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

int makeDirectory(const char* dirName){
	return !CreateDirectory(dirName, 0);
}

void killDirectory(const char* dirName){
	RemoveDirectory(dirName);
}

uintptr_t* openDirectory(const char* dirName){
	WIN32_FIND_DATA findDat;
	std::string searchName(dirName);
		searchName.push_back('\\');
		searchName.push_back('*');
	std::vector<bool> allFolds;
	std::vector<std::string> allNames;
	HANDLE hFind = FindFirstFile(searchName.c_str(), &findDat);
	if(hFind == INVALID_HANDLE_VALUE){ return 0; }
	uintptr_t totNameLen = 0;
	do{
		allFolds.push_back((findDat.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0);
		allNames.push_back(std::string(findDat.cFileName));
		totNameLen += (allNames[allNames.size()-1].size()+2);
	}while(FindNextFile(hFind, &findDat));
	FindClose(hFind);
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
