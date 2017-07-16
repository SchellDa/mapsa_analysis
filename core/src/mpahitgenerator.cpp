#include "mpahitgenerator.h"
#include "mpatransform.h"
#include <deque>
#include <algorithm>
#include <iostream>

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
		if(mpa.index != 2) continue;
		auto& data = (*mpa.data)->counter.pixels;
		for(size_t pixel = 0; pixel < 48; ++pixel) {
			if(data[pixel] == 0) {
				continue;
			}
			Eigen::Vector3d hit = transform.transform(pixel);
			//Eigen::Vector3d hit = transform.transform(pixel, mpa.index);
			hits.push_back(hit);
		}
	}
	return hits;
}

std::vector<Eigen::Vector2i> MpaHitGenerator::getCounterPixels(run_data_t run, const MpaTransform& transform)
{
	assert(MpaTransform::num_pixels == 48);
	std::vector<Eigen::Vector2i> hits;
	for(auto& mpa: run.mpaData) {
		if(mpa.index != 2) continue;
		auto& data = (*mpa.data)->counter.pixels;
		for(size_t pixel = 0; pixel < 48; ++pixel) {
			if(data[pixel] == 0) {
				continue;
			}
			Eigen::Vector2i hit = transform.translatePixelIndex(pixel);
			//Eigen::Vector2i hit = transform.translatePixelIndex(pixel, mpa.index);
			hits.push_back(hit);
		}
	}
	return hits;
}

std::vector<Eigen::Vector2d> MpaHitGenerator::getCounterClustersLocal(run_data_t run, MpaTransform transform,
                                                                      std::vector<int>* clusterSizes,
                                                                      std::vector<double>* clusterAreas)
{
	auto pixels = getCounterPixels(run, transform);
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
//	std::cout << "\n\nCLUSTERIZE!" << std::endl;
	std::vector<Eigen::Vector2d> clusters;
	if(clusterSizes)
		clusterSizes->clear();
	if(clusterAreas)
		clusterAreas->clear();
	if(hits.empty()) {
//		std::cout << " * no hits in input, aborting." << std::endl;
		return clusters;
	}
	std::deque<Eigen::Vector2i> visitQueue;
	visitQueue.push_back(hits.front());
	hits.erase(hits.begin());
	std::vector<Eigen::Vector2i> currentClusterHits;
	MpaTransform transform;
	transform.setOffset({0, 0, 0});
	transform.setRotation({0, 0, 0});
	while(!hits.empty() || !visitQueue.empty() || !currentClusterHits.empty()) {
//		std::cout << "<< LOOP >>";
//		std::cout << "\n  num hits " << hits.size();
//		std::cout << "\n  visit queue size " << visitQueue.size();
//		for(const auto& h: visitQueue) {
//			std::cout << "\n   " << h(0) << (1);}
//		std::cout << "\n  current cluster size " << currentClusterHits.size();
//		std::cout << "\n  num clusters found " << clusters.size();
//		std::cout << std::endl;
		if(visitQueue.begin() == visitQueue.end()) {
//			std::cout << " * Visit queue empty!" << std::endl;
			Eigen::Vector3d cluster{0, 0, 0};
			double totalArea = 0;
			for(auto& hit: currentClusterHits) {
				double area = MpaTransform::pixelArea(hit);
				cluster += transform.pixelCoordToGlobal(hit) * area;
				totalArea += area;
			}
			cluster /= totalArea;
			clusters.push_back(transform.globalToPixelCoord(cluster));
			if(clusterSizes)
				clusterSizes->push_back(currentClusterHits.size());
			if(clusterAreas)
				clusterAreas->push_back(totalArea);
			currentClusterHits.clear();
			if(!hits.empty()) {
				visitQueue.push_back(hits.front());
				hits.erase(hits.begin());
//				std::cout << " * Initialized new cluster!" << std::endl;
			}
		} else {
//			std::cout << " * Check neighbours..." << std::endl;
			Eigen::Vector2i pixel = visitQueue.front();
			currentClusterHits.push_back(pixel);
			visitQueue.pop_front();
			std::vector<Eigen::Vector2i> neighbours(8);
			neighbours[0] = (pixel + Eigen::Vector2i({-1, 0}));
			neighbours[1] = (pixel + Eigen::Vector2i({1, 0}));
			neighbours[2] = (pixel + Eigen::Vector2i({0, -1}));
			neighbours[3] = (pixel + Eigen::Vector2i({0, 1}));
			neighbours[4] = (pixel + Eigen::Vector2i({-1, -1}));
			neighbours[5] = (pixel + Eigen::Vector2i({1, 1}));
			neighbours[6] = (pixel + Eigen::Vector2i({1, -1}));
			neighbours[7] = (pixel + Eigen::Vector2i({-1, 1}));
			for(Eigen::Vector2i neigh: neighbours) {
//				std::cout << "    " << neigh(0) << " " << neigh(1) << "\n";
				auto hitit = std::find(hits.begin(), hits.end(), neigh);
				if(hitit != hits.end()) {
//					std::cout << "       found!" << std::endl;
					visitQueue.push_back(*hitit);
					hits.erase(hitit);
				}
			}
		}
	}
//	std::cout << "<< DONE >>"
//		  << "\n  num clusters found " << clusters.size()
//		  << std::endl;
	return clusters;
}
