#ifndef WHODUN_PARSE_TABLE_H
#define WHODUN_PARSE_TABLE_H 1

#include <vector>
#include <string>
#include <stdint.h>

#include "whodun_datread.h"
#include "whodun_compress.h"

/**Read a sequence of delimitted data.*/
class TabularReader{
public:
	/**The number of entries (columns) in the current entry.*/
	uintptr_t numEntries;
	/**The length of each entry.*/
	uintptr_t* entrySizes;
	/**The text of each entry.*/
	const char** curEntries;
	/**The number of read records.*/
	uintptr_t recordCount;
	/**Clear the above.*/
	TabularReader();
	/**Allow subclassing.*/
	virtual ~TabularReader();
	/**
	 * The next entry to read.
	 * @return Whether there was an entry (1).
	 */
	virtual int readNextEntry() = 0;
};

#define TSVTABLEREAD_BUFFER_SIZE 4096

/**Read a tsv file.*/
class TSVTabularReader : public TabularReader{
public:
	/**
	 * Set up a parser for the thing.
	 * @param escapes Whether escapes are enabled.
	 * @param mainFrom The thing to parse.
	 */
	TSVTabularReader(int escapes, InStream* mainFrom);
	/**Tear down.*/
	virtual ~TSVTabularReader();
	int readNextEntry();
	/**Whether escapes are enabled.*/
	int escEnable;
	/**The thing to parse.*/
	InStream* theStr;
	/**The lengths of each entry.*/
	std::vector<uintptr_t> entryLens;
	/**The full text.*/
	std::string entryFlat;
	/**The pointers to the head of each entry.*/
	std::vector<const char*> entryHeads;
	/**Head indices.*/
	std::vector<uintptr_t> headTmp;
	/**Storage for reads*/
	char readBuff[TSVTABLEREAD_BUFFER_SIZE];
	/**Offset into the read buffer.*/
	int readBuffO;
	/**Number of bytes in the read buffer.*/
	int readBuffS;
	/**Make sure the buffer has stuff in it.*/
	void fillBuffer();
};

/**Read a block compressed table file.*/
class BCompTabularReader : public TabularReader{
public:
	/**
	 * Set up a parser for the thing.
	 * @param toFlit The thing to read from.
	 * @param indFName The name of the index file.
	 */
	BCompTabularReader(BlockCompInStream* toFlit, const char* indFName);
	/**Tear down.*/
	virtual ~BCompTabularReader();
	int readNextEntry();
	/**
	 * Get the number of entries in the file.
	 * @return The number of entries.
	 */
	uintptr_t getNumEntries();
	/**
	 * Load an entry by index.
	 * @param entInd The index to load.
	 */
	void readSpecificEntry(uintptr_t entInd);
	
	/**If a random access was called, use this to reset the stream.*/
	intptr_t resetInd;
	/**The next index to report.*/
	uintptr_t focusInd;
	/**The number of entries in the file.*/
	uintptr_t rowCount;
	/**The actual thing to read.*/
	BlockCompInStream* theStr;
	/**The index file.*/
	FILE* indF;
	
	/**The lengths of each entry.*/
	std::vector<uintptr_t> entryLens;
	/**The full text.*/
	std::vector<char> entryFlat;
	/**The pointers to the head of each entry.*/
	std::vector<const char*> entryHeads;
};

/**Write a sequence of delimitted data.*/
class TabularWriter{
public:
	/**The number of entries (columns) in the current entry.*/
	uintptr_t numEntries;
	/**The length of each entry.*/
	uintptr_t* entrySizes;
	/**The text of each entry.*/
	const char** curEntries;
	/**Clear the above.*/
	TabularWriter();
	/**Allow subclassing.*/
	virtual ~TabularWriter();
	/**
	 * The next entry to read.
	 * @return Whether there was a problem (-1).
	 */
	virtual void writeNextEntry() = 0;
};

/**Write a tsv file.*/
class TSVTabularWriter : public TabularWriter{
public:
	/**
	 * Set up a parser for the thing.
	 * @param escapes Whether escapes are enabled.
	 * @param mainFrom The thing to parse.
	 */
	TSVTabularWriter(int escapes, OutStream* mainTo);
	/**Tear down.*/
	virtual ~TSVTabularWriter();
	void writeNextEntry();
	/**Whether escapes are enabled.*/
	int escEnable;
	/**The thing to parse.*/
	OutStream* theStr;
	/**Temporary storage for output stuff.*/
	std::string outTmp;
};

/**Write a tsv file.*/
class BCompTabularWriter : public TabularWriter{
public:
	/**
	 * Set up a parser for the thing.
	 * @param append Whether stuff is being appended.
	 * @param toFlit The thing to write to.
	 * @param indFName The name of the index file.
	 */
	BCompTabularWriter(int append, BlockCompOutStream* toFlit, const char* indFName);
	/**Tear down.*/
	virtual ~BCompTabularWriter();
	void writeNextEntry();
	/**The thing to parse.*/
	BlockCompOutStream* theStr;
	/**The index file.*/
	FILE* indF;
	/**Temporary storage.*/
	std::vector<char> outTmp;
};

/**A full set of tabular data.*/
class TabularData{
public:
	/**Make an empty table.*/
	TabularData();
	/**
	 * Preload the tabular data.
	 * @param toLoad The stream to load.
	 */
	TabularData(TabularReader* toLoad);
	/**Clean up.*/
	~TabularData();
	/**
	 * Write the data to the given writer.
	 * @param dumpTo The place to write.
	 */
	void dumpData(TabularWriter* dumpTo);
	/**The loaded entries.*/
	std::vector< std::vector<std::string> > allEntries;
};

#endif