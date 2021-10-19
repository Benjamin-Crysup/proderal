#include "whodun_suffix.h"

#include <string.h>

bool SingleStringSuffixRankSortOption_compMeth(void* unif, void* itemA, void* itemB){
	char* itemABts = ((char*)itemA) + SUFFIX_ARRAY_CANON_SIZE;
	char* itemBBts = ((char*)itemB) + SUFFIX_ARRAY_CANON_SIZE;
	return (memcmp(itemABts, itemBBts, 2*SUFFIX_ARRAY_CANON_SIZE) < 0);
}

bool SingleStringSuffixIndexSortOption_compMeth(void* unif, void* itemA, void* itemB){
	return (memcmp(itemA, itemB, SUFFIX_ARRAY_CANON_SIZE) < 0);
}

bool SingleStringSuffixRLPairSortOption_compMeth(void* unif, void* itemA, void* itemB){
	char* itemABts = ((char*)itemA) + SUFFIX_ARRAY_CANON_SIZE;
	char* itemBBts = ((char*)itemB) + SUFFIX_ARRAY_CANON_SIZE;
	return (memcmp(itemABts, itemBBts, SUFFIX_ARRAY_CANON_SIZE) < 0);
}

bool MultiStringSuffixRankSortOption_compMeth(void* unif, void* itemA, void* itemB){
	char* itemABts = ((char*)itemA) + 2*SUFFIX_ARRAY_CANON_SIZE;
	char* itemBBts = ((char*)itemB) + 2*SUFFIX_ARRAY_CANON_SIZE;
	return (memcmp(itemABts, itemBBts, 2*SUFFIX_ARRAY_CANON_SIZE) < 0);
}

bool MultiStringSuffixIndexSortOption_compMeth(void* unif, void* itemA, void* itemB){
	return (memcmp(itemA, itemB, 2*SUFFIX_ARRAY_CANON_SIZE) < 0);
}

bool MultiStringSuffixRLPairSortOption_compMeth(void* unif, void* itemA, void* itemB){
	char* itemABts = ((char*)itemA) + SUFFIX_ARRAY_CANON_SIZE;
	char* itemBBts = ((char*)itemB) + SUFFIX_ARRAY_CANON_SIZE;
	return (memcmp(itemABts, itemBBts, SUFFIX_ARRAY_CANON_SIZE) < 0);
}


