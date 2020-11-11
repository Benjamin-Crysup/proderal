#include "whodun_suffix.h"

#include <string.h>

bool SingleStringSuffixRankSortOption::compMeth(void* itemA, void* itemB){
	char* itemABts = ((char*)itemA) + SUFFIX_ARRAY_CANON_SIZE;
	char* itemBBts = ((char*)itemB) + SUFFIX_ARRAY_CANON_SIZE;
	return (memcmp(itemABts, itemBBts, 2*SUFFIX_ARRAY_CANON_SIZE) <= 0);
}

bool SingleStringSuffixIndexSortOption::compMeth(void* itemA, void* itemB){
	return (memcmp(itemA, itemB, SUFFIX_ARRAY_CANON_SIZE) <= 0);
}

bool SingleStringSuffixRLPairSortOption::compMeth(void* itemA, void* itemB){
	char* itemABts = ((char*)itemA) + SUFFIX_ARRAY_CANON_SIZE;
	char* itemBBts = ((char*)itemB) + SUFFIX_ARRAY_CANON_SIZE;
	return (memcmp(itemABts, itemBBts, SUFFIX_ARRAY_CANON_SIZE) <= 0);
}

bool MultiStringSuffixRankSortOption::compMeth(void* itemA, void* itemB){
	char* itemABts = ((char*)itemA) + 2*SUFFIX_ARRAY_CANON_SIZE;
	char* itemBBts = ((char*)itemB) + 2*SUFFIX_ARRAY_CANON_SIZE;
	return (memcmp(itemABts, itemBBts, 2*SUFFIX_ARRAY_CANON_SIZE) <= 0);
}

bool MultiStringSuffixIndexSortOption::compMeth(void* itemA, void* itemB){
	return (memcmp(itemA, itemB, 2*SUFFIX_ARRAY_CANON_SIZE) <= 0);
}

bool MultiStringSuffixRLPairSortOption::compMeth(void* itemA, void* itemB){
	char* itemABts = ((char*)itemA) + SUFFIX_ARRAY_CANON_SIZE;
	char* itemBBts = ((char*)itemB) + SUFFIX_ARRAY_CANON_SIZE;
	return (memcmp(itemABts, itemBBts, SUFFIX_ARRAY_CANON_SIZE) <= 0);
}

/**A default compression method.*/
RawCompressionMethod defaultComp;

#define SORT_OPTS_SETTINGS \
	maxLoad = 4096*itemSize;\
	numThread = 1;\
	workCom = 0;

SingleStringSuffixRankSortOption::SingleStringSuffixRankSortOption(){
	itemSize = 3*SUFFIX_ARRAY_CANON_SIZE;
	SORT_OPTS_SETTINGS
	compMeth = SingleStringSuffixRankSortOption_compMeth;
}

SingleStringSuffixIndexSortOption::SingleStringSuffixIndexSortOption(){
	itemSize = 3*SUFFIX_ARRAY_CANON_SIZE;
	SORT_OPTS_SETTINGS
	compMeth = SingleStringSuffixIndexSortOption_compMeth;
}

SingleStringSuffixRLPairSortOption::SingleStringSuffixRLPairSortOption(){
	itemSize = 2*SUFFIX_ARRAY_CANON_SIZE;
	SORT_OPTS_SETTINGS
	compMeth = SingleStringSuffixRLPairSortOption_compMeth;
}

MultiStringSuffixRankSortOption::MultiStringSuffixRankSortOption(){
	itemSize = 4*SUFFIX_ARRAY_CANON_SIZE;
	SORT_OPTS_SETTINGS
	compMeth = MultiStringSuffixRankSortOption_compMeth;
}

MultiStringSuffixIndexSortOption::MultiStringSuffixIndexSortOption(){
	itemSize = 4*SUFFIX_ARRAY_CANON_SIZE;
	SORT_OPTS_SETTINGS
	compMeth = MultiStringSuffixIndexSortOption_compMeth;
}

MultiStringSuffixRLPairSortOption::MultiStringSuffixRLPairSortOption(){
	itemSize = 2*SUFFIX_ARRAY_CANON_SIZE;
	SORT_OPTS_SETTINGS
	compMeth = MultiStringSuffixRLPairSortOption_compMeth;
}
