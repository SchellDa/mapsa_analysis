#include "triplettrack.h"
#include <TFitResult.h>

using namespace core;


TripletTrack::histograms_t TripletTrack::genDebugHistograms(std::string name_prefix)
{
	histograms_t hist;
	hist.down_angle_x = new TH1F((name_prefix+"downstream_angle_x").c_str(), "Angle of downstream triplets", 100, 0, 0.5);
	hist.down_angle_y = new TH1F((name_prefix+"downstream_angle_y").c_str(), "Angle of downstream triplets", 100, 0, 0.5);
	hist.down_res_x = new TH1F((name_prefix+"downstream_res_x").c_str(), "Center residual of downstream triplets", 100, 0, 0.5);
	hist.down_res_y = new TH1F((name_prefix+"downstream_res_y").c_str(), "Center residual of downstream triplets", 100, 0, 0.5);
	hist.up_angle_x = new TH1F((name_prefix+"upstream_angle_x").c_str(), "Angle of upstream triplets", 100, 0, 0.5);
	hist.up_angle_y = new TH1F((name_prefix+"upstream_angle_y").c_str(), "Angle of upstream triplets", 100, 0, 0.5);
	hist.up_res_x = new TH1F((name_prefix+"upstream_res_x").c_str(), "Center residual of upstream triplets", 100, 0, 0.5);
	hist.up_res_y = new TH1F((name_prefix+"upstream_res_y").c_str(), "Center residual of upstream triplets", 100, 0, 0.5);
	hist.ref_down_res_x = new TH1F((name_prefix+"ref_downstream_res_x").c_str(), "Residual between downstream and ref", 1000, -5, 5);
	hist.ref_down_res_y = new TH1F((name_prefix+"ref_downstream_res_y").c_str(), "Residual between downstream and ref", 1000, -5, 5);
	hist.dut_up_res_x = new TH1F((name_prefix+"dut_upstream_res_x").c_str(), "Residual between upstream and dut", 1000, -5, 5);
	hist.dut_up_res_y = new TH1F((name_prefix+"dut_upstream_res_y").c_str(), "Residual between upstream and dut", 1000, -5, 5);
	hist.track_kink_x = new TH1F((name_prefix+"track_kink_x").c_str(), "", 1000, 0, 0.1);
	hist.track_kink_y = new TH1F((name_prefix+"track_kink_y").c_str(), "", 1000, 0, 0.1);
	hist.track_residual_x = new TH1F((name_prefix+"track_residual_x").c_str(), "", 1000, -1, 10);
	hist.track_residual_y = new TH1F((name_prefix+"track_residual_y").c_str(), "", 1000, -1, 10);
	hist.planes_z = new TH1F((name_prefix+"planes_z").c_str(), "Z positions of hits in accepted triplets", 500, -100, 1000);
	hist.candidate_res_track_x = new TH1F((name_prefix+"candidate_res_track_x").c_str(), "", 500, -1, 1);
	hist.candidate_res_track_y = new TH1F((name_prefix+"candidate_res_track_y").c_str(), "", 500, -1, 1);
	hist.candidate_res_ref_x = new TH1F((name_prefix+"candidate_res_ref_x").c_str(), "", 500, -1, 1);
	hist.candidate_res_ref_y = new TH1F((name_prefix+"candidate_res_ref_y").c_str(), "", 500, -1, 1);
	return hist;
}

