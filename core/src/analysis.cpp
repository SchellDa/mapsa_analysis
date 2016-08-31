
#include "analysis.h"
#include <sstream>
#include <iomanip>
#include <cxxabi.h>

using namespace core;

Analysis::Analysis() :
	_options("Allowed options"), _analysisRunning(false)
{
	_options.add_options()
		("help,h", "Show help message")
		("define,D", po::value<std::vector<std::string>>(),
		 "Add definition to configuration namespace. Uses the CfgParse file syntax")
		("config,c", po::value<std::string>()->default_value("../config.cfg"),
		 "Configuration file to load variables from")
		("runlist,l", po::value<std::string>()->default_value("../runlist.csv"), "Per-run information table")
		("run,r", po::value<int>()->required(), "MPA Run ID")
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
	try {
		if(vm.count("runlist")) {
			_runlist.read(vm["runlist"].as<std::string>());
			auto run_id = vm["run"].as<int>();
			auto tel_run = _runlist.getTelRunByMpaRun(run_id);
			_config.setVariable("MpaRun", getMpaIdPadded(run_id));
			_config.setVariable("TelRun", getRunIdPadded(tel_run));
		}
	} catch(std::ios_base::failure& e) {
		std::cerr << "Runlist file not found!" << std::endl;
		return false;
	} catch(std::exception& e) {
		std::cerr << "Cannot parse runlist file: " << e.what() << std::endl;
		return false;
	}
	_mpaTransform.setSensitiveSize({_config.get<double>("mpa_size_x"),
	                               _config.get<double>("mpa_size_y")});
	_mpaTransform.setPixelSize({_config.get<double>("mpa_pixel_size_x"),
	                           _config.get<double>("mpa_pixel_size_y")});
	_mpaTransform.setNumPixels({_config.get<size_t>("mpa_num_pixels_x"),
	                           _config.get<size_t>("mpa_num_pixels_y")});
	return true;
}

void Analysis::run(const po::variables_map& vm)
{
	if(!loadConfig(vm))
		return;
	init(vm);
	core::MPAStreamReader mpareader(_config.getVariable("mapsa_data"));
	core::TrackStreamReader trackreader(_config.getVariable("track_data"));
	_analysisRunning = true;
	try {
		mpareader.begin();
	} catch(std::ios_base::failure& e) {
		std::cerr << "Cannot open MPA data file '" << _config.getVariable("mapsa_data") << "'." << std::endl;
		return;
	}
	try {
		trackreader.begin();
	} catch(std::ios_base::failure& e) {
		std::cerr << "Cannot open track data file '" << _config.getVariable("track_data") << "'." << std::endl;
		return;
	}
	for(const auto& process: _processes) {
		size_t evtCount = 0;
		if(process.mode == CS_ALWAYS && process.run) {
			auto track_it = trackreader.begin();
			for(const auto& mpa: mpareader) {
				if(track_it->eventNumber < mpa.eventNumber + _dataOffset)
					++track_it;
				if(evtCount % 1000 == 0) {
					std::cout << "Processing event " << evtCount << std::endl;
				}
				++evtCount;
				if(!process.run(*track_it, mpa))
					break;
			}
		} else if (process.run) {
			auto mpa_it = mpareader.begin();
			for(const auto& track: trackreader) {
				while(mpa_it->eventNumber + _dataOffset != track.eventNumber &&
				      mpa_it != mpareader.end())
					++mpa_it;
				if(evtCount % 1000 == 0) {
					std::cout << "Processing event " << evtCount << std::endl;
				}
				++evtCount;
				if(mpa_it == mpareader.end())
					break;
				if(!process.run(track, *mpa_it))
					break;
			}
		}
		if(process.post) {
			process.post();
		}
	}
	_analysisRunning = false;
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

std::string Analysis::getPaddedIdString(int id, unsigned int width)
{
	std::ostringstream sstr;
	sstr << std::setfill('0') << std::setw(width) << id;
	return sstr.str();
}

std::string Analysis::getMpaIdPadded(int id)
{
	return getPaddedIdString(id, _config.get<unsigned int>("mpa_id_padding"));
}

std::string Analysis::getRunIdPadded(int id)
{
	return getPaddedIdString(id, _config.get<unsigned int>("run_id_padding"));
}

std::string Analysis::getRootFilename() const
{
	std::ostringstream sstr;
	int status;
        char* demangled = abi::__cxa_demangle(typeid(*this).name(), 0, 0, &status);
	sstr << demangled << "_" << _config.getVariable("MpaRun") << ".root";
	free(demangled);
       return sstr.str();
}

void Analysis::addProcess(const process_t& proc)
{
	_processes.push_back(proc);
}

void Analysis::setDataOffset(int dataOffset)
{
	assert(!_analysisRunning);
	_dataOffset = dataOffset;
}

