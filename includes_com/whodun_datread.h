#ifndef WHODUN_DATREAD_H
#define WHODUN_DATREAD_H 1

#include <vector>
#include <string>
#include <stdio.h>

/**
 * This will read the contents of a file.
 * @param readFrom The file name to read.
 * @param lenFill The place to write the final length.
 * @return The read file, or null if problem.
 */
char* readEntireFile(const char* readFrom, uintptr_t* lenFill);

/**
 * This will read a line of a data file (and lookalikes).
 * @param toRead The file to read from.
 * @param toFill The place to put the read data.
 * @return The number of items read. 0 means end of file, -1 means error.
 */
intptr_t readDatFileLine(FILE* toRead, std::vector<std::string>* toFill);

#endif