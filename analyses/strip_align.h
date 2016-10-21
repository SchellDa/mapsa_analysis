
#ifndef STRIP_ALIGN_H
#define STRIP_ALIGN_H

#include "analysis.h"
#include "aligner.h"
#include <TH1D.h>
#include <TCanvas.h>
#include <TFile.h>
#include <fstream>

class StripAlign : public core::Analysis
{
public:
        StripAlign();
	virtual ~StripAlign();

	virtual void init(const po::variables_map& vm);
	virtual std::string getUsage(const std::string& argv0) const;
	virtual std::string getHelp(const std::string& argv0) const;

private:
	void scanInit();
        bool scanRun(const core::TrackStreamReader::event_t& track_event,
	             const core::BaseSensorStreamReader::event_t& mpa_event);
	void scanFinish();

	struct alignment_t {
		Eigen::Vector3d position;
		double sigma;
	};

	TFile* _file;
	TCanvas* _canvas;
	TH1D* _corHist;
	std::vector<alignment_t> _alignments;
	double _lowZ;
	double _highZ;
	double _currentZ;
	int _currentScanStep;
	int _numSteps;
	int _numProcessedSamples;
	int _sampleSize;
	std::ofstream _out;
	Eigen::Vector2d _currentSigmaMinimum;
};

#endif//STRIP_ALIGN_H
