#ifndef MPA_STREAM_READER_H
#define MPA_STREAM_READER_H

#include <vector>
#include <string>
#include <fstream>

namespace core {

class MPAStreamReader
{
public:
	struct event_t {
		size_t eventNumber;
		std::vector<int> data;
	};
	class EventIterator {
	public:
		EventIterator(const std::string& filename, bool end);
		EventIterator(const EventIterator& other);
		EventIterator(EventIterator&& other) noexcept;
		~EventIterator();
		EventIterator& operator=(const EventIterator& other);
		EventIterator& operator=(EventIterator&& other) noexcept;

		bool operator==(const EventIterator& other) const;
		bool operator!=(const EventIterator& other) const;

		EventIterator& operator++();
		EventIterator operator++(int);

		event_t operator*() const { return _currentEvent; }

		size_t getEventNumber() const noexcept { return _currentEvent.eventNumber; }
	private:
		void open();
		mutable std::ifstream _fin;
		std::string _filename;
		bool _end;
		size_t _numEventsRead;
		event_t _currentEvent;
	};

	MPAStreamReader(const std::string& filename);
	EventIterator begin() const;
	EventIterator end() const;
private:
	std::string _filename;
};

} // namespace core

#endif//MPA_STREAM_READER_H
