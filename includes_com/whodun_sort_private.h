#ifndef WHODUN_SORT_PRIVATE_H
#define WHODUN_SORT_PRIVATE_H 1

/**
 * Generates file names for temporary files.
 * @param forFolder The folder to put the files in.
 * @param numFiles The number of temporary files to make.
 * @return An array of files names. Only free the top.
 */
char** generateTemporaryFileNames(const char* forFolder, int numFiles);

/**
 * This will merge two chunks from two files.
 * @param fromA The first file.
 * @param fromB The second file.
 * @param toEnd THe file to merge into.
 * @param inMemSort Allocated memory, at least two items long.
 * @param inMemSize The number of items inMemSort can hold.
 * @param numMerge The number of items per chunk.
 * @param hitEndA Whether the end of fileA has been hit.
 * @param hitEndB Whether the end of fileB has been hit.
 * @param lastNumWritten If any items are read, this will be incremented by one (each call to this function will increment by at most one).
 * @param opts The options for the sort.
 * @return Whether there was an exception.
 */
int helpMergeTwo(void* fromA, void* fromB, void* toEnd, char* inMemSort, uintptr_t inMemSize, uintptr_t numMerge, SortOptions* opts, bool* hitEndA, bool* hitEndB, uintptr_t* lastNumWritten);

#endif