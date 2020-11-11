#ifndef WHODUN_DATREAD_H
#define WHODUN_DATREAD_H 1

#include <string>
#include <stdio.h>

/**IOStream is a mess.*/
class OutStream{
public:
	/**Basic setup.*/
	OutStream();
	/**Clean up and close.*/
	virtual ~OutStream();
	/**
	 * Write a byte.
	 * @param toW The byte to write.
	 */
	virtual void writeByte(int toW) = 0;
	/**
	 * Write bytes.
	 * @param toW The bytes to write.
	 * @param numW The number of bytes to write.
	 */
	virtual void writeBytes(const char* toW, uintptr_t numW);
	/**Flush waiting bytes.*/
	virtual void flush();
};

/**IOStream is a mess.*/
class InStream{
public:
	/**Basic setup.*/
	InStream();
	/**Clean up and close.*/
	virtual ~InStream();
	/**
	 * Read a byte.
	 * @return The read byte. -1 for eof.
	 */
	virtual int readByte() = 0;
	/**
	 * Read bytes.
	 * @param toR The buffer to store in.
	 * @param numR The maximum number of read.
	 * @return The number actually read. If less than numR, hit end.
	 */
	virtual uintptr_t readBytes(char* toR, uintptr_t numR);
};

/**Out to console.*/
class ConsoleOutStream: public OutStream{
public:
	/**Basic setup.*/
	ConsoleOutStream();
	/**Clean up and close.*/
	~ConsoleOutStream();
	void writeByte(int toW);
	void writeBytes(const char* toW, uintptr_t numW);
};

/**Out to error.*/
class ConsoleErrStream: public OutStream{
public:
	/**Basic setup.*/
	ConsoleErrStream();
	/**Clean up and close.*/
	~ConsoleErrStream();
	void writeByte(int toW);
};

/**In from console.*/
class ConsoleInStream: public InStream{
public:
	/**Basic setup.*/
	ConsoleInStream();
	/**Clean up and close.*/
	~ConsoleInStream();
	int readByte();
	uintptr_t readBytes(char* toR, uintptr_t numR);
};

/**Out to file.*/
class FileOutStream: public OutStream{
public:
	/**
	 * Open the file.
	 * @param append Whether to append to a file if it is already there.
	 * @param fileName The name of the file.
	 */
	FileOutStream(int append, const char* fileName);
	/**Clean up and close.*/
	~FileOutStream();
	void writeByte(int toW);
	void writeBytes(const char* toW, uintptr_t numW);
	/**The base file.*/
	FILE* baseFile;
	/**The name of the file.*/
	std::string myName;
};

/**In from file.*/
class FileInStream : public InStream{
public:
	/**
	 * Open the file.
	 * @param fileName The name of the file.
	 */
	FileInStream(const char* fileName);
	/**Clean up and close.*/
	~FileInStream();
	virtual int readByte();
	uintptr_t readBytes(char* toR, uintptr_t numR);
	/**The base file.*/
	FILE* baseFile;
	/**The name of the file.*/
	std::string myName;
};

/**
 * Read the entire contents of the stream.
 * @param readF The stream to read from.
 * @param toFill The place to put the bytes.
 */
void readStream(InStream* readF, std::string* toFill);

#endif
