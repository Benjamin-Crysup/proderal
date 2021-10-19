#include "whodun_fit_ranfor.h"

#include <algorithm>

#include "whodun_stringext.h"

DatapointKDSplit::DatapointKDSplit(){
	onDim = 0;
	memset(&atVal, 0, sizeof(DatapointCell));
}

DatapointKDSplit::~DatapointKDSplit(){}

DecisionTreeTraining::DecisionTreeTraining(){}

DecisionTreeTraining::~DecisionTreeTraining(){}

BinaryTreeNode<DatapointKDSplit>* DecisionTreeTraining::train(){
	idiotCheck();
	uintptr_t numCols = dataDesc->colTypes.size();
	//do some initial setup
	std::vector<uintptr_t> allLivePI = useRows;
	std::vector<uintptr_t> hotI0;
		hotI0.push_back(0);
	std::vector< BinaryTreeNode<DatapointKDSplit>* > hotLevels;
		BinaryTreeNode<DatapointKDSplit>* resTree = new BinaryTreeNode<DatapointKDSplit>();
		hotLevels.push_back(resTree);
	std::vector<uintptr_t> tmpLivePI;
	//build the tree
	while(hotLevels.size()){
		//get the current level
			BinaryTreeNode<DatapointKDSplit>* curFoc = hotLevels[hotLevels.size()-1];
			uintptr_t curI0 = hotI0[hotLevels.size()-1];
			uintptr_t curIN = allLivePI.size();
			hotI0.pop_back(); hotLevels.pop_back();
		//figure out how to split
			uintptr_t* liveIL = 0; if(curIN != curI0){ liveIL = &(allLivePI[curI0]); }
			std::pair<uintptr_t,DatapointCell> winSpl = figureIdealSplit(curIN - curI0, liveIL);
			uintptr_t winDim = winSpl.first;
			DatapointCell winDat = winSpl.second;
		//split the hot points according to the result
			tmpLivePI.clear();
			uintptr_t lowI0 = allLivePI.size();
			switch(dataDesc->colTypes[winDim]){
				case DATAPOINT_CELL_INT:
					for(uintptr_t pi = curI0; pi<curIN; pi++){
						uintptr_t pr = allLivePI[pi];
						if(allData[numCols*pr+winDim].idat <= winDat.idat){
							allLivePI.push_back(pr);
						}
						else{
							tmpLivePI.push_back(pr);
						}
					}
					break;
				case DATAPOINT_CELL_FLT:
					for(uintptr_t pi = curI0; pi<curIN; pi++){
						uintptr_t pr = allLivePI[pi];
						if(allData[numCols*pr+winDim].ddat <= winDat.ddat){
							allLivePI.push_back(pr);
						}
						else{
							tmpLivePI.push_back(pr);
						}
					}
					break;
				case DATAPOINT_CELL_CAT:
					for(uintptr_t pi = curI0; pi<curIN; pi++){
						uintptr_t pr = allLivePI[pi];
						if(allData[numCols*pr+winDim].cdat == winDat.cdat){
							allLivePI.push_back(pr);
						}
						else{
							tmpLivePI.push_back(pr);
						}
					}
					break;
				default:
					throw std::runtime_error("...dafuq?");
			}
			uintptr_t higI0 = allLivePI.size();
			allLivePI.insert(allLivePI.end(), tmpLivePI.begin(), tmpLivePI.end());
		//decide whether to split
			if((lowI0 == higI0) || (higI0 == allLivePI.size())){
				curFoc->payload.atLevel.insert(curFoc->payload.atLevel.end(), allLivePI.begin()+curI0, allLivePI.begin()+curIN);
				allLivePI.erase(allLivePI.begin()+curI0, allLivePI.end());
				continue;
			}
		//set up for the sub-levels
			allLivePI.erase(allLivePI.begin()+curI0, allLivePI.begin()+curIN);
			curFoc->payload.onDim = winDim;
			curFoc->payload.atVal = winDat;
			curFoc->leftC = new BinaryTreeNode<DatapointKDSplit>();
			curFoc->rightC = new BinaryTreeNode<DatapointKDSplit>();
			hotLevels.push_back(curFoc->leftC);
			hotLevels.push_back(curFoc->rightC);
			hotI0.push_back(lowI0 - (curIN-curI0));
			hotI0.push_back(higI0 - (curIN-curI0));
	}
	return resTree;
}

