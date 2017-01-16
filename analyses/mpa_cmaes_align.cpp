
#include "mpa_cmaes_align.h"
#include <TImage.h>
#include <TText.h>
#include <TGraph.h>
#include <cmaes.h>
#include <iomanip>
#include <cstdio>

using namespace libcmaes;

REGISTER_ANALYSIS_TYPE(MpaCmaesAlign, "Perform XYZ and angular alignment of MPA.")

MpaCmaesAlign::MpaCmaesAlign() :
 Analysis(), _aligner(), _file(nullptr)
{
	getOptionsDescription().add_options()
		("low-z", po::value<double>()->default_value(820), "Lower bound of Z align scan")
		("high-z", po::value<double>()->default_value(860), "Upper bound of Z align scan")
		("num-steps,s", po::value<int>()->default_value(10), "Number of steps in the range (low,high)")
		("sample-size,n", po::value<int>()->default_value(10000), "Number of data points to include in alignment histogram")
	;
	addProcess("load", /* CS_ALWAYS */ CS_TRACK,
	           core::Analysis::init_callback_t {},
	           std::bind(&MpaCmaesAlign::scanInit, this),
		   std::bind(&MpaCmaesAlign::scanRun, this, std::placeholders::_1, std::placeholders::_2),
	           core::Analysis::run_post_callback_t {},
	           std::bind(&MpaCmaesAlign::scanFinish, this)
	           );
}

MpaCmaesAlign::~MpaCmaesAlign()
{
	if(_file) {
		_file->Write();
		delete _file;
	}
}

void MpaCmaesAlign::init(const po::variables_map& vm)
{
	_file = new TFile(getFilename(".root").c_str(), "RECREATE");
	_aligner.initHistograms();
	_sampleSize = vm["sample-size"].as<int>();
	_eventCache.reserve(_sampleSize);
	_cacheFull = false;
	_forceStatus = _config.get<int>("cmaes_force_status") > 0;
	_allowedExitStatus = _config.getVector<int>("cmaes_allowed_exit_status");
	_maxForceStatusRuns = _config.get<int>("cmaes_max_force_status_runs");
	std::remove(getFilename(".status").c_str());
	std::ofstream statusFile(getFilename(".status"));
	statusFile << "# Rerun\tStatus" << std::endl;
	/*int num_plots = _numSteps + 4;
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
	_xCanvas = new TCanvas("xCanvas", "xCanvas", width, height);
	_xCanvas->Divide(n_x, n_y);
	_xCanvas->cd(1);
	auto txt = new TText(.5, .5, "Hallo Welt!");
	txt->SetTextAlign(22);
	txt->SetTextSize(0.2);
	txt->Draw();
	_yCanvas = new TCanvas("yCanvas", "yCanvas", width, height);
	_yCanvas->Divide(n_x, n_y);*/
}

std::string MpaCmaesAlign::getUsage(const std::string& argv0) const
{
	return Analysis::getUsage(argv0);
}

std::string MpaCmaesAlign::getHelp(const std::string& argv0) const
{
        return Analysis::getHelp(argv0);
}

void MpaCmaesAlign::scanInit()
{
}

bool MpaCmaesAlign::scanRun(const core::TrackStreamReader::event_t& track_event,
                      const core::BaseSensorStreamReader::event_t& mpa_event)
{
	if(_cacheFull) {
		return false;
	}
	size_t nhits = 0;
	size_t mpa_index = 0;
	for(size_t i=0; i < mpa_event.data.size(); ++i) {
		if(mpa_event.data[i]) {
			++nhits;
			mpa_index = i;
		}
	}
	if(track_event.tracks.size() == 1 && nhits == 1) {
		_eventCache.push_back({track_event.tracks[0], mpa_index});
	}
	return _eventCache.size() < _sampleSize;
}

