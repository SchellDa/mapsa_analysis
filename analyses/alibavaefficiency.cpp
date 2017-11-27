#include "alibavaefficiency.h"
#include "functions.h"
#include "triplet.h"
#include "aligner.h"
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
	    << _currentRunId;// << ".root";
	_file = new TFile((oss.str()+".root").c_str(), "RECREATE");
	_csv.open(oss.str()+".csv");
	
	_histoXBin = _config.get<int>("histo_binning_x");
	_histoYBin = _config.get<int>("histo_binning_y");
	_histoXMin = _config.get<double>("histo_min_x");
	_histoXMax = _config.get<double>("histo_max_x");
	_histoYMin = _config.get<double>("histo_min_y");
	_histoYMax = _config.get<double>("histo_max_y");

	_stepSizeX = _histoXBin/(_histoXMax - _histoXMin);
	_stepSizeY = _histoYBin/(_histoYMax - _histoYMin);

	_projMinX = _config.get<double>("proj_min_x");
	_projMaxX = _config.get<double>("proj_max_x");
	_projMinY = _config.get<double>("proj_min_y");
	_projMaxY = _config.get<double>("proj_max_y");

	_corX = new TH2F("ref_ali_cor_x", "REF <-> Alibava Correlation X",
			       100, -5, 5, 
			       100, -5, 5);
	_corY = new TH2F("ref_ali_cor_y", "REF <-> Alibava Correlation Y",
			       100, -5, 5, 
			       100, -5, 5);
	_dutTracks = new TH2F("dut_tracks", "Track hits at z(DUT)", 
			      _histoXBin, _histoXMin, _histoXMax, 
			      _histoYBin, _histoYMin, _histoYMax);
	_dutHits = new TH2F("dut_hits", "Tracks hits with DUT hits", 
			    _histoXBin, _histoXMin, _histoXMax, 
			    _histoYBin, _histoYMin, _histoYMax);
	_dutTracksInTime = new TH2F("dut_tracks_intime", "Track hits at z(DUT)", 
				    _histoXBin, _histoXMin, _histoXMax, 
				    _histoYBin, _histoYMin, _histoYMax);
	_dutHitsInTime = new TH2F("dut_hits_intime", "Track hits with DUT hits", 
				  _histoXBin, _histoXMin, _histoXMax, 
				  _histoYBin, _histoYMin, _histoYMax);
	_clusterSignal = new TH1F("cluster_signal", "Cluster signal", 
				  500, 0, 500);
	_clusterSignalCut = new TH1F("cluster_signal_cut", "Cluster signal (cutted)", 
				  500, 0, 500);
	_dutTiming = new TH1F("timing", "Timing", 25, 0, 100);
	_dutResX = new TH1D("dut_res_x", "DUT residual in x", 200, -1, 1);
	_dutResY = new TH1D("dut_res_y", "DUT residual in y", 200, -1, 1);
	_tracksPerEvent = new TH1D("tracks_per_event", "Tracks per event", 5e6, 0, 5e6);
	_dutHitsPerEvent = new TH1D("dut_hits_per_event", "DUT hits per event", 5e6, 0, 5e6);

	_dutFlip = _config.get<bool>("dut_flip");

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
	//auto tracks = core::TripletTrack::getTracksWithRef(_trackConsts, run, 
	//						   &hists, nullptr);
	/*
	auto tracks = core::TripletTrack::getTracksWithRefDut(_trackConsts, run,
							      &hists, nullptr,
							      nullptr, true,
							      _dutFlip);
	*/
	auto tracks = core::TripletTrack::getTracksWithAlibava(_trackConsts, run,
							      &hists, nullptr,
							      nullptr, true,
							      _dutFlip);
	size_t trackIdx = 0;
	for(size_t evt = 0; evt < run.tree->GetEntries(); ++evt) 
	{			
		if(evt%10000 == 0) 
			std::cout << evt << std::endl;
		// Assigning planes before loading the event
		// leads to a shift of the datastreams (pointer problem?)
		run.tree->GetEntry(evt);		
		auto ali  = (*run.alibavaData);

		// All cluster signals
		for(size_t iCluster=0; iCluster < ali->clusterSignal.GetNoElements(); ++iCluster) {
			_clusterSignal->Fill(std::abs(ali->clusterSignal[iCluster]));
		}

		
		// Limit analysis
		//if(evt < 1e6) 
		//	continue;
		if(evt > 1.5e6)
			break;
		
                for(; trackIdx < tracks.size() && std::get<0>(tracks[trackIdx]).getEventNo() < evt; ++trackIdx) {}
		for(; trackIdx < tracks.size() && std::get<0>(tracks[trackIdx]).getEventNo() == evt; ++trackIdx) {

			_tracksPerEvent->Fill(evt);
			
			auto track = std::get<0>(tracks[trackIdx]);
			auto dut = std::get<1>(tracks[trackIdx]);	
			auto alibava = std::get<2>(tracks[trackIdx]);

			// Extrapolate all tracks to DUT position 
			auto hit = track.upstream().extrapolate(_config.get<double>("dut_z"));			
			_dutTracks->Fill(hit[0], hit[1]);			
			
			// z = -1 corresponds to empty hit -> inefficiency
			if(dut(2) != -1.) {
				_dutHits->Fill(hit[0], hit[1]);	
				_dutHitsPerEvent->Fill(evt);
				_dutTiming->Fill(alibava.time[0]);
				_corX->Fill(dut(0), hit[0]);
				_corY->Fill(dut(1), hit[1]);
			}
			
			// Check timing (30 - 70)
			if(ali->time[0] >= 30 && ali->time[0] <= 70) {
				_dutTracksInTime->Fill(hit[0], hit[1]);				
				if(dut(2) != -1.) {
					auto resX = dut(0)-hit[0];
					auto resY = dut(1)-hit[1];
					_dutResX->Fill(resX); 
					_dutResY->Fill(resY);
					_dutHitsInTime->Fill(hit[0], hit[1]);			
					_clusterSignalCut->Fill(std::abs(alibava.clusterSignal[0]));
				}
			}							
		}		
	}

	std::cout << "Done with Event loop" << std::endl;
	
	auto _dutEff = (TH2F*)_dutHits->Clone("efficiency");
	_dutEff->Divide(_dutTracks);
	_dutEffX = _dutEff->ProjectionX("efficiency_profileX", 
					std::abs(_histoYMin - _projMinY)*_stepSizeY, 
					std::abs(_histoYMin - _projMaxY)*_stepSizeY);
	_dutEffY = _dutEff->ProjectionY("efficiency_profileY", 
					std::abs(_histoXMin - _projMinX)*_stepSizeX, 
					std::abs(_histoXMin - _projMaxX)*_stepSizeX);     


	// 1D projection
	auto _dutEffInTime = (TH2F*)_dutHitsInTime->Clone("efficiency_intime");
	_dutEffInTime->Divide(_dutTracksInTime);
	std::cout << "Done with Dividing" << std::endl;
	_dutEffXInTime = _dutEffInTime->ProjectionX("efficiency_intime_profileX",
						    std::abs(_histoYMin - _projMinY)*_stepSizeY, 
						    std::abs(_histoYMin - _projMaxY)*_stepSizeY);
	
	_dutEffYInTime = _dutEffInTime->ProjectionY("efficiency_intime_profileY", 
						    std::abs(_histoXMin - _projMinX)*_stepSizeX, 
						    std::abs(_histoXMin - _projMaxX)*_stepSizeX);     

	std::cout << "Done with 1D projection" << std::endl;

	for(size_t iBin=0; iBin<_dutEffYInTime->GetNbinsX(); iBin++) {
		_dutEffYInTime->SetBinContent(iBin, _dutEffYInTime->GetBinContent(iBin) /
					      (static_cast<int>((_projMaxX-_projMinX)*_stepSizeX)+1) );		
	}

	for(size_t iBin=0; iBin<_dutEffXInTime->GetNbinsX(); iBin++) {
		_dutEffXInTime->SetBinContent(iBin, _dutEffXInTime->GetBinContent(iBin) /
					      (static_cast<int>((_projMaxY-_projMinY)*_stepSizeY)+1) );		
	}
	
	std::vector<std::pair<double, std::vector<double>>> effTrans;
	for(double eff=0.0; eff<1.0; eff+=0.002) {
		std::vector<double> transitions;
		if(_dutFlip)
			transitions = core::Aligner::findTransition(_dutEffXInTime, eff);
		else
			transitions = core::Aligner::findTransition(_dutEffYInTime, eff);

		if(transitions.size() == 2) {
			effTrans.emplace_back(std::make_pair(eff, transitions));
		}
	}

	std::cout << "Done with transition scan" << std::endl;
	
	for(const auto& eff : effTrans) 
	{
		std::cout << "Eff: " << eff.first << " ";
		_csv << eff.first;
		for(const auto& trans : eff.second) {
			_csv << ", " << trans; 
			std::cout << trans << ", ";
		}
		_csv << "\n";
		std::cout << std::endl;
	}

}

