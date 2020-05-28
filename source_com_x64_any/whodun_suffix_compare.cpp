#include "whodun_suffix.h"
#include "whodun_suffix_private.h"

bool SingleStringSuffixRankSortOption::compMeth(void* itemA, void* itemB){
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

bool SingleStringSuffixIndexSortOption::compMeth(void* itemA, void* itemB){
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

bool SingleStringSuffixRLPairSortOption::compMeth(void* itemA, void* itemB){
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

bool MultiStringSuffixRankSortOption::compMeth(void* itemA, void* itemB){
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

bool MultiStringSuffixIndexSortOption::compMeth(void* itemA, void* itemB){
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

bool MultiStringSuffixRLPairSortOption::compMeth(void* itemA, void* itemB){
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