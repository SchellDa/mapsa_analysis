
#include "mpa_efficiency.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <TCanvas.h>
#include <TPaveStats.h>
#include <TLatex.h>
#include <TStyle.h>
#include <TImage.h>
#include <TText.h>
#include <TPaveText.h>

REGISTER_ANALYSIS_TYPE(MpaEfficiency, "Calculate MPA Efficiency based.")

MpaEfficiency::MpaEfficiency() :
 Analysis(), _file(nullptr)
{
	addProcess("analyze", CS_TRACK,
	 core::Analysis::init_callback_t{},
	 std::bind(&MpaEfficiency::analyzeRunInit, this),
	 std::bind(&MpaEfficiency::analyze, this, std::placeholders::_1, std::placeholders::_2),
	 Analysis::run_post_callback_t{},
	 std::bind(&MpaEfficiency::analyzeFinish, this)
	);
	getOptionsDescription().add_options()
	;
}

MpaEfficiency::~MpaEfficiency()
{
	if(_file) {
		_file->Write();
		_file->Close();
		delete _file;
	}
}

void MpaEfficiency::init(const po::variables_map& vm)
{
	_file = new TFile(getRootFilename().c_str(), "RECREATE");
	double sizeX = _mpaTransform.total_width;
	double sizeY = _mpaTransform.total_height;
	auto resolution = _config.get<unsigned int>("efficiency_histogram_resolution_factor");
	_correlated = new TH2D("correlated", "Correlated Hits", // 16, 0, 16, 3, 0, 3);
	                       _mpaTransform.num_pixels_x * resolution,
	                       0.0,
	                       sizeX,
	                       _mpaTransform.num_pixels_y * resolution * sizeY/sizeX,
	                       0.0,
	                       sizeY);
	_total = new TH2D("total", "Total Tracks Hits",
	                  _mpaTransform.num_pixels_x * resolution,
			  0.0,
			  sizeX,
	                  _mpaTransform.num_pixels_y * resolution * sizeY/sizeX,
	                  0.0,
			  sizeY);
	int nx = 2;
	int ny = 2;
	int overlayed_resolution_factor = 2;
	_correlatedOverlayed = new TH2D("correlatedOverlayed", "",
	                                nx * resolution*overlayed_resolution_factor,
	                                0,
	                                nx,
	                                ny * resolution*overlayed_resolution_factor,
	                                0,
	                                ny);
	_totalOverlayed = new TH2D("totalOverlayed", "",
	                           nx * resolution*overlayed_resolution_factor,
				   0,
				   nx,
	                           ny * resolution*overlayed_resolution_factor,
	                           0,
				   ny);
	_hitsPerEvent = new TH1D("hitsPerEvent", "MPA Hits per Event", 48, 0, 48);
	_hitsPerEvent->GetXaxis()->SetTitle("# of MPA Hits");
	_hitsPerEventWithTrack = new TH1D("hitsPerEventWithTrack", "MPA Hits per Event with Track", 48, 0, 48);
	_hitsPerEventWithTrack->GetXaxis()->SetTitle("# of MPA Hits");
	double max_x = _mpaTransform.num_pixels_x*2;
	_correlationDistance = new TH1D("correlationDistance", "Distances between correlated tracks and pixels",
	                                max_x*4, 0, max_x);
	_correlationDistance->GetXaxis()->SetTitle("Distance \\times 100{\\mu}m");
	_totalCount = 0;
	_correlatedCount = 0;
	try {
		std::ifstream fmask(_config.getVariable("pixel_mask"));
		_pixelMask.resize(_mpaTransform.num_pixels, false);
		while(fmask.good()) {
			int idx;
			fmask >> idx;
			if(idx < 0) {
				throw std::out_of_range("Invalid pixel mask index, must not be negative.");
			} else if(idx >= _mpaTransform.num_pixels) {
				throw std::out_of_range("Invalid pixel mask index, must be smaller than number of MPA pixels.");
			}
			_pixelMask[idx] = true;
		}
	} catch(core::CfgParse::no_variable_error& e) {
		std::cout << "No pixel_mask option set." << std::endl;
	}
}

std::string MpaEfficiency::getUsage(const std::string& argv0) const
{
	return Analysis::getUsage(argv0);
}

std::string MpaEfficiency::getHelp(const std::string& argv0) const
{
        return Analysis::getHelp(argv0);
}

