#ifndef MPA_MEMORY_STREAM_READER_H
#define MPA_MEMORY_STREAM_READER_H

#include <vector>
#include <string>
#include <fstream>
#include "basesensorstreamreader.h"

namespace core {

/** \brief Streamed access to MPA memory data files
 *
 * \todo Describe file format
 *
 * The MpaMemoryStreamReader is compatible with range-based for loops, as it implements an C++11 iterator interface
 * via BaseSensorStreamReader::iterator .
 *
 * \code{.cpp}
MpaMemoryStreamReader read("run0028_memory.txt_0");
for(auto event: read) {
//	event.data; is hopefully nice
}
\endcode
 */
class MpaMemoryStreamReader : public BaseSensorStreamReader
{
public:
	MpaMemoryStreamReader() : BaseSensorStreamReader() {}
	MpaMemoryStreamReader(const std::string& filename) : BaseSensorStreamReader(filename) {}

protected:
	/** \brief Iterator for traversing separate events in the MPA data file
	 *
	 * The work horse of the MpaMemoryStreamReader. A C++11 compliant copyable and movable iterator
	 * implementation. The first event is read during iterator construction, any subsequent read is
	 * performed when incrementing the iterator.
	 */
	class mpareader : public BaseSensorStreamReader::reader {
	public:
		mpareader(const std::string& filename, size_t seek=0);
		virtual ~mpareader();
		virtual bool next();
		virtual BaseSensorStreamReader::reader* clone() const;

	private:
		void open(size_t seek);
		mutable std::ifstream _fin;
		size_t _numEventsRead;
	};
	
	virtual BaseSensorStreamReader::reader* getReader(const std::string& filename) const;
};

} // namespace core

#endif//MPA_MEMORY_STREAM_READER_H
