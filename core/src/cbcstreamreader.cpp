#include "cbcstreamreader.h"
#include <cassert>
#include <regex.h>
#include <TFile.h>
#include <iostream>

using namespace core;

namespace core {
REGISTER_PIXEL_STREAM_READER_TYPE(CBCStreamReader)
}

CBCStreamReader::cbcreader::cbcreader(const std::string& filename, size_t eventNum)
	: reader(filename), _fin(new TFile(filename.c_str(),"READ")), _numEventsRead(eventNum), _dutEvent(new tbeam::dutEvent)
{
        open();
	if(eventNum == 0) {
		next();
	}
}

CBCStreamReader::cbcreader::~cbcreader()
{
	_fin->Close();
}

bool CBCStreamReader::cbcreader::next()
{
        if(_numEventsRead == _analysisTree->GetEntries())
	{
	        return true;
	}
	
	_analysisTree->GetEvent(_numEventsRead);
	
	_currentEvent.data.clear();
	_currentEvent.eventNumber = _numEventsRead++;
	
	for(const auto &n: _dutEvent->dut_channel["det"])
        {
	    _currentEvent.data.push_back(n);
	}  
	
	return false;
}


void CBCStreamReader::cbcreader::open()
{
        //_fin = TFile::Open(getFilename().c_str());
        if(!_fin)
	{
		throw std::ios_base::failure("Cannot open");
	}
	_analysisTree = dynamic_cast<TTree*>(_fin->Get("analysisTree"));
	if(!_analysisTree)
	{
		throw std::ios_base::failure("Cannot open");
	}
	
	_analysisTree->SetBranchAddress("DUT",&_dutEvent);
	
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
