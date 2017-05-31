
#ifndef STRIP_ALIGN_H
#define STRIP_ALIGN_H

#include "trackanalysis.h"
#include "aligner.h"
#include <TH1D.h>
#include <TH2D.h>
#include <TCanvas.h>
#include <TFile.h>
#include <fstream>

class StripAlign : public core::TrackAnalysis
{
public:
        StripAlign();
	virtual ~StripAlign();

	virtual void init(const po::variables_map& vm);
	virtual std::string getUsage(const std::string& argv0) const;
	virtual std::string getHelp(const std::string& argv0) const;

private:
	struct alignment_t {
		Eigen::Vector3d position;
		double sigma;
	};

	void scanInit();
        bool scanRun(const core::TrackStreamReader::event_t& track_event,
	             const core::BaseSensorStreamReader::event_t& mpa_event);
	void scanFinish();

	void yAlignInit();
        bool yAlignRun(const core::TrackStreamReader::event_t& track_event,
	               const core::BaseSensorStreamReader::event_t& mpa_event);
	void yAlignFinish();

	void writeAlignment(const alignment_t& align) const;

	TFile* _file;
	TCanvas* _canvas;
	TCanvas* _canvasX;
	TCanvas* _canvasY;
	TH1D* _corHist;
	TH2D* _corX;
	TH2D* _corY;
	std::vector<alignment_t> _alignments;
	alignment_t _zAlignment;
	double _lowZ;
	double _highZ;
	double _currentZ;
	double _lowY;
	double _highY;
	double _currentY;
	int _lowShift;
	int _highShift;
	bool _shift;
	bool _reuseZ;
	bool _noY;
	int _currentShift;
	int _currentScanStep;
	int _numSteps;
	int _numProcessedSamples;
	int _sampleSize;
	bool _noisyAsFuckMode;
	std::map<double, int> _yHitcounter;
	std::ofstream _out;
	Eigen::Vector2d _currentSigmaMinimum;
};

#endif//STRIP_ALIGN_H
