
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
 _currentDutResX(nullptr), _currentDutResY(nullptr),
 _trackHitCount(0), _realHitCount(0)
{
}

MpaTripletEfficiency::~MpaTripletEfficiency()
{
}

void MpaTripletEfficiency::init()
{
	_file = new TFile(getRootFilename().c_str(), "recreate");
	_resCutX = _config.get<double>("triplet_efficiency_res_x");
	_resCutY = _config.get<double>("triplet_efficiency_res_y");
	_trackConsts.angle_cut = _config.get<double>("angle_cut");
	_trackConsts.upstream_residual_cut = _config.get<double>("upstream_residual_cut");
	_trackConsts.downstream_residual_cut = _config.get<double>("downstream_residual_cut");
	_trackConsts.six_residual_cut = _config.get<double>("six_residual_cut");
	_trackConsts.six_kink_cut = _config.get<double>("six_kink_cut");
	_trackConsts.ref_residual_precut = _config.get<double>("ref_residual_precut");
	_trackConsts.ref_residual_cut = _config.get<double>("ref_residual_cut");
	_trackConsts.dut_residual_cut_x = _config.get<double>("dut_residual_cut_x");
	_trackConsts.dut_residual_cut_y = _config.get<double>("dut_residual_cut_y");
	_trackConsts.dut_offset = Eigen::Vector3d({
		_config.get<double>("dut_x"),
		_config.get<double>("dut_y"),
		_config.get<double>("dut_z")
		});
	_trackConsts.dut_rotation = Eigen::Vector3d({
		_config.get<double>("dut_phi"),
		_config.get<double>("dut_theta"),
		_config.get<double>("dut_omega")
		}) * M_PI / 180;
	_trackConsts.dut_plateau_x = _config.get<int>("dut_plateau_x") > 0;
	_trackHits = new TH2F("track_hits", "Tracks passing the MaPSA",
	                      160, 0, 16,
			      60, 0, 3);
	_realHits = new TH2F("real_hits", "Registered Tracks",
	                      160, 0, 16,
			      60, 0, 3);
	_pixelHits = new TH2F("pixel_hits", "Hits on MPA before clustering",
	                      160, 0, 16,
			      60, 0, 3);
	_clusterHits = new TH2F("cluster_hits", "Hits on MPA after clustering",
	                      160, 0, 16,
			      60, 0, 3);
	_fakeHits = new TH2F("fake_hits", "Formerly Shit-Tracks",
	                      160, 0, 16,
			      60, 0, 3);
	_overlayedTrackHits = new TH2F("overlayed_track_hits", "Tracks passing the MaPSA",
	                      30, 0, 2,
			      100, 0, 1);
	_overlayedRealHits = new TH2F("overlayed_real_hits", "Registered Tracks",
	                      30, 0, 2,
			      100, 0, 1);
	_dutResX = new TH1F("dut_res_x", "", 1000, -10, -10);
	_dutResY = new TH1F("dut_res_y", "", 1000, -10, -10);
	_dutResZ = new TH1F("dut_res_z", "", 1000, -10, -10);
	int numRuns = _allRunIds.size();
	int minId = *std::min_element(_allRunIds.begin(), _allRunIds.end());
	int maxId = *std::max_element(_allRunIds.begin(), _allRunIds.end());
	_mpaHitHist = new TH1F("mpa_hit_histogram", "Number of MPA hits per run", numRuns, minId, maxId);
	_trackHist = new TH1F("track_histogram", "Number of tracks hitting the MPA per run", numRuns, minId, maxId);
	_mpaActivationHist = new TH1F("mpa_activation_hist", "Number of activated pixels per run", numRuns, minId, maxId);
	_clusterSize = new TH1F("cluster_size", "Cluster sizes in MPA", 30, 0, 30);
}

