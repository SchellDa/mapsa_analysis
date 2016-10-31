
#include "efficiency_track.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <TCanvas.h>
#include <TStyle.h>
#include <TImage.h>
#include <TText.h>
#include <TPaveText.h>

REGISTER_ANALYSIS_TYPE(EfficiencyTrack, "Textual analysis description here.")

EfficiencyTrack::EfficiencyTrack() :
 Analysis(), _file(nullptr), _forceAlignment(false), _totalHitCount(0), _correlatedHitCount(0)
{
/*	addProcess("prealign", CS_TRACK,
	 std::bind(&EfficiencyTrack::prealignRun, this, std::placeholders::_1, std::placeholders::_2),
	 std::bind(&EfficiencyTrack::prealignFinish, this)
	);*/
	/*addProcess("align", CS_TRACK,
	 Analysis::init_callback_t{},
	 std::bind(&EfficiencyTrack::alignInit, this),
	 std::bind(&EfficiencyTrack::alignRun, this, std::placeholders::_1, std::placeholders::_2),
	 std::bind(&EfficiencyTrack::alignFinish, this),
	 Analysis::post_callback_t{}
	);*/
	/*addProcess("checkCorrelatedHits", CS_TRACK,
	 std::bind(&EfficiencyTrack::checkCorrelatedHits, this, std::placeholders::_1, std::placeholders::_2)
	);*/
	addProcess("analyze", CS_TRACK,
	 core::Analysis::init_callback_t{},
	 std::bind(&EfficiencyTrack::analyzeRunInit, this),
	 std::bind(&EfficiencyTrack::analyze, this, std::placeholders::_1, std::placeholders::_2),
	 Analysis::run_post_callback_t{},
	 std::bind(&EfficiencyTrack::analyzeFinish, this)
	);
	getOptionsDescription().add_options()
//		("align,a", "Force recalculation of alignment parameters")
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
	if(vm.count("align") >= 1) {
		_forceAlignment = true;
	}
	_file = new TFile(getRootFilename().c_str(), "RECREATE");
	double sizeX = _mpaTransform.getSensitiveSize()(0);
	double sizeY = _mpaTransform.getSensitiveSize()(1);
	auto resolution = _config.get<unsigned int>("efficiency_histogram_resolution_factor");
	_correlatedHits = new TH2D("correlatedHits", "Coordinates of hits correlated on X axis",
	                           250, -_mpaTransform.getSensitiveSize()(0), sizeX,
	                           250, -_mpaTransform.getSensitiveSize()(1), _mpaTransform.getSensitiveSize()(1));
	_correlatedHitsX = new TH1D("correlatedHitsX", "X coordinates of hits correlated on X axis", 500, -10, 10);
	_correlatedHitsY = new TH1D("correlatedHitsY", "Y coordinates of hits correlated on X axis", 500, -10, 10);
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
	int nx = 2;
	int ny = 3;
	int overlayed_resolution_factor = 2;
	_efficiencyOverlayed = new TH2D("efficiencyOverlayed", "",
	                           nx * resolution*overlayed_resolution_factor,
				   0,
				   nx,
	                           ny * resolution*overlayed_resolution_factor,
	                           0,
				   ny);
	_trackHitsOverlayed = new TH2D("trackHitsOverlayed", "",
	                           nx * resolution*overlayed_resolution_factor,
				   0,
				   nx,
	                           ny * resolution*overlayed_resolution_factor,
	                           0,
				   ny);
	_efficiencyLocal = new TH2D("efficiencyLocal", "Efficiency in local pixel coordinates",
	                           16 * resolution,
				   0,
				   16,
				   3 * resolution * sizeY/sizeX,
				   0,
				   3);
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
                                  const core::BaseSensorStreamReader::event_t& mpa_event)
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
	_mpaTransform.setAlignmentOffset(offset);
	std::cout << "Prealignment offset\n" << offset << std::endl;
}

