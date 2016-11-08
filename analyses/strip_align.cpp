
#include "strip_align.h"
#include <TImage.h>
#include <TText.h>
#include <TGraph.h>
#include <TFitResult.h>

REGISTER_ANALYSIS_TYPE(StripAlign, "Textual analysis description here.")

StripAlign::StripAlign() :
 Analysis(), _file(nullptr)
{
	getOptionsDescription().add_options()
		("low-y", po::value<double>()->default_value(-100), "Lower bound of Y align scan")
		("high-y", po::value<double>()->default_value(100), "Upper bound of Y align scan")
		("low-z", po::value<double>()->default_value(820), "Lower bound of Z align scan")
		("high-z", po::value<double>()->default_value(860), "Upper bound of Z align scan")
		("reuse-z", "If set, use an already calculated Z alignment and only redo the Y alignment.")
		("no-y", "Do not calculate Y alignment.")
		("num-steps,s", po::value<int>()->default_value(10), "Number of steps in the range (low,high)")
		("sample-size,n", po::value<int>()->default_value(10000), "Number of data points to include in alignment histogram")
		("low-shift", po::value<int>()->default_value(-10), "Lower shift value for --shift mode")
		("high-shift", po::value<int>()->default_value(10), "Upper shift value for --shift mode")
		("shift", "Do not scan different Z positions but different data shift values.")
	;
	addProcess("scan", /* CS_ALWAYS */ CS_TRACK,
	           core::Analysis::init_callback_t {},
	           std::bind(&StripAlign::scanInit, this),
		   std::bind(&StripAlign::scanRun, this, std::placeholders::_1, std::placeholders::_2),
	           core::Analysis::run_post_callback_t {},
	           std::bind(&StripAlign::scanFinish, this)
	           );
	addProcess("yAlign", /* CS_ALWAYS */ CS_TRACK,
	           core::Analysis::init_callback_t {},
	           std::bind(&StripAlign::yAlignInit, this),
		   std::bind(&StripAlign::yAlignRun, this, std::placeholders::_1, std::placeholders::_2),
	           core::Analysis::run_post_callback_t {},
	           std::bind(&StripAlign::yAlignFinish, this)
	           );
}

StripAlign::~StripAlign()
{
	_out.close();
	if(_file) {
		_file->Write();
		_file->Close();
		delete _file;
	}
}

