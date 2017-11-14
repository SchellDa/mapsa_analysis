#ifndef TRANSFORM_H
#define TRANSFORM_H

#include <Eigen/Dense>
#include "triplet.h"

namespace core{

class Transform
{
public:
	Transform();
	virtual ~Transform();
	void setRotation(const Eigen::Vector3d& rot);
	void setOffset(const Eigen::Vector3d& offset);
	Eigen::Vector3d transform(const Eigen::Vector3d& hit);
	Eigen::Vector3d planeTripletIntersect(const Triplet& triplet);

private:
	Eigen::Matrix3d _rotation;
	Eigen::Matrix3d _invRotation;
	Eigen::Vector3d _normal;
	Eigen::Hyperplane<double, 3> _plane;
	Eigen::Vector3d _offset;
	Eigen::Vector3d _angles;

};

} // namespace core
#endif // TRANSFORM_H
