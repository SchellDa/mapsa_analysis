
#include "test.h"
#include <iostream>
#include <algorithm>

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
	std::cout << "The following configuration variables where defined at runtime:\n";
	auto variables = _config.getDefinedVariables();
	std::sort(variables.begin(), variables.end());
	for(const auto& var: variables) {
		try {
			auto val = _config.getVariable(var);
			std::cout << " " << var << " = " << val << "\n";
		} catch(std::exception& e) {
			std::cout << "ERROR! " << var << " : " << e.what() << "\n";
		}
	}
	std::cout << std::flush;
}

std::string Test::getHelp(const std::string& argv0) const
{
	return "A dummy analysis for testing the framework.";
}
