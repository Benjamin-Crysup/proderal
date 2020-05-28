#include "whodun_stringext.h"

/**
 * This will swap byte by byte.
 * @param arrA THe first array.
 * @param arrB The second array.
 * @param numBts The number of bytes to swap.
 */
void memswap_direct_byte(char* arrA, char* arrB, size_t numBts){
	//just swap byte by byte
	asm volatile(
		"memswap_byte_rep%=:\n"
		"movb (%%rax), %%r8b\n"
		"movb (%%rbx), %%r9b\n"
		"movb %%r8b, (%%rbx)\n"
		"movb %%r9b, (%%rax)\n"
		"addq $1, %%rax\n"
		"addq $1, %%rbx\n"
		"addq $-1, %%rcx\n"
		"jnz memswap_byte_rep%=\n"
	:
	: "a" (arrA), "b" (arrB), "c" (numBts)
	: "cc", "memory", "r8", "r9"
	);
}

/**
 * This will swap word by word.
 * @param arrA THe first array.
 * @param arrB The second array.
 * @param numWord The number of words to swap.
 */
void memswap_direct_word(char* arrA, char* arrB, size_t numWord){
	//just swap word by word
	asm volatile(
		"memswap_word_rep%=:\n"
		"movq (%%rax), %%r8\n"
		"movq (%%rbx), %%r9\n"
		"movq %%r8, (%%rbx)\n"
		"movq %%r9, (%%rax)\n"
		"addq $8, %%rax\n"
		"addq $8, %%rbx\n"
		"addq $-1, %%rcx\n"
		"jnz memswap_word_rep%=\n"
	:
	: "a" (arrA), "b" (arrB), "c" (numWord)
	: "cc", "memory", "r8", "r9"
	);
}

void memswap(char* arrA, char* arrB, size_t numBts){
	if(numBts == 0){
		return;
	}
	if(numBts < 8){
		memswap_direct_byte(arrA, arrB, numBts);
		return;
	}
	uintptr_t charAAddr = (uintptr_t)arrA;
	uintptr_t charBAddr = (uintptr_t)arrB;
	uintptr_t charAMa = charAAddr & 0x07;
	uintptr_t charBMa = charBAddr & 0x07;
	char* workA = arrA;
	char* workB = arrB;
	size_t leftBts = numBts;
	if(charAMa == charBMa){
		//try to align
		if(charAMa){
			charAMa = 8 - charAMa;
			memswap_direct_byte(workA, workB, charAMa);
			workA += charAMa;
			workB += charAMa;
			leftBts -= charAMa;
		}
	}
	//move the words
	size_t numWords = leftBts >> 3;
	if(numWords){
		memswap_direct_word(workA, workB, numWords);
		size_t tmpBytes = numWords << 3;
		workA += tmpBytes;
		workB += tmpBytes;
		leftBts -= tmpBytes;
	}
	//move the leftovers
	if(leftBts){
		memswap_direct_byte(workA, workB, leftBts);
	}
}

void canonPrepareInt64(uint_least64_t toPrep, char* toBuffer){
	asm volatile (
		"bswapq %%rcx\n"
		"movq %%rcx, (%%rdx)\n"
	:
	: "c" (toPrep), "d" (toBuffer)
	: "cc", "memory"
	);
}

uint_least64_t canonParseInt64(const char* toDebuffer){
	uint_least64_t toRet = 0;
	asm volatile (
		"movq (%%rcx), %%rax\n"
		"bswapq %%rax\n"
	: "=a" (toRet)
	: "c" (toDebuffer)
	: "cc"
	);
	return toRet;
}
