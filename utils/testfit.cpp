
#include <iostream>
#include <TH1D.h>
#include <TFile.h>
#include <TCanvas.h>
#include <TImage.h>
#include <TMath.h>
#include <TRandom2.h>
#include "histogramfit.h"

double gauss1d(double* xx, double* par)
{
	return par[0]/TMath::Sqrt(2.0*M_PI * par[1]*par[1]) * TMath::Exp(-TMath::Power(xx[0] - par[2], 2) / (par[1]*par[1]));
}

int main(int argc, char* argv[])
{
	auto file = new TFile("test.root", "RECREATE");
	auto hist = new TH1D("test", "test", 300, 0, 10);
	TRandom2 r(0);
	for(size_t i=0; i<10000; ++i) {
		hist->Fill(r.Gaus(2, 0.8));
	}
	core::HistogramFit fit(hist, gauss1d, 3);
	fit.minimizer()->SetVariable(0, "const", 0.0, 0.1);
	fit.minimizer()->SetVariable(1, "sigma", 1.0, 0.1);
	fit.minimizer()->SetVariable(2, "mean", 0.0, 0.1);
	fit.fit();
	fit.addFittedFunction();
	file->Write();
	file->Close();
	return 0;
}
