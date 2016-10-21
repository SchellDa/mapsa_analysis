
#include "strip_align.h"
#include <TImage.h>
#include <TText.h>
#include <TGraph.h>
#include <TFitResult.h>

REGISTER_ANALYSIS_TYPE(StripAlign, "Textual analysis description here.")

StripAlign::StripAlign() :
 Analysis(), _file(nullptr)
{
	getOptionsDescription().add_options()
		("low-z", po::value<double>()->default_value(820), "Lower bound of Z align scan")
		("high-z", po::value<double>()->default_value(860), "Upper bound of Z align scan")
		("num-steps,s", po::value<int>()->default_value(10), "Number of steps in the range (low,high)")
		("sample-size,n", po::value<int>()->default_value(10000), "Number of data points to include in alignment histogram")
	;
	addProcess("scan", /* CS_ALWAYS */ CS_TRACK,
	           core::Analysis::init_callback_t {},
	           std::bind(&StripAlign::scanInit, this),
		   std::bind(&StripAlign::scanRun, this, std::placeholders::_1, std::placeholders::_2),
	           core::Analysis::run_post_callback_t {},
	           std::bind(&StripAlign::scanFinish, this)
	           );
}

StripAlign::~StripAlign()
{
	if(_file) {
		_file->Write();
		_file->Close();
		delete _file;
	}
}

void StripAlign::init(const po::variables_map& vm)
{
	_out.open(getFilename(".csv"));
	_file = new TFile(getFilename(".root").c_str(), "RECREATE");
	assert(_file);
	_sampleSize = vm["sample-size"].as<int>();
	_lowZ = vm["low-z"].as<double>();
	_highZ = vm["high-z"].as<double>();
	_numSteps = vm["num-steps"].as<int>();
	_currentScanStep = 0;
	int num_plots = _numSteps + 2;
	int n_x = std::sqrt(num_plots);
	int n_y = std::sqrt(num_plots);
	while(n_x*n_y < num_plots) {
		++n_y;
	}
	int width = n_x * 400;
	int height = n_y * 300;
	assert(n_x > 0);
	assert(n_y > 0);
	assert(n_x*n_y >= num_plots);
	_canvas = new TCanvas("Canvas", "Canvas", width, height);
	_canvas->Divide(n_x, n_y);
	_canvas->cd(1);
	auto txt = new TText(.5, .5, "Hallo Welt!");
	txt->SetTextAlign(22);
	txt->SetTextSize(0.2);
	txt->Draw();
	std::remove(getFilename(".csv").c_str());
}

std::string StripAlign::getUsage(const std::string& argv0) const
{
	return Analysis::getUsage(argv0);
}

std::string StripAlign::getHelp(const std::string& argv0) const
{
        return Analysis::getHelp(argv0);
}

void StripAlign::scanInit()
{
	assert(_file);
	_numProcessedSamples = 0;
	_currentZ = _lowZ + (_highZ - _lowZ) / (_numSteps-1) * _currentScanStep;
	_config.setVariable("mpa_z_offset", _currentZ);
	std::cout << _currentScanStep << "/" << _numSteps
	          << ": Calculate alignment constants for Z=" << _currentZ << std::endl;
	_corHist = new TH1D((std::string("align_")+std::to_string(_currentScanStep)).c_str(), "align", 2000, -200, 200);
	_file->Add(_corHist);
	_currentSigmaMinimum = { 0.0, -1.0 };
}

bool StripAlign::scanRun(const core::TrackStreamReader::event_t& track_event,
                      const core::BaseSensorStreamReader::event_t& mpa_event)
{
	int n_strips = _config.get<int>("strip_count");
	double pitch = _config.get<double>("strip_pitch");
	double length = _config.get<double>("strip_length");
	for(const auto& track: track_event.tracks) {
		auto b = track.extrapolateOnPlane(1, 3, _currentZ, 2);
		for(const auto& idx: mpa_event.data) {
			double x = (static_cast<double>(idx) - n_strips/2) * pitch;
			//std::cout << idx << " -> " << x << std::endl;
			_corHist->Fill(b(0) - x);
		}
	}
	return (_numProcessedSamples++ < _sampleSize);
}

void StripAlign::scanFinish()
{
	int cd = _currentScanStep+2;
	std::cout << "cd-ing to Pad " << cd << std::endl;
	_canvas->cd(cd);
	auto result = _corHist->Fit("gaus", "SAME");
	double sigma = result->Parameter(2);
	_alignments.push_back({
		Eigen::Vector3d(
			0, 0,
			_currentZ),
		sigma
	});
	if(_currentSigmaMinimum(1) < sigma || _currentSigmaMinimum(1) < 0) {
		_currentSigmaMinimum = { _currentZ, sigma };
	}
	std::string title = "Z = ";
	title += std::to_string(_currentZ);
	title += " mm";
	_corHist->SetTitle(title.c_str());
	_corHist->Draw();
	++_currentScanStep;
	if(_currentScanStep < _numSteps) {
		rerun();
	} else if(_currentScanStep+1 >= _numSteps) {

		auto xgraph = new TGraph(_numSteps);
		xgraph->SetName("x_sigma");
		for(size_t i=0; i < _alignments.size(); ++i) {
			const auto& align = _alignments[i];
			xgraph->SetPoint(i, align.position(2), align.sigma);
		}
		_file->Add(xgraph);
		double x_min = xgraph->GetX()[0];
		double y_min = xgraph->GetY()[0];
		size_t best_idx = 0;
		for(size_t i=0; i < xgraph->GetN(); ++i) {
			if(y_min > xgraph->GetY()[i]) {
				x_min = xgraph->GetX()[i];
				y_min = xgraph->GetY()[i];
				best_idx = i;
			}
		}
		_canvas->cd(_currentScanStep+2);
		xgraph->Draw("ACP");
		_canvas->Update();

		auto img = TImage::Create();
		img->FromPad(_canvas);
		img->WriteImage(getFilename(".png").c_str());
		delete img;

		std::ofstream of(getFilename("_best.align"));
		auto align = _alignments[best_idx];
		of << align.position(0) << " "
		   << align.position(1) << " "
		   << align.position(2) << " "
		   << align.sigma << " "
		   << 0.0 << "\n";
		of.flush();
		of.close();
	}
}