void EfficiencyTrack::alignInit()
{
	const int runId = getCurrentRunId();
	_aligner[runId].setNSigma(_config.get<double>("n_sigma_cut"));
	if(!_forceAlignment) {
		_aligner[runId].loadAlignmentData(getFilename(runId, ".align"));
	}
	if(!_aligner[runId].gotAlignmentData()) {
		_aligner[runId].initHistograms();
	}
}

bool EfficiencyTrack::alignRun(const core::TrackStreamReader::event_t& track_event,
                               const core::BaseSensorStreamReader::event_t&  mpa_event)
{
	const auto runId = getCurrentRunId();
	if(_aligner[runId].gotAlignmentData()) {
		return false;
	}
	bool empty = true;
	for(const auto& track: track_event.tracks) {
		auto b = track.extrapolateOnPlane(4, 5, 840, 2);
		for(size_t idx = 0; idx < mpa_event.data.size(); ++idx) {
			if(mpa_event.data[idx] == 0) continue;
			auto a = _mpaTransform.transform(idx);
			auto pc = _mpaTransform.translatePixelIndex(idx);
			_aligner[runId].Fill(b(0) - a(0), b(1) - a(1));
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
	const auto runId = getCurrentRunId();
	_aligner[runId].writeHistogramImage(getFilename(runId, "_align.png"));
	_aligner[runId].writeHistograms();
	_file->Write();
	_aligner[runId].calculateAlignment();
	auto offset = _mpaTransform.getOffset();
	offset += _aligner[runId].getOffset();
	auto cuts = _aligner[runId].getCuts();
	std::cout << "Alignment for run " << runId
	          << "\n  x_off = " << offset(0)
	          << "\n  y_off = " << offset(1)
		  << "\n  z_off = " << offset(2)
	          << "\n  x_sigma = " << cuts(0)
	          << "\n  y_width = " << cuts(1) << std::endl; 
	_aligner[runId].writeHistogramImage(getFilename(runId, "_align.png"));
	_aligner[runId].saveAlignmentData(getFilename(runId, ".align"));
}

bool EfficiencyTrack::checkCorrelatedHits(const core::TrackStreamReader::event_t& track_event,
                              const core::BaseSensorStreamReader::event_t& mpa_event)
{
	for(const auto& track: track_event.tracks) {
		for(size_t idx = 0; idx < mpa_event.data.size(); ++idx) {
			if(mpa_event.data[idx] == 0) continue;
			auto a = _mpaTransform.transform(idx);
			auto pc = _mpaTransform.translatePixelIndex(idx);
			auto b = track.extrapolateOnPlane(4, 5, 840, 2);
			auto center_b(b);
			center_b -= _mpaTransform.getOffset();
			if(_aligner[getCurrentRunId()].pointsCorrelatedX(a, b)) {
				_correlatedHits->Fill(center_b(0), center_b(1));
				_correlatedHitsX->Fill(center_b(0));
				_correlatedHitsY->Fill(center_b(1));
			}
		}
	}
	return true;
}

void EfficiencyTrack::analyzeRunInit()
{
	const int runId = getCurrentRunId();
	_aligner[runId].setNSigma(_config.get<double>("n_sigma_cut"));
	std::string alignfile (
		_config.get<std::string>("output_dir") +
		std::string("/ZAlignTest_") +
		getMpaIdPadded(runId) +
		".align"
	);
	_aligner[runId].loadAlignmentData(alignfile);
	if(!_aligner[runId].gotAlignmentData()) {
		throw std::runtime_error(std::string("Cannot find alignment data for run ")
		                         + std::to_string(runId));
	}
	_mpaTransform.setAlignmentOffset(_aligner[getCurrentRunId()].getOffset());
	auto offset = _mpaTransform.getOffset();
	std::cout << "Run MPA offset: "
	          << offset(0) << " "
		  << offset(1) << " "
		  << offset(2) << std::endl;
}

bool EfficiencyTrack::analyze(const core::TrackStreamReader::event_t& track_event,
                              const core::BaseSensorStreamReader::event_t& mpa_event)
{
	const auto runId = getCurrentRunId();
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
		auto b = track.extrapolateOnPlane(4, 5, _mpaTransform.getOffset()(2), 2);
		auto cb(b);
		cb -= _mpaTransform.getOffset();
		_trackHits->Fill(cb(0), cb(1));
		++_totalHitCount;
		// combine results of pixels in first and second row in a 2x2
		// pixel grid. That way, we have the same geometry
		// (punch-through etc.) for each overlayed pixel.
		auto globalHitpoint = _mpaTransform.mpaPlaneTrackIntersect(track, 4, 5);
		auto pc = _mpaTransform.globalToPixelCoord(globalHitpoint);
		Eigen::Vector2d subpc(std::fmod(pc(0), 2.0), std::fmod(pc(1), 3.0));
		if(subpc(1) < 0.0 || subpc(1) > 3.0) {
			subpc(1) += 10.0;
		}
		_trackHitsOverlayed->Fill(subpc(0), subpc(1));
		for(size_t idx = 0; idx < mpa_event.data.size(); ++idx) {
			auto a = _mpaTransform.transform(idx);
			if(_aligner[runId].pointsCorrelated(a, b)) {
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
						++_correlatedHitCount;
						_efficiency->Fill(cb(0), cb(1));
						_efficiencyOverlayed->Fill(subpc(0), subpc(1));
						_efficiencyLocal->Fill(pc(0), pc(1));
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
	std::cout << "Write analysis results..." << std::endl;
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

	auto canvas = new TCanvas("comparision", "", 400*3*2, 300*3*6);
	gStyle->SetOptStat(11);
	gStyle->cd();
	canvas->Divide(2, 6);
	canvas->cd(1);
	_trackHits->Draw("COLZ");

	canvas->cd(2);
	auto run = _runlist.getByMpaRun(getAllRunIds()[0]);
	std::ostringstream info;
	auto txt = new TPaveText(0.1, 0.1, 0.9, 0.9);
	info << "Info for MPA run: " << run.mpa_run << "\n";
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

	info.str("");
	info << "Threshold: " << run.threshold << " uA\n";
	txt->AddText(info.str().c_str());

	const auto& runs = getAllRunIds();
	if(runs.size() > 1) {
		info.str("");
		info << "Runs used for analysis:";
		txt->AddText(info.str().c_str());
		info.str("");
		int counter = 0;
		for(const auto& runId: runs) {
			if(++counter % 8 == 0) {
				txt->AddText(info.str().c_str());
				info.str("");
			}
			info << runId << " ";
		}
		txt->AddText(info.str().c_str());
	}
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

	canvas->cd(9);
	_trackHitsOverlayed->Draw("COLZ");
	canvas->cd(10);
	_efficiencyOverlayed->Draw("COLZ");

	canvas->cd(11);
	auto overlayeff = dynamic_cast<TH2D*>(_efficiencyOverlayed->Clone("efficiencyOverlayedScaled"));
	overlayeff->Divide(_trackHitsOverlayed);
	overlayeff->Draw("COLZ");


	auto img = TImage::Create();
	img->FromPad(canvas);
	img->WriteImage(getFilename("_results.png").c_str());
	delete img;

	double efficiency = static_cast<double>(_correlatedHitCount) / _totalHitCount;
	std::ofstream fout(getFilename(".eff"));
	fout << "0\t"
	     << _totalHitCount << "\t"
	     << _correlatedHitCount << "\t"
	     << efficiency << "\t"
	     << run.angle << "\t"
	     << run.threshold << "\t";
	std::cout << "Total hits: " << _totalHitCount
	          << "\nDUT hits: " << _correlatedHitCount
		  << "\nEfficiency: " << std::setprecision(2) << std::fixed
		  << efficiency * 100.0 << "%"
		  << std::endl;
	bool first = true;
	for(const auto runId: getAllRunIds()) {
		if(!first) fout << ",";
		fout << runId;
	}
	fout << "\n";
}

