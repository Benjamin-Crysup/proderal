#include "whodun_align_affinepd.h"
#include "whodun_align_affinepd_private.h"

#include <iostream>

void allocateAGPDCostTables(AGPDAlignProblem* toAddTo){
	int** toRetAll[NUMBER_OF_TABLES];
	for(int t = 0; t<NUMBER_OF_TABLES; t++){
		int** toRetI; int* curLocI;
		toRetI = (int**)malloc((toAddTo->lenA+1)*sizeof(int*) + (toAddTo->lenA+1)*(toAddTo->lenB+1)*sizeof(int));
		curLocI = (int*)(toRetI + (toAddTo->lenA+1));
		for(int i = 0; i<=toAddTo->lenA; i++){
			toRetI[i] = curLocI;
			curLocI += (toAddTo->lenB+1);
		}
		toRetAll[t] = toRetI;
	}
	toAddTo->costTable = toRetAll[0];
	toAddTo->matchTable = toRetAll[1];
	toAddTo->matchMatchTable = toRetAll[2];
	toAddTo->matchSkipATable = toRetAll[3];
	toAddTo->matchSkipBTable = toRetAll[4];
	toAddTo->skipATable = toRetAll[5];
	toAddTo->skipAMatchTable = toRetAll[6];
	toAddTo->skipASkipATable = toRetAll[7];
	toAddTo->skipASkipBTable = toRetAll[8];
	toAddTo->skipBTable = toRetAll[9];
	toAddTo->skipBMatchTable = toRetAll[10];
	toAddTo->skipBSkipATable = toRetAll[11];
	toAddTo->skipBSkipBTable = toRetAll[12];
}

void deallocateAGPDCostTables(AGPDAlignProblem* toAddTo){
	free(toAddTo->costTable);
	free(toAddTo->matchTable);
	free(toAddTo->matchMatchTable);
	free(toAddTo->matchSkipATable);
	free(toAddTo->matchSkipBTable);
	free(toAddTo->skipATable);
	free(toAddTo->skipAMatchTable);
	free(toAddTo->skipASkipATable);
	free(toAddTo->skipASkipBTable);
	free(toAddTo->skipBTable);
	free(toAddTo->skipBMatchTable);
	free(toAddTo->skipBSkipATable);
	free(toAddTo->skipBSkipBTable);
}

#define BEST_OF_THREE(itemA,itemB,itemC) (itemA > itemB) ? ((itemA > itemC) ? itemA : itemC) : ((itemB > itemC) ? itemB : itemC)

