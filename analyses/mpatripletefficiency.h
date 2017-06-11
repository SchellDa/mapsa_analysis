
#ifndef MPA_TRIPLET_EFFICIENCY_H
#define MPA_TRIPLET_EFFICIENCY_H

#include "mergedanalysis.h"
#include <TH1F.h>
#include <TH2F.h>
#include "triplettrack.h"

class MpaTripletEfficiency : public core::MergedAnalysis
{
public:
	MpaTripletEfficiency();
	virtual ~MpaTripletEfficiency();

	virtual void init();
	virtual void run(const core::run_data_t& run);
	virtual void finalize();

private:
	void loadCurrentAlignment();
	void calcTrack(core::TripletTrack track, std::vector<Eigen::Vector3d> mpaHits, core::MpaTransform transform, core::run_data_t run);
	TFile* _file;
	core::TripletTrack::constants_t _trackConsts;
	Eigen::Vector3d _refAlignOffset;
	Eigen::Vector3d _dutAlignOffset;
	TH2F* _trackHits;
	TH2F* _realHits;
	TH2F* _fakeHits;
	TH2F* _overlayedTrackHits;
	TH2F* _overlayedRealHits;
	TH1F* _dutResX;
	TH1F* _dutResY;
	TH1F* _currentDutResX;
	TH1F* _currentDutResY;
	TH1F* _mpaHitHist;
	TH1F* _trackHist;
	TH1F* _mpaActivationHist;
	std::vector<double> _runIdsDouble;
	std::vector<double> _meanResX;
	std::vector<double> _meanResY;
};

#endif//MPA_TRIPLET_EFFICIENCY_H
