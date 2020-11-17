#ifndef WHODUN_COMPRESS_H
#define WHODUN_COMPRESS_H 1

#include <string>
#include <vector>
#include <stdio.h>
#include <stdint.h>

#include <zlib.h>

#include "whodun_datread.h"

/**A method for compressing files.*/
class CompressionMethod{
public:
	/**Allow subclasses to deconstruct.*/
	virtual ~CompressionMethod();
	/**Decompress the data stored in the compData into theData.*/
	virtual void decompressData() = 0;
	/**Compress the data in theData into compData.*/
	virtual void compressData() = 0;
	/**The data to compress and/or the decompressed data.*/
	std::vector<char> theData;
	/**The compressed data and/or data to decompress.*/
	std::vector<char> compData;
};

/**Bytes for an annotation entry.*/
#define BLOCKCOMP_ANNOT_ENTLEN 32
/**The number of entries to load in memory: less jumping.*/
#define BLOCKCOMPIN_LASTLINESEEK 1024

/**Block compress output.*/
class BlockCompOutStream : public OutStream{
public:
	/**
	 * Open up the file to dump to.
	 * @param append Whether to append to a file if it is already there.
	 * @param blockSize The size of the compressed blocks.
	 * @param mainFN The name of the data file.
	 * @param annotFN The name of the annotation file.
	 * @param compMeth The compression method to use for the blocks.
	 */
	BlockCompOutStream(int append, uintptr_t blockSize, const char* mainFN, const char* annotFN, CompressionMethod* compMeth);
	/**Clean up and close.*/
	~BlockCompOutStream();
	void writeByte(int toW);
	void writeBytes(const char* toW, uintptr_t numW);
	/**Get the number of (uncompressed) bytes already written.*/
	uintptr_t tell();
	/**Dump the data.*/
	void dumpCompData();
	/**The number of bytes to accumulate before dumping a block.*/
	uintptr_t chunkSize;
	/**The total number of uncompressed bytes written to the start of the current block.*/
	uintptr_t preCompBS;
	/**The total number of compressed bytes written to the start of the current block.*/
	uintptr_t postCompBS;
	/**The data file.*/
	FILE* mainF;
	/**The annotation file. Quads of pre-comp address, post-comp address, pre-comp len, post-comp len.*/
	FILE* annotF;
	/**The compression method this uses.*/
	CompressionMethod* myComp;
};

/**Read a block compressed input stream.*/
class BlockCompInStream : public InStream{
public:
	/**
	 * Open up a blcok compressed file.
	 * @param mainFN The name of the data file.
	 * @param annotFN The name of the annotation file.
	 * @param compMeth The compression method to use for the blocks.
	 */
	BlockCompInStream(const char* mainFN, const char* annotFN, CompressionMethod* compMeth);
	/**Clean up and close.*/
	~BlockCompInStream();
	int readByte();
	uintptr_t readBytes(char* toR, uintptr_t numR);
	/**
	 * Change which byte will be returned next.
	 * @param toAddr The (pre-compression) address.
	 */
	void seek(uintptr_t toAddr);
	/**
	 * Get the uncompressed size of this file.
	 * @return The number of bytes in the file.
	 */
	uintptr_t getUncompressedSize();
	/**The number of blocks in the file.*/
	uintptr_t numBlocks;
	/**The number of blocks accounted for in lastLineBuff.*/
	uintptr_t numLastLine;
	/**Temporary storage for a seek.*/
	char* lastLineBuff;
	/**The pre-compressed addresses of those block infos.*/
	uintptr_t* lastLineAddrs;
	/**The data file.*/
	FILE* mainF;
	/**The annotation file. Quads of pre-comp address, post-comp address, pre-comp len, post-comp len.*/
	FILE* annotF;
	/**The index in the decompressed data the next read should return.*/
	uintptr_t nextReadI;
	/**The compression method this uses.*/
	CompressionMethod* myComp;
};

//TODO multithreaded block compression (input and output)

/**Out to gzip file.*/
class GZipOutStream : public OutStream{
public:
	/**
	 * Open the file.
	 * @param append Whether to append to a file if it is already there.
	 * @param fileName The name of the file.
	 */
	GZipOutStream(int append, const char* fileName);
	/**Clean up and close.*/
	~GZipOutStream();
	void writeByte(int toW);
	void writeBytes(const char* toW, uintptr_t numW);
	/**The base file.*/
	gzFile baseFile;
	/**The name of the file.*/
	std::string myName;
};

/**In from gzip file.*/
class GZipInStream : public InStream{
public:
	/**
	 * Open the file.
	 * @param fileName The name of the file.
	 */
	GZipInStream(const char* fileName);
	/**Clean up and close.*/
	~GZipInStream();
	int readByte();
	uintptr_t readBytes(char* toR, uintptr_t numR);
	/**The base file.*/
	gzFile baseFile;
	/**The name of the file.*/
	std::string myName;
};

/**Compress by doing nothing.*/
class RawCompressionMethod : public CompressionMethod{
public:
	/**Simple clean.*/
	~RawCompressionMethod();
	void decompressData();
	void compressData();
};

/**Compress using gzip.*/
class GZipCompressionMethod : public CompressionMethod{
public:
	/**Simple clean.*/
	~GZipCompressionMethod();
	void decompressData();
	void compressData();
};

#endif
