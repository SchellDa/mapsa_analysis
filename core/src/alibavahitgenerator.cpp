#include "alibavahitgenerator.h"

using namespace core;

std::vector<Eigen::Vector2d> AlibavaHitGenerator::getLocalHits(run_data_t run)
{
	auto ali = (*run.alibavaData);
	std::vector<Eigen::Vector2d> localHits;
	for(size_t iCluster; iCluster < ali->center.GetNoElements(); ++iCluster)
	{
		auto channel = ali->center[iCluster];
		double x;
		if (channel < ALIBAVA_N/2.)
			x = (channel-ALIBAVA_N/4.+0.5) * (ALIBAVA_PITCH/1000.);
		else
			x = (0.5-3*ALIBAVA_N/4.+channel) * (ALIBAVA_PITCH/1000.);
		Eigen::Vector2d localHit(x, 0.0);
		localHits.emplace_back(localHit);
	}

	return localHits;
}

std::vector<Eigen::Vector3d> AlibavaHitGenerator::getHits(run_data_t run)
{
	auto localHits = getLocalHits(run);
	std::vector<Eigen::Vector3d> hits;
	for(const auto& localHit : localHits)
	{
		Eigen::Vector3d hit(localHit(0), localHit(1), 0.0);
		hits.emplace_back(hit);
	}

	return hits;
}
