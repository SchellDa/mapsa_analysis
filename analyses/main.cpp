
#include <getopt.h>
#include <iostream>
#include "analysis.h"

int main(int argc, char* argv[])
{
	bool show_basic_help = false;
	bool list_analyses = false;
	if(argc == 1) {
		show_basic_help = true;
	} else {
		std::string name = argv[1];
		if(name == "-h" || name == "--help") {
			show_basic_help = true;
		} else if(name == "-l" || name == "--list") {
			list_analyses = true;
		} else if(name[0] == '-') {
			std::cerr << argv[0] << ": Invalid option " << name << ", excpected analysis name.\n\n"
				<< "Usage: " << argv[0] << " ANALYSIS [options]\n"
				"See '" << argv[0] << " --help' for more information" << std::endl;
			return 1;
		}
	}
	if(show_basic_help) {
		list_analyses = true;
	}
	if(list_analyses) {
		std::cout << "Supported analyses:\n";
		for(const auto& name: core::AnalysisFactory::Instance()->getTypes()) {
			std::cout << " " << name << " : " <<
				core::AnalysisFactory::Instance()->getDescription(name) << "\n";
		}
		return 0;
	}
	assert(argc > 1);
	std::string analysis_name = argv[1];
	std::string argv0(argv[0]);
	argv0 = argv0 + " " + argv[1];
	for(int i=1; i<argc-1; ++i) {
		argv[i] = argv[i+1];
	}
	argv[0] = const_cast<char*>(argv0.c_str());
	--argc;
	std::shared_ptr<core::Analysis> analysis;
	try {
		analysis = core::AnalysisFactory::Instance()->create(analysis_name);
	} catch(std::out_of_range& e) {
		std::cerr << "Analysis '" << analysis_name << "' was not found." << std::endl;
		return 1;
	}
	assert(analysis.get());
	po::variables_map vm;
	try {
		po::store(po::parse_command_line(argc, argv, analysis->getOptionsDescription()), vm);
	} catch(std::exception& e) {
		std::cerr << argv[0] << ": " << e.what();
		std::cerr << "\n\n" << analysis->getUsage(argv[0]) << std::endl;
		return 1;
	}
	if(vm.count("help")) {
		// get usage from analysis class
		auto help = analysis->getHelp(argv[0]);
		if(help != "")
			std::cout << help << "\n\n";
		std::cout << analysis->getOptionsDescription() << std::endl;
		return 0;
	}
	try {
		analysis->loadConfig(vm);
	} catch(core::CfgParse::parse_error& e) {
		std::cerr << argv[0] << ": Error while parsing configuration:\n" << e.what() << std::endl;
		return 1;
	}
	analysis->run(vm);
	return 0;
}
