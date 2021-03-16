#ifndef WHODUN_ALIGN_GRAPH_AFFINEPD_H
#define WHODUN_ALIGN_GRAPH_AFFINEPD_H 1

#include "whodun_align_graph.h"
#include "whodun_align_affinepd.h"

//TODO rebase position dependent with gaps

class PositionDependentAffineGapGraphSequenceAlignment;

class PositionDependentAffineGapGraphSequenceAlignmentIteration : public GraphSequenceAlignmentIteration{
public:
	/**
	 * Set up an optimal iteration through the given alignment.
	 * @param forAln The base alignment.
	 */
	PositionDependentAffineGapGraphSequenceAlignmentIteration(PositionDependentAffineGapGraphSequenceAlignment* forAln);
	/**Clean up.*/
	~PositionDependentAffineGapGraphSequenceAlignmentIteration();
	
	/**
	 * Change the problem this is working on.
	 * @param forAln The base alignment.
	 */
	void changeProblem(PositionDependentAffineGapGraphSequenceAlignment* forAln);
	/**
	 * Change the problem this is working on.
	 * @param forAln The base alignment.
	 * @param minimumScore The minimum score to entertain.
	 * @param maximumDupDeg The maximum number of times to entertain a duplicate score: zero for infinite.
	 * @param maximumNumScore The maximum number of scores to keep track of.
	 */
	void changeProblem(PositionDependentAffineGapGraphSequenceAlignment* forAln, intptr_t minimumScore, intptr_t maximumDupDeg, intptr_t maximumNumScore);
	
	int getNextAlignment();
	
	void updateMinimumScore(intptr_t newMin);
	
};

/**Do alignments with a position dependent cost function.*/
class PositionDependentAffineGapGraphSequenceAlignment : public GraphSequenceAlignment{
public:
	/**
	 * Prepare an empty alignment structure.
	 */
	PositionDependentAffineGapGraphSequenceAlignment();
	/**
	 * Prepare an alignment for the given sequences.
	 * @param numSeqEnds The number of ends to require in the alignment: 0 for local, 2 for semi-local and 4 for global.
	 * @param refSeq The reference sequence (sequence A).
	 * @param readSeq The read sequence (sequence B).
	 * @param alnCost The alignment parameters to use.
	 */
	PositionDependentAffineGapGraphSequenceAlignment(int numSeqEnds, SequenceGraph* refSeq, std::string* readSeq, PositionDependentCostKDTree* alnCost);
	/**Clean up.*/
	~PositionDependentAffineGapGraphSequenceAlignment();
	
	/**
	 * Change the problem to work on.
	 * @param numSeqEnds The number of ends to require in the alignment: 0 for local, 2 for semi-local and 4 for global.
	 * @param refSeq The reference sequence (sequence A).
	 * @param readSeq The read sequence (sequence B).
	 * @param alnCost The alignment parameters to use.
	 */
	void changeProblem(int numSeqEnds, SequenceGraph* refSeq, std::string* readSeq, PositionDependentCostKDTree* alnCost);
	
	void prepareAlignmentStructure();
	GraphSequenceAlignmentIteration* getIteratorToken();
	void startOptimalIteration(GraphSequenceAlignmentIteration* theIter);
	void startFuzzyIteration(GraphSequenceAlignmentIteration* theIter, intptr_t minScore, intptr_t maxDupDeg, intptr_t maxNumScore);
	
	/**The topological sort of the graph.*/
	SequenceGraphTopoSort topoSort;
	/**Number of ends to require in the alignment.*/
	int numEnds;
	/**The alignment parameters*/
	PositionDependentCostKDTree* alnCosts;
	/**The allocated cost table.*/
	intptr_t** costTable;
	/**The scores at each place for a match.*/
	intptr_t** matchTable;
	/**The scores at each place for a match, given the last thing was a match.*/
	intptr_t** matchMatchTable;
	/**The scores at each place for a match, given the last thing skipped a.*/
	intptr_t** matchSkipATable;
	/**The scores at each place for a match, given the last thing skipped b.*/
	intptr_t** matchSkipBTable;
	/**The scores at each place for skipping A.*/
	intptr_t** skipATable;
	/**The scores at each place for skipping A, given the last thing was a match.*/
	intptr_t** skipAMatchTable;
	/**The scores at each place for skipping A, given the last thing skipped a.*/
	intptr_t** skipASkipATable;
	/**The scores at each place for skipping A, given the last thing skipped b.*/
	intptr_t** skipASkipBTable;
	/**The scores at each place for skipping B.*/
	intptr_t** skipBTable;
	/**The scores at each place for skipping B, given the last thing was a match.*/
	intptr_t** skipBMatchTable;
	/**The scores at each place for skipping B, given the last thing skipped a.*/
	intptr_t** skipBSkipATable;
	/**The scores at each place for skipping B, given the last thing skipped b.*/
	intptr_t** skipBSkipBTable;
	
	/**A saved allocation.*/
	intptr_t** saveAlloc;
	/**The size of the saved allocation.*/
	uintptr_t saveSize;
	/**Saved storage for costs along a row*/
	std::vector<AlignCostAffine*> curCosts;
	/**Saved storage for costs along a row*/
	std::vector<AlignCostAffine*> matCosts;
	/**Saved storage for costs along a row*/
	std::vector<AlignCostAffine*> skaCosts;
	/**Saved storage for costs along a row*/
	std::vector<AlignCostAffine*> skbCosts;
	
	/**Something to use for graph operations.*/
	SequenceGraphOperations grapOps;
};

#endif