
#include <assert.h>

#include "whodun_thread.h"
#include "whodun_oshook.h"

void threadFunc(void* thePoolP){
	ThreadPool* thePool = (ThreadPool*)thePoolP;
	lockMutex(thePool->taskMut);
	while(thePool->poolLive){
		if(thePool->waitTask.size()){
			std::pair<void(*)(void*),void*> curTask = thePool->waitTask[0];
			thePool->waitTask.pop_front();
			unlockMutex(thePool->taskMut);
			curTask.first(curTask.second);
			lockMutex(thePool->taskMut);
		}
		else{
			waitCondition(thePool->taskMut, thePool->taskCond);
		}
	}
	unlockMutex(thePool->taskMut);
}

ThreadPool::ThreadPool(int numThread){
	numThr = numThread;
	poolLive = true;
	waitTask = std::deque< std::pair<void(*)(void*), void*> >();
	taskMut = makeMutex();
	taskCond = makeCondition(taskMut);
	for(int i = 0; i<numThread; i++){
		liveThread.push_back(startThread(threadFunc, this));
	}
}

ThreadPool::~ThreadPool(){
	lockMutex(taskMut);
		poolLive = false;
		broadcastCondition(taskMut, taskCond);
	unlockMutex(taskMut);
	for(unsigned i = 0; i<liveThread.size(); i++){
		joinThread(liveThread[i]);
	}
	killCondition(taskCond);
	killMutex(taskMut);
}

void ThreadPool::addTask(void (*toDo)(void*), void* toPass){
	lockMutex(taskMut);
		waitTask.push_back( std::pair<void(*)(void*), void*>(toDo, toPass) );
		signalCondition(taskMut, taskCond);
	unlockMutex(taskMut);
}

ThreadMultiWait::ThreadMultiWait(){
	myMut = makeMutex();
	myCond = makeCondition(myMut);
	curWait = 0;
}

ThreadMultiWait::~ThreadMultiWait(){
	killCondition(myCond);
	killMutex(myMut);
}

void ThreadMultiWait::waitOn(unsigned numWait){
	lockMutex(myMut);
	curWait += numWait;
	while(curWait){
		waitCondition(myMut, myCond);
	}
	unlockMutex(myMut);
}

void ThreadMultiWait::unwaitOne(){
	lockMutex(myMut);
	curWait--;
	if(curWait == 0){
		broadcastCondition(myMut, myCond);
	}
	unlockMutex(myMut);
}

