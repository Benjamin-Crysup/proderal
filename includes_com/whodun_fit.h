#ifndef WHODUN_FIT_H
#define WHODUN_FIT_H 1

#include "whodun_nmcy.h"
#include "whodun_parse_table.h"

#define DATAPOINT_CELL_INT 1
#define DATAPOINT_CELL_FLT 2
#define DATAPOINT_CELL_CAT 3

/**A cell in a table.*/
typedef union{
	/**For integer data.*/
	intptr_t idat;
	/**For real data.*/
	double ddat;
	/**For categorical data.*/
	int cdat;
} DatapointCell;

/**The layout of a table.*/
class DatapointDescription{
public:
	/**Set up.*/
	DatapointDescription();
	/**Tear down.*/
	~DatapointDescription();
	/**
	 * Figure out the mapping from columns to variables.
	 * @param parseF The table to parse from.
	 */
	void figureColumnsFromHeader(TabularReader* parseF);
	/**
	 * Mark that columns are in table order.
	 */
	void figureColumnsIdentity();
	/**
	 * Output the colum names to a header.
	 * @param dumpT The place to dump to.
	 */
	void dumpColumnHeader(TabularWriter* dumpT);
	
	/**
	 * Load a header from a file.
	 * @param parseF The file to parse.
	 */
	void loadHeader(InStream* parseF);
	
	/**
	 * Write a header to a file.
	 * @param parseF The file to write to.
	 */
	void writeHeader(OutStream* parseF);
	
	/**
	 * Parse a row of data from a table.
	 * @param parseF The place to parse from.
	 * @param saveL The place to save the parsed data.
	 */
	void parseRow(TabularReader* parseF, DatapointCell* saveL);
	/**
	 * Dump a row to a table.
	 * @param dumpT The place to write.
	 * @param saveL The data to write.
	 */
	void dumpRow(TabularWriter* dumpT, DatapointCell* saveL);
	
	/**
	 * Load a row from a file.
	 * @param parseF The file to parse.
	 * @param saveL The data to write.
	 * @return Whether there was a row.
	 */
	int loadRow(InStream* parseF, DatapointCell* saveL);
	
	/**
	 * Write a row to a file.
	 * @param parseF The file to parse.
	 * @param saveL The data to write.
	 */
	void writeRow(OutStream* parseF, DatapointCell* saveL);
	
	/**The types of data in each column.*/
	std::vector<int> colTypes;
	/**The names of the columns.*/
	std::vector<std::string> colNames;
	
	/**For categorical data from strings, the valid levels.*/
	std::vector< std::map<std::string,int> > factorColMap;
	/**The next level for categorical data.*/
	std::vector<int> factorMaxLevel;
	/**The maximum column index.*/
	uintptr_t maxCIndex;
	/**Map from names to indices: either fill in or parse from header.*/
	std::vector<uintptr_t> colIndices;
	/**Go back from a index to a name.*/
	std::vector< std::map<int,std::string> > colFactorMap;
	
	/**Temporary storage of sizes.*/
	std::vector<uintptr_t> tmpDumpS;
	/**Temporary storage of pointers.*/
	std::vector<char*> tmpDumpCp;
	/**Temporary storage of characters.*/
	std::vector<char> tmpDumpC;
	/**Temporary storage for a name.*/
	std::string tmpName;
	
	/**Staging area for io operations.*/
	std::vector<char> ioStage;
};

#endif