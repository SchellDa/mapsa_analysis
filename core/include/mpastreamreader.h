#ifndef MPA_STREAM_READER_H
#define MPA_STREAM_READER_H

#include <vector>
#include <string>
#include <fstream>
#include "basesensorstreamreader.h"

namespace core {

/** \brief Streamed access to MPA counter data files
 *
 * We store the MPA data as a simple text format. Each line contains the counter values for each pixel from a
 * single event. Each line is encoded like the representation of a Python list of integers (because that's
 * what it is). A regex matching the line would be \verbatim \[([0-9]+, )+\] \endverbatim
 *
 * The MPAStremReader is more tollerant, it just interprets every number in a line seperated by non-digit
 * characters as counter values.
 * \todo Maybe a check for line format consistency would be useful for debugging and robustness.
 *
 * The MPAStreamReader is compatible with range-based for loops, as it implements an C++11 iterator interface
 * via BaseSensorStreamReader::iterator .
 *
 * \code{.cpp}
MPAStreamReader read("run0028_counter.txt_0");
for(auto event: read) {
//	event.data; is hopefully nice
}
\endcode
 */
class MPAStreamReader : public BaseSensorStreamReader
{
public:
	MPAStreamReader() : BaseSensorStreamReader() {}
	MPAStreamReader(const std::string& filename) : BaseSensorStreamReader(filename) {}

protected:
	/** \brief Iterator for traversing separate events in the MPA data file
	 *
	 * The work horse of the MPAStreamReader. A C++11 compliant copyable and movable iterator
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

#endif//MPA_STREAM_READER_H
