#include "whodun_align_affinepd.h"
#include "whodun_align_affinepd_private.h"

#include <string.h>
#include <iostream>

/**
 * This will find the entries in a whitespace delimited text.
 * @param numChars The number of characters.
 * @param toParse The line to parse.
 * @param entryS The place to put the starts of the entries.
 * @param entryE The place to put the ends of the entries.
 * @return The character after the end of the line.
 */
const char* parseDelimitedLine(int numChars, const char* toParse, std::vector<const char*>* entryS, std::vector<const char*>* entryE){
	entryS->clear();
	entryE->clear();
	if(numChars <= 0){
		return toParse;
	}
	const char* nextFoc = (const char*)memchr(toParse, '\n', numChars);
		if(nextFoc){
			nextFoc++;
		}
		else{
			nextFoc = toParse + numChars;
		}
	for(const char* curFoc = toParse; curFoc < nextFoc; curFoc++){
		if(strchr(WHITESPACE, *curFoc)){
			if(entryS->size() != entryE->size()){
				entryE->push_back(curFoc);
			}
		}
		else{
			if(entryS->size() == entryE->size()){
				entryS->push_back(curFoc);
			}
		}
	}
	if(entryS->size() != entryE->size()){
		entryE->push_back(nextFoc);
	}
	return nextFoc;
}

void freePositionDependentQualityChange(PositionDependentQualityChanges* toKill){
	free(toKill->fromChars);
	free(toKill->toChars);
	for(int i = 0; i<toKill->numEntries; i++){
		free(toKill->charMangTodo[i]);
	}
	free(toKill->charMangTodo);
	free(toKill->gapOpenTodo);
	free(toKill->gapCloseTodo);
	free(toKill->gapExtendTodo);
	free(toKill);
}

void freePositionDependentQualityChangeSet(PositionDependentQualityChangeSet* toKill){
	free(toKill->allQualLow);
	free(toKill->allQualHigh);
	for(int i = 0; i<toKill->numQuals; i++){
		freePositionDependentQualityChange(toKill->allQChange[i]);
	}
	free(toKill->allQChange);
	free(toKill);
}

