
#include "mergedanalysis.h"
#include <sstream>
#include <iostream>

using namespace core;

MergedAnalysis::MergedAnalysis() :
 Analysis(), _runlist(_config)
{
	getOptionsDescription().add_options()
		("runlist,l", po::value<std::string>(), "Per-run information table")
	;
}

MergedAnalysis::~MergedAnalysis()
{
	for(const auto& data: _runData) {
		data.file->Close();
	}
}

void MergedAnalysis::init(const po::variables_map& vm)
{
	auto runs = vm["run"].as<std::vector<int>>();
	_allRunIds = runs;
	std::cout << "Init system" << std::endl;
	for(auto runId: runs) {
		run_data_t data { runId, nullptr, nullptr, nullptr };
		_currentRunId = runId;
		_config.setVariable("MpaRun", getMpaIdPadded(runId));
		auto filename = _config.getVariable("testbeam_data");
		data.file = new TFile(filename.c_str(), "readonly");
		if(!data.file || data.file->IsZombie()) {
			std::ostringstream sstr;
			sstr << "Cannot open ROOT file '" << filename << "' for run " << runId;
			throw std::runtime_error(sstr.str().c_str());
		}
		data.file->GetObject("data", data.tree);
		if(!data.tree) {
			std::ostringstream sstr;
			sstr << "Cannot find data tree in ROOT file '" << filename << "' for run " << runId;
			throw std::runtime_error(sstr.str().c_str());
		}
		data.telescopeData = new TelescopeData*;
		*data.telescopeData = nullptr;
		data.telescopeHits = new TelescopeHits*;
		*data.telescopeHits = nullptr;
		data.tree->SetBranchAddress("telescope", data.telescopeData);
		data.tree->SetBranchAddress("telhits", data.telescopeHits);
		assert(*data.telescopeData != nullptr);
		assert(*data.telescopeHits != nullptr);
		for(int mpa = 1; mpa <= 6; ++mpa) {
			std::ostringstream name;
			name << "mpa_" << mpa;
			if(data.tree->FindBranch(name.str().c_str())) {
				mpa_data_t mpaData { name.str(), mpa, new MpaData* };
				*(mpaData.data) = nullptr;
				data.mpaData.push_back(mpaData);
			}
		}
		for(const auto& mpaData: data.mpaData) {
			data.tree->SetBranchAddress(mpaData.name.c_str(), mpaData.data);
			assert(mpaData.data != nullptr);
		}
		_runData.push_back(data);
	}
	if(vm.count("runlist")) {
		try {
			_runlist.load(vm["runlist"].as<std::string>());
		} catch(std::exception& e) {
			std::cerr << "Error while loading runlist:\n" << e.what() << std::endl;
			throw;
		}
	}
}

void MergedAnalysis::run(const po::variables_map& vm)
{
	init(vm);
	_currentRunId = _runData[0].runId;
	_config.setVariable("MpaRun", getMpaIdPadded(_currentRunId));
	_runlist.loadRun();
	init();
	for(const auto& data: _runData) {
		_currentRunId = data.runId;
		_config.setVariable("MpaRun", getMpaIdPadded(_currentRunId));
		run(data);
	}
	finalize();
}

bool MergedAnalysis::multirunConsistencyCheck(const std::string& argv0, const po::variables_map& vm)
{
	return true;
}
