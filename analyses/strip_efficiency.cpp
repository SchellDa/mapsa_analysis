
#include "strip_efficiency.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <TCanvas.h>
#include <TStyle.h>
#include <TImage.h>
#include <TText.h>
#include <TPaveText.h>

REGISTER_ANALYSIS_TYPE(StripEfficiency, "Measure the efficiency of a strip sensor.")

StripEfficiency::StripEfficiency() :
 Analysis(), _file(nullptr)
{
	addProcess("analyze", CS_TRACK,
	 core::Analysis::init_callback_t{},
	 std::bind(&StripEfficiency::analyzeRunInit, this),
	 std::bind(&StripEfficiency::analyze, this, std::placeholders::_1, std::placeholders::_2),
	 Analysis::run_post_callback_t{},
	 std::bind(&StripEfficiency::analyzeFinish, this)
	);
	getOptionsDescription().add_options()
		("mask,m",
		 po::value<std::string>()->default_value("../masks/maskedChannelsMay16.txt"),
		 "Channel mask file used by analysis to ignore dead strips.")
		("no-mask",
		 "Ignore any masking file")
	;
}

StripEfficiency::~StripEfficiency()
{
	if(_file) {
		_file->Write();
		_file->Close();
		delete _file;
	}
}

void StripEfficiency::init(const po::variables_map& vm)
{
	_file = new TFile(getRootFilename().c_str(), "RECREATE");
	double sizeX = _mpaTransform.getSensitiveSize()(0);
	double sizeY = _mpaTransform.getSensitiveSize()(1);
	auto resolution = _config.get<unsigned int>("efficiency_histogram_resolution_factor");
	if(!vm.count("no-mask") > 0) {
		auto fname = vm["mask"].as<std::string>();
		std::ifstream fin(fname);
		if(!fin.good()) {
			throw std::ios_base::failure(std::string("Cannot open mask file ") + fname);
		}
		std::string line;
		std::getline(fin, line);
		int cbc_id;
		fin >> cbc_id;
		fin.ignore(10, ':');
		while(fin.good()) {
			int chnum;
			fin >> chnum;
			int spread_ch = chnum / 2;
			if(chnum % 2 == 1) {
				spread_ch = chnum / 2 + 254;
			}
			fin.ignore(10, ',');
			_channelMask.push_back(cbc_id*128 + spread_ch);
		}
		std::cout << "Use following channel mask: ";
		for(const auto& idx: _channelMask) {
			std::cout << idx << " ";
		}
		std::cout << std::endl;
	}
}

std::string StripEfficiency::getUsage(const std::string& argv0) const
{
	return Analysis::getUsage(argv0);
}

std::string StripEfficiency::getHelp(const std::string& argv0) const
{
        return Analysis::getHelp(argv0);
}


void StripEfficiency::analyzeRunInit()
{
//	_mpaTransform.setAlignmentOffset(_aligner[getCurrentRunId()].getOffset());
//	auto offset = _mpaTransform.getOffset();
//	std::cout << "Run MPA offset: "
//	          << offset(0) << " "
//		  << offset(1) << " "
//		  << offset(2) << std::endl;
	const auto runId = getCurrentRunId();
	std::ostringstream fname;
	fname << _config.getVariable("output_dir") << "/StripAlign_" << getMpaIdPadded(runId) << "_best.align";
	std::ifstream alignfile(fname.str());
	if(alignfile.good()) {
		double x, y, z, sigma;
		alignfile >> x >> y >> z >> sigma;
		alignment_t align;
		align.position = Eigen::Vector3d(x, y, z);
		align.sigma = sigma;
		if(!alignfile.good()) {
			std::cerr << "Error while reading alignemnt file: " << fname.str() << std::endl;
			throw std::runtime_error("Error while reading alignment file.");
		}
		std::cout << "Loaded the following alignment:"
		          << "\n  Position "
			  << align.position(0) << "mm "
			  << align.position(1) << "mm "
			  << align.position(2) << "mm"
			  << "\n  Sigma " << align.sigma << "mm" << std::endl;
		_alignments[runId] = align;
	} else {
		std::cerr << "Cannot find alignment file: " << fname.str() << std::endl;
		throw std::ios_base::failure("Alignment file does not exist. Make sure to generate all alignments.");
	}
}
bool StripEfficiency::analyze(const core::TrackStreamReader::event_t& track_event,
                              const core::BaseSensorStreamReader::event_t& mpa_event)
{
	const auto runId = getCurrentRunId();
	const double sensor_active_x = 11.860; // mm, 127 strips + bias ring
	const double sensor_active_y = 48.522; // mm, strip length
	const int strip_count = 127;
	const double strip_pitch = 0.09;
	const auto& align = _alignments[runId];
	double x_low = align.position(0) - sensor_active_x/2;
	double x_high = align.position(0) + sensor_active_x/2;
	double y_low = align.position(1) - sensor_active_y/2;
	double y_high = align.position(1) + sensor_active_y/2;
	if(track_event.tracks.size() != 1) {
		return true;
	}
	for(const auto& track: track_event.tracks) {
		bool got_trackhit = false;
		auto b = track.extrapolateOnPlane(1, 3, align.position(2), 2);
		if(x_low < b(0) && x_high > b(0) && y_low < b(1) && y_high > b(1)) {
			++_totalHits;
			for(const auto& strip_idx: mpa_event.data) {
				if(strip_idx >= 254) {
					continue;
				}
				double x = static_cast<double>(strip_idx) - strip_count/2;
				x *= strip_pitch;
				x += align.position(0);
				if(std::abs(x - b(0)) < 3*align.sigma) {
					++_correlatedHits;
					break;
				}
			}
		}
	}
	return true;
}

void StripEfficiency::analyzeFinish()
{
	double eff = static_cast<double>(_correlatedHits) / _totalHits;
	const auto runId = getCurrentRunId();
	std::cout << "Total hits: " << _totalHits
	          << "\nDUT hits: " << _correlatedHits
		  << "\nEfficiency: " << std::setprecision(2) << std::fixed
		  << eff * 100.0 << "%"
		  << std::endl;
	auto run = _runlist.getByMpaRun(runId);
	std::ofstream fout(getFilename(".eff"));
	fout << runId << "\t"
	     << _totalHits << "\t"
	     << _correlatedHits << "\t"
	     << eff << "\t"
	     << run.angle << "\t"
	     << run.threshold << "\n";
}

