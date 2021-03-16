#ifndef WHODUN_ARGS_H
#define WHODUN_ARGS_H 1

#include <map>
#include <set>
#include <string>
#include <vector>
#include <iostream>

/**Handle loading arguments from file.*/
class ArgumentRedirectParser{
public:
	/**
	 * Load in any redirect arguments.
	 * @param argc The starting number of arguments.
	 * @param argv The starting arguments.
	 */
	ArgumentRedirectParser(int argc, char** argv);
	/**Clean up.*/
	~ArgumentRedirectParser();
	/**The text of all the arguments.*/
	std::vector<char> allText;
	/**The arguments themselves.*/
	std::vector<char*> allArgs;
};

/**Meta information for a boolean flag.*/
class ArgumentParserBoolMeta{
public:
	/**The name to use for presentation.*/
	std::string presentName;
	/**Don't show this option.*/
	bool hidden;
	/**Set up empty metadata.*/
	ArgumentParserBoolMeta();
	/**
	 * Set a name.
	 * @param presName The presentation name.
	 */
	ArgumentParserBoolMeta(const char* presName);
	/**Copy constructor*/
	ArgumentParserBoolMeta(const ArgumentParserBoolMeta& toCopy);
	/**Copy assignment.*/
	ArgumentParserBoolMeta& operator=(const ArgumentParserBoolMeta& toCopy);
};

/**Meta information for an enumeration.*/
class ArgumentParserEnumMeta{
public:
	/**The name to use for presentation.*/
	std::string presentName;
	/**Don't show this option.*/
	bool hidden;
	/**The group of enum options this is with: used to build a radio set.*/
	std::string enumGroup;
	/**Set up empty metadata.*/
	ArgumentParserEnumMeta();
	/**
	 * Set up empty metadata
	 * @param presName The presentation name.
	 */
	ArgumentParserEnumMeta(const char* presName);
	/**Copy constructor*/
	ArgumentParserEnumMeta(const ArgumentParserEnumMeta& toCopy);
	/**Copy assignment.*/
	ArgumentParserEnumMeta& operator=(const ArgumentParserEnumMeta& toCopy);
};

/**Meta information for an integer.*/
class ArgumentParserIntMeta{
public:
	/**The name to use for presentation.*/
	std::string presentName;
	/**Don't show this option.*/
	bool hidden;
	/**Set up empty metadata.*/
	ArgumentParserIntMeta();
	/**
	 * Set up empty metadata
	 * @param presName The presentation name.
	 */
	ArgumentParserIntMeta(const char* presName);
	/**Copy constructor*/
	ArgumentParserIntMeta(const ArgumentParserIntMeta& toCopy);
	/**Copy assignment.*/
	ArgumentParserIntMeta& operator=(const ArgumentParserIntMeta& toCopy);
};

/**Meta information for a float.*/
class ArgumentParserFltMeta{
public:
	/**The name to use for presentation.*/
	std::string presentName;
	/**Don't show this option.*/
	bool hidden;
	/**Set up empty metadata.*/
	ArgumentParserFltMeta();
	/**
	 * Set up empty metadata
	 * @param presName The presentation name.
	 */
	ArgumentParserFltMeta(const char* presName);
	/**Copy constructor*/
	ArgumentParserFltMeta(const ArgumentParserFltMeta& toCopy);
	/**Copy assignment.*/
	ArgumentParserFltMeta& operator=(const ArgumentParserFltMeta& toCopy);
};

/**Meta information for a string.*/
class ArgumentParserStrMeta{
public:
	/**The name to use for presentation.*/
	std::string presentName;
	/**Don't show this option.*/
	bool hidden;
	
	/**Whether this string is actually a file.*/
	bool isFile;
	/**Whether this file will be written.*/
	bool fileWrite;
	/**Extensions to limit to, if any.*/
	std::set<std::string> fileExts;
	
	/**Whether this string is actually a folder.*/
	bool isFolder;
	/**Whether this folder will be written.*/
	bool foldWrite;
	
	/**Set up empty metadata.*/
	ArgumentParserStrMeta();
	/**
	 * Set up empty metadata
	 * @param presName The presentation name.
	 */
	ArgumentParserStrMeta(const char* presName);
	/**Copy constructor*/
	ArgumentParserStrMeta(const ArgumentParserStrMeta& toCopy);
	/**Copy assignment.*/
	ArgumentParserStrMeta& operator=(const ArgumentParserStrMeta& toCopy);
};

/**Meta information for an integer vector.*/
class ArgumentParserIntVecMeta{
public:
	/**The name to use for presentation.*/
	std::string presentName;
	/**Don't show this option.*/
	bool hidden;
	/**Set up empty metadata.*/
	ArgumentParserIntVecMeta();
	/**
	 * Set up empty metadata
	 * @param presName The presentation name.
	 */
	ArgumentParserIntVecMeta(const char* presName);
	/**Copy constructor*/
	ArgumentParserIntVecMeta(const ArgumentParserIntVecMeta& toCopy);
	/**Copy assignment.*/
	ArgumentParserIntVecMeta& operator=(const ArgumentParserIntVecMeta& toCopy);
};

