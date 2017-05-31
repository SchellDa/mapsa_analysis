
#ifndef MERGED_ANALYSIS_H
#define MERGED_ANALYSIS_H

#include "analysis.h"
#include "datastructures.h"
#include <TFile.h>
#include <TTree.h>
#include <string>
#include <vector>

namespace core {

class MergedAnalysis : public Analysis
{
public:
	struct mpa_data_t
	{
		std::string name;
		int index;
		MpaData** data;
	};
	struct run_data_t
	{
		int runId;
		TFile* file;
		TTree* tree;
		TelescopeData** telescopeData;
		TelescopeHits** telescopeHits;
		std::vector<mpa_data_t> mpaData;
	};
	MergedAnalysis();
	virtual ~MergedAnalysis();

	virtual void init(const po::variables_map& vm);
	virtual void init() = 0;

	virtual void run(const po::variables_map& vm);
	virtual void run(const run_data_t& run) = 0;

	virtual void finalize() = 0;

	virtual bool multirunConsistencyCheck(const std::string& argv0, const po::variables_map& vm);

private:
	std::vector<run_data_t> _runData;
};

}

#endif//MERGED_ANALYSIS_H