BinaryTreeNode<DatapointKDSplit>* datapointTreeFindLeaf(DatapointDescription* dataDesc, DatapointCell* theRow, BinaryTreeNode<DatapointKDSplit>* inTree){
	BinaryTreeNode<DatapointKDSplit>* curTree = inTree;
	while(curTree->leftC){
		uintptr_t curDim = curTree->payload.onDim;
		switch(dataDesc->colTypes[curDim]){
			case DATAPOINT_CELL_INT:
				curTree = (theRow[curDim].idat <= curTree->payload.atVal.idat) ? curTree->leftC : curTree->rightC;
				break;
			case DATAPOINT_CELL_FLT:
				curTree = (theRow[curDim].ddat <= curTree->payload.atVal.ddat) ? curTree->leftC : curTree->rightC;
				break;
			case DATAPOINT_CELL_CAT:
				curTree = (theRow[curDim].cdat == curTree->payload.atVal.cdat) ? curTree->leftC : curTree->rightC;
				break;
			default:
				throw std::runtime_error("...dafuq?");
		}
	}
	return curTree;
}

#define DD_WRITE_INT(toConv) nat2be64(toConv, tmpDmpB); parseF->writeBytes(tmpDmpB, 8);

void datapointWriteTree(OutStream* parseF, DatapointDescription* dataDesc, BinaryTreeNode<DatapointKDSplit>* inTree){
	char tmpDmpB[8];
	std::vector<BinaryTreeNode<DatapointKDSplit>*> treeStack;
	treeStack.push_back(inTree);
	while(treeStack.size()){
		BinaryTreeNode<DatapointKDSplit>* curTree = treeStack[treeStack.size()-1];
		treeStack.pop_back();
		//dump the things at this level
		DD_WRITE_INT(curTree->payload.atLevel.size());
		for(uintptr_t i = 0; i<curTree->payload.atLevel.size(); i++){
			DD_WRITE_INT(curTree->payload.atLevel[i]);
		}
		//dump whether there are children (and if so, split data and children)
		if(curTree->leftC){
			DD_WRITE_INT(1)
			DD_WRITE_INT(curTree->payload.onDim);
			switch(dataDesc->colTypes[curTree->payload.onDim]){
				case DATAPOINT_CELL_INT:
					DD_WRITE_INT(curTree->payload.atVal.idat);
					break;
				case DATAPOINT_CELL_FLT:
					DD_WRITE_INT(sdblbits(curTree->payload.atVal.ddat));
					break;
				case DATAPOINT_CELL_CAT:
					DD_WRITE_INT(curTree->payload.atVal.cdat);
					break;
				default:
					throw std::runtime_error("...da faq?");
			}
			treeStack.push_back(curTree->rightC);
			treeStack.push_back(curTree->leftC);
		}
		else{
			DD_WRITE_INT(0)
		}
	}
}

#define DD_LOAD_INT if(parseF->readBytes(tmpLdB,8)!=8){ throw std::runtime_error("Unexpected end of file."); } curLdV = be2nat64(tmpLdB);

