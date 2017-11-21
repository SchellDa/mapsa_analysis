#include "transform.h"
#include <iostream>

using namespace core;

Transform::Transform()
{
	setRotation({0.0, 0.0, 0.0});
}

Transform::~Transform()
{
}

void Transform::setRotation(const Eigen::Vector3d& rot)
{
	_angles = rot;
	_rotation = Eigen::AngleAxis<double>(rot(0), Eigen::Vector3d::UnitX()) *
		    Eigen::AngleAxis<double>(rot(1), Eigen::Vector3d::UnitY()) *
		    Eigen::AngleAxis<double>(rot(2), Eigen::Vector3d::UnitZ());
	_invRotation = _rotation.inverse();
	_normal = _rotation * Eigen::Vector3d::UnitZ();
	_plane = Eigen::Hyperplane<double, 3>(_normal, _offset);
}

void Transform::setOffset(const Eigen::Vector3d& offset)
{
	_offset = offset;
	_plane = Eigen::Hyperplane<double, 3>(_normal, _offset);
}

Eigen::Vector3d Transform::transform(const Eigen::Vector3d& hit)
{
	Eigen::Vector3d hitTrans = hit;

	hitTrans = _rotation*hitTrans;
	hitTrans += _offset;

	return hitTrans;
}

Eigen::Vector3d Transform::planeTripletIntersect(const Triplet& triplet)
{	
	Eigen::ParametrizedLine<double, 3> line(triplet[0],
						triplet[1] - triplet[0]);
	auto t = line.intersectionParameter(_plane);
	return line.pointAt(t);
}
