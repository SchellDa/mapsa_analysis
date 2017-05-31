
#include "refprealign.h"
#include <iostream>
#include <TImage.h>
#include <TCanvas.h>
#include <TFitResult.h>
#include <fstream>
#include "mpatransform.h"
#include <TF1.h>

REGISTER_ANALYSIS_TYPE(RefPreAlign, "Prealign the reference plane")

RefPreAlign::RefPreAlign() :
 core::MergedAnalysis(), _file(nullptr),
 _refResX(nullptr), _refResY(nullptr),
 _dutResX(nullptr), _dutResY(nullptr)
{
}

RefPreAlign::~RefPreAlign()
{
	_file->Write();
	_file->Close();
	delete _file;
	_file = nullptr;
}

void RefPreAlign::init()
{
	std::cout << "Init ref pre align: " << getRootFilename() << std::endl;
	_file = new TFile(getRootFilename().c_str(), "recreate");
	_refResX = new TH1F("ref_res_x", "Ref Residual X", 500, -10, 10);
	_refResY = new TH1F("ref_res_y", "Ref Residual Y", 500, -10, 10);
	_dutResX = new TH1F("dut_res_x", "DUT Residual X", 200, -10, 10);
	_dutResY = new TH1F("dut_res_y", "DUT Residual Y", 200, -10, 10);
}

void RefPreAlign::run(const core::MergedAnalysis::run_data_t& run)
{
	std::cout << "Run " << run.runId << std::endl;
	auto telHits = *run.telescopeHits;
	for(size_t evt = 0; evt < run.tree->GetEntries(); ++evt) {
		run.tree->GetEvent(evt);
		core::MpaTransform transform;
		for(size_t it = 0; it < telHits->p1.x.GetNoElements(); ++it) {
			for(size_t ir = 0; ir < telHits->ref.x.GetNoElements(); ++ir) {
				_refResX->Fill(telHits->ref.x[ir] - telHits->p1.x[it]);
				_refResY->Fill(telHits->ref.y[ir] - telHits->p1.y[it]);
			}
			for(size_t pixel = 0; pixel < 48; ++pixel) {
				if((*run.mpaData[1].data)->counter.pixels[pixel] == 0) {
					continue;
				}
				auto index = transform.translatePixelIndex(pixel);
				// if(index(0) == 0 || index(0) == 15) {
				// 	continue;
				// }
				// if(index(1) == 2) {
				// 	continue;
				// }
				auto hit = transform.transform(pixel);
				_dutResX->Fill(telHits->p1.y[it] + hit(0));
				_dutResY->Fill(telHits->p1.x[it] - hit(1));
			}
		}
	}
}

void RefPreAlign::finalize()
{
	if(!_file) {
		std::cout << "File not opened, no output will be generated" << std::endl;
		return;
	}
	auto func = new TF1("gaus_base", "[0]*exp(-(x-[1])**2 / [2]**2) + [3]");
	std::cout << "Finalize" << std::endl;
	double x_max = _refResX->GetXaxis()->GetBinCenter(_refResX->GetMaximumBin());
	double y_max = _refResY->GetXaxis()->GetBinCenter(_refResY->GetMaximumBin());
	func->SetParameter(0, 100);
	func->SetParameter(1, x_max);
	func->SetParameter(2, 1);
	func->SetParameter(3, 0);
	auto resultX = _refResX->Fit("gaus_base", "FSMR", "", x_max - _refResX->GetRMS()/2,
	                                              x_max + _refResX->GetRMS()/2);
	func->SetParameter(0, 100);
	func->SetParameter(1, y_max);
	func->SetParameter(2, 1);
	func->SetParameter(3, 0);
	auto resultY = _refResY->Fit("gaus_base", "FSMR", "", y_max - _refResY->GetRMS()/2,
	                                              y_max + _refResY->GetRMS()/2);
	if(!resultX || !resultY) {
		std::cerr << "Fitting failed!" << std::endl;
		return;
	}
	std::ofstream fout(getFilename("_ref.csv"));
	fout << resultX->Parameter(1) << " " << resultY->Parameter(1) << std::endl;
	std::cout << "Ref pre-alignment: " << resultX->Parameter(1) << " " << resultY->Parameter(1) << std::endl;
	x_max = _dutResX->GetXaxis()->GetBinCenter(_dutResX->GetMaximumBin());
	y_max = _dutResY->GetXaxis()->GetBinCenter(_dutResY->GetMaximumBin());
	func->SetParameter(0, 1);
	func->SetParameter(1, x_max);
	func->SetParameter(2, 0.1);
	func->SetParameter(3, 1);
	resultX = _dutResX->Fit("gaus_base", "FSMR", "", x_max - _dutResX->GetRMS()/2,
	                                              x_max + _dutResX->GetRMS()/2);
	func->SetParameter(0, 1);
	func->SetParameter(1, y_max);
	func->SetParameter(2, 0.1);
	func->SetParameter(3, 1);
	resultY = _dutResY->Fit("gaus_base", "FSMR", "", y_max - _dutResY->GetRMS()/2,
	                                              y_max + _dutResY->GetRMS()/2);
	std::cout << "DUT pre-alignment: " << resultX->Parameter(1) << " " << resultY->Parameter(1) << std::endl;
	fout.close();
	fout.open(getFilename("_dut.csv"));
	fout << resultX->Parameter(1) << " " << resultY->Parameter(1) << std::endl;
	fout.close();

	auto canvas = new TCanvas("canvas", "", 800, 600);
	canvas->Divide(2, 2);
	canvas->cd(1);
	_refResX->Draw();
	canvas->cd(2);
	_refResY->Draw();
	canvas->cd(3);
	_dutResX->Draw();
	canvas->cd(4);
	_dutResY->Draw();
	auto img = TImage::Create();
	img->FromPad(canvas);
	img->WriteImage(getFilename(".png").c_str());
	delete img;
}

