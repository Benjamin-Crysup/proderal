#ifndef WHODUN_FIT_RANFOR_H
#define WHODUN_FIT_RANFOR_H 1

#include "whodun_fit.h"
#include "whodun_tree.h"
#include "whodun_randoms.h"

/**Split the phase space based on a dimension in a datapoint.*/
class DatapointKDSplit{
public:
	/**Set up.*/
	DatapointKDSplit();
	/**Tear down.*/
	~DatapointKDSplit();
	/**The dimension to split on: also implicitly give the type used in atVal.*/
	uintptr_t onDim;
	/**The value to split on.*/
	DatapointCell atVal;
	/**The indices of things at this level.*/
	std::vector<uintptr_t> atLevel;
};

/**Produce a decision tree.*/
class DecisionTreeTraining{
public:
	/**Set up an empty trainer.*/
	DecisionTreeTraining();
	/**Tear down, and let subclasses do the same.*/
	virtual ~DecisionTreeTraining();
	
	/**A description of the data.*/
	DatapointDescription* dataDesc;
	/**The table used for this tree.*/
	DatapointCell* allData;
	/**The indices of the rows to use for this tree.*/
	std::vector<uintptr_t> useRows;
	/**The column to actually fit.*/
	uintptr_t targetCol;
	/**The columns to train on.*/
	std::vector<uintptr_t> useCols;
	
	/**
	 * Run the training with the given data.
	 * @return The tree.
	 */
	BinaryTreeNode<DatapointKDSplit>* train();
	
	/**Perform an idiot check on the setup.*/
	virtual void idiotCheck() = 0;
	
	/**
	 * Figure out how to split a set of data.
	 * @param numD The number of rows in the data.
	 * @param dInds The row indices of interest.
	 * @return The prefered split: dimension and value.
	 */
	virtual std::pair<uintptr_t,DatapointCell> figureIdealSplit(uintptr_t numD, uintptr_t* dInds) = 0;
	
	/**
	 * Allocate a copy of this thing.
	 * @return A copy. Will need to delete.
	 */
	virtual DecisionTreeTraining* clone() = 0;
};

/**
 * Find the leaf node a row would be in.
 * @param dataDesc A description of the data.
 * @param theRow The row in question.
 * @param inTree The tree in question.
 * @return The proper leaf node.
 */
BinaryTreeNode<DatapointKDSplit>* datapointTreeFindLeaf(DatapointDescription* dataDesc, DatapointCell* theRow, BinaryTreeNode<DatapointKDSplit>* inTree);

/**
 * Write a tree to file.
 * @param parseF The file to write to.
 * @param dataDesc A description of the data.
 * @param inTree The tree in question.
 */
void datapointWriteTree(OutStream* parseF, DatapointDescription* dataDesc, BinaryTreeNode<DatapointKDSplit>* inTree);

/**
 * Load a tree from a file.
 * @param parseF The file to read from.
 * @param dataDesc A description of the data.
 * @return The loaded tree.
 */
BinaryTreeNode<DatapointKDSplit>* datapointParseTree(InStream* parseF, DatapointDescription* dataDesc);

//TODO save/load individual trees

/**Train a tree by minimizing gini impurity.*/
class GINIPurityTreeTraining : public DecisionTreeTraining{
public:
	/**Set up an empty trainer.*/
	GINIPurityTreeTraining();
	/**Tear down.*/
	~GINIPurityTreeTraining();
	
	void idiotCheck();
	std::pair<uintptr_t,DatapointCell> figureIdealSplit(uintptr_t numD, uintptr_t* dInds);
	DecisionTreeTraining* clone();
	
	/**All values present at a level.*/
	std::vector< std::pair<intptr_t,int> > allValsInt;
	/**All values present at a level.*/
	std::vector< std::pair<double,int> > allValsDbl;
	/**All values present at a level.*/
	std::vector< std::pair<int,int> > allValsCat;
	/**The count of low categories.*/
	std::map<int,uintptr_t> lowCatCnt;
	/**The count of high categories.*/
	std::map<int,uintptr_t> higCatCnt;
};

class RandomForest;

/**A uniform to use for threading.*/
class RandomForestUniform{
public:
	/**The main thing for this uniform.*/
	RandomForest* myMain;
	/**The first tree this should work over.*/
	uintptr_t treeLowI;
	/**The end tree index.*/
	uintptr_t treeHigI;
	/**The column to report.*/
	uintptr_t focusCol;
	/**Temporary storage for votes.*/
	std::vector<int> saveVotes;
	/**The index of the thread.*/
	uintptr_t threadInd;
	/**The id of the thread running this.*/
	uintptr_t threadID;
	/**The sum for this thread.*/
	double threadSum;
	/**The data to actually look for.*/
	DatapointCell* lookFor;
	/**The counts at a leaf node.*/
	std::vector<int> leafCounts;
};

/**A random forest.*/
class RandomForest{
public:
	/**Set up an empty forest.*/
	RandomForest();
	/**Tear down.*/
	~RandomForest();
	
	/**A description of the data.*/
	DatapointDescription* dataDesc;
	/**The number of rows in the data.*/
	uintptr_t numRows;
	/**The table used for this tree.*/
	DatapointCell* allData;
	/**The number of trees to make in the forest.*/
	uintptr_t numTree;
	/**The number of threads to use for actions.*/
	uintptr_t numThread;
	/**The thread pool to use for actions.*/
	ThreadPool* usePool;
	/**The random number generator to use for each thread.*/
	std::vector<RandomGenerator*> useRands;
	/**The column to fit (when building).*/
	uintptr_t targetCol;
	/**The columns to train on.*/
	std::vector<uintptr_t> useCols;
	
	/**The trees.*/
	std::vector<BinaryTreeNode<DatapointKDSplit>*> bosque;
	
	/**Actually grow the forest.*/
	void grow();
	
	/**
	 * Try to classify the given point (most common of the trees).
	 * @param compData The data point to try to classify.
	 * @param classCol The categorical column index containing class: should probably be targetCol, but I'm not your boss.
	 * @return The fit class.
	 */
	int classify(DatapointCell* compData, uintptr_t classCol);
	
	/**
	 * Get the count of trees supporting each class.
	 * @param compData The data point to try to classify.
	 * @param classCol The categorical column index containing class: should probably be targetCol, but I'm not your boss.
	 * @param saveProps The place to put the count for each category: see dataDesc.factorMaxLevel.
	 */
	void classifyVotes(DatapointCell* compData, uintptr_t classCol, uintptr_t* saveProps);
	
	/**
	 * Classify, but the report is an (averaged) double.
	 * @param compData The data point to try to classify.
	 * @param valueCol The data column (to average across trees).
	 * @return The average value across the trees.
	 */
	double regress(DatapointCell* compData, uintptr_t valueCol);
	
	/**Temporary storage for counts.*/
	std::vector<uintptr_t> tmpCountStore;
	/**Uniforms to pass to the threads.*/
	std::vector<RandomForestUniform> myUnis;
};

#endif