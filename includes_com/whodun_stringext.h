#ifndef WHODUN_STRINGEXT_H
#define WHODUN_STRINGEXT_H 1

#include <string.h>
#include <stdint.h>

#include "whodun_thread.h"

/**
 * A multithreaded memcpy.
 * @param cpyTo The destination.
 * @param cpyFrom The source.
 * @param numBts The number of bytes.
 * @param numThread The number of threads to use.
 * @param mainPool The thread pool to use.
 * @return cpyTo
 */
void* memcpymt(void* cpyTo, const void* cpyFrom, size_t numBts, unsigned numThread, ThreadPool* mainPool);

/**
 * A multithreaded memset.
 * @param setP The place to set.
 * @param value The value to set.
 * @param numBts The number of bytes to set.
 * @param numThread The number of threads to use.
 * @param mainPool The thread pool to use.
 * @return setP
 */
void* memsetmt(void* setP, int value, size_t numBts, unsigned numThread, ThreadPool* mainPool);

//TODO multithreaded memcmp memcspn memspn memchr memmem memswap

/**
 * Determine if one string ends with another.
 * @param str1 The large string.
 * @param str2 The suspected ending.
 * @return Whether str1 ends with str2.
 */
int strendswith(const char* str1, const char* str2);

//memcpy good
//memcmp good
//memchr good

/**
 * Return the number of characters in str1 until one of the characters in str2 is encountered (or the end is reached).
 * @param str1 The string to walk along.
 * @param numB1 The length of said string.
 * @param str2 The characters to search for.
 * @param numB2 The number of characters to search for.
 * @return The number of characters until a break (one of the characters in str2 or end of string).
 */
size_t memcspn(const char* str1, size_t numB1, const char* str2, size_t numB2);

/**
 * Return the number of characters in str1 until something not in str2 is encountered.
 * @param str1 The string to walk along.
 * @param numB1 The length of said string.
 * @param str2 The characters to search for.
 * @param numB2 The number of characters to search for.
 * @return The number of characters until a break (something not in str2 or end of string).
 */
size_t memspn(const char* str1, size_t numB1, const char* str2, size_t numB2);

/**
 * Returns a pointer to the first occurence of str2 in str1.
 * @param str1 The string to walk along.
 * @param numB1 The length of said string.
 * @param str2 The string to search for.
 * @param numB2 The length of said string.
 * @return The location of str2 in str1, or null if not present.
 */
char* memmem(const char* str1, size_t numB1, const char* str2, size_t numB2);

/**
 * This will swap, byte by byte, the contents of two (non-overlapping) arrays.
 * @param arrA THe first array.
 * @param arrB The second array.
 * @param numBts The number of bytes to swap.
 */
void memswap(char* arrA, char* arrB, size_t numBts);

/**
 * This will figure the number of bytes until the first difference between the two chunks of memory.
 * @param str1 The first chunk.
 * @param numB1 The length of the first chunk.
 * @param str2 The second chunk.
 * @param numB2 The length of the second chunk.
 * @return The number of bytes to the first difference. End of string is always different.
 */
size_t memdiff(const char* str1, size_t numB1, const char* str2, size_t numB2);

/**
 * This will pack a 64 bit integer into memory in big-endian format.
 * @param toPrep The integer to prepare.
 * @param toBuffer The memory to put it in.
 */
void nat2be64(uint_least64_t toPrep, char* toBuffer);

/**
 * This will take 8 bytes of a big-endian memory storage and produce an integer.
 * @param toDebuffer The memory to read from.
 * @return The integer.
 */
uint_least64_t be2nat64(const char* toDebuffer);

/**
 * This will pack a 64 bit integer into memory in big-endian format.
 * @param toPrep The integer to prepare.
 * @param toBuffer The memory to put it in.
 */
void nat2le64(uint_least64_t toPrep, char* toBuffer);

/**
 * This will take 8 bytes of a big-endian memory storage and produce an integer.
 * @param toDebuffer The memory to read from.
 * @return The integer.
 */
uint_least64_t le2nat64(const char* toDebuffer);

/**
 * This will pack a 32 bit integer into memory in big-endian format.
 * @param toPrep The integer to prepare.
 * @param toBuffer The memory to put it in.
 */
void nat2be32(uint_least32_t toPrep, char* toBuffer);

/**
 * This will take 4 bytes of a big-endian memory storage and produce an integer.
 * @param toDebuffer The memory to read from.
 * @return The integer.
 */
uint_least32_t be2nat32(const char* toDebuffer);

/**
 * This will pack a 32 bit integer into memory in big-endian format.
 * @param toPrep The integer to prepare.
 * @param toBuffer The memory to put it in.
 */
void nat2le32(uint_least32_t toPrep, char* toBuffer);

/**
 * This will take 4 bytes of a big-endian memory storage and produce an integer.
 * @param toDebuffer The memory to read from.
 * @return The integer.
 */
uint_least32_t le2nat32(const char* toDebuffer);

/**
 * This will pack a 16 bit integer into memory in big-endian format.
 * @param toPrep The integer to prepare.
 * @param toBuffer The memory to put it in.
 */
void nat2be16(uint_least16_t toPrep, char* toBuffer);

/**
 * This will take 2 bytes of a big-endian memory storage and produce an integer.
 * @param toDebuffer The memory to read from.
 * @return The integer.
 */
uint_least16_t be2nat16(const char* toDebuffer);

/**
 * This will pack a 16 bit integer into memory in big-endian format.
 * @param toPrep The integer to prepare.
 * @param toBuffer The memory to put it in.
 */
void nat2le16(uint_least16_t toPrep, char* toBuffer);

/**
 * This will take 2 bytes of a big-endian memory storage and produce an integer.
 * @param toDebuffer The memory to read from.
 * @return The integer.
 */
uint_least16_t le2nat16(const char* toDebuffer);

/**
 * Get an IEEE single precision floating point number as a 32 bit integer: assumes float is IEEE single.
 * @param toConv The float to convert.
 * @return The bits.
 */
uint_least32_t sfltbits(float toConv);

/**
 * Get an IEEE single precision floating point number from a 32 bit integer: assumes float is IEEE single.
 * @param toConv The float to convert.
 * @return The bits.
 */
float sbitsflt(uint_least32_t toConv);

/**
 * Get an IEEE double precision floating point number as a 64 bit integer: assumes double is IEEE double.
 * @param toConv The float to convert.
 * @return The bits.
 */
uint_least64_t sdblbits(double toConv);

/**
 * Get an IEEE double precision floating point number from a 64 bit integer: assumes double is IEEE double.
 * @param toConv The float to convert.
 * @return The bits.
 */
double sbitsdbl(uint_least64_t toConv);

#endif