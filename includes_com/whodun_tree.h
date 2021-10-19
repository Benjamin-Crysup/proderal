#ifndef WHODUN_TREE_H
#define WHODUN_TREE_H 1

#include <vector>
#include <stdint.h>

/**A node in a tree.*/
template <typename PLT>
class TreeNode {
public:
	/** Set up a tree node with the default payload. */
	TreeNode(){
		parent = 0;
	}
	/**
	 * Set up a tree node.
	 * @param storeItem THe thing to store for this tree.
	 */
	TreeNode(PLT storeItem){
		payload = storeItem;
		parent = 0;
	}
	/**Clean up the children.*/
	~TreeNode(){
		while(children.size()){
			//avoid stack smash on large tree
			TreeNode<PLT>* nextD = children[children.size()-1];
			children.pop_back();
			children.insert(children.end(), nextD->children.begin(), nextD->children.end());
			nextD->children.clear();
			delete(nextD);
		}
	}
	/**
	 * Add a child to this node.
	 * @param cData The data for the child.
	 * @return The created node.
	 */
	TreeNode<PLT>* addChild(PLT cData){
		TreeNode<PLT>* newC = new TreeNode<PLT>(cData);
		children.push_back(newC);
		newC->parent = this;
		return newC;
	}
	/**
	 * Remove a child from this node.
	 * @param childI The index of the child.
	 */
	void removeChild(uintptr_t childI){
		delete(children[childI]);
		children.erase(children.begin() + childI);
	}
	/**The data at this node. If on the heap, you will have to clean it up.*/
	PLT payload;
	/**The parent of this node.*/
	TreeNode<PLT>* parent;
	/**The children of this node.*/
	std::vector< TreeNode<PLT>* > children;
};

/**A node in a binary tree.*/
template <typename PLT>
class BinaryTreeNode {
public:
	/** Set up a tree node with default payload. */
	BinaryTreeNode(){
		leftC = 0;
		rightC = 0;
	}
	/**
	 * Set up a tree node.
	 * @param storeItem THe thing to store for this tree.
	 */
	BinaryTreeNode(PLT storeItem){
		payload = storeItem;
		leftC = 0;
		rightC = 0;
	}
	/**Clean up the children.*/
	~BinaryTreeNode(){
		//avoid stack smashing on DEEP trees trees
		if(leftC || rightC){
			std::vector<BinaryTreeNode<PLT>*> destV;
			if(leftC){ destV.push_back(leftC); }
			if(rightC){ destV.push_back(rightC); }
			while(destV.size()){
				BinaryTreeNode<PLT>* nextD = destV[destV.size()-1];
				destV.pop_back();
				if(nextD->leftC){ destV.push_back(nextD->leftC); }
				if(nextD->rightC){ destV.push_back(nextD->rightC); }
				nextD->leftC = 0;
				nextD->rightC = 0;
				delete(nextD);
			}
		}
	}
	/**The data at this node. If on the heap, you will have to clean it up.*/
	PLT payload;
	/**The left child.*/
	BinaryTreeNode<PLT>* leftC;
	/**The right child.*/
	BinaryTreeNode<PLT>* rightC;
};

/**A split at a KD node.*/
class EuclidianKDSplit{
public:
	/**Set up.*/
	EuclidianKDSplit();
	/**Tear down.*/
	~EuclidianKDSplit();
	/**The dimension to split on.*/
	uintptr_t onDim;
	/**The value to split at.*/
	double atVal;
	/**The indices of things at this level.*/
	std::vector<uintptr_t> atLevel;
};

/**
 * Build a KD tree from points in Euclidian space.
 * @param spaceDim The dimensionality of the space.
 * @param numPts The number of points.
 * @param allPts All the points.
 * @return A KD tree of the points.
 */
BinaryTreeNode<EuclidianKDSplit>* buildEuclideanPointKDTree(uintptr_t spaceDim, uintptr_t numPts, double* allPts);

#endif