
#ifndef TEST_H_232
#define TEST_H_232

#include "analysis.h"

class Test : public core::Analysis
{
public:
	Test();
	virtual ~Test();
	virtual void run(const po::variables_map& vm);
	virtual std::string getHelp(const std::string& argv0) const;
};

#endif//TEST_H_232
