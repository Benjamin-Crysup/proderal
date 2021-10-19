#ifndef WHODUN_PROFILE_H
#define WHODUN_PROFILE_H 1

//profiles and pedigrees

#include <map>
#include <set>
#include <vector>
#include <stdint.h>

#include "whodun_datread.h"

/**A view of a profile.*/
class GeneticProfile{
public:
	
	/**Set up.*/
	GeneticProfile();
	/**Clean up*/
	~GeneticProfile();
	
	/**The start index of each loci.*/
	std::vector<uintptr_t> lociStart;
	/**The number of alleles at each locus.*/
	std::vector<uintptr_t> alleleCounts;
	
	/**The lengths of each allele.*/
	std::vector<uintptr_t> alleleLens;
	/**The alleles at each locus.*/
	std::vector<const char*> alleles;
};

/**Genetic profiles*/
class GeneticProfileSet{
public:
	
	/**
	 * Make an uninitialized profile.
	 */
	GeneticProfileSet();
	/**
	 * Make an empty profile.
	 * @param numberOfLoci The number of loci in the profile.
	 */
	GeneticProfileSet(uintptr_t numberOfLoci);
	/**Clean up.*/
	~GeneticProfileSet();
	
	/**The number of loci in the profile.*/
	uintptr_t numLoci;
	/**The start indices for each profile.*/
	std::vector<uintptr_t> profileSInds;
	/**The number of alleles at each locus for the individuals.*/
	std::vector<uintptr_t> lociCounts;
	/**The lengths of each allele at the loci.*/
	std::vector<uintptr_t> alleleLens;
	/**The start index of each allele item.*/
	std::vector<uintptr_t> alleleStarts;
	/**Storage for the alleles.*/
	std::vector<char> alleleText;
	
	/**
	 * Get the number of profiles.
	 * @return The number of profiles.
	 */
	uintptr_t size();
	
	/**
	 * Get a profile.
	 * @param profInd The index of the profile.
	 * @param storeLoc The place to put the profile.
	 */
	void getProfile(uintptr_t profInd, GeneticProfile* storeLoc);
	
	/**
	 * Add a profile.
	 * @param toAdd The profile to add.
	 */
	void addProfile(GeneticProfile* toAdd);
	
	/**
	 * Dump the profile set to a file.
	 * @param dumpTo The place to write.
	 */
	void dumpSet(OutStream* dumpTo);
	
	/**
	 * Load profile set from a file.
	 * @param dumpTo The place to read.
	 */
	void loadSet(InStream* dumpTo);
	
	/**
	 * Make a vector of uintptr_t ready to write.
	 * @param toPack The vector to make ready.
	 * @param packTo The place to store the pack.
	 */
	void helpPackVector(std::vector<uintptr_t>* toPack, std::vector<char>* packTo);
	
	/**
	 * Unpack bulk loaded data.
	 * @param toPack The vector to make ready.
	 * @param packTo The place to store the unpack.
	 */
	void helpUnpackVector(std::vector<char>* toPack, std::vector<uintptr_t>* packTo);
};

/**A pedigree.*/
class Pedigree{
public:
	/**Set up an empty pedigree.*/
	Pedigree();
	/**Clean up.*/
	~Pedigree();
	
	/**All the individuals in the pedigree, along with optional annotation.*/
	std::set<uintptr_t> allPeople;
	/**All the (direct) links between individuals, along with optional annotation. ChildID X ParentID.*/
	std::set< std::pair<uintptr_t,uintptr_t> > allParents;
	
	/**Prepare allChildren for use. Any edits to allParents will invalidate it.*/
	void prepareChildren();
	/**Storage for the inverted relationships.*/
	std::vector< std::pair<uintptr_t,uintptr_t> > allChildren;
	
	/**
	 * Get all immediate parents in the pedigree.
	 * @param ofPerson The person of interest.
	 * @param addTo The place to add the parents to.
	 */
	void getAllParents(uintptr_t ofPerson, std::vector<uintptr_t>* addTo);
	
	/**
	 * Get all immediate children in the pedigree.
	 * @param ofPerson The person of interest.
	 * @param addTo The place to add the children to.
	 */
	void getAllChildren(uintptr_t ofPerson, std::vector<uintptr_t>* addTo);
	
	/**
	 * Dump the pedigree to a file.
	 * @param dumpTo The place to write.
	 */
	void dumpPedigree(OutStream* dumpTo);
	
	/**
	 * Load pedigree from a file.
	 * @param dumpTo The place to read.
	 */
	void loadPedigree(InStream* dumpTo);
};



#endif