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
	static std::vector<Eigen::Vector2i> getCounterPixels(run_data_t run, const MpaTransform& transform);
	static std::vector<Eigen::Vector3d> getCounterClusters(run_data_t run, MpaTransform transform,
	                                                       std::vector<int>* clusterSizes,
	                                                       std::vector<double>* clusterAreas);
	static std::vector<Eigen::Vector2d> getCounterClustersLocal(run_data_t run, MpaTransform transform,
	                                                            std::vector<int>* clusterSizes,
	                                                            std::vector<double>* clusterAreas);
	static std::vector<Eigen::Vector2d> clusterize(std::vector<Eigen::Vector2i> hits,
	                                               std::vector<int>* clusterSizes,
	                                               std::vector<double>* clusterAreas);
};
} // core
#endif//MPA_HIT_GENERATOR_H
