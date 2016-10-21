#ifndef CBC_STREAM_READER_H
#define CBC_STREAM_READER_H

#include <vector>
#include <string>
#include <fstream>
#include <TFile.h>
#include <TTree.h>
#include "basesensorstreamreader.h"
#include "interface/DataFormats.h"

namespace core {

class CBCStreamReader : public BaseSensorStreamReader
{
public:
	CBCStreamReader();
	CBCStreamReader(const std::string& filename);
	virtual ~CBCStreamReader();

protected:
	/** \brief Iterator for traversing separate events in the CBC data file
	 *
	 * The work horse of the CBCStreamReader. A C++11 compliant copyable and movable iterator
	 * implementation. The first event is read during iterator construction, any subsequent read is
	 * performed when incrementing the iterator.
	 */
        class cbcreader : public BaseSensorStreamReader::reader {
	public:
		cbcreader(const std::string& filename, size_t eventNum=0);
		virtual ~cbcreader();
		virtual bool next();
		virtual BaseSensorStreamReader::reader* clone() const;

	private:
                void open();
                TFile _fin;
                TTree* _analysisTree;
                tbeam::dutEvent* _dutEvent;
		size_t _numEventsRead;
	};
	
	virtual BaseSensorStreamReader::reader* getReader(const std::string& filename) const;
};

} // namespace core

#endif//CBC_STREAM_READER_H
