
#include "test.h"
#include <iostream>

REGISTER_ANALYSIS_TYPE(Test, "Textual analysis description here.")

Test::Test() :
 Analysis()
{
	std::cout << "Constructor" << std::endl;
	addProcess("test", CS_ALWAYS, std::bind(&Test::analyze, this, std::placeholders::_1, std::placeholders::_2));
}

Test::~Test()
{
	std::cout << "Destructor" << std::endl;
}

void Test::init(const po::variables_map& vm)
{
	std::cout << "Init" << std::endl;
}

std::string Test::getUsage(const std::string& argv0) const
{
	return Analysis::getUsage(argv0);
}

std::string Test::getHelp(const std::string& argv0) const
{
        return Analysis::getHelp(argv0);
}

bool Test::analyze(const core::TrackStreamReader::event_t& track_event,
                   const core::MPAStreamReader::event_t& mpa_event)
{
	std::cout << "analyze: " << track_event.eventNumber << " " << mpa_event.eventNumber << std::endl;
	return false;
}
