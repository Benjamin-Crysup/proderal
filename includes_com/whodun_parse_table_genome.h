#ifndef WHODUN_PARSE_TABLE_GENOME_H
#define WHODUN_PARSE_TABLE_GENOME_H 1

#include "whodun_parse_table.h"

/**Interpret a tsv as a bed file.*/
class BedFileReader{
public:
	/**
	 * Parse the contents of a bed file.
	 * @param readFrom The tsv to read from (or csv, or whatever).
	 */
	BedFileReader(TabularReader* readFrom);
	/**Clean tear down.*/
	~BedFileReader();
	/**The chromosomes of each entry.*/
	std::vector<std::string> chromosomes;
	/**The zero based start locations of each entry.*/
	std::vector<uintptr_t> locStarts;
	/**The post-end locations of each entry.*/
	std::vector<uintptr_t> locEnds;
};

/**
 * Utility to parse a UCSC coordinate.
 * @param ucsc The coordinate string to parse.
 * @param refStore The place to put the reference.
 * @param errStore The place to put an error message.
 * @return The start and end coordinates.
 */
std::pair<uintptr_t,uintptr_t> parseUCSCCoordinate(const char* ucsc, std::string* refStore, std::string* errStore);

#define SAM_FLAG_MULTSEG 1
#define SAM_FLAG_ALLALN 2
#define SAM_FLAG_SEGUNMAP 4
#define SAM_FLAG_NEXTSEGUNMAP 8
#define SAM_FLAG_SEQREVCOMP 16
#define SAM_FLAG_NEXTSEQREVCOMP 32
#define SAM_FLAG_FIRST 64
#define SAM_FLAG_LAST 128
#define SAM_FLAG_SECONDARY 256
#define SAM_FLAG_FILTFAIL 512
#define SAM_FLAG_DUPLICATE 1024
#define SAM_FLAG_SUPPLEMENT 2048

/**A parsing of a sam file.*/
class CRBSAMFileContents{
public:
	/**Whether the last read was a header.*/
	int lastReadHead;
	/**The text of said header, if present.*/
	std::vector<char> headerTxt;
	/**The name of the last entry. Empty for unknown.*/
	std::vector<char> entryName;
	/**The flag of the last entry.*/
	int entryFlag;
	/**The reference it is on. Empty for unknown.*/
	std::vector<char> entryReference;
	/**The position in the reference of the first base in the sequence. -1 for unmapped.*/
	intptr_t entryPos;
	/**Mapping quality of the entry. 255 for fail.*/
	unsigned char entryMapq;
	/**The cigar string for the entry. Empty for unknown.*/
	std::vector<char> entryCigar;
	/**The reference the next part of the template is on. Empty for unknown.*/
	std::vector<char> nextReference;
	/**The position in the reference of the first base in the next part of the template. -1 for unmapped.*/
	intptr_t nextPos;
	/**The length of the template (positive for the early read, negative for the late read).*/
	intptr_t entryTempLen;
	/**The sequence. Empty for not present.*/
	std::vector<char> entrySeq;
	/**The phred based quality scores for the last entry. Empty for not present.*/
	std::vector<char> entryQual;
	/**Any extra crap for the entry.*/
	std::vector<char> entryExtra;
	/**Set up*/
	CRBSAMFileContents();
	/**Tear down.*/
	~CRBSAMFileContents();
	/**
	 * Get the number of bytes this uses when packed.
	 * @return The number of bytes used.
	 */
	uintptr_t getPackedSize();
	/**
	 * Pack this into storage.
	 * @param packInto THe place to pack to.
	 */
	void pack(char* packInto);
	/**
	 * Unpack from storage.
	 * @param unpackFrom The place to unpack from.
	 */
	void unpack(char* unpackFrom);
	/**Quickly clear this entry.*/
	void clear();
};

/**Read a "SAM" file of some flavor.*/
class CRBSAMFileReader{
public:
	/**The current entry.*/
	uintptr_t curEntry;
	/**The currently read entry.*/
	CRBSAMFileContents curEnt;
	/**Set up*/
	CRBSAMFileReader();
	/**Tear down.*/
	virtual ~CRBSAMFileReader();
	/**
	 * Read the next entry into internal storage.
	 * @return Whether there was an entry.
	 */
	int readNextEntry();
	/**
	 * Read the next entry into the supplied storage.
	 * @param toFill The place to read to.
	 * @return Whether there was an entry.
	 */
	virtual int readNextEntry(CRBSAMFileContents* toFill) = 0;
};

