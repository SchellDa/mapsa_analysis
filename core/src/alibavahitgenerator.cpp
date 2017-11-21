#include "alibavahitgenerator.h"

using namespace core;

std::vector<Eigen::Vector2d> AlibavaHitGenerator::getLocalHits(run_data_t run, bool flipDut)
{
	// All coordinates in mm
	auto ali = (*run.alibavaData);
	std::vector<Eigen::Vector2d> localHits;
	for(size_t iCluster; iCluster < ali->center.GetNoElements(); ++iCluster)
	{
		auto channel = ali->center[iCluster];
		double x, y;
		if(flipDut) {
 			if (channel < ALIBAVA_N/2.) {
				y = (ALIBAVA_N/4.-0.5-channel) * (ALIBAVA_PITCH/1000.);
				//x = ALIBAVA_STRIP_L/(2*1000.);
				x = 0.0;
			} else {
				y = (0.5-3*ALIBAVA_N/4.+channel) * (ALIBAVA_PITCH/1000.);
				//x = -ALIBAVA_STRIP_L/(2*1000.);
				x = 0.0;
			}

		} else {
			if (channel < ALIBAVA_N/2.) {
				x = (ALIBAVA_N/4.-0.5-channel) * (ALIBAVA_PITCH/1000.);
				//y = ALIBAVA_STRIP_L/(2*1000.);
				y = 0.0;
			} else {
				x = (0.5-3*ALIBAVA_N/4.+channel) * (ALIBAVA_PITCH/1000.);
				//y = -ALIBAVA_STRIP_L/(2*1000.);
				y = 0.0;
			}
		}
		Eigen::Vector2d localHit(x, y);
		localHits.emplace_back(localHit);
	}
	return localHits;
}

std::vector<Eigen::Vector3d> AlibavaHitGenerator::getHits(run_data_t run, double z, bool flipDut)
{
	auto localHits = getLocalHits(run, flipDut);
	std::vector<Eigen::Vector3d> hits;
	for(const auto& localHit : localHits)
	{
		Eigen::Vector3d hit(localHit(0), localHit(1), 0.0);
		hits.emplace_back(hit);
	}
	return hits;
}
