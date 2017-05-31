

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
	virtual void run(const core::MergedAnalysis::run_data_t& run);
	virtual void finalize();

	static Eigen::MatrixXd jacobianStep(double step);
	static Eigen::MatrixXd getDerivatives(core::Triplet t, double dut_z, Eigen::Vector3d angles);

private:
	std::vector<core::TripletTrack> getTrackCandidates(size_t maxCandidates, const core::MergedAnalysis::run_data_t& run);
	void fitTracks(std::vector<core::TripletTrack> trackCandidates);
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

	TH1F* _down_angle_x;
	TH1F* _down_angle_y;
	TH1F* _down_res_x;
	TH1F* _down_res_y;
	TH1F* _up_angle_x;
	TH1F* _up_angle_y;
	TH1F* _up_res_x;
	TH1F* _up_res_y;
	TH1F* _ref_down_res_x;
	TH1F* _ref_down_res_y;
	TH1F* _dut_up_res_x;
	TH1F* _dut_up_res_y;
	TH1F* _track_kink_x;
	TH1F* _track_kink_y;
	TH1F* _track_residual_x;
	TH1F* _track_residual_y;
	TH1F* _planes_z;
	TH1F* _candidate_res_track_x;
	TH1F* _candidate_res_track_y;
	TH1F* _candidate_res_ref_x;
	TH1F* _candidate_res_ref_y;
	TH1F* _gbl_chi2_dist;
};

#endif//GBL_ALIGN_H
