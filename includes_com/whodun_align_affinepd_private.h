#ifndef WHODUN_ALIGN_AFFINEPD_PRIVATE_H
#define WHODUN_ALIGN_AFFINEPD_PRIVATE_H 1

#include "whodun_align_affinepd.h"

#include <vector>

/**Whitespace characters*/
#define WHITESPACE " \t\r\n"
/**Digits*/
#define DIGITS "0123456789"
/**The number of tables.*/
#define NUMBER_OF_TABLES 13

/**
 * Frees many align costs.
 * @param toKill The costs to kill.
 */
void freeAlignCosts(std::vector<AlignCostAffine*>* toKill);

/**
 * Builds the position dependent kd tree, given cost regions.
 * @param allBounds The cost regions.
 * @return The relevant kd tree.
 */
PositionDependentAlignCostKDNode* buildPositionDependentKDTree(std::vector<PositionDependentBounds*>* allBounds);

/**
 * Flattens a tree.
 * @param toFlatten The tree to flatten.
 * @param toFill The place to add the entries.
 */
void flattenPositionDependentKD(PositionDependentAlignCostKDNode* toFlatten, std::vector<PositionDependentBounds*>* toFill);

#define ALIGN_NEED_MATCH_MATCH 1
#define ALIGN_NEED_MATCH_SKIPA 2
#define ALIGN_NEED_MATCH_SKIPB 4
#define ALIGN_NEED_SKIPA_MATCH 8
#define ALIGN_NEED_SKIPA_SKIPA 16
#define ALIGN_NEED_SKIPA_SKIPB 32
#define ALIGN_NEED_SKIPB_MATCH 64
#define ALIGN_NEED_SKIPB_SKIPA 128
#define ALIGN_NEED_SKIPB_SKIPB 256
#define ALIGN_NEED_MATCH (ALIGN_NEED_MATCH_MATCH | ALIGN_NEED_MATCH_SKIPA | ALIGN_NEED_MATCH_SKIPB)
#define ALIGN_NEED_SKIPA (ALIGN_NEED_SKIPA_MATCH | ALIGN_NEED_SKIPA_SKIPA | ALIGN_NEED_SKIPA_SKIPB)
#define ALIGN_NEED_SKIPB (ALIGN_NEED_SKIPB_MATCH | ALIGN_NEED_SKIPB_SKIPA | ALIGN_NEED_SKIPB_SKIPB)

/**An item of interest in an alignment.*/
typedef struct{
	/**The i index.*/
	int focI;
	/**The j index.*/
	int focJ;
	/**The directions left to consider.*/
	int liveDirs;
	/**The score for this path.*/
	int pathScore;
	/**How this location was approached.*/
	int howGot;
	/**Whether the default path for this entry has been hit.*/
	int seenDef;
} FocusStackEntry;

/**
 * Guards a direction entry.
 * @param fI The i index.
 * @param fJ The j index.
 * @param dirFlag The paths to go down.
 * @return The guarded paths to go down.
 */
int focusStackGuardFlag(int fI, int fJ, int dirFlag);

/**
 * Make an initializer for a FocusStackEntry.
 * @param fI The i index.
 * @param fJ The j index.
 * @param dirFlag The paths to go down.
 * @param pathSc The score of the path this is on.
 * @param howG How this node was arrived at. 0 for start.
 */
#define NEW_FOCUS_STACK_ENTRY(fI, fJ, dirFlag, pathSc, howG) {fI, fJ, focusStackGuardFlag(fI,fJ,dirFlag), pathSc, howG, 0}

/**A collection of starting locations.*/
class FocusStackStarts{
public:
	/**Sets up an empty set of stack starts.*/
	FocusStackStarts();
	/**Delete memory.*/
	~FocusStackStarts();
	/**
	 * This will add a starting location.
	 * @param atI The i coordinate to work from.
	 * @param atJ The j coordinate to work from.
	 * @param inDirs The future directions to consider.
	 * @param atCost The score of this location.
	 */
	void addStartingLocation(int atI, int atJ, int inDirs, int atCost);
	/**
	 * Limits the number of starting location scores.
	 * @param toNumber The number of scores to limit to.
	 */
	void limitStartingLocationScores(int toNumber);
	/**
	 * This will limit starting locations to scores above some threshold.
	 * @param minScore The threshold.
	 */
	void limitStartingLocationsByScore(int minScore);
	/**The starting locations of interest.*/
	std::vector< std::vector<FocusStackEntry>* > startingLocs;
	/**The costs for those starting locations.*/
	std::vector<int> startingLocCosts;
};

/**The state of an alignment iteration.*/
class AlignmentIterationState{
public:
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
	int currentPathScore();
	/**
	 * Get the score of the current path, if it ends here. Only useful for local alignment.
	 * @return The score of the current path.
	 */
	int currentPathEndScore();
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
	int dumpPath(int* aInds, int* bInds);
	/**
	 * Gets whether the last iteration changed the path being followed..
	 * @return Whether a new starting location was added, a lower score was encountered, or a duplicate score was hit.
	 */
	bool lastIterationChangedPath();
	/**The size of the alignment stack.*/
	int alnStackSize = 0;
	/**The alignment stack.*/
	FocusStackEntry* alnStack;
	/**Starting points waiting to work over.*/
	std::vector<FocusStackEntry> waitingStart;
	/**The problem in question.*/
	AGPDAlignProblem* forProb;
	/**The score for ending at the current location. Always valid, though not always useful.*/
	int dieHereScore = 0;
	/**Whether the last move involved a path change (initial state, push to new score, or push to another path with same score).*/
	bool lastIterChange;
	/**
	 * Sets up the iteration from the given set of starting locations.
	 * @param toStart The places to start at.
	 * @param forPro THe problem being worked on.
	 */
	AlignmentIterationState(FocusStackStarts* toStart, AGPDAlignProblem* forPro);
	/**Free malloc.*/
	~AlignmentIterationState();
};

/**
 * This will find alignment starting positions for a score search.
 * @param forProb The problem in question.
 * @return The found starting positions.
 */
FocusStackStarts* findStartingPositions(AGPDAlignProblem* forProb);

#endif
