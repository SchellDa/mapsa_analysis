#include "mpahitgenerator.h"
#include "mpatransform.h"

using namespace core;

std::vector<Eigen::Vector3d> MpaHitGenerator::getCounterHits(run_data_t run, Eigen::Vector3d offset, Eigen::Vector3d rotation)
{
	assert(MpaTransform::num_pixels == 48);
	MpaTransform transform;
	transform.setOffset(offset);
	transform.setRotation(rotation);
	return getCounterHits(run, transform);
}

std::vector<Eigen::Vector3d> MpaHitGenerator::getCounterHits(run_data_t run, const MpaTransform& transform)
{
	assert(MpaTransform::num_pixels == 48);
	std::vector<Eigen::Vector3d> hits;
	for(auto& mpa: run.mpaData) {
		auto& data = (*mpa.data)->counter.pixels;
		if(mpa.index != 2) break; // TODO: accept hits for all MPAs
		for(size_t pixel = 0; pixel < 48; ++pixel) {
			if(data[pixel] == 0) {
				continue;
			}
			Eigen::Vector3d hit = transform.transform(pixel);
			hits.push_back(hit);
		}
	}
	return hits;
}

std::vector<Eigen::Vector3d> MpaHitGenerator::getCounterClusters(run_data_t run, const MpaTransform& transform)
{
	assert(MpaTransform::num_pixels == 48);
	std::vector<Eigen::Vector3d> hits;
	std::vector<Eigen::Vector2d> currentCluster;
//	for(auto& mpa: run.mpaData) {
//		auto& data = (*mpa.data)->counter.pixels;
//		if(mpa.index != 2) break; // TODO: accept hits for all MPAs
//		for(size_t pixel = 0; pixel < 48; ++pixel) {
//			if(data[pixel] == 0) {
//				continue;
//			}
//			Eigen::Vector3d hit = transform.transform(pixel);
//			hits.push_back(hit);
//		}
//	}
	return hits;
}
