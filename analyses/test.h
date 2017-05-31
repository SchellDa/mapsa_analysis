
#ifndef TEST_H
#define TEST_H

#include "trackanalysis.h"

class Test : public core::TrackAnalysis
{
public:
        Test();
	virtual ~Test();

	virtual void init(const po::variables_map& vm);
	virtual std::string getUsage(const std::string& argv0) const;
	virtual std::string getHelp(const std::string& argv0) const;

private:
        bool analyze(const core::TrackStreamReader::event_t& track_event,
	             const core::BaseSensorStreamReader::event_t& mpa_event);
};

#endif//TEST_H
