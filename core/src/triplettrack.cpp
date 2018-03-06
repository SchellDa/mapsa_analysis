#include "triplettrack.h"
#include <TFitResult.h>
#include "mpahitgenerator.h"
#include <iostream>
#include "aligner.h"
#include <chrono>
#include "transform.h"
#include "alibavahitgenerator.h"

using namespace core;


TripletTrack::histograms_t TripletTrack::genDebugHistograms(std::string name_prefix)
{
	histograms_t hist;
	hist.down_angle_x = new TH1F((name_prefix+"downstream_angle_x").c_str(), "Angle of downstream triplets", 200, -0.5, 0.5);
	hist.down_angle_y = new TH1F((name_prefix+"downstream_angle_y").c_str(), "Angle of downstream triplets", 200, -0.5, 0.5);
	hist.down_res_x = new TH1F((name_prefix+"downstream_res_x").c_str(), "Center residual of downstream triplets", 200, -0.5, 0.5);
	hist.down_res_y = new TH1F((name_prefix+"downstream_res_y").c_str(), "Center residual of downstream triplets", 200, -0.5, 0.5);
	hist.up_angle_x = new TH1F((name_prefix+"upstream_angle_x").c_str(), "Angle of upstream triplets", 200, -0.5, 0.5);
	hist.up_angle_y = new TH1F((name_prefix+"upstream_angle_y").c_str(), "Angle of upstream triplets", 200, -0.5, 0.5);
	hist.up_res_x = new TH1F((name_prefix+"upstream_res_x").c_str(), "Center residual of upstream triplets", 200, -0.5, 0.5);
	hist.up_res_y = new TH1F((name_prefix+"upstream_res_y").c_str(), "Center residual of upstream triplets", 200, -0.5, 0.5);
	hist.ref_down_res_x = new TH1F((name_prefix+"ref_downstream_res_x").c_str(), "Residual between downstream and ref", 1000, -5, 5);
	hist.ref_down_res_y = new TH1F((name_prefix+"ref_downstream_res_y").c_str(), "Residual between downstream and ref", 1000, -5, 5);
	hist.dut_up_res_x = new TH1D((name_prefix+"dut_upstream_res_x").c_str(), "Residual between upstream and dut", 1000, -10, 10);
	hist.dut_up_res_y = new TH1D((name_prefix+"dut_upstream_res_y").c_str(), "Residual between upstream and dut", 1000, -10, 10);
	hist.dut_cluster_size = new TH1F((name_prefix+"dut_cluster_size").c_str(), "Size of clusters in DUT", 50, 0, 50);
	hist.track_kink_x = new TH1F((name_prefix+"track_kink_x").c_str(), "", 2000, -0.1, 0.1);
	hist.track_kink_y = new TH1F((name_prefix+"track_kink_y").c_str(), "", 2000, -0.1, 0.1);
	hist.track_residual_x = new TH1F((name_prefix+"track_residual_x").c_str(), "", 2000, -10, 10);
	hist.track_residual_y = new TH1F((name_prefix+"track_residual_y").c_str(), "", 2000, -10, 10);
	hist.planes_z = new TH1F((name_prefix+"planes_z").c_str(), "Z positions of hits in accepted triplets", 500, -100, 1000);
	hist.candidate_res_track_x = new TH1F((name_prefix+"candidate_res_track_x").c_str(), "", 500, -1, 1);
	hist.candidate_res_track_y = new TH1F((name_prefix+"candidate_res_track_y").c_str(), "", 500, -1, 1);
	hist.candidate_res_ref_x = new TH1F((name_prefix+"candidate_res_ref_x").c_str(), "", 500, -1, 1);
	hist.candidate_res_ref_y = new TH1F((name_prefix+"candidate_res_ref_y").c_str(), "", 500, -1, 1);
	hist.candidate_res_dut_x = new TH1F((name_prefix+"candidate_res_dut_x").c_str(), "", 500, -5, 5);
	hist.candidate_res_dut_y = new TH1F((name_prefix+"candidate_res_dut_y").c_str(), "", 500, -5, 5);
	return hist;
}

