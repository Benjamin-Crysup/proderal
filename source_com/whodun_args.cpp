#include "whodun_args.h"

#include <string.h>

#define LOAD_CHUNK_SIZE 2048

ArgumentRedirectParser::ArgumentRedirectParser(int argc, char** argv){
	std::vector<uintptr_t> tmpOffsets;
	for(int i = 0; i<argc; i++){
		char* curArg = argv[i];
		uintptr_t cargLen = strlen(curArg);
		//handle common argument
		if(curArg[0] != '@'){
			tmpOffsets.push_back(allText.size());
			allText.insert(allText.end(), curArg, curArg + cargLen + 1);
			continue;
		}
		
		//load the file
		std::vector<char> subText;
		FILE* fileLoad = fopen(curArg + 1, "rb");
		if(fileLoad == 0){
			std::string errMess("Could not open argument file: ");
			errMess.append(curArg+1);
			throw std::runtime_error(errMess);
		}
		char loadBuff[LOAD_CHUNK_SIZE];
		uintptr_t numLoad = fread(loadBuff, 1, LOAD_CHUNK_SIZE, fileLoad);
		while(numLoad){
			subText.insert(subText.end(), loadBuff, loadBuff + numLoad);
			numLoad = fread(loadBuff, 1, LOAD_CHUNK_SIZE, fileLoad);
		}
		fclose(fileLoad);
		//replace all newlines with 0
		for(uintptr_t j = 0; j<subText.size(); j++){
			if((subText[j] == '\r') || (subText[j] == '\n')){
				subText[j] = 0;
			}
		}
		subText.push_back(0);
		//block arguments
		std::vector<char*> subArgs;
		uintptr_t sj = 0;
		while(sj < subText.size()){
			char* curSub = &(subText[sj]);
			uintptr_t curSubL = strlen(curSub);
			if(curSubL){
				subArgs.push_back(curSub);
			}
			sj += (curSubL + 1);
		}
		//recurse
		ArgumentRedirectParser subRed(subArgs.size(), subArgs.size() ? &(subArgs[0]) : (char**)0);
		//and claim arguments
		for(uintptr_t j = 0; j<subRed.allArgs.size(); j++){
			char* curSub = subRed.allArgs[j];
			uintptr_t curSubL = strlen(curSub);
			tmpOffsets.push_back(allText.size());
			allText.insert(allText.end(), curSub, curSub + curSubL + 1);
		}
	}
	//figure out the pointers
	for(uintptr_t i = 0; i<tmpOffsets.size(); i++){
		allArgs.push_back(&(allText[tmpOffsets[i]]));
	}
}

ArgumentRedirectParser::~ArgumentRedirectParser(){}

ArgumentParserBoolMeta::ArgumentParserBoolMeta(){
	hidden = false;
}
ArgumentParserBoolMeta::ArgumentParserBoolMeta(const char* presName){
	presentName = presName;
	hidden = false;
}
ArgumentParserBoolMeta::ArgumentParserBoolMeta(const ArgumentParserBoolMeta& toCopy){
	presentName = toCopy.presentName;
	hidden = toCopy.hidden;
}
ArgumentParserBoolMeta& ArgumentParserBoolMeta::operator=(const ArgumentParserBoolMeta& toCopy){
	presentName = toCopy.presentName;
	hidden = toCopy.hidden;
	return *this;
}

ArgumentParserEnumMeta::ArgumentParserEnumMeta(){
	hidden = false;
}
ArgumentParserEnumMeta::ArgumentParserEnumMeta(const char* presName){
	presentName = presName;
	hidden = false;
}
ArgumentParserEnumMeta::ArgumentParserEnumMeta(const ArgumentParserEnumMeta& toCopy){
	presentName = toCopy.presentName;
	hidden = toCopy.hidden;
	enumGroup = toCopy.enumGroup;
}
ArgumentParserEnumMeta& ArgumentParserEnumMeta::operator=(const ArgumentParserEnumMeta& toCopy){
	presentName = toCopy.presentName;
	hidden = toCopy.hidden;
	enumGroup = toCopy.enumGroup;
	return *this;
}