BinaryTreeNode<DatapointKDSplit>* datapointParseTree(InStream* parseF, DatapointDescription* dataDesc){
	char tmpLdB[8];
	uintptr_t curLdV;
	std::vector<BinaryTreeNode<DatapointKDSplit>*> treeStack;
	BinaryTreeNode<DatapointKDSplit>* toRet = new BinaryTreeNode<DatapointKDSplit>();
	treeStack.push_back(toRet);
	while(treeStack.size()){
		BinaryTreeNode<DatapointKDSplit>* curTree = treeStack[treeStack.size()-1];
		treeStack.pop_back();
		DD_LOAD_INT uintptr_t numAtL = curLdV;
		for(uintptr_t i = 0; i<numAtL; i++){
			DD_LOAD_INT
			curTree->payload.atLevel.push_back(curLdV);
		}
		//dump whether there are children (and if so, split data and children)
		DD_LOAD_INT int haveC = curLdV;
		if(haveC){
			DD_LOAD_INT uintptr_t onDim = curLdV;
			curTree->payload.onDim = onDim;
			DD_LOAD_INT
			switch(dataDesc->colTypes[onDim]){
				case DATAPOINT_CELL_INT:
					curTree->payload.atVal.idat = curLdV;
					break;
				case DATAPOINT_CELL_FLT:
					curTree->payload.atVal.ddat = sbitsdbl(curLdV);
					break;
				case DATAPOINT_CELL_CAT:
					curTree->payload.atVal.cdat = curLdV;
					break;
				default:
					throw std::runtime_error("...da faq?");
			}
			curTree->leftC = new BinaryTreeNode<DatapointKDSplit>();
			curTree->rightC = new BinaryTreeNode<DatapointKDSplit>();
			treeStack.push_back(curTree->rightC);
			treeStack.push_back(curTree->leftC);
		}
	}
	return toRet;
}

GINIPurityTreeTraining::GINIPurityTreeTraining(){}

GINIPurityTreeTraining::~GINIPurityTreeTraining(){}

void GINIPurityTreeTraining::idiotCheck(){
	if(dataDesc->colTypes[targetCol] != DATAPOINT_CELL_CAT){
		throw std::runtime_error("Can only train with a categorical variable as the target.");
	}
	//TODO
}

/**
 * Calculate the gini impurity given a set of counts.
 * @param catCount The counts.
 * @param totNum The total count.
 */
double calculateGiniImpurity(std::map<int,uintptr_t>* catCount, uintptr_t totNum){
	if(totNum == 0.0){ return 0.0; }
	double totNumD = totNum;
	double giniImp = 1.0;
	for(std::map<int,uintptr_t>::iterator ccIt = catCount->begin(); ccIt != catCount->end(); ccIt++){
		double curPure = (ccIt->second / totNumD);
		curPure = curPure * curPure;
		giniImp -= curPure;
	}
	return std::max(giniImp, 0.0);
}
	
