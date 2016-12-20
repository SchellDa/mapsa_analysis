
#ifndef MPA_MINUIT_ALIGN_H
#define MPA_MINUIT_ALIGN_H

#include "analysis.h"
#include "aligner.h"
#include <TH1D.h>
#include <TCanvas.h>
#include <TFile.h>
#include <fstream>

class MpaMinuitAlign : public core::Analysis
{
public:
        MpaMinuitAlign();
	virtual ~MpaMinuitAlign();

	virtual void init(const po::variables_map& vm);
	virtual std::string getUsage(const std::string& argv0) const;
	virtual std::string getHelp(const std::string& argv0) const;

private:
	void scanInit();
        bool scanRun(const core::TrackStreamReader::event_t& track_event,
	             const core::BaseSensorStreamReader::event_t& mpa_event);
	void scanFinish();

	double chi2(const double* param);

	struct alignment_t {
		Eigen::Vector3d position;
		double x_sigma;
		double y_width;
	};
	struct event_t {
		core::Track track;
		size_t mpa_index;
	};
	std::vector<event_t> _eventCache;

	TFile* _file;
	core::Aligner _aligner;
	size_t _sampleSize;
	std::ofstream _spaceFile;
};

#endif//MPA_MINUIT_ALIGN_H
