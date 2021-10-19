#ifndef WHODUN_PARSE_SEQ_H
#define WHODUN_PARSE_SEQ_H 1

#include <vector>
#include <iostream>

#include "whodun_datread.h"
#include "whodun_compress.h"

/**
 * Turn an ascii phred score to a log (10) probability.
 * @param toConv The phred score to convert.
 * @return The log10 probability.
 */
double fastaPhredToLog10Prob(unsigned char toConv);

/**
 * Turn a log (10) probability to an ascii phred score.
 * @param toConv The log10 probability.
 * @return The phred score.
 */
unsigned char fastaLog10ProbToPhred(double toConv);

/**
 * Convert multiple.
 * @param numConv The number to convert.
 * @param toConv The phred scores to convert.
 * @param toStore The place to put the log10 probabilities.
 */
void fastaPhredsToLog10Prob(uintptr_t numConv, const unsigned char* toConv, double* toStore);

/**
 * Convert multiple.
 * @param numConv The number to convert.
 * @param toConv The log10 probabilities to convert.
 * @param toStore The place to put the phred scores.
 */
void fastaLog10ProbsToPhred(uintptr_t numConv, const double* toConv, unsigned char* toStore);

/**
 * This will reverse compliment a sequence.
 * @param revLen The length of the sequence.
 * @param toRev The sequence to reverse.
 * @param toRevQ The qualities, if present (null if not).
 */
void sequenceReverseCompliment(uintptr_t revLen, char* toRev, double* toRevQ);

/**Read a sequence.*/
class SequenceReader{
public:
	/**The length of the short name, if any.*/
	uintptr_t lastReadShortNameLen;
	/**The length of the last read name.*/
	uintptr_t lastReadNameLen;
	/**The last read name.*/
	const char* lastReadName;
	/**The lenght of the last read sequence.*/
	uintptr_t lastReadSeqLen;
	/**The last read sequence.*/
	const char* lastReadSeq;
	/**Whether the last read had a quality.*/
	uintptr_t lastReadHaveQual;
	/**The last read qualities.*/
	const double* lastReadQual;
	/**The number of read records.*/
	uintptr_t recordCount;
	/**Clear the above.*/
	SequenceReader();
	/**Allow subclassing.*/
	virtual ~SequenceReader();
	/**
	 * The next entry to read.
	 * @return Whether there was an entry (1).
	 */
	virtual int readNextEntry() = 0;
};

#define FASTAQREAD_BUFFER_SIZE 4096

/**Read sequences from a fastA / fastQ file.*/
class FastAQSequenceReader : public SequenceReader{
public:
	/**
	 * Set up a parser for the thing.
	 * @param mainFrom The thing to parse.
	 */
	FastAQSequenceReader(InStream* mainFrom);
	/**Tear down.*/
	virtual ~FastAQSequenceReader();
	int readNextEntry();
	/**The thing to parse.*/
	InStream* theStr;
	/**The allocation for the name.*/
	std::string nameStore;
	/**The allocation for the sequence.*/
	std::string seqStore;
	/**The allocation for the qualities.*/
	std::vector<double> qualStore;
	/**Storage for the quality characters.*/
	std::vector<unsigned char> tmpQualS;
	/**Storage for reads*/
	char readBuff[FASTAQREAD_BUFFER_SIZE];
	/**Offset into the read buffer.*/
	int readBuffO;
	/**Number of bytes in the read buffer.*/
	int readBuffS;
	/**Make sure the buffer has stuff in it.*/
	void fillBuffer();
};
//TODO quality as double
/**Read sequences from a gail file.*/
class GailAQSequenceReader : public SequenceReader{
public:
	/**
	 * Make a sequence reader.
	 * @param toFlit The thing to read from.
	 * @param indFName The name of the index file.
	 */
	GailAQSequenceReader(BlockCompInStream* toFlit, const char* indFName);
	/**Clean up.*/
	~GailAQSequenceReader();
	
	int readNextEntry();
	
	//random access stuff
	/**
	 * Get the number of entries in the file.
	 * @return The number of entries.
	 */
	uintptr_t getNumEntries();
	/**
	 * Get the length of an entry.
	 * @param entInd The index of the entry to get.
	 * @return The number of bases in the entry.
	 */
	uintptr_t getEntryLength(uintptr_t entInd);
	/**
	 * Load a part of an entry.
	 * @param entInd The index of the entry to get.
	 * @param fromBase The first base index to get.
	 * @param toBase The last base index to get.
	 */
	void getEntrySubsequence(uintptr_t entInd, uintptr_t fromBase, uintptr_t toBase);
	
