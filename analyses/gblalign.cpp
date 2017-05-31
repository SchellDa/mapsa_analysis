
#include "gblalign.h"
#include <iostream>
#include <TImage.h>
#include <TCanvas.h>
#include <TFitResult.h>
#include <fstream>
#include "mpatransform.h"
#include <TF1.h>
#include <GblTrajectory.h>
#include <MilleBinary.h>

REGISTER_ANALYSIS_TYPE(GblAlign, "Uses pre-alignment data and GBL to generate alignment using PEDE")

GblAlign::GblAlign() :
 core::MergedAnalysis(), _file(nullptr)
{
}

GblAlign::~GblAlign()
{
	_file->Write();
	_file->Close();
	delete _file;
	_file = nullptr;
}

void GblAlign::init()
{
	loadResolutions();
	_eBeam = _config.get<double>("e_beam");
	_file = new TFile(getRootFilename().c_str(), "recreate");
	_down_angle_x = new TH1F("downstream_angle_x", "Angle of downstream triplets", 100, 0, 0.5);
	_down_angle_y = new TH1F("downstream_angle_y", "Angle of downstream triplets", 100, 0, 0.5);
	_down_res_x = new TH1F("downstream_res_x", "Center residual of downstream triplets", 100, 0, 0.5);
	_down_res_y = new TH1F("downstream_res_y", "Center residual of downstream triplets", 100, 0, 0.5);
	_up_angle_x = new TH1F("upstream_angle_x", "Angle of upstream triplets", 100, 0, 0.5);
	_up_angle_y = new TH1F("upstream_angle_y", "Angle of upstream triplets", 100, 0, 0.5);
	_up_res_x = new TH1F("upstream_res_x", "Center residual of upstream triplets", 100, 0, 0.5);
	_up_res_y = new TH1F("upstream_res_y", "Center residual of upstream triplets", 100, 0, 0.5);
	_ref_down_res_x = new TH1F("ref_downstream_res_x", "Residual between downstream and ref", 1000, -5, 5);
	_ref_down_res_y = new TH1F("ref_downstream_res_y", "Residual between downstream and ref", 1000, -5, 5);
	_dut_up_res_x = new TH1F("dut_upstream_res_x", "Residual between upstream and dut", 1000, -5, 5);
	_dut_up_res_y = new TH1F("dut_upstream_res_y", "Residual between upstream and dut", 1000, -5, 5);
	_track_kink_x = new TH1F("track_kink_x", "", 1000, 0, 0.1);
	_track_kink_y = new TH1F("track_kink_y", "", 1000, 0, 0.1);
	_track_residual_x = new TH1F("track_residual_x", "", 1000, -1, 10);
	_track_residual_y = new TH1F("track_residual_y", "", 1000, -1, 10);
	_planes_z = new TH1F("planes_z", "Z positions of hits in accepted triplets", 500, -100, 1000);
	_candidate_res_track_x = new TH1F("candidate_res_track_x", "", 500, -1, 1);
	_candidate_res_track_y = new TH1F("candidate_res_track_y", "", 500, -1, 1);
	_candidate_res_ref_x = new TH1F("candidate_res_ref_x", "", 500, -1, 1);
	_candidate_res_ref_y = new TH1F("candidate_res_ref_y", "", 500, -1, 1);
	_gbl_chi2_dist = new TH1F("gbl_chi2ndf_dist", "", 1000, 0, 100);
}

void GblAlign::run(const core::MergedAnalysis::run_data_t& run)
{
	loadPrealignment();
	std::vector<Track> trackCandidates { getTrackCandidates(100000, run) };
	fitTracks(trackCandidates);
}

void GblAlign::finalize()
{
}

Eigen::MatrixXd GblAlign::jacobianStep(double step)
{
	Eigen::Matrix<double, 5, 5> jac{ Eigen::Matrix<double, 5, 5>::Identity() };
	jac(3,1) = step;
	jac(4,2) = step;
	return jac;
}