/**Meta information for a float vector.*/
class ArgumentParserFltVecMeta{
public:
	/**The name to use for presentation.*/
	std::string presentName;
	/**Don't show this option.*/
	bool hidden;
	/**Set up empty metadata.*/
	ArgumentParserFltVecMeta();
	/**
	 * Set up empty metadata
	 * @param presName The presentation name.
	 */
	ArgumentParserFltVecMeta(const char* presName);
	/**Copy constructor*/
	ArgumentParserFltVecMeta(const ArgumentParserFltVecMeta& toCopy);
	/**Copy assignment.*/
	ArgumentParserFltVecMeta& operator=(const ArgumentParserFltVecMeta& toCopy);
};

/**Meta information for a string vector.*/
class ArgumentParserStrVecMeta{
public:
	/**The name to use for presentation.*/
	std::string presentName;
	/**Don't show this option.*/
	bool hidden;
	
	/**Whether this string is actually a file.*/
	bool isFile;
	/**Whether this file will be written.*/
	bool fileWrite;
	/**Extensions to limit to, if any.*/
	std::set<std::string> fileExts;
	
	/**Whether this string is actually a folder.*/
	bool isFolder;
	/**Whether this folder will be written.*/
	bool foldWrite;
	
	/**Set up empty metadata.*/
	ArgumentParserStrVecMeta();
	/**
	 * Set up empty metadata
	 * @param presName The presentation name.
	 */
	ArgumentParserStrVecMeta(const char* presName);
	/**Copy constructor*/
	ArgumentParserStrVecMeta(const ArgumentParserStrVecMeta& toCopy);
	/**Copy assignment.*/
	ArgumentParserStrVecMeta& operator=(const ArgumentParserStrVecMeta& toCopy);
};

/**Parse arguments.*/
class ArgumentParser{
public:
	/**Basic setup.*/
	ArgumentParser();
	/**Subclassable teardown.*/
	virtual ~ArgumentParser();
	
	/**
	 * Parse arguments.
	 * @param argc The total number of arguments.
	 * @param argv The arguments.
	 * @param helpOut The place to write help to.
	 * @return The number of consumed arguments: negative for error.
	 */
	int parseArguments(int argc, char** argv, std::ostream* helpOut);
	/**
	 * Parse arguments.
	 * @param loadArgs The loaded arguments.
	 * @param helpOut The place to write help to.
	 * @return The number of consumed arguments: negative for error.
	 */
	int parseArguments(ArgumentRedirectParser* loadArgs, std::ostream* helpOut);
	
	/**Whether the program should try to run.*/
	int needRun;
	/**Whether there was an error (and error message).*/
	std::string argumentError;
	
	/**
	 * Print out the help documentation for this program.
	 * @param toPrint The place to print to.
	 */
	void printHelpDocumentation(std::ostream* toPrint);
	/**The main documentation for the program.*/
	const char* myMainDoc;
	/**Documentation for each argument.*/
	std::map<std::string,const char*> argDocs;
	
	/**
	 * Print out the version information for this program.
	 * @param toPrint The place to print to.
	 */
	void printVersionInformation(std::ostream* toPrint);
	/**The version of this software.*/
	const char* myVersionDoc;
	/**The copyright date of this software.*/
	const char* myCopyrightDoc;
	/**The license information. The default is LGPL.*/
	const char* myLicenseDoc;
	/**Warranty disclaimer.*/
	const char* myWarrantyDoc;
	
	/**
	 * Print information for building a gui.
	 * @param toPrint The place to write to.
	 */
	void printGUIInformation(std::ostream* toPrint);
	/**
	 * Print any extra information.
	 * @param toPrint The place to write to.
	 */
	virtual void printExtraGUIInformation(std::ostream* toPrint);
	/**The order the arguments were added in.*/
	std::vector<std::string> argOrder;
	
	/**
	 * Allow subclasses to do special things with an unknown argument.
	 * @param argc The total number of arguments.
	 * @param argv The arguments.
	 * @return The number of consumed arguments: negative for error.
	 */
	virtual int handleUnknownArgument(int argc, char** argv, std::ostream* helpOut);
	
	/**Flag arguments.*/
	std::map<std::string,bool*> boolHotFlags;
	/**Inverted flag arguments.*/
	std::map<std::string,bool*> boolCoolFlags;
	/**Meta information for flags.*/
	std::map<std::string,ArgumentParserBoolMeta> boolMeta;
	
	/**
	 * Add a boolean flag.
	 * @param flagName The name of the flag.
	 * @param flagVal The place to set.
	 * @param isHot Whether the flag will set or unset.
	 * @param argDoc The documentation for the argument.
	 * @param argMeta Any special type information, if any.
	 */
	void addBooleanFlag(const char* flagName, bool* flagVal, int isHot, const char* argDoc, ArgumentParserBoolMeta* argMeta);
	
	/**Enumeration settings.*/
	std::map<std::string,intptr_t*> enumSettings;
	/**The values for each flag.*/
	std::map<std::string,intptr_t> enumPossible;
	/**Meta information for enums.*/
	std::map<std::string,ArgumentParserEnumMeta> enumMeta;
	
