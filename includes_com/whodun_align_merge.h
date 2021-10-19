#ifndef WHODUN_ALIGN_MERGE_H
#define WHODUN_ALIGN_MERGE_H 1

#include "whodun_align.h"

/**Merge a pair of sequences from an alignment.*/
class AlignedSequenceMerger{
public:
	/**Set up a blank.*/
	AlignedSequenceMerger();
	/**Tear down.*/
	~AlignedSequenceMerger();
	/**
	 * Change the problem being worked on.
	 * @param seq1Len The length of the first sequence.
	 * @param seq1 The first sequence.
	 * @param qual1 The first quality.
	 * @param seq2Len The length of the second sequence.
	 * @param seq2 The second sequence.
	 * @param qual2 The second quality.
	 */
	void changeFocus(uintptr_t seq1Len, const char* seq1, const double* qual1, uintptr_t seq2Len, const char* seq2, const double* qual2, LinearPairwiseAlignmentIteration* alnUse);
	/**
	 * Change the problem being worked on.
	 * @param seq1Len The length of the first sequence.
	 * @param seq1 The first sequence.
	 * @param qual1 The first quality.
	 * @param seq2Len The length of the second sequence.
	 * @param seq2 The second sequence.
	 * @param qual2 The second quality.
	 */
	void changeFocus(uintptr_t seq1Len, const char* seq1, const char* qual1, uintptr_t seq2Len, const char* seq2, const char* qual2, LinearPairwiseAlignmentIteration* alnUse);
	/**Expand indels from the alignment.*/
	void expandIndels();
	/**Add any prefix: complain if there is no obvious prefix.*/
	void addInPrefix();
	/**Merge overlapping sequence.*/
	void mergeOverlap();
	/**Add any suffix: complain if there is no obvious suffix.*/
	void addInSuffix();
	
	/**Save a sequence.*/
	std::string saveSeqA;
	/**Save a sequence.*/
	std::string saveSeqB;
	/**Temporary quality storage*/
	std::vector<double> saveQualA;
	/**Temporary quality storage*/
	std::vector<double> saveQualB;
	/**The relevant alignment.*/
	LinearPairwiseAlignmentIteration* curIter;
	
	/**Temporary sequence storage*/
	std::vector<int> seqIL;
	/**Temporary quality storage*/
	std::vector<double> seqdIL;
	/**Temporary sequence storage*/
	std::vector<int> seqIR;
	/**Temporary quality storage*/
	std::vector<double> seqdIR;
	
	/**The final storage for the merged sequence.*/
	std::vector<char> mergeSeq;
	/**The final storage for the merged quality.*/
	std::vector<double> mergeQual;
};
//TODO

#endif