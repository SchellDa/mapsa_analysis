#ifndef ALIBAVA_DQM_H
#define ALIBAVA_DQM_H

#include "alibavaanalysis.h"
#include "TH2F.h"

#define MIMOSA_N_X 1152
#define MIMOSA_N_Y 576
#define FEI4_N_X 80
#define FEI4_N_Y 336
#define ALIBAVA_N 256

class AlibavaDQM : public core::AlibavaAnalysis
{
public:
	AlibavaDQM();
	virtual ~AlibavaDQM();
	
	virtual void init();
	virtual void run(const core::alibava_run_data_t& run);
	virtual void finalize();

private:
	TFile* _file;
	TH2F* _refAliCorX;
	TH2F* _refAliCorY;

};

#endif//ALIBAVA_DQM_H