void MpaTripletEfficiency::run(const core::run_data_t& run)
{
	loadCurrentAlignment();

	_maskedPixels.clear();
	core::MpaTransform transform;
	for(auto pixelIdx: _config.getVector<int>("triplet_efficiency_masked")) {
		_maskedPixels.push_back(transform.translatePixelIndex(pixelIdx));
	}
	_fiducialMin(0) = _config.get<double>("triplet_efficiency_fiducial_min_x");
	_fiducialMin(1) = _config.get<double>("triplet_efficiency_fiducial_min_y");
	_fiducialMax(0) = _config.get<double>("triplet_efficiency_fiducial_max_x");
	_fiducialMax(1) = _config.get<double>("triplet_efficiency_fiducial_max_y");

	std::string dir("run_");
	dir += std::to_string(_currentRunId);
	_file->mkdir(dir.c_str());
	_file->cd(dir.c_str());
	_currentDutResX = new TH1F("dut_res_x", "", 200, -10, -10);
	_currentDutResY = new TH1F("dut_res_y", "", 200, -10, -10);
	_currentDutResZ = new TH1F("dut_res_z", "", 200, -10, -10);
	std::cout << "Find tracks in datafile" << std::endl;
	auto hists = core::TripletTrack::genDebugHistograms();
	auto tracks = core::TripletTrack::getTracksWithRefDut(_trackConsts, run, &hists, nullptr, nullptr, false);
	size_t trackIdx = 0;
	transform.setOffset(_dutAlignOffset);
	transform.setRotation(_trackConsts.dut_rotation);
	std::cout << "Track particles to DUT" << std::endl;
//	std::ofstream fout(getFilename("_hits.csv"));
	for(size_t evt = 0; evt < run.tree->GetEntries(); ++evt) {
		run.tree->GetEntry(evt);
		auto pixelHits = core::MpaHitGenerator::getCounterPixels(run, transform);
		std::vector<int> clusterSizes;
		auto clusterHits = core::MpaHitGenerator::clusterize(pixelHits, &clusterSizes, nullptr);
		_mpaActivationHist->Fill(_currentRunId, pixelHits.size());
		for(int cs: clusterSizes) {
			_clusterSize->Fill(cs);
		}
		for(const auto& mpaHit: clusterHits) {
			_clusterHits->Fill(mpaHit(0), mpaHit(1));
		}
		for(const auto& mpaHit: pixelHits) {
			_pixelHits->Fill(mpaHit(0), mpaHit(1));
		}
		for(; trackIdx < tracks.size() && tracks[trackIdx].first.getEventNo() < evt; ++trackIdx) {
//			std::cout << " Skipping track " << trackIdx
//			          << "  event " << evt
//				  << " (" << tracks[trackIdx].first.getEventNo() << ")"
//				  << std::endl;
		}
		for(; trackIdx < tracks.size() && tracks[trackIdx].first.getEventNo() == evt; ++trackIdx) {
//			std::cout << "Accepting track " << trackIdx
//			          << "  event " << evt
//				  << " (" << tracks[trackIdx].first.getEventNo() << ")"
//				  << std::endl;
			auto track = tracks[trackIdx].first;
			for(const auto& mpaHit: clusterHits) {
				auto plane_hit = transform.mpaPlaneTrackIntersect(track.upstream());
				auto hit = transform.pixelCoordToGlobal(mpaHit);
				Eigen::Vector3d res = plane_hit - hit;
				_currentDutResX->Fill(res(0));
				_currentDutResY->Fill(res(1));
				_currentDutResZ->Fill(res(2));
			}
			calcTrack(track, clusterHits, transform, run);
//			for(auto hitpoint: core::MpaHitGenerator::getCounterHits(run, transform)) {
//				fout << hitpoint(0) << " " << hitpoint(1) << " " << hitpoint(2)+0.2 << "\n";
//			}
		}
//		fout << "\n";
	}
	_runIdsDouble.push_back(_currentRunId);
	_meanResX.push_back(_currentDutResX->GetMean());
	_meanResY.push_back(_currentDutResY->GetMean()), nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr;
}