void MpaEfficiency::analyzeRunInit()
{
	const int runId = getCurrentRunId();
	_aligner.setNSigma(_config.get<double>("n_sigma_cut"));
	_nSigma = _config.get<double>("n_sigma_cut");
	std::string alignfile (
		_config.get<std::string>("output_dir") +
		std::string("/ZAlignTest_") +
		getMpaIdPadded(runId) +
		".align"
	);
	if(!_aligner.loadAlignmentData(alignfile)) {
		throw std::runtime_error(std::string("Cannot find alignment data for run ")
		                         + std::to_string(runId));
	}
	_aligner.setCuts({_mpaTransform.inner_pixel_width, _mpaTransform.bottom_pixel_height});
	_mpaTransform.setOffset(_aligner.getOffset());
	auto offset = _mpaTransform.getOffset();
	std::cout << "Run MPA offset: "
	          << offset(0) << " "
		  << offset(1) << " "
		  << offset(2) << std::endl;
}

bool MpaEfficiency::analyze(const core::TrackStreamReader::event_t& track_event,
                              const core::BaseSensorStreamReader::event_t& mpa_event)
{
	const auto runId = getCurrentRunId();
	size_t mpaHits = 0;
	for(size_t idx = 0; idx < mpa_event.data.size(); ++idx) {
		if(mpa_event.data[idx] > 0)
			++mpaHits;
	}
	_hitsPerEvent->Fill(mpaHits);
	if(mpaHits != 1 || track_event.tracks.size() != 1) {
		return true;
	}
	bool hasTrackOnMpa = false;
	for(const auto& track: track_event.tracks) {
		Eigen::Vector3d t_global = track.extrapolateOnPlane(3, 5, _mpaTransform.getOffset()(2), 2);
		Eigen::Vector3d t_local(t_global - _mpaTransform.getOffset());
		const auto sizeX = _mpaTransform.total_width;
		const auto sizeY = _mpaTransform.total_height;
		if(t_local(0) < 0.0 || t_local(0) > sizeX ||
		   t_local(1) < 0.0 || t_local(1) > sizeY) {
			continue;
		}
		hasTrackOnMpa = true;
		bool no_hit = true;
		for(size_t idx = 0; idx < mpa_event.data.size(); ++idx) {
			auto pixel_coord = _mpaTransform.transform(idx, true);
			auto masked = _pixelMask[idx];
			auto pixel_size = _mpaTransform.getPixelSize(idx);
			if(!((pixel_coord - t_global).head<2>().array().abs() < pixel_size.array()*_nSigma).all()) {
				continue;
			}
			if(mpa_event.data[idx] > 0) {
				_correlated->Fill(t_local(0), t_local(1));
				++_correlatedCount;
				no_hit = false;
				_correlationDistance->Fill((pixel_coord-t_global).head<2>().norm() / 0.1);
				break;
			}
		}
		bool is_masked = false;
		static const double maskSigma = 0.5;
		if(no_hit) {
			for(size_t idx = 0; idx < mpa_event.data.size(); ++idx) {
				if(!_pixelMask[idx]) continue;
				auto pixel_coord = _mpaTransform.transform(idx, true);
				auto pixel_size = _mpaTransform.getPixelSize(idx);
				if(((pixel_coord - t_global).head<2>().array().abs() < pixel_size.array()*maskSigma).all()) {
					is_masked = true;
					break;
				}
			}
		}
		if(!is_masked) {
			_total->Fill(t_local(0), t_local(1));
			++_totalCount;
		}
	}
	if(hasTrackOnMpa) {
		_hitsPerEventWithTrack->Fill(mpaHits);
	}
	return true;
}

