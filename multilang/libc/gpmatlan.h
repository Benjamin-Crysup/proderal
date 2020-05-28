#ifndef GPMATLAN_H
#define GPMATLAN_H 1

#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
typedef bool _Bool;
extern "C" {
#endif

/**An array of booleans.*/
typedef struct{
	/**The length.*/
	int arrLen;
	/**The amount of space allocated*/
	int curAlloc;
	/**The contents.*/
	_Bool* arrConts;
} GPML_BoolArrayBase;

/**An array of bytes.*/
typedef struct{
	/**The length.*/
	int arrLen;
	/**The amount of space allocated*/
	int curAlloc;
	/**The contents.*/
	char* arrConts;
} GPML_ByteArrayBase;

/**An array of integers.*/
typedef struct{
	/**The length.*/
	int arrLen;
	/**The amount of space allocated*/
	int curAlloc;
	/**The contents.*/
	int* arrConts;
} GPML_IntArrayBase;

/**An array of floats.*/
typedef struct{
	/**The length.*/
	int arrLen;
	/**The amount of space allocated*/
	int curAlloc;
	/**The contents.*/
	double* arrConts;
} GPML_FloatArrayBase;

/**An nd array of booleans.*/
typedef struct{
	/**The dimensionality*/
	int dim;
	/**The length.*/
	int* arrLen;
	/**The contents.*/
	_Bool* arrConts;
} GPML_BoolArrayNDBase;

/**An nd array of bytes.*/
typedef struct{
	/**The dimensionality*/
	int dim;
	/**The length.*/
	int* arrLen;
	/**The contents.*/
	char* arrConts;
} GPML_ByteArrayNDBase;

/**An nd array of ints.*/
typedef struct{
	/**The dimensionality*/
	int dim;
	/**The length.*/
	int* arrLen;
	/**The contents.*/
	int* arrConts;
} GPML_IntArrayNDBase;

/**An nd array of floats.*/
typedef struct{
	/**The dimensionality*/
	int dim;
	/**The length.*/
	int* arrLen;
	/**The contents.*/
	double* arrConts;
} GPML_FloatArrayNDBase;

/**Helpful typedef for m4.*/
typedef _Bool GPML_Boolean;
/**Helpful typedef for m4.*/
typedef char GPML_Byte;
/**Helpful typedef for m4.*/
typedef int GPML_Integer;
/**Helpful typedef for m4.*/
typedef double GPML_Float;
/**Helpful typedef for m4.*/
typedef GPML_BoolArrayBase* GPML_BoolArray;
/**Helpful typedef for m4.*/
typedef GPML_ByteArrayBase* GPML_ByteArray;
/**Helpful typedef for m4.*/
typedef GPML_IntArrayBase* GPML_IntArray;
/**Helpful typedef for m4.*/
typedef GPML_FloatArrayBase* GPML_FloatArray;
/**Helpful typedef for m4.*/
typedef GPML_BoolArrayNDBase* GPML_BoolArrayND;
/**Helpful typedef for m4.*/
typedef GPML_ByteArrayNDBase* GPML_ByteArrayND;
/**Helpful typedef for m4.*/
typedef GPML_IntArrayNDBase* GPML_IntArrayND;
/**Helpful typedef for m4.*/
typedef GPML_FloatArrayNDBase* GPML_FloatArrayND;

/**
 * Makes a new byte array.
 * @param arrSize The size of the array.
 * @return The array, or null if problem.
 */
GPML_ByteArray allocateByteArray(int arrSize);

/**
 * Adds to a byte array.
 * @param toAddTo The thing to add to.
 * @param toAdd The thing to put at the end.
 * @return Whether there was a problem.
 */
_Bool addToByteArray(GPML_ByteArray toAddTo, char toAdd);

/**
 * Makes a new int array.
 * @param arrSize The size of the array.
 * @return The array, or null if problem.
 */
GPML_IntArray allocateIntArray(int arrSize);

/**
 * Makes a new float array.
 * @param arrSize The size of the array.
 * @return The array, or null if problem.
 */
GPML_FloatArray allocateFloatArray(int arrSize);

/**
 * Adds to an int array.
 * @param toAddTo The thing to add to.
 * @param toAdd The thing to put at the end.
 * @return Whether there was a problem.
 */
_Bool addToIntArray(GPML_IntArray toAddTo, int toAdd);

/**
 * Makes a new float array.
 * @param dim The number of dimensions.
 * @param dimLens The lengths in each dimension
 * @return The array, or null if problem.
 */
GPML_FloatArrayND allocateFloatArrayND(int dim, int* dimLens);

/**
 * Get a value from an nd array.
 * @param getFrom The thing to get from.
 * @param dimInds The indices of the thing to get.
 * @return The got thing.
 */
double getFromFloatArrayND(GPML_FloatArrayND getFrom, int* dimInds);

/**
 * This sets a value in an nd array.
 * @param getFrom The thing to set in.
 * @param dimInds The indices of the thing to set.
 * @param newVal The value to set to.
 */
void setInFloatArrayND(GPML_FloatArrayND getFrom, int* dimInds, double newVal);

/**An array of pointers.*/
typedef struct{
	/**The current length.*/
	int curLen;
	/**The allocated length of the array.*/
	int maxLen;
	/**The array entries.*/
	void** allEnts;
} PointerArray;

/**
 * Add a pointer to a set.
 * @param toAddTo The array to add to.
 * @param toAdd The pointer to add.
 * @return Whether there was a problem adding: if so, the array is left as is.
 */
_Bool addToPointerArray(PointerArray* toAddTo, void* toAdd);

/**
 * Ensures the capacity of an array.
 * @param toAddTo The array to add to.
 * @param byNum The number of pointers to add.
 * @return If there is a problem preparing the space.
 */
_Bool ensurePointerArrayCapacity(PointerArray* toAddTo, int byNum);

/**
 * Frees all pointers.
 * @param numKill The number of things to kill.
 * @param toKill The things to kill.
 */
void freeAllPointers(int numKill, void** toKill);

/**
 * Frees all pointers, baring a few.
 * @param numKill The number of things to kill.
 * @param toKill The things to kill.
 * @param numSave The number of things not to kill.
 * @param toSave The things to spare.
 */
void freeAllPointersExcept(int numKill, void** toKill, int numSave, void** toSave);

/**
 * Removes a pointer from an array.
 * @param numKill The number of things in the array.
 * @param toKill The array of things to kill.
 * @param toSpare The thing to remove.
 */
void removePointerFromKillArray(int* numKill, void** toKill, void* toSpare);

#ifdef __cplusplus
}
#endif

#endif
