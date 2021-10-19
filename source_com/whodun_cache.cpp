#include "whodun_cache.h"

UniqueIDSet::UniqueIDSet(){
	seekID = 0;
}

UniqueIDSet::~UniqueIDSet(){}

void UniqueIDSet::retireID(uintptr_t toKill){
	liveIDs.erase(toKill);
}

uintptr_t UniqueIDSet::reserveID(){
	std::set<uintptr_t>::iterator findIt = liveIDs.find(seekID);
	while(findIt != liveIDs.end()){
		seekID++;
	}
	uintptr_t toRet = seekID;
	seekID++;
	return toRet;
}
