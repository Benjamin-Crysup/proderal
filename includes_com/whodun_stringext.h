#ifndef WHODUN_STRINGEXT_H
#define WHODUN_STRINGEXT_H 1

#include <string.h>
#include <stdint.h>

/**
 * This will swap, byte by byte, the contents of two arrays.
 * @param arrA THe first array.
 * @param arrB The second array.
 * @param numBts The number of bytes to swap.
 */
void memswap(char* arrA, char* arrB, size_t numBts);

/**
 * This will pack a 64 bit integer into memory in big-endian format.
 * @param toPrep The integer to prepare.
 * @param toBuffer The memory to put it in.
 */
void canonPrepareInt64(uint_least64_t toPrep, char* toBuffer);

/**
 * This will take 8 bytes of a big-endian memory storage and produce an integer.
 * @param toDebuffer The memory to read from.
 * @return The integer.
 */
uint_least64_t canonParseInt64(const char* toDebuffer);

#endif