void StripAlign::init(const po::variables_map& vm)
{
	_out.open(getFilename(".csv"), std::ofstream::out);
	if(!_out.good()) {
		throw std::ios_base::failure("Cannot open debug output file.");
	}
	_file = new TFile(getFilename(".root").c_str(), "RECREATE");
	assert(_file);
	_sampleSize = vm["sample-size"].as<int>();
	_lowY = vm["low-y"].as<double>();
	_highY = vm["high-y"].as<double>();
	_lowZ = vm["low-z"].as<double>();
	_highZ = vm["high-z"].as<double>();
	_numSteps = vm["num-steps"].as<int>();
	_lowShift = vm["low-shift"].as<int>();
	_highShift = vm["high-shift"].as<int>();
	_shift = vm.count("shift") > 0;
	_reuseZ	= vm.count("reuse-z") > 0;
	_noY = vm.count("no-y") > 0;
	if(_shift) {
		assert(_highShift > _lowShift);
		_numSteps = _highShift - _lowShift + 1;
	}
	_currentScanStep = 0;
	{
		int num_plots = _numSteps + 2;
		int n_x = std::sqrt(num_plots);
		int n_y = std::sqrt(num_plots);
		while(n_x*n_y < num_plots) {
			++n_y;
		}
		int width = n_x * 400;
		int height = n_y * 300;
		assert(n_x > 0);
		assert(n_y > 0);
		assert(n_x*n_y >= num_plots);
		_canvas = new TCanvas("Canvas", "Canvas", width, height);
		_canvas->Divide(n_x, n_y);
	}
	{
		int num_plots = _numSteps;
		int n_x = std::sqrt(num_plots);
		int n_y = std::sqrt(num_plots);
		while(n_x*n_y < num_plots) {
			++n_y;
		}
		int width = n_x * 400;
		int height = n_y * 300;
		assert(n_x > 0);
		assert(n_y > 0);
		assert(n_x*n_y >= num_plots);
		_canvasX = new TCanvas("CanvasX", "Canvas", width, height);
		_canvasY = new TCanvas("CanvasY", "Canvas", width, height);
		_canvasX->Divide(n_x, n_y);
		_canvasY->Divide(n_x, n_y);
	}
	_canvas->cd(1);
	auto txt = new TText(.5, .5, "");
	txt->SetTextAlign(22);
	txt->SetTextSize(0.2);
	txt->Draw();
	if(_reuseZ) {
		std::ifstream alignfile(getFilename("_best.align"));
		if(alignfile.good()) {
			double x, y, z, sigma;
			alignfile >> x >> y >> z >> sigma;
			_zAlignment.position = Eigen::Vector3d(x, y, z);
			_zAlignment.sigma = sigma;
			if(!alignfile.good()) {
				std::cout << "Error while reading alignment file, recalculate Z alignment." << std::endl;
				_reuseZ = false;
			}
			std::cout << "Loaded the following XZ-Alignment:"
			          << "\n  Position "
				  << _zAlignment.position(0) << "mm "
				  << _zAlignment.position(1) << "mm "
				  << _zAlignment.position(2) << "mm"
				  << "\n  Sigma " << _zAlignment.sigma << "mm" << std::endl;
		} else {
			std::cout << "No alignment file found, recalculate Z alignment." << std::endl;
			_reuseZ = false;
		}
	}
	if(_reuseZ && _noY) {
		throw std::runtime_error("Reuse Z alignment and do not calculate Y alignment? Great, do nothing... asshole!");
	}
}

std::string StripAlign::getUsage(const std::string& argv0) const
{
	return Analysis::getUsage(argv0);
}

std::string StripAlign::getHelp(const std::string& argv0) const
{
        return Analysis::getHelp(argv0);
}

void StripAlign::scanInit()
{
	if(_reuseZ) {
		return;
	}
	assert(_file);
	_numProcessedSamples = 0;
	if(_shift) {
		_currentShift = _lowShift + _currentScanStep;
		setDataOffset(_currentShift);
		_currentZ = _lowZ;
		_config.setVariable("mpa_z_offset", _lowZ);
	} else {
		_currentZ = _lowZ + (_highZ - _lowZ) / (_numSteps-1) * _currentScanStep;
		_config.setVariable("mpa_z_offset", _currentZ);
	}
	std::cout << _currentScanStep+1 << "/" << _numSteps
	          << ": Calculate alignment constants for Z = " << _currentZ
		  << "mm, skip = " << getDataOffset() << std::endl;
	std::string header = "Z = ";
	header += std::to_string(_currentZ);
	header += "mm, skip = ";
	header += std::to_string(getDataOffset());
	const int k = 2;
	_corHist = new TH1D((std::string("align_")+std::to_string(_currentScanStep)).c_str(), header.c_str(), 4000*k, -10*k, 10*k);
	_corX = new TH2D((std::string("cor_x_")+std::to_string(_currentScanStep)).c_str(),
	                 (std::string("X: ")+header).c_str(), 50*k, -10*k, 10*k, 50*k, -10*k, 10*k);
	_corX->GetXaxis()->SetTitle("Track X");
	_corX->GetYaxis()->SetTitle("Strip Position");
	_corY = new TH2D((std::string("cor_y_")+std::to_string(_currentScanStep)).c_str(),
	                 (std::string("Y: ")+header).c_str(), 50*k, -10*k, 10*k, 50*k, -10*k, 10*k);
	_corY->GetXaxis()->SetTitle("Track Y");
	_corY->GetYaxis()->SetTitle("Strip Position");
	_file->Add(_corHist);
	_file->Add(_corX);
	_file->Add(_corY);
	_currentSigmaMinimum = { 0.0, -1.0 };
}

