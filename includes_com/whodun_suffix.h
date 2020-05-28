#ifndef WHODUN_SUFFIX_H
#define WHODUN_SUFFIX_H 1

#include <stdint.h>
#include <stdlib.h>

/**The number of bytes used to store entries in the suffix array.*/
#define SUFFIX_ARRAY_CANON_SIZE 8

/**
 * Builds a suffix array on a string in memory.
 * @param onData The stuff to build a suffix array for. Null terminated.
 * @param The sorted order of the suffixes of onData. Indices of starting points. Size must be that of onData, including the null.
 */
void buildSuffixArray(const char* onData, uintptr_t* sortStore);

/**
 * This will search for the first entry in sortStore that has toFind as a prefix.
 * @param toFind The string to search for.
 * @param onData The original string in question.
 * @param sortStore The suffix array.
 * @param numInSort The number of elements in sortStore. Equal to strlen(onData)+1.
 * @return The index of the relevent entry in sort store, or numInSort if not found.
 */
uintptr_t suffixArrayLowerBound(const char* toFind, const char* onData, uintptr_t* sortStore, uintptr_t numInSort);

/**
 * This will search for the last entry in sortStore that has toFind as a prefix.
 * @param toFind The string to search for.
 * @param onData The original string in question.
 * @param sortStore The suffix array.
 * @param numInSort The number of elements in sortStore. Equal to strlen(onData)+1.
 * @return The index of the entry after the relavent entry in sort store, or numInSort if not found.
 */
uintptr_t suffixArrayUpperBound(const char* toFind, const char* onData, uintptr_t* sortStore, uintptr_t numInSort);

/**
 * This will build a multistring suffix array.
 * @param numStrings The number of strings to consider.
 * @param onData The strings to sort suffixes for.
 * @param strIStore THe place to store the string indices, [0,numStrings). Must be of length sum(strlen(onData[i])+1).
 * @param sortStore THe place to store the suffix starting points.
 */
void buildMultistringSuffixArray(size_t numStrings, const char** onData, uintptr_t* strIStore, uintptr_t* sortStore);

/**
 * This will search for the first entry in sortStore that has toFind as a prefix.
 * @param toFind The string to search for.
 * @param numStrings The number of strings to consider.
 * @param onData The original strings in question.
 * @param strIStore The string index map.
 * @param sortStore The suffix array.
 * @param numInSort The number of elements in sortStore. Equal to sum(strlen(onData)+1).
 * @return The index of the relevent entry in sort store, or numInSort if not found.
 */
uintptr_t multiSuffixArrayLowerBound(const char* toFind, uintptr_t numStrings, const char** onData, uintptr_t* strIStore, uintptr_t* sortStore, uintptr_t numInSort);

/**
 * This will search for the last entry in sortStore that has toFind as a prefix.
 * @param toFind The string to search for.
 * @param numStrings The number of strings to consider.
 * @param onData The original strings in question.
 * @param strIStore The string index map.
 * @param sortStore The suffix array.
 * @param numInSort The number of elements in sortStore. Equal to sum(strlen(onData)+1).
 * @return The index of the entry after the relavent entry in sort store, or numInSort if not found.
 */
uintptr_t multiSuffixArrayUpperBound(const char* toFind, uintptr_t numStrings, const char** onData, uintptr_t* strIStore, uintptr_t* sortStore, uintptr_t numInSort);

#endif