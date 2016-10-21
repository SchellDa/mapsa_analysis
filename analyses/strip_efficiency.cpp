
#include "strip_efficiency.h"
#include <iostream>
#include <algorithm>
#include <TCanvas.h>
#include <TStyle.h>
#include <TImage.h>
#include <TText.h>
#include <TPaveText.h>

REGISTER_ANALYSIS_TYPE(StripEfficiency, "Measure the efficiency of a strip sensor.")

StripEfficiency::StripEfficiency() :
 Analysis(), _file(nullptr), _forceAlignment(false)
{
	addProcess("align", CS_TRACK,
	 Analysis::init_callback_t{},
	 std::bind(&StripEfficiency::alignInit, this),
	 std::bind(&StripEfficiency::alignRun, this, std::placeholders::_1, std::placeholders::_2),
	 std::bind(&StripEfficiency::alignFinish, this),
	 Analysis::post_callback_t{}
	);
	addProcess("analyze", CS_TRACK,
	 core::Analysis::init_callback_t{},
	 std::bind(&StripEfficiency::analyzeRunInit, this),
	 std::bind(&StripEfficiency::analyze, this, std::placeholders::_1, std::placeholders::_2),
	 Analysis::run_post_callback_t{},
	 std::bind(&StripEfficiency::analyzeFinish, this)
	);
	getOptionsDescription().add_options()
		("align,a", "Force recalculation of alignment parameters")
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
	if(vm.count("align") >= 1) {
		_forceAlignment = true;
	}
	_file = new TFile(getRootFilename().c_str(), "RECREATE");
	double sizeX = _mpaTransform.getSensitiveSize()(0);
	double sizeY = _mpaTransform.getSensitiveSize()(1);
	auto resolution = _config.get<unsigned int>("efficiency_histogram_resolution_factor");
}

std::string StripEfficiency::getUsage(const std::string& argv0) const
{
	return Analysis::getUsage(argv0);
}

std::string StripEfficiency::getHelp(const std::string& argv0) const
{
        return Analysis::getHelp(argv0);
}

void StripEfficiency::alignInit()
{
	const int runId = getCurrentRunId();
	_aligner[runId].setNSigma(_config.get<double>("n_sigma_cut"));
	if(!_forceAlignment) {
		_aligner[runId].loadAlignmentData(getFilename(runId, ".align"));
	}
	if(!_aligner[runId].gotAlignmentData()) {
		_aligner[runId].xHistogramConfig = { 1000, -150, 150  };
		_aligner[runId].yHistogramConfig = _aligner[runId].xHistogramConfig;
		_aligner[runId].initHistograms();
	}
}

bool StripEfficiency::alignRun(const core::TrackStreamReader::event_t& track_event,
                               const core::BaseSensorStreamReader::event_t&  mpa_event)
{
	const auto runId = getCurrentRunId();
	if(_aligner[runId].gotAlignmentData()) {
		return false;
	}
	int n_strips = _config.get<int>("strip_count");
	double pitch = _config.get<double>("strip_pitch");
	double length = _config.get<double>("strip_length");
	for(const auto& track: track_event.tracks) {
		auto b = track.extrapolateOnPlane(4, 5, 840, 2);
		for(const auto& idx: mpa_event.data) {
			double x = -(static_cast<double>(idx) - n_strips/2) * pitch;
			//std::cout << idx << " -> " << x << std::endl;
			_aligner[runId].Fill(b(0) - x, b(1) - x);
		}
	}
	return true;
}

void StripEfficiency::alignFinish()
{
	const auto runId = getCurrentRunId();
	_aligner[runId].writeHistogramImage(getFilename(runId, "_align.png"));
	_aligner[runId].writeHistograms();
	_file->Write();
	_aligner[runId].calculateAlignment();
	auto offset = _mpaTransform.getOffset();
	offset += _aligner[runId].getOffset();
	auto cuts = _aligner[runId].getCuts();
	std::cout << "Alignment for run " << runId
	          << "\n  x_off = " << offset(0)
	          << "\n  y_off = " << offset(1)
		  << "\n  z_off = " << offset(2)
	          << "\n  x_sigma = " << cuts(0)
	          << "\n  y_width = " << cuts(1) << std::endl; 
	_aligner[runId].writeHistogramImage(getFilename(runId, "_align.png"));
	_aligner[runId].saveAlignmentData(getFilename(runId, ".align"));
}

void StripEfficiency::analyzeRunInit()
{
//	_mpaTransform.setAlignmentOffset(_aligner[getCurrentRunId()].getOffset());
//	auto offset = _mpaTransform.getOffset();
//	std::cout << "Run MPA offset: "
//	          << offset(0) << " "
//		  << offset(1) << " "
//		  << offset(2) << std::endl;
}