int parseReadQualityEffectFile(int numChars, const char* toParse, std::map<std::string, PositionDependentQualityChangeSet* >* toFill){
	const char* focPar = toParse;
	int cnumC = numChars;
	std::vector<const char*> lineEntS;
	std::vector<const char*> lineEntE;
	//state
		bool haveRef = false;
		bool haveQual = false;
		std::string curRef = "";
		unsigned char lowQual = 0;
		unsigned char highQual = 0;
		std::string qualRef;
		std::string qualRead;
		std::vector<std::string> qualMang;
		std::string openMang;
		std::string closeMang;
		std::string extendMang;
		std::vector<PositionDependentQualityChanges*> curRefStore;
		std::string curRefQualLow;
		std::string curRefQualHigh;
	//loop through
	while(cnumC){
		const char* nextFoc = parseDelimitedLine(cnumC, focPar, &lineEntS, &lineEntE);
		if(lineEntS.size()){
			const char* curComS = lineEntS[0];
			switch(*curComS){
				case 'r':
					{
						if(haveRef){
							std::cerr << "Need to close a reference before starting a new one." << std::endl;
							goto errorHandle;
						}
						if(lineEntS.size() != 2){
							std::cerr << "Malformed reference entry." << std::endl;
							goto errorHandle;
						}
						std::string newRName(lineEntS[1], lineEntE[1]);
						curRef = newRName;
						haveRef = true;
					}
					break;
				case 'q':
					{
						if(haveQual){
							std::cerr << "Need to close a quality entry before starting a new one." << std::endl;
							goto errorHandle;
						}
						if((lineEntS.size() < 2) || (lineEntS.size() > 3)){
							std::cerr << "Malformed quality." << std::endl;
							goto errorHandle;
						}
						lowQual = 0x00FF & atoi(lineEntS[1]);
						if(lineEntS.size() == 3){
							highQual = 0x00FF & atoi(lineEntS[2]);
						}
						else{
							highQual = lowQual;
						}
						haveQual = true;
					}
					break;
				case '~':
					{
						if(!haveQual){
							std::cerr << "Need to open a quality entry before adding modifications." << std::endl;
							goto errorHandle;
						}
						if(lineEntS.size() < 4){
							std::cerr << "Malformed transliteration entry." << std::endl;
							goto errorHandle;
						}
						qualRef.push_back(atoi(lineEntS[1]));
						qualRead.push_back(atoi(lineEntS[2]));
						std::string curMang(lineEntS[3], lineEntE[lineEntS.size()-1]);
						qualMang.push_back(curMang);
					}
					break;
				case 'G':
					{
						const char* curComE = lineEntE[0];
						if((curComS+1) == curComE){
							std::cerr << "Unknown command " << *curComS << std::endl;
							goto errorHandle;
						}
						if(lineEntS.size() < 2){
							std::cerr << "Malformed gap entry." << std::endl;
							goto errorHandle;
						}
						std::string curMang(lineEntS[1], lineEntE[lineEntS.size()-1]);
						switch(curComS[1]){
							case 'o':
								openMang = curMang;
								break;
							case 'x':
								extendMang = curMang;
								break;
							case 'c':
								closeMang = curMang;
								break;
							default:
								std::cerr << "Unknown command " << curComS[0] << curComS[1] << std::endl;
								goto errorHandle;
						}
					}
					break;
				case 'Q':
					{
						if(!haveQual){
							std::cerr << "Missing quality." << std::endl;
							goto errorHandle;
						}
						PositionDependentQualityChanges* curQC = (PositionDependentQualityChanges*)malloc(sizeof(PositionDependentQualityChanges));
						int numComp = qualRef.size();
						curQC->numEntries = numComp;
						curQC->fromChars = (char*)malloc(numComp);
						curQC->toChars = (char*)malloc(numComp);
						curQC->charMangTodo = (char**)malloc(numComp*sizeof(char*));
						for(int j = 0; j<numComp; j++){
							curQC->fromChars[j] = qualRef[j];
							curQC->toChars[j] = qualRead[j];
							curQC->charMangTodo[j] = strdup(qualMang[j].c_str());
						}
						curQC->gapOpenTodo = strdup(openMang.c_str());
						curQC->gapCloseTodo = strdup(closeMang.c_str());
						curQC->gapExtendTodo = strdup(extendMang.c_str());
						curRefQualLow.push_back(lowQual);
						curRefQualHigh.push_back(highQual);
						curRefStore.push_back(curQC);
						qualRef.clear();
						qualRead.clear();
						qualMang.clear();
						openMang.clear();
						closeMang.clear();
						extendMang.clear();
						haveQual = false;
					}
					break;
				case 'R':
					{
						if(!haveRef){
							std::cerr << "Missing reference." << std::endl;
							goto errorHandle;
						}
						std::map<std::string, PositionDependentQualityChangeSet*>::iterator foundRN = toFill->find(curRef);
						if(foundRN != toFill->end()){
							std::cerr << "Duplicate reference entry " << curRef << std::endl;
							goto errorHandle;
						}
						PositionDependentQualityChangeSet* curRSet = (PositionDependentQualityChangeSet*)malloc(sizeof(PositionDependentQualityChangeSet));
						curRSet->numQuals = curRefStore.size();
						curRSet->allQualLow = (unsigned char*)(strdup(curRefQualLow.c_str()));
						curRSet->allQualHigh = (unsigned char*)(strdup(curRefQualHigh.c_str()));
						curRSet->allQChange = (PositionDependentQualityChanges**)malloc(sizeof(PositionDependentQualityChanges*)*curRefStore.size());
						std::copy(curRefStore.begin(), curRefStore.end(), curRSet->allQChange);
						curRefStore.clear();
						curRefQualLow.clear();
						curRefQualHigh.clear();
						(*toFill)[curRef] = curRSet;
						haveRef = false;
					}
					break;
				default:
					std::cerr << "Unknown command " << *curComS << std::endl;
					goto errorHandle;
			}
		}
		cnumC -= (nextFoc - focPar);
		focPar = nextFoc;
	}
	return 0;
	errorHandle:
	for(unsigned i = 0; i<curRefStore.size(); i++){
		freePositionDependentQualityChange(curRefStore[i]);
	}
	return 1;
}

