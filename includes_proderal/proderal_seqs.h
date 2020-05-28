#ifndef PRODERAL_SEQS_H
#define PRODERAL_SEQS_H 1

#include <iostream>
#include <stdint.h>

/**Cigar information.*/
typedef struct{
	/**The low index.*/
	intptr_t lowInd;
	/**The high index.*/
	intptr_t higInd;
	/**The number of bases skipped on the low side.*/
	intptr_t preSkip;
	/**The number of bases skipped on the high side.*/
	intptr_t postSkip;
} CigarBounds;

/**
 * This will figure out the bounds of a cigar.
 * @param seqLen The length of the sequence in question.
 * @param cigarStr The cigar string.
 * @param fromLoc The base the cigar is relative to.
 * @param toFill The place to put the results.
 * @param errDump The place to write the result.
 * @return Whether there was an error.
 */
int findCigarBounds(int seqLen, const char* cigarStr, intptr_t fromLoc, CigarBounds* toFill, std::ostream* errDump);

/**
 * Gets the reverse complement of a string.
 * @param startC The first character in the sequence.
 * @param endC THe end pointer in the sequence.
 * @return An allocated reverse complement string. Zero terminated.
 */
char* reverseComplement(const char* startC, const char* endC);

#endif