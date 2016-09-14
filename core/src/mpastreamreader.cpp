
#include "mpastreamreader.h"
#include <cassert>
#include <regex.h>

using namespace core;

namespace core {
REGISTER_PIXEL_STREAM_READER_TYPE(MPAStreamReader)
}

MPAStreamReader::mpareader::mpareader(const std::string& filename, size_t seek)
 : reader(filename), _fin(), _numEventsRead(0)
{
	open(seek);
	if(seek == 0) {
		next();
	}
}

MPAStreamReader::mpareader::~mpareader()
{
	_fin.close();
}

bool MPAStreamReader::mpareader::next()
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
	_currentEvent.eventNumber = _numEventsRead++;

	regex_t regex;
	// possible optimization: do not recompile regex all the time...
	int ret = regcomp(&regex, "[0-9]+", REG_EXTENDED);
	size_t offset = 0;
	regmatch_t m[1];
	while((ret = regexec(&regex, line.c_str()+offset, 1, m, 0)) == 0) {
		auto counter = std::stoi(line.substr(offset+m[0].rm_so, m[0].rm_eo - m[0].rm_so));
		_currentEvent.data.push_back(counter);
		if(offset >= line.length()) {
			break;
		}
		offset += m[0].rm_eo;
	}
	regfree(&regex);
	return false;
}


void MPAStreamReader::mpareader::open(size_t seek)
{
	_fin.exceptions(std::ios_base::failbit);
	_fin.open(getFilename());
	_fin.exceptions(std::ios_base::goodbit);
	if(seek) {
		_fin.seekg(seek);
	}
}

BaseSensorStreamReader::reader* MPAStreamReader::mpareader::clone() const
{
	auto newReader = new mpareader(getFilename(), _fin.tellg());
	newReader->_currentEvent = _currentEvent;
	newReader->_numEventsRead = _numEventsRead;
	return newReader;
}

BaseSensorStreamReader::reader* MPAStreamReader::getReader(const std::string& filename) const
{
	return new mpareader(filename);
}