void MpaEfficiency::analyzeFinish()
{
	std::cout << "Write analysis results..." << std::endl;
/*	for(size_t x = 0; x < _trackHits->GetNbinsX(); ++x) {
		for(size_t y = 0; y < _trackHits->GetNbinsY(); ++y) {
			auto bin = _trackHits->GetBin(x, y);
			auto total =  _trackHits->GetBinContent(bin);
			if(total <= 0.0) continue;
			_efficiency->SetBinContent(bin, _efficiency->GetBinContent(bin) / total);
			_neighbourHits->SetBinContent(bin, _neighbourHits->GetBinContent(bin) / total);
			_directHits->SetBinContent(bin, _directHits->GetBinContent(bin) / total);
		}
	}*/
	auto run = _runlist.getByMpaRun(getAllRunIds()[0]);
	double efficiency = static_cast<double>(_correlatedCount) / _totalCount;
	std::ofstream fout(getFilename(".eff"));
	fout << "# 0\tTotal\tCorrelated\tEfficiency\tAngle\tThresh\tnSigma\tRuns\n";
	fout << "0\t"
	     << _totalCount << "\t"
	     << _correlatedCount << "\t"
	     << efficiency << "\t"
	     << run.angle << "\t"
	     << run.threshold << "\t"
	     << _nSigma << "\t";
	bool first = true;
	for(const auto runId: getAllRunIds()) {
		if(!first) fout << ",";
		fout << runId;
		first = false;
	}
	fout << "\n";
	fout.close();
	
	auto eff = dynamic_cast<TH2D*>(_correlated->Clone("efficiency"));
	eff->Divide(_total);

	auto canvas = new TCanvas("comparision", "", 400*3*2, 300*3*2);
	gStyle->SetOptStat(11);
	gStyle->cd();
	canvas->Divide(2, 2);
	canvas->cd(1);
	eff->Draw("COLZ");

	canvas->cd(2);
	std::ostringstream info;
	auto txt = new TPaveText(0.1, 0.1, 0.9, 0.9);
	info << "Info for MPA run: " << run.mpa_run << "\n";
	txt->AddText(info.str().c_str());
	
	info.str("");
	info << "Tel run no: " << run.telescope_run << "\n";
	txt->AddText(info.str().c_str());
	
	info.str("");
	info << "Angle: " << run.angle << " degrees\n";
	txt->AddText(info.str().c_str());

	info.str("");
	info << "Bias voltage: " << run.bias_voltage << " V\n";
	txt->AddText(info.str().c_str());

	info.str("");
	info << "Bias current: " << run.bias_current << " uA\n";
	txt->AddText(info.str().c_str());

	info.str("");
	info << "Threshold: " << run.threshold << " uA\n";
	txt->AddText(info.str().c_str());

	info.str("");
	info << "Global Efficiency: " << std::setprecision(2) << std::fixed
		  << efficiency * 100.0 << "%";
	txt->AddText(info.str().c_str());

	info.str("");
	info << "n sigma: " << std::setprecision(2) << std::fixed
		  << _nSigma;
	txt->AddText(info.str().c_str());

	const auto& runs = getAllRunIds();
	if(runs.size() > 1) {
		info.str("");
		info << "Runs used for analysis:";
		txt->AddText(info.str().c_str());
		info.str("");
		int counter = 0;
		for(const auto& runId: runs) {
			if(++counter % 8 == 0) {
				txt->AddText(info.str().c_str());
				info.str("");
			}
			info << runId << " ";
		}
		txt->AddText(info.str().c_str());
	}
	txt->Draw();

	canvas->cd(3);
	_total->Draw("COLZ");
	canvas->cd(4);
	_correlated->Draw("COLZ");
	
	auto img = TImage::Create();
	img->FromPad(canvas);
	img->WriteImage(getFilename("_results.png").c_str());
	delete img;

	canvas = new TCanvas("hpe_canvas", "", 800, 1200);
	canvas->Divide(1, 2);
	canvas->cd(1);
	_hitsPerEvent->Draw();
	canvas->cd(2);
	_hitsPerEventWithTrack->Draw();
	img = TImage::Create();
	img->FromPad(canvas);
	img->WriteImage(getFilename("_hits.png").c_str());
	delete img;

	canvas = new TCanvas("cordist_canvas", "", 800, 600);
	gStyle->SetOptStat("nmer");
	_correlationDistance->SetStats(true);
	_correlationDistance->Draw();
	canvas->Update();
	auto ps = static_cast<TPaveStats*>(canvas->GetPrimitive("stats"));
	assert(ps != nullptr);
	ps->SetName("myEditedStats"); // Important! Otherwise PaveStats disappear
	TList* lof = ps->GetListOfLines();
	assert(lof != nullptr);
	info.str("");
	info << "n_{\\sigma_\\text{cut}} = " << std::setprecision(2) << std::fixed
		  << _nSigma;
	TLatex* line = new TLatex(0, 0, info.str().c_str());
	line ->SetTextSize(0.04);
	lof->Add(line);
	_correlationDistance->SetStats(false);
	canvas->Modified();
	img = TImage::Create();
	img->FromPad(canvas);
	img->WriteImage(getFilename("_cordist.png").c_str());
	delete img;

	assert(_total->GetSize() == _correlated->GetSize());
	int total_bins = _total->GetSize();
	std::cout << "Total hits: " << _totalCount
	          << "\nDUT hits: " << _correlatedCount
		  << "\nEfficiency: " << std::setprecision(2) << std::fixed
		  << efficiency * 100.0 << "%"
		  << std::endl;
	int total2d_xflow = _total->GetBinContent(0) + _total->GetBinContent(total_bins-1);
	int hit2d_xflow = _correlated->GetBinContent(0) + _correlated->GetBinContent(total_bins-1);
	int total2d = 0;
	int hit2d = 0;
	for(size_t i=1; i<total_bins; ++i) {
		total2d += _total->GetBinContent(i);
		hit2d += _correlated->GetBinContent(i);
	}
	std::cout << "Global Efficiency based on 2D Histograms"
	          << "\n <Total> Xflow entries: " << total2d_xflow
		  << "\n <Total> bin entries:   " << total2d
		  << "\n <Hit> Xflow entries:   " << hit2d_xflow
		  << "\n <Hit> bin entries:     " << hit2d
		  << std::setprecision(2) << std::fixed
		  << "\n Resulting efficiency:  " << static_cast<double>(hit2d)/total2d*100.0 << "%"
		  << std::endl;
}

