
#include "efficiency_track.h"
#include <iostream>
#include <algorithm>
#include <TCanvas.h>
#include <TStyle.h>
#include <TImage.h>
#include <TText.h>
#include <TPaveText.h>

REGISTER_ANALYSIS_TYPE(EfficiencyTrack, "Textual analysis description here.")

EfficiencyTrack::EfficiencyTrack() :
 Analysis(), _aligner(), _file(nullptr)
{
/*	addProcess("prealign", CS_TRACK,
	 std::bind(&EfficiencyTrack::prealignRun, this, std::placeholders::_1, std::placeholders::_2),
	 std::bind(&EfficiencyTrack::prealignFinish, this)
	);*/
	addProcess("align", CS_TRACK,
	 std::bind(&EfficiencyTrack::align, this, std::placeholders::_1, std::placeholders::_2),
	 std::bind(&EfficiencyTrack::alignFinish, this)
	);
	/*addProcess("checkCorrelatedHits", CS_TRACK,
	 std::bind(&EfficiencyTrack::checkCorrelatedHits, this, std::placeholders::_1, std::placeholders::_2)
	);*/
	addProcess("analyze", CS_TRACK,
	 std::bind(&EfficiencyTrack::analyze, this, std::placeholders::_1, std::placeholders::_2),
	 std::bind(&EfficiencyTrack::analyzeFinish, this)
	);
	getOptionsDescription().add_options()
		("align,a", "Force recalculation of alignment parameters")
	;
}

EfficiencyTrack::~EfficiencyTrack()
{
	if(_file) {
		_file->Write();
		_file->Close();
		delete _file;
	}
}

void EfficiencyTrack::init(const po::variables_map& vm)
{
	_numPrealigmentPoints = 20000;
	_aligner.setNSigma(_config.get<double>("n_sigma_cut"));
	if(vm.count("align") < 1) {
		_aligner.loadAlignmentData(getFilename(".align"));
	}
	_file = new TFile(getRootFilename().c_str(), "RECREATE");
	if(!_aligner.gotAlignmentData()) {
		_aligner.initHistograms();
	}
	double sizeX = _mpaTransform.getSensitiveSize()(0);
	double sizeY = _mpaTransform.getSensitiveSize()(1);
	auto resolution = _config.get<unsigned int>("efficiency_histogram_resolution_factor");
	_correlatedHits = new TH2D("correlatedHits", "Coordinates of hits correlated on X axis",
	                           250, -_mpaTransform.getSensitiveSize()(0), sizeX,
	                           250, -_mpaTransform.getSensitiveSize()(1), _mpaTransform.getSensitiveSize()(1));
	_correlatedHitsX = new TH1D("correlatedHitsX", "X coordinates of hits correlated on X axis", 250, -5, 5);
	_correlatedHitsY = new TH1D("correlatedHitsY", "Y coordinates of hits correlated on X axis", 250, -5, 5);
	_efficiency = new TH2D("correlatedHits", "", // 16, 0, 16, 3, 0, 3);
	                           _mpaTransform.getNumPixels()(0) * resolution,
				   -sizeX / 2,
				   sizeX / 2,
	                           _mpaTransform.getNumPixels()(1) * resolution * sizeY/sizeX,
	                           -sizeY / 2,
				   sizeY / 2);
	_trackHits = new TH2D("trackHits", "",
	                           _mpaTransform.getNumPixels()(0) * resolution,
				   -sizeX / 2,
				   sizeX / 2,
	                           _mpaTransform.getNumPixels()(1) * resolution * sizeY/sizeX,
	                           -sizeY / 2,
				   sizeY / 2);
	_directHits = new TH2D("directHits", "",
	                           _mpaTransform.getNumPixels()(0) * resolution,
				   -sizeX / 2,
				   sizeX / 2,
	                           _mpaTransform.getNumPixels()(1) * resolution * sizeY/sizeX,
	                           -sizeY / 2,
				   sizeY / 2);
	_neighbourHits = new TH2D("neighbourHits", "",
	                           _mpaTransform.getNumPixels()(0) * resolution,
				   -sizeX / 2,
				   sizeX / 2,
	                           _mpaTransform.getNumPixels()(1) * resolution * sizeY/sizeX,
	                           -sizeY / 2,
				   sizeY / 2);
	_totalPixelHits.resize(48, 0);
	_activatedPixelHits.resize(48, 0);
	_alignFile.open(getFilename("_align.csv"));
	_analysisHitFile.open(getFilename("_corhits.csv"));
}

std::string EfficiencyTrack::getUsage(const std::string& argv0) const
{
	return Analysis::getUsage(argv0);
}

std::string EfficiencyTrack::getHelp(const std::string& argv0) const
{
        return Analysis::getHelp(argv0);
}