void fillInAGPDCostTables(AGPDAlignProblem* toFill){
	//make some common variables local
		int** costTable = toFill->costTable;
		int** matchTable = toFill->matchTable;
		int** matchMatchTable = toFill->matchMatchTable;
		int** matchSkipATable = toFill->matchSkipATable;
		int** matchSkipBTable = toFill->matchSkipBTable;
		int** skipATable = toFill->skipATable;
		int** skipAMatchTable = toFill->skipAMatchTable;
		int** skipASkipATable = toFill->skipASkipATable;
		int** skipASkipBTable = toFill->skipASkipBTable;
		int** skipBTable = toFill->skipBTable;
		int** skipBMatchTable = toFill->skipBMatchTable;
		int** skipBSkipATable = toFill->skipBSkipATable;
		int** skipBSkipBTable = toFill->skipBSkipBTable;
		PositionDependentAlignCostKDNode* alnCosts = toFill->alnCosts;
		int lenA = toFill->lenA;
		int lenB = toFill->lenB;
		const char* seqA = toFill->seqA;
		const char* seqB = toFill->seqB;
	//helpful space for variables
		AlignCostAffine* curCost;
		AlignCostAffine* matCost;
		AlignCostAffine* skaCost;
		AlignCostAffine* skbCost;
	//precalculate some information
		int worstScore = ((unsigned)-1) << (8*sizeof(int)-1);
		bool negToZero = toFill->numEnds == 0;
		int startIJ;
		switch(toFill->numEnds){
			case 0: startIJ = 1; break;
			case 2: startIJ = 1; break;
			case 4: startIJ = 0; break;
			default:
				return;
		};
	//loop through the table
	for(int i = 0; i<=lenA; i++){
		for(int j = 0; j<=lenB; j++){
			curCost = getPositionDependentAlignmentCosts(alnCosts, i-1, j-1);
			matCost = getPositionDependentAlignmentCosts(alnCosts, i-2, j-2);
			skaCost = getPositionDependentAlignmentCosts(alnCosts, i-2, j-1);
			skbCost = getPositionDependentAlignmentCosts(alnCosts, i-1, j-2);
			//match
				int matchCost = (i ? (j ? (curCost->allMMCost[curCost->charMap[0x00FF&seqA[i-1]]][curCost->charMap[0x00FF&seqB[j-1]]]) : 0) : 0);
				int winMM = worstScore;
				int winMA = worstScore;
				int winMB = worstScore;
				switch(i){
					case 0:
						switch(j){
							case 0:
								winMM = 0;
								break;
							case 1:
								break;
							default:
								break;
						};
						break;
					case 1:
						switch(j){
							case 0:
								break;
							case 1:
								winMM = matchTable[i-1][j-1] + matchCost;
								break;
							default:
								winMB = skipBTable[i-1][j-1] + ((startIJ == 0) ? matCost->closeCost : 0) + matchCost;
						};
						break;
					default:
						switch(j){
							case 0:
								break;
							case 1:
								winMA = skipATable[i-1][j-1] + ((startIJ == 0) ? matCost->closeCost : 0) + matchCost;
								break;
							default:
								winMM = matchTable[i-1][j-1] + matchCost;
								winMA = skipATable[i-1][j-1] + matCost->closeCost + matchCost;
								winMB = skipBTable[i-1][j-1] + matCost->closeCost + matchCost;
						};
				};
				if(negToZero){
					if((winMM < 0) && (winMM != worstScore)){winMM = 0;}
					if((winMA < 0) && (winMA != worstScore)){winMA = 0;}
					if((winMB < 0) && (winMB != worstScore)){winMB = 0;}
				}
				matchMatchTable[i][j] = winMM;
				matchSkipATable[i][j] = winMA;
				matchSkipBTable[i][j] = winMB;
				matchTable[i][j] = BEST_OF_THREE(winMM, winMA, winMB);
			//skip A
				int winAM = worstScore;
				int winAA = worstScore;
				int winAB = worstScore;
				switch(i){
					case 0:
						switch(j){
							case 0:
								break;
							case 1:
								break;
							default:
								break;
						};
						break;
					case 1:
						switch(j){
							case 0:
								winAM = matchTable[i-1][j] + curCost->openCost + curCost->extendCost;
								break;
							case 1:
								winAB = skipBTable[i-1][j] + ((startIJ == 0) ? skaCost->closeCost : 0) + curCost->openCost + curCost->extendCost;
								break;
							default:
								winAB = skipBTable[i-1][j] + ((startIJ == 0) ? skaCost->closeCost : 0) + curCost->openCost + curCost->extendCost;
						};
						break;
					default:
						switch(j){
							case 0:
								winAA = skipATable[i-1][j] + curCost->extendCost;
								break;
							case 1:
								winAM = matchTable[i-1][j] + curCost->openCost + curCost->extendCost;
								winAA = skipATable[i-1][j] + curCost->extendCost;
								winAB = skipBTable[i-1][j] + ((startIJ == 0) ? skaCost->closeCost : 0) + curCost->openCost + curCost->extendCost;
								break;
							default:
								winAM = matchTable[i-1][j] + curCost->openCost + curCost->extendCost;
								winAA = skipATable[i-1][j] + curCost->extendCost;
								winAB = skipBTable[i-1][j] + skaCost->closeCost + curCost->openCost + curCost->extendCost;
						};
				};
				if(!(i && j) && (startIJ > 0)){
					winAM = 0;
					winAA = 0;
					winAB = 0;
				}
				if(negToZero){
					if((winAM < 0) && (winAM != worstScore)){winAM = 0;}
					if((winAA < 0) && (winAA != worstScore)){winAA = 0;}
					if((winAB < 0) && (winAB != worstScore)){winAB = 0;}
				}
				skipAMatchTable[i][j] = winAM;
				skipASkipATable[i][j] = winAA;
				skipASkipBTable[i][j] = winAB;
				skipATable[i][j] = BEST_OF_THREE(winAM, winAA, winAB);
			//skip B
				int winBM = worstScore;
				int winBA = worstScore;
				int winBB = worstScore;
				switch(i){
					case 0:
						switch(j){
							case 0:
								break;
							case 1:
								winBM = matchTable[i][j-1] + curCost->openCost + curCost->extendCost;
								break;
							default:
								winBB = skipBTable[i][j-1] + curCost->extendCost;
						};
						break;
					case 1:
						switch(j){
							case 0:
								break;
							case 1:
								winBA = skipATable[i][j-1] + ((startIJ == 0) ? skbCost->closeCost : 0) + curCost->openCost + curCost->extendCost;
								break;
							default:
								winBM = matchTable[i][j-1] + curCost->openCost + curCost->extendCost;
								winBA = skipATable[i][j-1] + ((startIJ == 0) ? skbCost->closeCost : 0) + curCost->openCost + curCost->extendCost;
								winBB = skipBTable[i][j-1] + curCost->extendCost;
						};
						break;
					default:
						switch(j){
							case 0:
								break;
							case 1:
								winBA = skipATable[i][j-1] + ((startIJ == 0) ? skbCost->closeCost : 0) + curCost->openCost + curCost->extendCost;
								break;
							default:
								winBM = matchTable[i][j-1] + curCost->openCost + curCost->extendCost;
								winBA = skipATable[i][j-1] + skbCost->closeCost + curCost->openCost + curCost->extendCost;
								winBB = skipBTable[i][j-1] + curCost->extendCost;
						};
				};
				if(!(i && j) && (startIJ > 0)){
					winBM = 0;
					winBA = 0;
					winBB = 0;
				}
				if(negToZero){
					if((winBM < 0) && (winBM != worstScore)){winBM = 0;}
					if((winBA < 0) && (winBA != worstScore)){winBA = 0;}
					if((winBB < 0) && (winBB != worstScore)){winBB = 0;}
				}
				skipBMatchTable[i][j] = winBM;
				skipBSkipATable[i][j] = winBA;
				skipBSkipBTable[i][j] = winBB;
				skipBTable[i][j] = BEST_OF_THREE(winBM, winBA, winBB);
			costTable[i][j] = BEST_OF_THREE(matchTable[i][j], skipATable[i][j], skipBTable[i][j]);
		}
	}
	//dump out the tables
	#define DUMP_TABLE(dumpTab) for(int j = 0; j<=lenB; j++){for(int i = 0; i<=lenA; i++){std::cout << dumpTab[i][j] << " ";} std::cout << std::endl;}
	//std::cout << "costTable" << std::endl; DUMP_TABLE(costTable)
	//std::cout << "matchTable" << std::endl; DUMP_TABLE(matchTable)
	//std::cout << "skipATable" << std::endl; DUMP_TABLE(skipATable)
	//std::cout << "skipBTable" << std::endl; DUMP_TABLE(skipBTable)
	//std::cout << "skipASkipATable" << std::endl; DUMP_TABLE(skipASkipATable)
	//std::cout << "skipAMatchTable" << std::endl; DUMP_TABLE(skipAMatchTable)
	//std::cout << "skipASkipBTable" << std::endl; DUMP_TABLE(skipASkipBTable)
	//std::cout << "matchSkipATable" << std::endl; DUMP_TABLE(matchSkipATable)
	//std::cout << "matchMatchTable" << std::endl; DUMP_TABLE(matchMatchTable)
	//std::cout << "matchSkipBTable" << std::endl; DUMP_TABLE(matchSkipBTable)
	//std::cout << "skipBSkipATable" << std::endl; DUMP_TABLE(skipBSkipATable)
	//std::cout << "skipBMatchTable" << std::endl; DUMP_TABLE(skipBMatchTable)
	//std::cout << "skipBSkipBTable" << std::endl; DUMP_TABLE(skipBSkipBTable)
}
