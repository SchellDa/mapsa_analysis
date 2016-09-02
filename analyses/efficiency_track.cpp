
#include "efficiency_track.h"
#include <iostream>
#include <TFitResult.h>
#include <algorithm>

REGISTER_ANALYSIS_TYPE(EfficiencyTrack, "Textual analysis description here.")

EfficiencyTrack::EfficiencyTrack() :
 Analysis(),
 _file(nullptr), _alignCorX(nullptr), _alignCorY(nullptr)
{
/*	addProcess(CS_TRACK,
	 std::bind(&EfficiencyTrack::prealignRun, this, std::placeholders::_1, std::placeholders::_2),
	 std::bind(&EfficiencyTrack::prealignFinish, this)
	);*/
	addProcess(CS_TRACK,
	 std::bind(&EfficiencyTrack::align, this, std::placeholders::_1, std::placeholders::_2),
	 std::bind(&EfficiencyTrack::alignFinish, this)
	);
/*	addProcess(CS_TRACK,
	 std::bind(&EfficiencyTrack::checkCorrelatedHits, this, std::placeholders::_1, std::placeholders::_2)
	);*/
	addProcess(CS_TRACK,
	 std::bind(&EfficiencyTrack::analyze, this, std::placeholders::_1, std::placeholders::_2),
	 std::bind(&EfficiencyTrack::analyzeFinish, this)
	);
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
	_file = new TFile(getRootFilename().c_str(), "RECREATE");
	_alignCorX = new TH1D("alignCorX", "", 1000, -5, 5);
	_alignCorY = new TH1D("alignCorY", "", 250, -5, 5);
	_correlatedHits = new TH2D("correlatedHits", "Coordinates of hits correlated on X axis",
	                           250, -_mpaTransform.getSensitiveSize()(0), _mpaTransform.getSensitiveSize()(0),
	                           250, -_mpaTransform.getSensitiveSize()(1), _mpaTransform.getSensitiveSize()(1));
	_correlatedHitsX = new TH1D("correlatedHitsX", "X coordinates of hits correlated on X axis", 250, -5, 5);
	_correlatedHitsY = new TH1D("correlatedHitsY", "Y coordinates of hits correlated on X axis", 250, -5, 5);
	_efficiency = new TH2D("pixelEfficiency", "", 16, 0, 16, 3, 0, 3);
	_neighbourActivation = nullptr;
	_totalPixelHits.resize(48, 0);
	_activatedPixelHits.resize(48, 0);
	fout.open("test.csv");
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
	bool empty = true;
	for(const auto& track: track_event.tracks) {
		for(size_t idx = 0; idx < mpa_event.data.size(); ++idx) {
			if(mpa_event.data[idx] == 0) continue;
			auto a = _mpaTransform.transform(idx);
			auto pc = _mpaTransform.translatePixelIndex(idx);
			auto b = track.extrapolateOnPlane(4, 5, 840, 2);
			_alignCorX->Fill(b(0) - a(0));
			_alignCorY->Fill(b(1) - a(1));
			fout << idx << " "
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
		fout << "\n\n" << std::flush;
	return true;
}

void EfficiencyTrack::alignFinish()
{
	auto offset = _mpaTransform.getOffset();
	auto fitX = getAlignOffset(_alignCorX, 0.5);
	auto fitY = getAlignOffset(_alignCorY, 2);
	offset += Eigen::Vector3d(
		fitX(0),
		fitY(0),
		0.0);
	_alignSigma = Eigen::Vector3d(
		fitX(2),
		fitY(2),
		0.0);
	auto oldY = getAlignOffset(_alignCorY, 0.5);
	std::cout << "y difference = " << (fitY(0) - oldY(0))/fitY(0)*100.0 << std::endl;
	std::cout << "x_off = " << offset(0)
	          << "\ny_off = " << offset(1)
		  << "\nz_off = " << offset(2)  << std::endl;
	_mpaTransform.setOffset(offset);
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
			auto center_b = b - _mpaTransform.getOffset();
			if(abs(a(0)-b(0)) < _alignSigma(0)*2) {
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
		for(size_t idx = 0; idx < mpa_event.data.size(); ++idx) {
			auto a = _mpaTransform.transform(idx);
			auto b = track.extrapolateOnPlane(4, 5, 840, 2);
			auto cb = b - _mpaTransform.getOffset();
			auto pc = _mpaTransform.translatePixelIndex(idx);
			if(abs(a(0)-cb(0)) < _alignSigma(0)*2 &&
			   abs(a(1)-cb(1)) < _alignSigma(1)*2) {
				_totalPixelHits[idx] += 1;
				if(mpa_event.data[idx] == 0) continue;
				_efficiency->Fill(pc(0), pc(1));
			}
		}
	}
	return true;
}

void EfficiencyTrack::analyzeFinish()
{
	for(size_t idx = 0; idx < _totalPixelHits.size(); ++idx) {
		auto pc = _mpaTransform.translatePixelIndex(idx);
//		double efficiency = static_cast<double>(_activatedPixelHits[idx]) / _totalPixelHits[idx];
//		_efficiency->SetPoint(idx, pc(0), pc(1), efficiency);
		auto binNum = _efficiency->FindBin(pc(0), pc(1));
		auto count = _efficiency->GetBinContent(binNum);
		if(_totalPixelHits[idx] > 0) {
			std::cout << idx << " " << count << " " << _totalPixelHits[idx] << " " << count/_totalPixelHits[idx] << std::endl;
			_efficiency->SetBinContent(binNum, count/_totalPixelHits[idx]);
		}
	}
}

Eigen::Vector4d EfficiencyTrack::getAlignOffset(TH1D* cor, const double& nrms=0.5)
{
	auto rms = cor->GetRMS();
	auto mean = cor->GetMean();
//	cor->SetParameter(1, mean);
//	cor->SetParameter(2, rms);
	auto result = cor->Fit("gaus", "RMS+", "", mean-rms*nrms, mean+rms*nrms);
	return {
		result->Parameter(1),
		result->Error(1),
		result->Parameter(2),
		result->Error(2),
	};
}
