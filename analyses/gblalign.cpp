
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
	_trackConsts.angle_cut = _config.get<double>("angle_cut");
	_trackConsts.upstream_residual_cut = _config.get<double>("upstream_residual_cut");
	_trackConsts.downstream_residual_cut = _config.get<double>("downstream_residual_cut");
	_trackConsts.six_residual_cut = _config.get<double>("six_residual_cut");
	_trackConsts.six_kink_cut = _config.get<double>("six_kink_cut");
	_trackConsts.ref_residual_precut = _config.get<double>("ref_residual_precut");
	_trackConsts.ref_residual_cut = _config.get<double>("ref_residual_cut");
	_trackConsts.dut_residual_cut_x = _config.get<double>("dut_residual_cut_x");
	_trackConsts.dut_residual_cut_y = _config.get<double>("dut_residual_cut_y");
	_trackConsts.dut_offset = Eigen::Vector3d({
		_config.get<double>("dut_x"),
		_config.get<double>("dut_y"),
		_config.get<double>("dut_z")
		});
	_trackConsts.dut_rotation = Eigen::Vector3d({
		_config.get<double>("dut_phi"),
		_config.get<double>("dut_theta"),
		_config.get<double>("dut_omega")
		}) * M_PI / 180;
	_trackConsts.dut_plateau_x = true;
	std::cout << "DUT Offset:\n" << _trackConsts.dut_offset << std::endl;
	std::cout << "DUT Rotation:\n" << _trackConsts.dut_rotation << std::endl;
	_gbl_chi2_dist = new TH1F("gbl_chi2ndf_dist", "", 1000, 0, 100);
}

