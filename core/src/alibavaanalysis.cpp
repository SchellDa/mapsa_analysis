/*
  This class exists to serve as an intermediate interface to the 
  alibava data.
  All the functionalty should be merged into the MergedAnalysis class to offer 
  a coherent interface to ROOT tree based test beam data
 */

#include "alibavaanalysis.h"

#include <iostream>
#include <sstream>

using namespace core;

AlibavaAnalysis::AlibavaAnalysis() :
	Analysis()
{
}

AlibavaAnalysis::~AlibavaAnalysis()
{
}

void AlibavaAnalysis::init(const po::variables_map& vm)
{
	std::cout << "Init system" << std::endl;
	auto runs = vm["run"].as<std::vector<int>>();
	_allRunIds = runs;
	
	// Loop over all runs
	for(auto runId : runs) {
		alibava_run_data_t data {runId, nullptr, nullptr, nullptr};
		_currentRunId = runId;
		//TODO remove hard coded padded width
		_config.setVariable("Run", getPaddedIdString(runId, 6));
		auto filename = _config.getVariable("testbeam_data");
		
		data.file = new TFile(filename.c_str(), "READ");
		if( !data.file || data.file->IsZombie() ) {
			std::ostringstream err;
			err << "Cannot open ROOT file '" << filename 
			    << "' for run " << runId; 
			throw std::runtime_error(err.str().c_str());
		}
		
		data.file->GetObject("data", data.tree);
		if( !data.tree ) {
			std::ostringstream err;
			err << "Cannot find data tree in ROOT file '"
			    << "' for run " << runId;
			throw std::runtime_error(err.str().c_str());
		}
		
		data.telescopeData = new TelescopeData;
		//*data.telescopeData = nullptr;
		data.telescopeHits = new TelescopeHits;
		//*data.telescopeHits = nullptr;
		data.tree->SetBranchAddress("telescope", &data.telescopeData);
		data.tree->SetBranchAddress("telHits", &data.telescopeHits);
		//assert(*data.telescopeData != nullptr);
		//assert(*data.telescopeHits != nullptr);
		
		data.alibavaData = new AlibavaData;
		data.tree->SetBranchAddress("alibava", &data.alibavaData);
		
		_runData.push_back(data);
	}

}


void AlibavaAnalysis::run(const po::variables_map& vm)
{
	init(vm);
	_currentRunId = _runData[0].runId;
	_config.setVariable("Run", getPaddedIdString(_currentRunId, 6));
	init();
	for(const auto& data : _runData) {
		_currentRunId = data.runId;
		_config.setVariable("Run", getPaddedIdString(_currentRunId, 6));
		run(data);
	}
	finalize();
}

bool AlibavaAnalysis::multirunConsistencyCheck(const std::string& argv0, const po::variables_map& vm)
{
	return true;
}
