#include "whodun_datread.h"

#include <stdexcept>

OutStream::OutStream(){}

OutStream::~OutStream(){}

void OutStream::writeBytes(const char* toW, uintptr_t numW){
	for(uintptr_t i = 0; i<numW; i++){
		writeByte(toW[i]);
	}
}

void OutStream::flush(){}

InStream::InStream(){}

InStream::~InStream(){}

uintptr_t InStream::readBytes(char* toR, uintptr_t numR){
	for(uintptr_t i = 0; i<numR; i++){
		int curR = readByte();
		if(curR < 0){
			return i;
		}
		toR[i] = curR;
	}
	return numR;
}

ConsoleOutStream::ConsoleOutStream(){}
ConsoleOutStream::~ConsoleOutStream(){}
void ConsoleOutStream::writeByte(int toW){
	fputc(toW, stdout);
}
void ConsoleOutStream::writeBytes(const char* toW, uintptr_t numW){
	fwrite(toW, 1, numW, stdout);
}

ConsoleErrStream::ConsoleErrStream(){}
ConsoleErrStream::~ConsoleErrStream(){}
void ConsoleErrStream::writeByte(int toW){
	fputc(toW, stderr);
}

ConsoleInStream::ConsoleInStream(){}
ConsoleInStream::~ConsoleInStream(){}
int ConsoleInStream::readByte(){
	return fgetc(stdin);
}
uintptr_t ConsoleInStream::readBytes(char* toR, uintptr_t numR){
	return fread(toR, 1, numR, stdin);
}

FileOutStream::FileOutStream(int append, const char* fileName){
	myName = fileName;
	if(append){
		baseFile = fopen(fileName, "ab");
	}
	else{
		baseFile = fopen(fileName, "wb");
	}
	if(baseFile == 0){
		throw std::runtime_error("Could not open file " + myName);
	}
}
FileOutStream::~FileOutStream(){
	fclose(baseFile);
}
void FileOutStream::writeByte(int toW){
	if(fputc(toW, baseFile) < 0){
		throw std::runtime_error("Problem writing file " + myName);
	}
}
void FileOutStream::writeBytes(const char* toW, uintptr_t numW){
	if(fwrite(toW, 1, numW, baseFile) != numW){
		throw std::runtime_error("Problem writing file " + myName);
	}
}

FileInStream::FileInStream(const char* fileName){
	myName = fileName;
	baseFile = fopen(fileName, "rb");
	if(baseFile == 0){
		throw std::runtime_error("Could not open file " + myName);
	}
}
FileInStream::~FileInStream(){
	fclose(baseFile);
}
int FileInStream::readByte(){
	int toR = fgetc(baseFile);
	if((toR < 0) && ferror(baseFile)){
		throw std::runtime_error("Problem reading file " + myName);
	}
	return toR;
}
uintptr_t FileInStream::readBytes(char* toR, uintptr_t numR){
	return fread(toR, 1, numR, baseFile);
}

#define READ_CHUNK 1024

void readStream(InStream* readF, std::string* toFill){
	char buffer[READ_CHUNK];
	while(true){
		uintptr_t numR = readF->readBytes(buffer, READ_CHUNK);
		toFill->insert(toFill->end(), buffer, buffer + numR);
		if(numR != READ_CHUNK){
			break;
		}
	}
}
