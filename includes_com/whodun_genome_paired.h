#ifndef WHODUN_GENOME_PAIRED_H
#define WHODUN_GENOME_PAIRED_H 1

#include <map>

#include "whodun_cache.h"
#include "whodun_parse_table_genome.h"

//tools for dealing with paired end data

/**
 * Get whether the given sequence is a primary alignment.
 * @param toExamine The sequence to examine.
 * @return WHether the entry is a primary alignment.
 */
bool samEntryIsPrimary(CRBSAMFileContents* toExamine);

/**
 * Get whether the given alignment represents an alignment.
 * @param toExamine THe sequence to examine.
 * @return Whether it was an alignment: mapped, cigar, and not all soft clipped.
 */
bool samEntryIsAlign(CRBSAMFileContents* toExamine);

/**
 * Determine if the entry needs matching with a pair.
 * @param toExamine The sam entry.
 * @return Whether it is waiting on a pair.
 */
bool samEntryNeedPair(CRBSAMFileContents* toExamine);

/**Cache sequences until their pair is found.*/
class PairedEndCache{
public:
	/**Set up an empty cache*/
	PairedEndCache();
	/**Clean up.*/
	~PairedEndCache();
	
	/**
	 * Is pair data present for the given entry.
	 * @param toExamine The entry to examine.
	 * @return Whether its pair is present.
	 */
	bool havePair(CRBSAMFileContents* toExamine);
	
	/**
	 * Get the pair data and remove from cache.
	 * @param forAln The alignment to get the pair of.
	 * @return The ID for the cached entry, and the cached entry.
	 */
	std::pair<uintptr_t,CRBSAMFileContents*> getPair(CRBSAMFileContents* forAln);
	
	/**
	 * Store the entry until its pair comes up.
	 * @param alnID An ID to associate with the entry.
	 * @param forAln The entry to store.
	 */
	void waitForPair(uintptr_t alnID, CRBSAMFileContents* forAln);
	
	/**
	 * Get whether there are any outstanding entries.
	 * @return Whether there are any outstanding.
	 */
	bool haveOutstanding();
	
	/**
	 * Get one of the remaining alignments.
	 * @return The ID for the cached entry, and the cached entry.
	 */
	std::pair<uintptr_t,CRBSAMFileContents*> getOutstanding();
	
	/**Names of sequences waiting for a pair.*/
	std::map<std::string, uintptr_t> waitingPair;
	/**The sequence alignments waiting for a pair.*/
	std::map<uintptr_t, CRBSAMFileContents* > pairData;
	
	/**Temporary storage for a name.*/
	std::string nameTmp;
};

#endif