void AlibavaEfficiency::finalize()
{
	// Normalize
	for(size_t iBin=0; iBin<_histoXBin; iBin++) {
		_dutEffX->SetBinContent(iBin, _dutEffX->GetBinContent(iBin)/(static_cast<int>((_projMaxY-_projMinY)*_stepSizeY+1)+1));
		//_dutEffXInTime->SetBinContent(iBin, _dutEffXInTime->GetBinContent(iBin)/(static_cast<int>((_projMaxY-_projMinY)*_stepSizeY)+1));
		//_dutIneffXInTime->SetBinContent(iBin, 1.-_dutEffXInTime->GetBinContent(iBin));
	}
	for(size_t iBin=0; iBin<_histoYBin; iBin++) {
		_dutEffY->SetBinContent(iBin, _dutEffY->GetBinContent(iBin)/(static_cast<int>((_projMaxX-_projMinX)*_stepSizeX+1)+1));
		//_dutEffYInTime->SetBinContent(iBin, _dutEffYInTime->GetBinContent(iBin)/(static_cast<int>((_projMax-_projMin)*_stepSize)+1));
		//_dutIneffYInTime->SetBinContent(iBin, 1.-_dutEffYInTime->GetBinContent(iBin));
	}

	
	// Set range
	_dutEffX->GetYaxis()->SetRangeUser(0, 1.05);
	_dutEffY->GetYaxis()->SetRangeUser(0, 1.05);
	_dutEffXInTime->GetYaxis()->SetRangeUser(0, 1.05);
	_dutEffYInTime->GetYaxis()->SetRangeUser(0, 1.05);
	//_dutIneffXInTime->GetYaxis()->SetRangeUser(0, 1.05);
	//_dutIneffYInTime->GetYaxis()->SetRangeUser(0, 1.05);


	if(_file) {
		_file->Write();
		_file->Close();
	}
	if(_csv) {
		_csv.close();
	}
}

TH1D* AlibavaEfficiency::projection(TH2F* hist, char axis, int bMin, int bMax, std::string name)
{	
	if(axis == 'y') {
		std::cout << "Project histogram on Y axis" << std::endl;
		std::cout << "Calculate mean between: " << bMin << " and " 
			  << bMax << std::endl;
		auto projection = new TH1D(name.c_str(), 
					   name.c_str(), 
					   hist->GetNbinsY(), 
					   hist->GetYaxis()->GetBinCenter(0), 
					   hist->GetYaxis()->GetBinCenter(hist->GetNbinsY()));

		for(size_t binY=0; binY<projection->GetNbinsX(); ++binY) 
		{
			double bc = 0;
			for(size_t binX=bMin; binX<bMax; ++binX)
				bc += hist->GetBinContent(binX, binY);
			std::cout << "Bin content: " << bc << std::endl;
			projection->SetBinContent(binY, bc/static_cast<double>(bMax-bMin));
		}
		return projection; 
	} else {
		return nullptr;
	}

	
}
