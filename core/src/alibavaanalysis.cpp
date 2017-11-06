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
	int run = runs[0];
	_allRunIds.push_back(run);
	
	std::cout << "Processing run " << run << std::endl;
	run_data_t data;
	data.runId = run;
	_currentRunId = run;
	//TODO remove hard coded padded width
	_config.setVariable("run", 
			    getPaddedIdString(run, _config.get<int>("id_padding")));

	// Read file
	auto filename = _config.getVariable("testbeam_data");	
	data.file = new TFile(filename.c_str(), "READ");
	if( !data.file || data.file->IsZombie() ) {
		std::ostringstream err;
		err << "Cannot open ROOT file '" << filename 
		    << "' for run " << run; 
		throw std::runtime_error(err.str().c_str());
	}
		
	data.file->GetObject("data", data.tree);
	if( !data.tree ) {
		std::ostringstream err;
		err << "Cannot find data tree in ROOT file '"
		    << "' for run " << run;
		throw std::runtime_error(err.str().c_str());
	}
		
	data.tree->Print();
	data.telescopeData = new TelescopeData*;
	*data.telescopeData = nullptr;
	data.telescopeHits = new TelescopeHits*;
	*data.telescopeHits = nullptr;
	data.alibavaData = new AlibavaData*;
	*data.alibavaData = nullptr;

	data.tree->SetBranchAddress("telescope", data.telescopeData);
	data.tree->SetBranchAddress("telHits", data.telescopeHits);
	data.tree->SetBranchAddress("alibava", data.alibavaData);
	assert(*data.telescopeData != nullptr);
	assert(*data.telescopeHits != nullptr);
	assert(*data.alibavaData != nullptr);

	_runData = data;
	       
}


void AlibavaAnalysis::run(const po::variables_map& vm)
{
	std::cout << "Start Run" << std::endl;
	init(vm);
	_currentRunId = _runData.runId;
	_config.setVariable("run", getPaddedIdString(_currentRunId, 6));
	init();
	run(_runData);
	finalize();
}

bool AlibavaAnalysis::multirunConsistencyCheck(const std::string& argv0, const po::variables_map& vm)
{
	return true;
}
