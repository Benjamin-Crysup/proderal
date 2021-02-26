
#include <assert.h>
#include <stdexcept>

#include "whodun_thread.h"
#include "whodun_oshook.h"

void threadFunc(void* thePoolP){
	ThreadPool* thePool = (ThreadPool*)thePoolP;
	lockMutex(thePool->taskMut);
	while(thePool->poolLive){
		if(thePool->waitTask.size()){
			//run the task
			ThreadPoolTaskInfo curTask = thePool->waitTask[0];
			thePool->waitTask.pop_front();
			unlockMutex(thePool->taskMut);
			curTask.taskFun(curTask.taskUni);
			lockMutex(thePool->taskMut);
			//let things know they're done
			thePool->doneTasks.insert(curTask.taskID);
			thePool->hotTasks.erase(curTask.taskID);
			std::map<uintptr_t,void*>::iterator idIt = thePool->doneCondMap.find(curTask.taskID);
			if(idIt != thePool->doneCondMap.end()){
				signalCondition(thePool->taskMut, idIt->second);
			}
		}
		else{
			waitCondition(thePool->taskMut, thePool->taskCond);
		}
	}
	unlockMutex(thePool->taskMut);
}

ThreadPool::ThreadPool(int numThread){
	nextID = 0;
	numThr = numThread;
	poolLive = true;
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
	if(hotTasks.size() || doneTasks.size()){
		//killing pool with stuff in the queue
		std::terminate();
	}
	for(uintptr_t i = 0; i<saveConds.size(); i++){
		killCondition(saveConds[i]);
	}
	killCondition(taskCond);
	killMutex(taskMut);
}

uintptr_t ThreadPool::addTask(void (*toDo)(void*), void* toPass){
	lockMutex(taskMut);
		while(hotTasks.count(nextID) || doneTasks.count(nextID)){
			nextID++;
		}
		ThreadPoolTaskInfo nextTask = {nextID, toDo, toPass};
		waitTask.push_back( nextTask );
		hotTasks.insert(nextID);
		nextID++;
		signalCondition(taskMut, taskCond);
	unlockMutex(taskMut);
	return nextTask.taskID;
}

void ThreadPool::joinTask(uintptr_t taskID){
	lockMutex(taskMut);
		std::set<uintptr_t>::iterator doneIt = doneTasks.find(taskID);
		if(doneIt == doneTasks.end()){
			//make a unique condition to wait on
				void* newCond;
				if(saveConds.size()){
					newCond = saveConds[saveConds.size()-1];
					saveConds.pop_back();
				}
				else{
					newCond = makeCondition(taskMut);
				}
			//note the wait and wait
				doneCondMap[taskID] = newCond;
				while(doneIt == doneTasks.end()){
					waitCondition(taskMut, newCond);
					doneIt = doneTasks.find(taskID);
				}
			//clean up
				saveConds.push_back(newCond);
				doneCondMap.erase(taskID);
		}
		doneTasks.erase(doneIt);
	unlockMutex(taskMut);
}

