
#include "functions.h"
#include <TMath.h>

namespace core
{

double general_plateau_functions(double* xx, double* par)
{
	auto x = xx[0];
	auto& x_0 = par[0];
	auto& x_1 = par[1];
	auto& y_0 = par[2];
	auto& y_1 = par[2];
	auto& sigma_0 = par[4];
	auto& sigma_1 = par[5];
	auto& c_0 = par[6];
	auto& c_1 = par[7];
	double y0 = y_0 + c_0 * (TMath::Exp(-TMath::Power((x - x_0)/sigma_0, 2)/2.) - 1.0);
	double y1 = y_0 + (y_1 - y_0) * x / (x_1 - x_0);
	double y2 = y_1 + c_1 * (TMath::Exp(-TMath::Power((x - x_1)/sigma_1, 2)/2.) - 1.0);
	if(x < x_0) return y0;
	if(x > x_1) return y2;
	return y1;
}

double symmetric_plateau_function(double* xx, double* par)
{
	auto x = xx[0];
	auto& x_0 = par[0];
	auto& x_1 = par[1];
	auto& y_0 = par[2];
	auto& y_1 = par[2];
	auto& sigma_0 = par[3];
	auto& sigma_1 = par[3];
	auto& c_0 = par[4];
	auto& c_1 = par[4];
	double y0 = y_0 + c_0 * (TMath::Exp(-TMath::Power((x - x_0)/sigma_0, 2.)/2.) - 1.0);
	double y1 = y_0 + (y_1 - y_0) * x / (x_1 - x_0);
	double y2 = y_1 + c_1 * (TMath::Exp(-TMath::Power((x - x_1)/sigma_1, 2.)/2.) - 1.0);
	if(x < x_0) return y0;
	if(x > x_1) return y2;
	return y1;
}

double symmetric_plateau_function2(double* xx, double* par)
{
	auto x = xx[0];
	auto x_0 = par[0]-par[1]/2;
	auto x_1 = par[0]+par[1]/2;
	auto& y_0 = par[2];
	auto& y_1 = par[2];
	auto& sigma_0 = par[3];
	auto& sigma_1 = par[3];
	auto& c_0 = par[4];
	auto& c_1 = par[4];
	double y0 = y_0 + c_0 * (TMath::Exp(-TMath::Power((x - x_0)/sigma_0, 2.)/2.) - 1.0);
	double y1 = y_0 + (y_1 - y_0) * x / (x_1 - x_0);
	double y2 = y_1 + c_1 * (TMath::Exp(-TMath::Power((x - x_1)/sigma_1, 2.)/2.) - 1.0);
	if(x < x_0) return y0;
	if(x > x_1) return y2;
	return y1;
}

double gauss1d(double* xx, double* par)
{
	return par[0]/TMath::Sqrt(2.0*M_PI * par[1]*par[1]) * TMath::Exp(-TMath::Power(xx[0] - par[2], 2) / (par[1]*par[1]));
}

double gauss1d_offset(double* xx, double* par)
{
	return par[0]/TMath::Sqrt(2.0*M_PI * par[1]*par[1]) * TMath::Exp(-TMath::Power(xx[0] - par[2], 2) / (par[1]*par[1])) + par[3];
}

}// namespace core
