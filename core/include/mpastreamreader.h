#ifndef MPA_STREAM_READER_H
#define MPA_STREAM_READER_H

#include <vector>
#include <string>
#include <fstream>

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
 * via MPAStreamReader::EventIterator.
 *
 * \code{.cpp}
MPAStreamReader read("run0028_counter.txt_0");
for(auto event: read) {
//	event.data; is hopefully nice
}
\endcode
 */
class MPAStreamReader
{
public:
	struct event_t {
		size_t eventNumber;
		std::vector<int> data;
	};
	/** \brief Iterator for traversing separate events in the MPA data file
	 *
	 * The work horse of the MPAStreamReader. A C++11 compliant copyable and movable iterator
	 * implementation. The first event is read during iterator construction, any subsequent read is
	 * performed when incrementing the iterator.
	 */
	class EventIterator {
	public:
		/** Construct a new MPA data iterator.
		 * \param filename The filename of the file to be opened.
		 * \param end If set to true, the iterator will be a beyond-last-element iterator. The file
		 * will not be opened in that case.
		 */
		EventIterator(const std::string& filename, bool end);
		EventIterator(const EventIterator& other);
		EventIterator(EventIterator&& other) noexcept;
		~EventIterator();
		EventIterator& operator=(const EventIterator& other);
		EventIterator& operator=(EventIterator&& other) noexcept;

		/** Compare iterators for equality.
		 *
		 * As part of the comparision, the filenames are compared. If you compare iterators
		 * from different stream readers pointing to the same file via different paths
		 * (relative/absolute), the iterators might not compare equal! But you should do that
		 * anyway...
		 */
		bool operator==(const EventIterator& other) const;
		bool operator!=(const EventIterator& other) const;

		/// Prefix-increment to the next event.
		/* This will load the next event from file. When the EOF was reached, the iterator is
		 * converted to an beyond-last-element iterator.
		 */
		EventIterator& operator++();
		EventIterator operator++(int);

		/** Get current event the iterator is pointing to. */
		event_t operator*() const { return _currentEvent; }

		/** Get the current event number. Same as \code *it.eventNumber \endcode */
		size_t getEventNumber() const noexcept { return _currentEvent.eventNumber; }
	private:
		void open();
		mutable std::ifstream _fin;
		std::string _filename;
		bool _end;
		size_t _numEventsRead;
		event_t _currentEvent;
	};

	/** \brief Construct a new MPAStreamReader instance.
	 *
	 * This will not perform any checks or filesystem operations! The first access to the data file will
	 * be when calling begin() to create a new iterator.
	 * \param filename Path of the file to open.
	 */
	MPAStreamReader(const std::string& filename);

	/** Get the iterator pointing to the first event.
	 *
	 * This will perform a read operation to retrieve the first event.
	 */
	EventIterator begin() const;

	/** Get beyond-last-element iterator */
	EventIterator end() const;
private:
	std::string _filename;
};

} // namespace core

#endif//MPA_STREAM_READER_H
