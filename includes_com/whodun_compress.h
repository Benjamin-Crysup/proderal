#ifndef WHODUN_COMPRESS_H
#define WHODUN_COMPRESS_H 1

#include <stdio.h>
#include <stdint.h>

/**A method for compressing files.*/
class CompressionMethod{
public:
	/**Allow subclasses to deconstruct.*/
	virtual ~CompressionMethod();
	
	/**Open a file for compression using something. Same interface as fopen.*/
	virtual void* comfopen(const char* fileName, const char* mode) = 0;

	/**Write to a compress file. Same interface as fwrite.*/
	virtual size_t comfwrite(const void* ptr, size_t size, size_t count, void* stream) = 0;

	/**Read from a compressed file. Same interface as fread.*/
	virtual size_t comfread(void* ptr, size_t size, size_t count, void* stream) = 0;

	/**Get the current position of the stream (number of bytes read or written). Same interface as ftell, except it returns a pointer integer.*/
	virtual intptr_t comftell(void* stream) = 0;

	/**Close a compressed file. Same interface as fclose.*/
	virtual int comfclose(void* stream) = 0;
	
	/**
	 * Decompress standalone data.
	 * @param datLen The length of the compressed data.
	 * @param data The compressed data.
	 * @return The decompressed data: length followed by data. WIll need to be freed.
	 */
	virtual uintptr_t* decompressData(uintptr_t datLen, char* data) = 0;
	
	/**
	 * Decompress standalone data.
	 * @param datLen The length of the decompressed data.
	 * @param data The decompressed data.
	 * @return The compressed data. WIll need to be freed.
	 */
	virtual uintptr_t* compressData(uintptr_t datLen, char* data) = 0;
};

/**This will just dump with no compression.*/
class RawOutputCompressionMethod : public CompressionMethod{
public:
	/**Deconstruct*/
	~RawOutputCompressionMethod();
	
	void* comfopen(const char* fileName, const char* mode);
	size_t comfwrite(const void* ptr, size_t size, size_t count, void* stream);
	size_t comfread(void* ptr, size_t size, size_t count, void* stream);
	intptr_t comftell(void* stream);
	int comfclose(void* stream);
	uintptr_t* decompressData(uintptr_t datLen, char* data);
	uintptr_t* compressData(uintptr_t datLen, char* data);
};

#endif
