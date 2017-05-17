#include "histogramfit.h"
#include <iostream>
#include <cassert>
#include <TMath.h>
#include <TH1.h>
#include <Math/Factory.h>
#include <TGraph.h>
#include <TList.h>

using namespace core;

HistogramFit::HistogramFit(TH1* hist, function_t func, size_t nparams) :
 _hist(hist), _function(func), _min(ROOT::Math::Factory::CreateMinimizer("Minuit", "Migrad")), _nparams(nparams),
 _chi2fctor(this, &HistogramFit::Chi2, _nparams)
{
	assert(_hist != nullptr);
	assert(_function != nullptr);
	assert(_min != nullptr);
	assert(_nparams > 0);
	_min->SetFunction(_chi2fctor);
	_min->SetMaxFunctionCalls(1000000);
	_min->SetMaxIterations(1000);
	_min->SetTolerance(0.001);
	_fitMin.resize(3);
	_fitMax.resize(3);
	setLimitX();
	setLimitY();
	setLimitZ();
	// _min->SetPrintLevel(1);

}

HistogramFit::~HistogramFit()
{
	delete _min;
}

size_t HistogramFit::getNumDimensions() const
{
	return _hist->GetDimension();
}

void HistogramFit::fit()
{
	size_t nbins = _hist->GetNbinsX() * _hist->GetNbinsY() * _hist->GetNbinsZ();
	_histX.clear();
	_histY.clear();
	_histErr.clear();
	_histX.reserve(nbins);
	_histY.reserve(nbins);
	_histErr.reserve(nbins);
	/* std::cout << "Limits: ";
	for(size_t i=0; i<3; ++i) {
		std::cout << "[ " << _fitMin[i] << " | " << _fitMax[i] << " ]  ";
	}*/
	std::cout << std::endl;
	for(size_t bin = 1; bin < nbins+1; ++bin) {
		int nx, ny, nz;
		_hist->GetBinXYZ(bin, nx, ny, nz);
		auto ndim = getNumDimensions();
		double px = _hist->GetXaxis()->GetBinCenter(nx);
		double py = (ndim<2)? 0.0 :_hist->GetYaxis()->GetBinCenter(ny);
		double pz = (ndim<3)? 0.0 : _hist->GetZaxis()->GetBinCenter(nz);
		// std::cout << bin << " => " << px << " " << py << " " << pz << " | " << nx << " " << ny << " " << nz << std::endl;
		if(_fitMin[0] > px || px > _fitMax[0] ||
		   _fitMin[1] > py || py > _fitMax[1] ||
		   _fitMin[2] > pz || pz > _fitMax[2]) {
			// std::cout << " DISCARD!" << std::endl;
			continue;
		}
		double error = _hist->GetBinError(bin);
		if(error > 0.0) {
			_histErr.push_back(_hist->GetBinError(bin));
			_histX.push_back({px, py, pz});
			_histY.push_back(_hist->GetBinContent(bin));
		}
	}
	_min->Minimize();
}

void HistogramFit::setLimitX(const double& min, const double& max)
{
	if(std::isinf(min) || std::isnan(min)) {
		_fitMin[0] = _hist->GetXaxis()->GetBinUpEdge(0);
	} else {
		_fitMin[0] = min;
	}
	if(std::isinf(max) || std::isnan(max)) {
		_fitMax[0] = _hist->GetXaxis()->GetBinLowEdge(_hist->GetNbinsX()+1);
	} else {
		_fitMax[0] = max;
	}
}

void HistogramFit::setLimitY(const double& min, const double& max)
{
	if(std::isinf(min) || std::isnan(min)) {
		_fitMin[1] = _hist->GetYaxis()->GetBinLowEdge(0);
	} else {
		_fitMin[1] = min;
	}
	if(std::isinf(max) || std::isnan(max)) {
		_fitMax[1] = _hist->GetYaxis()->GetBinUpEdge(_hist->GetNbinsY()+1);
	} else {
		_fitMax[1] = max;
	}
}

void HistogramFit::setLimitZ(const double& min, const double& max)
{
	if(std::isinf(min) || std::isnan(min)) {
		_fitMin[2] = _hist->GetZaxis()->GetBinLowEdge(0.0);
	} else {
		_fitMin[2] = min;
	}
	if(std::isinf(max) || std::isnan(max)) {
		_fitMax[2] = _hist->GetZaxis()->GetBinUpEdge(_hist->GetNbinsZ()-1);
	} else {
		_fitMax[2] = max;
	}
}

double HistogramFit::Chi2(const double* par)
{
	assert(_histX.size() == _histY.size());
	assert(_histX.size() == _histErr.size());
	double xx[3];
	double chi2 = 0.0;
	for(size_t i=0; i < _histX.size(); ++i) {
		// fuck ROOT!
		double yt = _function(_histX[i].data(), const_cast<double*>(par));
		chi2 += TMath::Power(yt - _histY[i], 2) / (_histErr[i]*_histErr[i]);
	}
	// std::cout << "chi2 " << " -> " << chi2 << std::endl;
	return chi2;
}

TGraph* HistogramFit::createFittedFunction() const
{
	auto graph = new TGraph(_histX.size());
	auto param = getResult();	
	for(size_t i = 0; i < _histX.size(); ++i) {
		auto& x = _histX[i];
		double y = _function(const_cast<double*>(x.data()), &param.front());
		graph->SetPoint(i, x(0), y);
	}
	return graph;
}

void HistogramFit::addFittedFunction()
{
	auto graph = createFittedFunction();
	graph->SetLineColorAlpha(kRed, 0.95);
	graph->SetLineWidth(2);
	_hist->GetListOfFunctions()->Add(graph);
}

std::vector<double> HistogramFit::getResult() const
{
	std::vector<double> param;
	param.assign(_min->X(), _min->X()+_nparams);
	return param;
}
