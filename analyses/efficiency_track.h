
#ifndef EFFICIENCY_TRACK_H
#define EFFICIENCY_TRACK_H

#include "analysis.h"
#include "aligner.h"
#include <TFile.h>
#include <TH1D.h>
#include <TH2D.h>
#include <TGraph2D.h>
#include <fstream>

class EfficiencyTrack : public core::Analysis
{
public:
        EfficiencyTrack();
	virtual ~EfficiencyTrack();

	virtual void init(const po::variables_map& vm);
	virtual std::string getUsage(const std::string& argv0) const;
	virtual std::string getHelp(const std::string& argv0) const;

private:
        bool prealignRun(const core::TrackStreamReader::event_t& track_event,
	              const core::BaseSensorStreamReader::event_t& mpa_event);
	void prealignFinish();
	void alignInit();
        bool alignRun(const core::TrackStreamReader::event_t& track_event,
	           const core::BaseSensorStreamReader::event_t& mpa_event);
	void alignFinish();
        bool checkCorrelatedHits(const core::TrackStreamReader::event_t& track_event,
	           const core::BaseSensorStreamReader::event_t& mpa_event);
	void analyzeRunInit();
        bool analyze(const core::TrackStreamReader::event_t& track_event,
	             const core::BaseSensorStreamReader::event_t& mpa_event);
	void analyzeFinish();

	std::vector<Eigen::Vector3d> _prealignPoints;
	size_t _numPrealigmentPoints;
	TFile* _file;
	bool _forceAlignment;
	std::map<int, core::Aligner> _aligner;
	TH2D* _correlatedHits;
	TH1D* _correlatedHitsX;
	TH1D* _correlatedHitsY;
	TH2D* _efficiency;
	TH2D* _efficiencyOverlayed;
	TH2D* _efficiencyLocal;
	TH2D* _trackHits;
	TH2D* _trackHitsOverlayed;
	TH2D* _directHits;
	TH2D* _neighbourHits;
	double _nSigmaCut;
	std::vector<size_t> _totalPixelHits;
	std::vector<size_t> _activatedPixelHits;
	std::ofstream _alignFile;
	std::ofstream _analysisHitFile;
	size_t _totalHitCount;
	size_t _correlatedHitCount;
};

#endif//EFFICIENCY_TRACK_H
