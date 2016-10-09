
#include "z_align_test.h"
#include <TImage.h>
#include <TText.h>

REGISTER_ANALYSIS_TYPE(ZAlignTest, "Textual analysis description here.")

ZAlignTest::ZAlignTest() :
 Analysis(), _aligner()
{
	getOptionsDescription().add_options()
		("low-z", po::value<double>()->default_value(820), "Lower bound of Z align scan")
		("high-z", po::value<double>()->default_value(860), "Upper bound of Z align scan")
		("num-steps,s", po::value<int>()->default_value(10), "Number of steps in the range (low,high)")
		("sample-size,n", po::value<int>()->default_value(10000), "Number of data points to include in alignment histogram")
	;
	addProcess("align", /* CS_ALWAYS */ CS_TRACK,
	           core::Analysis::init_callback_t {},
	           std::bind(&ZAlignTest::initRun, this),	           
		   std::bind(&ZAlignTest::analyze, this, std::placeholders::_1, std::placeholders::_2),
	           core::Analysis::run_post_callback_t {},
	           std::bind(&ZAlignTest::finish, this)
	           );
}

ZAlignTest::~ZAlignTest()
{
}

void ZAlignTest::init(const po::variables_map& vm)
{
	_sampleSize = vm["sample-size"].as<int>();
	_lowZ = vm["low-z"].as<double>();
	_highZ = vm["high-z"].as<double>();
	_numSteps = vm["num-steps"].as<int>();
	_currentScanStep = 0;
	int n_x = std::sqrt(_numSteps + 1);
	int n_y = std::sqrt(_numSteps + 1);
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

std::string ZAlignTest::getUsage(const std::string& argv0) const
{
	return Analysis::getUsage(argv0);
}

std::string ZAlignTest::getHelp(const std::string& argv0) const
{
        return Analysis::getHelp(argv0);
}

void ZAlignTest::initRun()
{
	_numProcessedSamples = 0;
	_currentZ = _lowZ + (_highZ - _lowZ) / (_numSteps-1) * _currentScanStep;
	_config.setVariable("mpa_z_offset", _currentZ);
	std::cout << _currentScanStep << "/" << _numSteps
	          << ": Calculate alignment constants for Z=" << _currentZ << std::endl;
	_aligner.initHistograms(std::string("x_align_")+std::to_string(_currentScanStep),
	                        std::string("y_align_")+std::to_string(_currentScanStep));

}

bool ZAlignTest::analyze(const core::TrackStreamReader::event_t& track_event,
                      const core::BaseSensorStreamReader::event_t& mpa_event)
{
	for(const auto& track: track_event.tracks) {
		auto b = track.extrapolateOnPlane(4, 5, _currentZ, 2);
		for(size_t idx = 0; idx < mpa_event.data.size(); ++idx) {
			if(mpa_event.data[idx] == 0) continue;
			auto a = _mpaTransform.transform(idx);
			auto pc = _mpaTransform.translatePixelIndex(idx);
			_aligner.Fill(b(0) - a(0), b(1) - a(1));
		}
	}
	return (_numProcessedSamples++ < _sampleSize);
}

void ZAlignTest::finish()
{
	int cd = _currentScanStep+2;
	std::cout << "cd-ing to Pad " << cd << std::endl;
	_xCanvas->cd(cd);
	_aligner.calculateAlignment();
	_aligner.appendAlignmentData(getFilename(".csv"), std::to_string(_currentZ));
	std::string title = "Z = ";
	title += std::to_string(_currentZ);
	title += " mm";
	auto xcor = _aligner.getHistX();
	auto ycor = _aligner.getHistY();
	xcor->SetTitle(title.c_str());
	ycor->SetTitle(title.c_str());
	xcor->Draw();
	_yCanvas->cd(cd);
	ycor->Draw();
	++_currentScanStep;
	if(_currentScanStep < _numSteps) {
		rerun();
	} else {
		auto img = TImage::Create();
		img->FromPad(_xCanvas);
		img->WriteImage(getFilename("_x.png").c_str());
		delete img;
		img = TImage::Create();
		img->FromPad(_yCanvas);
		img->WriteImage(getFilename("_y.png").c_str());
		delete img;
	}
}
