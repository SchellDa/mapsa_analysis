
#ifndef TRIM_H_
#define TRIM_H_

#include <cctype>
#include <string>
#include <algorithm>

inline std::string trim(const std::string &s)
{
	// http://stackoverflow.com/a/17976541
	auto wsfront=std::find_if_not(s.begin(),s.end(),[](int c){return std::isspace(c);});
	auto wsback=std::find_if_not(s.rbegin(),s.rend(),[](int c){return std::isspace(c);}).base();
	return (wsback<=wsfront ? std::string() : std::string(wsfront,wsback));
}

#endif//TRIM_H_
