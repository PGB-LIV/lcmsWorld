#include "Utils.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <cstdlib>  

#ifdef _WIN32
#include <windows.h>
#elif MACOS
#include <sys/param.h>
#include <sys/sysctl.h>
#else
#include <unistd.h>
#endif
#include <regex>
#include "Structs.h"

int Utils::getNumberOfCores() {
#ifdef WIN32
	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);
	return sysinfo.dwNumberOfProcessors;
#elif MACOS
	int nm[2];
	size_t len = 4;
	uint32_t count;

	nm[0] = CTL_HW; nm[1] = HW_AVAILCPU;
	sysctl(nm, 2, &count, &len, NULL, 0);

	if (count < 1) {
		nm[1] = HW_NCPU;
		sysctl(nm, 2, &count, &len, NULL, 0);
		if (count < 1) { count = 1; }
	}
	return count;
#else
	return sysconf(_SC_NPROCESSORS_ONLN);
#endif
}


Utils::Utils()
{
}


Utils::~Utils()
{
}

std::string ReplaceAll(std::string str, const std::string& from, const std::string& to) 
{

	std::regex re("("+from+")");
	std::string l = str;
	std::string l1 = "";
	while (l != l1)
	{
		l1 = l;
		l = std::regex_replace(l, re, to);
	}

	return l;
}

std::string tolower(std::string a)
{
	std::string af = a;
	std::transform(af.begin(), af.end(), af.begin(), static_cast<int(*)(int)>(tolower));
	return af;
}
std::string toupper(std::string a)
{
	std::string af = a;
	std::transform(af.begin(), af.end(), af.begin(), static_cast<int(*)(int)>(toupper));
	return af;
}
 
std::vector<std::string> csplit(const std::string& input, const std::string& regexs) {
	// passing -1 as the submatch index parameter performs splitting
	std::regex re(regexs);
	std::sregex_token_iterator
		first{ input.begin(), input.end(), re, -1 },
		last;
	return { first, last };
}


std::vector<std::string> Utils::split(const std::string& s, char delimiter)
{
	std::vector<std::string> tokens;
	std::string token;
	std::istringstream tokenStream(s);
 
	 
		
		std::stringstream ss;
	
		std::string l = s;

 //the below code doesn't work with whitespaces
 
		if (delimiter == '\t')
			return(csplit(s,std::string(1,delimiter)));


		ss << l;
		int col = 0;
		while (ss >> std::ws) {
			std::string csvElement;

			if (ss.peek() == '"')  {
				ss >> std::quoted(csvElement);
				std::string discard;
				std::getline(ss, discard, delimiter);
			}
			else {
				std::getline(ss, csvElement, delimiter);
			}
 
			tokens.push_back(csvElement);
		 
		}
	return tokens;
}