bool EfficiencyTrack::prealignRun(const core::TrackStreamReader::event_t& track_event,
                                  const core::MPAStreamReader::event_t& mpa_event)
{
	bool hasFiredPixel = false;
	for(const auto& counter: mpa_event.data) {
		if(counter)
			hasFiredPixel = true;
	}
	for(const auto& track: track_event.tracks) {
		_prealignPoints.push_back(track.extrapolateOnPlane(0, 4, 840, 2));
	}
	return _prealignPoints.size() < _numPrealigmentPoints;
}

void EfficiencyTrack::prealignFinish()
{
	std::cout << "Maximize count in prealigned MPA rectangle." << std::endl;
	Eigen::Vector3d offset(0, 0, 0);
	for(const auto& p: _prealignPoints) {
		offset += p;
	}
	offset /= _prealignPoints.size();
	auto hsize = _mpaTransform.getSensitiveSize()/2;
	auto shift = std::min(_mpaTransform.getPixelSize()(0), _mpaTransform.getPixelSize()(1));
	while(1) {
		std::vector<Eigen::Vector3d> t;
		std::vector<size_t> count;
		t.push_back(offset);
		t.push_back(offset + Eigen::Vector3d(shift, 0.0, 0.0));
		t.push_back(offset + Eigen::Vector3d(-shift, 0.0, 0.0));
		t.push_back(offset + Eigen::Vector3d(0.0, shift, 0.0));
		t.push_back(offset + Eigen::Vector3d(0.0, -shift, 0.0));
		for(const auto& of: t) {
			size_t c = 0;
			for(const auto& p: _prealignPoints) {
				if((p(0) < of(0)+hsize(0)) &&
				   (p(0) > of(0)-hsize(0)) &&
				   (p(1) < of(1)+hsize(1)) &&
				   (p(1) > of(1)-hsize(1))) {
					++c;
				}
			}
			count.push_back(c);
		}
		auto max = *std::max_element(count.begin(), count.end());
		assert(t.size() == count.size());
		size_t max_idx = 0;
		for(; max_idx < t.size(); ++max_idx) {
			if(count[max_idx] == max) {
				break;
			}
		}
		if(max_idx == 0)
			break;
		offset = t[max_idx];
	}
	_mpaTransform.setOffset(offset);
	std::cout << "Prealignment offset\n" << offset << std::endl;
}

bool EfficiencyTrack::align(const core::TrackStreamReader::event_t& track_event,
                            const core::MPAStreamReader::event_t&  mpa_event)
{
	if(_aligner.gotAlignmentData()) {
		return false;
	}
	bool empty = true;
	for(const auto& track: track_event.tracks) {
		auto b = track.extrapolateOnPlane(4, 5, 840, 2);
		for(size_t idx = 0; idx < mpa_event.data.size(); ++idx) {
			if(mpa_event.data[idx] == 0) continue;
			auto a = _mpaTransform.transform(idx);
			auto pc = _mpaTransform.translatePixelIndex(idx);
			_aligner.Fill(b(0) - a(0), b(1) - a(1));
			_alignFile << idx << " "
			     << a(0) << " "
			     << a(1) << " "
			     << b(0) << " "
			     << b(1) << " "
			     << pc(0) << " "
			     << pc(1) << "\n";
			empty = false;
		}
	}
	if(!empty)
		_alignFile << "\n\n" << std::flush;
	return true;
}

void EfficiencyTrack::alignFinish()
{
	_aligner.calculateAlignment();
	auto offset = _mpaTransform.getOffset();
	offset += _aligner.getOffset();
	auto cuts = _aligner.getCuts();
	std::cout << "x_off = " << offset(0)
	          << "\ny_off = " << offset(1)
		  << "\nz_off = " << offset(2)
	          << "\nx_sigma = " << cuts(0)
	          << "\ny_width = " << cuts(1) << std::endl; 
	_mpaTransform.setOffset(offset);
	_aligner.writeHistogramImage(getFilename("_align.png"));
	_aligner.saveAlignmentData(getFilename(".align"));
}

bool EfficiencyTrack::checkCorrelatedHits(const core::TrackStreamReader::event_t& track_event,
                              const core::MPAStreamReader::event_t& mpa_event)
{
	for(const auto& track: track_event.tracks) {
		for(size_t idx = 0; idx < mpa_event.data.size(); ++idx) {
			if(mpa_event.data[idx] == 0) continue;
			auto a = _mpaTransform.transform(idx);
			auto pc = _mpaTransform.translatePixelIndex(idx);
			auto b = track.extrapolateOnPlane(4, 5, 840, 2);
			auto center_b(b);
			center_b -= _mpaTransform.getOffset();
			if(_aligner.pointsCorrelatedX(a, b)) {
				_correlatedHits->Fill(center_b(0), center_b(1));
				_correlatedHitsX->Fill(center_b(0));
				_correlatedHitsY->Fill(center_b(1));
			}
		}
	}
	return true;
}