std::pair<uintptr_t,DatapointCell> GINIPurityTreeTraining::figureIdealSplit(uintptr_t numD, uintptr_t* dInds){
	uintptr_t bestDim = 0;
	double bestImp = 1.0 / 0.0;
	DatapointCell bestVal;
	int highCat = dataDesc->factorMaxLevel[targetCol];
	uintptr_t numRow = dataDesc->colTypes.size();
	for(uintptr_t di = 0; di<useCols.size(); di++){
		lowCatCnt.clear();
		higCatCnt.clear();
		uintptr_t numHig = numD;
		uintptr_t numLow = 0;
		uintptr_t curSplIL;
		uintptr_t curSplIH;
		uintptr_t curDim = useCols[di];
		switch(dataDesc->colTypes[curDim]){
			case DATAPOINT_CELL_INT:
				//pack and sort the data
				allValsInt.resize(numD);
				for(uintptr_t i = 0; i<numD; i++){
					uintptr_t rowBI = numRow*dInds[i];
					allValsInt[i].first = allData[rowBI+curDim].idat;
					allValsInt[i].second = allData[rowBI+targetCol].cdat;
				}
				std::sort(allValsInt.begin(), allValsInt.end());
				//count the number of each category
				for(uintptr_t i = 0; i<numD; i++){
					higCatCnt[allValsInt[i].second]++;
				}
				//run through the splits, looking for the best purity
				curSplIL = 0;
				while(curSplIL < numD){
					//find the next break
					curSplIH = std::upper_bound(allValsInt.begin() + curSplIL, allValsInt.end(), std::pair<intptr_t,int>(allValsInt[curSplIL].first, highCat)) - allValsInt.begin();
					//update counts
					for(uintptr_t i = curSplIL; i<curSplIH; i++){
						lowCatCnt[allValsInt[i].second]++;
						higCatCnt[allValsInt[i].second]--;
					}
					numHig -= (curSplIH - curSplIL);
					numLow += (curSplIH - curSplIL);
					//calculate gini
					double lowGin = calculateGiniImpurity(&lowCatCnt, numLow);
					double higGin = calculateGiniImpurity(&higCatCnt, numHig);
					double avgGin = (lowGin + higGin) / 2.0;
					//test against best
					if(avgGin < bestImp){
						bestDim = curDim;
						bestImp = avgGin;
						bestVal.idat = allValsInt[curSplIL].first;
					}
					//and prepare for next search
					curSplIL = curSplIH;
				}
				break;
			case DATAPOINT_CELL_FLT:
				//pack and sort the data
				allValsDbl.resize(numD);
				for(uintptr_t i = 0; i<numD; i++){
					uintptr_t rowBI = numRow*dInds[i];
					allValsDbl[i].first = allData[rowBI+curDim].ddat;
					allValsDbl[i].second = allData[rowBI+targetCol].cdat;
				}
				std::sort(allValsDbl.begin(), allValsDbl.end());
				//count the number of each category
				for(uintptr_t i = 0; i<numD; i++){
					higCatCnt[allValsDbl[i].second]++;
				}
				//run through the splits, looking for the best purity
				curSplIL = 0;
				while(curSplIL < numD){
					//find the next break
					curSplIH = std::upper_bound(allValsDbl.begin() + curSplIL, allValsDbl.end(), std::pair<double,int>(allValsDbl[curSplIL].first, highCat)) - allValsDbl.begin();
					//update counts
					for(uintptr_t i = curSplIL; i<curSplIH; i++){
						lowCatCnt[allValsDbl[i].second]++;
						higCatCnt[allValsDbl[i].second]--;
					}
					numHig -= (curSplIH - curSplIL);
					numLow += (curSplIH - curSplIL);
					//calculate gini
					double lowGin = calculateGiniImpurity(&lowCatCnt, numLow);
					double higGin = calculateGiniImpurity(&higCatCnt, numHig);
					double avgGin = (lowGin + higGin) / 2.0;
					//test against best
					if(avgGin < bestImp){
						bestDim = curDim;
						bestImp = avgGin;
						bestVal.ddat = allValsDbl[curSplIL].first;
					}
					//and prepare for next search
					curSplIL = curSplIH;
				}
				break;
			case DATAPOINT_CELL_CAT:
				//pack and sort the data
				allValsCat.resize(numD);
				for(uintptr_t i = 0; i<numD; i++){
					uintptr_t rowBI = numRow*dInds[i];
					allValsCat[i].first = allData[rowBI+curDim].cdat;
					allValsCat[i].second = allData[rowBI+targetCol].cdat;
				}
				std::sort(allValsCat.begin(), allValsCat.end());
				//count the number of each category
				for(uintptr_t i = 0; i<numD; i++){
					higCatCnt[allValsCat[i].second]++;
				}
				//run through the splits, looking for the best purity
				curSplIL = 0;
				while(curSplIL < numD){
					//find the next break
					curSplIH = std::upper_bound(allValsCat.begin() + curSplIL, allValsCat.end(), std::pair<int,int>(allValsCat[curSplIL].first, highCat)) - allValsCat.begin();
					//update counts
					for(uintptr_t i = curSplIL; i<curSplIH; i++){
						lowCatCnt[allValsCat[i].second]++;
						higCatCnt[allValsCat[i].second]--;
					}
					numLow = (curSplIH - curSplIL);
					//calculate gini
					double lowGin = calculateGiniImpurity(&lowCatCnt, numLow);
					double higGin = calculateGiniImpurity(&higCatCnt, numD - numLow);
					double avgGin = (lowGin + higGin) / 2.0;
					//test against best
					if(avgGin < bestImp){
						bestDim = curDim;
						bestImp = avgGin;
						bestVal.cdat = allValsCat[curSplIL].first;
					}
					//and prepare for next search
					for(std::map<int,uintptr_t>::iterator ccIt = lowCatCnt.begin(); ccIt != lowCatCnt.end(); ccIt++){
						higCatCnt[ccIt->first] += ccIt->second;
						ccIt->second = 0;
					}
					curSplIL = curSplIH;
				}
				break;
			default:
				throw std::runtime_error("...dafuq?");
		}
	}
	return std::pair<uintptr_t,DatapointCell>(bestDim, bestVal);
}

