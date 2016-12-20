
#ifndef _FITTING_H_
#define _FITTING_H_

#include <cmath>
#include <cstddef>
#include <vector>
#include <exception>
#include <Math/Minimizer.h>
#include <Math/Functor.h>
#include <Eigen/Dense>

class TH1;
class TGraph;

namespace core
{

class HistogramFit
{
public:
	typedef double (*function_t)(double*, double*);
	HistogramFit(TH1* hist, function_t func, size_t nparams);
	HistogramFit(const HistogramFit& oth) = delete;
	HistogramFit(HistogramFit&& oth) = delete;
	~HistogramFit();

	size_t getNumDimensions() const;

	void fit();

	void setLimitX(const double& min=-INFINITY, const double& max=INFINITY);
	void setLimitY(const double& min=-INFINITY, const double& max=INFINITY);
	void setLimitZ(const double& min=-INFINITY, const double& max=INFINITY);

	class dimenion_mismatch : public std::exception { };

	ROOT::Math::Minimizer* minimizer() { return _min; }
	const ROOT::Math::Minimizer* minimizer() const { return _min; }

	TGraph* createFittedFunction() const;
	void addFittedFunction();

	std::vector<double> getResult() const;

private:
	double Chi2(const double* par);
	TH1* _hist;
	function_t _function;
	ROOT::Math::Minimizer* _min;
	size_t _nparams;
	std::vector<Eigen::Vector3d> _histX;
	std::vector<double> _histY;
	std::vector<double> _histErr;
	std::vector<double> _fitMin;
	std::vector<double> _fitMax;
	ROOT::Math::Functor _chi2fctor;
};

}//namespace core

#endif//_FITTING_H_