ArgumentParserIntMeta::ArgumentParserIntMeta(){
	hidden = false;
}
ArgumentParserIntMeta::ArgumentParserIntMeta(const char* presName){
	presentName = presName;
	hidden = false;
}
ArgumentParserIntMeta::ArgumentParserIntMeta(const ArgumentParserIntMeta& toCopy){
	presentName = toCopy.presentName;
	hidden = toCopy.hidden;
}
ArgumentParserIntMeta& ArgumentParserIntMeta::operator=(const ArgumentParserIntMeta& toCopy){
	presentName = toCopy.presentName;
	hidden = toCopy.hidden;
	return *this;
}

ArgumentParserFltMeta::ArgumentParserFltMeta(){
	hidden = false;
}
ArgumentParserFltMeta::ArgumentParserFltMeta(const char* presName){
	presentName = presName;
	hidden = false;
}
ArgumentParserFltMeta::ArgumentParserFltMeta(const ArgumentParserFltMeta& toCopy){
	presentName = toCopy.presentName;
	hidden = toCopy.hidden;
}
ArgumentParserFltMeta& ArgumentParserFltMeta::operator=(const ArgumentParserFltMeta& toCopy){
	presentName = toCopy.presentName;
	hidden = toCopy.hidden;
	return *this;
}

ArgumentParserStrMeta::ArgumentParserStrMeta(){
	hidden = false;
	isFile = false;
	fileWrite = false;
	isFolder = false;
	foldWrite = false;
}
ArgumentParserStrMeta::ArgumentParserStrMeta(const char* presName){
	presentName = presName;
	hidden = false;
	isFile = false;
	fileWrite = false;
	isFolder = false;
	foldWrite = false;
}
ArgumentParserStrMeta::ArgumentParserStrMeta(const ArgumentParserStrMeta& toCopy){
	presentName = toCopy.presentName;
	hidden = toCopy.hidden;
	isFile = toCopy.isFile;
	fileWrite = toCopy.fileWrite;
	fileExts = toCopy.fileExts;
	isFolder = toCopy.isFolder;
	foldWrite = toCopy.foldWrite;
}
ArgumentParserStrMeta& ArgumentParserStrMeta::operator=(const ArgumentParserStrMeta& toCopy){
	presentName = toCopy.presentName;
	hidden = toCopy.hidden;
	isFile = toCopy.isFile;
	fileWrite = toCopy.fileWrite;
	fileExts = toCopy.fileExts;
	isFolder = toCopy.isFolder;
	foldWrite = toCopy.foldWrite;
	return *this;
}

ArgumentParserIntVecMeta::ArgumentParserIntVecMeta(){
	hidden = false;
}
ArgumentParserIntVecMeta::ArgumentParserIntVecMeta(const char* presName){
	presentName = presName;
	hidden = false;
}
ArgumentParserIntVecMeta::ArgumentParserIntVecMeta(const ArgumentParserIntVecMeta& toCopy){
	presentName = toCopy.presentName;
	hidden = toCopy.hidden;
}
ArgumentParserIntVecMeta& ArgumentParserIntVecMeta::operator=(const ArgumentParserIntVecMeta& toCopy){
	presentName = toCopy.presentName;
	hidden = toCopy.hidden;
	return *this;
}

ArgumentParserFltVecMeta::ArgumentParserFltVecMeta(){
	hidden = false;
}
ArgumentParserFltVecMeta::ArgumentParserFltVecMeta(const char* presName){
	presentName = presName;
	hidden = false;
}
ArgumentParserFltVecMeta::ArgumentParserFltVecMeta(const ArgumentParserFltVecMeta& toCopy){
	presentName = toCopy.presentName;
	hidden = toCopy.hidden;
}
ArgumentParserFltVecMeta& ArgumentParserFltVecMeta::operator=(const ArgumentParserFltVecMeta& toCopy){
	presentName = toCopy.presentName;
	hidden = toCopy.hidden;
	return *this;
}

