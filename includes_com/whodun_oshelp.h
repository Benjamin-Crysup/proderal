#ifndef WHODUN_OSHELP_H
#define WHODUN_OSHELP_H 1

#include "whodun_oshook.h"

/**A managed mutex.*/
class OSMutex{
public:
	/**Set up the mutex.*/
	OSMutex();
	/**Destroy the muted.*/
	~OSMutex();
	/**Lock the mutex.*/
	void lock();
	/**Unlock the mutex.*/
	void unlock();
	/**The mutex.*/
	void* myMut;
};

/**A managed condition.*/
class OSCondition{
public:
	/**Make a condition around a mutex.*/
	OSCondition(OSMutex* baseMut);
	/**Destroy.*/
	~OSCondition();
	/**Wait on the condition.*/
	void wait();
	/**Signal one from the condition.*/
	void signal();
	/**Signal all from the condition.*/
	void broadcast();
	/**The mutex.*/
	void* saveMut;
	/**The condition.*/
	void* myCond;
};

/**A managed dll.*/
class OSDLLSO{
public:
	/**
	 * Load the dll.
	 * @param dllName The name of the dll to load.
	 */
	OSDLLSO(const char* dllName);
	/**Unload.*/
	~OSDLLSO();
	/**
	 * Get a location from this dll.
	 * @param locName The name of the thing to get.
	 * @return The location, or null if not found.
	 */
	void* get(const char* locName);
	/**The loaded dll.*/
	void* myDLL;
};

#endif