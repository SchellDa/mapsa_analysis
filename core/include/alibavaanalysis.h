#ifndef ALIBAVA_ANALYSIS_H
#define ALIBAVA_ANALYSIS_H

#include "analysis.h"
#include "datastructures.h"
#include <vector>

namespace core {
	
class AlibavaAnalysis : public Analysis
{
public:
	AlibavaAnalysis();
	virtual ~AlibavaAnalysis();
	
	virtual void init(const po::variables_map& vm);
	virtual void init() = 0;

	virtual void run(const po::variables_map& vm);
	virtual void run(const alibava_run_data_t& run) = 0;

	virtual void finalize() = 0;

	virtual bool multirunConsistencyCheck(const std::string& argv0, 
					      const po::variables_map& vm);
  
private:
	alibava_run_data_t _runData;
};

}

#endif//ALIBAVA_ANALYSIS_H