	/**
	 * Add an enumeration.
	 * @param flagName The name of the flag.
	 * @param flagVal The place to set.
	 * @param flagSet The value to set to.
	 * @param argDoc The documentation for the argument.
	 * @param argMeta Any special type information, if any.
	 */
	void addEnumerationFlag(const char* flagName, intptr_t* flagVal, intptr_t flagSet, const char* argDoc, ArgumentParserEnumMeta* argMeta);
	
	/**The places to set.*/
	std::map<std::string,intptr_t*> intSettings;
	/**Basic checking functions.*/
	std::map<std::string,const char*(*)(intptr_t)> intIdjits;
	/**Meta information for integer.*/
	std::map<std::string,ArgumentParserIntMeta> intMeta;
	/**
	 * Add an integer option.
	 * @param optName The name of the option.
	 * @param optVal The place to set.
	 * @param optCheck The idiot check to use, or null.
	 * @param argDoc The documentation for the argument.
	 * @param argMeta Any special type information, if any.
	 */
	void addIntegerOption(const char* optName, intptr_t* optVal, const char* (*optCheck)(intptr_t), const char* argDoc, ArgumentParserIntMeta* argMeta);
	
	/**The places to set.*/
	std::map<std::string,double*> floatSettings;
	/**Basic checking functions.*/
	std::map<std::string,const char*(*)(double)> floatIdjits;
	/**Meta information for floats.*/
	std::map<std::string,ArgumentParserFltMeta> floatMeta;
	/**
	 * Add a float option.
	 * @param optName The name of the option.
	 * @param optVal The place to set.
	 * @param optCheck The idiot check to use, or null.
	 * @param argDoc The documentation for the argument.
	 * @param argMeta Any special type information, if any.
	 */
	void addFloatOption(const char* optName, double* optVal, const char* (*optCheck)(double), const char* argDoc, ArgumentParserFltMeta* argMeta);
	
	/**String options.*/
	std::map<std::string,char**> stringSettings;
	/**Idiot checks for said options.*/
	std::map<std::string,const char*(*)(char*)> stringIdjits;
	/**Meta information for strings.*/
	std::map<std::string,ArgumentParserStrMeta> stringMeta;
	/**
	 * Add a string option.
	 * @param optName The name of the option.
	 * @param optVal The place to set.
	 * @param optCheck The idiot check to use, or null.
	 * @param argDoc The documentation for the argument.
	 * @param argMeta Any special type information, if any.
	 */
	void addStringOption(const char* optName, char** optVal, const char* (*optCheck)(char*), const char* argDoc, ArgumentParserStrMeta* argMeta);
	
	/**The places to set.*/
	std::map<std::string,std::vector<intptr_t>*> intVecSettings;
	/**Basic checking functions.*/
	std::map<std::string,const char*(*)(intptr_t)> intVecIdjits;
	/**Meta information for integer.*/
	std::map<std::string,ArgumentParserIntVecMeta> intVecMeta;
	
	/**
	 * Add an integer option.
	 * @param optName The name of the option.
	 * @param optVal The place to set.
	 * @param optCheck The idiot check to use, or null.
	 * @param argDoc The documentation for the argument.
	 * @param argMeta Any special type information, if any.
	 */
	void addIntegerVectorOption(const char* optName, std::vector<intptr_t>* optVal, const char* (*optCheck)(intptr_t), const char* argDoc, ArgumentParserIntVecMeta* argMeta);
	
	/**The places to set.*/
	std::map<std::string,std::vector<double>*> floatVecSettings;
	/**Basic checking functions.*/
	std::map<std::string,const char*(*)(double)> floatVecIdjits;
	/**Meta information for floats.*/
	std::map<std::string,ArgumentParserFltVecMeta> floatVecMeta;
	
	/**
	 * Add a float option.
	 * @param optName The name of the option.
	 * @param optVal The place to set.
	 * @param optCheck The idiot check to use, or null.
	 * @param argDoc The documentation for the argument.
	 * @param argMeta Any special type information, if any.
	 */
	void addFloatVectorOption(const char* optName, std::vector<double>* optVal, const char* (*optCheck)(double), const char* argDoc, ArgumentParserFltVecMeta* argMeta);
	
	/**String options.*/
	std::map<std::string,std::vector<char*>*> stringVecSettings;
	/**Idiot checks for said options.*/
	std::map<std::string,const char*(*)(char*)> stringVecIdjits;
	/**Meta information for strings.*/
	std::map<std::string,ArgumentParserStrVecMeta> stringVecMeta;
	
	/**
	 * Add a string option.
	 * @param optName The name of the option.
	 * @param optVal The place to set.
	 * @param optCheck The idiot check to use, or null.
	 * @param argDoc The documentation for the argument.
	 * @param argMeta Any special type information, if any.
	 */
	void addStringVectorOption(const char* optName, std::vector<char*>* optVal, const char* (*optCheck)(char*), const char* argDoc, ArgumentParserStrVecMeta* argMeta);
	
	/**
	 * A final check after the arguments have been passed through.
	 * @return Whether there is a problem.
	 */
	virtual int posteriorCheck();
};

#endif