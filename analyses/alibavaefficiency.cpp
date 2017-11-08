#include "alibavaefficiency.h"
#include "triplet.h"
#include <sstream>

REGISTER_ANALYSIS_TYPE(AlibavaEfficiency, "Perform alibava test beam analysis")

AlibavaEfficiency::AlibavaEfficiency()
{
}

AlibavaEfficiency::~AlibavaEfficiency()
{
}

void AlibavaEfficiency::init()
{
	std::ostringstream oss;
	oss << "run" << _currentRunId << ".root";
	// mpa_id_padding seems to be hard coded 
	//_file = new TFile(getRootFilename().c_str(), "RECREATE");
	_file = new TFile(oss.str().c_str(), "RECREATE");
	
	_refAliCorX = new TH2F("ref_ali_cor_x", "REF <-> Alibava Correlation X",
			       ALIBAVA_N, 0, ALIBAVA_N,
			       FEI4_N_X, 0, FEI4_N_X);
	_refAliCorY = new TH2F("ref_ali_cor_y", "REF <-> Alibava Correlation Y",
			       ALIBAVA_N, 0, ALIBAVA_N,
			       FEI4_N_Y, 0, FEI4_N_Y);

	_dutTracks = new TH2F("dut_tracks", "Track hits at z(DUT)", 
			   400, -5, 5, 
			   400, -5, 5);
	_dutHits = new TH2F("dut_hits", "Tracks hits with DUT hits", 
			   400, -5, 5, 
			   400, -5, 5);

	_dutTracksInTime = new TH2F("dut_tracks_intime", "Track hits at z(DUT)", 
			   400, -5, 5, 
			   400, -5, 5);

	_dutHitsInTime = new TH2F("dut_hits_intime", "Track hits with DUT hits", 
			   400, -5, 5, 
			   400, -5, 5);


	// Track cuts
	_trackConsts.angle_cut = _config.get<double>("angle_cut");
	_trackConsts.upstream_residual_cut = _config.get<double>("upstream_residual_cut");
	_trackConsts.downstream_residual_cut = _config.get<double>("downstream_residual_cut");
	_trackConsts.six_residual_cut = _config.get<double>("six_residual_cut");
	_trackConsts.six_kink_cut = _config.get<double>("six_kink_cut");
	
	// Ref cuts
	_trackConsts.ref_residual_precut = _config.get<double>("ref_residual_precut");
	_trackConsts.ref_residual_cut = _config.get<double>("ref_residual_cut");

	// Dut cuts and alignment
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

	std::cout << "Initialized" << std::endl;
}

void AlibavaEfficiency::run(const core::run_data_t& run)
{
	auto hists = core::TripletTrack::genDebugHistograms();
	auto tracks = core::TripletTrack::getTracksWithRef(_trackConsts, run, 
							   &hists, nullptr);

	size_t trackIdx = 0;
	for(size_t evt = 0; evt < run.tree->GetEntries(); ++evt) 
	{						
		// Assigning planes before loading the event
		// leads to a shift of the datastreams (pointer problem?)
		run.tree->GetEntry(evt);		
		auto ali  = (*run.alibavaData);
		
		for(; trackIdx < tracks.size() && tracks[trackIdx].getEventNo() < evt; ++trackIdx) {}
		for(; trackIdx < tracks.size() && tracks[trackIdx].getEventNo() == evt; ++trackIdx) {
			auto track = tracks[trackIdx];
			auto hit = track.upstream().extrapolate(370);
			_dutTracks->Fill(hit[0], hit[1]);			

			// Very simple efficiency calculation
			// Just check if there is ANY hit in the DUT
			if(ali->center.GetNoElements() != 0) {
				_dutHits->Fill(hit[0], hit[1]);			
			}
			
			// Check timing (30 - 70)
			if(ali->time[0] >= 30 && ali->time[0] <= 70) {
				_dutTracksInTime->Fill(hit[0], hit[1]);
				
				if(ali->center.GetNoElements() != 0) {
					_dutHitsInTime->Fill(hit[0], hit[1]);
				}
			}
			
		}
	}

	auto _dutEff = (TH2F*)_dutHits->Clone("efficiency");
	_dutEff->Divide(_dutTracks);
	auto _dutEffX = _dutEff->ProfileX("efficiency_profile");

	auto _dutEffInTime = (TH2F*)_dutHitsInTime->Clone("efficiency_intime");
	_dutEffInTime->Divide(_dutTracksInTime);
	auto _dutEffXInTime = _dutEffInTime->ProfileX("efficiency_intime_profile");

}

void AlibavaEfficiency::finalize()
{
	if(_file) {
		_file->Write();
		_file->Close();
	}
}
