
#include "aligner.h"
#include "functions.h"
#include "histogramfit.h"

#include <cassert>
#include <fstream>
#include <cmath>
#include <TH1D.h>
#include <TCanvas.h>
#include <TImage.h>
#include <TF1.h>
#include <TFitResult.h>

using namespace core;


Aligner::Aligner() :
 xHistogramConfig{-5, 5, 2000} , yHistogramConfig{-5, 5, 250},
 _nsigma(1.0), _alignX(nullptr), _alignY(nullptr),
 _calculated(false), _offset(0, 0, 0), _cuts(0, 0)
{
}

TH1D* Aligner::getHistX() const
{
	return _alignX;
}
TH1D* Aligner::getHistY() const
{
	return _alignY;
}

void Aligner::initHistograms(const std::string& xname, const std::string& yname)
{
	_calculated = false;
	_alignX = new TH1D(xname.c_str(), "Alignment Correlation on X axis",
	                   xHistogramConfig.nbins, xHistogramConfig.min, xHistogramConfig.max);
	_alignY = new TH1D(yname.c_str(), "Alignment Correlation on Y axis",
	                   yHistogramConfig.nbins, yHistogramConfig.min, yHistogramConfig.max);
}

void Aligner::writeHistograms()
{
	if(_alignX && _alignY) {
		_alignX->Write();
		_alignY->Write();
	}
}

void Aligner::writeHistogramImage(const std::string& filename)
{
	if(!_alignX || !_alignY)
		return;
	std::ostringstream info;
	auto canvas = new TCanvas("alignmentCanvas", "", 400, 600);
	canvas->Divide(1, 2);
	canvas->cd(1);
	_alignX->Draw();

	canvas->cd(2);
	_alignY->Draw();
	auto img = TImage::Create();
	img->FromPad(canvas);
	img->WriteImage(filename.c_str());
	delete img;
}

void Aligner::calculateAlignment(const bool& quiet)
{
	if(_calculated)
		return;
	assert(_alignX);
	assert(_alignY);
	_calculated = true;
	auto xalign = alignGaussian(_alignX, 0.5, 0.1, quiet);
	auto yalign = alignPlateau(_alignY, 1, 0.05, quiet);	
	_offset = { xalign(0), yalign(0), 0.0 };
	_cuts = { xalign(1), yalign(1) };
	if(_fixedMean) {
		/*if(xalign(0) < xalign(1)) {
			auto xalignfixed = alignGaussian(_alignX, 0.5, 0.1, quiet);
			_cuts(0) = std::max(std::abs(xalign(0)), xalignfixed(1));
		} else {
			_cuts(0) = std::abs(xalign(0));
		}
		if(yalign(0) < yalign(1)) {
			auto yalignfixed = alignPlateau(_alignY, 1, 0.05, quiet);
			_cuts(1) = std::max(std::abs(yalign(0)), yalignfixed(1));
		} else {
			_cuts(1) = std::abs(yalign(0));
		}*/
		_cuts(0) = std::abs(xalign(0)) + std::abs(xalign(1));
		_cuts(1) = std::abs(yalign(0)) + std::abs(yalign(1));
	}
}

void Aligner::Fill(const double& xdiff, const double& ydiff)
{
	assert(_alignX);
	assert(_alignY);
	_alignX->Fill(xdiff);
	_alignY->Fill(ydiff);
}

void Aligner::clear()
{
	_calculated = false;
	_alignX->Reset();
	_alignY->Reset();
}

Eigen::Vector3d Aligner::getOffset() const
{
	assert(_calculated);
	return _offset;
}

Eigen::Vector2d Aligner::getCuts() const
{
	assert(_calculated);
	return _cuts;
}

bool Aligner::pointsCorrelated(const Eigen::Vector2d& a, const Eigen::Vector2d& b) const
{
	assert(_calculated);
	Eigen::Array2d diff{(a - b).array().abs()};
	return diff(0) < _cuts(0)*_nsigma &&
	       diff(1) < _cuts(1)*_nsigma;
}

bool Aligner::pointsCorrelatedX(const double& a, const double& b) const
{
	assert(_calculated);
	return std::abs(a-b) < _cuts(0)*_nsigma;
}

bool Aligner::pointsCorrelatedY(const double& a, const double& b) const
{
	assert(_calculated);
	return std::abs(a-b) < _cuts(1)*_nsigma;
}

