#include "whodun_parse_seq_graph.h"

#include <algorithm>

#include "whodun_stringext.h"

SequenceGraphReader::SequenceGraphReader(SequenceReader* toWrap){
	baseReader = toWrap;
	numNode = 0;
	nodeNames = 0;
	nodeCharInd = 0;
	numJumps = 0;
	jumpCharInd = 0;
	jumpNumJumps = 0;
	jumpTgtNames = 0;
	jumpPassThrough = 0;
}

SequenceGraphReader::~SequenceGraphReader(){
	//nothing needed
}

int SequenceGraphReader::readNextEntry(){
	//read raw
		int haveSeq = baseReader->readNextEntry();
		if(!haveSeq){ return 0; }
	//copy the name
		lastReadShortNameLen = baseReader->lastReadShortNameLen;
		lastReadNameLen = baseReader->lastReadNameLen;
		lastReadName = baseReader->lastReadName;
		lastReadHaveQual = 0;
	//clear the storage
		seqStore.clear();
		nodeNameStore.clear();
		nodeNameStoreP.clear();
		nodeCharIndV.clear();
		jumpCharIndV.clear();
		jumpNumJumpsV.clear();
		jumpTgtStore.clear();
		jumpTgtStoreP.clear();
		jumpTgtStorePP.clear();
	//parse the stupid thing
		uintptr_t baseSeqLen = baseReader->lastReadSeqLen;
		const char* baseSeq = baseReader->lastReadSeq;
		uintptr_t totNumTgts = 0;
		uintptr_t i = 0;
		while(i < baseSeqLen){
			size_t toNextCut = memcspn(baseSeq + i, baseSeqLen - i, "&[]", 3);
			if(toNextCut){
				seqStore.insert(seqStore.end(), baseSeq+i, baseSeq+i+toNextCut);
				i += toNextCut;
			}
			else if(baseSeq[i] == '&'){
				const char* semicLoc = (const char*)memchr(baseSeq+i, ';', baseSeqLen-i);
				if(!semicLoc){
					throw std::runtime_error("Named target name missing ending semicolon.");
				}
				nodeNameStore.insert(nodeNameStore.end(), baseSeq+i+1, semicLoc);
				nodeNameStore.push_back(0);
				nodeCharIndV.push_back(seqStore.size());
				i = (semicLoc - baseSeq) + 1;
			}
			else if(baseSeq[i] == '['){
				const char* rbrackLoc = (const char*)memchr(baseSeq+i, ']', baseSeqLen-i);
				if(!rbrackLoc){
					throw std::runtime_error("Jump set missing closing bracket.");
				}
				uintptr_t numTgts = 0;
				int isFT = 1;
				const char* curTxt = baseSeq+i+1;
				while(true){
					curTxt += memspn(curTxt, rbrackLoc - curTxt, " \r\t", 3);
					if(curTxt >= rbrackLoc){ break; }
					const char* namEnd = curTxt + memcspn(curTxt, rbrackLoc - curTxt, " \r\t", 3);
					if((namEnd-curTxt)==1 && (*curTxt == '/')){
						isFT = 0;
					}
					else{
						jumpTgtStore.insert(jumpTgtStore.end(), curTxt, namEnd);
						jumpTgtStore.push_back(0);
						numTgts++;
					}
					curTxt = namEnd;
				}
				if(numTgts == 0){
					throw std::runtime_error("Empty jump set.");
				}
				totNumTgts += numTgts;
				jumpCharIndV.push_back(seqStore.size());
				jumpNumJumpsV.push_back(numTgts);
				jumpPassThroughV.push_back(isFT);
				i = (rbrackLoc - baseSeq) + 1;
			}
			else{
				throw std::runtime_error("Mismatched bracket.");
			}
		}
	//link up some of the vectors
		char* nodeNTmp = nodeNameStore.size() ? &(nodeNameStore[0]) : 0;
		nodeNameStoreP.resize(nodeCharIndV.size());
		for(uintptr_t i = 0; i<nodeCharIndV.size(); i++){
			nodeNameStoreP[i] = nodeNTmp;
			nodeNTmp += strlen(nodeNTmp);
			nodeNTmp++;
		}
		char* jumpNTmp = jumpTgtStore.size() ? &(jumpTgtStore[0]) : 0;
		jumpTgtStoreP.resize(totNumTgts);
		for(uintptr_t i = 0; i<totNumTgts; i++){
			jumpTgtStoreP[i] = jumpNTmp;
			jumpNTmp += strlen(jumpNTmp);
			jumpNTmp++;
		}
		char** jumpPTmp = totNumTgts ? &(jumpTgtStoreP[0]) : 0;
		jumpTgtStorePP.resize(jumpCharIndV.size());
		for(uintptr_t i = 0; i<jumpCharIndV.size(); i++){
			jumpTgtStorePP[i] = jumpPTmp;
			jumpPTmp += jumpNumJumpsV[i];
		}
	//and make the results visible
		lastReadSeqLen = seqStore.size();
		lastReadSeq = seqStore.size() ? &(seqStore[0]) : 0;
		numNode = nodeCharIndV.size();
		nodeNames = numNode ? &(nodeNameStoreP[0]) : 0;
		nodeCharInd = numNode ? &(nodeCharIndV[0]) : 0;
		numJumps = jumpCharIndV.size();
		jumpCharInd = numJumps ? &(jumpCharIndV[0]) : 0;
		jumpNumJumps = numJumps ? &(jumpNumJumpsV[0]) : 0;
		jumpTgtNames = numJumps ? &(jumpTgtStorePP[0]) : 0;
		jumpPassThrough = numJumps ? &(jumpPassThroughV[0]) : 0;
	return 1;
}

