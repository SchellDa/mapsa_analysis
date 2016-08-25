
#ifndef ANALYSIS_H
#define ANALYSIS_H

#include <boost/program_options.hpp>
#include "abstractfactory.h"

namespace po = boost::program_options;

namespace core {

class Analysis
{
public:
	Analysis();
	virtual ~Analysis() {}
	virtual void run(const po::variables_map& vm) = 0;

	po::options_description& getOptionsDescriptionRef() { return _options; }
	po::options_description getOptionsDescription() const { return _options; }

	virtual std::string getUsage(const std::string& argv0) const;
	virtual std::string getHelp(const std::string& argv0) const;

private:
	po::options_description _options;
};

typedef AbstractFactory<Analysis, std::string> AnalysisFactory;
#define REGISTER_ANALYSIS_TYPE(type, descr) REGISTER_FACTORY_TYPE_WITH_DESCR(core::Analysis, type, descr)

}// namespace core

#endif//ANALYSIS_H