bool StripAlign::scanRun(const core::TrackStreamReader::event_t& track_event,
                      const core::BaseSensorStreamReader::event_t& mpa_event)
{
	if(_reuseZ) {
		return false;
	}
	int n_strips = _config.get<int>("strip_count");
	double pitch = _config.get<double>("strip_pitch");
	double length = _config.get<double>("strip_length");
	for(const auto& track: track_event.tracks) {
		auto b = track.extrapolateOnPlane(1, 3, _currentZ, 2);
		for(const auto& idx: mpa_event.data) {
			// ignore det1
			if(idx >= 254) {
				continue;
			}
			double x = (static_cast<double>(idx) - n_strips/2) * pitch;
			_out << mpa_event.eventNumber << "\t"
			     << b(0) << "\t"
			     << b(1) << "\t"
			     << b(2) << "\t"
			     << idx << "\t"
			     << x << std::endl;
			//std::cout << idx << " -> " << x << std::endl;
			_corHist->Fill(b(0) - x);
			_corX->Fill(b(0), x);
			_corY->Fill(b(1), x);
		}
	}
	_out << std::endl;
	return (_numProcessedSamples++ < _sampleSize);
}

void StripAlign::scanFinish()
{
	if(_reuseZ)
		return;
	_canvasX->cd(_currentScanStep+1);
	_corX->Draw("COLZ");
	_canvasY->cd(_currentScanStep+1);
	_corY->Draw("COLZ");
	_out.flush();
	int cd = _currentScanStep+2;
	std::cout << "cd-ing to Pad " << cd << std::endl;
	_canvas->cd(cd);
	double mean = _corHist->GetMean();
	double rms = _corHist->GetRMS();
	auto result = _corHist->Fit("gaus", "SAME", "", mean-rms*2, mean+rms*2);
	double sigma = result->Parameter(2);
	double x = result->Parameter(1);
	_alignments.push_back({
		Eigen::Vector3d(
			x, 0,
			_currentZ),
		sigma
	});
	if(_currentSigmaMinimum(1) < sigma || _currentSigmaMinimum(1) < 0) {
		_currentSigmaMinimum = { _currentZ, sigma };
	}
	_corHist->Draw();
	++_currentScanStep;
	if(_currentScanStep < _numSteps) {
		rerun();
	} else if(_currentScanStep+1 >= _numSteps) {

		auto xgraph = new TGraph(_numSteps);
		xgraph->SetName("x_sigma");
		for(size_t i=0; i < _alignments.size(); ++i) {
			const auto& align = _alignments[i];
			if(_shift) {
				xgraph->SetPoint(i, _lowShift+i, align.sigma);
			} else {
				xgraph->SetPoint(i, align.position(2), align.sigma);
			}
		}
		_file->Add(xgraph);
		double x_min = xgraph->GetX()[0];
		double y_min = xgraph->GetY()[0];
		size_t best_idx = 0;
		for(size_t i=0; i < xgraph->GetN(); ++i) {
			if(y_min > xgraph->GetY()[i]) {
				x_min = xgraph->GetX()[i];
				y_min = xgraph->GetY()[i];
				best_idx = i;
			}
		}
		_canvas->cd(_currentScanStep+2);
		xgraph->Draw("A*");
		_canvas->Update();

		auto img = TImage::Create();
		img->FromPad(_canvas);
		img->WriteImage(getFilename(".png").c_str());
		delete img;

		img = TImage::Create();
		img->FromPad(_canvasX);
		img->WriteImage(getFilename("_cor_x.png").c_str());
		delete img;

		img = TImage::Create();
		img->FromPad(_canvasY);
		img->WriteImage(getFilename("_cor_y.png").c_str());
		delete img;

		const auto& align = _alignments[best_idx];
		_zAlignment = align;
		std::cout << "Best aligment at Z = " << align.position(2) << "mm with a sigma of "
		          << align.sigma << "mm" << std::endl;
		writeAlignment(align);
	}
}

