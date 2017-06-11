
#include "mpatripletefficiency.h"
#include "mpahitgenerator.h"
#include <iostream>
#include <TImage.h>
#include <TCanvas.h>
#include <TFitResult.h>
#include <fstream>
#include "mpatransform.h"
#include <TF1.h>
#include <algorithm>
#include <TGraph.h>
#include <cmath>

REGISTER_ANALYSIS_TYPE(MpaTripletEfficiency, "Calculate MPA Efficiency based on triplet-tracks")

MpaTripletEfficiency::MpaTripletEfficiency() :
 _currentDutResX(nullptr), _currentDutResY(nullptr)
{
}

MpaTripletEfficiency::~MpaTripletEfficiency()
{
}

void MpaTripletEfficiency::init()
{
	_file = new TFile(getRootFilename().c_str(), "recreate");
	_trackConsts.angle_cut = 0.16;
	_trackConsts.upstream_residual_cut = 0.1;
	_trackConsts.downstream_residual_cut = 0.1;
	_trackConsts.six_residual_cut = 0.1;
	_trackConsts.six_kink_cut = 0.01;
	_trackConsts.ref_residual_precut = 0.7;
	_trackConsts.ref_residual_cut = 0.1;
	_trackConsts.dut_residual_cut_x = 0.6;
	_trackConsts.dut_residual_cut_y = 0.07;
	_trackConsts.dut_z = 385;
	_trackConsts.dut_rot = 0;
	_trackConsts.dut_plateau_x = true;
	_trackHits = new TH2F("track_hits", "Tracks passing the MaPSA",
	                      160, 0, 16,
			      30, 0, 3);
	_realHits = new TH2F("real_hits", "Registered Tracks",
	                      160, 0, 16,
			      30, 0, 3);
	_fakeHits = new TH2F("fake_hits", "Formerly Shit-Tracks",
	                      160, 0, 16,
			      30, 0, 3);
	_overlayedTrackHits = new TH2F("overlayed_track_hits", "Tracks passing the MaPSA",
	                      60, 0, 2,
			      200, 0, 1);
	_overlayedRealHits = new TH2F("overlayed_real_hits", "Registered Tracks",
	                      60, 0, 2,
			      200, 0, 1);
	_dutResX = new TH1F("dut_res_x", "", 1000, -10, -10);
	_dutResY = new TH1F("dut_res_y", "", 1000, -10, -10);
	int numRuns = _allRunIds.size();
	int minId = *std::min_element(_allRunIds.begin(), _allRunIds.end());
	int maxId = *std::max_element(_allRunIds.begin(), _allRunIds.end());
	_mpaHitHist = new TH1F("mpa_hit_histogram", "Number of MPA hits per run", numRuns, minId, maxId);
	_trackHist = new TH1F("track_histogram", "Number of tracks hitting the MPA per run", numRuns, minId, maxId);
	_mpaActivationHist = new TH1F("mpa_activation_hist", "Number of activated pixels per run", numRuns, minId, maxId);
}

void MpaTripletEfficiency::run(const core::run_data_t& run)
{
	loadCurrentAlignment();
	std::string dir("run_");
	dir += std::to_string(_currentRunId);
	_file->mkdir(dir.c_str());
	_file->cd(dir.c_str());
	_currentDutResX = new TH1F("dut_res_x", "", 1000, -10, -10);
	_currentDutResY = new TH1F("dut_res_y", "", 1000, -10, -10);
	std::cout << "Find tracks in datafile" << std::endl;
	auto hists = core::TripletTrack::genDebugHistograms();
	auto tracks = core::TripletTrack::getTracksWithRefDut(_trackConsts, run, hists, nullptr, nullptr, false);
	size_t trackIdx = 0;
	core::MpaTransform transform;
	transform.setOffset(-_dutAlignOffset);
	transform.setRotation({_trackConsts.dut_rot, 0, 3.1415 / 180 * 90});
	std::cout << "Track particles to DUT" << std::endl;
//	std::ofstream fout(getFilename("_hits.csv"));
	for(size_t evt = 0; evt < run.tree->GetEntries(); ++evt) {
		run.tree->GetEntry(evt);
		auto mpaHits = core::MpaHitGenerator::getCounterHits(run, transform);
		_mpaActivationHist->Fill(_currentRunId, mpaHits.size());
		for(; trackIdx < tracks.size() && tracks[trackIdx].first.getEventNo() < evt; ++trackIdx) {
//			std::cout << " Skipping track " << trackIdx
//			          << "  event " << evt
//				  << " (" << tracks[trackIdx].first.getEventNo() << ")"
//				  << std::endl;
		}
		for(; trackIdx < tracks.size() && tracks[trackIdx].first.getEventNo() == evt; ++trackIdx) {
		//	std::cout << "Accepting track " << trackIdx
		//	          << "  event " << evt
		//		  << " (" << tracks[trackIdx].first.getEventNo() << ")"
		//		  << std::endl;
			auto track = tracks[trackIdx].first;
			calcTrack(track, mpaHits, transform, run);
//			for(auto hitpoint: core::MpaHitGenerator::getCounterHits(run, transform)) {
//				fout << hitpoint(0) << " " << hitpoint(1) << " " << hitpoint(2)+0.2 << "\n";
//			}
		}
//		fout << "\n";
	}
	_runIdsDouble.push_back(_currentRunId);
	_meanResX.push_back(_currentDutResX->GetMean());
	_meanResY.push_back(_currentDutResY->GetMean());
}

