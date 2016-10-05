#ifndef TRACK_STREAM_READER_H
#define TRACK_STREAM_READER_H

#include <vector>
#include <string>
#include <fstream>
#include <regex.h>
#include "track.h"

namespace core {

/** \brief Streamed access to aligned and cut telescope tracks.
 *
 * This class provides access to data files containing information about tracks potentially hitting a DUT. The
 * expected file-format is a gnuplot-compatible whitespace-seperated-columns format. The individual hit points
 * are stored one per line, with different tracks being seperated by two empty lines (so each track would be a
 * block in gnuplot). The column layout is in order
 *  - x coord (double)
 *  - y coord (double)
 *  - z coord (double)
 *  - sensor plane ID (int)
 *  - event number (int)
 *  - run ID (int)
 *
 *  A line can start or contain a comment using the # symbol. A comment-only line is not considered empty!
 *  Additional columns may be inserted and are ignored by the parser. An example track file could look like
 *  this
 *  \code{.csv}
# X	Y	Z	SensorID	Evt	Run
0.00306126	-1.07667	0	0	7	1
0.026713	-1.07356	151	1	7	1
0.0508347	-1.0704d	305	2	7	1
0.0801253	-1.06656	492	8	7	1


-2.50308	-0.657463	0	0	11	1
-2.57001	-0.743347	151	1	11	1
-2.63826	-0.830937	305	2	11	1
-2.72114	-0.937297	492	8	11	1


3.180044	1.49489	0	0	20	1
3.151063	1.23167	151	1	20	1
3.121516	0.96321	305	2	20	1
3.085633	0.63724	492	8	20	1
\endcode

The sensor plane ID can be used to exclude certain points from the track (e.g. a reference plane). The
event number is used to group tracks into events. It is expected that all tracks of an event are
consecutive in the data file and that the event number is not decreasing. The run ID is read and stored,
but not used by the TrackStreamReader.

The TrackStreamReader is compatible with range-based for loops, as it implements an C++11 iterator interface
via TrackStreamReader::EventIterator.

\code{.cpp}
TrackStreamReader read("run0028_counter.txt_0");
for(auto event: read) {
	// >>> have fun here <<<
}
\endcode
 */
class TrackStreamReader
{
public:
	/// Description of a single event
	struct event_t {
		/// Number of the event as stored in the data file
		int eventNumber;
		/// Run ID for the event as in the data file.
		int runID;
		/// A vector of all tracks.
		std::vector<Track> tracks;
	};

	/** Exception class indicating a consistency error in the data file.
	 *
	 * A consistency error is thrown when the runID or eventID changes inside
	 * a track block, both values are expected to be constant for _all_ points in the track.
	 */	
	class consistency_error : public std::runtime_error {
	public:
		consistency_error(const std::string& file, const int& row, const std::string& message)
		 : std::runtime_error(""), msg()
		{
			std::ostringstream sstr;
			sstr << file << ":" << row << ": " << message;
			msg = sstr.str();
		}
		virtual const char* what() const noexcept { return msg.c_str(); }

	private:
		std::string msg;
	};
	
	/** Exception class indicating a parsing error.
	 *
	 * Thrown when the data lines do not obey the correct format. Extra columns and comments
	 * are valid. Only missing columns, wrong delimiters and generally incorrect data values
	 * will raise this exception.
	 */	
	class parse_error : public std::runtime_error {
	public:
		parse_error(const std::string& file, const int& row, const int& col, const std::string& message)
		 : std::runtime_error(""), msg()
		{
			std::ostringstream sstr;
			sstr << file << ":" << row;
			if(col > 0) sstr << ":" << col;
			sstr << ": " << message;
			msg = sstr.str();
		}
		virtual const char* what() const noexcept { return msg.c_str(); }

	private:
		std::string msg;
	};

	/** \brief Iterator for traversing separate events in the track data file
	 *
	 * The work horse of the TrackStreamReader. A C++11 compliant copyable and movable iterator
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
		/** This will load the next event from file. When the EOF was reached, the iterator is
		 * converted to an beyond-last-element iterator.
		 * \throw parse_error Malformed lines and non-parsable data values
		 * \throw consistency_error Prevents changing of runID and eventID in a track block.
		 */
		EventIterator& operator++();
		/// \sa operator++()
		EventIterator operator++(int);

		/** \brief Get current event the iterator is pointing to. */
		const event_t& operator*() const noexcept { return _currentEvent; }

		/** \brief Get address of current event the iterator is pointing to.
		 *
		 * Used for it->eventNumber style access.
		 */
		const event_t* operator->() const noexcept { return &_currentEvent; }

		/** \brief Get the current event number. */
		int getEventNumber() const noexcept { return _currentEvent.eventNumber; }

		/** \brief Get the number of read events */
		size_t getNumReadEvents() const noexcept { return _eventsRead; }

		/** \brief Check wether iterator is beyond-last-element iterator */
		bool isEnd() const noexcept { return _end; }

	private:
		void open();
		void compileRegex();
		mutable std::ifstream _fin;
		std::string _filename;
		bool _end;
		event_t _currentEvent;
		event_t _nextEvent;
		bool _regexCompiled;
		regex_t _regexLine;
		regex_t _regexComment;
		size_t _eventsRead;
		size_t _currentLineNo;
	};

	/** \brief Construct a new TrackStreamReader instance.
	 *
	 * This will not perform any checks or filesystem operations! The first access to the data file will
	 * be when calling begin() to create a new iterator.
	 * \param filename Path of the file to open.
	 */
	TrackStreamReader(const std::string& filename);

	/** Get the iterator pointing to the first event.
	 *
	 * This will perform a read operation to retrieve the first event.
	 */
	EventIterator begin() const;

	/** Get beyond-last-element iterator */
	EventIterator end() const;

	std::string getFilename() const { return _filename; }
private:
	std::string _filename;
};

} // namespace core

#endif//TRACK_STREAM_READER_H