bool EfficiencyTrack::analyze(const core::TrackStreamReader::event_t& track_event,
                              const core::MPAStreamReader::event_t& mpa_event)
{
	for(const auto& track: track_event.tracks) {
/*		try {
			auto hit_idx = _mpaTransform.getPixelIndex(track);
			auto pc = _mpaTransform.translatePixelIndex(hit_idx);
			_totalPixelHits[hit_idx] += 1;
			if(mpa_event.data[hit_idx] > 0) {
				_activatedPixelHits[hit_idx] += 1;
				_efficiency->Fill(pc(0), pc(1));
			}
		} catch(std::out_of_range& e) {
			// track missed MPA
		}*/
		bool hitMpa = false;
		bool hitActivatedPixel = false;
		auto b = track.extrapolateOnPlane(4, 5, 840, 2);
		auto cb(b);
		cb -= _mpaTransform.getOffset();
		_trackHits->Fill(cb(0), cb(1));
		for(size_t idx = 0; idx < mpa_event.data.size(); ++idx) {
			auto a = _mpaTransform.transform(idx);
			auto pc = _mpaTransform.translatePixelIndex(idx);
			if(_aligner.pointsCorrelated(a, b)) {
				try {
					auto hit_idx = _mpaTransform.getPixelIndex(track);
					if(hit_idx == idx)
						_directHits->Fill(cb(0), cb(1));
					else
					       _neighbourHits->Fill(cb(0), cb(1));	
				} catch(std::out_of_range& e) {
					// track missed MPA...
					_neighbourHits->Fill(cb(0), cb(1));
				}
				_totalPixelHits[idx] += 1;
				hitMpa = true;
				_analysisHitFile << track_event.eventNumber << "\t" << cb(0) << "\t" << cb(1)
					<< "\t" << a(0) << "\t" << a(1);
				if(mpa_event.data[idx] == 0) {
					_analysisHitFile << "\t0/0\t0/0\t0\n";
				} else {
					_analysisHitFile << "\t"
						<< a(0) << "\t" << a(1)
						<<"\t1\n";
					if(!hitActivatedPixel) {
						_efficiency->Fill(cb(0), cb(1));
						hitActivatedPixel = true;
					}
				}
			}
		}
		if(hitMpa) {
			_analysisHitFile << "\n\n";
			_analysisHitFile.flush();
		}
	}
	return true;
}

void EfficiencyTrack::analyzeFinish()
{
/*	for(size_t x = 0; x < _trackHits->GetNbinsX(); ++x) {
		for(size_t y = 0; y < _trackHits->GetNbinsY(); ++y) {
			auto bin = _trackHits->GetBin(x, y);
			auto total =  _trackHits->GetBinContent(bin);
			if(total <= 0.0) continue;
			_efficiency->SetBinContent(bin, _efficiency->GetBinContent(bin) / total);
			_neighbourHits->SetBinContent(bin, _neighbourHits->GetBinContent(bin) / total);
			_directHits->SetBinContent(bin, _directHits->GetBinContent(bin) / total);
		}
	}*/
	auto eff = dynamic_cast<TH2D*>(_efficiency->Clone("pixelEfficiency"));
	auto neigh = dynamic_cast<TH2D*>(_neighbourHits->Clone("neighbourHitsScaled"));
	auto dir = dynamic_cast<TH2D*>(_directHits->Clone("directHitsScaled"));
	eff->Divide(_trackHits);
	neigh->Divide(_trackHits);
	dir->Divide(_trackHits);

	auto canvas = new TCanvas("comparision", "", 400*3*2, 300*3*4);
	gStyle->SetOptStat(11);
	gStyle->cd();
	canvas->Divide(2,4);
	canvas->cd(1);
	_trackHits->Draw("COLZ");

	canvas->cd(2);
	auto run = _runlist.getByMpaRun(_config.get<int>("MpaRun"));
	std::ostringstream info;
	auto txt = new TPaveText(0.1, 0.1, 0.9, 0.9);
	info << "MPA run no: " << run.mpa_run << "\n";
	txt->AddText(info.str().c_str());
	
	info.str("");
	info << "Tel run no: " << run.telescope_run << "\n";
	txt->AddText(info.str().c_str());
	
	info.str("");
	info << "Angle: " << run.angle << " degrees\n";
	txt->AddText(info.str().c_str());

	info.str("");
	info << "Bias voltage: " << run.bias_voltage << " V\n";
	txt->AddText(info.str().c_str());

	info.str("");
	info << "Bias current: " << run.bias_current << " uA\n";
	txt->AddText(info.str().c_str());
	txt->Draw();

	info.str("");
	info << "Threshold: " << run.threshold << " uA\n";
	txt->AddText(info.str().c_str());
	txt->Draw();

	canvas->cd(3);
	_efficiency->Draw("COLZ");
	canvas->cd(4);
	eff->Draw("COLZ");

	canvas->cd(5);
	_neighbourHits->Draw("COLZ");
	canvas->cd(6);
	neigh->Draw("COLZ");

	canvas->cd(7);
	_directHits->Draw("COLZ");
	canvas->cd(8);
	dir->Draw("COLZ");
	_file->Add(canvas);
	auto img = TImage::Create();
	img->FromPad(canvas);
	img->WriteImage(getFilename("_results.png").c_str());
	delete img;
}

