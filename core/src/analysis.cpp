
#include "analysis.h"
#include <sstream>
#include <iomanip>
#include <iostream>
#include <cxxabi.h>
#include <algorithm>
#include "mpastreamreader.h"
#include "util.h"

using namespace core;

Analysis::Analysis() :
	_options("Allowed options")
{
	_options.add_options()
		("help,h", "Show help message")
		("define,D", po::value<std::vector<std::string>>(),
		 "Add definition to configuration namespace. Uses the CfgParse file syntax")
		("config,c", po::value<std::string>()->default_value("../config.cfg"),
		 "Configuration file to load variables from")
		("run,r", po::value<std::vector<int>>()->required(), "MPA Run ID")
	;
}

bool Analysis::loadConfig(const po::variables_map& vm)
{
	_config.load(vm["config"].as<std::string>());
	if(vm.count("define")) {
		for(const auto& def: vm["define"].as<std::vector<std::string>>()) {
			_config.parse(def, std::string("Option ")+def);
		}
	}
	return true;
}

std::string Analysis::getUsage(const std::string& argv0) const
{
	std::ostringstream sstr;
	sstr << "Try '" << argv0 << " -h' for more information.";
	return sstr.str();
}

std::string Analysis::getHelp(const std::string& argv0) const
{
	return "";
}

std::string Analysis::getPaddedIdString(int id, unsigned int width)
{
	std::ostringstream sstr;
	sstr << std::setfill('0') << std::setw(width) << id;
	return sstr.str();
}

std::string Analysis::getMpaIdPadded(int id) const
{
	return getPaddedIdString(id, _config.get<unsigned int>("mpa_id_padding"));
}

std::string Analysis::getName() const
{
	int status;
        char* demangled = abi::__cxa_demangle(typeid(*this).name(), 0, 0, &status);
	std::string name(demangled);
	free(demangled);
       return name;
}

std::string Analysis::getFilename(const std::string& suffix) const
{
	return getFilename(getName(), suffix, true);
}

std::string Analysis::getFilename(const std::string& prefix, const std::string& suffix, bool extraPrefixes, bool allRuns) const
{
	std::ostringstream sstr;
	sstr << _config.getVariable("output_dir") << "/" << prefix;
	if(extraPrefixes) {
		try {
			auto prefix = _config.getVariable("output_prefix");
			if(prefix.size()) {
				sstr << "_" << prefix;
			}
		} catch(CfgParse::no_variable_error& e) {
		}
	}
	if(allRuns) {
		for(const auto& id: _allRunIds) {
			sstr << "_" << getMpaIdPadded(id);
		}
	} else {
		sstr << "_" << getMpaIdPadded(_currentRunId);
	}
	sstr << suffix;
       return sstr.str();
}

std::string Analysis::getFilename(const int& run_id, const std::string& suffix) const
{
	std::ostringstream sstr;
	sstr << _config.getVariable("output_dir") << "/" << getName()
	     << "_" << getMpaIdPadded(run_id) << suffix;
       return sstr.str();
}

std::string Analysis::getRootFilename(const std::string& suffix) const
{
	return getFilename(suffix)+".root";
}

