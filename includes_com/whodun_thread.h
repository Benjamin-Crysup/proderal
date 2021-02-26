#ifndef WHODUN_THREAD_H
#define WHODUN_THREAD_H 1

#include <set>
#include <map>
#include <deque>
#include <vector>
#include <stdint.h>

#include "whodun_cache.h"
#include "whodun_oshook.h"

/**Internal storage for a task.*/
typedef struct{
	/**The ID of the task.*/
	uintptr_t taskID;
	/**The function for the task.*/
	void(*taskFun)(void*);
	/**The uniform for the task.*/
	void* taskUni;
} ThreadPoolTaskInfo;

/**A pool of reusable threads.*/
class ThreadPool{
public:
	/**
	 * Set up the threads.
	 * @param numThread The number of threads.
	 */
	ThreadPool(int numThread);
	/**Kill the threads.*/
	~ThreadPool();
	/**
	 * Add a task to run.
	 * @param toDo The function to run.
	 * @param toPass The thing to add.
	 * @return The ID of the task. Will need to join at some point.
	 */
	uintptr_t addTask(void (*toDo)(void*), void* toPass);
	/**
	 * Wait for a task to finish.
	 * @param taskID The ID of the task.
	 */
	void joinTask(uintptr_t taskID);
	/**The number of threads in this pool.*/
	int numThr;
	/**The task mutex.*/
	void* taskMut;
	/**The task conditions.*/
	void* taskCond;
	/**The ID to use for the next job.*/
	uintptr_t nextID;
	/**The tasks waiting to be run.*/
	std::deque<ThreadPoolTaskInfo> waitTask;
	/**The IDs of the running/waiting tasks.*/
	std::set<uintptr_t> hotTasks;
	/**The IDs of the finished tasks.*/
	std::set<uintptr_t> doneTasks;
	/**Condition variables for things actively waiting on jobs.*/
	std::map<uintptr_t,void*> doneCondMap;
	/**Avoid allocating many conditions.*/
	std::vector<void*> saveConds;
	/**Whether the pool is live.*/
	bool poolLive;
	/**The live threads.*/
	std::vector<void*> liveThread;
};

/**Allocate containers in a threadsafe, reusable manner.*/
template <typename OfT>
class ThreadsafeReusableContainerCache{
public:
	/**Get the lock ready.*/
	ThreadsafeReusableContainerCache(){
		myMut = makeMutex();
	}
	/**Destroy the allocated stuff.*/
	~ThreadsafeReusableContainerCache(){
		killMutex(myMut);
	}
	/**
	 * Allocate a container.
	 * @return The allocated container. Must be returned with dealloc.
	 */
	OfT* alloc(){
		lockMutex(myMut);
		OfT* toRet = actCache.alloc();
		unlockMutex(myMut);
		return toRet;
	}
	/**
	 * Return a container.
	 * @param toDe The container to return.
	 */
	void dealloc(OfT* toDe){
		lockMutex(myMut);
		actCache.dealloc(toDe);
		unlockMutex(myMut);
	}
	
	/**The mutex for this thing.*/
	void* myMut;
	/**The actual cache.*/
	ReusableContainerCache<OfT> actCache;
};

/**Collect stuff between threads.*/
template <typename OfT>
class ThreadProdComCollector{
public:
	/**Whether end has been called.*/
	bool queueEnded;
	/**The maximum size of the queue.*/
	uintptr_t maxQueue;
	/**Lock for edit.*/
	void* myMut;
	/**Wait for more things.*/
	void* myConMore;
	/**Wait for less things.*/
	void* myConLess;
	/**Things added and waiting to be handled.*/
	std::deque<OfT*> waitQueue;
	/**Protect by a lock.*/
	ThreadsafeReusableContainerCache<OfT> taskCache;
	
	/**
	 * Set up a producer consumer thing.
	 * @param maxQueueSize The maximum number of things in the queue.
	 */
	ThreadProdComCollector(uintptr_t maxQueueSize){
		queueEnded = false;
		maxQueue = maxQueueSize;
		myMut = makeMutex();
		myConMore = makeCondition(myMut);
		myConLess = makeCondition(myMut);
	}
	
	~ThreadProdComCollector(){
		for(uintptr_t i = 0; i<waitQueue.size(); i++){
			taskCache.dealloc(waitQueue[i]);
		}
		killCondition(myConMore);
		killCondition(myConLess);
		killMutex(myMut);
	}
	
	/**
	 * Add the thing to the job list.
	 * @param toAdd The thing to add.
	 */
	void addThing(OfT* toAdd){
		lockMutex(myMut);
			while(waitQueue.size() >= maxQueue){ if(queueEnded){ break; } waitCondition(myMut, myConLess); }
			waitQueue.push_back(toAdd);
			signalCondition(myMut, myConMore);
		unlockMutex(myMut);
	}
	
	/**
	 * Get a thing.
	 * @return The got thing: null if end called and nothing left.
	 */
	OfT* getThing(){
		lockMutex(myMut);
			while(waitQueue.size() == 0){ if(queueEnded){ break; } waitCondition(myMut, myConMore); }
			OfT* nxtThing = 0;
			if(waitQueue.size()){
				nxtThing = waitQueue[0];
				waitQueue.pop_front();
				signalCondition(myMut, myConLess);
			}
		unlockMutex(myMut);
		return nxtThing;
	}
	
	/**
	 * Get a thing, if any.
	 * @return The got thing.
	 */
	OfT* tryThing(){
		lockMutex(myMut);
			OfT* nxtThing = 0;
			if(waitQueue.size()){
				nxtThing = waitQueue[0];
				waitQueue.pop_front();
				signalCondition(myMut, myConLess);
			}
		unlockMutex(myMut);
		return nxtThing;
	}
	
	/**Wake up ALL waiters, let all know things are ending.*/
	void end(){
		if(queueEnded){ return; }
		lockMutex(myMut);
			queueEnded = true;
			broadcastCondition(myMut, myConMore);
			broadcastCondition(myMut, myConLess);
		unlockMutex(myMut);
	}
};

#endif