ArgumentParserStrVecMeta::ArgumentParserStrVecMeta(){
	hidden = false;
	isFile = false;
	fileWrite = false;
	isFolder = false;
	foldWrite = false;
}
ArgumentParserStrVecMeta::ArgumentParserStrVecMeta(const char* presName){
	presentName = presName;
	hidden = false;
	isFile = false;
	fileWrite = false;
	isFolder = false;
	foldWrite = false;
}
ArgumentParserStrVecMeta::ArgumentParserStrVecMeta(const ArgumentParserStrVecMeta& toCopy){
	presentName = toCopy.presentName;
	hidden = toCopy.hidden;
	isFile = toCopy.isFile;
	fileWrite = toCopy.fileWrite;
	fileExts = toCopy.fileExts;
	isFolder = toCopy.isFolder;
	foldWrite = toCopy.foldWrite;
}
ArgumentParserStrVecMeta& ArgumentParserStrVecMeta::operator=(const ArgumentParserStrVecMeta& toCopy){
	presentName = toCopy.presentName;
	hidden = toCopy.hidden;
	isFile = toCopy.isFile;
	fileWrite = toCopy.fileWrite;
	fileExts = toCopy.fileExts;
	isFolder = toCopy.isFolder;
	foldWrite = toCopy.foldWrite;
	return *this;
}

ArgumentParser::ArgumentParser(){
	needRun = 1;
	myMainDoc = "ADD SOME DAMN DOCUMENTATION!\n";
	myVersionDoc = "69 Bottles of Beer on the Wall\n";
	myCopyrightDoc = "Copyright (C) 1066 The French\n";
	myLicenseDoc = "License LGPLv3+: GNU LGPL version 3 or later\n    <https://www.gnu.org/licenses/lgpl-3.0.html>\nThis is free software: you are free to change and redistribute it.\n";
	myWarrantyDoc = "There is NO WARRANTY, to the extent permitted by law.\n";
}

ArgumentParser::~ArgumentParser(){
}

int ArgumentParser::parseArguments(int argc, char** argv, std::ostream* helpOut){
	int ci = 0;
	while(ci < argc){
		std::string carg(argv[ci]);
		if((carg == "--help") || (carg == "-h") || (carg == "/?")){
			printHelpDocumentation(helpOut);
			needRun = 0;
			return ci+1;
		}
		if(carg == "--version"){
			printVersionInformation(helpOut);
			needRun = 0;
			return ci+1;
		}
		if(carg == "--helpdumpgui"){
			printGUIInformation(helpOut);
			needRun = 0;
			return ci+1;
		}
		if(boolHotFlags.find(carg) != boolHotFlags.end()){
			*boolHotFlags[carg] = true;
			ci++; continue;
		}
		if(boolCoolFlags.find(carg) != boolCoolFlags.end()){
			*boolCoolFlags[carg] = false;
			ci++; continue;
		}
		if(enumSettings.find(carg) != enumSettings.end()){
			*enumSettings[carg] = enumPossible[carg];
			ci++; continue;
		}
		#define SIMPLE_SETTING(theSettings, theIdjits, valueCode) \
			if(theSettings.find(carg) != theSettings.end()){\
				ci++;\
				if(ci >= argc){\
					argumentError = carg + " missing setting.";\
					return -1;\
				}\
				valueCode\
				if(theIdjits.find(carg) != theIdjits.end()){\
					const char* errMess = theIdjits[carg](newValue);\
					if(errMess){\
						argumentError = errMess;\
						return -1;\
					}\
				}\
				*theSettings[carg] = newValue;\
				ci++; continue;\
			}
		SIMPLE_SETTING(intSettings, intIdjits, intptr_t newValue = atol(argv[ci]);)
		SIMPLE_SETTING(floatSettings, floatIdjits, double newValue = atof(argv[ci]);)
		SIMPLE_SETTING(stringSettings, stringIdjits, char* newValue = argv[ci];)
		#define VECTOR_SETTING(theSettings, theIdjits, valueCode) \
			if(theSettings.find(carg) != theSettings.end()){\
				ci++;\
				if(ci >= argc){\
					argumentError = carg + " missing setting.";\
					return -1;\
				}\
				valueCode\
				if(theIdjits.find(carg) != theIdjits.end()){\
					const char* errMess = theIdjits[carg](newValue);\
					if(errMess){\
						argumentError = errMess;\
						return -1;\
					}\
				}\
				theSettings[carg]->push_back(newValue);\
				ci++; continue;\
			}
		VECTOR_SETTING(intVecSettings, intVecIdjits, intptr_t newValue = atol(argv[ci]);)
		VECTOR_SETTING(floatVecSettings, floatVecIdjits, double newValue = atof(argv[ci]);)
		VECTOR_SETTING(stringVecSettings, stringVecIdjits, char* newValue = argv[ci];)
		int unkUseOpts = handleUnknownArgument(argc - ci, argv + ci, helpOut);
		if(unkUseOpts < 0){
			return -1;
		}
		if(unkUseOpts == 0){ break; }
		ci += unkUseOpts;
		if(needRun == 0){ return ci; }
	}
	if(posteriorCheck()){
		return -1;
	}
	return ci;
}