void StripAlign::yAlignInit()
{
	if(_noY) return;
	_numProcessedSamples = 0;
	_currentY = _lowY + (_highY - _lowY)/(_numSteps) * getRerunNumber();
	_yHitcounter[_currentY] = 0;
	std::cout << getRerunNumber()+1 << "/" << _numSteps
	          << ": Count hits for Y = " << _currentY
		  << "mm" << std::endl;
}

bool StripAlign::yAlignRun(const core::TrackStreamReader::event_t& track_event,
                           const core::BaseSensorStreamReader::event_t& mpa_event)
{
	if(_noY) return false;
	const double sensor_active_x = 11.860; // mm, 127 strips + bias ring
	const double sensor_active_y = 48.522; // mm, strip length
	double x_low = -_zAlignment.position(0) - sensor_active_x/2;
	double x_high = -_zAlignment.position(0) + sensor_active_x/2;
	double y_low = _currentY - sensor_active_y/2;
	double y_high = _currentY + sensor_active_y/2;
	auto& counter = _yHitcounter[_currentY];
	for(const auto& track: track_event.tracks) {
		auto b = track.extrapolateOnPlane(1, 3, _zAlignment.position(2), 2);
		//std::cout << _currentY << " -> "
		//          << b(0) << " "
		//          << b(1) << ": ["
		//	  << x_low << " "
		//	  << x_high << "] ["
		//	  << y_low << " "
		//	  << y_high << "]" << std::endl;
		// Only count tracks hitting active sensor area
		if(b(0) > x_low && b(0) < x_high &&
		   b(1) > y_low && b(1) < y_high) {
			++counter;
		}
	}
//	std::cout << "counter: " << _yHitcounter[_currentY] << std::endl;
	return (_numProcessedSamples++ < _sampleSize);
}

void StripAlign::yAlignFinish()
{
	if(_noY) return;
	if(getRerunNumber() < _numSteps) {
		rerun();
	} else {
		double best_y = _yHitcounter.begin()->first;
		double best_y_max = best_y;
		double best_hits = _yHitcounter.begin()->second;
		auto canvas = new TCanvas("canvas", "");
		auto graph = new TGraph(_yHitcounter.size());
		size_t i=0;
		for(const auto& point: _yHitcounter) {
			graph->SetPoint(i++, point.first, point.second);
			if(best_hits < point.second) {
				best_y = point.first;
				best_y_max = best_y;
				best_hits = point.second;
			} else if(best_hits == point.second) {
				best_y_max = point.first;
			}
		}
		graph->SetTitle("Y alignment");
		graph->GetXaxis()->SetTitle("Y position (mm)");
		graph->GetYaxis()->SetTitle("# track hits");
		graph->Draw("A*");
		auto img = TImage::Create();
		img->FromPad(canvas);
		img->WriteImage(getFilename("_yalign.png").c_str());
		delete img;
		std::cout << "Maximum number of hits (" << best_hits << ") for Y = "
		          << best_y << "mm to " << best_y_max << "mm\n";
		_zAlignment.position(1) = (best_y + best_y_max)/2;
		std::cout << "Compromise on mean value: Y = " << _zAlignment.position(1) << "mm" << std::endl;
		writeAlignment(_zAlignment);
		std::cout << "Final alignment:"
		          << "\n  Position "
			  << _zAlignment.position(0) << "mm "
			  << _zAlignment.position(1) << "mm "
			  << _zAlignment.position(2) << "mm"
			  << "\n  Sigma " << _zAlignment.sigma << "mm" << std::endl;
	}
}

void StripAlign::writeAlignment(const alignment_t& align) const
{
	std::ofstream of(getFilename("_best.align"));
	of << align.position(0) << " "
	   << align.position(1) << " "
	   << align.position(2) << " "
	   << align.sigma << " "
	   << 0.0 << "\n";
	of.flush();
	of.close();
}
