
#ifndef TEST_H
#define TEST_H

#include "analysis.h"

class Test : public core::Analysis
{
public:
        Test();
	virtual ~Test();

	virtual void init(const po::variables_map& vm);
	virtual std::string getUsage(const std::string& argv0) const;
	virtual std::string getHelp(const std::string& argv0) const;

private:
        void analyze(const core::TrackStreamReader::event_t& track_event,
	             const core::MPAStreamReader::event_t& mpa_event);
};

#endif//TEST_H
