#ifndef TRIPLET_H
#define TRIPLET_H

#include "datastructures.h"
#include <Eigen/Dense>

namespace core
{

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

	static std::vector<Triplet> findTriplets(const core::run_data_t& run,
	                                         double angle_cut,
	                                         double residual_cut,
	                                         std::array<int, 3> planes);

private:
	std::array<Eigen::Vector3d, 3> _hits;
};

}

std::ostream& operator<<(std::ostream& stream, const core::Triplet& T);

#endif//TRIPLET_H