Eigen::MatrixXd GblAlign::getDerivatives(Triplet t, double dut_z, Eigen::Vector3d angles)
{
	double turn = angles(0);
	double tilt = angles(1);
	double DUTrot = angles(2);
	double upsign = -1;
	double wt = atan(1) / 180;
	double Nx =-sin( turn*wt )*cos( tilt*wt );
	double Ny = sin( tilt*wt );
	double Nz =-cos( turn*wt )*cos( tilt*wt );
	double co = cos( turn*wt );
	double so = sin( turn*wt );
	double ca = cos( tilt*wt );
	double sa = sin( tilt*wt );
	double cf = cos( DUTrot );
	double sf = sin( DUTrot );

	double dNxdo =-co*ca;
	double dNxda = so*sa;
	double dNydo = 0;
	double dNyda = ca;
	double dNzdo = so*ca;
	double dNzda = co*sa;

	double xA = t.extrapolatex(dut_z); // triplet impact point on CMS
	double yA = t.extrapolatey(dut_z);
	double avx = t.base()(0);
	double avy = t.base()(1);
	double avz = t.base()(2);
	double tx = t.slope()(0);
	double ty = t.slope()(1);
	double zA = dut_z - avz; // z CMS from mid of triplet
	double zc = (Nz*zA - Ny*avy - Nx*avx) / (Nx*tx + Ny*ty + Nz); // from avz
	double yc = avy + ty * zc;
	double xc = avx + tx * zc;

	// transform into DUT system: (passive).
	// large rotations don't commute: careful with order

	double dzc = zc + avz - dut_z;
	
	double x1 = co*xc - so*dzc; // turn
	double y1 = yc;
	double z1 = so*xc + co*dzc;
	
	double x2 = x1;
	double y2 = ca*y1 + sa*z1; // tilt
	double z2 =-sa*y1 + ca*z1; // should be zero (in DUT plane). is zero
	
	double dxcdz = tx*Nz / (Nx*tx + Ny*ty + Nz);
	double dycdz = ty*Nz / (Nx*tx + Ny*ty + Nz);
	
	double dzcda = ( dNzda*zA - dNyda*avy - dNxda*avx ) / (Nx*tx + Ny*ty + Nz) - zc*( dNxda*tx + dNyda*ty + dNzda ) / (Nx*tx + Ny*ty + Nz);
	double dzcdo = ( dNzdo*zA - dNydo*avy - dNxdo*avx ) / (Nx*tx + Ny*ty + Nz) - zc*( dNxdo*tx + dNydo*ty + dNzdo ) / (Nx*tx + Ny*ty + Nz);
	
	double ddzcdz = Nz / (Nx*tx + Ny*ty + Nz) - 1; // ddzc/dDUTz

	Eigen::Matrix<double, 2, 6> der;
	der(0,0) = -1.0; // dresidx/ddeltax
	der(1,0) =  0.0;

	der(0,1) =  0.0;
	der(1,1) = -1.0; // dresidy/ddeltay

	der(0,2) = -upsign*y2; // dresidx/drot, linearized
	der(1,2) = -upsign*x2; // dresidy/drot

	der(0,3) = -upsign*( cf*(-so*dzcda)+sf*(-sa*y1 + ca*z1 + sa*co*dzcda)); // dresidx/dtilt a
	der(1,3) =  upsign*(-sf*(-so*dzcda)+cf*(-sa*y1 + ca*z1 + sa*co*dzcda)); // dresidy/dtilt a

	der(0,4) = -upsign*( cf*(-so*xc - co*dzc - so*dzcdo) + sf*sa*(co*xc-so*dzc+co*dzcdo)); // dresidx/dturn o
	der(1,4) =  upsign*(-sf*(-so*xc - co*dzc - so*dzcdo) + cf*sa*(co*xc-so*dzc+co*dzcdo)); // dresidy/dturn o

 	der(0,5) = -upsign*( cf*(co*dxcdz-so*ddzcdz) + sf*(ca*dycdz+sa*(so*dxcdz+co*ddzcdz))); // dresidx/dz
  	der(1,5) =  upsign*(-sf*(co*dxcdz-so*ddzcdz) + cf*(ca*dycdz+sa*(so*dxcdz+co*ddzcdz))); // dresidy/dz
	return der;
}

