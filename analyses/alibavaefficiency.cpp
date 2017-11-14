#include "alibavaefficiency.h"
#include "functions.h"
#include "triplet.h"
#include <sstream>
#include <iomanip>

#include "TF1.h"

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
	oss << _config.get<std::string>("output_dir") << "/run" 
	    << std::setfill('0') << std::setw(_config.get<int>("id_padding")) 
	    << _currentRunId << ".root";
	// mpa_id_padding seems to be hard coded 
	//_file = new TFile(getRootFilename().c_str(), "RECREATE");
	_file = new TFile(oss.str().c_str(), "RECREATE");
	
	_histoBin = _config.get<int>("histo_binning");
	_histoMin = _config.get<int>("histo_min");
	_histoMax = _config.get<int>("histo_max");
	_stepSize = _histoBin/(_histoMax - _histoMin);

	_projMin = _config.get<int>("proj_min");
	_projMax = _config.get<int>("proj_max");

	_refAliCorX = new TH2F("ref_ali_cor_x", "REF <-> Alibava Correlation X",
			       ALIBAVA_N, 0, ALIBAVA_N,
			       FEI4_N_X, 0, FEI4_N_X);
	_refAliCorY = new TH2F("ref_ali_cor_y", "REF <-> Alibava Correlation Y",
			       ALIBAVA_N, 0, ALIBAVA_N,
			       FEI4_N_Y, 0, FEI4_N_Y);
	_dutTracks = new TH2F("dut_tracks", "Track hits at z(DUT)", 
			      _histoBin, _histoMin, _histoMax, 
			      _histoBin, _histoMin, _histoMax);
	_dutHits = new TH2F("dut_hits", "Tracks hits with DUT hits", 
			    _histoBin, _histoMin, _histoMax, 
			    _histoBin, _histoMin, _histoMax);
	_dutTracksInTime = new TH2F("dut_tracks_intime", "Track hits at z(DUT)", 
				    _histoBin, _histoMin, _histoMax, 
				    _histoBin, _histoMin, _histoMax);
	_dutHitsInTime = new TH2F("dut_hits_intime", "Track hits with DUT hits", 
				  _histoBin, _histoMin, _histoMax, 
				  _histoBin, _histoMin, _histoMax);

	_dutResX = new TH1D("dut_res_x", "DUT residual in x", 100, -5, 5);
	_dutResY = new TH1D("dut_res_y", "DUT residual in y", 100, -5, 5);
	
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
			auto hit = track.upstream().extrapolate(_config.get<double>("dut_z"));
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
				
				for(size_t iCluster=0; iCluster < ali->center.GetNoElements(); ++iCluster)
				{
					_dutResX->Fill(posX(ali->center[iCluster])-hit[0]); 
					_dutResY->Fill(posX(ali->center[iCluster])-hit[1]); 
				}
			}			
		}
	}

	auto _dutEff = (TH2F*)_dutHits->Clone("efficiency");
	_dutEff->Divide(_dutTracks);
	_dutEffX = _dutEff->ProjectionX("efficiency_profileX", 
					_projMin*_stepSize, _projMax*_stepSize);
	_dutEffY = _dutEff->ProjectionY("efficiency_profileY", 
					_projMin*_stepSize, _projMax*_stepSize);

	// 1D projection
	auto _dutEffInTime = (TH2F*)_dutHitsInTime->Clone("efficiency_intime");
	_dutEffInTime->Divide(_dutTracksInTime);
	_dutEffXInTime = _dutEffInTime->ProjectionX("efficiency_intime_profileX",
						    std::abs(_histoMin - _projMin)*_stepSize, 
						    std::abs(_histoMin - _projMax)*_stepSize);
	_dutEffYInTime = _dutEffInTime->ProjectionY("efficiency_intime_profileY", 
						    std::abs(_histoMin - _projMin)*_stepSize, 
						    std::abs(_histoMin - _projMax)*_stepSize);     
	//Inefficiency projection
	_dutIneffXInTime = _dutEffInTime->ProjectionX("Inefficiency_intime_profileX", 
						      std::abs(_histoMin - _projMin)*_stepSize, 
						      std::abs(_histoMin - _projMax)*_stepSize);
	_dutIneffYInTime = _dutEffInTime->ProjectionY("Inefficiency_intime_profileY", 
						      std::abs(_histoMin - _projMin)*_stepSize, 
						      std::abs(_histoMin - _projMax)*_stepSize);

}

void AlibavaEfficiency::finalize()
{
	// Normalize
	for(size_t iBin=0; iBin<_histoBin; iBin++) {
		_dutEffX->SetBinContent(iBin, _dutEffX->GetBinContent(iBin)/(static_cast<int>((_projMax-_projMin)*_stepSize+1)+1));
		_dutEffY->SetBinContent(iBin, _dutEffY->GetBinContent(iBin)/(static_cast<int>((_projMax-_projMin)*_stepSize+1)+1));
		_dutEffXInTime->SetBinContent(iBin, _dutEffXInTime->GetBinContent(iBin)/(static_cast<int>((_projMax-_projMin)*_stepSize)+1));
		_dutEffYInTime->SetBinContent(iBin, _dutEffYInTime->GetBinContent(iBin)/(static_cast<int>((_projMax-_projMin)*_stepSize)+1));
		_dutIneffXInTime->SetBinContent(iBin, 1.-_dutEffXInTime->GetBinContent(iBin));
		_dutIneffYInTime->SetBinContent(iBin, 1.-_dutEffYInTime->GetBinContent(iBin));
	}

	TF1 *fitFunc = new TF1("scurve", core::scurve, 
			       _config.get<double>("fit_min"), 
			       _config.get<double>("fit_max"), 
			       3);
	fitFunc->SetParameters(0.7, -0.55, 0.05);
	_dutIneffYInTime->Fit("scurve", "R"); 

	if(_file) {
		_file->Write();
		_file->Close();
	}
}

double AlibavaEfficiency::posX(int channel, bool swap) 
{
	if (channel < ALIBAVA_N/2.)
		return (channel-ALIBAVA_N/4.+0.5) * (ALIBAVA_PITCH/1000.);
	else
		return (0.5-3*ALIBAVA_N/4.+channel) * (ALIBAVA_PITCH/1000.);
}
