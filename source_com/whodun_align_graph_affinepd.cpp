#include "whodun_align_graph_affinepd.h"


PositionDependentAffineGapGraphSequenceAlignment::PositionDependentAffineGapGraphSequenceAlignment(){
	numEnds = 4;
	alnCosts = 0;
	seqAs = 0;
	seqBs = 0;
	costTable = 0;
	saveAlloc = (intptr_t**)malloc(8*sizeof(intptr_t*));
	saveSize = 8;
}

PositionDependentAffineGapGraphSequenceAlignment::PositionDependentAffineGapGraphSequenceAlignment(int numSeqEnds, SequenceGraph* refSeq, std::string* readSeq, PositionDependentCostKDTree* alnCost){
	numEnds = numSeqEnds;
	alnCosts = alnCost;
	seqAs = refSeq;
	seqBs = readSeq;
	costTable = 0;
	saveAlloc = (intptr_t**)malloc(8*sizeof(intptr_t*));
	saveSize = 8;
}

PositionDependentAffineGapGraphSequenceAlignment::~PositionDependentAffineGapGraphSequenceAlignment(){
	free(saveAlloc);
}

void PositionDependentAffineGapGraphSequenceAlignment::changeProblem(int numSeqEnds, SequenceGraph* refSeq, std::string* readSeq, PositionDependentCostKDTree* alnCost){
	numEnds = numSeqEnds;
	alnCosts = alnCost;
	seqAs = refSeq;
	seqBs = readSeq;
	costTable = 0;
}

void PositionDependentAffineGapGraphSequenceAlignment::prepareAlignmentStructure(){
	if(costTable){ return; }
	//topological sort
		grapOps.topologicalSort(seqAs, &topoSort);
	//allocate
	//TODO
	//fill in the tables
	//TODO
}

GraphSequenceAlignmentIteration* PositionDependentAffineGapGraphSequenceAlignment::getIteratorToken(){
	return new PositionDependentAffineGapGraphSequenceAlignmentIteration(this);
}

void PositionDependentAffineGapGraphSequenceAlignment::startOptimalIteration(GraphSequenceAlignmentIteration* theIter){
	PositionDependentAffineGapGraphSequenceAlignmentIteration* realIter = (PositionDependentAffineGapGraphSequenceAlignmentIteration*)theIter;
	realIter->changeProblem(this);
}

void PositionDependentAffineGapGraphSequenceAlignment::startFuzzyIteration(GraphSequenceAlignmentIteration* theIter, intptr_t minScore, intptr_t maxDupDeg, intptr_t maxNumScore){
	PositionDependentAffineGapGraphSequenceAlignmentIteration* realIter = (PositionDependentAffineGapGraphSequenceAlignmentIteration*)theIter;
	realIter->changeProblem(this, minScore, maxDupDeg, maxNumScore);
}



PositionDependentAffineGapGraphSequenceAlignmentIteration::PositionDependentAffineGapGraphSequenceAlignmentIteration(PositionDependentAffineGapGraphSequenceAlignment* forAln) : GraphSequenceAlignmentIteration(forAln){
	//TODO
}

PositionDependentAffineGapGraphSequenceAlignmentIteration::~PositionDependentAffineGapGraphSequenceAlignmentIteration(){
	//TODO
}

void PositionDependentAffineGapGraphSequenceAlignmentIteration::changeProblem(PositionDependentAffineGapGraphSequenceAlignment* forAln){
	//TODO
}

void PositionDependentAffineGapGraphSequenceAlignmentIteration::changeProblem(PositionDependentAffineGapGraphSequenceAlignment* forAln, intptr_t minimumScore, intptr_t maximumDupDeg, intptr_t maximumNumScore){
	//TODO
}

int PositionDependentAffineGapGraphSequenceAlignmentIteration::getNextAlignment(){
	//TODO
	return 0;
}

void PositionDependentAffineGapGraphSequenceAlignmentIteration::updateMinimumScore(intptr_t newMin){
	//TODO
}

