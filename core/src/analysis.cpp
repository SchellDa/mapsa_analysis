
#include "analysis.h"
#include <sstream>
#include <iomanip>
#include <iostream>
#include <cxxabi.h>
#include "mpastreamreader.h"

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
		("telescope,t", "The number specified by --run is a telescope run ID")
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
			int run_id = vm["run"].as<int>();
			int tel_run;
			if(vm.count("telescope")) {
				tel_run = run_id;
				run_id = _runlist.getMpaRunByTelRun(tel_run);
			} else {
				tel_run = _runlist.getTelRunByMpaRun(run_id);
			}
			_config.setVariable("MpaRun", getMpaIdPadded(run_id));
			_config.setVariable("TelRun", getRunIdPadded(tel_run));
			setDataOffset(_runlist.getByMpaRun(run_id).data_offset);
		}
	} catch(std::ios_base::failure& e) {
		std::cerr << "Runlist file not found!" << std::endl;
		return false;
	} catch(std::exception& e) {
		std::cerr << "Cannot parse runlist file: " << e.what() << std::endl;
		return false;
	}
	_mpaTransform.setOffset({0.0, 0.0, _config.get<double>("mpa_z_offset")});
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
		executeProcess(mpareader, trackreader, process);
	}
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
	std::ostringstream sstr;
	sstr << _config.getVariable("output_dir") << "/" << getName() << "_" << _config.getVariable("MpaRun") << suffix;
       return sstr.str();
}

std::string Analysis::getRootFilename(const std::string& suffix) const
{
	return getFilename(suffix)+".root";
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

void Analysis::rerun()
{
	_rerunProcess = true;
}

void Analysis::executeProcess(core::BaseSensorStreamReader& pixelreader,
		core::TrackStreamReader& trackreader, const process_t& process)
{
	int run = 0;
	do {
		_analysisRunning = true;
		_rerunProcess = false;
		size_t evtCount = 0;
		if(process.mode == CS_ALWAYS && process.run) {
			auto track_it = trackreader.begin();
			for(const auto& pixel: pixelreader) {
				if(track_it->eventNumber < (int)pixel.eventNumber + _dataOffset)
					++track_it;
				if(evtCount % 1000 == 0) {
					std::cout << process.name << ": Processing step " << evtCount;
					if(run)
						std::cout << " rerun " << run;
					std::cout << std::endl;
				}
				++evtCount;
				if(!process.run(*track_it, pixel))
					break;
			}
		} else if (process.run) {
			auto pixel_it = pixelreader.begin();
			for(const auto& track: trackreader) {
				while((int)pixel_it->eventNumber + _dataOffset < track.eventNumber &&
				      pixel_it != pixelreader.end())
					++pixel_it;
				if((int)pixel_it->eventNumber + _dataOffset > track.eventNumber) {
					continue;
				}
				if(evtCount % 1000 == 0) {
					std::cout << process.name <<  ": Processing step " << evtCount;
					if(run)
						std::cout << " rerun " << run;
					std::cout << " event no. " << track.eventNumber << "/" << pixel_it->eventNumber;
					std::cout << std::endl;
				}
				++evtCount;
				if(pixel_it == pixelreader.end())
					break;
				assert((int)pixel_it->eventNumber + _dataOffset == track.eventNumber);
				if(!process.run(track, *pixel_it))
					break;
			}
		}
		_analysisRunning = false;
		if(process.post) {
			process.post();
		}
		++run;
	} while(_rerunProcess);
}
