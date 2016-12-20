
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
	_options("Allowed options"), _analysisRunning(false)
{
	_options.add_options()
		("help,h", "Show help message")
		("define,D", po::value<std::vector<std::string>>(),
		 "Add definition to configuration namespace. Uses the CfgParse file syntax")
		("config,c", po::value<std::string>()->default_value("../config.cfg"),
		 "Configuration file to load variables from")
		("runlist,l", po::value<std::string>()->default_value("../runlist.csv"), "Per-run information table")
		("run,r", po::value<std::vector<int>>()->required(), "MPA Run ID")
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
			auto runs = vm["run"].as<std::vector<int>>();
			int run_id = runs[0];
			int tel_run;
			if(vm.count("telescope")) {
				tel_run = run_id;
				run_id = _runlist.getMpaRunByTelRun(tel_run);
				for(const auto& tel: runs) {
					_allRunIds.push_back(_runlist.getMpaRunByTelRun(tel));
				}
			} else {
				_allRunIds = runs;
				tel_run = _runlist.getTelRunByMpaRun(run_id);
			}
			std::sort(_allRunIds.begin(), _allRunIds.end());
			_currentRunId = run_id;
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
	return true;
}

void Analysis::run(const po::variables_map& vm)
{
	init(vm);
	std::vector<run_read_pair_t> readers;
	for(auto runId: _allRunIds) {
		_config.setVariable("TelRun", getRunIdPadded(_runlist.getTelRunByMpaRun(runId)));
		_config.setVariable("MpaRun", getMpaIdPadded(runId));
		std::string reader_type("MPAStreamReader");
		try {
			reader_type = _config.getVariable("pixel_reader_type");
		} catch(CfgParse::no_variable_error& e) {
		} catch(std::out_of_range& e) {
			std::cout << "The specified pixel_reader_type " << reader_type << " is unknown!" << std::endl;
			throw;
		}
		auto reader = BaseSensorStreamReader::Factory::Instance()->createShared(reader_type);
		reader->setFilename(_config.getVariable("mapsa_data"));
		run_read_pair_t r {
			runId,
			reader,
			{_config.getVariable("track_data")}
		};

		try {
			r.pixelreader->begin();
		} catch(std::ios_base::failure& e) {
			std::cerr << "Cannot open MPA data file '" << _config.getVariable("mapsa_data") << "'." << std::endl;
			return;
		}
		try {
			r.trackreader.begin();
		} catch(std::ios_base::failure& e) {
			std::cerr << "Cannot open track data file '" << _config.getVariable("track_data") << "'." << std::endl;
			return;
		}
		readers.push_back(r);
	}
	for(const auto& process: _processes) {
		executeProcess(readers, process);
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

std::string Analysis::getMpaIdPadded(int id) const
{
	return getPaddedIdString(id, _config.get<unsigned int>("mpa_id_padding"));
}

std::string Analysis::getRunIdPadded(int id) const
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
	sstr << _config.getVariable("output_dir") << "/" << getName();
	try {
		auto prefix = _config.getVariable("output_prefix");
		if(prefix.size()) {
			sstr << "_" << prefix;
		}
	} catch(CfgParse::no_variable_error& e) {
	}
	for(const auto& id: _allRunIds) {
		sstr << "_" << getMpaIdPadded(id);
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

bool Analysis::multirunConsistencyCheck(const std::string& argv0, const po::variables_map& vm)
{
	const auto runs = vm["run"].as<std::vector<int>>();
	assert(runs.size() > 0);
	if(runs.size() == 1) {
		return true;
	}
	int firstRunId = runs[0];
	if(vm.count("telescope")) {
		firstRunId = _runlist.getMpaRunByTelRun(firstRunId);
	}
	std::cout << "Run ID " << firstRunId << std::endl;
	auto firstRun = _runlist.getByMpaRun(firstRunId);
	std::vector<int> additionalMismatches;
	std::vector<int> excludeRuns;
	bool acceptAll = false;
	for(auto runId: runs) {
		if(vm.count("telescope")) {
			runId = _runlist.getMpaRunByTelRun(runId);
		}
		auto run = _runlist.getByMpaRun(runId);
		bool mismatch =
			firstRun.angle != run.angle ||
			firstRun.bias_voltage != run.bias_voltage ||
			firstRun.bias_current != run.bias_current ||
			firstRun.threshold != run.threshold;
		if(acceptAll && mismatch) {
			additionalMismatches.push_back(runId);
		} else if(mismatch) {
			additionalMismatches.push_back(runId);
			std::cout << argv0  << ": Mismatch between MPA runs " << firstRunId << " and "
			          << runId << "\n" << std::setfill(' ')
				  << std::setw(20) << "Run: " << std::setw(10) << firstRun.mpa_run << " " << run.mpa_run << "\n"
				  << std::setw(20) << "Angle: " << std::setw(10) << firstRun.angle << " " << run.angle << "\n"
				  << std::setw(20) << "Bias Voltage: " << std::setw(10) << firstRun.bias_voltage << " " << run.bias_voltage << "\n"
				  << std::setw(20) << "Bias Current: " << std::setw(10) << firstRun.bias_current << " " << run.bias_current << "\n"
				  << std::setw(20) << "Threshold: " << std::setw(10) << firstRun.threshold << " " << run.threshold  << "\n\n" << std::flush;
			do {
				std::cout << "Use run " << runId << " for analysis anyway? (y)es/(N)o/(a)ll "
					  << std::flush;
				std::string line;
				std::getline(std::cin, line);
				if(line.size() == 0) {
					line = "n";
				}
				char ch = line[0];
				if(ch == 'y' || ch == 'Y') {
					break;
				} else if (ch == 'n' || ch == 'N') {
					excludeRuns.push_back(runId);
					break;
				} else if (ch == 'a' || ch == 'A') {
					acceptAll = true;
					break;
				}
			} while(true);
		}
	}
	for(const auto& exclude: excludeRuns) {
		_allRunIds.erase(std::remove(_allRunIds.begin(), _allRunIds.end(), exclude), _allRunIds.end());
		additionalMismatches.erase(std::remove(additionalMismatches.begin(), additionalMismatches.end(), exclude), additionalMismatches.end());
	}
	if(additionalMismatches.size() && acceptAll) {
		std::cout << "Runlist arguments mismatches between MPA runs \n * " << firstRunId << "\n";
		for(const auto& runId: additionalMismatches) {
			std::cout << " * " << runId << "\n";
		}
		std::cout << std::flush;
	}
	return true;
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
	assert(_analysisRunning == false);
	_rerunProcess = true;
}

void Analysis::executeProcess(const std::vector<Analysis::run_read_pair_t>& reader, const process_t& process)
{
	_rerunNumber = 0;
	do {
		if(process.init) {
			process.init();
		}
		_rerunProcess = false;
		size_t evtCount = 0;
		if(process.mode == CS_ALWAYS && process.run) {
			for(const auto& read: reader) {
				_currentRunId = read.runId;
				_config.setVariable("TelRun", getRunIdPadded(_runlist.getTelRunByMpaRun(_currentRunId)));
				_config.setVariable("MpaRun", getMpaIdPadded(_currentRunId));
				if(process.run_init) {
					process.run_init();
				}
				_analysisRunning = true;
				evtCount = 0;
				auto track_it = read.trackreader.begin();
				for(const auto& pixel: *read.pixelreader) {
					if(track_it->eventNumber < (int)pixel.eventNumber + _dataOffset)
						++track_it;
					if(evtCount % 1000 == 0) {
						std::cout << process.name << ": Processing step " << evtCount;
						if(_rerunNumber)
							std::cout << " rerun " << _rerunNumber;
						std::cout << " for MPA run " << read.runId << std::endl;
					}
					++evtCount;
					if(!process.run(*track_it, pixel))
						break;
				}
				_analysisRunning = false;
				if(process.run_post) {
					process.run_post();
				}
			}
		} else if (process.run) {
			for(const auto& read: reader) {
				_currentRunId = read.runId;
				_config.setVariable("TelRun", getRunIdPadded(_runlist.getTelRunByMpaRun(_currentRunId)));
				_config.setVariable("MpaRun", getMpaIdPadded(_currentRunId));
				if(process.run_init) {
					process.run_init();
				}
				_analysisRunning = true;
				evtCount = 0;
				auto pixel_it = read.pixelreader->begin();
				for(const auto& track: read.trackreader) {
					while((int)pixel_it->eventNumber + _dataOffset < track.eventNumber &&
					      pixel_it != read.pixelreader->end())
						++pixel_it;
					if((int)pixel_it->eventNumber + _dataOffset > track.eventNumber) {
						continue;
					}
					if(evtCount % 1000 == 0) {
						std::cout << process.name <<  ": Processing step " << evtCount;
						if(_rerunNumber)
							std::cout << " rerun " << _rerunNumber;
						std::cout << " event no. " << track.eventNumber << "/" << pixel_it->eventNumber;
						std::cout << " for MPA run " << _currentRunId << std::endl;
					}
					++evtCount;
					if(pixel_it == read.pixelreader->end())
						break;
					assert((int)pixel_it->eventNumber + _dataOffset == track.eventNumber);
					if(!process.run(track, *pixel_it))
						break;
				}
				_analysisRunning = false;
				if(process.run_post) {
					process.run_post();
				}
			}
		}
		if(process.post) {
			process.post();
		}
		++_rerunNumber;
	} while(_rerunProcess);
}
