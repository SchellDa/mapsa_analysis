
#ifndef BASE_SENSOR_STREAM_READER_H
#define BASE_SENSOR_STREAM_READER_H

#include "abstractfactory.h"
#include <type_traits>

namespace core {

class BaseSensorStreamReader
{
public:
	struct event_t {
		int eventNumber;
		std::vector<int> data;
	};

	class reader {
	public:
		reader(const std::string& filename) : _filename(filename) {}
		virtual ~reader() {}
		virtual bool next() = 0;
		virtual reader* clone() const = 0;
		virtual int eventNumber() const
		{
			return _currentEvent.eventNumber;
		}

		event_t& get() { return _currentEvent; }
		const event_t& get() const { return _currentEvent; }
		const std::string& getFilename() const { return _filename; }

	protected:
		event_t _currentEvent;
	private:
		std::string _filename;
	};

	template<bool is_const_iterator>
	class const_noconst_iterator : public std::iterator<std::forward_iterator_tag, event_t> {
	public:
		typedef typename std::conditional<is_const_iterator, const event_t&, event_t&>::type event_ref_t;
		typedef typename std::conditional<is_const_iterator, const event_t*, event_t*>::type event_ptr_t;

		const_noconst_iterator(reader* read, bool end) :
		 _reader(read), _end(end)
		{
		}

		const_noconst_iterator(const const_noconst_iterator<false>& other) :
		 _reader(nullptr), _end(other._end)
		{
			if(other._reader) {
				_reader = other._reader->clone();
			}
		}

		~const_noconst_iterator()
		{
			if(_reader) {
				delete _reader;
			}
		}

		const_noconst_iterator& operator=(const const_noconst_iterator& other)
		{
			if(_reader)
				delete _reader;
			_reader = other._reader->clone();
			_end = other._end;
			return *this;
		}

		bool operator==(const const_noconst_iterator& other) const
		{
			if(other._end != _end)
				return false;
			if(_end) {
				return true;
			}
			return (_reader->eventNumber() == other._reader->eventNumber()) || 
			       (_reader->getFilename() == other._reader->getFilename());
		}

		bool operator!=(const const_noconst_iterator& other) const
		{
			return !(*this == other);
		}

		event_ref_t operator*()
		{
			if(_reader) {
				return _reader->get();
			}
			return _empty;
		}

		event_ptr_t operator->()
		{
			return &(**this);
		}

		const_noconst_iterator& operator++()
		{
			if(_reader && !_end) {
				_end = _reader->next();
			}
			return *this;
		}

		const_noconst_iterator operator++(int)
		{
			const const_noconst_iterator old(*this);
			++(*this);
			return old;
		}

		friend class const_noconst_iterator<true>;

	private:
		reader* _reader;
		event_t _empty;
		bool _end;
	};

	typedef const_noconst_iterator<false> iterator;
	typedef const_noconst_iterator<true> const_iterator;
	
	BaseSensorStreamReader() : _filename("") {}
	BaseSensorStreamReader(const std::string& filename) : _filename(filename) {}

	void setFilename(const std::string& filename)
	{
		_filename = filename;
	}

	iterator begin()
	{
		return iterator(getReader(_filename), false);
	}

	const_iterator begin() const
	{
		return const_iterator(getReader(_filename), false);
	}

	iterator end()
	{
		return iterator(nullptr, true);
	}

	const_iterator end() const
	{
		return const_iterator(nullptr, true);
	}

protected:
	virtual reader* getReader(const std::string& filename) const = 0;

private:
	std::string _filename;
};

typedef AbstractFactory<BaseSensorStreamReader> BaseSensorStreamReaderFactory;

#define REGISTER_PIXEL_STREAM_READER_TYPE(type) REGISTER_FACTORY_TYPE(core::BaseSensorStreamReader, type)

}//namespace core

#endif//BASE_SENSOR_STREAM_READER_H
