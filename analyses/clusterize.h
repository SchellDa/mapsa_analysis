
#ifndef CLUSTERIZE_H
#define CLUSTERIZE_H

#include "analysis.h"
#include "aligner.h"
#include <fstream>
#include "eigenhash.h"
#include <unordered_map>
#include <TFile.h>
#include <TH1F.h>

class Clusterize : public core::Analysis
{
public:
	struct cluster_t {
		Eigen::Vector2d center;
		std::unordered_map<Eigen::Vector2i, int> points;
	};
	struct event_t {
		int eventNumber;
		std::vector<cluster_t> clusters;
	};
        Clusterize();
	virtual ~Clusterize();

	virtual void init(const po::variables_map& vm);
	virtual std::string getUsage(const std::string& argv0) const;
	virtual std::string getHelp(const std::string& argv0) const;

private:
        bool clusterize(const core::TrackStreamReader::event_t& track_event,
	             const core::BaseSensorStreamReader::event_t& mpa_event);
	void finishClusterize();
        bool align(const core::TrackStreamReader::event_t& track_event,
	             const core::BaseSensorStreamReader::event_t& mpa_event);
	void finishAlign();
        bool cutClusterSize(const core::TrackStreamReader::event_t& track_event,
	             const core::BaseSensorStreamReader::event_t& mpa_event);
	void finishCutClusterSize();

	std::unordered_map<Eigen::Vector2i, int> getCluster(const std::unordered_map<Eigen::Vector2i, int>& pixels) const;
	TFile* _file;
	core::Aligner _aligner;
	TH1F* _clusterSizeHist;
	TH1F* _cutClusterSizeHist;
	std::ofstream _clusterFile;
	std::map<int, event_t> _eventData;
};

#endif//CLUSTERIZE_H
