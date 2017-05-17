
#ifndef MPA_EFFICIENCY_H
#define MPA_EFFICIENCY_H

#include "analysis.h"
#include "aligner.h"
#include <TFile.h>
#include <TH1D.h>
#include <TH2D.h>
#include <TGraph2D.h>
#include <fstream>

class MpaEfficiency : public core::Analysis
{
public:
        MpaEfficiency();
	virtual ~MpaEfficiency();

	virtual void init(const po::variables_map& vm);
	virtual std::string getUsage(const std::string& argv0) const;
	virtual std::string getHelp(const std::string& argv0) const;

private:
	void analyzeRunInit();
        bool analyze(const core::TrackStreamReader::event_t& track_event,
	             const core::BaseSensorStreamReader::event_t& mpa_event);
	void analyzeFinish();

	TFile* _file;
	core::Aligner _aligner;
	TH2D* _correlated;
	TH2D* _correlatedOverlayed;
	TH2D* _total;
	TH2D* _totalOverlayed;
	TH2D* _fake;
	TH1D* _hitsPerEvent;
	TH1D* _hitsPerEventWithTrack;
	TH1D* _hitsPerEventWithTrackMasked;
	TH2D* _shitTracks;
	TH1D* _correlationDistance;
	TH1D* _fiducialResidual;
	TH1D* _bunchCrossingId;
	std::vector<bool> _pixelMask;
	double _nSigma;
	size_t _totalCount;
	size_t _correlatedCount;
	size_t _fakeCount;
	size_t _totalMpaCount;
	bool _singularEventAnalysis;
	bool _antiSingularEventAnalysis;
	bool _inactiveMask;
	std::string _alignType;
};

#endif//MPA_EFFICIENCY_H
