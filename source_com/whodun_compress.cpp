#include "whodun_compress.h"

#include <string.h>
#include <stdlib.h>

#include "whodun_oshook.h"

CompressionMethod::~CompressionMethod(){}

RawOutputCompressionMethod::~RawOutputCompressionMethod(){}

void* RawOutputCompressionMethod::comfopen(const char* fileName, const char* mode){
	FILE* toRet = fopen(fileName, mode);
	return toRet;
}

size_t RawOutputCompressionMethod::comfwrite(const void* ptr, size_t size, size_t count, void* stream){
	FILE* toRet = (FILE*)stream;
	return fwrite(ptr, size, count, toRet);
}

size_t RawOutputCompressionMethod::comfread(void* ptr, size_t size, size_t count, void* stream){
	FILE* toRet = (FILE*)stream;
	return fread(ptr, size, count, toRet);
}

intptr_t RawOutputCompressionMethod::comftell(void* stream){
	FILE* toRet = (FILE*)stream;
	return ftellPointer(toRet);
}

int RawOutputCompressionMethod::comfclose(void* stream){
	FILE* toRet = (FILE*)stream;
	return fclose(toRet);
}

uintptr_t* RawOutputCompressionMethod::decompressData(uintptr_t datLen, char* data){
	uintptr_t* toRet = (uintptr_t*)malloc(sizeof(uintptr_t) + datLen);
	*toRet = datLen;
	char* toWork = (char*)(toRet+1);
	memcpy(toWork, data, datLen);
	return toRet;
}

uintptr_t* RawOutputCompressionMethod::compressData(uintptr_t datLen, char* data){
	uintptr_t* toRet = (uintptr_t*)malloc(sizeof(uintptr_t) + datLen);
	*toRet = datLen;
	char* toWork = (char*)(toRet+1);
	memcpy(toWork, data, datLen);
	return toRet;
}

