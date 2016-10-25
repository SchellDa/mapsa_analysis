#include "cbcstreamreader.h"
#include <cassert>
#include <regex.h>
#include <TFile.h>
#include <iostream>

using namespace core;


CBCStreamReader::CBCStreamReader() :
 BaseSensorStreamReader()
{
}

CBCStreamReader::CBCStreamReader(const std::string& filename) :
 BaseSensorStreamReader(filename)
{
}

CBCStreamReader::~CBCStreamReader()
{
}

CBCStreamReader::cbcreader::cbcreader(const std::string& filename, size_t eventNum) :
 reader(filename), _fin(filename.c_str(), "READ"), _analysisTree(nullptr), _dutEvent(new tbeam::dutEvent),
 _condition(new tbeam::condEvent), _telescopeEvent(new tbeam::TelescopeEvent),_goodEventFlag(false),
 _numEventsRead(eventNum)
{
	_currentEvent.eventNumber = 0;
        open();
	if(eventNum == 0) {
		next();
	}
}

CBCStreamReader::cbcreader::~cbcreader()
{
	_fin.Close();
	delete _dutEvent;
	delete _condition;
	delete _telescopeEvent;
}

bool CBCStreamReader::cbcreader::next()
{
	_currentEvent.data.clear();
	if(_numEventsRead == _analysisTree->GetEntries())
	{
		return true;
	}
	_analysisTree->GetEvent(_numEventsRead);
	// The +2 offset was "empiricaly" observed. Quite a magic constant ATM
	if(_currentEvent.eventNumber + 2 == _condition->event) {
		//good = _goodEventFlag > 0;
	/*	std::cout << "\n-----------------------------\n  Event No:" << _currentEvent.eventNumber
		          << "\ngoodEvent " << _goodEventFlag
			  << "\ncondEvent.event " << _condition->event
			  << "\nTelescopeEvent.euEvt " << _telescopeEvent->euEvt
			  << "\nevt difference " << (_condition->event - _currentEvent.eventNumber)
			  << "\n-----------------------------"
			  << std::endl; */
		for(const auto &n: _dutEvent->dut_channel.at("det0"))
		{
		    _currentEvent.data.push_back(n);
		}
		++_numEventsRead;
	}
	_currentEvent.eventNumber++;
	return false;
}


void CBCStreamReader::cbcreader::open()
{
        /*if(_fin)
	{
		throw std::ios_base::failure("Cannot open");
	}*/
	_analysisTree = dynamic_cast<TTree*>(_fin.Get("analysisTree"));
	if(!_analysisTree)
	{
		throw std::ios_base::failure("analysisTree not found in ROOT file.");
	}
	_analysisTree->SetBranchAddress("DUT", &_dutEvent);
	_analysisTree->SetBranchAddress("Condition", &_condition);
	_analysisTree->SetBranchAddress("TelescopeEvent", &_telescopeEvent);
	_analysisTree->SetBranchAddress("goodEventFlag", &_goodEventFlag);
}

BaseSensorStreamReader::reader* CBCStreamReader::cbcreader::clone() const
{
	auto newReader = new cbcreader(getFilename(), _numEventsRead);
	newReader->_currentEvent = _currentEvent;
	newReader->_numEventsRead = _numEventsRead;
	return newReader;
}

BaseSensorStreamReader::reader* CBCStreamReader::getReader(const std::string& filename) const
{
	return new cbcreader(filename);
}
