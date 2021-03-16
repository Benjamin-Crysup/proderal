#include "whodun_align_graph.h"

#include <set>
#include <queue>
#include <string.h>
#include <iostream>
#include <stdexcept>
#include <algorithm>

#include "whodun_stringext.h"

GraphSequenceAlignmentIteration::GraphSequenceAlignmentIteration(GraphSequenceAlignment* forAln){
	baseAln = forAln;
}

GraphSequenceAlignmentIteration::~GraphSequenceAlignmentIteration(){}

GraphSequenceAlignment::GraphSequenceAlignment(){}

GraphSequenceAlignment::~GraphSequenceAlignment(){}

int GraphSequenceAlignment::findAlignmentScores(GraphSequenceAlignmentIteration* theIter, int numFind, intptr_t* storeCost, intptr_t minScore, intptr_t maxDupDeg){
	std::greater<intptr_t> compMeth;
	unsigned numFindU = numFind;
	uintptr_t numFound = 0;
	intptr_t curMin = minScore;
	intptr_t realLim = minScore;
	GraphSequenceAlignmentIteration* curIter = theIter;
	startFuzzyIteration(curIter, realLim, maxDupDeg, numFind);
	while(curIter->getNextAlignment()){
		intptr_t* tsIt = std::lower_bound(storeCost, storeCost + numFound, curIter->alnScore, compMeth);
		uintptr_t tsInd = tsIt - storeCost;
		if((tsInd < numFound) && (*tsIt == curIter->alnScore)){ continue; }
		if(tsInd == numFindU){ continue; }
		if(numFound == numFindU){ numFound--; }
		memmove(tsIt + 1, tsIt, (numFound - tsInd)*sizeof(intptr_t));
		*tsIt = curIter->alnScore;
		numFound++;
		if((numFound == numFindU) && (storeCost[numFindU-1] > curMin)){
			curMin = storeCost[numFindU-1];
			realLim = curMin + 1;
			curIter->updateMinimumScore(realLim);
		}
	}
	return numFound;
}

SequenceGraphOperations::SequenceGraphOperations(){}

SequenceGraphOperations::~SequenceGraphOperations(){}