int ArgumentParser::parseArguments(ArgumentRedirectParser* loadArgs, std::ostream* helpOut){
	return parseArguments(loadArgs->allArgs.size(), loadArgs->allArgs.size() ? &(loadArgs->allArgs[0]) : (char**)0, helpOut);
}

void ArgumentParser::printHelpDocumentation(std::ostream* toPrint){
	(*toPrint) << myMainDoc;
	for(std::map<std::string,const char*>::iterator argIt = argDocs.begin(); argIt != argDocs.end(); argIt++){
		(*toPrint) << argIt->first << std::endl;
		(*toPrint) << argIt->second;
	}
}

void ArgumentParser::printVersionInformation(std::ostream* toPrint){
	(*toPrint) << myVersionDoc;
	(*toPrint) << myCopyrightDoc;
	(*toPrint) << myLicenseDoc;
	(*toPrint) << myWarrantyDoc;
}

#define PRINT_GUI_GET_META(metType, metMap) \
	metType* curMeta = 0;\
	if(metMap.find(curArg) != metMap.end()){ curMeta = &(metMap[curArg]); }

#define PRINT_GUI_COMMON_META \
	if(curMeta->presentName.size()){\
		(*toPrint) << "\tNAME\t" << curMeta->presentName;\
	}\
	if(curMeta->hidden){\
		(*toPrint) << "\tPRIVATE";\
	}

#define PRINT_GUI_DOC \
	const char* baseDoc = argDocs[curArg];\
	(*toPrint) << "\tDOCUMENT\t";\
	while(*baseDoc){\
		(*toPrint) << hexConv[(*baseDoc >> 4) & 0x0F];\
		(*toPrint) << hexConv[*baseDoc & 0x0F];\
		baseDoc++;\
	}

