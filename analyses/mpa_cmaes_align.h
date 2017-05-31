
#ifndef MPA_CMAES_ALIGN_H
#define MPA_CMAES_ALIGN_H

#include "trackanalysis.h"
#include "aligner.h"
#include <TH1D.h>
#include <TCanvas.h>
#include <TFile.h>
#include <cmaes.h>

class MpaCmaesAlign : public core::TracTrackkAnalysis
{
public:
        MpaCmaesAlign();
	virtual ~MpaCmaesAlign();

	virtual void init(const po::variables_map& vm);
	virtual std::string getUsage(const std::string& argv0) const;
	virtual std::string getHelp(const std::string& argv0) const;

private:
	void scanInit();
        bool scanRun(const core::TrackStreamReader::event_t& track_event,
	             const core::BaseSensorStreamReader::event_t& mpa_event);
	void scanFinish();

	libcmaes::CMAParameters<libcmaes::GenoPheno<libcmaes::pwqBoundStrategy>> getParametersFromConfig() const;
	
	double modelChi2(const double* param, const int N, std::ofstream* funcfile=nullptr);
	double modelEfficiency(const double* param, const int N, std::ofstream* funcfile=nullptr);

	struct alignment_t {
		Eigen::Vector3d position;
		double x_sigma;
		double y_width;
	};
	struct event_t {
		core::Track track;
		int mpa_index;
	};
	std::vector<event_t> _eventCache;
	bool _cacheFull;

	TFile* _file;
	core::Aligner _aligner;
	size_t _sampleSize;
	std::vector<int> _allowedExitStatus;
	bool _forceStatus;
	bool _initFromAlignment;
	size_t _maxForceStatusRuns;

	bool _writeCache;
	bool _writeFunction;
	bool _modelEfficiency;
	double _nSigma;
	std::vector<bool> _pixelMask;
};

#endif//MPA_CMAES_ALIGN_H