void SequenceGraphOperations::getSequenceSubGraph(SequenceGraph* bigGraph, SequenceGraph* storeGraph, std::set<uintptr_t>* startNodes, uintptr_t numExpand, std::vector< std::pair<uintptr_t,uintptr_t> >* subSeqMap){
	//set up
		storeGraph->seqStore.clear();
		storeGraph->forwJumps.clear();
		storeGraph->backJumps.clear();
		subSeqMap->clear();
		gssgHandChars.clear();
		gssgWaitChars.clear();
		gssgWaitChars.insert(gssgWaitChars.end(), startNodes->begin(), startNodes->end());
	//expand out
		for(uintptr_t expCyc = 0; expCyc <= numExpand; expCyc++){ //<= is right: there is a lag before things actually expand
			gssgNextWait.clear();
			for(uintptr_t wci = 0; wci < gssgWaitChars.size(); wci++){
				uintptr_t curCI = gssgWaitChars[wci];
				if(gssgHandChars.count(curCI)){continue;}
				//get the relevant links (in both directions)
				std::vector< std::pair<uintptr_t,uintptr_t> >::iterator forLinkItB = std::lower_bound(bigGraph->forwJumps.begin(), bigGraph->forwJumps.end(), std::pair<uintptr_t,uintptr_t>(curCI, 0));
				std::vector< std::pair<uintptr_t,uintptr_t> >::iterator forLinkItE = std::upper_bound(bigGraph->forwJumps.begin(), bigGraph->forwJumps.end(), std::pair<uintptr_t,uintptr_t>(curCI, -1));
				std::vector< std::pair<uintptr_t,uintptr_t> >::iterator bakLinkItB = std::lower_bound(bigGraph->backJumps.begin(), bigGraph->backJumps.end(), std::pair<uintptr_t,uintptr_t>(curCI, 0));
				std::vector< std::pair<uintptr_t,uintptr_t> >::iterator bakLinkItE = std::upper_bound(bigGraph->backJumps.begin(), bigGraph->backJumps.end(), std::pair<uintptr_t,uintptr_t>(curCI, -1));
				bool needCFor = forLinkItB == forLinkItE;
				bool needCBak = bakLinkItB == bakLinkItE;
				//follow the links (in both directions)
				while(forLinkItB != forLinkItE){
					uintptr_t linkCI = forLinkItB->second;
					if(gssgHandChars.count(linkCI)){goto cur_continueF;}
					needCFor = needCFor || (forLinkItB->first == forLinkItB->second);
					gssgNextWait.push_back(linkCI);
					cur_continueF:
					forLinkItB++;
				}
				while(bakLinkItB != bakLinkItE){
					uintptr_t linkCI = bakLinkItB->second;
					if(gssgHandChars.count(linkCI)){goto cur_continueB;}
					needCBak = needCBak || (bakLinkItB->first == bakLinkItB->second);
					gssgNextWait.push_back(linkCI);
					cur_continueB:
					bakLinkItB++;
				}
				//follow the character sequence
				if(needCFor){
					uintptr_t forCI = curCI + 1;
					if(forCI <= bigGraph->seqStore.size()){
						gssgNextWait.push_back(curCI+1);
					}
				}
				if(needCBak && curCI){
					gssgNextWait.push_back(curCI-1);
				}
				//add character to handled set
				gssgHandChars.insert(curCI);
			}
			//swap waiting
			std::swap(gssgNextWait, gssgWaitChars);
		}
	//get the relevant sequence
	gssgInvSeqMap.clear();
	uintptr_t nextExp = -1;
	std::set<uintptr_t>::iterator curHCIt = gssgHandChars.begin();
	while(curHCIt != gssgHandChars.end()){
		uintptr_t curCI = *curHCIt;
		if(curCI != nextExp){
			subSeqMap->push_back( std::pair<uintptr_t,uintptr_t>(storeGraph->seqStore.size(), curCI) );
		}
		gssgInvSeqMap[curCI] = storeGraph->seqStore.size();
		if(curCI < bigGraph->seqStore.size()){
			storeGraph->seqStore.push_back(bigGraph->seqStore[curCI]);
		}
		nextExp = curCI+1;
		curHCIt++;
	}
	//get the relevant links
	curHCIt = gssgHandChars.begin();
	while(curHCIt != gssgHandChars.end()){
		uintptr_t curCI = *curHCIt;
		std::vector< std::pair<uintptr_t,uintptr_t> >::iterator forLinkItB = std::lower_bound(bigGraph->forwJumps.begin(), bigGraph->forwJumps.end(), std::pair<uintptr_t,uintptr_t>(curCI, 0));
		std::vector< std::pair<uintptr_t,uintptr_t> >::iterator forLinkItE = std::upper_bound(bigGraph->forwJumps.begin(), bigGraph->forwJumps.end(), std::pair<uintptr_t,uintptr_t>(curCI, -1));
		while(forLinkItB != forLinkItE){
			uintptr_t toTgt = forLinkItB->second;
			if(gssgInvSeqMap.find(toTgt) != gssgInvSeqMap.end()){
				storeGraph->forwJumps.push_back(std::pair<uintptr_t,uintptr_t>(gssgInvSeqMap[forLinkItB->first], gssgInvSeqMap[forLinkItB->second]));
			}
			forLinkItB++;
		}
		std::vector< std::pair<uintptr_t,uintptr_t> >::iterator bakLinkItB = std::lower_bound(bigGraph->backJumps.begin(), bigGraph->backJumps.end(), std::pair<uintptr_t,uintptr_t>(curCI, 0));
		std::vector< std::pair<uintptr_t,uintptr_t> >::iterator bakLinkItE = std::upper_bound(bigGraph->backJumps.begin(), bigGraph->backJumps.end(), std::pair<uintptr_t,uintptr_t>(curCI, -1));
		while(bakLinkItB != bakLinkItE){
			uintptr_t toTgt = bakLinkItB->second;
			if(gssgInvSeqMap.find(toTgt) != gssgInvSeqMap.end()){
				storeGraph->backJumps.push_back(std::pair<uintptr_t,uintptr_t>(gssgInvSeqMap[bakLinkItB->first], gssgInvSeqMap[bakLinkItB->second]));
			}
			bakLinkItB++;
		}
		curHCIt++;
	}
}

SequenceGraphTopoSort::SequenceGraphTopoSort(){}

SequenceGraphTopoSort::~SequenceGraphTopoSort(){}