DecisionTreeTraining* GINIPurityTreeTraining::clone(){
	return new GINIPurityTreeTraining();
}

RandomForest::RandomForest(){
	dataDesc = 0;
	numRows = 0;
	allData = 0;
	numTree = 1024;
	numThread = 1;
	usePool = 0;
	targetCol = -1;
}

RandomForest::~RandomForest(){
	for(uintptr_t i = 0; i<bosque.size(); i++){
		delete(bosque[i]);
	}
}

/**Grow some of the trees.*/
void randomForestGrowMethod(void* theUni){
	RandomForestUniform* myU = (RandomForestUniform*)theUni;
	uintptr_t numData = myU->myMain->numRows;
	RandomGenerator* myRand = myU->myMain->useRands[myU->threadInd];
	GINIPurityTreeTraining trainMeth;
	trainMeth.dataDesc = myU->myMain->dataDesc;
	trainMeth.allData = myU->myMain->allData;
	trainMeth.targetCol = myU->myMain->targetCol;
	trainMeth.useCols = myU->myMain->useCols;
	trainMeth.useRows.resize(numData);
	for(uintptr_t i = myU->treeLowI; i < myU->treeHigI; i++){
		//generate a bootstrap
		myRand->getBytes(numData * sizeof(uintptr_t), (char*)&(trainMeth.useRows[0]));
		for(uintptr_t j = 0; j<numData; j++){
			trainMeth.useRows[j] = trainMeth.useRows[j] % numData;
		}
		//make the tree
		myU->myMain->bosque[i] = trainMeth.train();
	}
}

void RandomForest::grow(){
	myUnis.resize(numThread);
	bosque.resize(numTree);
	uintptr_t numPerT = numTree / numThread;
	uintptr_t numExtT = numTree % numThread;
	uintptr_t nextTI = 0;
	for(uintptr_t i = 0; i<numThread; i++){
		RandomForestUniform* cuni = &(myUnis[i]);
		cuni->myMain = this;
		cuni->treeLowI = nextTI;
		nextTI += (numPerT + (i<numExtT));
		cuni->treeHigI = nextTI;
		cuni->threadInd = i;
		cuni->threadID = usePool->addTask(randomForestGrowMethod, cuni);
	}
	for(uintptr_t i = 0; i<numThread; i++){
		usePool->joinTask(myUnis[i].threadID);
	}
}

int RandomForest::classify(DatapointCell* compData, uintptr_t classCol){
	tmpCountStore.clear();
	tmpCountStore.resize(dataDesc->factorMaxLevel[classCol]);
	classifyVotes(compData, classCol, &(tmpCountStore[0]));
	uintptr_t maxCount = 0;
	int winClass = 0;
	for(uintptr_t i = 0; i<tmpCountStore.size(); i++){
		if(tmpCountStore[i] >= maxCount){
			maxCount = tmpCountStore[i];
			winClass = i;
		}
	}
	return winClass;
}

