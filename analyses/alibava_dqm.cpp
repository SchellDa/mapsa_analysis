#include "alibava_dqm.h"
#include <typeinfo>

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
			       FEI4_N_X, 0, FEI4_N_X);
	_refAliCorY = new TH2F("ref_ali_cor_y", "REF <-> Alibava Correlation Y",
			       ALIBAVA_N, 0, ALIBAVA_N,
			       FEI4_N_Y, 0, FEI4_N_Y);
}

void AlibavaDQM::run(const core::alibava_run_data_t& run)
{
	for(size_t evt = 0; evt < run.tree->GetEntries(); ++evt) 
	{		
		// the order here is important!
		run.tree->GetEntry(evt);		
		auto ref = (*run.telescopeData)->ref;
		auto ali = (*run.alibavaData);

		for(size_t refHit = 0; refHit < ref.y.GetNoElements(); refHit++) 
		{			
			for(size_t aliHit = 0; 
			    aliHit < ali->center.GetNoElements(); aliHit++) 
			{
				_refAliCorX->Fill(ali->center[aliHit], 
						  ref.x[refHit]);
				_refAliCorY->Fill(ali->center[aliHit], 
						  ref.y[refHit]);								
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