std::vector<core::TripletTrack> TripletTrack::getTracks(constants_t consts,
                                                        const core::MergedAnalysis::run_data_t& run,
                                                        histograms_t* hist)
{
	std::vector<core::TripletTrack> candidates;
	for(size_t evt = 0; evt < run.tree->GetEntries(); ++evt) {
		run.tree->GetEntry(evt);
		auto downstream = core::Triplet::findTriplets(run, consts.angle_cut, consts.downstream_residual_cut, {3, 4, 5});
		if(hist) {
			// debug histograms
			for(const auto& triplet: downstream) {
				hist->down_angle_x->Fill(std::abs(triplet.getdx() / triplet.getdz()));
				hist->down_angle_y->Fill(std::abs(triplet.getdy() / triplet.getdz()));
				hist->down_res_x->Fill(std::abs(triplet.getdx(1)));
				hist->down_res_y->Fill(std::abs(triplet.getdy(1)));
			}
		}
		auto upstream = core::Triplet::findTriplets(run, consts.angle_cut, consts.upstream_residual_cut, {0, 1, 2});
		if(hist) {
			// debug histograms
			for(const auto& triplet: upstream) {
				hist->up_angle_x->Fill(std::abs(triplet.getdx() / triplet.getdz()));
				hist->up_angle_y->Fill(std::abs(triplet.getdy() / triplet.getdz()));
				hist->up_res_x->Fill(std::abs(triplet.getdx(1)));
				hist->up_res_y->Fill(std::abs(triplet.getdy(1)));
			}
		}
		// build tracks
		for(const auto& down: downstream) {
			for(const auto& up: upstream) {
				core::TripletTrack t(up, down);
				auto resx = t.xresidualat(consts.dut_z);
				auto resy = t.yresidualat(consts.dut_z);
				auto kinkx = std::abs(t.kinkx());
				auto kinky = std::abs(t.kinky());
				if(std::abs(resx) > consts.six_residual_cut || std::abs(resy) > consts.six_residual_cut) {
					continue;
				}
				if(kinkx > consts.six_kink_cut || kinky > consts.six_kink_cut) {
					continue;
				}
				if(hist) {
					hist->track_kink_x->Fill(kinkx);
					hist->track_kink_y->Fill(kinky);
					hist->track_residual_x->Fill(resx);
					hist->track_residual_y->Fill(resy);
					for(const auto& hit: downstream) {
						for(int i = 0; i < 3; ++i) {
							hist->planes_z->Fill(hit[i](2));
						}
					}
					for(const auto& hit: upstream) {
						for(int i = 0; i < 3; ++i) {
							hist->planes_z->Fill(hit[i](2));
						}
					}
				}
				candidates.push_back(t);
			}
		}
	}
	return candidates;
}