void SequenceGraphOperations::topologicalSort(SequenceGraph* toSort, SequenceGraphTopoSort* toStore){
	uintptr_t maxi = -1;
	toStore->sortIndices.clear();
	toStore->indRanges.clear();
	toStore->rangeCycle.clear();
	//start by running Kosaraju's algorithm
		//visit pass
		topoNodeOrderL.clear();
		topoNodeVisit.clear();
			topoNodeVisit.insert(topoNodeVisit.end(), toSort->seqStore.size()+1, false);
		for(uintptr_t wni = 0; wni < topoNodeVisit.size(); wni++){
			if(topoNodeVisit[wni]){ continue; }
			topoVisitStack.clear();
			
			uintptr_t nu = wni;
			uintptr_t forLinkItB;
			uintptr_t forLinkItE;
			bool needCFor;
			//Visit(u) - the following uses goto because actual recursion might overflow
			visit_u:
			//mark u as visited
				if(topoNodeVisit[nu]){ goto visit_return; }
				topoNodeVisit[nu] = true;
			//for each out-neighbor v of u, run Visit(v)
				forLinkItB = std::lower_bound(toSort->forwJumps.begin(), toSort->forwJumps.end(), std::pair<uintptr_t,uintptr_t>(nu, 0)) - toSort->forwJumps.begin();
				forLinkItE = std::upper_bound(toSort->forwJumps.begin(), toSort->forwJumps.end(), std::pair<uintptr_t,uintptr_t>(nu, maxi)) - toSort->forwJumps.begin();
				needCFor = forLinkItB == forLinkItE;
				
				revisit_u: //return target (if frames on stack)
				if(forLinkItB != forLinkItE){
					needCFor = needCFor || (nu == toSort->forwJumps[forLinkItB].second);
					topoSortVisitStack saveFrame = {nu, forLinkItB+1, forLinkItE, needCFor};
					topoVisitStack.push_back(saveFrame);
					nu = toSort->forwJumps[forLinkItB].second;
					goto visit_u;
				}
				needCFor = needCFor && ((nu+1) <= toSort->seqStore.size());
				if(needCFor){
					topoSortVisitStack saveFrame = {nu, forLinkItB, forLinkItE, needCFor};
					topoVisitStack.push_back(saveFrame);
					nu++;
					goto visit_u;
				}
			//prepend u to L
				topoNodeOrderL.push_front(nu);
				//"return"
				visit_return:
				if(topoVisitStack.size()){
					topoSortVisitStack tmpUPack = topoVisitStack[topoVisitStack.size()-1];
					nu = tmpUPack.nodeI;
					forLinkItB = tmpUPack.fromFLI;
					forLinkItE = tmpUPack.toFLI;
					needCFor = tmpUPack.needNC;
					topoVisitStack.pop_back();
					goto revisit_u;
				}
		}
		//assign pass
		topoMetaNodeAssn.resize(toSort->seqStore.size()+1);
			for(uintptr_t i = 0; i<topoMetaNodeAssn.size(); i++){
				topoMetaNodeAssn[i] = std::pair<uintptr_t,uintptr_t>(maxi, i);
			}
		for(uintptr_t wni = 0; wni < topoNodeOrderL.size(); wni++){
			uintptr_t nu = topoNodeOrderL[wni];
			if(topoMetaNodeAssn[nu].first != maxi){ continue; }
			topoVisitStack.clear();
			uintptr_t assNode = nu;
			
			uintptr_t forLinkItB;
			uintptr_t forLinkItE;
			bool needCFor;
			//Assign(u) - the following uses goto because actual recursion might overflow
			assign_u:
			//mark u as owned
				if(topoMetaNodeAssn[nu].first != maxi){ goto assign_return; }
				topoMetaNodeAssn[nu].first = assNode;
			//for each in-neighbor v of u, run Assign(v)
				forLinkItB = std::lower_bound(toSort->backJumps.begin(), toSort->backJumps.end(), std::pair<uintptr_t,uintptr_t>(nu, 0)) - toSort->backJumps.begin();
				forLinkItE = std::upper_bound(toSort->backJumps.begin(), toSort->backJumps.end(), std::pair<uintptr_t,uintptr_t>(nu, maxi)) - toSort->backJumps.begin();
				needCFor = forLinkItB == forLinkItE;
				
				reassign_u: //return target (if frames on stack)
				if(forLinkItB != forLinkItE){
					needCFor = needCFor || (nu == toSort->backJumps[forLinkItB].second);
					topoSortVisitStack saveFrame = {nu, forLinkItB+1, forLinkItE, needCFor};
					topoVisitStack.push_back(saveFrame);
					nu = toSort->backJumps[forLinkItB].second;
					goto assign_u;
				}
				needCFor = needCFor && nu;
				if(needCFor){
					topoSortVisitStack saveFrame = {nu, forLinkItB, forLinkItE, needCFor};
					topoVisitStack.push_back(saveFrame);
					nu--;
					goto assign_u;
				}
			//"return"
				assign_return:
				if(topoVisitStack.size()){
					topoSortVisitStack tmpUPack = topoVisitStack[topoVisitStack.size()-1];
					nu = tmpUPack.nodeI;
					forLinkItB = tmpUPack.fromFLI;
					forLinkItE = tmpUPack.toFLI;
					needCFor = tmpUPack.needNC;
					topoVisitStack.pop_back();
					goto reassign_u;
				}
		}
	//note the nodes in each metanode
		topoMetaNodeAssnRev.resize(topoMetaNodeAssn.size());
		for(uintptr_t i = 0; i<topoMetaNodeAssn.size(); i++){
			std::pair<uintptr_t,uintptr_t> tmpSav = topoMetaNodeAssn[i];
			topoMetaNodeAssnRev[i] = std::pair<uintptr_t,uintptr_t>(tmpSav.second, tmpSav.first);
		}
		std::sort(topoMetaNodeAssn.begin(), topoMetaNodeAssn.end());
		std::sort(topoMetaNodeAssnRev.begin(), topoMetaNodeAssnRev.end());
	//count the number of forward links to each "metanode"
		topoMetaCountW.clear();
		topoMetaCountW.insert(topoMetaCountW.end(), topoMetaNodeAssn.size(), 0);
		for(uintptr_t metaI = 0; metaI < topoMetaCountW.size(); metaI++){
			std::vector< std::pair<uintptr_t,uintptr_t> >::iterator metRNItB = std::lower_bound(topoMetaNodeAssn.begin(), topoMetaNodeAssn.end(), std::pair<uintptr_t,uintptr_t>(metaI, 0));
			std::vector< std::pair<uintptr_t,uintptr_t> >::iterator metRNItE = std::upper_bound(topoMetaNodeAssn.begin(), topoMetaNodeAssn.end(), std::pair<uintptr_t,uintptr_t>(metaI, maxi));
			while(metRNItB != metRNItE){
				uintptr_t curNI = metRNItB->second;
				std::vector< std::pair<uintptr_t,uintptr_t> >::iterator bakLinkItB = std::lower_bound(toSort->backJumps.begin(), toSort->backJumps.end(), std::pair<uintptr_t,uintptr_t>(curNI, 0));
				std::vector< std::pair<uintptr_t,uintptr_t> >::iterator bakLinkItE = std::upper_bound(toSort->backJumps.begin(), toSort->backJumps.end(), std::pair<uintptr_t,uintptr_t>(curNI, maxi));
				bool needCBak = bakLinkItB == bakLinkItE;
				while(bakLinkItB != bakLinkItE){
					uintptr_t prevNI = bakLinkItB->second;
					if(prevNI == curNI){needCBak = true;}
					uintptr_t prevMI = std::lower_bound(topoMetaNodeAssnRev.begin(), topoMetaNodeAssnRev.end(), std::pair<uintptr_t,uintptr_t>(prevNI, 0))->second;
					if(prevMI != metaI){
						topoMetaCountW[metaI]++;
					}
					bakLinkItB++;
				}
				needCBak = needCBak && curNI;
				if(needCBak){
					uintptr_t prevNI = curNI - 1;
					uintptr_t prevMI = std::lower_bound(topoMetaNodeAssnRev.begin(), topoMetaNodeAssnRev.end(), std::pair<uintptr_t,uintptr_t>(prevNI, 0))->second;
					if(prevMI != metaI){
						topoMetaCountW[metaI]++;
					}
				}
				metRNItB++;
			}
		}
	//note which meta nodes have 0 at the start
		topoReadyMetas.clear();
		for(uintptr_t metaI = 0; metaI<topoMetaCountW.size(); metaI++){
			if(topoMetaCountW[metaI]){ continue; }
			topoReadyMetas.push_back(metaI);
		}
	//topological sort the meta nodes
		topoReadyMetasC.clear();
		topoReadyMetasNext.clear();
		while(topoReadyMetas.size()){
			//handle the easy cases
			uintptr_t numGroupS = toStore->sortIndices.size();
			for(uintptr_t mri = 0; mri < topoReadyMetas.size(); mri++){
				//figure out which type of meta node it is
					uintptr_t metaI = topoReadyMetas[mri];
					std::vector< std::pair<uintptr_t,uintptr_t> >::iterator metRNItB = std::lower_bound(topoMetaNodeAssn.begin(), topoMetaNodeAssn.end(), std::pair<uintptr_t,uintptr_t>(metaI, 0));
					std::vector< std::pair<uintptr_t,uintptr_t> >::iterator metRNItE = std::upper_bound(topoMetaNodeAssn.begin(), topoMetaNodeAssn.end(), std::pair<uintptr_t,uintptr_t>(metaI, maxi));
					uintptr_t metNN = metRNItE - metRNItB;
					if(metNN == 0){ continue; }
					else if(metNN > 1){
						topoReadyMetasC.push_back(metNN);
						continue;
					}
				//easy case, add node
				#define TOPO_NODE_ADD_AND_FOLLOW_LINKS \
					uintptr_t curNI = metRNItB->second;\
						toStore->sortIndices.push_back(curNI);\
					std::vector< std::pair<uintptr_t,uintptr_t> >::iterator forLinkItB = std::lower_bound(toSort->forwJumps.begin(), toSort->forwJumps.end(), std::pair<uintptr_t,uintptr_t>(curNI, 0));\
					std::vector< std::pair<uintptr_t,uintptr_t> >::iterator forLinkItE = std::upper_bound(toSort->forwJumps.begin(), toSort->forwJumps.end(), std::pair<uintptr_t,uintptr_t>(curNI, maxi));\
					bool needCFor = forLinkItB == forLinkItE;\
					while(forLinkItB != forLinkItE){\
						uintptr_t forwNI = forLinkItB->second;\
						if(forwNI == curNI){ needCFor = true; }\
						uintptr_t forwMI = std::lower_bound(topoMetaNodeAssnRev.begin(), topoMetaNodeAssnRev.end(), std::pair<uintptr_t,uintptr_t>(forwNI, 0))->second;\
						if(forwMI != metaI){\
							topoMetaCountW[forwMI]--;\
							if(topoMetaCountW[forwMI] == 0){\
								topoReadyMetasNext.push_back(forwMI);\
							}\
						}\
						forLinkItB++;\
					}\
					needCFor = needCFor && ((curNI+1) <= toSort->seqStore.size());\
					if(needCFor){\
						uintptr_t forwNI = curNI + 1;\
						uintptr_t forwMI = std::lower_bound(topoMetaNodeAssnRev.begin(), topoMetaNodeAssnRev.end(), std::pair<uintptr_t,uintptr_t>(forwNI, 0))->second;\
						if(forwMI != metaI){\
							topoMetaCountW[forwMI]--;\
							if(topoMetaCountW[forwMI] == 0){\
								topoReadyMetasNext.push_back(forwMI);\
							}\
						}\
					}
				TOPO_NODE_ADD_AND_FOLLOW_LINKS
			}
			if(numGroupS != toStore->sortIndices.size()){
				toStore->indRanges.push_back( std::pair<uintptr_t,uintptr_t>(numGroupS, toStore->sortIndices.size()) );
				toStore->rangeCycle.push_back(false);
			}
			//handle the cyclic cases
			for(uintptr_t mri = 0; mri < topoReadyMetasC.size(); mri++){
				numGroupS = toStore->sortIndices.size();
				uintptr_t metaI = topoReadyMetas[mri];
				std::vector< std::pair<uintptr_t,uintptr_t> >::iterator metRNItB = std::lower_bound(topoMetaNodeAssn.begin(), topoMetaNodeAssn.end(), std::pair<uintptr_t,uintptr_t>(metaI, 0));
				std::vector< std::pair<uintptr_t,uintptr_t> >::iterator metRNItE = std::upper_bound(topoMetaNodeAssn.begin(), topoMetaNodeAssn.end(), std::pair<uintptr_t,uintptr_t>(metaI, maxi));
				//add the nodes
				while(metRNItB != metRNItE){
					TOPO_NODE_ADD_AND_FOLLOW_LINKS
				}
				//note the cycle
				toStore->indRanges.push_back( std::pair<uintptr_t,uintptr_t>(numGroupS, toStore->sortIndices.size()) );
				toStore->rangeCycle.push_back(true);
			}
			//get ready for the next round
			topoReadyMetas.clear();
			std::swap(topoReadyMetas, topoReadyMetasNext);
		}
}