SequenceGraph::SequenceGraph(){}

SequenceGraph::~SequenceGraph(){}

void SequenceGraphReader::flattenGraph(SequenceGraph* storeGraph){
	//clone the sequence
		storeGraph->seqStore.clear();
		storeGraph->seqStore.insert(storeGraph->seqStore.end(), seqStore.begin(), seqStore.end());
	//figure out where the nodes are
		nodeNameMap.clear();
		for(uintptr_t i = 0; i<numNode; i++){
			nodeNameMap[nodeNames[i]] = nodeCharInd[i];
		}
	//start adding forward jumps
		storeGraph->forwJumps.clear();
		for(uintptr_t i = 0; i<numJumps; i++){
			uintptr_t jumpBefore = jumpCharInd[i];
			uintptr_t numJ = jumpNumJumps[i];
			for(uintptr_t j = 0; j<numJ; j++){
				uintptr_t jumpTo = nodeNameMap[jumpTgtNames[i][j]];
				storeGraph->forwJumps.push_back(std::pair<uintptr_t,uintptr_t>(jumpBefore,jumpTo));
			}
			if(jumpPassThrough[i]){
				storeGraph->forwJumps.push_back(std::pair<uintptr_t,uintptr_t>(jumpBefore,jumpBefore));
			}
		}
		std::sort(storeGraph->forwJumps.begin(), storeGraph->forwJumps.end());
	//all targets that have no origin entries need a self origin
		nodeSelfL.clear();
		for(uintptr_t i = 0; i<storeGraph->forwJumps.size(); i++){
			uintptr_t curTgt = storeGraph->forwJumps[i].second;
			std::vector< std::pair<uintptr_t,uintptr_t> >::iterator tgtOriB = std::lower_bound(storeGraph->forwJumps.begin(), storeGraph->forwJumps.end(), std::pair<uintptr_t,uintptr_t>(curTgt, 0));
			std::vector< std::pair<uintptr_t,uintptr_t> >::iterator tgtOriE = std::upper_bound(storeGraph->forwJumps.begin(), storeGraph->forwJumps.end(), std::pair<uintptr_t,uintptr_t>(curTgt, (uintptr_t)-1));
			if(tgtOriB == tgtOriE){
				nodeSelfL.insert(curTgt);
			}
		}
		for(std::set<uintptr_t>::iterator curIt = nodeSelfL.begin(); curIt != nodeSelfL.end(); curIt++){
			storeGraph->forwJumps.push_back( std::pair<uintptr_t,uintptr_t>(*curIt, *curIt) );
		}
		std::sort(storeGraph->forwJumps.begin(), storeGraph->forwJumps.end());
	//add the backward jumps
		storeGraph->backJumps.clear();
		for(uintptr_t i = 0; i<storeGraph->forwJumps.size(); i++){
			std::pair<uintptr_t,uintptr_t> curDat = storeGraph->forwJumps[i];
			std::pair<uintptr_t,uintptr_t> curRev = std::pair<uintptr_t,uintptr_t>(curDat.second, curDat.first);
			storeGraph->backJumps.push_back(curRev);
		}
		std::sort(storeGraph->backJumps.begin(), storeGraph->backJumps.end());
}

