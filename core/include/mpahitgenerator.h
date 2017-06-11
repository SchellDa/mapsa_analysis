#ifndef MPA_HIT_GENERATOR_H
#define MPA_HIT_GENERATOR_H

#include <Eigen/Dense>
#include "datastructures.h"
#include "mpatransform.h"

namespace core {

class MpaHitGenerator
{
public:
	static std::vector<Eigen::Vector3d> getCounterHits(run_data_t run, Eigen::Vector3d offset, Eigen::Vector3d rotation);
	static std::vector<Eigen::Vector3d> getCounterHits(run_data_t run, const MpaTransform& transform);
	static std::vector<Eigen::Vector3d> getCounterClusters(run_data_t run, const MpaTransform& transform);
};
} // core
#endif//MPA_HIT_GENERATOR_H
