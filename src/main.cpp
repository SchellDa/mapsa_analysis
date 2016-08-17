
#include "config.h"
#include <regex>
#include <iostream>
#include <getopt.h>
#include "cfgparse.h"


void print_usage(char* argv[])
{
	std::cout << "Usage: " << argv[0] << " [-hv] [-c cfgfile] [-r runlist] MAPSA_RUN" << std::endl;
}

void print_help(char* argv[])
{
	print_usage(argv);
	std::cout << "" << std::endl;
}

void print_version(char* argv[])
{
#ifdef VERSION_TAG
	std::cout << argv[0] << ": Version " << VERSION_TAG << std::endl;
#else
	std::cout << argv[0] << ": Version " << VERSION_MAJOR << "." << VERSION_MINOR << std::endl;
#endif
	std::cout << "Think about a license here..." << std::endl;
}

int main(int argc, char* argv[])
{
	// command line format string
	const char* cmd_fmt = "r:c:vh";
	static struct option long_options[] = {
		{"runlist", 1, 0, 'r'},
		{"config", 1, 0, 'c'},
		{"version", 0, 0, 'v'},
		{"help", 0, 0, 'h'},
		{0,0,0,0}
	};
	std::string config_file = "";
	while(1) {
		int opt_idx = 0;
		int c = getopt_long(argc, argv, cmd_fmt, long_options, &opt_idx);
		if(c == -1)
			break;
		switch(c) {
		case 0:
			break;
		case 'r':
			break;
		case 'c':
			if(config_file != "") {
				std::cerr << argv[0] << ": More than one config file specified." << std::endl;
				print_usage(argv);
				exit(EXIT_FAILURE);
			}
			config_file = optarg;
			break;
		case 'v':
			print_version(argv);
			exit(EXIT_SUCCESS);
			break;
		case 'h':
			print_help(argv);
			exit(EXIT_SUCCESS);
			break;
		case '?': // unknown option or missing argument
			print_usage(argv);
			exit(EXIT_FAILURE);
			break;
		default:
			std::cerr << argv[0] << ": WARNING option " << c << " not implemented yet..." << std::endl;
		}
	}
	if(argc-optind != 1) {
		std::cerr << argv[0] << ": Require exactly one MaPSA Run ID" << std::endl;
		print_usage(argv);
		exit(EXIT_FAILURE);
	}
	if(config_file != "") {
		try {
			CfgParse cfg;
			cfg.load(config_file);
			cfg.setVariable("RunNumber", "000001");
			std::cout << "test: " << cfg.getVariable("test") << std::endl;
			std::cout << "recurse: " << cfg.getVariable("recurse") << std::endl;
		} catch(std::regex_error e) {
			std::cerr << "EXCEPTION! " << e.what() << std::endl;
			std::cerr << "CODE: " << e.code() << " " << std::regex_constants::error_brack << std::endl;
		}
	}
	return 0;
}
