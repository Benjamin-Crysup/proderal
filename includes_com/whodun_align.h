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

class GraphSequenceAlignment;

/**An iterator through alignments of sequences against graphs.*/
class GraphSequenceAlignmentIteration{
public:
	/**
	 * Basic setup.
	 * @param forAln The alignment this is for.
	 */
	GraphSequenceAlignmentIteration(GraphSequenceAlignment* forAln);
	/**Tear down.*/
	virtual ~GraphSequenceAlignmentIteration();
	/**Whether each visited node in A consumed a character in the graph, as opposed to a skip.*/
	std::vector<bool> consumeA;
	/**The visited indices in sequence graph a.*/
	std::vector<intptr_t> aInds;
	/**The visited indices in sequence b.*/
	std::vector<intptr_t> bInds;
	/**The score of the most recent alignment.*/
	intptr_t alnScore;
	/**The original alignment.*/
	GraphSequenceAlignment* baseAln;
	//need a graph cigar thing
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

/**Align pairs of sequences against a graph.*/
class GraphSequenceAlignment{
public:
	/**The sequence graph for the refernce sequence.*/
	SequenceGraph* seqAs;
	/**The characters in the second sequence.*/
	std::string* seqBs;
	/**Basic setup.*/
	GraphSequenceAlignment();
	/**Subclass cleanup.*/
	virtual ~GraphSequenceAlignment();
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
	int findAlignmentScores(GraphSequenceAlignmentIteration* theIter, int numFind, intptr_t* storeCost, intptr_t minScore, intptr_t maxDupDeg);
	/**
	 * Get an iterator object that works for this type of alignment method.
	 * @return An iterator object. Delete on finish.
	 */
	virtual GraphSequenceAlignmentIteration* getIteratorToken() = 0;
	/**
	 * Start an iteration through the optimal alignments.
	 * @param theIter The iteration token.
	 */
	virtual void startOptimalIteration(GraphSequenceAlignmentIteration* theIter) = 0;
	/**
	 * (Re)Start a non-optimal iteration.
	 * @param theIter The iteration token.
	 * @param minScore The minimum score to accept.
	 * @param maxDupDeg The maximum number of times to hit a duplicate degraded score. Zero for no such limit.
	 * @param maxNumScore The maximum number of scores to keep track of: if more are encountered, the minimum score will rise.
	 */
	virtual void startFuzzyIteration(GraphSequenceAlignmentIteration* theIter, intptr_t minScore, intptr_t maxDupDeg, intptr_t maxNumScore) = 0;
};

/**A topological sort of a graph.*/
class SequenceGraphTopoSort{
public:
	/**Set up a blank topological sort.*/
	SequenceGraphTopoSort();
	/**Clean up.*/
	~SequenceGraphTopoSort();
	/**The character indices (including start and end).*/
	std::vector<uintptr_t> sortIndices;
	/**Mark ranges.*/
	std::vector< std::pair<uintptr_t,uintptr_t> > indRanges;
	/**Whether each range is sequential or a loop.*/
	std::vector<bool> rangeCycle;
};

/**A stack frame in a topological sort.*/
typedef struct{
	/**The node index.*/
	uintptr_t nodeI;
	/**The current forward link index.*/
	uintptr_t fromFLI;
	/**The target forward link index.*/
	uintptr_t toFLI;
	/**Whether the next character needs to be handled.*/
	bool needNC;
} topoSortVisitStack;

/**Operations on sequence graphs.*/
class SequenceGraphOperations{
public:
	/**Set up basic operations.*/
	SequenceGraphOperations();
	/**Tear down.*/
	~SequenceGraphOperations();
	/**
	 * Get a subgraph of a sequence graph by expanding out a set of nodes backwards and forwards.
	 * @param bigGraph The full graph.
	 * @param storeGraph The place to put the sub-graph.
	 * @param startNodes On call, the characters to start the expansion from. On return, the full set of expanded characters.
	 * @param numExpand The number of characters to expand out.
	 * @param subSeqMap The locations the sequence came from. Map from storeGraph to bigGraph.
	 */
	void getSequenceSubGraph(SequenceGraph* bigGraph, SequenceGraph* storeGraph, std::set<uintptr_t>* startNodes, uintptr_t numExpand, std::vector< std::pair<uintptr_t,uintptr_t> >* subSeqMap);
	/**The handled characters.*/
	std::set<uintptr_t> gssgHandChars;
	/**The waiting characters.*/
	std::vector<uintptr_t> gssgWaitChars;
	/**The waiting characters.*/
	std::vector<uintptr_t> gssgNextWait;
	/**Inverse of subSeqMap.*/
	std::map<uintptr_t,uintptr_t> gssgInvSeqMap;
	/**
	 * Topologically sort a sequence graph.
	 * @param toSort The graph to sort.
	 * @param toStore The place to put the sort data.
	 */
	void topologicalSort(SequenceGraph* toSort, SequenceGraphTopoSort* toStore);
	/**The visited node order list L (Kosaraju algorithm).*/
	std::deque<uintptr_t> topoNodeOrderL;
	/**Which nodes have been visited in the Kosaraju pass.*/
	std::vector<bool> topoNodeVisit;
	/**A stack for handling visited nodes. Actual recursion might overflow.*/
	std::vector<topoSortVisitStack> topoVisitStack;
	/**The group assignment of each node: metanode-realnode.*/
	std::vector< std::pair<uintptr_t,uintptr_t> > topoMetaNodeAssn;
	/**The group assignment of each node: realnode-metanode.*/
	std::vector< std::pair<uintptr_t,uintptr_t> > topoMetaNodeAssnRev;
	/**The number of waiting links for each meta node.*/
	std::vector<uintptr_t> topoMetaCountW;
	/**The meta nodes that are ready to go.*/
	std::vector<uintptr_t> topoReadyMetas;
	/**Ready meta nodes that are cyclic.*/
	std::vector<uintptr_t> topoReadyMetasC;
	/**Storage for the next round of ready meta-nodes.*/
	std::vector<uintptr_t> topoReadyMetasNext;
};

#endif