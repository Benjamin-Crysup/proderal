#ifndef WHODUN_PARSE_H
#define WHODUN_PARSE_H 1

#include <vector>

/**
 * Split a section of memory into tokens.
 * @param splitF The start of the section.
 * @param splitE The end of the section.
 * @param splitC The character to split on.
 * @param splitS The place to add the token start locations.
 * @param splitE The place to add the token end locations.
 */
void splitOnCharacter(const char* splitF, const char* splitT, int splitC, std::vector<const char*>* splitS, std::vector<const char*>* splitE);

/**
 * Split a section of memory into tokens.
 * @param splitF The start of the section.
 * @param splitE The end of the section.
 * @param splitCN The number of characters to split on.
 * @param splitCs The characters to split on.
 * @param splitS The place to add the token start locations.
 * @param splitE The place to add the token end locations.
 */
void splitOnCharacters(const char* splitF, const char* splitT, int splitCN, const char* splitCs, std::vector<const char*>* splitS, std::vector<const char*>* splitE);

#endif