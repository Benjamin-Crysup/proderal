#ifndef WHODUN_BAMFA_H
#define WHODUN_BAMFA_H 1

#include <stdio.h>
#include <stdint.h>

#include <zlib.h>

/**An open fastq file.*/
typedef struct{
	/**The last read name.*/
	char* lastReadName;
	/**The last read sequence.*/
	char* lastReadSeq;
	/**The last read quality.*/
	char* lastReadQual;
	/**Whether the file is compressed.*/
	bool isComp;
	/**The currently open file (if raw).*/
	FILE* srcFile;
	/**The currently open file (if compressed).*/
	gzFile srcFileC;
	/**The amount of allocated space for lastReadName.*/
	uintptr_t nameAlloc;
	/**The amount of allocated space for lastReadSeq.*/
	uintptr_t seqAlloc;
	/**The amount of allocated space for lastReadQual.*/
	uintptr_t qualAlloc;
	/**Reading might need to read one ahead.*/
	char lastC;
} LiveFastqFileReader;

/**
 * This will open a fastq file for reading.
 * @param faqFileName The name of the file to open. If it ends in gz or gzip, it will decompress it.
 * @return The open file handle. Null if problem opening.
 */
LiveFastqFileReader* openFastqFile(const char* faqFileName);

/**
 * This will open a fastq file for reading.
 * @param faqFile The file to claim.
 * @return The open file handle.
 */
LiveFastqFileReader* wrapFastqFile(FILE* faqFile);

/**
 * This will read the next entry from the fast q file.
 * @param forFile The file to read for.
 * @return Whether there was another entry waiting.
 */
bool readNextFastqEntry(LiveFastqFileReader* forFile);

/**
 * This will close a fastq file.
 * @param toClose The file to close.
 */
void closeFastqFile(LiveFastqFileReader* toClose);

/**An open SAM file.*/
typedef struct{
	/**The last read header. If null, last read was not a header.*/
	char* lastReadHeader;
	/**The last read name. If null, last read was not an alignment.*/
	char* lastAlignQName;
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
	/**The last read sequence.*/
	char* lastAlignSeq;
	/**The last read quality.*/
	char* lastAlignQual;
	/**Any and all extra items.*/
	char* lastAlignExtra;
	/**Whether the file is compressed.*/
	bool isComp;
	/**The currently open file (if raw).*/
	FILE* srcFile;
	/**The currently open file (if compressed).*/
	gzFile srcFileC;
	/**Reading might need to read one ahead.*/
	char lastC;
	/**A common allocation for all strings.*/
	char* lastAlloc;
	/**The current size of said allocation.*/
	uintptr_t allocSize;
} LiveSAMFileReader;

/**
 * This will open a SAM file for reading.
 * @param faqFileName The name of the file to open. If it ends in gz or gzip, it will decompress it.
 * @return The open file handle. Null if problem opening.
 */
LiveSAMFileReader* openSAMFile(const char* faqFileName);

/**
 * This will open a SAM file for reading.
 * @param faqFile The name of the file to claim.
 * @return The open file handle..
 */
LiveSAMFileReader* wrapSAMFile(FILE* faqFile);

/**
 * This will read the next entry from the sam file.
 * @param forFile The file to read for.
 * @return Whether there was another entry waiting.
 */
bool readNextSAMEntry(LiveSAMFileReader* forFile);

/**
 * This will close a sam file.
 * @param toClose The file to close.
 */
void closeSAMFile(LiveSAMFileReader* toClose);

/**An open tsv file.*/
typedef struct{
	/**The last read line.*/
	char* lastLine;
	/**The number of entries.*/
	int numColumns;
	/**The starting points of each entry.*/
	char** colStarts;
	/**The ending points of each entry.*/
	char** colEnds;
	/**Whether the file is compressed.*/
	bool isComp;
	/**The currently open file (if raw).*/
	FILE* srcFile;
	/**The currently open file (if compressed).*/
	gzFile srcFileC;
	/**The number of bytes allocated for the line.*/
	int lineAlloc;
	/**The number of entries allocated for the columns.*/
	int colAlloc;
} LiveTSVFileReader;

/**
 * This will open a TSV file for reading.
 * @param faqFileName The name of the file to open. If it ends in gz or gzip, it will decompress it.
 * @return The open file handle. Null if problem opening.
 */
LiveTSVFileReader* openTSVFile(const char* faqFileName);

/**
 * This will open a TSV file for reading.
 * @param faqFile The name of the file to claim.
 * @return The open file handle..
 */
LiveTSVFileReader* wrapTSVFile(FILE* faqFile);

/**
 * This will read the next entry from the tsv file.
 * @param forFile The file to read for.
 * @return Whether there was another entry waiting.
 */
bool readNextTSVEntry(LiveTSVFileReader* forFile);

/**
 * This will close a tsv file.
 * @param toClose The file to close.
 */
void closeTSVFile(LiveTSVFileReader* toClose);

/**The contents of a bed file.*/
typedef struct{
	/**The number of entries.*/
	int numEnts;
	/**The chromosome locations for each entry.*/
	char** chroms;
	/**The start locations for each entry, zero based, inclusive.*/
	uintptr_t* starts;
	/**The end locations for each entry (exclusive).*/
	uintptr_t* ends;
	/**Any tailing stuff after the entry.*/
	char** extras;
} BedFileContents;

/**
 * Reads a bed file from a tsv.
 * @param toWork The tsv file to work over.
 * @return The read bed file.
 */
BedFileContents* readBedFile(LiveTSVFileReader* toWork);

/**
 * Free a bed file.
 * @param toKill The file contents to 
 */
void freeBedFile(BedFileContents* toKill);

#endif