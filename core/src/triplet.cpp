#include "triplet.h"
#include <chrono>
#include <iostream>

using namespace core;

std::vector<Triplet> Triplet::findTriplets(const core::run_data_t& run,
                                           double angle_cut,
                                           double residual_cut,
                                           std::array<int, 3> planes)
{
	
	std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
	std::vector<Triplet> triplets;
	auto td = &(*run.telescopeHits)->p1;
	//std::vector<int> ub;
	//std::vector<int> uc;
	for(int ia = 0; ia < td[planes[0]].x.GetNoElements(); ++ia) {
		for(int ib = 0; ib < td[planes[1]].x.GetNoElements(); ++ib) {
			/*
			if(std::find(ub.begin(), ub.end(), ib) != ub.end()) {
				continue;
			}
			*/
			for(int ic = 0; ic < td[planes[2]].x.GetNoElements(); ++ic) {
				/*
				if(std::find(uc.begin(), uc.end(), ic) != uc.end()) {
					continue;
				}
				*/
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
				//ub.push_back(ib);
				//uc.push_back(ic);
			}
		}
	}
	return triplets;
}

std::ostream& operator<<(std::ostream& stream, const Triplet& T)
{
	for(auto h: T.getHits()) {
		stream << h(0) << " " << h(1) << " " << h(2) << "\n";
	}
	return stream;
}
