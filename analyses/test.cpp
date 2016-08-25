
#include "test.h"
#include <iostream>

REGISTER_ANALYSIS_TYPE(Test, "A dummy analysis for testing the framework.")

Test::Test() :
 Analysis()
{
	std::cout << "Constructing analysis" << std::endl;
}

Test::~Test()
{
	std::cout << "Desctructing analysis" << std::endl;
}

void Test::run(const po::variables_map& vm)
{
	std::cout << "Running analysis" << std::endl;
}

std::string Test::getHelp(const std::string& argv0) const
{
	return "A dummy analysis for testing the framework.";
}
