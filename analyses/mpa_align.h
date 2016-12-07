
#ifndef MPA_ALIGN_H
#define MPA_ALIGN_H

#include "analysis.h"
#include "aligner.h"
#include <TH1D.h>
#include <TCanvas.h>
#include <TFile.h>

class MpaAlign : public core::Analysis
{
public:
        MpaAlign();
	virtual ~MpaAlign();

	virtual void init(const po::variables_map& vm);
	virtual std::string getUsage(const std::string& argv0) const;
	virtual std::string getHelp(const std::string& argv0) const;

private:
	void scanInit();
        bool scanRun(const core::TrackStreamReader::event_t& track_event,
	             const core::BaseSensorStreamReader::event_t& mpa_event);
	void scanFinish();

	void scanFineInit();
	void scanFineFinish();

	struct alignment_t {
		Eigen::Vector3d position;
		double x_sigma;
		double y_width;
	};

	core::Aligner _aligner;
	TFile* _file;
	TCanvas* _xCanvas;
	TCanvas* _yCanvas;
	std::vector<alignment_t> _alignments;
	double _lowZ;
	double _highZ;
	double _currentZ;
	int _currentScanStep;
	int _numSteps;
	int _numProcessedSamples;
	int _sampleSize;
	bool _twoPass;
	Eigen::Vector2d _currentSigmaMinimum;
};

#endif//MPA_ALIGN_H
