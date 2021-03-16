#ifndef WHODUN_ALIGN_AFFINEPD_H
#define WHODUN_ALIGN_AFFINEPD_H 1

#include <map>

#include "whodun_align_affine.h"

/**Map from quality ranges to mangles.*/
class PositionalQualityMangleSet{
public:
	/**
	 * Get the mangle for a quality.
	 * @param relMang The quality (ascii phred score).
	 * @return The relevant mangle.
	 */
	AlignCostAffineMangleSet* getMangleForQuality(unsigned char relMang);
	/**
	 * Parse a set of quality specific mangles.
	 * @param parseS The first character to parse.
	 * @param parseE The last character to parse.
	 */
	void parseQualityMangleSet(const char* parseS, const char* parseE);
	/**The (inclusive) quality rangles.*/
	std::vector<unsigned char> allQualLow;
	/**The (inclusive) quality rangles.*/
	std::vector<unsigned char> allQualHigh;
	/**The relevant mangles*/
	std::vector<AlignCostAffineMangleSet> relMangs;
};

/**Quality mangles specific to a region in the reference.*/
class PositionDependentQualityMangleSet{
public:
	/**
	 * Parse a set of position specific mangles.
	 * @param parseS The first character to parse.
	 * @param parseE The last character to parse.
	 */
	void parsePositionMangleSet(const char* parseS, const char* parseE);
	
	/**
	 * Take another mangle set and limit it to a given range.
	 * @param toRebase The set to rebase.
	 * @param newLow The new low reference index.
	 * @param newHig The new high reference index.
	 */
	void rebase(PositionDependentQualityMangleSet* toRebase, uintptr_t newLow, uintptr_t newHig);
	
	/**The default mangle.*/
	PositionalQualityMangleSet defMangle;
	/**The low parts of each range, exclusive.*/
	std::vector<uintptr_t> rangLow;
	/**The high parts of each range, inclusive.*/
	std::vector<uintptr_t> rangHig;
	/**The mangles for each range.*/
	std::vector<PositionalQualityMangleSet> rangMang;
};

/**
 * Parse a multiregion position dependent quality mangle specification.
 * @param parseS The start of the string to parse.
 * @param parseE The end of the string to parse.
 * @param toFill The place to put the parsed entries.
 */
void parseMultiregionPositionQualityMangle(const char* parseS, const char* parseE, std::map<std::string, PositionDependentQualityMangleSet>* toFill);

/**Map from quality ranges to mangles.*/
class PositionalBiQualityMangleSet{
public:
	/**
	 * Get the mangle for a quality.
	 * @param relMangA The quality (ascii phred score).
	 * @param relMangB The quality (ascii phred score).
	 * @return The relevant mangle.
	 */
	AlignCostAffineMangleSet* getMangleForQualities(unsigned char relMangA, unsigned char relMangB);
	/**
	 * Parse a set of quality specific mangles.
	 * @param parseS The first character to parse.
	 * @param parseE The last character to parse.
	 */
	void parseQualityMangleSet(const char* parseS, const char* parseE);
	/**The (inclusive) quality rangles.*/
	std::vector< std::pair<int,int> > allQualLow;
	/**The (inclusive) quality rangles.*/
	std::vector< std::pair<int,int> > allQualHigh;
	/**The relevant mangles*/
	std::vector<AlignCostAffineMangleSet> relMangs;
};

/**A description of costs in a genomic pair region.*/
class PositionDependentCostRegion{
public:
	/**The starting position in A of this region.*/
	intptr_t startA;
	/**The ending position in A of this region.*/
	intptr_t endA;
	/**The starting position in B of this region.*/
	intptr_t startB;
	/**The ending position in B of this region.*/
	intptr_t endB;
	/**The priority of this region.*/
	int priority;
	/**The costs of this region.*/
	AlignCostAffine regCosts;
};

