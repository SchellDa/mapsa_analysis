
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
	_trackHists = core::TripletTrack::genDebugHistograms();
	_trackConsts.angle_cut = 0.16;
	_trackConsts.upstream_residual_cut = 0.1;
	_trackConsts.downstream_residual_cut = 0.1;
	_trackConsts.six_residual_cut = 0.1;
	_trackConsts.six_kink_cut = 0.01;
	_trackConsts.ref_residual_precut = 0.7;
	_trackConsts.ref_residual_cut = 0.1;
	_trackConsts.dut_z = 385;
	_gbl_chi2_dist = new TH1F("gbl_chi2ndf_dist", "", 1000, 0, 100);
}

void GblAlign::run(const core::MergedAnalysis::run_data_t& run)
{
	loadPrealignment();
	_trackConsts.ref_prealign = _refPreAlign;
	auto trackCandidates = core::TripletTrack::getTracksWithRef(_trackConsts, run, _trackHists, &_refPreAlign);
	std::cout << " * new extrapolated ref prealignment:\n" << _refPreAlign << std::endl;
	std::ofstream fout(getFilename("_all_tracks.csv"));
	for(const auto& track: trackCandidates) {
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

Eigen::MatrixXd GblAlign::getDerivatives(core::Triplet t, double dut_z, Eigen::Vector3d angles)
{
	double turn = angles(0);
	double tilt = angles(1);
	double DUTrot = angles(2);
	double upsign = -1;
	double wt = atan(1.0) / 45.0;
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

void GblAlign::fitTracks(std::vector<core::TripletTrack> trackCandidates)
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