std::vector<Triplet> GblAlign::findTriplets(const core::MergedAnalysis::run_data_t& run, double angle_cut, double residual_cut,
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

std::vector<Track> GblAlign::getTrackCandidates(size_t maxCandidates, const core::MergedAnalysis::run_data_t& run)
{
	const double angle_cut = 0.16;
	const double residual_cut = 0.1;
	const double ref_residual_cut = 0.7;
	const double ref_residual_cut_narrow = 0.06;
	const double track_residual_cut = 0.1;
	const double kink_cut = 0.001;
	const double dut_z = 385;
	std::vector<Track> candidates;
	for(size_t evt = 0; evt < run.tree->GetEntries(); ++evt) {
		run.tree->GetEntry(evt);
		auto downstream = findTriplets(run, angle_cut, residual_cut, {3, 4, 5});
		// debug histograms
		for(const auto& triplet: downstream) {
			_down_angle_x->Fill(std::abs(triplet.getdx() / triplet.getdz()));
			_down_angle_y->Fill(std::abs(triplet.getdy() / triplet.getdz()));
			_down_res_x->Fill(std::abs(triplet.getdx(1)));
			_down_res_y->Fill(std::abs(triplet.getdy(1)));
		}
		auto upstream = findTriplets(run, angle_cut*100, residual_cut*100, {0, 1, 2});
		// debug histograms
		for(const auto& triplet: upstream) {
			_up_angle_x->Fill(std::abs(triplet.getdx() / triplet.getdz()));
			_up_angle_y->Fill(std::abs(triplet.getdy() / triplet.getdz()));
			_up_res_x->Fill(std::abs(triplet.getdx(1)));
			_up_res_y->Fill(std::abs(triplet.getdy(1)));
		}
		// cut downstream triplets on their residual to ref hit
		auto refData = (*run.telescopeHits)->ref;
//		std::vector<Triplet> acceptedDownstream;
		std::vector<std::pair<Triplet, Eigen::Vector3d>> fullDownstream;
		for(int i = 0; i < refData.x.GetNoElements(); ++i) {
			Eigen::Vector3d hit(refData.x[i],
			                     refData.y[i],
					     refData.z[i]);
			for(const auto& triplet: downstream) {
				double resx = triplet.getdx(hit - _refPreAlign);
				double resy = triplet.getdy(hit - _refPreAlign);
				if(std::abs(resx) > ref_residual_cut || std::abs(resy) > ref_residual_cut) {
					continue;
				}
				_ref_down_res_x->Fill(resx);
				_ref_down_res_y->Fill(resy);
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
				Track t(up, down, ref);
				auto resx = t.xresidualat(dut_z);
				auto resy = t.yresidualat(dut_z);
				auto kinkx = std::abs(t.kinkx());
				auto kinky = std::abs(t.kinky());
				if(std::abs(resx) > track_residual_cut || std::abs(resy) > track_residual_cut) {
					continue;
				}
				if(kinkx > kink_cut || kinky > kink_cut) {
					continue;
				}
				_track_kink_x->Fill(kinkx);
				_track_kink_y->Fill(kinky);
				_track_residual_x->Fill(resx);
				_track_residual_y->Fill(resy);
				candidates.push_back(t);
			}
		}
		for(const auto& pair: fullDownstream) {
			const auto& hit = pair.first;
			const auto& ref = pair.second;
			for(int i = 0; i < 3; ++i) {
				_planes_z->Fill(hit[i](2));
			}
			_planes_z->Fill(ref(2));
		}
		for(const auto& hit: upstream) {
			for(int i = 0; i < 3; ++i) {
				_planes_z->Fill(hit[i](2));
			}
		}
	}
	// find new prealignment (more exact)
	auto result = _ref_down_res_x->Fit("gaus", "FSMR", "");
	_refPreAlign(0) += result->Parameter(1);
	result = _ref_down_res_y->Fit("gaus", "FSMR", "");
	_refPreAlign(1) += result->Parameter(1);
	std::cout << " * extrapolated ref pre-alignment\n" << _refPreAlign << std::endl;
	std::vector<Track> accepted;
	for(auto track: candidates) {
		auto track_x = track.xresidualat(dut_z);
		auto track_y = track.yresidualat(dut_z);
		auto ref_x = track.xrefresidual(_refPreAlign);
		auto ref_y = track.yrefresidual(_refPreAlign);
		if(std::abs(ref_x) > ref_residual_cut_narrow || std::abs(ref_y) > ref_residual_cut_narrow) {
			continue;
		}
		_candidate_res_track_x->Fill(track_x);
		_candidate_res_track_y->Fill(track_y);
		_candidate_res_ref_x->Fill(ref_x);
		_candidate_res_ref_y->Fill(ref_y);
		accepted.push_back(track);
	}
	std::ofstream fout(getFilename("_all_tracks.csv"));
	for(const auto& track: candidates) {
		for(const auto& hit: track.upstream().getHits()) {
			fout << hit(0) << " " << hit(1) << " " << hit(2) << "\n";
		}
		for(const auto& hit: track.downstream().getHits()) {
			fout << hit(0) << " " << hit(1) << " " << hit(2) << "\n";
		}
		//for(int i = 0; i < 3; ++i) {
		//	auto hit = track.upstream()[i];
		//	fout << hit(0) << " " << hit(1) << " " << hit(2) << "\n";
		//}
		//for(int i = 0; i < 3; ++i) {
		//	auto hit = track.downstream()[i];
		//	fout << hit(0) << " " << hit(1) << " " << hit(2) << "\n";
		//}
		//auto hit = track.refHit();
		//fout << hit(0) << " " << hit(1) << " " << hit(2) << "\n";
		fout << "\n\n";
	}
	return accepted;
}

void GblAlign::fitTracks(std::vector<Track> trackCandidates)
{
	Eigen::Matrix2d proj;
	proj << 1, 0,
	        0, 1;
	Eigen::Vector2d scatter(0, 0);
	double X0Si = 65e-3 / 94;
	double tetSi = 0.0136 * std::sqrt(X0Si) / _eBeam * (1 + 0.038 * std::log(X0Si));
	Eigen::Vector2d wscatter(1, 1);
	wscatter *= 1.0 / tetSi / tetSi;
	std::ofstream fout(getFilename("_trackfits.csv"));
	std::ofstream fout_tracks(getFilename("_tracks.csv"));
	gbl::MilleBinary mille(getFilename("_mille.bin"));	
	for(const auto& track: trackCandidates) {
		std::vector<gbl::GblPoint> trajectory;
		double prev_z = track.upstream()[0](0);
		for(const auto& hit: track.upstream().getHits()) {
			gbl::GblPoint p(jacobianStep(hit(2) - prev_z));
			prev_z = hit(2);
			p.addMeasurement(proj, track.upstream().getds(hit), _precisionTel);
			p.addScatterer(scatter, wscatter);
			trajectory.push_back(p);
			fout_tracks << hit(0) << " " << hit(1) << " " << hit(2) << "\n";
		} {
		}
		for(const auto& hit: track.downstream().getHits()) {
			gbl::GblPoint p(jacobianStep(hit(2) - prev_z));
			prev_z = hit(2);
			p.addMeasurement(proj, track.downstream().getds(hit), _precisionTel);
			p.addScatterer(scatter, wscatter);
			trajectory.push_back(p);
			fout_tracks << hit(0) << " " << hit(1) << " " << hit(2) << "\n";
		} {
			Eigen::Vector3d hit = track.refHit() - _refPreAlign;
			fout_tracks << hit(0) << " " << hit(1) << " " << hit(2) << "\n";
			gbl::GblPoint p(jacobianStep(hit(2) - prev_z));
			prev_z = hit(2);
			p.addMeasurement(proj, track.downstream().getds(hit), _precisionRef);
			p.addScatterer(scatter, wscatter);
//			auto der = getDerivatives(track.upstream(), hit(2), {0, 0, 0});
			std::vector<int> labels(3);
			labels[0] = 1;
			labels[1] = 2;
			labels[2] = 3;
//			labels[3] = 4;
//			labels[4] = 5;
//			labels[5] = 6;
			Eigen::Matrix<double, 2, 3> der;
			der << 1.0, 0.0, track.upstream().slope()(0),
			       0.0, 1.0, track.upstream().slope()(1);
			p.addGlobals(labels, der);
			trajectory.push_back(p);
		}
		gbl::GblTrajectory gblTrajectory(trajectory, false);
		double chi2, lostWeight;
		int Ndf;
		gblTrajectory.fit(chi2, Ndf, lostWeight);
		_gbl_chi2_dist->Fill(chi2/Ndf);
		gblTrajectory.milleOut(mille);
		fout << chi2 << "," << Ndf << "," << lostWeight << "\n";
		fout_tracks << "\n\n";
	}
	fout_tracks.flush();
	fout_tracks.close();
	fout.flush();
	fout.close();
	std::ofstream fsteer(getFilename("_steering.txt"));
	fsteer << "! Generated by GblAlign\n"
	       << "Cfiles\n"
	       << getFilename("_mille.bin") << "\n"
	       << "\n"
	       << "Parameter\n"
	       << "1  0.0  0.0\n" // dx
	       << "2  0.0  0.0\n" // dy
	       << "3  0.0  0.0\n" // dz
//	       << "1  0.0  0.0\n" // dx
//	       << "2  0.0  0.0\n" // dy
//	       << "3  0.0  0.0\n" // rot
//	       << "4  0.0  0.0\n" // dtilt
//	       << "5  0.0  0.0\n" // dturn
//	       << "6  0.0  0.0\n" // dz
	       << "\n"
	       << "! chisqcut 5.0 2.5\n"
	       << "outlierdownweighting 4\n"
	       << "\n"
	       << "method inversion 10 0.1\n"
	       << "threads 10 1\n"
	       << "\n"
	       << "histprint\n"
	       << "\n"
	       << "end\n";
}

Eigen::Vector3d GblAlign::calcFitDebugHistograms(int planeId, gbl::GblTrajectory* traj)
{
	//TVectorD correction(5);
	//TMatrixDSym cov(5);
	//tray->getResults(planeId, correction, cov);
	//std::string prefix("Plane ");
	//prefix += std::to_string(plane_id);
	//prefix += " Correction;
	//auto histAngle = new TH1F((prefix+" 
	return {0, 0};
}

void GblAlign::loadPrealignment()
{
	std::cout << "Load Prealignment for current run" << std::endl;
	auto refFilename = getFilename("RefPreAlign", "_ref.csv", false, false);
	auto dutFilename = getFilename("RefPreAlign", "_dut.csv", false, false);
	std::cout << " * " << refFilename << std::endl;
	std::cout << " * " << dutFilename << std::endl;
	std::ifstream fin(refFilename);
	if(!fin.good()) {
		throw std::runtime_error("Cannot open ref prealignment file");
	}
	double x, y;
	fin >> x >> y;
	_refPreAlign = Eigen::Vector3d { x, y, 0 };
	std::cout <<  " * ref:\n" << _refPreAlign << std::endl;
	fin.close();
	fin.open(dutFilename);
	if(!fin.good()) {
		throw std::runtime_error("Cannot open dut prealignment file");
	}
	fin >> x >> y;
	_dutPreAlign = Eigen::Vector3d { x, y, 0 };
	std::cout <<  " * dut:\n" << _dutPreAlign << std::endl;
}

void GblAlign::loadResolutions()
{
	auto tel_x = _config.get<double>("gbl_resolution_tel_x");
	auto tel_y = _config.get<double>("gbl_resolution_tel_y");
	auto ref_x = _config.get<double>("gbl_resolution_ref_x");
	auto ref_y = _config.get<double>("gbl_resolution_ref_y");
	auto mpa_x = _config.get<double>("gbl_resolution_mpa_x");
	auto mpa_y = _config.get<double>("gbl_resolution_mpa_y");
	_precisionTel = Eigen::Vector2d(1 / tel_x / tel_x, 1 / tel_y / tel_y);
	_precisionRef = Eigen::Vector2d(1 / ref_x / ref_x, 1 / ref_y / ref_y);
	_precisionMpa = Eigen::Vector2d(1 / mpa_x / mpa_x, 1 / mpa_y / mpa_y);
}

