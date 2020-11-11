#ifndef WHODUN_THREAD_H
#define WHODUN_THREAD_H 1

#include <deque>
#include <vector>
#include <stdint.h>

#include "whodun_cache.h"
#include "whodun_oshook.h"

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
	 */
	void addTask(void (*toDo)(void*), void* toPass);
	/**The number of threads in this pool.*/
	int numThr;
	/**The task mutex.*/
	void* taskMut;
	/**The task conditions.*/
	void* taskCond;
	/**The wating tasks.*/
	std::deque< std::pair<void(*)(void*), void*> > waitTask;
	/**Whether the pool is live.*/
	bool poolLive;
	/**The live threads.*/
	std::vector<void*> liveThread;
};

/**Wait for multiple.*/
class ThreadMultiWait{
public:
	/**Set up the base.*/
	ThreadMultiWait();
	/**Tear down.*/
	~ThreadMultiWait();
	/**
	 * Add to the amount to wait and wait until zero.
	 * @param numWait The number of things to wait on.
	 */
	void waitOn(unsigned numWait);
	/**Call when something being waited on finishes.*/
	void unwaitOne();
	/**The mutex for this thing.*/
	void* myMut;
	/**The condition to wait on for this thing.*/
	void* myCond;
	/**The number still in the air.*/
	int curWait;
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
