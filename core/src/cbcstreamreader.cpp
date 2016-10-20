#include "cbcstreamreader.h"
#include <cassert>
#include <regex.h>
#include <TFile.h>

using namespace core;

namespace core {
REGISTER_PIXEL_STREAM_READER_TYPE(CBCStreamReader)
}

CBCStreamReader::cbcreader::cbcreader(const std::string& filename, size_t seek)
: reader(filename), _fin(filename), _numEventsRead(0)
{
        _dutEvent = new tbeam::dutEvent;
        open()
	if(seek == 0) {
		next();
	}
}

CBCStreamReader::cbcreader::~cbcreader()
{
	_fin.Close();
}

bool CBCStreamReader::cbcreader::next()
{
        if(_t1.GetEntries() == _numEventsRead)
	{
	        return true;
	}
	
	_t1.GetEvent(_numEventsRead);
	
	_currentEvent.data.clear();
	_currentEvent.eventNumber = _numEventsRead++;
	
	for(const auto &n: _dutEvent->dut_channel["det"])
        {
	    _currentEvent.data.push_back(n);
	}  
	
	return false;
}


void CBCStreamReader::cbcreader::open(size_t seek)
{
        _fin = TFile::Open(getFilename());
        if(!_fin)
	{
	        std::cout << "File " << getFilename() <<  "could not be opened!" << std::endl;
	        return false;
	}
	_analysisTree = dynamic_cast<TTree*>(_fin->Get("analysisTree"));
	if(!_analysisTree)
	{
	        std::cout << "Analysis Tree not found" << std::endl;
		return false;
	}
	
	_analysisTree->SetBranchAddress("DUT",&_dutEvent);
	

}


BaseSensorStreamReader::reader* CBCStreamReader::cbcreader::clone() const
{
	auto newReader = new cbcreader(getFilename(), _fin.tellg());
	newReader->_currentEvent = _currentEvent;
	newReader->_numEventsRead = _numEventsRead;
	return newReader;
}

BaseSensorStreamReader::reader* CBCStreamReader::getReader(const std::string& filename) const
{
	return new cbcreader(filename);
}
