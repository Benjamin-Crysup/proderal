#include "whodun_suffix.h"
#include "whodun_suffix_private.h"

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