void MpaTripletEfficiency::finalize()
{
	_file->cd();
	auto meanResX = new TGraph(_runIdsDouble.size(), &_runIdsDouble[0], &_meanResX[0]);
	auto meanResY = new TGraph(_runIdsDouble.size(), &_runIdsDouble[0], &_meanResY[0]);
	meanResX->Write("mean_res_x");
	meanResY->Write("mean_res_y");
	auto efficiency = (TH2F*)_realHits->Clone("efficiency");
	efficiency->Divide(_trackHits);
	efficiency->SetTitle("MaPSA Efficiency Map");
	auto overlayed_efficiency = (TH2F*)_overlayedRealHits->Clone("overlayed_efficiency");
	overlayed_efficiency->Divide(_overlayedTrackHits);
	overlayed_efficiency->SetTitle("MaPSA Pixel Map");
	if(_file) {
		_file->Write();
		_file->Close();
	}
}

void MpaTripletEfficiency::loadCurrentAlignment()
{
	auto filename = getFilename("GblAlign", "_alignment.txt", false, false);
	std::cout << "Load alignment file " << filename << std::endl;
	std::ifstream fin(filename);
	while(fin.good()) {
		int label;
		double value;
		fin >> label >> value;
		if(label == 1) {
			_refAlignOffset(0) = value;
		} else if(label == 2) {
			_refAlignOffset(1) = value;
		} else if(label == 3) {
			_refAlignOffset(2) = value;
		} else if(label == 11) {
			_dutAlignOffset(0) = value;
		} else if(label == 12) {
			_dutAlignOffset(1) = value;
		} else if(label == 13) {
			_dutAlignOffset(2) = -value;
		}
	}
	_trackConsts.ref_prealign = _refAlignOffset;
	_trackConsts.dut_prealign = _dutAlignOffset;
	std::cout << "Ref Alignment:\n" << _refAlignOffset << std::endl;
	std::cout << "Dut Alignment:\n" << _dutAlignOffset << std::endl;
}

void MpaTripletEfficiency::calcTrack(core::TripletTrack track, std::vector<Eigen::Vector3d> mpaHits, core::MpaTransform transform, core::run_data_t run)
{
	for(auto hit: mpaHits) {
		_dutResX->Fill(track.upstream().getdx(hit));
		_dutResY->Fill(track.upstream().getdy(hit));
		_currentDutResX->Fill(track.upstream().getdx(hit));
		_currentDutResY->Fill(track.upstream().getdy(hit));
	}
	try { 
		auto hitpoint = transform.mpaPlaneTrackIntersect(track.upstream());
		Eigen::Vector2d pc = transform.globalToPixelCoord(hitpoint);
		Eigen::Vector2d overlay_pc(fmod(pc(0), 2), fmod(pc(1), 1));
		auto pixelIdx = transform.pixelCoordToIndex(pc.cast<int>());
		_trackHits->Fill(pc(0), pc(1));
		bool overlaying_pixel = true;
		if(pc(0) < 1.0 || pc(0) > 15 || pc(1) > 2) {
			overlaying_pixel = false;
		} else {
			_overlayedTrackHits->Fill(overlay_pc(0), overlay_pc(1));
		}
		_trackHist->Fill(_currentRunId);
		auto pixels = (*run.mpaData[0].data)->counter.pixels;
		for(size_t idx = 0; idx < 48; ++idx) {
			if(pixels[pixelIdx] == 0) {
				continue;
			}
			Eigen::Vector2d pc2 = transform.translatePixelIndex(idx).cast<double>() + Eigen::Vector2d{0.5, 0.5};
			Eigen::Vector3d gc2 = transform.pixelCoordToGlobal(pc2);
			if(((pc - pc2).array().abs() > Eigen::Array2d{3, 2}).any()) {
				continue;
			}
			_realHits->Fill(pc(0), pc(1));
			_mpaHitHist->Fill(_currentRunId);
			if(overlaying_pixel) {
				_overlayedRealHits->Fill(overlay_pc(0), overlay_pc(1));
			}
			break;
		}
	} catch(std::out_of_range& e) {
	}
}
