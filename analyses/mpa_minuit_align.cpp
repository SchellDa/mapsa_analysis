
#include "mpa_minuit_align.h"
#include <TImage.h>
#include <TText.h>
#include <TGraph.h>
#include <iomanip>
#include <Math/Functor.h>
#include <Math/Factory.h>
#include <Math/Minimizer.h>

REGISTER_ANALYSIS_TYPE(MpaMinuitAlign, "Perform XYZ and angular alignment of MPA.")

MpaMinuitAlign::MpaMinuitAlign() :
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
	           std::bind(&MpaMinuitAlign::scanInit, this),
		   std::bind(&MpaMinuitAlign::scanRun, this, std::placeholders::_1, std::placeholders::_2),
	           core::Analysis::run_post_callback_t {},
	           std::bind(&MpaMinuitAlign::scanFinish, this)
	           );
}

MpaMinuitAlign::~MpaMinuitAlign()
{
	if(_file) {
		_file->Write();
		delete _file;
	}
}

void MpaMinuitAlign::init(const po::variables_map& vm)
{
	_file = new TFile(getFilename(".root").c_str(), "RECREATE");
	_aligner.initHistograms();
	_sampleSize = vm["sample-size"].as<int>();
	_eventCache.reserve(_sampleSize);
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

std::string MpaMinuitAlign::getUsage(const std::string& argv0) const
{
	return Analysis::getUsage(argv0);
}

std::string MpaMinuitAlign::getHelp(const std::string& argv0) const
{
        return Analysis::getHelp(argv0);
}

void MpaMinuitAlign::scanInit()
{
}

bool MpaMinuitAlign::scanRun(const core::TrackStreamReader::event_t& track_event,
                      const core::BaseSensorStreamReader::event_t& mpa_event)
{
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

void MpaMinuitAlign::scanFinish()
{
	_spaceFile.open(getFilename("_space.csv"));
	ROOT::Math::Functor fctor(this, &MpaMinuitAlign::chi2, 6);
	std::unique_ptr<ROOT::Math::Minimizer> min(ROOT::Math::Factory::CreateMinimizer("Minuit2", "Migrad"));
	min->SetFunction(fctor);
	min->SetMaxFunctionCalls(1000000);
	min->SetMaxIterations(1000);
	min->SetTolerance(0.1);
	min->SetPrintLevel(2);
	min->SetVariable(0, "X", -2.96, 1);
	min->SetVariable(1, "Y", -2.78, 1);
	min->SetVariable(2, "Z", 878, 1);
	min->SetVariable(3, "phi", 0, 0.1);
	min->SetVariable(4, "theta", 0, 0.1);
	min->SetVariable(5, "omega", 0, 0.1);
//	min->SetVariable(0, "X", 0.0, 5);
//	min->SetVariable(1, "Y", 0.0, 5);
//	min->SetVariable(2, "Z", 860, 10);
//	min->SetVariable(3, "phi", 0, 0.1);
//	min->SetVariable(4, "theta", 0, 0.1);
//	min->SetVariable(5, "omega", 0, 0.1);
	if(!min->Minimize()) {
		std::cout << "Minimization failed!" << std::endl;
		return;
	}
	auto par = min->X();
	Eigen::VectorXd bestparam(6);
	bestparam << par[0], par[1], par[2], par[3], par[4], par[5];

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
}

/*libcmaes::CMAParameters<GenoPheno<pwqBoundStrategy>> MpaMinuitAlign::getParametersFromConfig() const
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
			throw std::out_of_range("Initial MINUIT parameter out of bounds!");
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
}*/

double MpaMinuitAlign::chi2(const double* param)
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
		}
	}
	double fitness = 1000;
	if(num_entries > total_entries/100) {
		fitness = chi2val / num_entries;
	}
	// fitness = chi2val;
	_spaceFile << fitness << "\t"
	           << param[0] << "\t"
	           << param[1] << "\t"
	           << param[2] << "\t"
	           << param[3] << "\t"
	           << param[4] << "\t"
	           << param[5] << "\t"
		   << num_entries << "\n";
	return fitness;
};