#define POSITIONDEPENDENT_SPLITAXIS_A 1
#define POSITIONDEPENDENT_SPLITAXIS_B 2

/**A node in the KD tree.*/
class PositionDependentCostKDNode{
public:
	/**Set up an empty.*/
	PositionDependentCostKDNode();
	/**Tear down.*/
	~PositionDependentCostKDNode();
	/**The axis to split on.*/
	int splitOn;
	/**The location to split at.*/
	intptr_t splitAt;
	/**The node for everything less than, if any.*/
	PositionDependentCostKDNode* subNodeLesser;
	/**The node index for everything greater than or equal (-1 for no such node).*/
	PositionDependentCostKDNode* subNodeGreatE;
	/**The regions that straddle this split.*/
	std::vector<PositionDependentCostRegion*> allSpans;
};

/**A kdtree for position dependent alignment costs.*/
class PositionDependentCostKDTree{
public:
	/**Storage for the raw regions.*/
	std::vector<PositionDependentCostRegion> allRegions;
	/**The nodes of this tree.*/
	PositionDependentCostKDNode* allNodes;
	/**Storage for the tree.*/
	std::vector<PositionDependentCostKDNode> builtTree;
	/**The number of nodes in the built tree that are in use.*/
	uintptr_t builtInUse;
	/**
	 * Copy another tree.
	 * @param toCopy The tree to copy.
	 */
	PositionDependentCostKDTree(const PositionDependentCostKDTree& toCopy);
	/**
	 * Copy another tree.
	 * @param toClone The thing to copy.
	 */
	PositionDependentCostKDTree& operator=(const PositionDependentCostKDTree& toClone);
	/**Set up an empty tree.*/
	PositionDependentCostKDTree();
	/**Tear down.*/
	~PositionDependentCostKDTree();
	
	/**
	 * Take the things in allRegions and build the tree.
	 */
	void produceFromRegions();
	/**Call if you manually update allRegions.*/
	void regionsUpdated();
	/**
	 * Produce regions for a uniform cost tree.
	 * @param baseCost The cost to apply.
	 */
	void regionsUniform(AlignCostAffine* baseCost);
	/**
	 * Fill in regions by rebasing another tree.
	 * @param toRebase The tree to rebase.
	 * @param newLowA The point in the first sequence to consider the new zero. Negative to disable filtering on a.
	 * @param newHigA The point in the first sequence to cut off at.
	 * @param newLowB The point in the second sequence to consider the new zero. Negative to disable filtering on b.
	 * @param newHigB The point in the second sequence to cut off at.
	 */
	void regionsRebased(PositionDependentCostKDTree* toRebase, intptr_t newLowA, intptr_t newHighA, intptr_t newLowB, intptr_t newHighB);
	/**
	 * Mangle region parameters based on read quality.
	 * @param toMangle The tree to mangle.
	 * @param useMangle The mangle set to use.
	 * @param readQuals The ascii phred scores of the bases in the read.
	 */
	void regionsQualityMangled(PositionDependentCostKDTree* toMangle, PositionDependentQualityMangleSet* useMangle, std::vector<char>* readQuals);
	/**
	 * Mangle region parameters based on multiple read qualities.
	 * @param toMangle The tree to mangle.
	 * @param useMangle The mangle set to use.
	 * @param readQualsA The ascii phred scores of the bases in the first sequence ("reference").
	 * @param readQualsB The ascii phred scores of the bases in the second sequence ("read").
	 */
	void regionsBiQualityMangled(PositionDependentCostKDTree* toMangle, PositionalBiQualityMangleSet* useMangle, std::vector<char>* readQualsA, std::vector<char>* readQualsB);
	/**
	 * Parse regions from text.
	 * @param parseS The first character to try to parse.
	 * @param parseE The character after the last possible character.
	 * @return The character after the last byte this used.
	 */
	const char* regionsParse(const char* parseS, const char* parseE);
	