void Aligner::saveAlignmentData(const std::string& filename, const std::string& extra) const
{
	std::ofstream of(filename);
	of << _offset(0) << " "
	   << _offset(1) << " "
	   << _offset(2) << " "
	   << _cuts(0) << " "
	   << _cuts(1);
	if(extra.size()) {
		of << " " << extra;
	}
	of << std::endl;
	of.close();
}

void Aligner::appendAlignmentData(const std::string& filename, const std::string& extra) const
{
	std::ofstream of(filename, std::ios_base::app);
	of << _offset(0) << " "
	   << _offset(1) << " "
	   << _offset(2) << " "
	   << _cuts(0) << " "
	   << _cuts(1);
	if(extra.size()) {
		of << " " << extra;
	}
	of << std::endl;
	of.close();
}

bool Aligner::loadAlignmentData(const std::string& filename)
{
	std::cout << "Alignment data filename: " << filename << std::endl;
	std::ifstream fin(filename);
	if(fin.fail()) {
		return false;
	}
	double x, y, z, sigma, dist;
	fin >> x >> y >> z >> sigma >> dist;
	_offset = { x, y, z };
	_cuts = { sigma, dist };
	_calculated = true;
	return true;
}

bool Aligner::rebinIfNeccessary(TH1D* cor, const double& nrms, const double& binratio)
{
	auto maxBin = cor->GetMaximumBin();
	auto mean = cor->GetBinLowEdge(maxBin);
	auto rms = cor->GetRMS();
	if(cor->GetEntries() * binratio * 2 < cor->GetNbinsX()) {
		/*std::cout << "REBIN!\n"
		          << "Num entries: " << cor->GetEntries() << "\n"
		          << "Entry threshold: " << cor->GetEntries() * binratio * 2 << "\n"
			  << "Num X bins: " << cor->GetNbinsX() << "\n";*/
		cor->Rebin(cor->GetNbinsX() / (cor->GetEntries() * binratio));
		// std::cout << "New reduced bin number: " << cor->GetNbinsX() << std::endl;

//		mean += cor->GetMean();
//		mean /= 2;
		return true;
	}
	return false;
}

Eigen::Vector2d Aligner::alignPlateau(TH1D* cor, const double& nrms, const double& binratio, const bool& quiet, const bool& fixedMean)
{
	auto maxBin = cor->GetMaximumBin();
	auto mean = cor->GetBinLowEdge(maxBin);
	auto rms = cor->GetRMS();
	auto rebinned = rebinIfNeccessary(cor, nrms, binratio);
	if(fixedMean) {
		mean = 0.0;
	}
	auto piecewise = new TF1("piecewise", symmetric_plateau_function2, mean-rms*nrms, mean+rms*nrms, 5);
	if(fixedMean) {
		piecewise->FixParameter(0, 0.0);
	} else {
		piecewise->SetParameter(0, mean);
	}
	piecewise->SetParameter(1, rms);
	piecewise->SetParameter(2, cor->GetMaximum());
	piecewise->SetParameter(3, 0.1);
	piecewise->SetParameter(4, cor->GetMaximum());
	auto result = cor->Fit(piecewise, quiet?"RMSq+":"RMS+", "", mean-rms*nrms, mean+rms*nrms);
	if(result.Get() == nullptr) {
		return {0, 10000.0};
	}
	return {
		result->Parameter(0),
		result->Parameter(1) + result->Parameter(3)
	};
}

Eigen::Vector2d Aligner::alignGaussian(TH1D* cor, const double& nrms, const double& binratio, const bool& quiet, const bool& fixedMean)
{
	auto maxBin = cor->GetMaximumBin();
	auto mean = cor->GetBinLowEdge(maxBin);
	auto rms = cor->GetRMS();
	auto rebinned = rebinIfNeccessary(cor, nrms, binratio);
	if(fixedMean) {
		mean = 0.0;
	}
	auto gaus = new TF1("newgaus", "gaus", mean-rms*nrms, mean+rms*nrms);
	if(fixedMean) {
		gaus->FixParameter(1, 0);
	}
	auto result = cor->Fit(gaus, quiet?"RMSq+":"RMS+", "", mean-rms*nrms, mean+rms*nrms);
	if(result.Get() == nullptr) {
		return {0, 10000.0};
	}
	return {
		result->Parameter(1),
		result->Parameter(2)
	};
}
