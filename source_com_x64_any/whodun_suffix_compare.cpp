#include "whodun_suffix.h"

bool SingleStringSuffixRankSortOption_compMeth(void* unif, void* itemA, void* itemB){
	bool toRet;
	//inline assembly
	asm volatile (
		"xorq %%rax, %%rax\n"
		"movq 8(%%rcx), %%r8\n"
		"movq 8(%%rdx), %%r9\n"
		"bswapq %%r8\n"
		"bswapq %%r9\n"
		"subq %%r8, %%r9\n"
		"setg %%al\n"
		"jne endCSOTest%=\n"
		"movq 16(%%rcx), %%r8\n"
		"movq 16(%%rdx), %%r9\n"
		"bswapq %%r8\n"
		"bswapq %%r9\n"
		"subq %%r8, %%r9\n"
		"setg %%al\n"
		"endCSOTest%=:\n"
	: "=a" (toRet)
	: "c" (itemA), "d" (itemB)
	: "r8", "r9", "cc"
	);
	return toRet;
}

bool SingleStringSuffixIndexSortOption_compMeth(void* unif, void* itemA, void* itemB){
	bool toRet;
	//inline assembly
	asm volatile (
		"xorq %%rax, %%rax\n"
		"movq (%%rcx), %%r8\n"
		"movq (%%rdx), %%r9\n"
		"bswapq %%r8\n"
		"bswapq %%r9\n"
		"subq %%r8, %%r9\n"
		"setg %%al\n"
	: "=a" (toRet)
	: "c" (itemA), "d" (itemB)
	: "r8", "r9", "cc"
	);
	return toRet;
}

bool SingleStringSuffixRLPairSortOption_compMeth(void* unif, void* itemA, void* itemB){
	bool toRet;
	//inline assembly
	asm volatile (
		"xorq %%rax, %%rax\n"
		"movq 8(%%rcx), %%r8\n"
		"movq 8(%%rdx), %%r9\n"
		"bswapq %%r8\n"
		"bswapq %%r9\n"
		"subq %%r8, %%r9\n"
		"setg %%al\n"
	: "=a" (toRet)
	: "c" (itemA), "d" (itemB)
	: "r8", "r9", "cc"
	);
	return toRet;
}

bool MultiStringSuffixRankSortOption_compMeth(void* unif, void* itemA, void* itemB){
	bool toRet;
	//inline assembly
	asm volatile (
		"xorq %%rax, %%rax\n"
		"movq 16(%%rcx), %%r8\n"
		"movq 16(%%rdx), %%r9\n"
		"bswapq %%r8\n"
		"bswapq %%r9\n"
		"subq %%r8, %%r9\n"
		"setg %%al\n"
		"jne endCSOTest%=\n"
		"movq 24(%%rcx), %%r8\n"
		"movq 24(%%rdx), %%r9\n"
		"bswapq %%r8\n"
		"bswapq %%r9\n"
		"subq %%r8, %%r9\n"
		"setg %%al\n"
		"endCSOTest%=:\n"
	: "=a" (toRet)
	: "c" (itemA), "d" (itemB)
	: "r8", "r9", "cc"
	);
	return toRet;
}

bool MultiStringSuffixIndexSortOption_compMeth(void* unif, void* itemA, void* itemB){
	bool toRet;
	//inline assembly
	asm volatile (
		"xorq %%rax, %%rax\n"
		"movq (%%rcx), %%r8\n"
		"movq (%%rdx), %%r9\n"
		"bswapq %%r8\n"
		"bswapq %%r9\n"
		"subq %%r8, %%r9\n"
		"setg %%al\n"
		"jne endCSOTest%=\n"
		"movq 8(%%rcx), %%r8\n"
		"movq 8(%%rdx), %%r9\n"
		"bswapq %%r8\n"
		"bswapq %%r9\n"
		"subq %%r8, %%r9\n"
		"setg %%al\n"
		"endCSOTest%=:\n"
	: "=a" (toRet)
	: "c" (itemA), "d" (itemB)
	: "r8", "r9", "cc"
	);
	return toRet;
}

bool MultiStringSuffixRLPairSortOption_compMeth(void* unif, void* itemA, void* itemB){
	bool toRet;
	//inline assembly
	asm volatile (
		"xorq %%rax, %%rax\n"
		"movq 8(%%rcx), %%r8\n"
		"movq 8(%%rdx), %%r9\n"
		"bswapq %%r8\n"
		"bswapq %%r9\n"
		"subq %%r8, %%r9\n"
		"setg %%al\n"
	: "=a" (toRet)
	: "c" (itemA), "d" (itemB)
	: "r8", "r9", "cc"
	);
	return toRet;
}


