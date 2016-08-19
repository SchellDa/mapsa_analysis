
#include "trackstreamreader.h"
#include <cassert>
#include <regex.h>
#include <iostream>

using namespace core;

#define REG_SUBSTR(str, match) str.substr(match.rm_so, match.rm_eo - match.rm_so)

TrackStreamReader::EventIterator::EventIterator(const std::string& filename, bool end) :
 _fin(), _filename(filename), _end(end), _currentEvent(), _nextEvent(),_regexCompiled(false), _eventsRead(0),
 _currentLineNo(0)
{
	if(!_end) {
		compileRegex();
		open();
		++(*this);
	}
}

TrackStreamReader::EventIterator::EventIterator(const EventIterator& other)
 : _fin(), _filename(other._filename), _end(other._end), 
   _currentEvent(other._currentEvent), _nextEvent(other._nextEvent), _regexCompiled(false),
   _eventsRead(other._eventsRead), _currentLineNo(other._currentLineNo)
{
	if(!_end) {
		compileRegex();
		open();
		_fin.seekg(other._fin.tellg());
	}
}

TrackStreamReader::EventIterator::EventIterator(EventIterator&& other) noexcept :
#ifdef NO_IOSTREAM_MOVE
 _fin(),
#else
 _fin(std::move(other._fin)),
#endif
 _filename(other._filename), _end(other._end),
 _currentEvent(std::move(other._currentEvent)),
 _nextEvent(std::move(other._nextEvent)),
 _regexCompiled(other._regexCompiled), _regexLine(other._regexLine), _regexComment(other._regexComment),
 _eventsRead(other._eventsRead), _currentLineNo(other._currentLineNo)
{
#ifdef NO_IOSTREAM_MOVE
	open();
	_fin.seekg(other._fin.tellg());
	other._regexCompiled = false; // steal ownership of compiled regexes
#endif
}

TrackStreamReader::EventIterator::~EventIterator()
{
	if(_regexCompiled) {
		regfree(&_regexLine);
		regfree(&_regexComment);
	}
	_fin.close();
}

TrackStreamReader::EventIterator& TrackStreamReader::EventIterator::operator=(const EventIterator& other)
{
	EventIterator tmp(other);
	*this = std::move(tmp);
	return *this;
}

TrackStreamReader::EventIterator& TrackStreamReader::EventIterator::operator=(EventIterator&& other) noexcept
{
	_fin.close();
#ifdef NO_IOSTREAM_MOVE
	open();
	_fin.seekg(other._fin.tellg());
#else
	_fin = std::move(other._fin);
#endif
	_filename = other._filename;
	_end = other._end;
	_currentEvent = std::move(other._currentEvent);
	_nextEvent = std::move(other._nextEvent);
	_regexCompiled = other._regexCompiled;
	_regexLine = other._regexLine;
	_regexComment = other._regexLine;
	_eventsRead = other._eventsRead;
	_currentLineNo = other._currentLineNo;
	other._regexCompiled = false; // steal regex ownership
	return *this;
}

bool TrackStreamReader::EventIterator::operator==(const EventIterator& other) const
{
	// iterators are always equal when they are end iterator, otherwise
	// event numbers need to match.
	return _end == other._end && _filename == other._filename &&
		(_end || getEventNumber() == other.getEventNumber());
}

bool TrackStreamReader::EventIterator::operator!=(const EventIterator& other) const
{
	return !(*this == other);
}

