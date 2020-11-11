#ifndef WHODUN_ALIGN_AFFINE_H
#define WHODUN_ALIGN_AFFINE_H 1

#include "whodun_align.h"

/**An affine gap alignment cost.*/
class AlignCostAffine{
public:
	/**Set up an empty cost.*/
	AlignCostAffine();
	/**
	 * Clone another cost.
	 * @param toCopy The thing to copy.
	 */
	AlignCostAffine(const AlignCostAffine& toCopy);
	/**
	 * Copy another cost.
	 * @param toClone The thing to copy.
	 */
	AlignCostAffine& operator=(const AlignCostAffine& toClone);
	/**Free any allocations.*/
	~AlignCostAffine();
	/**
	 * Parse a specification.
	 * @param parseS The first character to try to parse.
	 * @param parseE The character after the last possible character.
	 * @return The character after the last byte this used.
	 */
	const char* parseSpecification(const char* parseS, const char* parseE);
	/**The costs of each interaction pair.*/
	int** allMMCost;
	/**The number of character entries in the cost table.*/
	int numLiveChars;
	/**The cost to open a gap.*/
	int openCost;
	/**The cost to extend a gap.*/
	int extendCost;
	/**The cost to close a gap.*/
	int closeCost;
	/**Map from character values to indices.*/
	short charMap[256];
};

/**Output for debug.*/
std::ostream& operator<<(std::ostream& os, const AlignCostAffine& toOut);

/**A set of mangles for an affine gap cost.*/
class AlignCostAffineMangleSet{
public:
	/**
	 * Copy a mangle set.
	 * @param toCopy The set to copy.
	 */
	AlignCostAffineMangleSet(const AlignCostAffineMangleSet& toCopy);
	/**Set up an empty set.*/
	AlignCostAffineMangleSet();
	/**Clean up.*/
	~AlignCostAffineMangleSet();
	/**
	 * Copy another mangle.
	 * @param toClone The thing to copy.
	 */
	AlignCostAffineMangleSet& operator=(const AlignCostAffineMangleSet& toClone);
	/**
	 * Perform the actions of this thing.
	 * @param onCost The cost to apply to.
	 */
	void performActions(AlignCostAffine* onCost);
	/**
	 * Parse a mangle set.
	 * @param parseS The start character.
	 * @param parseE The end character.
	 * @return The character after the last character this uses.
	 */
	void parseMangleSet(const char* parseS, const char* parseE);
	/**The mismatch reference characters.*/
	std::string fromChars;
	/**The mismatch read characters.*/
	std::string toChars;
	/**Mangle actions for mismatch.*/
	std::vector<AlignCostMangleAction*> mmActions;
	/**Mangle actions for gap open.*/
	AlignCostMangleAction* openAction;
	/**Mangle actions for gap extend.*/
	AlignCostMangleAction* extAction;
	/**Mangle actions for gap close.*/
	AlignCostMangleAction* closeAction;
};

/**
 * Get the probability of getting a read starting from the reference sequence.
 * @param alnPro The problem specification.
 * @param readQuals The qualities.
 * @param forAln The results of the alignment.
 * @param lproGapOpen The (log10) probability of opening a gap.
 * @param lproGapExtend The (log10) probability of extending a gap.
 * @return log_10(p(read|position,reference))
 */
double linearReferenceAlignProbabilityAffine(LinearPairwiseSequenceAlignment* alnPro, double* readQuals, LinearPairwiseAlignmentIteration* forAln, double lproGapOpen, double lproGapExtend);

//TODO basic affine gap alignment

#endif
