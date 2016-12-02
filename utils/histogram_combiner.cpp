#include <iostream>
#include <sstream>
#include <boost/program_options.hpp>
#include <TH2.h>
#include <TFile.h>

namespace po = boost::program_options;

std::string getHelp(const std::string& argv0)
{
	return "";
}

std::string getUsage(const std::string& argv0)
{
	std::ostringstream str;
	str << "Usage: " << argv0 << " [-o file] [-i img] [-h histname] [-c] [--grid-size \"x,y\"] [-s \"x,y\"] [--operation op] [[-c \"x,y\"} ...] infile [[infile1] ...]";
	return str.str();
}

int main(int argc, char* argv[])
{
	po::options_description options;
	options.add_options()
		("help,h", "Show help message")
		("output,o", po::value<std::string>()->default_value("combined.root"), "Output ROOT file")
		("image,i", po::value<std::string>(), "Output an image version with given filename")
		("histogram,n", po::value<std::string>()->default_value("hist"), "Add histograms with this name")
		("combine,c", "Combine bins defined by a grid or individual pairs")
		("operation", po::value<std::string>()->default_value("+"), "Combine-operation, either + - * /")
		("grid-size", po::value<std::string>()->default_value("16,3"), "Total number grid cells in X,Y format")
		("subgrid,s", po::value<std::string>(), "Size of subgrid to add. Cannot be combined with --cell option")
		("cell,c", po::value<std::string>(), "Add all specified cells in each histogram. Cannot be used with --subgrid")
	;
	po::positional_options_description positionals;
	positionals.add("input-file", -1);
	po::variables_map vm;
	try {
		po::store(po::command_line_parser(argc, argv)
		          .options(options)
			  .positional(positionals)
			  .run(),
		          vm);
	} catch(std::exception& e) {
		std::cerr << argv[0] << ": " << e.what();
		std::cerr << "\n\n" << getUsage(argv[0]) << std::endl;
		return 1;
	}
	if(vm.count("help")) {
		// get usage from analysis class
		auto help = getHelp(argv[0]);
		if(help != "")
			std::cout << help << "\n\n";
		std::cout << "Options:\n" << options
		          << "\nPositional arguments:\n  infile [[infile] ...]\tOne or more ROOT input files."
			  << std::endl;
		return 0;
	}
	po::notify(vm);
	TH2F* histogram = nullptr;

	return 0;
}