/**Write a "SAM" file of some flavor.*/
class CRBSAMFileWriter{
public:
	/**The entry to write.*/
	CRBSAMFileContents curEnt;
	/**Set up*/
	CRBSAMFileWriter();
	/**Tear down.*/
	virtual ~CRBSAMFileWriter();
	/**
	 * Write the next entry from internal storage.
	 */
	void writeNextEntry();
	/**
	 * Write the next entry from a location.
	 * @param toFill The entry to write.
	 */
	virtual void writeNextEntry(CRBSAMFileContents* toFill) = 0;
};

/**Read a sam file.*/
class SAMFileReader : public CRBSAMFileReader{
public:
	/**
	 * Wrap a TSV.
	 * @param toParse The TSV to wrap.
	 */
	SAMFileReader(TabularReader* toParse);
	/**Tear down.*/
	~SAMFileReader();
	int readNextEntry(CRBSAMFileContents* toFill);
	/**The tsv file to parse.*/
	TabularReader* fromTsv;
	/**Temporary storage for stuff.*/
	std::string tempStore;
};

/**Write a sam file.*/
class SAMFileWriter : public CRBSAMFileWriter{
public:
	/**
	 * Wrap a TSV.
	 * @param toParse The TSV to wrap.
	 */
	SAMFileWriter(TabularWriter* toParse);
	/**Tear down.*/
	~SAMFileWriter();
	void writeNextEntry(CRBSAMFileContents* toFill);
	/**The tsv file to dump.*/
	TabularWriter* fromTsv;
	/**Temporary text storage.*/
	std::vector<char> tmpFlat;
	/**Temporary text storage.*/
	std::vector<uintptr_t> tmpFlatS;
	/**Temporary text storage.*/
	std::vector<uintptr_t> tmpFlatE;
	/**Temporary token storage.*/
	std::vector<char*> tmpS;
	/**Temporary token storage.*/
	std::vector<char*> tmpE;
	/**Temporary token storage.*/
	std::vector<uintptr_t> tmpL;
};

/**Read a binary sam file.*/
class BAMFileReader : public CRBSAMFileReader{
public:
	/**
	 * Wrap a TSV.
	 * @param toParse The TSV to wrap.
	 */
	BAMFileReader(InStream* toParse);
	/**Tear down.*/
	~BAMFileReader();
	int readNextEntry(CRBSAMFileContents* toFill);
	/**The file to parse.*/
	InStream* fromSrc;
	/**Whether the header has been handled.*/
	int doneHead;
	/**offset into temp store (for reading the header).*/
	uintptr_t tempO;
	/**Temporary storage for stuff.*/
	std::vector<char> tempStore;
	/**Names of the reference sequences.*/
	std::vector<std::string> refNames;
};

/**
 * Turn a cigar string to reference positions.
 * @param refPos0 The 0 position.
 * @param cigStr The cigar string.
 * @param fillPos The place to put the positions the read bases correspond to: -1 for no correspondence.
 * @return The number of soft clipped bases before and after the sequence.
 */
std::pair<uintptr_t,uintptr_t> cigarStringToReferencePositions(uintptr_t refPos0, std::vector<char>* cigStr, std::vector<intptr_t>* fillPos);

/**
 * Get the extent of some reference bounds.
 * @param lookPos The positions of the things in the reference.
 * @return The low and high indices in the reference (inclusive): if all are insertions, will be -1,-1.
 */
std::pair<intptr_t,intptr_t> getCigarReferenceBounds(std::vector<intptr_t>* lookPos);

/**
 * Open a named sam/bam/cram file for reading.
 * @param fileName The name of the file to open: "-" for stdin.
 * @param saveIS The base input stream, if any.
 * @param saveTS The base table stream, if any.
 * @param saveSS The cbsam stream.
 */
void openCRBSamFileRead(const char* fileName, InStream** saveIS, TabularReader** saveTS, CRBSAMFileReader** saveSS);

/**
 * Open a named sam/bam/cram file for writing.
 * @param fileName The name of the file to open: "-" for stdout.
 * @param saveIS The base output stream, if any.
 * @param saveTS The base table stream, if any.
 * @param saveSS The cbsam stream.
 */
void openCRBSamFileWrite(const char* fileName, OutStream** saveIS, TabularWriter** saveTS, CRBSAMFileWriter** saveSS);

#endif