std::vector<core::TripletTrack> TripletTrack::getTracksWithRef(constants_t consts,
                                                               const core::MergedAnalysis::run_data_t& run,
                                                               histograms_t hist,
                                                               Eigen::Vector3d* new_ref_prealign)
{
	assert(hist.down_angle_x);
	std::vector<core::TripletTrack> candidates;
	for(size_t evt = 0; evt < run.tree->GetEntries(); ++evt) {
		run.tree->GetEntry(evt);
		auto downstream = core::Triplet::findTriplets(run, consts.angle_cut, consts.downstream_residual_cut, {3, 4, 5});
		// debug histograms
		for(const auto& triplet: downstream) {
			hist.down_angle_x->Fill(std::abs(triplet.getdx() / triplet.getdz()));
			hist.down_angle_y->Fill(std::abs(triplet.getdy() / triplet.getdz()));
			hist.down_res_x->Fill(std::abs(triplet.getdx(1)));
			hist.down_res_y->Fill(std::abs(triplet.getdy(1)));
		}
		auto upstream = core::Triplet::findTriplets(run, consts.angle_cut, consts.upstream_residual_cut, {0, 1, 2});
		// debug histograms
		for(const auto& triplet: upstream) {
			hist.up_angle_x->Fill(std::abs(triplet.getdx() / triplet.getdz()));
			hist.up_angle_y->Fill(std::abs(triplet.getdy() / triplet.getdz()));
			hist.up_res_x->Fill(std::abs(triplet.getdx(1)));
			hist.up_res_y->Fill(std::abs(triplet.getdy(1)));
		}
		// cut downstream triplets on their residual to ref hit
		auto refData = (*run.telescopeHits)->ref;
//		std::vector<Triplet> acceptedDownstream;
		std::vector<std::pair<core::Triplet, Eigen::Vector3d>> fullDownstream;
		for(int i = 0; i < refData.x.GetNoElements(); ++i) {
			Eigen::Vector3d hit(refData.x[i],
			                     refData.y[i],
					     refData.z[i]);
			for(const auto& triplet: downstream) {
				double resx = triplet.getdx(hit - consts.ref_prealign);
				double resy = triplet.getdy(hit - consts.ref_prealign);
				if(std::abs(resx) > consts.ref_residual_precut || std::abs(resy) > consts.ref_residual_precut) {
					continue;
				}
				hist.ref_down_res_x->Fill(resx);
				hist.ref_down_res_y->Fill(resy);
				// acceptedDownstream.push_back(triplet);
				fullDownstream.push_back({triplet, hit});
			}
		}
//		// find new prealignment (more exact)
//		auto result = _ref_down_res_x->Fit("gaus", "FSMR", "");
//		_refPreAlign(0) = result->Parameter(1);
//		result = _track_residual_y->Fit("gaus", "FSMR", "");
//		_refPreAlign(1) = result->Parameter(1);
//		// cut downstream triplets on their residual to ref hit
//		std::vector<std::pair<Triplet, Eigen::Vector3d>> fullDownstream;
//		for(int i = 0; i < refData.x.GetNoElements(); ++i) {
//			Eigen::Vector3d hit(refData.x[i],
//			                    refData.y[i],
//					    refData.z[i]);
//			hit -= _refPreAlign;
//			for(const auto& triplet: acceptedDownstream) {
//				double resx = std::abs(triplet.getdx(hit));
//				double resy = std::abs(triplet.getdy(hit));
//				if(resx > ref_residual_narrow_cut || resy > ref_residual_narrow_cut) {
//					continue;
//				}
//				_ref_down_res_x_pre->Fill(resx);
//				_ref_down_res_y_pre->Fill(resy);
//				fullDownstream.push_back({triplet, hit});
//			}
//		}
		// build tracks
		for(const auto& pair: fullDownstream) {
			auto down = pair.first;
			auto ref = pair.second;
			for(const auto& up: upstream) {
				core::TripletTrack t(up, down, ref);
				auto resx = t.xresidualat(consts.dut_z);
				auto resy = t.yresidualat(consts.dut_z);
				auto kinkx = std::abs(t.kinkx());
				auto kinky = std::abs(t.kinky());
				if(std::abs(resx) > consts.six_residual_cut || std::abs(resy) > consts.six_residual_cut) {
					continue;
				}
				if(kinkx > consts.six_kink_cut || kinky > consts.six_kink_cut) {
					continue;
				}
				hist.track_kink_x->Fill(kinkx);
				hist.track_kink_y->Fill(kinky);
				hist.track_residual_x->Fill(resx);
				hist.track_residual_y->Fill(resy);
				candidates.push_back(t);
			}
		}
		for(const auto& pair: fullDownstream) {
			const auto& hit = pair.first;
			const auto& ref = pair.second;
			for(int i = 0; i < 3; ++i) {
				hist.planes_z->Fill(hit[i](2));
			}
			hist.planes_z->Fill(ref(2));
		}
		for(const auto& hit: upstream) {
			for(int i = 0; i < 3; ++i) {
				hist.planes_z->Fill(hit[i](2));
			}
		}
	}
	// find new prealignment (more exact)
	Eigen::Vector3d refPreAlign(consts.ref_prealign);
	auto result = hist.ref_down_res_x->Fit("gaus", "FSMR", "");
	refPreAlign(0) += result->Parameter(1);
	result = hist.ref_down_res_y->Fit("gaus", "FSMR", "");
	refPreAlign(1) += result->Parameter(1);
	std::vector<core::TripletTrack> accepted;
	if(new_ref_prealign) {
		*new_ref_prealign = refPreAlign;
	}
	for(auto track: candidates) {
		auto track_x = track.xresidualat(consts.dut_z);
		auto track_y = track.yresidualat(consts.dut_z);
		auto ref_x = track.xrefresidual(refPreAlign);
		auto ref_y = track.yrefresidual(refPreAlign);
		if(std::abs(ref_x) > consts.ref_residual_cut || std::abs(ref_y) > consts.ref_residual_cut) {
			continue;
		}
		hist.candidate_res_track_x->Fill(track_x);
		hist.candidate_res_track_y->Fill(track_y);
		hist.candidate_res_ref_x->Fill(ref_x);
		hist.candidate_res_ref_y->Fill(ref_y);
		accepted.push_back(track);
	}
	return accepted;
}
