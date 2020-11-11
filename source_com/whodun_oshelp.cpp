#include "whodun_oshelp.h"

#include <stdexcept>

OSMutex::OSMutex(){
	myMut = makeMutex();
}

OSMutex::~OSMutex(){
	killMutex(myMut);
}

void OSMutex::lock(){
	lockMutex(myMut);
}

void OSMutex::unlock(){
	unlockMutex(myMut);
}

OSCondition::OSCondition(OSMutex* baseMut){
	saveMut = baseMut->myMut;
	myCond = makeCondition(saveMut);
}

OSCondition::~OSCondition(){
	killCondition(myCond);
}

void OSCondition::wait(){
	waitCondition(saveMut, myCond);
}

void OSCondition::signal(){
	signalCondition(saveMut, myCond);
}

void OSCondition::broadcast(){
	broadcastCondition(saveMut, myCond);
}

OSDLLSO::OSDLLSO(const char* dllName){
	myDLL = loadInDLL(dllName);
	if(myDLL == 0){
		throw std::runtime_error("Could not load dll.");
	}
}

OSDLLSO::~OSDLLSO(){
	unloadDLL(myDLL);
}

void* OSDLLSO::get(const char* locName){
	return getDLLLocation(myDLL, locName);
}


