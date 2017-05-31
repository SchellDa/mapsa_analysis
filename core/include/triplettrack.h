#ifndef TRIPLET_TRACK_H
#define TRIPLET_TRACK_H

#include "triplet.h"

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

}

#endif//TRIPLET_TRACK_H