	/**
	 * Get the costs at the given location.
	 * @param inA The location in the reference.
	 * @param inB The location in the read.
	 */
	AlignCostAffine* getCostsAt(intptr_t inA, intptr_t inB);
	/**
	 * Get the costs along a line.
	 * @param inA The index in A.
	 * @param fromB The index in B to start at.
	 * @param toB The index in B to go to.
	 * @param saveRes The place to save the results (one entry for each point in b).
	 */
	void getCostsForA(intptr_t inA, intptr_t fromB, intptr_t toB, std::vector<AlignCostAffine*>* saveRes);
	/**
	 * Get the costs along a line.
	 * @param fromA The index in A to start at.
	 * @param toA The index in A to go to.
	 * @param inB The index in B.
	 * @param saveRes The place to save the results (one entry for each point in a).
	 */
	void getCostsForB(intptr_t fromA, intptr_t toA, intptr_t inB, std::vector<AlignCostAffine*>* saveRes);
private:
	/**
	 * Build the tree from the regions.
	 */
	void buildPDTreeSplits(uintptr_t numRegs, PositionDependentCostRegion** theRegs);
	/**Save pointers to the regions.*/
	std::vector<PositionDependentCostRegion*> saveRegPtrs;
};

/**Output for debug.*/
std::ostream& operator<<(std::ostream& os, const PositionDependentCostKDTree& toOut);

/**
 * Parse a multiregion position dependent cost specification.
 * @param parseS The start of the string to parse.
 * @param parseE The end of the string to parse.
 * @param toFill The place to put the parsed entries.
 */
void parseMultiregionPositionDependentCost(const char* parseS, const char* parseE, std::map<std::string,PositionDependentCostKDTree>* toFill);

class PositionDependentAffineGapLinearPairwiseAlignment;

/**A focus point in an iteration.*/
typedef struct{
	/**The i index.*/
	intptr_t focI;
	/**The j index.*/
	intptr_t focJ;
	/**The directions left to consider.*/
	intptr_t liveDirs;
	/**The score for this path.*/
	intptr_t pathScore;
	/**How this location was approached.*/
	intptr_t howGot;
	/**Whether the default path for this entry has been hit.*/
	intptr_t seenDef;
} PositionDependentAGLPFocusStackEntry;

class PositionDependentAffineGapLinearPairwiseAlignmentIteration : public LinearPairwiseAlignmentIteration{
public:
	/**
	 * Set up an optimal iteration through the given alignment.
	 * @param forAln The base alignment.
	 */
	PositionDependentAffineGapLinearPairwiseAlignmentIteration(PositionDependentAffineGapLinearPairwiseAlignment* forAln);
	/**Clean up.*/
	~PositionDependentAffineGapLinearPairwiseAlignmentIteration();
	
	/**
	 * Change the problem this is working on.
	 * @param forAln The base alignment.
	 */
	void changeProblem(PositionDependentAffineGapLinearPairwiseAlignment* forAln);
	/**
	 * Change the problem this is working on.
	 * @param forAln The base alignment.
	 * @param minimumScore The minimum score to entertain.
	 * @param maximumDupDeg The maximum number of times to entertain a duplicate score: zero for infinite.
	 * @param maximumNumScore The maximum number of scores to keep track of.
	 */
	void changeProblem(PositionDependentAffineGapLinearPairwiseAlignment* forAln, intptr_t minimumScore, intptr_t maximumDupDeg, intptr_t maximumNumScore);
	
	int getNextAlignment();
	
	void updateMinimumScore(intptr_t newMin);
	
private:
	/**The minimum score to entertain.*/
	intptr_t minScore;
	/**The maximum number of times to entertain a duplicate score: zero for infinite.*/
	intptr_t maxDupDeg;
	/**The maximum number of scores to keep track of.*/
	intptr_t maxNumScore;
	/**The scores.*/
	std::vector<intptr_t> allScore;
	/**The number of times each has been seen.*/
	std::vector<intptr_t> allScoreSeen;
	
