#ifndef ALIBAVA_EFFICIENCY_H
#define ALIBAVA_EFFICIENCY_H

#include "alibavaanalysis.h"
#include "triplettrack.h"

#include "TH2F.h"
#include "TProfile.h"
#include "TGraph.h"

#define MIMOSA_N_X 1152
#define MIMOSA_N_Y 576
#define FEI4_N_X 80
#define FEI4_N_Y 336
#define ALIBAVA_N 256

class AlibavaEfficiency : public core::AlibavaAnalysis
{
public:
	AlibavaEfficiency();
	virtual ~AlibavaEfficiency();
	
	virtual void init();
	virtual void run(const core::run_data_t& run);
	virtual void finalize();

private:
	TFile* _file;
	TH2F* _refAliCorX;
	TH2F* _refAliCorY;
	TH2F* _dutTracks;
	TH2F* _dutHits;
	TH2F* _dutTracksInTime;
	TH2F* _dutHitsInTime;
	TH2F* _dutEffInTime;
	TProfile* _dutProfileX;
	TProfile* _dutProfileY;
	TH1D* _dutEffX;
	TH1D* _dutEffY;
	TH1D* _dutEffXInTime;
	TH1D* _dutEffYInTime;
	TH1D* _dutIneffXInTime;
	TH1D* _dutIneffYInTime;
	core::TripletTrack::constants_t _trackConsts;

	int _histoBin;
	int _histoMin;
	int _histoMax;
	int _projMin;
	int _projMax;
	double _stepSize;
};

#endif//ALIBAVA_EFFICIENCY_H
