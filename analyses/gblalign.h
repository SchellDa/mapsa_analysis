

#ifndef GBL_ALIGN_H
#define GBL_ALIGN_H

#include "mergedanalysis.h"
#include <TH1F.h>
#include <array>
#include "triplettrack.h"

namespace gbl {
	class GblTrajectory;
}


class GblAlign : public core::MergedAnalysis
{
public:
	GblAlign();
	virtual ~GblAlign();

	virtual void init();
	virtual void run(const core::run_data_t& run);
	virtual void finalize();

	static Eigen::MatrixXd jacobianStep(double step);
	static Eigen::MatrixXd getDerivatives(core::Triplet t, double dut_z, Eigen::Vector3d angles);

private:
	void fitTracks(std::vector<std::pair<core::TripletTrack, Eigen::Vector3d>> trackCandidates);
	Eigen::Vector3d calcFitDebugHistograms(int planeId, gbl::GblTrajectory* traj);
	void loadPrealignment();
	void loadResolutions();
	TFile* _file;
	Eigen::Vector3d _refPreAlign;
	Eigen::Vector3d _dutPreAlign;
	Eigen::Vector2d _precisionTel;
	Eigen::Vector2d _precisionRef;
	Eigen::Vector2d _precisionMpa;
	double _eBeam;

	core::TripletTrack::histograms_t _trackHists;
	core::TripletTrack::constants_t _trackConsts;
	TH1F* _gbl_chi2_dist;
};

#endif//GBL_ALIGN_H
