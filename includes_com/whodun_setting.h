#ifndef WHODUN_SETTING_H
#define WHODUN_SETTING_H 1

#include <vector>

#include "whodun_datread.h"

/**Load an ini file.*/
class LoadedSettings{
public:
	/**
	 * Load some settings.
	 * @param loadFrom The place to load from.
	 */
	LoadedSettings(InStream* loadFrom);
	
	/**The loaded entries*/
	std::vector<char*> entryStore;
	/**Raw storage for the bytes.*/
	std::vector<char> byteStore;
};

#endif