std::vector<core::TripletTrack> TripletTrack::getTracks(constants_t consts,
                                                        const core::run_data_t& run,
                                                        histograms_t* hist)
{
	//core::Triplet::counter = 0;
	std::vector<core::TripletTrack> candidates;
	for(size_t evt = 0; evt < run.tree->GetEntries(); ++evt) {
		
		if(evt%10000 == 0) {
			std::cout << "Processing event:" << std::setw(9)
				  << evt << std::endl;
		}
		run.tree->GetEntry(evt);
		auto downstream = core::Triplet::findTriplets(run, consts.angle_cut, consts.downstream_residual_cut, {3, 4, 5});
		if(hist) {
			// debug histograms
			for(const auto& triplet: downstream) {
				hist->down_angle_x->Fill(triplet.getdx() / triplet.getdz());
				hist->down_angle_y->Fill(triplet.getdy() / triplet.getdz());
				hist->down_res_x->Fill(triplet.getdx(1));
				hist->down_res_y->Fill(triplet.getdy(1));
			}
		}
		auto upstream = core::Triplet::findTriplets(run, consts.angle_cut, consts.upstream_residual_cut, {0, 1, 2});
		if(hist) {
			// debug histograms
			for(const auto& triplet: upstream) {
				hist->up_angle_x->Fill(triplet.getdx() / triplet.getdz());
				hist->up_angle_y->Fill(triplet.getdy() / triplet.getdz());
				hist->up_res_x->Fill(triplet.getdx(1));
				hist->up_res_y->Fill(triplet.getdy(1));
			}
		}
		// build tracks
		for(const auto& down: downstream) {
			for(const auto& up: upstream) {
				core::TripletTrack t(evt, up, down);
				auto resx = t.xresidualat(consts.dut_offset(2));
				auto resy = t.yresidualat(consts.dut_offset(2));
				auto kinkx = (t.kinkx());
				auto kinky = (t.kinky());
				if(std::abs(resx) > consts.six_residual_cut || std::abs(resy) > consts.six_residual_cut) {
					continue;
				}
				if(std::abs(kinkx) > consts.six_kink_cut || std::abs(kinky) > consts.six_kink_cut) {
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
                                                               const core::run_data_t& run,
                                                               histograms_t* hist,
                                                               Eigen::Vector3d* new_ref_prealign)
{
	//assert(hist->down_angle_x);
	std::vector<core::TripletTrack> candidates;
	for(size_t evt = 0; evt < run.tree->GetEntries(); ++evt) {
		if(evt%10000 == 0) {
			std::cout << "Processing event:" << std::setw(9)
				  << evt << std::endl;
		}
		run.tree->GetEntry(evt);
		/*
		//std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
		if(evt%1000 == 0) {
			std::cout << "Before Downstream\t" 
				  << std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start).count()
				  << "\n";
		}
		*/
		auto downstream = core::Triplet::findTriplets(run, consts.angle_cut, consts.downstream_residual_cut, {3, 4, 5}); // very time consuming
		// debug histograms
		if(hist) {

			for(const auto& triplet: downstream) {
				hist->down_angle_x->Fill(triplet.getdx() / triplet.getdz());
				hist->down_angle_y->Fill(triplet.getdy() / triplet.getdz());
				hist->down_res_x->Fill(triplet.getdx(1));
				hist->down_res_y->Fill(triplet.getdy(1));
			}
		}
		auto upstream = core::Triplet::findTriplets(run, consts.angle_cut, consts.upstream_residual_cut, {0, 1, 2}); // very time consuming
		// debug histograms		
		if(hist) {
			for(const auto& triplet: upstream) {
				hist->up_angle_x->Fill(triplet.getdx() / triplet.getdz());
				hist->up_angle_y->Fill(triplet.getdy() / triplet.getdz());
				hist->up_res_x->Fill(triplet.getdx(1));
				hist->up_res_y->Fill(triplet.getdy(1));
			}
		}
		// cut downstream triplets on their residual to ref hit
		auto refData = (*run.telescopeHits)->ref;
		//std::vector<Triplet> acceptedDownstream;
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
				if(hist) {
					hist->ref_down_res_x->Fill(resx);
					hist->ref_down_res_y->Fill(resy);
				}
				// acceptedDownstream.push_back(triplet);
				fullDownstream.push_back({triplet, hit});
			}
		}
		// build tracks
		for(const auto& pair: fullDownstream) {
			auto down = pair.first;
			auto ref = pair.second;
			for(const auto& up: upstream) {
				core::TripletTrack t(evt, up, down, ref);
				auto resx = t.xresidualat(consts.dut_offset(2));
				auto resy = t.yresidualat(consts.dut_offset(2));
				auto kinkx = t.kinkx();
				auto kinky = t.kinky();
				if(std::abs(resx) > consts.six_residual_cut || std::abs(resy) > consts.six_residual_cut) {
					continue;
				}
				if(std::abs(kinkx) > consts.six_kink_cut || std::abs(kinky) > consts.six_kink_cut) {
					continue;
				}
				if(hist) {
					hist->track_kink_x->Fill(kinkx);
					hist->track_kink_y->Fill(kinky);
					hist->track_residual_x->Fill(resx);
					hist->track_residual_y->Fill(resy);
				}
				candidates.push_back(t);
			}
		}
		if(hist) {
			for(const auto& pair: fullDownstream) {
				const auto& hit = pair.first;
				const auto& ref = pair.second;
				for(int i = 0; i < 3; ++i) {
					hist->planes_z->Fill(hit[i](2));
				}
				hist->planes_z->Fill(ref(2));
			}
			for(const auto& hit: upstream) {
				for(int i = 0; i < 3; ++i) {
					hist->planes_z->Fill(hit[i](2));
				}
			}
		}
	}
	// find new prealignment (more exact)
	Eigen::Vector3d refPreAlign(consts.ref_prealign);
	auto result = hist->ref_down_res_x->Fit("gaus", "FSMR", "");
	refPreAlign(0) += result->Parameter(1);
	result = hist->ref_down_res_y->Fit("gaus", "FSMR", "");
	refPreAlign(1) += result->Parameter(1);
	std::vector<core::TripletTrack> accepted;
	if(new_ref_prealign) {
		*new_ref_prealign = refPreAlign;
	}
	for(auto track: candidates) {
		auto track_x = track.xresidualat(consts.dut_offset(2));
		auto track_y = track.yresidualat(consts.dut_offset(2));
		auto ref_x = track.xrefresidual(refPreAlign);
		auto ref_y = track.yrefresidual(refPreAlign);
		if(std::abs(ref_x) > consts.ref_residual_cut || std::abs(ref_y) > consts.ref_residual_cut) {
			continue;
		}
		if(hist) {
			hist->candidate_res_track_x->Fill(track_x);
			hist->candidate_res_track_y->Fill(track_y);
			hist->candidate_res_ref_x->Fill(ref_x);
			hist->candidate_res_ref_y->Fill(ref_y);
		}
		accepted.push_back(track);
	}
	return accepted;
}

std::vector<std::pair<core::TripletTrack, Eigen::Vector3d>> TripletTrack::getTracksWithRefDut(constants_t consts,
                                                                  const core::run_data_t& run,
                                                                  histograms_t* hist,
                                                                  Eigen::Vector3d* new_ref_prealign,
								  Eigen::Vector3d* new_dut_prealign,
								  bool useDut, bool flipDut)
{
	assert(hist->down_angle_x);
	std::cout << "Flip DUT: " << flipDut << std::endl;
	std::cout << "DUT prealignment" << consts.dut_offset << std::endl;
	std::vector<std::pair<core::TripletTrack, Eigen::Vector3d>> candidates;
	MpaTransform transform;
	transform.setOffset(consts.dut_offset);
	transform.setRotation(consts.dut_rotation);
	int numMpa = 0;
	Transform trans;
	trans.setRotation(consts.dut_rotation);
	trans.setOffset(consts.dut_offset);
	for(size_t evt = 0; evt < run.tree->GetEntries(); ++evt) {
		if(evt%10000 == 0) {
			std::cout << "Processing event:" << std::setw(9)
				  << evt << std::endl;
		}
		run.tree->GetEntry(evt);
		auto downstream = core::Triplet::findTriplets(run, consts.angle_cut, consts.downstream_residual_cut, {3, 4, 5});
		// debug histograms
		for(const auto& triplet: downstream) {
			hist->down_angle_x->Fill(triplet.getdx() / triplet.getdz());
			hist->down_angle_y->Fill(triplet.getdy() / triplet.getdz());
			hist->down_res_x->Fill(triplet.getdx(1));
			hist->down_res_y->Fill(triplet.getdy(1));
		}
		auto upstream = core::Triplet::findTriplets(run, consts.angle_cut, consts.upstream_residual_cut, {0, 1, 2});
		// debug histograms
		for(const auto& triplet: upstream) {
			hist->up_angle_x->Fill(triplet.getdx() / triplet.getdz());
			hist->up_angle_y->Fill(triplet.getdy() / triplet.getdz());
			hist->up_res_x->Fill(triplet.getdx(1));
			hist->up_res_y->Fill(triplet.getdy(1));
		}
		// cut downstream triplets on their residual to ref hit
		auto refData = (*run.telescopeHits)->ref;
		std::vector<std::pair<core::Triplet, Eigen::Vector3d>> fullDownstream;
		for(int i = 0; i < refData.x.GetNoElements(); ++i) {
			Eigen::Vector3d hit(refData.x[i],
			                     refData.y[i],
					     refData.z[i]);
			for(auto triplet: downstream) {
				double resx = triplet.getdx(hit - consts.ref_prealign);
				double resy = triplet.getdy(hit - consts.ref_prealign);
				if(std::abs(resx) > consts.ref_residual_precut || std::abs(resy) > consts.ref_residual_precut) {
					continue;
				}
				hist->ref_down_res_x->Fill(resx);
				hist->ref_down_res_y->Fill(resy);
				fullDownstream.push_back({triplet, hit});
			}
		}
		// build upstream vector
		std::vector<std::pair<core::Triplet, Eigen::Vector3d>> fullUpstream;
		if(useDut) {
			std::vector<int> clusterSize;
			// TODO: Generalize DUT Hits -> use alibava hits for now
			// auto mpaHits = MpaHitGenerator::getCounterClusters(run, transform, &clusterSize, nullptr);
			auto hits = AlibavaHitGenerator::getHits(run, 0, flipDut);
			//for(const auto& hit: mpaHits) {
			for(const auto& triplet: upstream) { // Add 'empty' hit
				if( hits.empty() ) {
					fullUpstream.push_back({triplet, Eigen::Vector3d(-100, -100, -1)});
				}
				for(const auto& hit: hits) { 
					// NECESSARY FOR MPA
                                        //auto plane_hit = transform.mpaPlaneTrackIntersect(triplet); 
					auto plane_hit = trans.planeTripletIntersect(triplet);
					Eigen::Vector3d res = plane_hit - hit - consts.dut_offset; // prealignment
					// Eigen::Vector3d plane_local_hit = transform.getInverseRotationMatrix()*(res);
					// double resx = plane_local_hit(0);
					// double resy = plane_local_hit(1);
					double resx = res(0);
					double resy = res(1);
					//double resx = triplet.getdx(plane_local_hit(2));
					//double resy = triplet.getdy(plane_local_hit(2));
					hist->dut_up_res_x->Fill(resx);
					hist->dut_up_res_y->Fill(resy);					
					fullUpstream.push_back({triplet, hit});
				}
				
			}
			for(auto size: clusterSize) {
				hist->dut_cluster_size->Fill(size);
			}
		} else {
			for(const auto& triplet: upstream) {
				fullUpstream.push_back({triplet, {0, 0, 0}});
			}
		}
		// build tracks
		int numNewCandidates = 0;
		for(auto pair: fullDownstream) {
			auto down = pair.first;
			auto ref = pair.second;
			for(const auto uppair: fullUpstream) {
				auto up = uppair.first;
				Eigen::Vector3d dut = uppair.second;
				core::TripletTrack t(evt, up, down, ref);
				auto resx = t.xresidualat(consts.dut_offset(2));
				auto resy = t.yresidualat(consts.dut_offset(2));
				auto kinkx = t.kinkx();
				auto kinky = t.kinky();
				if(std::abs(resx) > consts.six_residual_cut || std::abs(resy) > consts.six_residual_cut) {
					continue;
				}
				if(std::abs(kinkx) > consts.six_kink_cut || std::abs(kinky) > consts.six_kink_cut) {
					continue;
				}
				hist->track_kink_x->Fill(kinkx);
				hist->track_kink_y->Fill(kinky);
				hist->track_residual_x->Fill(resx);
				hist->track_residual_y->Fill(resy);
				candidates.push_back({t, dut});
				++numNewCandidates;
			}
		}
		for(const auto& pair: fullDownstream) {
			const auto& hit = pair.first;
			const auto& ref = pair.second;
			for(int i = 0; i < 3; ++i) {
				hist->planes_z->Fill(hit[i](2));
			}
			hist->planes_z->Fill(ref(2));
		}
		for(const auto& hit: upstream) {
			for(int i = 0; i < 3; ++i) {
				hist->planes_z->Fill(hit[i](2));
			}
		}
	}
	// find new prealignment (more exact)
	Eigen::Vector3d refPreAlign(consts.ref_prealign);
	auto result = hist->ref_down_res_x->Fit("gaus", "FSMR", "");
	refPreAlign(0) += result->Parameter(1);
	result = hist->ref_down_res_y->Fit("gaus", "FSMR", "");
	refPreAlign(1) += result->Parameter(1);
	if(new_ref_prealign) {
		*new_ref_prealign = refPreAlign;
	}

	// find DUT prealignment	
	Eigen::Vector3d dutPreAlign(consts.dut_prealign);
	//dutPreAlign = fitDutPrealignment(hist->dut_up_res_x, hist->dut_up_res_y, transform, consts.dut_plateau_x);
	//refPreAlign(1) += result->Parameter(1);
	if(flipDut) {
		auto dutResult = hist->dut_up_res_y->Fit("gaus", "FSMR", "");
		dutPreAlign(1) += dutResult->Parameter(1);
		dutPreAlign(0) += Aligner::alignByDip(hist->dut_up_res_x);
	} else {
		auto dutResult = hist->dut_up_res_x->Fit("gaus", "FSMR", "");
		dutPreAlign(0) += dutResult->Parameter(1);
		dutPreAlign(1) += Aligner::alignByDip(hist->dut_up_res_y);
	}
	// z Align
	std::vector<std::pair<double, double>> z_res;
	for(auto iz = -20; iz<20; iz+=1) 
	{
		dutPreAlign(2) = iz;
		trans.setOffset(consts.dut_offset + dutPreAlign); // z = 370 + preAlign
		std::ostringstream hXName, hYName;
		hXName << "dut_res_x_z" << iz;
		hYName << "dut_res_y_z" << iz;
		auto dutResX = new TH1F(hXName.str().c_str(), "DUT residual in x", 500, -5, 5);
		auto dutResY = new TH1F(hXName.str().c_str(), "DUT residual in y", 500, -5, 5);
		for(auto pair : candidates) {
			TripletTrack track = pair.first;
			Eigen::Vector3d dut = pair.second + consts.dut_offset + dutPreAlign;
			Eigen::Vector3d plane_hit = trans.planeTripletIntersect(track.upstream());
			Eigen::Vector3d dut_res = plane_hit - dut;
			dutResX->Fill(dut_res(0));
			dutResY->Fill(dut_res(1));
		}
		if(flipDut) {
			auto fitResult = dutResY->Fit("gaus", "FSMR", "");
			float sigma = fitResult->Parameter(2);
			z_res.push_back(std::make_pair(iz, sigma));		
		} else {
			auto fitResult = dutResX->Fit("gaus", "FSMR", "");
			float sigma = fitResult->Parameter(2);
			z_res.push_back(std::make_pair(iz, sigma));		
		}
		hist->z_scan.push_back(dutResX);
	}

	double minRes = 10000;
	for(const auto& pair : z_res) 
	{
		std::cout << (consts.dut_offset(2)+pair.first) << " - " << pair.second << std::endl;
		if(pair.second < minRes) {
			minRes = pair.second;
			dutPreAlign(2) = pair.first;
		}		
	}
	if(new_dut_prealign) {
		*new_dut_prealign = dutPreAlign;
	}
	std::cout << "z correction: " << dutPreAlign(2) << std::endl;	
	std::vector<std::pair<core::TripletTrack, Eigen::Vector3d>> accepted;
	//transform.setOffset(consts.dut_offset + dutPreAlign);
	trans.setOffset(consts.dut_offset + dutPreAlign);
	// end of DUT prealignment


	// Final cuts (ref and dut)
	for(auto pair: candidates) {
		TripletTrack track = pair.first;
		Eigen::Vector3d dut = pair.second + consts.dut_offset + dutPreAlign; // activated DUT pixel in global coords
		auto track_x = track.xresidualat(consts.dut_offset(2));
		auto track_y = track.yresidualat(consts.dut_offset(2));
		auto ref_x = track.xrefresidual(refPreAlign);
		auto ref_y = track.yrefresidual(refPreAlign);
		//Eigen::Vector3d plane_hit = transform.mpaPlaneTrackIntersect(track.upstream());
		Eigen::Vector3d plane_hit = trans.planeTripletIntersect(track.upstream());
		Eigen::Vector3d dut_res = plane_hit - dut;
		if(std::abs(ref_x) > consts.ref_residual_cut || std::abs(ref_y) > consts.ref_residual_cut) {
			continue;
		}

		if(useDut && (std::abs(dut_res(0)) > consts.dut_residual_cut_x
				|| std::abs(dut_res(1)) > consts.dut_residual_cut_y)) {
			pair.second = Eigen::Vector3d(-1, -1, -1);				
		} else {
			pair.second = dut;
			hist->candidate_res_dut_x->Fill(dut_res(0));
			hist->candidate_res_dut_y->Fill(dut_res(1));
		}

		hist->candidate_res_track_x->Fill(track_x);
		hist->candidate_res_track_y->Fill(track_y);
		hist->candidate_res_ref_x->Fill(ref_x);
		hist->candidate_res_ref_y->Fill(ref_y);
		accepted.emplace_back(pair);
	}
	return accepted;
}

std::vector<std::tuple<core::TripletTrack, Eigen::Vector3d, AlibavaData>> TripletTrack::getTracksWithAlibava(constants_t consts, 
													     const core::run_data_t& run, 
													     histograms_t* hist, 
													     Eigen::Vector3d* new_ref_prealign, 
													     Eigen::Vector3d* new_dut_prealign,	
													     bool useDut, bool flipDut)
	

{
	std::cout << "Flip DUT: " << flipDut << std::endl;
	std::cout << "DUT prealignment" << consts.dut_offset << std::endl;
	std::vector<std::tuple<core::TripletTrack, Eigen::Vector3d, AlibavaData>> candidates;
	std::cout << "Got candidates" << std::endl;
	Transform trans;
	trans.setRotation(consts.dut_rotation);
	trans.setOffset(consts.dut_offset);
	for(size_t evt = 0; evt < run.tree->GetEntries(); ++evt) {
		if(evt%10000 == 0) {
			std::cout << "Processing event:" << std::setw(9)
				  << evt << std::endl;
		}
		run.tree->GetEntry(evt);
		auto downstream = core::Triplet::findTriplets(run, consts.angle_cut, consts.downstream_residual_cut, {3, 4, 5});
		// debug histograms
		for(const auto& triplet: downstream) {
			hist->down_angle_x->Fill(triplet.getdx() / triplet.getdz());
			hist->down_angle_y->Fill(triplet.getdy() / triplet.getdz());
			hist->down_res_x->Fill(triplet.getdx(1));
			hist->down_res_y->Fill(triplet.getdy(1));
		}
		auto upstream = core::Triplet::findTriplets(run, consts.angle_cut, consts.upstream_residual_cut, {0, 1, 2});
		// debug histograms
		for(const auto& triplet: upstream) {
			hist->up_angle_x->Fill(triplet.getdx() / triplet.getdz());
			hist->up_angle_y->Fill(triplet.getdy() / triplet.getdz());
			hist->up_res_x->Fill(triplet.getdx(1));
			hist->up_res_y->Fill(triplet.getdy(1));
		}
		// cut downstream triplets on their residual to ref hit
		auto refData = (*run.telescopeHits)->ref;
		std::vector<std::pair<core::Triplet, Eigen::Vector3d>> fullDownstream;
		for(int i = 0; i < refData.x.GetNoElements(); ++i) {
			Eigen::Vector3d hit(refData.x[i],
			                     refData.y[i],
					     refData.z[i]);
			for(auto triplet: downstream) {
				double resx = triplet.getdx(hit - consts.ref_prealign);
				double resy = triplet.getdy(hit - consts.ref_prealign);
				if(std::abs(resx) > consts.ref_residual_precut || std::abs(resy) > consts.ref_residual_precut) {
					continue;
				}
				hist->ref_down_res_x->Fill(resx);
				hist->ref_down_res_y->Fill(resy);
				fullDownstream.push_back({triplet, hit});
			}
		}
		// build upstream vector
		std::vector<std::tuple<core::Triplet, Eigen::Vector3d, AlibavaData>> fullUpstream;
		if(useDut) {
			auto hits = AlibavaHitGenerator::getHitsWithData(run, 0, flipDut); // vector<pair<Vector3d, AlibavaData>>
			for(const auto& triplet: upstream) { // Add 'empty' hit
				if( hits.empty() ) {
					AlibavaData emptyData;
					fullUpstream.emplace_back(std::make_tuple(triplet, Eigen::Vector3d(-100, -100, -1), emptyData));
				}
				for(const auto& hit: hits) { 
					auto plane_hit = trans.planeTripletIntersect(triplet); // Extrapolites to z(DUT) (no x and y)
					Eigen::Vector3d res = plane_hit - hit.first - consts.dut_offset; // prealignment
					double resx = res(0);
					double resy = res(1);
					hist->dut_up_res_x->Fill(resx);
					hist->dut_up_res_y->Fill(resy);					
					fullUpstream.push_back(std::make_tuple(triplet, hit.first, hit.second));
				}
				
			}
		} else {
			for(const auto& triplet: upstream) {
				AlibavaData emptyData;
				Eigen::Vector3d emptyVector = {0, 0, 0};
				fullUpstream.push_back(std::make_tuple(triplet, emptyVector, emptyData));
			}
		}
		// build tracks
		int numNewCandidates = 0;
		for(auto pair: fullDownstream) {
			auto down = pair.first;
			auto ref = pair.second;
			for(const auto uptuple: fullUpstream) {
				auto up = std::get<0>(uptuple);
				Eigen::Vector3d dut = std::get<1>(uptuple); // not aligned
				AlibavaData ali = std::get<2>(uptuple);
				core::TripletTrack t(evt, up, down, ref);
				auto resx = t.xresidualat(consts.dut_offset(2));
				auto resy = t.yresidualat(consts.dut_offset(2));
				auto kinkx = t.kinkx();
				auto kinky = t.kinky();
				if(std::abs(resx) > consts.six_residual_cut || std::abs(resy) > consts.six_residual_cut) {
					continue;
				}
				if(std::abs(kinkx) > consts.six_kink_cut || std::abs(kinky) > consts.six_kink_cut) {
					continue;
				}
				hist->track_kink_x->Fill(kinkx);
				hist->track_kink_y->Fill(kinky);
				hist->track_residual_x->Fill(resx);
				hist->track_residual_y->Fill(resy);
				candidates.push_back(std::make_tuple(t, dut, ali));
				++numNewCandidates;
			}
		}
		// z positions of all planes
		for(const auto& pair: fullDownstream) {
			const auto& hit = pair.first;
			const auto& ref = pair.second;
			for(int i = 0; i < 3; ++i) {
				hist->planes_z->Fill(hit[i](2));
			}
			hist->planes_z->Fill(ref(2));
		}
		for(const auto& hit: upstream) {
			for(int i = 0; i < 3; ++i) {
				hist->planes_z->Fill(hit[i](2));
			}
		}
	}
	// find new prealignment (more exact)
	Eigen::Vector3d refPreAlign(consts.ref_prealign);
	auto result = hist->ref_down_res_x->Fit("gaus", "FSMR", "");
	refPreAlign(0) += result->Parameter(1);
	result = hist->ref_down_res_y->Fit("gaus", "FSMR", "");
	refPreAlign(1) += result->Parameter(1);
	if(new_ref_prealign) {
		*new_ref_prealign = refPreAlign;
	}

	// find DUT prealignment	
	Eigen::Vector3d dutPreAlign(consts.dut_offset);
	if(flipDut) {
		auto dutResult = hist->dut_up_res_y->Fit("gaus", "FSMR", "");
		dutPreAlign(1) -= dutResult->Parameter(1);
		std::cout << "Mean: " << dutResult->Parameter(1) << " : " 
			  << dutPreAlign(1) << std::endl;
		//dutPreAlign(1) += 0.0;
		dutPreAlign(0) -= Aligner::alignByDip(hist->dut_up_res_x);
	} else {
		auto dutResult = hist->dut_up_res_x->Fit("gaus", "FSMR", "");
		dutPreAlign(0) -= dutResult->Parameter(1);
		std::cout << "Mean: " << dutResult->Parameter(0) << " : " 
			  << dutPreAlign(0) << std::endl;
		//dutPreAlign(0) += 0.0;
		dutPreAlign(1) -= Aligner::alignByDip(hist->dut_up_res_y);
	}
	// z Align
	std::cout << "Start z scan" << std::endl;
	std::vector<std::pair<double, double>> z_res;
	for(auto iz = -20; iz<20; iz+=1) 
	{
		dutPreAlign(2) = iz;
		trans.setOffset(consts.dut_offset + dutPreAlign); // z = 370 + preAlign
		std::ostringstream hXName, hYName;
		hXName << "dut_res_x_z" << iz;
		hYName << "dut_res_y_z" << iz;
		auto dutResX = new TH1F(hXName.str().c_str(), "DUT residual in x", 500, -5, 5);
		auto dutResY = new TH1F(hXName.str().c_str(), "DUT residual in y", 500, -5, 5);
		for(auto tuple : candidates) {
			TripletTrack track = std::get<0>(tuple);
			Eigen::Vector3d dut = std::get<1>(tuple) + consts.dut_offset + dutPreAlign;
			Eigen::Vector3d plane_hit = trans.planeTripletIntersect(track.upstream());
			Eigen::Vector3d dut_res = plane_hit - dut;
			dutResX->Fill(dut_res(0));
			dutResY->Fill(dut_res(1));
		}
		if(flipDut)
			auto result = dutResY->Fit("gaus", "FSMR", "");
		else
			auto result = dutResX->Fit("gaus", "FSMR", "");
		z_res.emplace_back(std::make_pair(iz, result->Parameter(2)));		
		hist->z_scan.emplace_back(dutResX);
	}

	double minRes = 10000;
	for(const auto& pair : z_res) 
	{
		std::cout << pair.first << " : " << pair.second << std::endl;
		if(pair.second < minRes) {
			minRes = pair.second;
			dutPreAlign(2) = pair.first;
		}		
	}
	if(new_dut_prealign) {
		*new_dut_prealign = dutPreAlign;
	}
	std::cout << "z correction: " << dutPreAlign(2) << std::endl;	
	trans.setOffset(consts.dut_offset + dutPreAlign);
	// end of DUT prealignment

	std::vector<std::tuple<core::TripletTrack, Eigen::Vector3d, AlibavaData>> accepted;
	// Final cuts (ref and dut)
	for(auto tuple: candidates) {
		TripletTrack track = std::get<0>(tuple);
		Eigen::Vector3d dut = std::get<1>(tuple) + consts.dut_offset + dutPreAlign; // activated DUT pixel in global coords
		auto track_x = track.xresidualat(consts.dut_offset(2));
		auto track_y = track.yresidualat(consts.dut_offset(2));
		auto ref_x = track.xrefresidual(refPreAlign);
		auto ref_y = track.yrefresidual(refPreAlign);
		//Eigen::Vector3d plane_hit = transform.mpaPlaneTrackIntersect(track.upstream());
		Eigen::Vector3d plane_hit = trans.planeTripletIntersect(track.upstream());
		Eigen::Vector3d dut_res = plane_hit - dut;
		if(std::abs(ref_x) > consts.ref_residual_cut || std::abs(ref_y) > consts.ref_residual_cut) {
			continue;
		}

		if(useDut && (std::abs(dut_res(0)) > consts.dut_residual_cut_x
				|| std::abs(dut_res(1)) > consts.dut_residual_cut_y)) {
			std::get<1>(tuple) = Eigen::Vector3d(-1, -1, -1);				
		} else {
			std::get<1>(tuple) = dut;
			hist->candidate_res_dut_x->Fill(dut_res(0));
			hist->candidate_res_dut_y->Fill(dut_res(1));
		}

		hist->candidate_res_track_x->Fill(track_x);
		hist->candidate_res_track_y->Fill(track_y);
		hist->candidate_res_ref_x->Fill(ref_x);
		hist->candidate_res_ref_y->Fill(ref_y);
		accepted.push_back(tuple);
	}

	std::cout << "Finished track processing!" << std::endl;
	return accepted;

}

Eigen::Vector3d TripletTrack::fitDutPrealignment(TH1D* x, TH1D* y, const MpaTransform& transform, bool plateau_x)
{
	Eigen::Vector3d offset{0, 0, 0};
	if(plateau_x) {
		offset(0) = Aligner::alignPlateau(x, 1, 0.005, true)(0);
		offset(1) = Aligner::alignGaussian(y, 0.5, 1, true)(0);
	} else {
		offset(0) = Aligner::alignGaussian(x, 0.5, 1, true)(0);
		offset(1) = Aligner::alignPlateau(y, 1, 0.005, true)(0);
	}
	return offset;
}

std::ostream& operator<<(std::ostream& stream, const TripletTrack& T)
{
	stream << T.upstream() << T.downstream();
	if(T.hasRef()) {
		auto h = T.refHit();
		stream << h(0) << " " << h(1) << " " << h(2) << "\n";
	}
	return stream;
}
