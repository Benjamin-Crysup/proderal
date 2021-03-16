#ifndef WHODUN_ALIGN_H
#define WHODUN_ALIGN_H 1

#include <set>
#include <deque>
#include <string>
#include <vector>
#include <stdint.h>

#include "whodun_parse_seq_graph.h"

/**Mangle an alignment cost.*/
class AlignCostMangleAction{
public:
	/**Tear down.*/
	virtual ~AlignCostMangleAction();
	/**
	 * Perform this action on a cost.
	 * @param onCost The cost to perform the action on.
	 */
	virtual int performAction(int onCost) = 0;
	/**
	 * Clone this thing.
	 * @return The clone.
	 */
	virtual AlignCostMangleAction* cloneMe() = 0;
};

/**A script mangle action.*/
class ScriptAlignCostMangleAction : public AlignCostMangleAction{
public:
	/**
	 * Copy another script.
	 * @param toCopy The thing to copy.
	 */
	ScriptAlignCostMangleAction(const ScriptAlignCostMangleAction& toCopy);
	/**
	 * Copy another script.
	 * @param toClone The thing to copy.
	 */
	ScriptAlignCostMangleAction& operator=(const ScriptAlignCostMangleAction& toClone);
	/**
	 * Set the script to follow.
	 * @param myActions The actions this should do.
	 */
	ScriptAlignCostMangleAction(std::string* myActions);
	/**Tear down.*/
	~ScriptAlignCostMangleAction();
	/**The actions of this thing.*/
	std::vector<int> myActs;
	/**Arguments to those actions.*/
	std::vector<int> actArguments;
	
	int performAction(int onCost);
	AlignCostMangleAction* cloneMe();
};

class LinearPairwiseSequenceAlignment;

/**An iterator through linear sequence pair alignments.*/
class LinearPairwiseAlignmentIteration{
public:
	/**
	 * Basic setup.
	 * @param forAln The alignment this is for.
	 */
	LinearPairwiseAlignmentIteration(LinearPairwiseSequenceAlignment* forAln);
	/**Tear down.*/
	virtual ~LinearPairwiseAlignmentIteration();
	/**The visited indices in sequence a.*/
	std::vector<intptr_t> aInds;
	/**The visited indices in sequence b.*/
	std::vector<intptr_t> bInds;
	/**The score of the most recent alignment.*/
	intptr_t alnScore;
	/**The original alignment.*/
	LinearPairwiseSequenceAlignment* baseAln;
	/**
	 * Turn this alignment to a cigar string (sequence A being the reference).
	 * @param fillStr The string to add to.
	 * @param startAddr The place to put the start address.
	 */
	void toCigar(std::string* fillStr, uintptr_t* startAddr);
	/**
	 * Get the next alignmetn.
	 * @return Whether there was an alignment.
	 */
	virtual int getNextAlignment() = 0;
	
	/**
	 * Update the new minimum score.
	 * @param newMin The new minimum score.
	 */
	virtual void updateMinimumScore(intptr_t newMin) = 0;
};

/**Align pairs of linear sequences.*/
class LinearPairwiseSequenceAlignment{
public:
	/**The characters in the first sequence.*/
	std::string* seqAs;
	/**The characters in the second sequence.*/
	std::string* seqBs;
	/**Basic setup.*/
	LinearPairwiseSequenceAlignment();
	/**Subclass cleanup.*/
	virtual ~LinearPairwiseSequenceAlignment();
	/**
	 * Prepare the relevant alignment tables (if not already done).
	 */
	virtual void prepareAlignmentStructure() = 0;
	/**
	 * This will find some number of alignment scores.
	 * @param theIter An iteration token.
	 * @param numFind The maximum number of scores to look for.
	 * @param storeCost The place to put the results.
	 * @param minScore The minimum score to entertain.
	 * @param maxDupDeg The maximum number of times to hit a duplicate degraded score. Zero for no such limit.
	 * @return The number of found scores.
	 */
	int findAlignmentScores(LinearPairwiseAlignmentIteration* theIter, int numFind, intptr_t* storeCost, intptr_t minScore, intptr_t maxDupDeg);
	/**
	 * Get an iterator object that works for this type of alignment method.
	 * @return An iterator object. Delete on finish.
	 */
	virtual LinearPairwiseAlignmentIteration* getIteratorToken() = 0;
	/**
	 * Start an iteration through the optimal alignments.
	 * @param theIter The iteration token.
	 */
	virtual void startOptimalIteration(LinearPairwiseAlignmentIteration* theIter) = 0;
	/**
	 * (Re)Start a non-optimal iteration.
	 * @param theIter The iteration token.
	 * @param minScore The minimum score to accept.
	 * @param maxDupDeg The maximum number of times to hit a duplicate degraded score. Zero for no such limit.
	 * @param maxNumScore The maximum number of scores to keep track of: if more are encountered, the minimum score will rise.
	 */
	virtual void startFuzzyIteration(LinearPairwiseAlignmentIteration* theIter, intptr_t minScore, intptr_t maxDupDeg, intptr_t maxNumScore) = 0;
};

#endif