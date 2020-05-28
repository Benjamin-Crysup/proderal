#ifndef WHODUN_ALIGN_AFFINEPD_H
#define WHODUN_ALIGN_AFFINEPD_H 1

#include <map>
#include <string>

/* **********************************************
 Specification of individual costs
 */

/**Affine gap cost information.*/
typedef struct{
	/**The map from character values to indices.*/
	int* charMap;
	/**The number of character entries in the cost table.*/
	int numLiveChars;
	/**The costs of each interaction pair.*/
	int** allMMCost;
	/**The cost to open a gap.*/
	int openCost;
	/**The cost to extend a gap.*/
	int extendCost;
	/**The cost to close a gap.*/
	int closeCost;
} AlignCostAffine;

/**
 * This will parse an affine gap cost.
 * @param numPChars The number of characters in toParse (not including the null).
 * @param toParse The thing to parse.
 * @param toFill The place to put the filled in cost.
 * @return The character location after the last byte parsed (null on error).
 */
const char* parseAffineGapCostSpecification(int numPChars, const char* toParse, AlignCostAffine** toFill);

/**
 * This will clone an affine gap alignment cost.
 * @param toClone The thing to clone.
 * @return The cloned value.
 */
AlignCostAffine* cloneAlignCostAffine(AlignCostAffine* toClone);

/**
 * Will free an affine gap align cost.
 * @param toKill The thing to kill.
 */
void freeAlignCostAffine(AlignCostAffine* toKill);

/* **********************************************
 Specification of position dependent costs
 */
 
/**A description of a region.*/
typedef struct{
	/**The starting position in A of this region.*/
	int startA;
	/**The ending position in A of this region.*/
	int endA;
	/**The starting position in B of this region.*/
	int startB;
	/**The ending position in B of this region.*/
	int endB;
	/**The priority of this region.*/
	int priority;
	/**The costs of this region.*/
	AlignCostAffine* regCosts;
} PositionDependentBounds;
 
 #define PDAG_AXIS_A 1
 #define PDAG_AXIS_B 2
 
/**A node in the region tree for a position dependent alignment.*/
typedef struct{
	/**The axis being split on.*/
	int splitOn;
	/**The location split at.*/
	int splitAt;
	/**The node for everything strictly less than.*/
	void* subNodeLesser;
	/**The node for everything greater than or equal.*/
	void* subNodeGreatE;
	/**The number that span.*/
	int numSpan;
	/**The bounds that span.*/
	PositionDependentBounds** allSpans;
} PositionDependentAlignCostKDNode;
 
 /**
  * This will find the relevant cost for a position.
  * @param forPDCost THe position dependent cost.
  * @param inA THe index in A.
  * @param inB The index in B.
  * @return The relevant costs of (inA,inB).
  */
AlignCostAffine* getPositionDependentAlignmentCosts(PositionDependentAlignCostKDNode* forPDCost, int inA, int inB);

/**
 * This will make a universally applicable position dependent cost.
 * @param useEvery The cost to use everywhere.
 * @return A position dependent cost with a single spec.
 */
PositionDependentAlignCostKDNode* produceUniversalPositionDependent(AlignCostAffine* useEvery);

/**
 * This will parse a cost file.
 * @param numChars THe number of bytes in the file.
 * @param toParse The bytes from the file.
 * @return The cost data.
 */
PositionDependentAlignCostKDNode* parseAsciiPositionDependentSpecification(int numChars, const char* toParse);

/**
 * This will take a position dependent cost specification and limit it to those thinge relevant to a region.
 * @param startPoint The original cost.
 * @param newLowA The point in the first sequence to consider the new zero. Negative to disable filtering on a.
 * @param newHigA The point in the first sequence to cut off at.
 * @param newLowB The point in the second sequence to consider the new zero. Negative to disable filtering on b.
 * @param newHigB The point in the second sequence to cut off at.
 * @return The rebased item.
 */
PositionDependentAlignCostKDNode* rebasePositionDependentSpecification(PositionDependentAlignCostKDNode* startPoint, int newLowA, int newHigA, int newLowB, int newHigB);

/**
 * This will parse a multi-region cost file.
 * @param numChars THe number of bytes in the file.
 * @param toParse The bytes from the file.
 * @param toFill The place to put the read stuff. All added PositionDependentAlignCostAffine will need to be freed.
 * @return Whether there was a problem.
 */
int parseAsciiMultiregionPositionDependentSpecification(int numChars, const char* toParse, std::map<std::string,PositionDependentAlignCostKDNode*>* toFill);

/**
 * Frees cost information read by a parse method.
 * @param toKill The thing to kill.
 */
void freePositionDependentCostInformation(PositionDependentAlignCostKDNode* toKill);

/* **********************************************
 Read quality
 */

/**Change score based on quality.*/
typedef struct{
	/**The number of transliteration alterations.*/
	int numEntries;
	/**The reference characters.*/
	char* fromChars;
	/**The read characters.*/
	char* toChars;
	/**What to do to transliteration costs.*/
	char** charMangTodo;
	/**What to do to gap open cost.*/
	char* gapOpenTodo;
	/**What to do to gap close cost.*/
	char* gapCloseTodo;
	/**What to do to gap extends.*/
	char* gapExtendTodo;
} PositionDependentQualityChanges;

/**All changes for quality in a given reference.*/
typedef struct{
	/**The number of qualities in question.*/
	int numQuals;
	/**The low qualities in question.*/
	unsigned char* allQualLow;
	/**The high qualities in question.*/
	unsigned char* allQualHigh;
	/**All relevante changes.*/
	PositionDependentQualityChanges** allQChange;
} PositionDependentQualityChangeSet;

