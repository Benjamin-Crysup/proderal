#include "whodun_suffix.h"

bool SingleStringSuffixRankSortOption_compMeth(void* itemA, void* itemB){
	bool toRet;
	//inline assembly
	asm volatile (
		"xorq %%rax, %%rax\n"
		"movq 8(%%rcx), %%r8\n"
		"movq 8(%%rdx), %%r9\n"
		"bswapq %%r8\n"
		"bswapq %%r9\n"
		"subq %%r8, %%r9\n"
		"setge %%al\n"
		"jne endCSOTest%=\n"
		"movq 16(%%rcx), %%r8\n"
		"movq 16(%%rdx), %%r9\n"
		"bswapq %%r8\n"
		"bswapq %%r9\n"
		"subq %%r8, %%r9\n"
		"setge %%al\n"
		"endCSOTest%=:\n"
	: "=a" (toRet)
	: "c" (itemA), "d" (itemB)
	: "r8", "r9", "cc"
	);
	return toRet;
}

bool SingleStringSuffixIndexSortOption_compMeth(void* itemA, void* itemB){
	bool toRet;
	//inline assembly
	asm volatile (
		"xorq %%rax, %%rax\n"
		"movq (%%rcx), %%r8\n"
		"movq (%%rdx), %%r9\n"
		"bswapq %%r8\n"
		"bswapq %%r9\n"
		"subq %%r8, %%r9\n"
		"setge %%al\n"
	: "=a" (toRet)
	: "c" (itemA), "d" (itemB)
	: "r8", "r9", "cc"
	);
	return toRet;
}

bool SingleStringSuffixRLPairSortOption_compMeth(void* itemA, void* itemB){
	bool toRet;
	//inline assembly
	asm volatile (
		"xorq %%rax, %%rax\n"
		"movq 8(%%rcx), %%r8\n"
		"movq 8(%%rdx), %%r9\n"
		"bswapq %%r8\n"
		"bswapq %%r9\n"
		"subq %%r8, %%r9\n"
		"setge %%al\n"
	: "=a" (toRet)
	: "c" (itemA), "d" (itemB)
	: "r8", "r9", "cc"
	);
	return toRet;
}

bool MultiStringSuffixRankSortOption_compMeth(void* itemA, void* itemB){
	bool toRet;
	//inline assembly
	asm volatile (
		"xorq %%rax, %%rax\n"
		"movq 16(%%rcx), %%r8\n"
		"movq 16(%%rdx), %%r9\n"
		"bswapq %%r8\n"
		"bswapq %%r9\n"
		"subq %%r8, %%r9\n"
		"setge %%al\n"
		"jne endCSOTest%=\n"
		"movq 24(%%rcx), %%r8\n"
		"movq 24(%%rdx), %%r9\n"
		"bswapq %%r8\n"
		"bswapq %%r9\n"
		"subq %%r8, %%r9\n"
		"setge %%al\n"
		"endCSOTest%=:\n"
	: "=a" (toRet)
	: "c" (itemA), "d" (itemB)
	: "r8", "r9", "cc"
	);
	return toRet;
}

bool MultiStringSuffixIndexSortOption_compMeth(void* itemA, void* itemB){
	bool toRet;
	//inline assembly
	asm volatile (
		"xorq %%rax, %%rax\n"
		"movq (%%rcx), %%r8\n"
		"movq (%%rdx), %%r9\n"
		"bswapq %%r8\n"
		"bswapq %%r9\n"
		"subq %%r8, %%r9\n"
		"setge %%al\n"
		"jne endCSOTest%=\n"
		"movq 8(%%rcx), %%r8\n"
		"movq 8(%%rdx), %%r9\n"
		"bswapq %%r8\n"
		"bswapq %%r9\n"
		"subq %%r8, %%r9\n"
		"setge %%al\n"
		"endCSOTest%=:\n"
	: "=a" (toRet)
	: "c" (itemA), "d" (itemB)
	: "r8", "r9", "cc"
	);
	return toRet;
}

bool MultiStringSuffixRLPairSortOption_compMeth(void* itemA, void* itemB){
	bool toRet;
	//inline assembly
	asm volatile (
		"xorq %%rax, %%rax\n"
		"movq 8(%%rcx), %%r8\n"
		"movq 8(%%rdx), %%r9\n"
		"bswapq %%r8\n"
		"bswapq %%r9\n"
		"subq %%r8, %%r9\n"
		"setge %%al\n"
	: "=a" (toRet)
	: "c" (itemA), "d" (itemB)
	: "r8", "r9", "cc"
	);
	return toRet;
}

#define SORT_OPTS_SETTINGS \
	maxLoad = 4096*itemSize;\
	numThread = 1;

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
