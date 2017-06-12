#include "mpahitgenerator.h"
#include "mpatransform.h"
#include <deque>
#include <algorithm>

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
		for(size_t pixel = 0; pixel < 48; ++pixel) {
			if(data[pixel] == 0) {
				continue;
			}
			Eigen::Vector3d hit = transform.transform(pixel, mpa.index);
			hits.push_back(hit);
		}
	}
	return hits;
}

std::vector<Eigen::Vector2d> MpaHitGenerator::getCounterClustersLocal(run_data_t run, MpaTransform transform,
                                                                      std::vector<int>* clusterSizes,
                                                                      std::vector<double>* clusterAreas)
{
	assert(MpaTransform::num_pixels == 48);
	std::vector<Eigen::Vector2i> pixels;
	for(auto& mpa: run.mpaData) {
		auto& data = (*mpa.data)->counter.pixels;
		for(size_t pixel = 0; pixel < 48; ++pixel) {
			if(data[pixel] == 0) {
				continue;
			}
			Eigen::Vector2i hit = transform.translatePixelIndex(pixel, mpa.index);
			pixels.push_back(hit);
		}
	}
	return clusterize(pixels, clusterSizes, clusterAreas);
}

std::vector<Eigen::Vector3d> MpaHitGenerator::getCounterClusters(run_data_t run, MpaTransform transform,
                                                                 std::vector<int>* clusterSizes,
                                                                 std::vector<double>* clusterAreas)
{
	auto clusters = getCounterClustersLocal(run, transform, clusterSizes, clusterAreas);
	std::vector<Eigen::Vector3d> hits;
	for(auto& cluster: clusters) {
		hits.push_back(transform.pixelCoordToGlobal(cluster));
	}
	return hits;
}

std::vector<Eigen::Vector2d> MpaHitGenerator::clusterize(std::vector<Eigen::Vector2i> hits,
                                                         std::vector<int>* clusterSizes,
                                                         std::vector<double>* clusterAreas)
{
	std::vector<Eigen::Vector2d> clusters;
	if(clusterSizes)
		clusterSizes->clear();
	if(clusterAreas)
		clusterAreas->clear();
	if(hits.empty()) {
		return clusters;
	}
	std::deque<Eigen::Vector2i> visitQueue;
	visitQueue.push_back(hits.front());
	hits.erase(hits.begin());
	std::vector<Eigen::Vector2i> currentClusterHits;
	while(!hits.empty()) {
		if(visitQueue.begin() == visitQueue.end()) {
			Eigen::Vector2d cluster{0, 0};
			double totalArea = 0;
			for(auto& hit: currentClusterHits) {
				double area = MpaTransform::pixelArea(hit);
				cluster += hit.cast<double>() * area;
				totalArea += area;
			}
			cluster /= totalArea;
			clusters.push_back(cluster);
			if(clusterSizes)
				clusterSizes->push_back(currentClusterHits.size());
			if(clusterAreas)
				clusterAreas->push_back(totalArea);
			currentClusterHits.clear();
			if(!hits.empty()) {
				visitQueue.push_back(hits.front());
				hits.erase(hits.begin());
			}
		} else {
			Eigen::Vector2i pixel = visitQueue.front();
			currentClusterHits.push_back(pixel);
			visitQueue.pop_front();
			std::vector<Eigen::Vector2i> neighbours(4);
			neighbours.push_back(pixel + Eigen::Vector2i({-1, 0}));
			neighbours.push_back(pixel + Eigen::Vector2i({1, 0}));
			neighbours.push_back(pixel + Eigen::Vector2i({0, -1}));
			neighbours.push_back(pixel + Eigen::Vector2i({0, 1}));
			for(Eigen::Vector2i neigh: neighbours) {
				auto hitit = std::find(hits.begin(), hits.end(), neigh);
				if(hitit != hits.end()) {
					visitQueue.push_back(*hitit);
					hits.erase(hitit);
				}
			}
		}
	}
	return clusters;
}