void MpaTripletEfficiency::finalize()
{
	_file->cd();
	auto meanResX = new TGraph(_runIdsDouble.size(), &_runIdsDouble[0], &_meanResX[0]);
	auto meanResY = new TGraph(_runIdsDouble.size(), &_runIdsDouble[0], &_meanResY[0]);
	meanResX->Write("mean_res_x");
	meanResY->Write("mean_res_y");
	auto canvas = new TCanvas("canvas", "");
	canvas->Divide(2, 1);
	canvas->cd(1);
	meanResX->Draw("AP");
	canvas->cd(2);
	meanResY->Draw("AP");
	auto img = TImage::Create();
	img->FromPad(canvas);
	img->WriteImage(getFilename(".png").c_str());
	delete img;
	auto efficiency = (TH2F*)_realHits->Clone("efficiency");
	efficiency->Divide(_trackHits);
	efficiency->SetTitle("MaPSA Efficiency Map");
	auto overlayed_efficiency = (TH2F*)_overlayedRealHits->Clone("overlayed_efficiency");
	overlayed_efficiency->Divide(_overlayedTrackHits);
	overlayed_efficiency->SetTitle("MaPSA Pixel Map");
	std::ofstream fefficiency(getFilename("_efficiency.txt"));
	double eff = (double)_realHitCount / (double)_trackHitCount;
	fefficiency << "# Total\tHits\tEfficiency\n"
	            << _trackHitCount << "\t" << _realHitCount << "\t" << eff << "\n";
	fefficiency.flush();
	fefficiency.close();
	std::cout << "Efficiency: " << eff*100 << " %" << std::endl;
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
			_dutAlignOffset(2) = value;
		}
	}
	_trackConsts.ref_prealign = _refAlignOffset;
	_trackConsts.dut_offset = _dutAlignOffset;
	std::cout << "Ref Alignment:\n" << _refAlignOffset << std::endl;
	std::cout << "Dut Alignment:\n" << _dutAlignOffset << std::endl;
}

void MpaTripletEfficiency::calcTrack(core::TripletTrack track, std::vector<Eigen::Vector2d> mpaHits, core::MpaTransform transform, core::run_data_t run)
{
	try { 
		auto hitpoint = transform.mpaPlaneTrackIntersect(track.upstream());
		Eigen::Vector2d pc = transform.globalToPixelCoord(hitpoint);
		Eigen::Vector2d overlay_pc(fmod(pc(0), 2), fmod(pc(1), 1));
		auto pixelIdx = transform.pixelCoordToIndex(pc.cast<int>());
		if(pc(0) < _fiducialMin(0) || pc(0) > _fiducialMax(0) ||
		   pc(1) < _fiducialMin(1) || pc(1) > _fiducialMax(1)) {
			return;
		}
		for(auto maskedPc: _maskedPixels) {
			if((int)pc(0) == maskedPc(0) && (int)pc(1) == maskedPc(1)) {
				return;
			}
		}
		_trackHits->Fill(pc(0), pc(1));
		_trackHitCount++;
		bool overlaying_pixel = true;
		if(pc(0) < 1.0 || pc(0) > 15 || pc(1) > 2) {
			overlaying_pixel = false;
		} else {
			_overlayedTrackHits->Fill(overlay_pc(0), overlay_pc(1));
		}
		_trackHist->Fill(_currentRunId);
		auto pixels = (*run.mpaData[0].data)->counter.pixels;
		for(const auto& pc2: mpaHits) {
			auto mpaHit = transform.pixelCoordToGlobal(pc2);
//			if(((pc - pc2).array().abs() > Eigen::Array2d{1.5, 1.5}).any()) {
//				continue;
//			}
			Eigen::Vector3d res = hitpoint - mpaHit;
			if(std::abs(res(1)) > _resCutY ||
			   std::abs(res(0)) > _resCutX) {
				continue;
			}
			_realHits->Fill(pc(0), pc(1));
			_mpaHitHist->Fill(_currentRunId);
			if(overlaying_pixel) {
				_overlayedRealHits->Fill(overlay_pc(0), overlay_pc(1));
			}
			_dutResX->Fill(res(0));
			_dutResY->Fill(res(1));
			_dutResZ->Fill(res(2));
			_realHitCount++;
			break;
		}
	} catch(std::out_of_range& e) {
	}
}