/**
 * This will perform a quality action.
 * @param toPerf The action to perform.
 * @param startV The start value.
 * @param endV The place to put the end value.
 * @return whether there was a problem.
 */
int performQualityDependentAction(const char* toPerf, int startV, int* endV){
	const char* curFoc = toPerf;
	const char* nextFoc;
	int curV = startV;
	while(*curFoc){
		switch(*curFoc){
			case '+':
				nextFoc = curFoc + 1;
				nextFoc += strspn(nextFoc, WHITESPACE);
				if((*nextFoc == 0) || (strspn(nextFoc, DIGITS) == 0)){
					std::cerr << "Missing number for addition." << std::endl;
					return 1;
				}
				curV = curV + atoi(nextFoc);
				curFoc = nextFoc + strspn(nextFoc, DIGITS);
				break;
			case '-':
				nextFoc = curFoc + 1;
				nextFoc += strspn(nextFoc, WHITESPACE);
				if((*nextFoc == 0) || (strspn(nextFoc, DIGITS) == 0)){
					std::cerr << "Missing number for subtraction." << std::endl;
					return 1;
				}
				curV = curV - atoi(nextFoc);
				curFoc = nextFoc + strspn(nextFoc, DIGITS);
				break;
			case '*':
				nextFoc = curFoc + 1;
				nextFoc += strspn(nextFoc, WHITESPACE);
				if((*nextFoc == 0) || (strspn(nextFoc, DIGITS) == 0)){
					std::cerr << "Missing number for multiplication." << std::endl;
					return 1;
				}
				curV = curV * atoi(nextFoc);
				curFoc = nextFoc + strspn(nextFoc, DIGITS);
				break;
			case '/':
				nextFoc = curFoc + 1;
				nextFoc += strspn(nextFoc, WHITESPACE);
				if((*nextFoc == 0) || (strspn(nextFoc, DIGITS) == 0)){
					std::cerr << "Missing number for division." << std::endl;
					return 1;
				}
				curV = curV / atoi(nextFoc);
				curFoc = nextFoc + strspn(nextFoc, DIGITS);
				break;
			case '=':
				nextFoc = curFoc + 1;
				nextFoc += strspn(nextFoc, WHITESPACE);
				if((*nextFoc == 0) || (strspn(nextFoc, DIGITS) == 0)){
					std::cerr << "Missing number for set." << std::endl;
					return 1;
				}
				curV = atoi(nextFoc);
				curFoc = nextFoc + strspn(nextFoc, DIGITS);
				break;
			case '\n':
			case '\r':
			case '\t':
			case ' ':
				curFoc++;
				break;
			default:
				std::cerr << "Unknown command " << *curFoc << std::endl;
				return 1;
		}
	}
	*endV = curV;
	return 0;
}

/**
 * This will find the relevant set of changes for a quality.
 * @a qualGuide The full set of changes.
 * @a forQual The quality in question.
 * @r The relevant changes.
 */
PositionDependentQualityChanges* findRelevantChangeSet(PositionDependentQualityChangeSet* qualGuide, unsigned char forQual){
	for(int i = 0; i<qualGuide->numQuals; i++){
		if(qualGuide->allQualLow[i] <= forQual){
			if(qualGuide->allQualHigh[i] >= forQual){
				return qualGuide->allQChange[i];
			}
		}
	}
	return 0;
}

/**Too many arguments to a function.*/
typedef struct{
	/**The start positions in the reference.*/
	std::vector<int>* fromRef;
	/**The end positions in the reference.*/
	std::vector<int>* toRef;
	/**The start positions in the read.*/
	std::vector<int>* fromRead;
	/**The end positions in the read.*/
	std::vector<int>* toRead;
	/**The relavent parameters.*/
	std::vector<AlignCostAffine*>* useParam;
	/**The low reference index in play.*/
	int fromIndRef;
	/**The high reference index in play.*/
	int toIndRef;
	/**The low read index in play.*/
	int fromIndRead;
	/**The high read index in play.*/
	int toIndRead;
} PositionRectangleArgs;

