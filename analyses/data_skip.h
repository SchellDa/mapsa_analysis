
#ifndef DATA_SKIP_H
#define DATA_SKIP_H

#include "analysis.h"
#include <TFile.h>
#include <TH1D.h>
#include <TCanvas.h>

class DataSkip : public core::Analysis
{
public:
        DataSkip();
	virtual ~DataSkip();

	virtual void init(const po::variables_map& vm);
	virtual std::string getUsage(const std::string& argv0) const;
	virtual std::string getHelp(const std::string& argv0) const;

private:
        bool analyze(const core::TrackStreamReader::event_t& track_event,
	             const core::MPAStreamReader::event_t& mpa_event);
	void finish();
	TFile* _file;
	TH1D* _currentHist;
	TCanvas* _canvas;
	int _numBins;
	int _eventsPerRun;
	int _range;
};

#endif//DATA_SKIP_H