void ArgumentParser::printGUIInformation(std::ostream* toPrint){
	const char* hexConv = "0123456789ABCDEF";
	for(uintptr_t ai = 0; ai < argOrder.size(); ai++){
		std::string curArg = argOrder[ai];
		if(boolHotFlags.find(curArg) != boolHotFlags.end()){
			(*toPrint) << "BOOL\t" << curArg << "\tTRUE";
			PRINT_GUI_GET_META(ArgumentParserBoolMeta, boolMeta)
			if(curMeta){
				PRINT_GUI_COMMON_META
			}
			PRINT_GUI_DOC
			(*toPrint) << std::endl;
		}
		else if(boolCoolFlags.find(curArg) != boolCoolFlags.end()){
			(*toPrint) << "BOOL\t" << curArg << "\tFALSE";
			PRINT_GUI_GET_META(ArgumentParserBoolMeta, boolMeta)
			if(curMeta){
				PRINT_GUI_COMMON_META
			}
			PRINT_GUI_DOC
			(*toPrint) << std::endl;
		}
		else if(enumSettings.find(curArg) != enumSettings.end()){
			intptr_t curValue = *(enumSettings[curArg]);
			intptr_t setValue = enumPossible[curArg];
			(*toPrint) << "ENUM\t" << curArg << "\t" << (curValue==setValue ? "TRUE" : "FALSE");
			PRINT_GUI_GET_META(ArgumentParserEnumMeta, enumMeta)
			if(curMeta){
				PRINT_GUI_COMMON_META
				if(curMeta->enumGroup.size()){
					(*toPrint) << "\tGROUP\t" << curMeta->enumGroup;
				}
			}
			PRINT_GUI_DOC
			(*toPrint) << std::endl;
		}
		else if(intSettings.find(curArg) != intSettings.end()){
			intptr_t curValue = *(intSettings[curArg]);
			(*toPrint) << "INT\t" << curArg << "\t" << curValue;
			PRINT_GUI_GET_META(ArgumentParserIntMeta, intMeta)
			if(curMeta){
				PRINT_GUI_COMMON_META
			}
			PRINT_GUI_DOC
			(*toPrint) << std::endl;
		}
		else if(floatSettings.find(curArg) != floatSettings.end()){
			double curValue = *(floatSettings[curArg]);
			(*toPrint) << "FLOAT\t" << curArg << "\t" << curValue;
			PRINT_GUI_GET_META(ArgumentParserFltMeta, floatMeta)
			if(curMeta){
				PRINT_GUI_COMMON_META
			}
			PRINT_GUI_DOC
			(*toPrint) << std::endl;
		}
		else if(stringSettings.find(curArg) != stringSettings.end()){
			char* curValue = *(stringSettings[curArg]);
			(*toPrint) << "STRING\t" << curArg << "\t" << (curValue ? curValue : "");
			PRINT_GUI_GET_META(ArgumentParserStrMeta, stringMeta)
			if(curMeta){
				PRINT_GUI_COMMON_META
				if(curMeta->isFile){
					(*toPrint) << "\tFILE\t" << (curMeta->fileWrite ? "WRITE" : "READ") << "\t" << curMeta->fileExts.size();
					for(std::set<std::string>::iterator extIt = curMeta->fileExts.begin(); extIt != curMeta->fileExts.end(); extIt++){
						(*toPrint) << "\t" << *extIt;
					}
				}
				if(curMeta->isFolder){
					(*toPrint) << "\tFOLDER\t" << (curMeta->foldWrite ? "WRITE" : "READ");
				}
			}
			PRINT_GUI_DOC
			(*toPrint) << std::endl;
		}
		else if(intVecSettings.find(curArg) != intVecSettings.end()){
			(*toPrint) << "INTVEC\t" << curArg;
			PRINT_GUI_GET_META(ArgumentParserIntVecMeta, intVecMeta)
			if(curMeta){
				PRINT_GUI_COMMON_META
			}
			PRINT_GUI_DOC
			(*toPrint) << std::endl;
		}
		else if(floatVecSettings.find(curArg) != floatVecSettings.end()){
			(*toPrint) << "FLOATVEC\t" << curArg;
			PRINT_GUI_GET_META(ArgumentParserFltVecMeta, floatVecMeta)
			if(curMeta){
				PRINT_GUI_COMMON_META
			}
			PRINT_GUI_DOC
			(*toPrint) << std::endl;
		}
		else if(stringVecSettings.find(curArg) != stringVecSettings.end()){
			(*toPrint) << "STRINGVEC\t" << curArg;
			PRINT_GUI_GET_META(ArgumentParserStrVecMeta, stringVecMeta)
			if(curMeta){
				PRINT_GUI_COMMON_META
				if(curMeta->isFile){
					(*toPrint) << "\tFILE\t" << (curMeta->fileWrite ? "WRITE" : "READ") << "\t" << curMeta->fileExts.size();
					for(std::set<std::string>::iterator extIt = curMeta->fileExts.begin(); extIt != curMeta->fileExts.end(); extIt++){
						(*toPrint) << "\t" << *extIt;
					}
				}
				if(curMeta->isFolder){
					(*toPrint) << "\tFOLDER\t" << (curMeta->foldWrite ? "WRITE" : "READ");
				}
			}
			PRINT_GUI_DOC
			(*toPrint) << std::endl;
		}
	}
	printExtraGUIInformation(toPrint);
}

void ArgumentParser::printExtraGUIInformation(std::ostream* toPrint){
	//by default, nothing
}

int ArgumentParser::handleUnknownArgument(int argc, char** argv, std::ostream* helpOut){
	argumentError.append("Unknown argument ");
	argumentError.append(argv[0]);
	return -1;
}

