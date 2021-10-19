#include "whodun_tree.h"

EuclidianKDSplit::EuclidianKDSplit(){}
EuclidianKDSplit::~EuclidianKDSplit(){}

BinaryTreeNode<EuclidianKDSplit>* buildEuclideanPointKDTree(uintptr_t spaceDim, uintptr_t numPts, double* allPts){
	//do some initial setup
	std::vector<uintptr_t> allLivePI;
		for(uintptr_t i = 0; i<numPts; i++){ allLivePI.push_back(i); }
	std::vector<uintptr_t> hotI0;
		hotI0.push_back(0);
	std::vector< BinaryTreeNode<EuclidianKDSplit>* > hotLevels;
		BinaryTreeNode<EuclidianKDSplit>* toRet = new BinaryTreeNode<EuclidianKDSplit>();
		hotLevels.push_back(toRet);
	std::vector<uintptr_t> tmpLivePI;
	//build the tree
	std::vector<double> minCoord(spaceDim);
	std::vector<double> maxCoord(spaceDim);
	while(hotLevels.size()){
		//get the current level
			BinaryTreeNode<EuclidianKDSplit>* curFoc = hotLevels[hotLevels.size()-1];
			uintptr_t curI0 = hotI0[hotLevels.size()-1];
			uintptr_t curIN = allLivePI.size();
			hotI0.pop_back(); hotLevels.pop_back();
		//find the dimension with the largest spread
			for(uintptr_t d = 0; d<spaceDim; d++){
				minCoord[d] = 1.0 / 0.0;
				maxCoord[d] = -1.0 / 0.0;
			}
			for(uintptr_t pi = curI0; pi<curIN; pi++){
				uintptr_t pr = spaceDim * allLivePI[pi];
				for(uintptr_t d = 0; d<spaceDim; d++){
					maxCoord[d] = std::max(allPts[pr+d], maxCoord[d]);
					minCoord[d] = std::min(allPts[pr+d], minCoord[d]);
				}
			}
			uintptr_t largeDim = 0;
			double largeDimS = 0.0;
			for(uintptr_t d = 0; d<spaceDim; d++){
				double curDimS = maxCoord[d] - minCoord[d];
				if(curDimS > largeDimS){
					largeDim = d;
					largeDimS = curDimS;
				}
			}
			double splitAt = (maxCoord[largeDim] + minCoord[largeDim])/2;
		//split the hot points at the middle
			tmpLivePI.clear();
			uintptr_t lowI0 = allLivePI.size();
			for(uintptr_t pi = curI0; pi<curIN; pi++){
				uintptr_t pr = allLivePI[pi];
				if(allPts[spaceDim*pr + largeDim] <= splitAt){
					allLivePI.push_back(pr);
				}
				else{
					tmpLivePI.push_back(pr);
				}
			}
			uintptr_t higI0 = allLivePI.size();
			allLivePI.insert(allLivePI.end(), tmpLivePI.begin(), tmpLivePI.end());
		//decide whether to split
			if((lowI0 == higI0) || (higI0 == allLivePI.size())){
				curFoc->payload.atLevel.insert(curFoc->payload.atLevel.end(), allLivePI.begin()+curI0, allLivePI.begin()+curIN);
				allLivePI.erase(allLivePI.begin()+curI0, allLivePI.end());
				continue;
			}
		//set up for the sub-levels
			allLivePI.erase(allLivePI.begin()+curI0, allLivePI.begin()+curIN);
			curFoc->payload.onDim = largeDim;
			curFoc->payload.atVal = splitAt;
			curFoc->leftC = new BinaryTreeNode<EuclidianKDSplit>();
			curFoc->rightC = new BinaryTreeNode<EuclidianKDSplit>();
			hotLevels.push_back(curFoc->leftC);
			hotLevels.push_back(curFoc->rightC);
			hotI0.push_back(lowI0 - (curIN-curI0));
			hotI0.push_back(higI0 - (curIN-curI0));
	}
	return toRet;
}