void findEquivalentRectangleRegions(PositionDependentAlignCostKDNode* startPoint, PositionRectangleArgs* pargs){
	if(pargs->fromIndRef == pargs->toIndRef){
		return;
	}
	if(pargs->fromIndRead == pargs->toIndRead){
		return;
	}
	int fromIA = pargs->fromIndRef;
	int fromIB = pargs->fromIndRead;
	AlignCostAffine* curCP = getPositionDependentAlignmentCosts(startPoint, fromIA, fromIB);
	int toIA = fromIA + 1;
	int toIB = fromIB + 1;
	bool canExpRef = toIA < pargs->toIndRef;
	bool canExpRead = toIB < pargs->toIndRead;
	bool wasExp = true;
	while(wasExp){
		wasExp = false;
		//try to expand along reference
		if(canExpRef){
			for(int i = fromIB; i<toIB; i++){
				AlignCostAffine* nextCP = getPositionDependentAlignmentCosts(startPoint, toIA, i);
				if(nextCP != curCP){
					canExpRef = false;
					break;
				}
			}
			if(canExpRef){
				wasExp = true;
				toIA++;
				canExpRef = toIA < pargs->toIndRef;
			}
		}
		//try to expand along the read
		if(canExpRead){
			for(int i = fromIA; i<toIA; i++){
				AlignCostAffine* nextCP = getPositionDependentAlignmentCosts(startPoint, i, toIB);
				if(nextCP != curCP){
					canExpRead = false;
					break;
				}
			}
			if(canExpRead){
				wasExp = true;
				toIB++;
				canExpRead = toIB < pargs->toIndRead;
			}
		}
	}
	pargs->fromRef->push_back(fromIA);
	pargs->toRef->push_back(toIA);
	pargs->fromRead->push_back(fromIB);
	pargs->toRead->push_back(toIB);
	pargs->useParam->push_back(curCP);
	PositionRectangleArgs nargs;
	nargs = *pargs; nargs.fromIndRef = toIA;
		findEquivalentRectangleRegions(startPoint, &nargs);
	nargs = *pargs; nargs.fromIndRead = toIB; nargs.toIndRef = toIA;
		findEquivalentRectangleRegions(startPoint, &nargs);
}