	/**
	 * Returns whether there are any more paths.
	 * @return Whether there is still a path being looked at.
	 */
	bool hasPath();
	/**
	 * Returns whether the current path has hit its end.
	 * @return Whether the current path is terminal.
	 */
	bool pathTerminal();
	/**
	 * Get the score of the current path.
	 * @return The score of the current path.
	 */
	intptr_t currentPathScore();
	/**
	 * Get the score of the current path, if it ends here. Only useful for local alignment.
	 * @return The score of the current path.
	 */
	intptr_t currentPathEndScore();
	/**
	 * Move forward to the next path step.
	 */
	void iterateNextPathStep();
	/**
	 * The current path does not match some specification, drop it. Also calls iterateNextPathStep.
	 */
	void abandonPath();
	/**
	 * This will dump out the current path.
	 * @param aInds The place to store the one-based indices in sequence a.
	 * @param bInds The place to store the one-based indices in sequence b.
	 * @return The number of entries in said path.
	 */
	void dumpPath();
	/**
	 * Gets whether the last iteration changed the path being followed..
	 * @return Whether a new starting location was added, a lower score was encountered, or a duplicate score was hit.
	 */
	bool lastIterationChangedPath();
	
	/**The size of the alignment stack.*/
	intptr_t alnStackSize;
	/**The alignment stack.*/
	PositionDependentAGLPFocusStackEntry* alnStack;
	/**Starting points waiting to work over.*/
	std::vector<PositionDependentAGLPFocusStackEntry> waitingStart;
	/**The score for ending at the current location. Always valid, though not always useful.*/
	intptr_t dieHereScore;
	/**Whether the last move involved a path change (initial state, push to new score, or push to another path with same score).*/
	bool lastIterChange;
	/**The number of bytes allocated for the stack.*/
	uintptr_t alnStackAlloc;
	/**Some extra stuff for this thing.*/
	void* saveExtra;
};

/**Do alignments with a position dependent cost function.*/
class PositionDependentAffineGapLinearPairwiseAlignment : public LinearPairwiseSequenceAlignment{
public:
	/**
	 * Prepare an empty alignment structure.
	 */
	PositionDependentAffineGapLinearPairwiseAlignment();
	/**
	 * Prepare an alignment for the given sequences.
	 * @param numSeqEnds The number of ends to require in the alignment: 0 for local, 2 for semi-local and 4 for global.
	 * @param refSeq The reference sequence (sequence A).
	 * @param readSeq The read sequence (sequence B).
	 * @param alnCost The alignment parameters to use.
	 */
	PositionDependentAffineGapLinearPairwiseAlignment(int numSeqEnds, std::string* refSeq, std::string* readSeq, PositionDependentCostKDTree* alnCost);
	/**Clean up.*/
	~PositionDependentAffineGapLinearPairwiseAlignment();
	
	/**
	 * Change the problem to work on.
	 * @param numSeqEnds The number of ends to require in the alignment: 0 for local, 2 for semi-local and 4 for global.
	 * @param refSeq The reference sequence (sequence A).
	 * @param readSeq The read sequence (sequence B).
	 * @param alnCost The alignment parameters to use.
	 */
	void changeProblem(int numSeqEnds, std::string* refSeq, std::string* readSeq, PositionDependentCostKDTree* alnCost);
	
	void prepareAlignmentStructure();
	LinearPairwiseAlignmentIteration* getIteratorToken();
	void startOptimalIteration(LinearPairwiseAlignmentIteration* theIter);
	void startFuzzyIteration(LinearPairwiseAlignmentIteration* theIter, intptr_t minScore, intptr_t maxDupDeg, intptr_t maxNumScore);
	
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
};

/**Output tables for debug.*/
std::ostream& operator<<(std::ostream& os, const PositionDependentAffineGapLinearPairwiseAlignment& toOut);

#endif
