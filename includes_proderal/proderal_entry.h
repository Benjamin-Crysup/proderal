#ifndef PRODERAL_ENTRY_H
#define PRODERAL_ENTRY_H 1

#include <string>
#include <vector>
#include <iostream>
#include <stdint.h>

#include "whodun_bamfa.h"

/**Used to specify an entry to add.*/
typedef struct{
	/**The last flag code.*/
	int lastAlignFlag;
	/**The reference name of the last read.*/
	char* lastAlignRName;
	/**The starting poihnt of the alignment (ZERO base).*/
	intptr_t lastAlignPos;
	/**The mapping quality of this alignment.*/
	unsigned char lastAlignMapq;
	/**The cigar string of the last read.*/
	char* lastAlignCIGAR;
	/**The last align rnext.*/
	char* lastAlignRNext;
	/**The last align pnext.*/
	intptr_t lastAlignPNext;
	/**The last align tlen.*/
	intptr_t lastAlignTLen;
	/**Any and all extra items.*/
	char* lastAlignExtra;
} WaitPairEntry;

/**
 * Copy a sam entry to a wait pair.
 * @param toNote The sam entry.
 * @param toFill The thing to fill.
 */
void fillEntryFromSAM(LiveSAMFileReader* toNote, WaitPairEntry* toFill);

/**A set of entries waiting for their pair.*/
class WaitingForPairSet{
public:
	/**
	 * Sets up a single entry based on a SAM entry.
	 * @param toNote The SAM file entry.
	 * @param errDump The place to write in case of error.
	 */
	WaitingForPairSet(LiveSAMFileReader* toNote, std::ostream* errDump);
	/**
	 * This sets up a set of zero entries.
	 * @param queryName The name of the template.
	 * @param sequence The sequence of the template.
	 * @param qualString The quality of the template.
	 */
	WaitingForPairSet(const char* queryName, const char* sequence, const char* qualString);
	/**
	 * Adds an entry to this set.
	 * @param infoGet The entry data.
	 * @param errDump The place to write in case of error.
	 */
	void addEntry(WaitPairEntry* infoGet, std::ostream* errDump);
	/**
	 * No pair found, write out as is.
	 * @param toDump The stream to write to.
	 */
	void writeEntryWithoutPair(std::ostream* toDump);
	/**
	 * Note pair info and write out.
	 * @param toDump The stream to write to.
	 * @param pairInfo The paired entry: first is canonical.
	 */
	void writeEntryWithPair(std::ostream* toDump, WaitingForPairSet* pairInfo);
	
	/**The name of this entry.*/
	std::string myName;
	/**The flags for each entry*/
	std::vector<int> knownFlags;
	/**The references each item went to.*/
	std::vector<std::string> mapRefs;
	/**The position and mapping quality of each item.*/
	std::vector<std::string> mapPosMapCigs;
	/**The lines to use if no pair is found.*/
	std::vector<std::string> defDumps;
	/**The sequence.*/
	std::string sequen;
	/**The quality*/
	std::string qualstr;
	/**The extra stuff for each entry.*/
	std::vector<std::string> extras;
	/**The low occupied indices for each entry.*/
	std::vector<intptr_t> lowInds;
	/**The high occupied indices for each entry.*/
	std::vector<intptr_t> higInds;
};

/**
 * This will dump an unpaired entry as is.
 * @param unpEnt The entry in question.
 * @param toDump THe place to write.
 */
void dumpUnpairedAsIs(LiveSAMFileReader* unpEnt, std::ostream* toDump);

#endif