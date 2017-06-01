#ifndef TRIPLET_TRACK_H
#define TRIPLET_TRACK_H

#include "triplet.h"
#include <TH1F.h>

namespace core
{

class TripletTrack
{
public:
	TripletTrack(Triplet up, Triplet down) :
	 _upstream(up), _downstream(down), _hasRef(false), _refHit(0, 0, 0)
	{
	}

	TripletTrack(Triplet up, Triplet down, Eigen::Vector3d refHit) :
	 _upstream(up), _downstream(down), _hasRef(true), _refHit(refHit)
	{
	}

	double kinkx() const
	{
		return _downstream.slope()(0) - _upstream.slope()(0);
	}

	double kinky() const
	{
		return _downstream.slope()(1) - _upstream.slope()(1);
	}

	double xresidualat(double z) const
	{
		auto a = _upstream.extrapolate(z);
		auto b = _downstream.extrapolate(z);
		return a(0) - b(0);
	}

	double yresidualat(double z) const
	{
		auto a = _upstream.extrapolate(z);
		auto b = _downstream.extrapolate(z);
		return a(1) - b(1);
	}

	double xrefresidual(Eigen::Vector3d align=Eigen::Vector3d::Zero()) const
	{
		assert(_hasRef == true);
		return _downstream.getdx(_refHit - align);
	}

	double yrefresidual(Eigen::Vector3d align=Eigen::Vector3d::Zero()) const
	{
		assert(_hasRef == true);
		return _downstream.getdy(_refHit - align);
	}

	void setRef(Eigen::Vector3d ref)
	{
		_refHit = ref;
		_hasRef = true;
	}

	void clearRef()
	{
		_hasRef = false;
	}

	bool hasRef() const { return _hasRef; }
	Eigen::Vector3d refHit() const { return _refHit; }
	Triplet upstream() const { return _upstream; }
	Triplet downstream() const { return _downstream; }

	struct histograms_t {
		TH1F* down_angle_x;
		TH1F* down_angle_y;
		TH1F* down_res_x;
		TH1F* down_res_y;
		TH1F* up_angle_x;
		TH1F* up_angle_y;
		TH1F* up_res_x;
		TH1F* up_res_y;
		TH1F* ref_down_res_x;
		TH1F* ref_down_res_y;
		TH1F* dut_up_res_x;
		TH1F* dut_up_res_y;
		TH1F* track_kink_x;
		TH1F* track_kink_y;
		TH1F* track_residual_x;
		TH1F* track_residual_y;
		TH1F* planes_z;
		TH1F* candidate_res_track_x;
		TH1F* candidate_res_track_y;
		TH1F* candidate_res_ref_x;
		TH1F* candidate_res_ref_y;
		TH1F* gbl_chi2_dist;
	};

	struct constants_t {
		double angle_cut;
		double upstream_residual_cut;
		double downstream_residual_cut;
		double six_residual_cut;
		double six_kink_cut;
		double ref_residual_precut;
		double ref_residual_cut;
		double dut_z;
		Eigen::Vector3d ref_prealign;
	};
	static histograms_t genDebugHistograms(std::string name_prefix="");
	static std::vector<core::TripletTrack> getTracks(constants_t consts,
	                                                 const core::MergedAnalysis::run_data_t& run,
	                                                 histograms_t* hist);
	static std::vector<core::TripletTrack> getTracksWithRef(constants_t consts,
	                                                 const core::MergedAnalysis::run_data_t& run,
	                                                 histograms_t hist, Eigen::Vector3d* new_ref_prealign);

private:
	Triplet _upstream;
	Triplet _downstream;
	bool _hasRef;
	Eigen::Vector3d _refHit;
};

}

#endif//TRIPLET_TRACK_H
