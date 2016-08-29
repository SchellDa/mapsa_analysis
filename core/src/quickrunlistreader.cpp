
#include "quickrunlistreader.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <boost/algorithm/string.hpp>

using namespace core;

void QuickRunlistReader::read(const std::string& filename)
{
	std::ifstream fin(filename);
	std::string line;
	int line_no = 0;
	while(getline(fin, line)) {
		++line_no;
		if(line_no == 1)
			continue;
		boost::trim(line);
		if(!line.size())
			continue;
		if(line[0] == '#')
			continue;
		int colno = 0;
		std::istringstream sstr(line);
		std::string col;
		run_t run;
		while(getline(sstr, col, '\t')) {
			try {
				if(colno == 0) run.mpa_run = std::stoi(col);
				else if(colno == 1) run.telescope_run = std::stoi(col);
				else if(colno == 2) run.angle = std::stod(col);
				else if(colno == 10) run.data_offset = std::stoi(col);
			} catch(std::invalid_argument& e) {
			}
			colno++;
		}
		_runs.push_back(run);
	}
}

int QuickRunlistReader::getMpaRunByTelRun(int telRun) const
{
	for(const auto& run: _runs) {
		if(run.telescope_run == telRun) {
			return run.mpa_run;
		}
	}
	throw std::invalid_argument("Telescope Run ID not found");
	return 0;
}

int QuickRunlistReader::getTelRunByMpaRun(int mpaRun) const
{
	for(const auto& run: _runs) {
		if(run.mpa_run == mpaRun) {
			return run.telescope_run;
		}
	}
	throw std::invalid_argument("MPA Run ID not found");
	return 0;
}