TrackStreamReader::EventIterator& TrackStreamReader::EventIterator::operator++()
{
	assert(_regexCompiled);
	// last read reached EOF, so we are an end-iterator now
	if(!_fin.good()) {
		_end = true;
		return *this;
	}
	std::string line;

	// we might have a single point from the next event from the previous event read. This is because we
	// use a change in the  event number as abortion criterion, which makes ist neccessary to read
	// "future" data.
	_currentEvent.eventNumber = _nextEvent.eventNumber;
	_currentEvent.runID = _nextEvent.runID;
	track_t cur_track;
	if(_nextEvent.tracks.size()) {
		cur_track = _nextEvent.tracks[0];
	}
	_currentEvent.tracks.clear();
	_nextEvent.tracks.clear();

	bool first_event = _eventsRead == 0;
	bool last_line_parsed = false;

	regmatch_t m[10];
	// read until event number changes
	while(true) {
		int num_empty_lines = 0;
		// read until non empty line is found
		while(std::getline(_fin, line)) {
			_currentLineNo++;
			if(line == "\r" || line == "")
				num_empty_lines++;
			else
				break;
		}
		// beginning of new block or file ended, save current track into event
		if((num_empty_lines >= 2 || (!_fin.good() && last_line_parsed)) &&
		   cur_track.points.size()) {
			_currentEvent.tracks.push_back(cur_track);
			cur_track.sensorIDs.clear();
			cur_track.points.clear();
			cur_track.sensorIDs.clear();
		}
		// EOF! Do not convert to beyond-last-element iterator if we have a non-empty _currentEvent
		if(!_fin.good() && last_line_parsed) {
			if(_currentEvent.tracks.size() > 0) {
				_eventsRead++;
			} else {
				_end = true;
			}
			break;
		}
		if(!_fin.good()) {
			last_line_parsed = true;
		}
		// skip empty lines, might happen if more than two block separator lines are used
		if(line == "" || line == "\r") {
			continue;
		}
/*		if(num_empty_lines >= 2) {
			continue;
		}*/
		// Is line comment? Ignore
		if(regexec(&_regexComment, line.c_str(), 10, m, 0) != REG_NOMATCH) {
			continue;
		}
		// Try to match data regex
		if(regexec(&_regexLine, line.c_str(), 10, m, 0) == REG_NOMATCH) {
			throw parse_error(_filename, _currentLineNo, -1, "Line does not match regular expression");
		}
		double x, y, z;
		int sensorID, eventNumber, runID;
		try { x = std::stod(REG_SUBSTR(line, m[1])); }
		catch(std::logic_error e) { throw parse_error(_filename, _currentLineNo, m[1].rm_so, e.what()); }
		try { y = std::stod(REG_SUBSTR(line, m[2])); }
		catch(std::logic_error e) { throw parse_error(_filename, _currentLineNo, m[2].rm_so, e.what()); }
		try { z = std::stod(REG_SUBSTR(line, m[3])); }
		catch(std::logic_error e) { throw parse_error(_filename, _currentLineNo, m[3].rm_so, e.what()); }
		try { sensorID = std::stoi(REG_SUBSTR(line, m[4])); }
		catch(std::logic_error e) { throw parse_error(_filename, _currentLineNo, m[4].rm_so, e.what()); }
		try { eventNumber = std::stoi(REG_SUBSTR(line, m[5])); }
		catch(std::logic_error e) { throw parse_error(_filename, _currentLineNo, m[5].rm_so, e.what()); }
		try { runID = std::stoi(REG_SUBSTR(line, m[6])); }
		catch(std::logic_error e) { throw parse_error(_filename, _currentLineNo, m[6].rm_so, e.what()); }
		Eigen::Vector3d pos(x, y, z);
		if(first_event) {
			_currentEvent.eventNumber = eventNumber;
			_currentEvent.runID = runID;
			first_event = false;
		}
		if(runID != _currentEvent.runID) {
			throw consistency_error(_filename, _currentLineNo, 
				"Run ID must not change in a single track block!");
		}

		// Does event number change? Finish reading of current event
		if(eventNumber != _currentEvent.eventNumber) {
			// Changing the event-number while constructing a track is
			// considered a file format error
			if(cur_track.points.size()) {
				throw consistency_error(_filename, _currentLineNo, 
					"Event number must not change in a single track block!");
			}
			_eventsRead++;
			// Add current point to new track in next event
			_nextEvent.eventNumber = eventNumber;
			_nextEvent.runID = runID;
			track_t tr;
			tr.sensorIDs.push_back(sensorID);
			tr.points.push_back(pos);
			_nextEvent.tracks.push_back(tr);
			// We're done
			break;
		} else {
			// add data to current track
			cur_track.sensorIDs.push_back(sensorID);
			cur_track.points.push_back(pos);
		}
	}

	return *this;
}

TrackStreamReader::EventIterator TrackStreamReader::EventIterator::operator++(int)
{
	EventIterator old(*this);
	++(*this);
	return old;
}

void TrackStreamReader::EventIterator::open()
{
	_fin.exceptions(std::ios_base::failbit);
	_fin.open(_filename);
	_fin.exceptions(std::ios_base::goodbit);
}

void TrackStreamReader::EventIterator::compileRegex()
{
	if(_regexCompiled) {
		regfree(&_regexComment);
		regfree(&_regexLine);
		_regexCompiled = false;
	}
	char error[500];
	int ret = regcomp(&_regexComment, "^\\s*#", REG_EXTENDED);
	if(ret != 0) {
		regerror(ret, &_regexComment, error, sizeof(error));
		std::cerr << "Error while compiling TrackStreamReader comment regex:\n"
		          << error << std::endl;
	}
	assert(ret == 0);
	ret = regcomp(&_regexLine, "^\\s*([0-9]+.?[0-9]*)\\s+([0-9]+.?[0-9]*)\\s+([0-9]+.?[0-9]*)\\s+"
	                           "([0-9]+)\\s+([0-9]+)\\s+([0-9]+)", REG_EXTENDED);
	if(ret != 0) {
		regerror(ret, &_regexLine, error, sizeof(error));
		std::cerr << "Error while compiling TrackStreamReader line regex:\n"
		          << error << std::endl;
		regfree(&_regexComment);
	}
	assert(ret == 0);
	_regexCompiled = true;
}

TrackStreamReader::TrackStreamReader(const std::string& filename)
 : _filename(filename)
{
}

TrackStreamReader::EventIterator TrackStreamReader::begin() const
{
	return EventIterator(_filename, false);
}

TrackStreamReader::EventIterator TrackStreamReader::end() const
{
	return EventIterator(_filename, true);
}