void GblAlign::run(const core::run_data_t& run)
{
	loadPrealignment();
	_trackConsts.ref_prealign = _refPreAlign;
	auto trackCandidates = core::TripletTrack::getTracksWithRefDut(_trackConsts, run, _trackHists, &_refPreAlign, &_dutPreAlign);
	std::cout << " * new extrapolated ref prealignment:\n" << _refPreAlign << std::endl;
	std::cout << " * dut prealignment:\n" << _dutPreAlign << std::endl;
	std::ofstream fout(getFilename("_all_tracks.csv"));
	for(auto pair: trackCandidates) {
		auto track = pair.first;
		fout << track.upstream();
		Eigen::Vector3d hit;
		hit = pair.second - _dutPreAlign;
		fout << hit(0) << " " << hit(1) << " " << hit(2) << "\n";
		fout << track.downstream();
		hit = track.refHit() + _refPreAlign;
		fout << hit(0) << " " << hit(1) << " " << hit(2) << "\n";
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
	const double phi = angles(0);
	const double theta = angles(1);
	const double omega = angles(2);
	const double k_x = 0;
	const double k_y = 0;
	const double k_z = dut_z;

	const double s_x = t.slope()(0);
	const double s_y = t.slope()(1);

	const double tb_x = t.base()(0);
	const double tb_y = t.base()(1);
	const double tb_z = t.base()(2);

	const double cp = cos(phi);
	const double sp = sin(phi);
	const double ct = cos(theta);
	const double st = sin(theta);
	const double co = cos(omega);
	const double so = sin(omega);

	double drxdx = ((2 * co * s_x * ct + so * cp * s_y + so * sp) * st + (2 * co * cp - 2 * co * sp * s_y) * ct*ct + co * sp * s_y - co * cp) / (s_x * st + (cp - sp * s_y) * ct);
	double drydx =  - ((2 * so * s_x * ct - co * cp * s_y - co * sp) * st + (2 * so * cp - 2 * so * sp * s_y) * ct*ct + so * sp * s_y - so * cp) / (s_x * st + (cp - sp * s_y) * ct);
	double drxdy =  - (((2 * co * sp*sp * s_y - 2 * co * cp * sp) * ct - so * cp * s_x) * st + 2 * co * sp * s_x * ct*ct + (2 * so * cp * sp * s_y + 2 * so * sp*sp - so) * ct - co * sp * s_x) / (s_x * st + (cp - sp * s_y) * ct);
	double drydy = (((2 * so * sp*sp * s_y - 2 * so * cp * sp) * ct + co * cp * s_x) * st + 2 * so * sp * s_x * ct*ct + ( - 2 * co * cp * sp * s_y - 2 * co * sp*sp + co) * ct - so * sp * s_x) / (s_x * st + (cp - sp * s_y) * ct);
	double drxdz = (((2 * co * cp * sp * s_y - 2 * co * cp*cp) * ct + so * sp * s_x) * st + 2 * co * cp * s_x * ct*ct + ((2 * so * cp*cp - so) * s_y + 2 * so * cp * sp) * ct - co * cp * s_x) / (s_x * st + (cp - sp * s_y) * ct);
	double drydz =  - (((2 * so * cp * sp * s_y - 2 * so * cp*cp) * ct - co * sp * s_x) * st + 2 * so * cp * s_x * ct*ct + ((co - 2 * co * cp*cp) * s_y - 2 * co * cp * sp) * ct - so * cp * s_x) / (s_x * st + (cp - sp * s_y) * ct);
	double drxdphi =  - ((((4 * k_z * co * sp*sp + 4 * k_y * co * cp * sp - 2 * k_z * co) * s_x * s_y + (4 * k_y * co * sp*sp - 4 * k_z * co * cp * sp - 2 * k_y * co) * s_x) * ct - so * cp * s_x*s_x * tb_z + so * sp * s_x*s_x * tb_y + (so * cp * s_x - so * sp * s_x * s_y) * tb_x + k_x * so * sp * s_x * s_y + (k_y * so * sp - k_z * so * cp) * s_x*s_x - k_x * so * cp * s_x) * st*st + ((( - 2 * k_z * co * pow(sp, 3) - 2 * k_y * co * cp * sp*sp) * s_y*s_y + ( - 4 * k_y * co * pow(sp, 3) + 4 * k_z * co * cp * sp*sp + 4 * k_y * co * sp) * s_y + (2 * k_z * co * sp + 2 * k_y * co * cp) * s_x*s_x + 2 * k_z * co * pow(sp, 3) + 2 * k_y * co * cp * sp*sp - 2 * k_z * co * sp - 2 * k_y * co * cp) * ct*ct + ( - so * s_x * tb_z - so * s_x * s_y * tb_y + (so * s_y*s_y + so) * tb_x - k_x * so * s_y*s_y + ( - 4 * k_y * so * sp*sp + 4 * k_z * so * cp * sp + k_y * so) * s_x * s_y + (4 * k_z * so * sp*sp + 4 * k_y * so * cp * sp - 3 * k_z * so) * s_x - k_x * so) * ct - co * sp * s_x*s_x * tb_z - co * cp * s_x*s_x * tb_y + (co * cp * s_x * s_y + co * sp * s_x) * tb_x - k_x * co * cp * s_x * s_y + ( - k_z * co * sp - k_y * co * cp) * s_x*s_x - k_x * co * sp * s_x) * st + (2 * k_y * co * s_x - 2 * k_z * co * s_x * s_y) * pow(ct, 3) + ((so * cp * s_y*s_y + so * sp * s_y) * tb_z + ( - so * cp * s_y - so * sp) * tb_y + (2 * k_y * so * pow(sp, 3) - 2 * k_z * so * cp * sp*sp - k_z * so * cp) * s_y*s_y + ( - 4 * k_z * so * pow(sp, 3) - 4 * k_y * so * cp * sp*sp + 3 * k_z * so * sp + k_y * so * cp) * s_y - 2 * k_y * so * pow(sp, 3) + 2 * k_z * so * cp * sp*sp + 3 * k_y * so * sp - 2 * k_z * so * cp) * ct*ct + (co * s_x * s_y * tb_z - co * s_x * tb_y + k_z * co * s_x * s_y - k_y * co * s_x) * ct) / (s_x*s_x * st*st + (2 * cp * s_x - 2 * sp * s_x * s_y) * ct * st + (sp*sp * s_y*s_y - 2 * cp * sp * s_y - sp*sp + 1) * ct*ct);
	double drydphi = ((((4 * k_z * so * sp*sp + 4 * k_y * so * cp * sp - 2 * k_z * so) * s_x * s_y + (4 * k_y * so * sp*sp - 4 * k_z * so * cp * sp - 2 * k_y * so) * s_x) * ct + co * cp * s_x*s_x * tb_z - co * sp * s_x*s_x * tb_y + (co * sp * s_x * s_y - co * cp * s_x) * tb_x - k_x * co * sp * s_x * s_y + (k_z * co * cp - k_y * co * sp) * s_x*s_x + k_x * co * cp * s_x) * st*st + ((( - 2 * k_z * so * pow(sp, 3) - 2 * k_y * so * cp * sp*sp) * s_y*s_y + ( - 4 * k_y * so * pow(sp, 3) + 4 * k_z * so * cp * sp*sp + 4 * k_y * so * sp) * s_y + (2 * k_z * so * sp + 2 * k_y * so * cp) * s_x*s_x + 2 * k_z * so * pow(sp, 3) + 2 * k_y * so * cp * sp*sp - 2 * k_z * so * sp - 2 * k_y * so * cp) * ct*ct + (co * s_x * tb_z + co * s_x * s_y * tb_y + ( - co * s_y*s_y - co) * tb_x + k_x * co * s_y*s_y + (4 * k_y * co * sp*sp - 4 * k_z * co * cp * sp - k_y * co) * s_x * s_y + ( - 4 * k_z * co * sp*sp - 4 * k_y * co * cp * sp + 3 * k_z * co) * s_x + k_x * co) * ct - so * sp * s_x*s_x * tb_z - so * cp * s_x*s_x * tb_y + (so * cp * s_x * s_y + so * sp * s_x) * tb_x - k_x * so * cp * s_x * s_y + ( - k_z * so * sp - k_y * so * cp) * s_x*s_x - k_x * so * sp * s_x) * st + (2 * k_y * so * s_x - 2 * k_z * so * s_x * s_y) * pow(ct, 3) + (( - co * cp * s_y*s_y - co * sp * s_y) * tb_z + (co * cp * s_y + co * sp) * tb_y + ( - 2 * k_y * co * pow(sp, 3) + 2 * k_z * co * cp * sp*sp + k_z * co * cp) * s_y*s_y + (4 * k_z * co * pow(sp, 3) + 4 * k_y * co * cp * sp*sp - 3 * k_z * co * sp - k_y * co * cp) * s_y + 2 * k_y * co * pow(sp, 3) - 2 * k_z * co * cp * sp*sp - 3 * k_y * co * sp + 2 * k_z * co * cp) * ct*ct + (so * s_x * s_y * tb_z - so * s_x * tb_y + k_z * so * s_x * s_y - k_y * so * s_x) * ct) / (s_x*s_x * st*st + (2 * cp * s_x - 2 * sp * s_x * s_y) * ct * st + (sp*sp * s_y*s_y - 2 * cp * sp * s_y - sp*sp + 1) * ct*ct);
	double drxdtheta = (((2 * k_x * co * sp*sp * s_y*s_y + ((4 * k_y * co * sp*sp - 4 * k_z * co * cp * sp) * s_x - 4 * k_x * co * cp * sp) * s_y - 2 * k_x * co * s_x*s_x + ( - 4 * k_z * co * sp*sp - 4 * k_y * co * cp * sp + 4 * k_z * co) * s_x - 2 * k_x * co * sp*sp + 2 * k_x * co) * ct*ct + ((co - co * sp*sp) * s_x - co * cp * sp * s_x * s_y) * tb_z + (co * sp*sp * s_x * s_y - co * cp * sp * s_x) * tb_y + ( - co * sp*sp * s_y*s_y + 2 * co * cp * sp * s_y + co * sp*sp - co) * tb_x + k_x * co * sp*sp * s_y*s_y + ((k_z * co * cp * sp - k_y * co * sp*sp) * s_x - 2 * k_x * co * cp * sp) * s_y + 2 * k_x * co * s_x*s_x + (k_z * co * sp*sp + k_y * co * cp * sp - k_z * co) * s_x - k_x * co * sp*sp + k_x * co) * st + ((2 * k_z * co * cp * sp*sp - 2 * k_y * co * pow(sp, 3)) * s_y*s_y + (4 * k_x * co * sp * s_x + 4 * k_z * co * pow(sp, 3) + 4 * k_y * co * cp * sp*sp - 4 * k_z * co * sp) * s_y + (2 * k_y * co * sp - 2 * k_z * co * cp) * s_x*s_x - 4 * k_x * co * cp * s_x + 2 * k_y * co * pow(sp, 3) - 2 * k_z * co * cp * sp*sp - 2 * k_y * co * sp + 2 * k_z * co * cp) * pow(ct, 3) + ( - co * cp * s_x*s_x * tb_z + co * sp * s_x*s_x * tb_y + (co * cp * s_x - co * sp * s_x * s_y) * tb_x - 3 * k_x * co * sp * s_x * s_y + (3 * k_z * co * cp - 3 * k_y * co * sp) * s_x*s_x + 3 * k_x * co * cp * s_x) * ct + ((so * sp*sp - so) * s_x * s_y - so * cp * sp * s_x) * tb_z + (so * cp * sp * s_x * s_y + so * sp*sp * s_x) * tb_y + ( - so * cp * sp * s_y*s_y + (so - 2 * so * sp*sp) * s_y + so * cp * sp) * tb_x + k_x * so * cp * sp * s_y*s_y + (( - k_z * so * sp*sp - k_y * so * cp * sp + k_z * so) * s_x + 2 * k_x * so * sp*sp - k_x * so) * s_y + (k_z * so * cp * sp - k_y * so * sp*sp) * s_x - k_x * so * cp * sp) / ((2 * sp * s_x * s_y - 2 * cp * s_x) * ct * st + ( - sp*sp * s_y*s_y + 2 * cp * sp * s_y + s_x*s_x + sp*sp - 1) * ct*ct - s_x*s_x);
	double drydtheta =  - (((2 * k_x * so * sp*sp * s_y*s_y + ((4 * k_y * so * sp*sp - 4 * k_z * so * cp * sp) * s_x - 4 * k_x * so * cp * sp) * s_y - 2 * k_x * so * s_x*s_x + ( - 4 * k_z * so * sp*sp - 4 * k_y * so * cp * sp + 4 * k_z * so) * s_x - 2 * k_x * so * sp*sp + 2 * k_x * so) * ct*ct + ((so - so * sp*sp) * s_x - so * cp * sp * s_x * s_y) * tb_z + (so * sp*sp * s_x * s_y - so * cp * sp * s_x) * tb_y + ( - so * sp*sp * s_y*s_y + 2 * so * cp * sp * s_y + so * sp*sp - so) * tb_x + k_x * so * sp*sp * s_y*s_y + ((k_z * so * cp * sp - k_y * so * sp*sp) * s_x - 2 * k_x * so * cp * sp) * s_y + 2 * k_x * so * s_x*s_x + (k_z * so * sp*sp + k_y * so * cp * sp - k_z * so) * s_x - k_x * so * sp*sp + k_x * so) * st + ((2 * k_z * so * cp * sp*sp - 2 * k_y * so * pow(sp, 3)) * s_y*s_y + (4 * k_x * so * sp * s_x + 4 * k_z * so * pow(sp, 3) + 4 * k_y * so * cp * sp*sp - 4 * k_z * so * sp) * s_y + (2 * k_y * so * sp - 2 * k_z * so * cp) * s_x*s_x - 4 * k_x * so * cp * s_x + 2 * k_y * so * pow(sp, 3) - 2 * k_z * so * cp * sp*sp - 2 * k_y * so * sp + 2 * k_z * so * cp) * pow(ct, 3) + ( - so * cp * s_x*s_x * tb_z + so * sp * s_x*s_x * tb_y + (so * cp * s_x - so * sp * s_x * s_y) * tb_x - 3 * k_x * so * sp * s_x * s_y + (3 * k_z * so * cp - 3 * k_y * so * sp) * s_x*s_x + 3 * k_x * so * cp * s_x) * ct + ((co - co * sp*sp) * s_x * s_y + co * cp * sp * s_x) * tb_z + ( - co * cp * sp * s_x * s_y - co * sp*sp * s_x) * tb_y + (co * cp * sp * s_y*s_y + (2 * co * sp*sp - co) * s_y - co * cp * sp) * tb_x - k_x * co * cp * sp * s_y*s_y + ((k_z * co * sp*sp + k_y * co * cp * sp - k_z * co) * s_x - 2 * k_x * co * sp*sp + k_x * co) * s_y + (k_y * co * sp*sp - k_z * co * cp * sp) * s_x + k_x * co * cp * sp) / ((2 * sp * s_x * s_y - 2 * cp * s_x) * ct * st + ( - sp*sp * s_y*s_y + 2 * cp * sp * s_y + s_x*s_x + sp*sp - 1) * ct*ct - s_x*s_x);
	double drxdomega = ((((2 * k_y * so * sp*sp - 2 * k_z * so * cp * sp) * s_y - 2 * k_x * so * s_x - 2 * k_z * so * sp*sp - 2 * k_y * so * cp * sp + 2 * k_z * so) * ct + co * sp * s_x * tb_z + co * cp * s_x * tb_y + ( - co * cp * s_y - co * sp) * tb_x + k_x * co * cp * s_y + (k_z * co * sp + k_y * co * cp) * s_x + k_x * co * sp) * st + (2 * k_x * so * sp * s_y + (2 * k_y * so * sp - 2 * k_z * so * cp) * s_x - 2 * k_x * so * cp) * ct*ct + ( - co * s_y * tb_z + co * tb_y + ( - 2 * k_z * co * sp*sp - 2 * k_y * co * cp * sp + k_z * co) * s_y - 2 * k_y * co * sp*sp + 2 * k_z * co * cp * sp + k_y * co) * ct + so * cp * s_x * tb_z - so * sp * s_x * tb_y + (so * sp * s_y - so * cp) * tb_x - k_x * so * sp * s_y + (k_z * so * cp - k_y * so * sp) * s_x + k_x * so * cp) / (s_x * st + (cp - sp * s_y) * ct);
	double drydomega = ((((2 * k_y * co * sp*sp - 2 * k_z * co * cp * sp) * s_y - 2 * k_x * co * s_x - 2 * k_z * co * sp*sp - 2 * k_y * co * cp * sp + 2 * k_z * co) * ct - so * sp * s_x * tb_z - so * cp * s_x * tb_y + (so * cp * s_y + so * sp) * tb_x - k_x * so * cp * s_y + ( - k_z * so * sp - k_y * so * cp) * s_x - k_x * so * sp) * st + (2 * k_x * co * sp * s_y + (2 * k_y * co * sp - 2 * k_z * co * cp) * s_x - 2 * k_x * co * cp) * ct*ct + (so * s_y * tb_z - so * tb_y + (2 * k_z * so * sp*sp + 2 * k_y * so * cp * sp - k_z * so) * s_y + 2 * k_y * so * sp*sp - 2 * k_z * so * cp * sp - k_y * so) * ct + co * cp * s_x * tb_z - co * sp * s_x * tb_y + (co * sp * s_y - co * cp) * tb_x - k_x * co * sp * s_y + (k_z * co * cp - k_y * co * sp) * s_x + k_x * co * cp) / (s_x * st + (cp - sp * s_y) * ct);
	Eigen::Matrix<double, 2, 6> der;
	der(0,0) = drxdx;
	der(1,0) = drydx;

	der(0,1) = drxdy;
	der(1,1) = drydy;

	der(0,2) = drxdz;
	der(1,2) = drydz;

	der(0,3) = drxdphi;
	der(1,3) = drydphi;

	der(0,4) = drxdtheta;
	der(1,4) = drydtheta;

	der(0,5) = drxdomega;
	der(1,5) = drydomega;
	return der;
}

void GblAlign::fitTracks(std::vector<std::pair<core::TripletTrack, Eigen::Vector3d>> trackCandidates)
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
	size_t numTracks = 0;
	size_t maxTracks = _config.get<size_t>("gbl_max_tracks");
	for(const auto& pair: trackCandidates) {
		auto& track = pair.first;
		auto& dutHit = pair.second;
		std::vector<gbl::GblPoint> trajectory;
		double prev_z = track.upstream()[0](0);
		for(const auto& hit: track.upstream().getHits()) {
			gbl::GblPoint p(jacobianStep(hit(2) - prev_z));
			prev_z = hit(2);
			p.addMeasurement(proj, track.upstream().getds(hit), _precisionTel);
			p.addScatterer(scatter, wscatter);
			trajectory.push_back(p);
			fout_tracks << hit(0) << " " << hit(1) << " " << hit(2) << "\n";
		} { /* DUT */
			Eigen::Vector3d hit = dutHit + _dutPreAlign;
			fout_tracks << hit(0) << " " << hit(1) << " " << hit(2) << "\n";
			gbl::GblPoint p(jacobianStep(hit(2) - prev_z));
			prev_z = hit(2);
			Eigen::Vector2d precision(_precisionMpa);
			if(true) {
				precision(0) = _precisionMpa(1);
				precision(1) = _precisionMpa(0);
			}
			p.addMeasurement(proj, track.upstream().getds(hit), precision);
			p.addScatterer(scatter, wscatter);
			//auto der = getDerivatives(track.upstream(), 0, {0, 0, 0});
			//std::cout << "Der:\n" << der << std::endl;
			std::vector<int> labels(6);
			labels[0] = 11;
			labels[1] = 12;
			labels[2] = 13;
			//labels[3] = 14;
			//labels[4] = 15;
			//labels[5] = 16;
			Eigen::Matrix<double, 2, 3> der;
			der << 1.0, 0.0, track.upstream().slope()(0),
			       0.0, 1.0, track.upstream().slope()(1);
			p.addGlobals(labels, der);
			trajectory.push_back(p);
		}
		/* DOWNSTREAM */
		for(const auto& hit: track.downstream().getHits()) {
			gbl::GblPoint p(jacobianStep(hit(2) - prev_z));
			prev_z = hit(2);
			p.addMeasurement(proj, track.downstream().getds(hit), _precisionTel);
			p.addScatterer(scatter, wscatter);
			trajectory.push_back(p);
			fout_tracks << hit(0) << " " << hit(1) << " " << hit(2) << "\n";
		} { /* REF */
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
		++numTracks;
		if(numTracks >= maxTracks) {
			break;
		}
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
	       << "11  0.0  0.0\n" // dx
	       << "12  0.0  0.0\n" // dy
	       << "13  0.0  0.0\n" // dz
//	       << "14  0.0  0.0\n" // dphi
//	       << "15  0.0  0.0\n" // dtheta
//	       << "16  0.0  0.0\n" // domega
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
	std::ofstream fprealign(getFilename("_prealign.txt"));
	Eigen::Vector3d dut = _dutPreAlign + _trackConsts.dut_offset;
	fprealign << "1 " << _refPreAlign(0) << "\n"
	          << "2 " << _refPreAlign(1) << "\n"
	          << "3 " << _refPreAlign(2) << "\n"
	          << "11 " << dut(0) << "\n"
	          << "12 " << dut(1) << "\n"
	          << "13 " << dut(2) << "\n"
		  << std::flush;
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

