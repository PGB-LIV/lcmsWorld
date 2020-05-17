#pragma once
#include <string>
#include <algorithm>
#include <cctype>
#include <vector>
#ifdef _MSC_VER 
//not #if defined(_WIN32) || defined(_WIN64) because we have strncasecmp in mingw
#define strncasecmp _strnicmp
#define strcasecmp _stricmp
#define starts_with _Starts_with
#define ends_with _Ends_with


#endif
class Utils
{
public:
	Utils();
	~Utils();
	static int getNumberOfCores();
	static std::vector<std::string> split(const std::string& s, char delimiter);

};

#if 0
inline double doubleRand(double min, double max) {
	thread_local std::mt19937 generator(std::random_device{}());
	std::uniform_real_distribution<double> distribution(min, max);
	return distribution(generator);

}
#endif

inline int strcasecmp(const std::string& s1, const std::string& s2)
{
	return strcasecmp(s1.c_str(), s2.c_str());
}

std::string ReplaceAll(std::string str, const std::string& from, const std::string& to);
std::string tolower(std::string a);
std::string toupper(std::string a);

inline void ltrim(std::string &s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
		return !std::isspace(ch);
	}));
}

inline bool ReplaceString(std::string& str, const std::string& oldStr, const std::string& newStr)
{
	bool Finded = false;
	size_t pos = 0;
	while ((pos = str.find(oldStr, pos)) != std::string::npos) {
		Finded = true;
		str.replace(pos, oldStr.length(), newStr);
		pos += newStr.length();
	}
	return Finded;
}


inline std::string convertFilename(std::string input)
{

#ifdef _WIN32
	return input;
#endif
	std::string path = input;
	std::replace(path.begin(), path.end(), '\\', '/');
	ReplaceString(path, "//", "/");
	return path;
}

// trim from end (in place)
inline void rtrim(std::string &s) {
	s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
		return !std::isspace(ch);
	}).base(), s.end());
}

#ifdef _WIN32
const static std::string sep = "\\";
const static std::string base = ".";
const static char sepc = '\\';
#else
const static std::string sep = "/";
const static char sepc = '/';
const static std::string base = "/";
#endif