void MpaCmaesAlign::scanFinish()
{
	_cacheFull = true;
	std::ofstream func_file(getFilename("_space.csv"));
//	std::ofstream hitpoints_file(getFilename("_hitpoints.csv"));
//	std::ofstream misspoints_file(getFilename("_misspoints.csv"));
	std::ofstream path_file(getFilename("_path.csv"));
	FitFunc chi2 = [this, &func_file/*, &hitpoints_file, &misspoints_file*/](const double* param, const int N) mutable
	{
		core::MpaTransform trans;
		trans.setOffset({param[0], param[1], param[2]});
		trans.setRotation({param[3], param[4], param[5]});
		double chi2val = 0.0;
		size_t total_entries = 0;
		size_t num_entries = 0;
		for(const auto& evt: _eventCache) {
			auto b = trans.mpaPlaneTrackIntersect(evt.track, 0, 5);
			++total_entries;
			auto a = trans.transform(evt.mpa_index);
			auto sqrdist = (b-a).squaredNorm();
			if(sqrdist < 1) {
				chi2val += sqrdist;
				++num_entries;
		/*		hitpoints_file << a(0) << " "
				               << a(1) << " "
				               << a(2) << "\t"
				               << b(0) << " "
				               << b(1) << " "
				               << b(2) << "\n";*/
			} else {
/*				misspoints_file << a(0) << " "
				                << a(1) << " "
				                << a(2) << "\t"
				                << b(0) << " "
				                << b(1) << " "
				                << b(2) << "\n";*/
			}
		}
//		hitpoints_file << "\n\n";
//		misspoints_file << "\n\n";
		double fitness = 1000;
		if(num_entries > total_entries/100) {
			fitness = chi2val / num_entries;
		}
		func_file << fitness << "\t"
		          << param[0] << "\t"
		          << param[1] << "\t"
		          << param[2] << "\t"
		          << param[3] << "\t"
		          << param[4] << "\t"
		          << param[5] << "\t"
			  << num_entries << "\t"
		          << _eventCache.size() << "\n";
		return fitness;
	};
	ProgressFunc<CMAParameters<GenoPheno<pwqBoundStrategy>>, CMASolutions> select_time =
		[this, &path_file](const CMAParameters<GenoPheno<pwqBoundStrategy>>& params, const CMASolutions& cmasols)
	{
		auto cand = cmasols.best_candidate();
		Eigen::VectorXd bestparam = params.get_gp().pheno(cand.get_x_dvec());
		std::cout << "Iteration " << std::setprecision(8) << cmasols.niter() << ", " << cmasols.elapsed_last_iter() << "ms"
		          << ", fitness=" << cand.get_fvalue()
			  << ", sigma=" << cmasols.sigma()
			  << ", parameters (";
		for(size_t i=0; i<3; ++i) {
			std::cout << " " << bestparam[i];
		}
		for(size_t i=3; i<cand.get_x_size(); ++i) {
			std::cout << " " << std::fixed << std::setprecision(2) << 180*bestparam[i]/3.1415;
		}
		std::cout << " )" << std::endl;
		path_file << cand.get_fvalue() << "\t"
		          << bestparam[0] << "\t"
		          << bestparam[1] << "\t"
		          << bestparam[2] << "\t"
		          << bestparam[3] << "\t"
		          << bestparam[4] << "\t"
		          << bestparam[5] << "\t"
			  << cmasols.sigma() << "\n" << std::flush;
		return 0;
	};
	auto cmaparams = getParametersFromConfig();
	std::cout << "Samples per Generation lambda = " << cmaparams.lambda() << std::endl;
	CMASolutions cmasols = cmaes<GenoPheno<pwqBoundStrategy>>(chi2, cmaparams, select_time);
	std::cout << "best solution: " << cmasols << std::endl;
	std::cout << "optimization took: " << cmasols.elapsed_time() / 1000.0 << " seconds" << std::endl;
	std::cout << "status: " << cmasols.run_status() << std::endl;
	Candidate cand = cmasols.get_best_seen_candidate();
	Eigen::VectorXd bestparam = cmaparams.get_gp().pheno(cand.get_x_dvec());
	std::ofstream of(getFilename(".align"));
	of << bestparam(0) << " "
	   << bestparam(1) << " "
	   << bestparam(2) << " "
	   << cand.get_fvalue() << " "
	   << bestparam(3) << " "
	   << bestparam(4) << " "
	   << bestparam(5) << "\n"
	   << "# best solution: " << cmasols << "\n"
	   << "# status: " << cmasols.run_status() << "\n";
	of.flush();
	of.close();

	std::ofstream statusfile(getFilename("_status.csv"));
	statusfile << cmasols.run_status() << std::endl;
	statusfile.close();

	/*std::ofstream df(getFilename("_solution_zscan.csv"));
	df << "# x=" << cand.get_x()[2] << ", chi2=" << cand.get_fvalue() << "\n";
	for(size_t i=0; i < initial_parameters.size(); ++i) {
		initial_parameters[i] = cand.get_x()[2];
	}
	for(double z = 0; z < 1000; z += 5.0) {
		initial_parameters[2] = z;
		df << z << " "
		   << chi2(&initial_parameters.front(), initial_parameters.size()) << "\n";
	}
	df.flush();
	df.close();*/

	_mpaTransform.setOffset(bestparam.head<3>());
	_mpaTransform.setRotation(bestparam.tail<3>());
	_aligner.initHistograms("test_x", "test_y");
	for(const auto& evt: _eventCache) {
		// auto b = track.extrapolateOnPlane(0, 5, cand.get_x()[2], 2);
		auto b = _mpaTransform.mpaPlaneTrackIntersect(evt.track, 0, 5);
		auto a = _mpaTransform.transform(evt.mpa_index);
		_aligner.Fill(b(0) - a(0), b(1) - a(1));
	}
	_aligner.calculateAlignment();
	_aligner.writeHistograms();
	_aligner.writeHistogramImage(getFilename("_aligntest.png"));

	_file->Write();

	std::ofstream statusFile(getFilename(".status"), std::ios_base::app);
	statusFile << getRerunNumber() << "\t" << cmasols.run_status() << std::endl;
	auto acceptable = std::find(_allowedExitStatus.begin(), _allowedExitStatus.end(), cmasols.run_status());
	if(acceptable == _allowedExitStatus.end() && _forceStatus) {
		if(getRerunNumber() < _maxForceStatusRuns) {
			rerun();
		} else {
			std::cerr << "Did not find acceptable solution in required time." << std::endl;
			statusFile << "# not acceptable!" << std::endl;
		}
	} else if(_forceStatus){
		std::cerr << "Found acceptable solution after " << getRerunNumber() << " optimization candidates." << std::endl;
		statusFile << "# acceptable!" << std::endl;
	}
}