/**Get class votes from trees.*/
void randomForestVoteMethod(void* theUni){
	RandomForestUniform* myU = (RandomForestUniform*)theUni;
	DatapointCell* fullTab = myU->myMain->allData;
	uintptr_t numCols = myU->myMain->dataDesc->colTypes.size();
	myU->saveVotes.clear();
	for(uintptr_t i = myU->treeLowI; i < myU->treeHigI; i++){
		//get the proper leaf
		BinaryTreeNode<DatapointKDSplit>* winLeaf = datapointTreeFindLeaf(myU->myMain->dataDesc, myU->lookFor, myU->myMain->bosque[i]);
		std::vector<uintptr_t>* atLevel = &(winLeaf->payload.atLevel);
		if(atLevel->size() == 0){ continue; }
		//figure out the set of votes
		myU->leafCounts.clear();
		for(uintptr_t j = 0; j<atLevel->size(); j++){
			uintptr_t curR = (*atLevel)[j];
			myU->leafCounts.push_back(fullTab[numCols*curR + myU->focusCol].cdat);
		}
		std::sort(myU->leafCounts.begin(), myU->leafCounts.end());
		//get the most common
		int curMostCom = 0;
		uintptr_t curComCount = 0;
		std::vector<int>::iterator curLCIt = myU->leafCounts.begin();
		while(curLCIt != myU->leafCounts.end()){
			std::vector<int>::iterator nxtLCIt = curLCIt;
			while(*nxtLCIt == *curLCIt){
				nxtLCIt++;
				if(nxtLCIt == myU->leafCounts.end()){ break; }
			}
			uintptr_t curCnt = (nxtLCIt - curLCIt);
			if(curCnt > curComCount){
				curMostCom = *curLCIt;
				curComCount = curCnt;
			}
			curLCIt = nxtLCIt;
		}
		//vote
		myU->saveVotes.push_back(curMostCom);
	}
}

void RandomForest::classifyVotes(DatapointCell* compData, uintptr_t classCol, uintptr_t* saveProps){
	for(uintptr_t i = 0; i<numThread; i++){
		RandomForestUniform* cuni = &(myUnis[i]);
		cuni->focusCol = classCol;
		cuni->lookFor = compData;
		cuni->threadID = usePool->addTask(randomForestVoteMethod, cuni);
	}
	for(uintptr_t i = 0; i<numThread; i++){
		usePool->joinTask(myUnis[i].threadID);
	}
	//add up the results
	uintptr_t totFacVals = dataDesc->factorMaxLevel[classCol];
	memset(saveProps, 0, totFacVals*sizeof(uintptr_t));
	for(uintptr_t i = 0; i<numThread; i++){
		RandomForestUniform* cuni = &(myUnis[i]);
		for(uintptr_t j = 0; j<cuni->saveVotes.size(); j++){
			saveProps[cuni->saveVotes[j]]++;
		}
	}
}

/**Average values across trees.*/
void randomForestAverageMethod(void* theUni){
	RandomForestUniform* myU = (RandomForestUniform*)theUni;
	myU->threadSum = 0;
	DatapointCell* fullTab = myU->myMain->allData;
	uintptr_t numCols = myU->myMain->dataDesc->colTypes.size();
	for(uintptr_t i = myU->treeLowI; i < myU->treeHigI; i++){
		//get the proper leaf
		BinaryTreeNode<DatapointKDSplit>* winLeaf = datapointTreeFindLeaf(myU->myMain->dataDesc, myU->lookFor, myU->myMain->bosque[i]);
		std::vector<uintptr_t>* atLevel = &(winLeaf->payload.atLevel);
		if(atLevel->size() == 0){ continue; }
		//get the average of the value
		double leafSum = 0.0;
		for(uintptr_t j = 0; j<atLevel->size(); j++){
			uintptr_t curR = (*atLevel)[j];
			leafSum += fullTab[numCols*curR + myU->focusCol].ddat;
		}
		leafSum = leafSum / atLevel->size();
		//add to the uniform
		myU->threadSum += leafSum;
	}
}

double RandomForest::regress(DatapointCell* compData, uintptr_t valueCol){
	for(uintptr_t i = 0; i<numThread; i++){
		RandomForestUniform* cuni = &(myUnis[i]);
		cuni->focusCol = valueCol;
		cuni->lookFor = compData;
		cuni->threadID = usePool->addTask(randomForestAverageMethod, cuni);
	}
	for(uintptr_t i = 0; i<numThread; i++){
		usePool->joinTask(myUnis[i].threadID);
	}
	//add up the results
	double totRet = 0.0;
	for(uintptr_t i = 0; i<numThread; i++){
		totRet += myUnis[i].threadSum;
	}
	return totRet / numTree;
}



