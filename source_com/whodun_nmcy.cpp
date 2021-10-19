#include "whodun_nmcy.h"

#include <string.h>
#include <algorithm>

bool pointerStringMapCompare(std::pair<uintptr_t,char*> strA, std::pair<uintptr_t,char*> strB){
	uintptr_t compTo = std::min(strA.first, strB.first);
	int compV = memcmp(strA.second, strB.second, compTo);
	if(compV){ return compV < 0; }
	return (strA.first < strB.first);
}



