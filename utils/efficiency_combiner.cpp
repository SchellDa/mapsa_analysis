#include <iostream>
#include <memory>
#include <sstream>
#include <boost/program_options.hpp>
#include <TH1.h>
#include <TFile.h>
#include <TImage.h>
#include <TCanvas.h>

namespace po = boost::program_options;

std::string getHelp(const std::string& argv0)
{
	return "";
}

std::string getUsage(const std::string& argv0)
{
	std::ostringstream str;
	str << "Usage: " << argv0 << " [-o file] [-i img] [-d histname] [-t histname] infile [[infile1] ...]";
	return str.str();
}

int main(int argc, char* argv[])
{
	po::options_description options;
	options.add_options()
		("help,h", "Show help message")
		("output,o", po::value<std::string>()->default_value("combined.root"), "Output ROOT file")
		("image,i", po::value<std::string>(), "Output an image version with given filename")
		("detected-hist,d", po::value<std::string>()->default_value("correlatedHits"), "Name of the nominator histogram")
		("total-hist,t", po::value<std::string>()->default_value("trackHits"), "Name of the denominator histogram")
		("draw-mode", po::value<std::string>()->default_value("COLZ"), "Root histogram draw mode for image output")
		("input-file", po::value<std::vector<std::string>>(), "")
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
	if(vm.count("input-file") == 0) {
		std::cerr << "No input files specified." << std::endl;
		return 1;
	}
	std::unique_ptr<TFile> outfile(new TFile(vm["output"].as<std::string>().c_str(), "RECREATE"));
	TH1* total = nullptr;
	TH1* correlated = nullptr;
	std::string totalName = vm["total-hist"].as<std::string>();
	std::string correlatedName = vm["detected-hist"].as<std::string>();
	for(const auto& input: vm["input-file"].as<std::vector<std::string>>()) {
		//std::unique_ptr<TFile> file(new TFile(input.c_str(), "READ"));
		TFile* file = new TFile(input.c_str(), "READ");
		auto totalFile = dynamic_cast<TH1*>(file->Get(totalName.c_str()));
		auto correlatedFile = dynamic_cast<TH1*>(file->Get(correlatedName.c_str()));
		assert(totalFile);
		assert(correlatedFile);
		if(!total) {
			total = dynamic_cast<TH1*>(totalFile->Clone("summedTotal"));
			assert(total != nullptr);
			outfile->Add(total);
		} else {
			total->Add(totalFile);
		}
		if(!correlated) {
			correlated = dynamic_cast<TH1*>(correlatedFile->Clone("summedCorrelated"));
			assert(correlated != nullptr);
			outfile->Add(correlated);
		} else {
			correlated->Add(correlatedFile);
		}
	}
	assert(total != nullptr);
	assert(correlated != nullptr);
	auto efficiency = dynamic_cast<TH1*>(correlated->Clone("summedEfficiency"));
	efficiency->Divide(total);
	outfile->Add(efficiency);
	if(vm.count("image")) {
		TCanvas* canvas = new TCanvas("canvas", "canvas", 800*2, 600*2);
		canvas->Divide(2, 2);
		auto mode = vm["draw-mode"].as<std::string>();
		canvas->cd(1);
		efficiency->Draw(mode.c_str());
		canvas->cd(3);
		total->Draw(mode.c_str());
		canvas->cd(4);
		correlated->Draw(mode.c_str());
		auto img = TImage::Create();
		img->FromPad(canvas);
		img->WriteImage(vm["image"].as<std::string>().c_str());
		delete img;
	}
	outfile->Write();
	return 0;
}