PositionDependentAlignCostKDNode* qualifyPositionDependentSpecification(PositionDependentAlignCostKDNode* startPoint, PositionDependentQualityChangeSet* qualGuide, int readLen, const char* readQual, int refLen){
	//flatten the tree
		std::vector<PositionDependentBounds*> allFlat;
		flattenPositionDependentKD(startPoint, &allFlat);
	//copy and cut anything outside the read and reference
		std::vector<PositionDependentBounds*> winBound;
		for(unsigned i = 0; i<allFlat.size(); i++){
			PositionDependentBounds* curBnd = allFlat[i];
			int fromLowA = curBnd->startA;
			if(fromLowA < 0){
				PositionDependentBounds* newBnd = (PositionDependentBounds*)malloc(sizeof(PositionDependentBounds));
				newBnd->startA = curBnd->startA;
				newBnd->endA = 0;
				newBnd->startB = curBnd->startB;
				newBnd->endB = curBnd->endB;
				newBnd->regCosts = cloneAlignCostAffine(curBnd->regCosts);
				fromLowA = 0;
				winBound.push_back(newBnd);
			}
			int toHighA = curBnd->endA;
			if(curBnd->endA < 0){
				PositionDependentBounds* newBnd = (PositionDependentBounds*)malloc(sizeof(PositionDependentBounds));
				newBnd->startA = refLen;
				newBnd->endA = curBnd->endA;
				newBnd->startB = curBnd->startB;
				newBnd->endB = curBnd->endB;
				newBnd->regCosts = cloneAlignCostAffine(curBnd->regCosts);
				toHighA = refLen;
				winBound.push_back(newBnd);
			}
			if(curBnd->startB < 0){
				PositionDependentBounds* newBnd = (PositionDependentBounds*)malloc(sizeof(PositionDependentBounds));
				newBnd->startA = fromLowA;
				newBnd->endA = toHighA;
				newBnd->startB = curBnd->startB;
				newBnd->endB = 0;
				newBnd->regCosts = cloneAlignCostAffine(curBnd->regCosts);
				winBound.push_back(newBnd);
			}
			if(curBnd->endB < 0){
				PositionDependentBounds* newBnd = (PositionDependentBounds*)malloc(sizeof(PositionDependentBounds));
				newBnd->startA = fromLowA;
				newBnd->endA = toHighA;
				newBnd->startB = readLen;
				newBnd->endB = curBnd->endB;
				newBnd->regCosts = cloneAlignCostAffine(curBnd->regCosts);
				winBound.push_back(newBnd);
			}
		}
	//look for runs of quality
	int focInd = 0;
	while(focInd < readLen){
		unsigned char cfqual = readQual[focInd];
		PositionDependentQualityChanges* cfChange = findRelevantChangeSet(qualGuide, cfqual);
		int runEI = readLen;
		for(int i = focInd + 1; i<readLen; i++){
			unsigned char curQ = readQual[i];
			PositionDependentQualityChanges* curChange = findRelevantChangeSet(qualGuide, curQ);
			if(curChange != cfChange){
				runEI = i;
				break;
			}
		}
		//find the equivalent regions
		std::vector<int> fromRef;
		std::vector<int> toRef;
		std::vector<int> fromRead;
		std::vector<int> toRead;
		std::vector<AlignCostAffine*> useParam;
		PositionRectangleArgs newRects = {&fromRef, &toRef, &fromRead, &toRead, &useParam, 0, refLen, focInd, runEI};
		findEquivalentRectangleRegions(startPoint, &newRects);
		//run the modifiers and add
		for(unsigned i = 0; i<fromRef.size(); i++){
			PositionDependentBounds* newBnd = (PositionDependentBounds*)malloc(sizeof(PositionDependentBounds));
			newBnd->startA = fromRef[i];
			newBnd->endA = toRef[i];
			newBnd->startB = fromRead[i];
			newBnd->endB = toRead[i];
			newBnd->regCosts = cloneAlignCostAffine(useParam[i]);
			winBound.push_back(newBnd);
			if(cfChange){
				int* valLoc;
				for(int j = 0; j<cfChange->numEntries; j++){
					int entryIndA = newBnd->regCosts->charMap[0x00FF&(cfChange->fromChars[j])];
					int entryIndB = newBnd->regCosts->charMap[0x00FF&(cfChange->toChars[j])];
					valLoc = &(newBnd->regCosts->allMMCost[entryIndA][entryIndB]);
					if(performQualityDependentAction(cfChange->charMangTodo[j], *valLoc, valLoc)){
						goto errortarget;
					}
				}
				valLoc = &(newBnd->regCosts->openCost);
				if(performQualityDependentAction(cfChange->gapOpenTodo, *valLoc, valLoc)){
					goto errortarget;
				}
				valLoc = &(newBnd->regCosts->extendCost);
				if(performQualityDependentAction(cfChange->gapExtendTodo, *valLoc, valLoc)){
					goto errortarget;
				}
				valLoc = &(newBnd->regCosts->closeCost);
				if(performQualityDependentAction(cfChange->gapCloseTodo, *valLoc, valLoc)){
					goto errortarget;
				}
			}
		}
		//prepare for the next round
		focInd = runEI;
	}
	//set priority
	for(unsigned i = 0; i<winBound.size(); i++){
		PositionDependentBounds* newBnd = winBound[i];
		newBnd->priority = 0;
		newBnd->priority += ((newBnd->startA >= 0) ? 1 : 0);
		newBnd->priority += ((newBnd->endA >= 0) ? 1 : 0);
		newBnd->priority += ((newBnd->startB >= 0) ? 1 : 0);
		newBnd->priority += ((newBnd->endB >= 0) ? 1 : 0);
	}
	return buildPositionDependentKDTree(&winBound);
	errortarget:
		for(unsigned k = 0; k<winBound.size(); k++){
			freeAlignCostAffine(winBound[k]->regCosts);
			free(winBound[k]);
		}
		return 0;
}