	/**If a random access was called, use this to reset the stream.*/
	intptr_t resetInd;
	/**The next index to report.*/
	uintptr_t focusInd;
	/**The number of entries in the file.*/
	uintptr_t numEntries;
	/**The actual thing to read.*/
	BlockCompInStream* theStr;
	/**The index file.*/
	FILE* indF;
	/**The allocation for the name.*/
	std::vector<char> nameStore;
	/**The allocation for the sequence.*/
	std::vector<char> seqStore;
	/**Storage for the quality characters.*/
	std::vector<unsigned char> tmpQualS;
	/**The allocation for the qualities.*/
	std::vector<double> qualStore;
};

/**Read sequences sequentially from a gail file.*/
class SequentialGailAQSequenceReader : public SequenceReader{
public:
	/**
	 * Make a sequence reader.
	 * @param toFlit The thing to read from.
	 * @param indFName The name of the index file.
	 * @param loadSize The amount of data to load at once.
	 * @param useTCount The number of threads to use (for converting qualities).
	 * @param useThreads The threads to use.
	 */
	SequentialGailAQSequenceReader(InStream* toFlit, const char* indFName, uintptr_t loadSize, uintptr_t useTCount, ThreadPool* useThreads);
	/**Clean up.*/
	~SequentialGailAQSequenceReader();
	
	int readNextEntry();
	
	/**The actual thing to read.*/
	InStream* theStr;
	/**The index file.*/
	FILE* indF;
	/**The amount of data to load at once.*/
	uintptr_t bufferSize;
	/**The number of entries in the file.*/
	uintptr_t numEntries;
	/**The number of entries read through (the index of entry zero in the cache).*/
	uintptr_t handEntries;
	/**The next entry in the cache to report.*/
	uintptr_t nextReport;
	/**The header data for the stuff in the cache.*/
	std::vector<uintptr_t> cacheEntries;
	/**The allocated size of the cache.*/
	uintptr_t cacheSize;
	/**The cache.*/
	char* cache;
};

/**Write sequence data.*/
class SequenceWriter{
public:
	/**The length of the last read name.*/
	uintptr_t nextNameLen;
	/**The length of the short name.*/
	uintptr_t nextShortNameLen;
	/**The last read name.*/
	const char* nextName;
	/**The lenght of the last read sequence.*/
	uintptr_t nextSeqLen;
	/**The last read sequence.*/
	const char* nextSeq;
	/**Whether the last read had a quality.*/
	uintptr_t nextHaveQual;
	/**The last read qualities.*/
	const double* nextQual;
	/**Clear the above.*/
	SequenceWriter();
	/**Allow subclassing.*/
	virtual ~SequenceWriter();
	/**
	 * Take the above data and write it.
	 */
	virtual void writeNextEntry() = 0;
};

/**Write sequences to a fastA / fastQ file.*/
class FastAQSequenceWriter : public SequenceWriter{
public:
	/**
	 * Set up a parser for the thing.
	 * @param mainTo The thing to write to.
	 */
	FastAQSequenceWriter(OutStream* mainTo);
	/**Tear down.*/
	virtual ~FastAQSequenceWriter();
	void writeNextEntry();
	/**The thing to parse.*/
	OutStream* theStr;
	/**Storage for the quality characters.*/
	std::vector<unsigned char> tmpQualS;
};

/**Write sequences to a gail file.*/
class GailAQSequenceWriter : public SequenceWriter{
public:
	/**
	 * Set up a parser for the thing.
	 * @param append Whether stuff is being appended.
	 * @param toFlit The thing to write to.
	 * @param indFName The name of the index file.
	 */
	GailAQSequenceWriter(int append, BlockCompOutStream* toFlit, const char* indFName);
	/**
	 * Make a sequence writer.
	 * @param append Whether stuff is being appended.
	 * @param toFlit The thing to write to.
	 * @param indFName The name of the index file.
	 */
	GailAQSequenceWriter(MultithreadBlockCompOutStream* toFlit, const char* indFName);
	/**Tear down.*/
	virtual ~GailAQSequenceWriter();
	void writeNextEntry();
	/**The thing to parse.*/
	BlockCompOutStream* theStr;
	/**Aleternate place to write*/
	MultithreadBlockCompOutStream* theStrMT;
	/**The index file.*/
	FILE* indF;
	/**Storage for the quality characters.*/
	std::vector<unsigned char> tmpQualS;
};

/**
 * Open a named sam/bam/cram file for reading.
 * @param fileName The name of the file to open: "-" for stdin.
 * @param saveIS The base input stream, if any.
 * @param saveSS The cbsam stream.
 */
void openSequenceFileRead(const char* fileName, InStream** saveIS, SequenceReader** saveSS);

/**
 * Open a named sam/bam/cram file for writing.
 * @param fileName The name of the file to open: "-" for stdout.
 * @param saveIS The base output stream, if any.
 * @param saveSS The cbsam stream.
 */
void openSequenceFileWrite(const char* fileName, OutStream** saveIS, SequenceWriter** saveSS);

#endif