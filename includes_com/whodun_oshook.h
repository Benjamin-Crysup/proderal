#ifndef WHODUN_OSHOOK_H
#define WHODUN_OSHOOK_H 1

#include <stdio.h>
#include <stdint.h>

/**The seperator for path elements.*/
extern const char* pathElementSep;

/**
 * Gets whether a file exists.
 * @param fileName THe name of the file to test.
 * @return Whether it exists.
 */
bool fileExists(const char* fileName);

/**
 * Deletes a file, if it exists.
 * @param fileName THe name of the file to test.
 */
void killFile(const char* fileName);

/**
 * Gets the size of a file.
 * @param fileName THe name of the file to test.
 * @return The size of said file.
 */
intptr_t getFileSize(const char* fileName);

/**
 * A version of fseek that uses pointer integers for offsets.
 * @param stream The file to seek.
 * @param offset The number of bytes in the file.
 * @param whence The relative location.
 * @return Whether there was a problem.
 */
int fseekPointer(FILE* stream, intptr_t offset, int whence);

/**
 * A version of ftell that uses pointer integers.
 * @param stream The file to get the position in.
 * @return The current byte offset.
 */
intptr_t ftellPointer(FILE* stream);

/**
 * This will start a thread.
 * @param callFun The thread function.
 * @param callUniform The thing to pass to said function.
 * @return A handle to the thread.
 */
void* startThread(void(*callFun)(void*), void* callUniform);

/**
 * Joins on a thread and drops it.
 * @param tHandle THe thread to join on.
 */
void joinThread(void* tHandle);

/**
 * Make a mutex for future use.
 * @return The created mutex.
 */
void* makeMutex();

/**
 * Get a lock.
 * @param toLock The lock to get.
 */
void lockMutex(void* toLock);

/**
 * Release a lock.
 * @param toUnlock The lock to get.
 */
void unlockMutex(void* toUnlock);

/**
 * Delete a mutex.
 * @param toKill The lock to delete.
 */
void killMutex(void* toKill);

/**
 * Make a condition variable.
 * @param forMutex The lock to make it for.
 * @return The cretated condition variable.
 */
void* makeCondition(void* forMutex);

/**
 * Wait on a condition.
 * @param forMutex The relevant lock.
 * @param forCondition The condition in question.
 */
void waitCondition(void* forMutex, void* forCondition);

/**
 * Start one waiting on condition.
 * @param forMutex The relevant lock.
 * @param forCondition The condition in question.
 */
void signalCondition(void* forMutex, void* forCondition);

/**
 * Start all waiting on condition.
 * @param forMutex The relevant lock.
 * @param forCondition The condition in question.
 */
void broadcastCondition(void* forMutex, void* forCondition);

/**
 * Delete a condition variable.
 * @param forCondition The condition to kill.
 */
void killCondition(void* forCondition);

/**
 * Load a dll.
 * @param loadFrom The name of the dll.
 * @return A handle to it.
 */
void* loadInDLL(const char* loadFrom);

/**
 * Get the location of a loaded item in a dll.
 * @param fromDLL The dll to get from.
 * @param locName The name.
 */
void* getDLLLocation(void* fromDLL, const char* locName);

/**
 * Unload a dll.
 * @param toKill The dll to load.
 */
void unloadDLL(void* toKill);

#endif
