#ifndef PRODERAL_SETUP_H
#define PRODERAL_SETUP_H 1

#include <map>
#include <vector>
#include <iostream>

#include "whodun_bamfa.h"
#include "whodun_align_affinepd.h"

#include "proderal_entry.h"

/**Various command line arguments*/
#define CLARG_HELPA "--help"
#define CLARG_HELPB "-h"
#define CLARG_HELPC "/?"
#define CLARG_VERSION "--version"
#define CLARG_REF "--ref"
#define CLARG_PROB "--prob"
#define CLARG_PROMIS "--promiscuous"
#define CLARG_COST "--cost"
#define CLARG_OVERRUN "--overrun"
#define CLARG_LOCAL "--atlocal"
#define CLARG_SEMI "--atsemi"
#define CLARG_GLOBAL "--atglobal"
#define CLARG_RANK "--rank"
#define CLARG_COUNT "--count"
#define CLARG_SOFT "--reclaimsoft"
#define CLARG_QUALM "--qualm"
#define CLARG_CACHE "--cache"
#define CLARG_HFUZZ "--hfuzz"

/**Parsed command line arguments.*/
typedef struct{
	/**Whether the rest needs to run.*/
	bool needRun;
	/**The reference file.*/
	const char* refFile;
	/**The problem region file.*/
	const char* probFile;
	/**Whether all regions should be realigned.*/
	bool hitAll;
	/**The cost specification file.*/
	const char* costFile;
	/**The amount of overrun for region realign.*/
	int overRun;
	/**The number of ends in the alignment.*/
	int numEnds;
	/**The worst rank to look at.*/
	int uptoRank;
	/**The number of alignments to report.*/
	int uptoCount;
	/**Try to reclaim soft clipped bases.*/
	int recSoft;
	/**The amount of cache.*/
	uintptr_t cachesize;
	/**The quality mangle file.*/
	const char* qualmFile;
	/**The maximum number of times to see a score before stopping.*/
	int hotfuzz;
} ProderalCLIArgs;

/**
 * This will parse command line arguments.
 * @param argc The number of command line arguments.
 * @param argv The arguments.
 * @param toFill The place to add the parsed data.
 * @param fileStore The place to put files.
 * @return Whether there was a problem.
 */
int parseArguments(int argc, char** argv, ProderalCLIArgs* toFill, std::vector<const char*>* fileStore);

/**Handles loading information for future steps.*/
class ProblemLoadSetup{
public:
	/**
	 * Loads problem information.
	 * @param The program arguments.
	 */
	ProblemLoadSetup(ProderalCLIArgs* pargs);
	/**Free the costs*/
	~ProblemLoadSetup();
	/**Whether this setup correctly loaded.*/
	bool setupValid;
	/**If invalid, the setup error message.*/
	std::string setupErr;
	/**the reference sequences: map from reference name to reference sequence*/
	std::map< std::string, std::string > allRefs;
	/**The problematic regions, by reference.*/
	std::map< std::string , std::vector< std::pair<intptr_t,intptr_t> > > probRegMap;
	/**The position dependent cost information.*/
	std::map<std::string,PositionDependentAlignCostKDNode*> allRegCosts;
	/**The quality mangle stuff.*/
	std::map<std::string, PositionDependentQualityChangeSet* > allQualMangs;
};

/**A collection of sam files being read through.*/
class SAMFileSet{
public:
	/**
	 * Opens all the sam files.
	 * @param fileNames The sam files to work over.
	 * @param toDump The place to write the header information to.
	 */
	SAMFileSet(std::vector<const char*>* fileNames, std::ostream* toDump);
	/**Close all the files.*/
	~SAMFileSet();
	/**
	 * Load the next entry.
	 * @return The loaded entry. Null if nothing left.
	 */
	LiveSAMFileReader* getNextEntry();
	/**The open files.*/
	std::vector<LiveSAMFileReader*> openFiles;
	/**Whether each file has an entry waiting.*/
	std::vector<bool> entryWaiting;
};

/**Load and prepare for a run.*/
class ProderalPreload{
public:
	/**
	 * Load everything.
	 * @param argc The number of arguments.
	 * @param argv The arguments.
	 */
	ProderalPreload(int argc, char** argv);
	/**Clean up.*/
	~ProderalPreload();
	/**Whether everything loaded.*/
	bool isValid;
	/**The parsed arguments.*/
	ProderalCLIArgs* pargs;
	/**Reference information.*/
	ProblemLoadSetup* refd;
	/**The files to read through.*/
	SAMFileSet* allFs;
	/**Things waiting for pairs.*/
	std::map< std::string, WaitingForPairSet* > waitingPairs;
};

#endif