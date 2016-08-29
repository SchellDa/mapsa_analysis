
#ifndef CORE_TRACK_H
#define CORE_TRACK_H

#include <Eigen/Dense>
#include <vector>

namespace core {

/** \brief Track description and helper methods
 *
 */
struct Track
{
	/// \brief Extrapolate the track via the points a and b onto plane perpendicular to axis given by axis
	//in distance dist
	Eigen::Vector3d extrapolateOnPlane(size_t a, size_t b, double distance, size_t axis) const
	{
		assert(axis < 3);
		auto A = points.at(a);
		auto B = points.at(b);
		auto D = B-A;
		double t = (distance - A(axis)) / D(axis);
		return D*t + A;
	}
	
	/// A list of the sensor IDs for consecutive points in the track
	std::vector<int> sensorIDs;
	/// 3D-positions of consecutive points in the track
	std::vector<Eigen::Vector3d> points;
};

} // namespace core

#endif//CORE_TRACK_H