libcmaes::CMAParameters<GenoPheno<pwqBoundStrategy>> MpaCmaesAlign::getParametersFromConfig() const
{
	static const int dim = 6;
	auto low = _config.getVector<double>("cmaes_parameters_low");
	auto init = _config.getVector<double>("cmaes_parameters_init");
	auto high = _config.getVector<double>("cmaes_parameters_high");
	auto elitism = _config.get<int>("cmaes_elitism");
	if(low.size() != dim) {
		throw std::out_of_range("Config variable 'cmaes_parameters_low' must define exactly 6 entries!");
	}
	if(init.size() != dim) {
		throw std::out_of_range("Config variable 'cmaes_parameters_init' must define exactly 6 entries!");
	}
	if(high.size() != dim) {
		throw std::out_of_range("Config variable 'cmaes_parameters_high' must define exactly 6 entries!");
	}
	for(size_t i=0; i < dim; ++i) {
		if(init[i] < low[i] || init[i] > high[i]) {
			std::cerr << "The initial value " << init[i] << "of parameter " << i << " must be in range ["
			          << low[i] << ", " << high[i] << "]!" << std::endl;
			throw std::out_of_range("Initial CMAES parameter out of bounds!");
		}
	}
	double sigma = 2.0;
	int lambda = -1;
	try {
		sigma = _config.get<double>("cmaes_initial_sigma");
	} catch(core::CfgParse::no_variable_error) {
		std::cout << "Using default sigma_0 = " << sigma << std::endl;
	}
	try {
		lambda = _config.get<int>("cmaes_lambda");
	} catch(core::CfgParse::no_variable_error) {
	}
	GenoPheno<pwqBoundStrategy> gp(&low.front(), &high.front(), dim);
	CMAParameters<GenoPheno<pwqBoundStrategy>> cmaparams(init, sigma, lambda, 0, gp);
	cmaparams.set_max_fevals(100000);
	cmaparams.set_ftarget(0.1);
	cmaparams.set_fplot(getFilename("_cmaes.dat"));
	cmaparams.set_elitism(elitism);
	return cmaparams;
}