/**
 * This will parse a quality effect file.
 * @param numChars The number of characters.
 * @param toParse The file to parse.
 * @param toFill The thing to fill.
 * @return Whether there was a problem.
 */
int parseReadQualityEffectFile(int numChars, const char* toParse, std::map<std::string, PositionDependentQualityChangeSet* >* toFill);

/**
 * This will free a single set of changes.
 * @param toKill The set to kill.
 */
void freePositionDependentQualityChange(PositionDependentQualityChanges* toKill);

/**
 * Free a change set.
 * @param toKill The thing to kill.
 */
void freePositionDependentQualityChangeSet(PositionDependentQualityChangeSet* toKill);

/**
 * This will modify a position dependent specification with base quality.
 * @param startPoint The original cost.
 * @param qualGuide The things to do for each quality.
 * @param readLen The length of the read.
 * @param readQual The qualities.
 * @param refLen The length of the reference.
 */
PositionDependentAlignCostKDNode* qualifyPositionDependentSpecification(PositionDependentAlignCostKDNode* startPoint, PositionDependentQualityChangeSet* qualGuide, int readLen, const char* readQual, int refLen);

/* **********************************************
 Alignment
 */

/**An alignment problem to solve.*/
typedef struct{
	/**The number of characters in the first sequence.*/
	int lenA;
	/**The characters in the first sequence.*/
	const char* seqA;
	/**The number of characters in the second sequence.*/
	int lenB;
	/**The characters in the second sequence.*/
	const char* seqB;
	/**The number of ends in the requested alignment.*/
	int numEnds;
	/**The allocated cost table.*/
	int** costTable;
	/**The scores at each place for a match.*/
	int** matchTable;
	/**The scores at each place for a match, given the last thing was a match.*/
	int** matchMatchTable;
	/**The scores at each place for a match, given the last thing skipped a.*/
	int** matchSkipATable;
	/**The scores at each place for a match, given the last thing skipped b.*/
	int** matchSkipBTable;
	/**The scores at each place for skipping A.*/
	int** skipATable;
	/**The scores at each place for skipping A, given the last thing was a match.*/
	int** skipAMatchTable;
	/**The scores at each place for skipping A, given the last thing skipped a.*/
	int** skipASkipATable;
	/**The scores at each place for skipping A, given the last thing skipped b.*/
	int** skipASkipBTable;
	/**The scores at each place for skipping B.*/
	int** skipBTable;
	/**The scores at each place for skipping B, given the last thing was a match.*/
	int** skipBMatchTable;
	/**The scores at each place for skipping B, given the last thing skipped a.*/
	int** skipBSkipATable;
	/**The scores at each place for skipping B, given the last thing skipped b.*/
	int** skipBSkipBTable;
	/**The alignment costs*/
	PositionDependentAlignCostKDNode* alnCosts;
} AGPDAlignProblem;

/**
 * This will allocate cost tables.
 * @param toAddTo The problem to fill in.
 */
void allocateAGPDCostTables(AGPDAlignProblem* toAddTo);

/**
 * This will deallocate cost tables.
 * @param toAddTo The problem to clean up.
 */
void deallocateAGPDCostTables(AGPDAlignProblem* toAddTo);

/**
 * This will fill in the costs of an affine gap alignment problem.
 * @param toFill The problem to work on.
 */
void fillInAGPDCostTables(AGPDAlignProblem* toFill);

/**
 * This will find alignment scores.
 * @param forProb THe problem in question.
 * @param numFind THe number of scores to look for.
 * @param storeCost The place to put the found scores.
 * @param maxDupDeg The maximum number of times to hit a duplicate degraded score. Zero for no such limit.
 * @return The number of scores actually found.
 */
int findAGPDAlignmentScores(AGPDAlignProblem* forProb, int numFind, int* storeCost, int maxDupDeg);

/**A place to store an alignment.*/
typedef struct{
	/**The place to put the visited indices in sequence a.*/
	int* aInds;
	/**The place to put the visited indices in sequence b.*/
	int* bInds;
	/**The place to put the length of the alignment. At most lenA + lenB + 2*/
	int alnLen;
	/**The score of the most recent alignment.*/
	int alnScore;
} AlignmentFiller;

/**
 * This will start an iteration through the optimal alignments.
 * @param forProb The problem to iterate through.
 * @return An iterator; type opaque.
 */
void* initializeAGPDOptimalAlignmentIteration(AGPDAlignProblem* forProb);

/**
 * This will start an iteration through the alignments within some range of optimal.
 * @param forProb The problem to iterate through.
 * @param minScore The minimum score to accept.
 * @param maxDupDeg The maximum number of times to hit a duplicate degraded score. Zero for no such limit.
 * @return An iterator; type opaque.
 */
void* initializeAGPDFuzzedAlignmentIteration(AGPDAlignProblem* forProb, int minScore, int maxDupDeg);

/**
 * This will get an alignment.
 * @param iterator The iteration structure from the initialize method.
 * @param storeLoc The place to put the alignment.
 * @return Whether there was an alignment.
 */
int getNextAGPDAlignment(void* iterator, AlignmentFiller* storeLoc);

/**
 * This will end an alignment iteration.
 * @param toTerm The iterator.
 */
void terminateAGPDAlignmentIteration(void* toTerm);

/**
 * Turns an alignment to a cigar string.
 * @param numEnt THe number of entries in the alignment.
 * @param refInds THe reference indices.
 * @param seqInds The sequence indices.
 * @param fillStr THe string to add the CIGAR to.
 * @param startAddr The place to put the start address.
 */
void alignmentToCIGAR(int numEnt, int* refInds, int* seqInds, int seqLen, std::string* fillStr, long int* startAddr);

#endif