bool StripEfficiency::analyze(const core::TrackStreamReader::event_t& track_event,
                              const core::BaseSensorStreamReader::event_t& mpa_event)
{
	const auto runId = getCurrentRunId();
//	for(const auto& track: track_event.tracks) {
///*		try {
//			auto hit_idx = _mpaTransform.getPixelIndex(track);
//			auto pc = _mpaTransform.translatePixelIndex(hit_idx);
//			_totalPixelHits[hit_idx] += 1;
//			if(mpa_event.data[hit_idx] > 0) {
//				_activatedPixelHits[hit_idx] += 1;
//				_efficiency->Fill(pc(0), pc(1));
//			}
//		} catch(std::out_of_range& e) {
//			// track missed MPA
//		}*/
//		bool hitMpa = false;
//		bool hitActivatedPixel = false;
//		auto b = track.extrapolateOnPlane(4, 5, _mpaTransform.getOffset()(2), 2);
//		auto cb(b);
//		cb -= _mpaTransform.getOffset();
//		_trackHits->Fill(cb(0), cb(1));
//		// combine results of pixels in first and second row in a 2x2
//		// pixel grid. That way, we have the same geometry
//		// (punch-through etc.) for each overlayed pixel.
//		auto globalHitpoint = _mpaTransform.mpaPlaneTrackIntersect(track, 4, 5);
//		auto pc = _mpaTransform.globalToPixelCoord(globalHitpoint);
//		Eigen::Vector2d subpc(std::fmod(pc(0), 2.0), std::fmod(pc(1), 3.0));
//		if(subpc(1) < 0.0 || subpc(1) > 3.0) {
//			subpc(1) += 10.0;
//		}
//		_trackHitsOverlayed->Fill(subpc(0), subpc(1));
//		for(size_t idx = 0; idx < mpa_event.data.size(); ++idx) {
//			auto a = _mpaTransform.transform(idx);
//			if(_aligner[runId].pointsCorrelated(a, b)) {
//				try {
//					auto hit_idx = _mpaTransform.getPixelIndex(track);
//					if(hit_idx == idx)
//						_directHits->Fill(cb(0), cb(1));
//					else
//					       _neighbourHits->Fill(cb(0), cb(1));	
//				} catch(std::out_of_range& e) {
//					// track missed MPA...
//					_neighbourHits->Fill(cb(0), cb(1));
//				}
//				_totalPixelHits[idx] += 1;
//				hitMpa = true;
//				_analysisHitFile << track_event.eventNumber << "\t" << cb(0) << "\t" << cb(1)
//					<< "\t" << a(0) << "\t" << a(1);
//				if(mpa_event.data[idx] == 0) {
//					_analysisHitFile << "\t0/0\t0/0\t0\n";
//				} else {
//					_analysisHitFile << "\t"
//						<< a(0) << "\t" << a(1)
//						<<"\t1\n";
//					if(!hitActivatedPixel) {
//						_efficiency->Fill(cb(0), cb(1));
//						_efficiencyOverlayed->Fill(subpc(0), subpc(1));
//						_efficiencyLocal->Fill(pc(0), pc(1));
//						hitActivatedPixel = true;
//					}
//				}
//			}
//		}
//		if(hitMpa) {
//			_analysisHitFile << "\n\n";
//			_analysisHitFile.flush();
//		}
//	}
//	return true;
}

void StripEfficiency::analyzeFinish()
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
/*	auto eff = dynamic_cast<TH2D*>(_efficiency->Clone("pixelEfficiency"));
	auto neigh = dynamic_cast<TH2D*>(_neighbourHits->Clone("neighbourHitsScaled"));
	auto dir = dynamic_cast<TH2D*>(_directHits->Clone("directHitsScaled"));
	eff->Divide(_trackHits);
	neigh->Divide(_trackHits);
	dir->Divide(_trackHits);

	auto canvas = new TCanvas("comparision", "", 400*3*2, 300*3*6);
	gStyle->SetOptStat(11);
	gStyle->cd();
	canvas->Divide(2, 6);
	canvas->cd(1);
	_trackHits->Draw("COLZ");

	canvas->cd(2);
	auto run = _runlist.getByMpaRun(getAllRunIds()[0]);
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
	_efficiency->Draw("COLZ");
	canvas->cd(4);
	eff->Draw("COLZ");

	canvas->cd(5);
	_neighbourHits->Draw("COLZ");
	canvas->cd(6);
	neigh->Draw("COLZ");

	canvas->cd(7);
	_directHits->Draw("COLZ");
	canvas->cd(8);
	dir->Draw("COLZ");

	canvas->cd(9);
	_trackHitsOverlayed->Draw("COLZ");
	canvas->cd(10);
	_efficiencyOverlayed->Draw("COLZ");

	canvas->cd(11);
	auto overlayeff = dynamic_cast<TH2D*>(_efficiencyOverlayed->Clone("efficiencyOverlayedScaled"));
	overlayeff->Divide(_trackHitsOverlayed);
	overlayeff->Draw("COLZ");
*/

	auto img = TImage::Create();
//	img->FromPad(canvas);
	img->WriteImage(getFilename("_results.png").c_str());
	delete img;
}

