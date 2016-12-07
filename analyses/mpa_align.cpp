
#include "mpa_align.h"
#include <TImage.h>
#include <TText.h>
#include <TGraph.h>

REGISTER_ANALYSIS_TYPE(MpaAlign, "Perform XYZ and angular alignment of MPA.")

MpaAlign::MpaAlign() :
 Analysis(), _aligner(), _file(nullptr)
{
	getOptionsDescription().add_options()
		("low-z", po::value<double>()->default_value(820), "Lower bound of Z align scan")
		("high-z", po::value<double>()->default_value(860), "Upper bound of Z align scan")
		("num-steps,s", po::value<int>()->default_value(10), "Number of steps in the range (low,high)")
		("sample-size,n", po::value<int>()->default_value(10000), "Number of data points to include in alignment histogram")
	;
	addProcess("scan", /* CS_ALWAYS */ CS_TRACK,
	           core::Analysis::init_callback_t {},
	           std::bind(&MpaAlign::scanInit, this),
		   std::bind(&MpaAlign::scanRun, this, std::placeholders::_1, std::placeholders::_2),
	           core::Analysis::run_post_callback_t {},
	           std::bind(&MpaAlign::scanFinish, this)
	           );
}

MpaAlign::~MpaAlign()
{
	if(_file) {
		delete _file;
	}
}

void MpaAlign::init(const po::variables_map& vm)
{
	_file = new TFile(getFilename(".root").c_str(), "RECREATE");
	_twoPass = vm.count("two-pass") > 0;
	_sampleSize = vm["sample-size"].as<int>();
	_lowZ = vm["low-z"].as<double>();
	_highZ = vm["high-z"].as<double>();
	_numSteps = vm["num-steps"].as<int>();
	_currentScanStep = 0;
	int num_plots = _numSteps + 3;
	int n_x = std::sqrt(num_plots);
	int n_y = std::sqrt(num_plots);
	while(n_x*n_y < _numSteps + 1) {
		++n_y;
	}
	int width = n_x * 400;
	int height = n_y * 300;
	assert(n_x > 0);
	assert(n_y > 0);
	assert(n_x*n_y >= _numSteps + 1);
	_xCanvas = new TCanvas("xCanvas", "xCanvas", width, height);
	_xCanvas->Divide(n_x, n_y);
	_xCanvas->cd(1);
	auto txt = new TText(.5, .5, "Hallo Welt!");
	txt->SetTextAlign(22);
	txt->SetTextSize(0.2);
	txt->Draw();
	_yCanvas = new TCanvas("yCanvas", "yCanvas", width, height);
	_yCanvas->Divide(n_x, n_y);
	std::remove(getFilename(".csv").c_str());
}

std::string MpaAlign::getUsage(const std::string& argv0) const
{
	return Analysis::getUsage(argv0);
}

std::string MpaAlign::getHelp(const std::string& argv0) const
{
        return Analysis::getHelp(argv0);
}

void MpaAlign::scanInit()
{
	_numProcessedSamples = 0;
	_currentZ = _lowZ + (_highZ - _lowZ) / (_numSteps-1) * _currentScanStep;
	_mpaTransform.setOffset({0, 0, _currentZ});
	std::cout << _currentScanStep << "/" << _numSteps
	          << ": Calculate alignment constants for Z=" << _currentZ << std::endl;
	_aligner.initHistograms(std::string("x_align_")+std::to_string(_currentScanStep),
	                        std::string("y_align_")+std::to_string(_currentScanStep));
	_currentSigmaMinimum = { 0.0, -1.0 };
}

bool MpaAlign::scanRun(const core::TrackStreamReader::event_t& track_event,
                      const core::BaseSensorStreamReader::event_t& mpa_event)
{
	for(const auto& track: track_event.tracks) {
		auto b = track.extrapolateOnPlane(4, 5, _currentZ, 2);
		for(size_t idx = 0; idx < mpa_event.data.size(); ++idx) {
			if(mpa_event.data[idx] == 0) continue;
			auto a = _mpaTransform.transform(idx);
			_aligner.Fill(b(0) - a(0), b(1) - a(1));
		}
	}
	return (_numProcessedSamples++ < _sampleSize);
}

void MpaAlign::scanFinish()
{
	int cd = _currentScanStep+2;
	std::cout << "cd-ing to Pad " << cd << std::endl;
	_xCanvas->cd(cd);
	_aligner.calculateAlignment();
	_aligner.appendAlignmentData(getFilename(".csv"), std::to_string(_currentZ));
	_alignments.push_back({
		Eigen::Vector3d(
			_aligner.getOffset()(0),
			_aligner.getOffset()(1),
			_currentZ),
		_aligner.getCuts()(0),
		_aligner.getCuts()(1)
	});
	if(_currentSigmaMinimum(1) < _aligner.getCuts()(0) || _currentSigmaMinimum(1) < 0) {
		_currentSigmaMinimum = { _currentZ, _aligner.getCuts()(0) };
	}
	std::string title = "Z = ";
	title += std::to_string(_currentZ);
	title += " mm";
	auto xcor = _aligner.getHistX();
	auto ycor = _aligner.getHistY();
	xcor->Write();
	ycor->Write();
	xcor->SetTitle(title.c_str());
	ycor->SetTitle(title.c_str());
	_xCanvas->cd(cd);
	xcor->Draw();
	_yCanvas->cd(cd);
	ycor->Draw();
	++_currentScanStep;
	if(_currentScanStep < _numSteps) {
		rerun();
	} else {
		auto xgraph = new TGraph(_numSteps);
		xgraph->SetName("x_sigma");
		auto ygraph = new TGraph(_numSteps);
		ygraph->SetName("y_width");
		for(size_t i=0; i < _alignments.size(); ++i) {
			const auto& align = _alignments[i];
			xgraph->SetPoint(i, align.position(2), align.x_sigma);
			ygraph->SetPoint(i, align.position(2), align.y_width);
		}
		xgraph->Write();
		ygraph->Write();

		_xCanvas->cd(_currentScanStep+2);
		xgraph->Draw("A*");
		_xCanvas->Update();
		_yCanvas->cd(_currentScanStep+2);
		ygraph->Draw("A*");
		_yCanvas->Update();

		auto img = TImage::Create();
		img->FromPad(_xCanvas);
		img->WriteImage(getFilename("_x.png").c_str());
		delete img;
		img = TImage::Create();
		img->FromPad(_yCanvas);
		img->WriteImage(getFilename("_y.png").c_str());
		delete img;

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
		if(_currentScanStep+1 >= _numSteps) {
			std::ofstream of(getFilename(".align"));
			auto align = _alignments[best_idx];
			of << align.position(0) << " "
			   << align.position(1) << " "
			   << align.position(2) << " "
			   << align.x_sigma << " "
			   << align.y_width << "\n";
			of.flush();
			of.close();
		}
	}
}

