#ifndef WHODUN_SUFFIX_PRIVATE_H
#define WHODUN_SUFFIX_PRIVATE_H 1

#include "whodun_sort.h"
#include "whodun_compress.h"

/**Handles sorting by rank and next-rank.*/
class SingleStringSuffixRankSortOption : public SortOptions{
public:
	/**Basic setup.*/
	SingleStringSuffixRankSortOption();
	bool compMeth(void* itemA, void* itemB);
};

/**Handles sorting on index: useless in-memory, vital outside.*/
class SingleStringSuffixIndexSortOption : public SortOptions{
public:
	/**Basic setup.*/
	SingleStringSuffixIndexSortOption();
	bool compMeth(void* itemA, void* itemB);
};

/**Sorts rank/location pairs by location.*/
class SingleStringSuffixRLPairSortOption : public SortOptions{
public:
	/**Basic setup.*/
	SingleStringSuffixRLPairSortOption();
	bool compMeth(void* itemA, void* itemB);
};

/**Handles sorting by rank and next-rank.*/
class MultiStringSuffixRankSortOption : public SortOptions{
public:
	/**Basic setup.*/
	MultiStringSuffixRankSortOption();
	bool compMeth(void* itemA, void* itemB);
};

/**Handles sorting on index: useless in-memory, vital outside.*/
class MultiStringSuffixIndexSortOption : public SortOptions{
public:
	/**Basic setup.*/
	MultiStringSuffixIndexSortOption();
	bool compMeth(void* itemA, void* itemB);
};

/**Sorts rank/location pairs by location.*/
class MultiStringSuffixRLPairSortOption : public SortOptions{
public:
	/**Basic setup.*/
	MultiStringSuffixRLPairSortOption();
	bool compMeth(void* itemA, void* itemB);
};

#endif