void ArgumentParser::addBooleanFlag(const char* flagName, bool* flagVal, int isHot, const char* argDoc, ArgumentParserBoolMeta* argMeta){
	std::string tmpName(flagName);
	argDocs[tmpName] = argDoc;
	argOrder.push_back(tmpName);
	(isHot ? boolHotFlags : boolCoolFlags)[tmpName] = flagVal;
	if(argMeta){ boolMeta[tmpName] = *argMeta; }
}

void ArgumentParser::addEnumerationFlag(const char* flagName, intptr_t* flagVal, intptr_t flagSet, const char* argDoc, ArgumentParserEnumMeta* argMeta){
	std::string tmpName(flagName);
	argDocs[tmpName] = argDoc;
	argOrder.push_back(tmpName);
	enumSettings[tmpName] = flagVal;
	enumPossible[tmpName] = flagSet;
	if(argMeta){ enumMeta[tmpName] = *argMeta; }
}

void ArgumentParser::addIntegerOption(const char* optName, intptr_t* optVal, const char* (*optCheck)(intptr_t), const char* argDoc, ArgumentParserIntMeta* argMeta){
	std::string tmpName(optName);
	argDocs[tmpName] = argDoc;
	argOrder.push_back(tmpName);
	intSettings[tmpName] = optVal;
	if(optCheck){ intIdjits[tmpName] = optCheck; }
	if(argMeta){ intMeta[tmpName] = *argMeta; }
}

void ArgumentParser::addFloatOption(const char* optName, double* optVal, const char* (*optCheck)(double), const char* argDoc, ArgumentParserFltMeta* argMeta){
	std::string tmpName(optName);
	argDocs[tmpName] = argDoc;
	argOrder.push_back(tmpName);
	floatSettings[tmpName] = optVal;
	if(optCheck){ floatIdjits[tmpName] = optCheck; }
	if(argMeta){ floatMeta[tmpName] = *argMeta; }
}

void ArgumentParser::addStringOption(const char* optName, char** optVal, const char* (*optCheck)(char*), const char* argDoc, ArgumentParserStrMeta* argMeta){
	std::string tmpName(optName);
	argDocs[tmpName] = argDoc;
	argOrder.push_back(tmpName);
	stringSettings[tmpName] = optVal;
	if(optCheck){ stringIdjits[tmpName] = optCheck; }
	if(argMeta){ stringMeta[tmpName] = *argMeta; }
}

void ArgumentParser::addIntegerVectorOption(const char* optName, std::vector<intptr_t>* optVal, const char* (*optCheck)(intptr_t), const char* argDoc, ArgumentParserIntVecMeta* argMeta){
	std::string tmpName(optName);
	argDocs[tmpName] = argDoc;
	argOrder.push_back(tmpName);
	intVecSettings[tmpName] = optVal;
	if(optCheck){ intVecIdjits[tmpName] = optCheck; }
	if(argMeta){ intVecMeta[tmpName] = *argMeta; }
}

void ArgumentParser::addFloatVectorOption(const char* optName, std::vector<double>* optVal, const char* (*optCheck)(double), const char* argDoc, ArgumentParserFltVecMeta* argMeta){
	std::string tmpName(optName);
	argDocs[tmpName] = argDoc;
	argOrder.push_back(tmpName);
	floatVecSettings[tmpName] = optVal;
	if(optCheck){ floatVecIdjits[tmpName] = optCheck; }
	if(argMeta){ floatVecMeta[tmpName] = *argMeta; }
}

void ArgumentParser::addStringVectorOption(const char* optName, std::vector<char*>* optVal, const char* (*optCheck)(char*), const char* argDoc, ArgumentParserStrVecMeta* argMeta){
	std::string tmpName(optName);
	argDocs[tmpName] = argDoc;
	argOrder.push_back(tmpName);
	stringVecSettings[tmpName] = optVal;
	if(optCheck){ stringVecIdjits[tmpName] = optCheck; }
	if(argMeta){ stringVecMeta[tmpName] = *argMeta; }
}

int ArgumentParser::posteriorCheck(){
	return 0;
}
