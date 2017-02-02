
#include "mpa_cmaes_align.h"
#include <TImage.h>
#include <TText.h>
#include <TGraph.h>
#include <cmaes.h>
#include <iomanip>
#include <cstdio>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

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
		("multirun-paths,M", po::value<std::string>(), "Put results in directory specialized for multiple alignment execution. Useful for analyzing the alignment with further postprocessing.")
		("write-cache,C", "If set, up to 1000 hits from the cache are written to disk. Useful for debugging.")
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
	if(vm.count("multirun-paths") > 0) {
		auto path_prefix = vm["multirun-paths"].as<std::string>();
		auto output_dir = _config.getVariable("output_dir");
		output_dir += "/MpaCmaesAlign/";
		mkdir(output_dir.c_str(), S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
		output_dir += path_prefix;
		mkdir(output_dir.c_str(), S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
		output_dir += "/run";
		output_dir += std::to_string(getAllRunIds()[0]);
		int status = mkdir(output_dir.c_str(), S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
		if(status == -1 && errno != EEXIST) {
			std::cerr << "Cannot create output directory" << std::endl;
			throw std::exception();
		}
		_config.setVariable("output_dir", output_dir);
	}

	_file = new TFile(getFilename(".root").c_str(), "RECREATE");
	_aligner.initHistograms();
	_sampleSize = vm["sample-size"].as<int>();
	_writeCache = vm.count("write-cache") > 0;
	_eventCache.reserve(_sampleSize);
	_cacheFull = false;
	_forceStatus = _config.get<int>("cmaes_force_status") > 0;
	_allowedExitStatus = _config.getVector<int>("cmaes_allowed_exit_status");
	_maxForceStatusRuns = _config.get<int>("cmaes_max_force_status_runs");
	std::remove(getFilename(".status").c_str());
	std::ofstream statusFile(getFilename(".status"));
	statusFile << "# Rerun\tStatus" << std::endl;
	std::ofstream configFile(getFilename(".config"));
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
	if(!_cacheFull && _writeCache) {
		std::ofstream fout(getFilename(".cache"));
		fout << "# MPA Index\tax ay az\tbx by bz\n";
		size_t numEventsWritten = 0;
		for(const auto& evt: _eventCache) {
			++numEventsWritten;
			fout << evt.mpa_index << "\t"
			     << evt.track.points[3](0) << "\t"
			     << evt.track.points[3](1) << "\t"
			     << evt.track.points[3](2) << "\t"
			     << evt.track.points[5](0) << "\t"
			     << evt.track.points[5](1) << "\t"
			     << evt.track.points[5](2) << "\n";
			if(numEventsWritten >= 1000) {
				break;
			}
		}
	}
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
			auto b = trans.mpaPlaneTrackIntersect(evt.track, 3, 5);
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
/*	_aligner.initHistograms("test_x", "test_y");
	for(const auto& evt: _eventCache) {
		// auto b = track.extrapolateOnPlane(0, 5, cand.get_x()[2], 2);
		auto b = _mpaTransform.mpaPlaneTrackIntersect(evt.track, 0, 5);
		auto a = _mpaTransform.transform(evt.mpa_index);
		_aligner.Fill(b(0) - a(0), b(1) - a(1));
	}
	_aligner.calculateAlignment();
	_aligner.writeHistograms();
	_aligner.writeHistogramImage(getFilename("_aligntest.png"));*/

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
	int max_iter = -1;
	try {
		sigma = _config.get<double>("cmaes_initial_sigma");
	} catch(core::CfgParse::no_variable_error) {
		std::cout << "Using default sigma_0 = " << sigma << std::endl;
	}
	try {
		lambda = _config.get<int>("cmaes_lambda");
	} catch(core::CfgParse::no_variable_error) {
	}
	max_iter = _config.get<int>("cmaes_max_iterations");
	GenoPheno<pwqBoundStrategy> gp(&low.front(), &high.front(), dim);
	CMAParameters<GenoPheno<pwqBoundStrategy>> cmaparams(init, sigma, lambda, 0, gp);
	cmaparams.set_max_fevals(100000);
	cmaparams.set_max_iter(max_iter);
	cmaparams.set_ftarget(0.1);
	cmaparams.set_fplot(getFilename("_cmaes.dat"));
	cmaparams.set_elitism(elitism);
	
	std::ofstream fout(getFilename(".config"));
	auto run = _runlist.getByMpaRun(getCurrentRunId());
	fout << "int lambda " << lambda << "\n";
	fout << "int elitism " << elitism << "\n";
	fout << "int sampleSize " << _sampleSize << "\n";
	fout << "float sigma0 " << sigma << "\n";
	fout << "float angle " << run.angle << "\n";
	fout << "float bias_voltage " << run.bias_voltage << "\n";
	fout << "float bias_current " << run.bias_current << "\n";
	fout << "float threshold " << run.threshold << "\n";
	fout << "int telescope_run " << run.telescope_run << "\n";

	return cmaparams;
}
