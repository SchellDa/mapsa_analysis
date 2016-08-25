
#include "analysis.h"
#include <sstream>

using namespace core;

Analysis::Analysis() :
	_options("Allowed options")
{
	_options.add_options()
		("help,h", "Show help message")
	;
}

std::string Analysis::getUsage(const std::string& argv0) const
{
	std::ostringstream sstr;
	sstr << "Try '" << argv0 << " -h' for more information.";
	return sstr.str();
	return sstr.str();;
}

std::string Analysis::getHelp(const std::string& argv0) const
{
	return "";
}

