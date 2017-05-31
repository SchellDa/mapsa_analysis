

#ifndef GBL_ALIGN_H
#define GBL_ALIGN_H

#include "mergedanalysis.h"
#include <TH1F.h>
#include <array>

namespace gbl {
	class GblTrajectory;
}

class Triplet
{
public:
	Triplet(Eigen::Vector3d a, Eigen::Vector3d b, Eigen::Vector3d c) :
	 _hits { { a, b, c } }
	{
	}

	Triplet(const Triplet& othr) :
	 _hits { { othr._hits[0], othr._hits[1], othr._hits[2] } }
	{
	}

	Triplet(std::array<Eigen::Vector3d, 3> hits) :
	 _hits{ { hits[0], hits[1], hits[2] } }
	{
	}

	double getdx() const
	{
		return _hits[2](0) - _hits[0](0);
	}

	double getdy() const
	{
		return _hits[2](1) - _hits[0](1);
	}

	Eigen::Vector2d getds() const
	{
		return { getdx(), getdy() };
	}

	double getdx(int plane) const
	{
		assert(plane >= 0);
		assert(plane < 3);
		return _hits[plane](0) - base()(0) - slope()(0) * (_hits[plane](2) - base()(2));
	}

	double getdy(int plane) const
	{
		assert(plane >= 0);
		assert(plane < 3);
		return _hits[plane](1) - base()(1) - slope()(1) * (_hits[plane](2) - base()(2));
	}

	Eigen::Vector2d getds(int plane) const
	{
		return { getdx(plane), getdy(plane) };
	}

	double getdx(Eigen::Vector3d hit) const
	{
		return hit(0) - base()(0) - slope()(0) * (hit(2) - base()(2));
	}

	double getdy(Eigen::Vector3d hit) const
	{
		return hit(1) - base()(1) - slope()(1) * (hit(2) - base()(2));
	}

	Eigen::Vector2d getds(Eigen::Vector3d hit) const
	{
		return { getdx(hit), getdy(hit) };
	}

	double getdz() const
	{
		return _hits[2](2) - _hits[0](2);
	}

	Eigen::Vector3d base() const
	{
		return (_hits[0] + _hits[2]) / 2.0;
	}

	Eigen::Vector2d slope() const
	{
		return (_hits[2] - _hits[0]).head<2>() / getdz();
	}

	Eigen::Vector3d slope3() const
	{
		return (_hits[2] - _hits[0]) / getdz();
	}

	Eigen::Vector3d extrapolate(double z) const
	{
		return _hits[0] + slope3() * (z - _hits[0](2));
	}

	double extrapolatex(double z) const
	{
		return extrapolate(z)(0);
	}

	double extrapolatey(double z) const
	{
		return extrapolate(z)(1);
	}

	Eigen::Vector3d& operator[](int idx)
	{
		assert(idx >= 0);
		assert(idx < 3);
		return _hits[idx];
	}

	const Eigen::Vector3d& operator[](int idx) const
	{
		assert(idx >= 0);
		assert(idx < 3);
		return _hits[idx];
	}

//	std::array<Eigen::Vector3d, 3>& getHits()
//	{
//		return _hits;
//	}

//	const std::array<Eigen::Vector3d, 3>& getHits() const
//	{
//		return _hits;
//	}

	std::array<Eigen::Vector3d, 3> getHits() const
	{
		return _hits;
	}

private:
	std::array<Eigen::Vector3d, 3> _hits;
};

class Track
{
public:
	Track(Triplet up, Triplet down) : _upstream(up), _downstream(down), _hasRef(false), _refHit(0, 0, 0)
	{
	}

	Track(Triplet up, Triplet down, Eigen::Vector3d refHit) :
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

	bool hasRef() const { return _hasRef; }
	Eigen::Vector3d refHit() const { return _refHit; }
	Triplet upstream() const { return _upstream; }
	Triplet downstream() const { return _downstream; }

private:
	Triplet _upstream;
	Triplet _downstream;
	bool _hasRef;
	Eigen::Vector3d _refHit;
};

class GblAlign : public core::MergedAnalysis
{
public:
	GblAlign();
	virtual ~GblAlign();

	virtual void init();
	virtual void run(const core::MergedAnalysis::run_data_t& run);
	virtual void finalize();

	static Eigen::MatrixXd jacobianStep(double step);
	static Eigen::MatrixXd getDerivatives(Triplet t, double dut_z, Eigen::Vector3d angles);

private:
	std::vector<Triplet> findTriplets(const core::MergedAnalysis::run_data_t& run, double angle_cut, double residual_cut,
	                                  std::array<int, 3> planes);
	std::vector<Track> getTrackCandidates(size_t maxCandidates, const core::MergedAnalysis::run_data_t& run);
	void fitTracks(std::vector<Track> trackCandidates);
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
