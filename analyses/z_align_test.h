
#ifndef Z_ALIGN_TEST_H
#define Z_ALIGN_TEST_H

#include "analysis.h"
#include "aligner.h"
#include <TH1D.h>
#include <TCanvas.h>

class ZAlignTest : public core::Analysis
{
public:
        ZAlignTest();
	virtual ~ZAlignTest();

	virtual void init(const po::variables_map& vm);
	virtual std::string getUsage(const std::string& argv0) const;
	virtual std::string getHelp(const std::string& argv0) const;

private:
	void scanInit();
        bool scanRun(const core::TrackStreamReader::event_t& track_event,
	             const core::BaseSensorStreamReader::event_t& mpa_event);
	void scanFinish();

	core::Aligner _aligner;
	TCanvas* _xCanvas;
	TCanvas* _yCanvas;
	double _lowZ;
	double _highZ;
	double _currentZ;
	int _currentScanStep;
	int _numSteps;
	int _numProcessedSamples;
	int _sampleSize;
};

#endif//Z_ALIGN_TEST_H
