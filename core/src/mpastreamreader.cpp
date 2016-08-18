
#include "mpastreamreader.h"
#include <cassert>
#include <regex.h>

using namespace core;


MPAStreamReader::EventIterator::EventIterator(const std::string& filename, bool end)
 : _fin(), _filename(filename), _end(end), _numEventsRead(0), _currentEvent()
{
	if(!_end) {
		open();
		++(*this);
	}
}

MPAStreamReader::EventIterator::EventIterator(const EventIterator& other)
 : _fin(), _filename(other._filename), _end(other._end), _numEventsRead(other._numEventsRead),
   _currentEvent(other._currentEvent)
{
	if(_end) {
		_numEventsRead = other._numEventsRead;
	} else {
		open();
		_fin.seekg(other._fin.tellg());
	}
}

MPAStreamReader::EventIterator::EventIterator(EventIterator&& other) noexcept :
#ifdef NO_IOSTREAM_MOVE
 _fin(),
#else
 _fin(std::move(other._fin)),
#endif
 _filename(other._filename), _end(other._end), _numEventsRead(other._numEventsRead),
 _currentEvent{other._currentEvent.eventNumber, std::move(other._currentEvent.data)}
{
#ifdef NO_IOSTREAM_MOVE
	open();
	_fin.seekg(other._fin.tellg());
#endif
}

MPAStreamReader::EventIterator::~EventIterator()
{
}

MPAStreamReader::EventIterator& MPAStreamReader::EventIterator::operator=(const EventIterator& other)
{
	EventIterator tmp(other);
	*this = std::move(tmp);
	return *this;
}

MPAStreamReader::EventIterator& MPAStreamReader::EventIterator::operator=(EventIterator&& other) noexcept
{
	_fin.close();
#ifdef NO_IOSTREAM_MOVE
	open();
	_fin.seekg(other._fin.tellg());
#else
	_fin = std::move(other._fin);
#endif
	_filename = other._filename;
	_numEventsRead = other._numEventsRead;
	_end = other._end;
	_currentEvent = std::move(other._currentEvent);
	return *this;
}

bool MPAStreamReader::EventIterator::operator==(const EventIterator& other) const
{
	// iterators are always equal when they are end iterator, otherwise
	// event numbers need to match.
	return _end == other._end && _filename == other._filename &&
		(_end || _numEventsRead == other._numEventsRead);
}

bool MPAStreamReader::EventIterator::operator!=(const EventIterator& other) const
{
	return !(*this == other);
}

MPAStreamReader::EventIterator& MPAStreamReader::EventIterator::operator++()
{
	// last read reached EOF, so we are an end-iterator now
	if(!_fin.good()) {
		_end = true;
		return *this;
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
		_end = true;
		return *this;
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
	return *this;
}

MPAStreamReader::EventIterator MPAStreamReader::EventIterator::operator++(int)
{
	EventIterator old(*this);
	++(*this);
	return old;
}

void MPAStreamReader::EventIterator::open()
{
	_fin.exceptions(std::ios_base::failbit);
	_fin.open(_filename);
	_fin.exceptions(std::ios_base::goodbit);
}

MPAStreamReader::MPAStreamReader(const std::string& filename)
 : _filename(filename)
{
}

MPAStreamReader::EventIterator MPAStreamReader::begin() const
{
	return EventIterator(_filename, false);
}

MPAStreamReader::EventIterator MPAStreamReader::end() const
{
	return EventIterator(_filename, true);
}
