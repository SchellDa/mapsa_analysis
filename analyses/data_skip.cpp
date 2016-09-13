
#include "data_skip.h"
#include "util.h"
#include <TImage.h>
#include <TText.h>
#include <cmath>

REGISTER_ANALYSIS_TYPE(DataSkip, "Textual analysis description here.")

DataSkip::DataSkip() :
 Analysis(), _file(nullptr), _currentHist(nullptr)
{
	addProcess("analyze", CS_TRACK,
		std::bind(&DataSkip::analyze, this, std::placeholders::_1, std::placeholders::_2),
	        std::bind(&DataSkip::finish, this));
	getOptionsDescription().add_options()
		("range", po::value<int>()->default_value(10), "Generate correlation in data offset range -NUM to NUM")
		("num,n", po::value<int>()->default_value(5000), "Number of events used for correlation histogram")
		("bins,b", po::value<int>()->default_value(500), "Number of bins in the history")
	;
}

DataSkip::~DataSkip()
{
	if(_file) {
		_file->Write();
		_file->Close();
		delete _file;
	}
}

void DataSkip::init(const po::variables_map& vm)
{
	_file = new TFile(getRootFilename().c_str(), "RECREATE");
	_numBins = vm["bins"].as<int>();
	_range = vm["range"].as<int>();
	setDataOffset(-_range);
	_eventsPerRun = vm["num"].as<int>();

	std::ostringstream name;
	name << "xcorrelation_" << getDataOffset()+_range;
	std::ostringstream title;
	title << "data skip = " << getDataOffset();
	_currentHist = new TH1D(name.str().c_str(), title.str().c_str(), _numBins, -10, -10);
	int nx = core::max(1, static_cast<int>(std::sqrt(_range*2+2)));
	int ny = core::max(1, (_range*2 + 2) / nx) + 1;
	assert(nx*ny >= _range*2+2);

	_canvas = new TCanvas("canvas", "", 400*nx, 300*ny);
	_canvas->Divide(nx, ny);
	_canvas->cd(1);
	std::ostringstream runStr;
	runStr << "MPA run " << _config.get<int>("MpaRun");
	auto txt = new TText(.5, .5, runStr.str().c_str());
	txt->SetTextAlign(22);
	txt->SetTextSize(0.2);
	txt->Draw();
}

std::string DataSkip::getUsage(const std::string& argv0) const
{
	return Analysis::getUsage(argv0);
}

std::string DataSkip::getHelp(const std::string& argv0) const
{
        return Analysis::getHelp(argv0);
}

bool DataSkip::analyze(const core::TrackStreamReader::event_t& track_event,
                      const core::MPAStreamReader::event_t& mpa_event)
{
	for(const auto& track: track_event.tracks) {
		for(size_t idx = 0; idx < mpa_event.data.size(); ++idx) {
			if(!mpa_event.data[idx]) continue;
			auto a = _mpaTransform.transform(idx);
			auto b = track.extrapolateOnPlane(0, 5, 840, 2);
			_currentHist->Fill(b(0) - a(0));
		}
	}
	return _currentHist->GetEntries() < _eventsPerRun;
}

void DataSkip::finish()
{
	const double binratio = 0.1;
	if(_currentHist->GetEntries() * binratio * 2 < _currentHist->GetNbinsX()) {
		/*std::cout << "REBIN!\n"
		          << "Num entries: " << _currentHist->GetEntries() << "\n"
		          << "Entry threshold: " << _currentHist->GetEntries() * binratio * 2 << "\n"
			  << "Num X bins: " << _currentHist->GetNbinsX() << "\n";*/
		_currentHist->Rebin(_currentHist->GetNbinsX() / (_currentHist->GetEntries() * binratio));
//		std::cout << "New reduced bin number: " << _currentHist->GetNbinsX() << std::endl;
	}
	_canvas->cd(getDataOffset()+_range+2);
	_currentHist->Draw();
	if(getDataOffset()+1 <= _range) {
		setDataOffset(getDataOffset()+1);
		rerun();
		
		std::ostringstream name;
		name << "xcorrelation_" << getDataOffset()+_range;
		std::ostringstream title;
		title << "data skip = " << getDataOffset();
		_currentHist = new TH1D(name.str().c_str(), title.str().c_str(), _numBins, -10, -10);
	} else {
		auto img = TImage::Create();
		img->FromPad(_canvas);
		img->WriteImage(getFilename(".png").c_str());
		delete img;
	}
}
