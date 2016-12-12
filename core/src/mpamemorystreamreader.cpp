
#include "mpamemorystreamreader.h"
#include <cassert>
#include <regex.h>
#include <bitset>
#include <iostream>

using namespace core;

MpaMemoryStreamReader::mpareader::mpareader(const std::string& filename, size_t seek)
 : reader(filename), _fin(), _numEventsRead(0)
{
	open(seek);
	if(seek == 0) {
		next();
	}
}

MpaMemoryStreamReader::mpareader::~mpareader()
{
	_fin.close();
}

bool MpaMemoryStreamReader::mpareader::next()
{
	// last read reached EOF, so we are an end-iterator now
	if(!_fin.good()) {
		return true;
	}
	std::string line;
	// try to read only non-empty lines
	while(std::getline(_fin, line)) {
		if(line == "\r" || line == "")
			continue;
		break;
	}
	// empty line(s) at end of file -> reached end!
	if(!_fin.good()) {
		return true;
	}

	_currentEvent.data.clear();
	_currentEvent.data.resize(48, 0);
	_currentEvent.bunchCrossing.clear();
	_currentEvent.eventNumber = _numEventsRead++;

	regex_t regex;
	// possible optimization: do not recompile regex all the time...
	int ret = regcomp(&regex, "'1{8}([01]{16})([01]{48})'", REG_EXTENDED);
	size_t offset = 0;
	regmatch_t m[3];
	while((ret = regexec(&regex, line.c_str()+offset, 3, m, 0)) == 0) {
		auto bxId = line.substr(offset+m[1].rm_so, m[1].rm_eo-m[1].rm_so);
		auto pixelmap = line.substr(offset+m[2].rm_so, m[2].rm_eo - m[2].rm_so);
		assert(bxId.size() == 16);
		assert(pixelmap.size() == 48);
		_currentEvent.bunchCrossing.push_back(std::stoi(bxId, nullptr, 2));
		for(size_t i = 0; i < pixelmap.size(); ++i) {
			int new_idx = i;
			if(i >= 16 && i <= 31) {
				new_idx = 47 - i;
			}
			if(pixelmap[i] == '1') {
				_currentEvent.data[new_idx] = 1;
			}
		}
		if(offset >= line.length()) {
			break;
		}
		offset += m[0].rm_eo;
	}
	regfree(&regex);
	return false;
}


void MpaMemoryStreamReader::mpareader::open(size_t seek)
{
	_fin.exceptions(std::ios_base::failbit);
	_fin.open(getFilename());
	_fin.exceptions(std::ios_base::goodbit);
	if(seek) {
		_fin.seekg(seek);
	}
}

BaseSensorStreamReader::reader* MpaMemoryStreamReader::mpareader::clone() const
{
	auto newReader = new mpareader(getFilename(), _fin.tellg());
	newReader->_currentEvent = _currentEvent;
	newReader->_numEventsRead = _numEventsRead;
	return newReader;
}

BaseSensorStreamReader::reader* MpaMemoryStreamReader::getReader(const std::string& filename) const
{
	return new mpareader(filename);
}
