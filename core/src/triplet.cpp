#include "triplet.h"

using namespace core;

std::vector<Triplet> Triplet::findTriplets(const core::MergedAnalysis::run_data_t& run,
                                           double angle_cut,
                                           double residual_cut,
                                           std::array<int, 3> planes)
{
	std::vector<Triplet> triplets;
	auto td = &(*run.telescopeHits)->p1;
	for(int ia = 0; ia < td[planes[0]].x.GetNoElements(); ++ia) {
		for(int ib = 0; ib < td[planes[1]].x.GetNoElements(); ++ib) {
			for(int ic = 0; ic < td[planes[2]].x.GetNoElements(); ++ic) {
				Triplet t({td[planes[0]].x[ia], td[planes[0]].y[ia], td[planes[0]].z[ia]},
				          {td[planes[1]].x[ib], td[planes[1]].y[ib], td[planes[1]].z[ib]},
				          {td[planes[2]].x[ic], td[planes[2]].y[ic], td[planes[2]].z[ic]});
				if(std::abs(t.getdx()) > angle_cut * t.getdz())
					continue;
				if(std::abs(t.getdy()) > angle_cut * t.getdz())
					continue;
				if(std::abs(t.getdx(1)) > residual_cut)
					continue;
				if(std::abs(t.getdy(1)) > residual_cut)
					continue;
				triplets.push_back(t);
			}
		}
	}
	return triplets;
}
