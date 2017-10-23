#include "alibava_dqm.h"

REGISTER_ANALYSIS_TYPE(AlibavaDQM, "Execute simple DQM analysis")

AlibavaDQM::AlibavaDQM()
{
}

AlibavaDQM::~AlibavaDQM()
{
}

void AlibavaDQM::init()
{
	_file = new TFile("testoutput.root", "RECREATE");
	_refAliCorX = new TH2F("ref_ali_cor_x", "REF <-> Alibava Correlation X",
			       ALIBAVA_N, 0, ALIBAVA_N,
			       MIMOSA_N_X, 0, MIMOSA_N_X);
	_refAliCorY = new TH2F("ref_ali_cor_y", "REF <-> Alibava Correlation Y",
			       ALIBAVA_N, 0, ALIBAVA_N,
			       MIMOSA_N_Y, 0, MIMOSA_N_Y);
}

void AlibavaDQM::run(const core::alibava_run_data_t& run)
{
	auto refPlane = (&run.telescopeData->p1) + 6;

	for(size_t evt = 0; evt < run.tree->GetEntries(); ++evt) 
	{
		run.tree->GetEntry(evt);
		
		for(size_t refHit = 0; refHit < refPlane->x.GetNoElements(); 
		    ++refHit) 
		{
			for(size_t aliHit = 0; 
			    aliHit < run.alibavaData->center.GetNoElements(); 
			    ++aliHit) 
			{
				_refAliCorX->Fill(run.alibavaData->center[aliHit], 
						  refPlane->y[refHit]);
				_refAliCorY->Fill(run.alibavaData->center[aliHit], 
						  refPlane->x[refHit]);								
			} 
		}
	}
}

void AlibavaDQM::finalize()
{
	if(_file) {
		_file->Write();
		_file->Close();
	}
}
