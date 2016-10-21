
#ifndef BASE_SENSOR_STREAM_READER_H
#define BASE_SENSOR_STREAM_READER_H

#include "abstractfactory.h"
#include <type_traits>

namespace core {

/** \brief Abstract pixel/strip sensor data reader class
 *
 * To easily implement different file formats for pixelish sensor data (e.g. CMS pixel, MaPSA light or CBC
 * sensor data, an abstract class is provided. The very basic BaseSensorStreamReader::event_t contains the
 * data of each event in a very simple fashion. For easy access, two iterator classes are provided (iterator
 * and const_iterator), satisfying the C++ iterator concepts.
 *
 * The actual work is performed by a subclassed BaseSensorStreamReader::reader class.
 */
class BaseSensorStreamReader
{
public:
	/** \brief Short-hand for the BaseSensorStreamReader factory class
	 *
	 * A specialisation of the AbstractFactory class.
	 */
	typedef AbstractFactory<BaseSensorStreamReader> Factory;

	/** \brief Event data structure
	 */
	struct event_t {
		/// Event number
		int eventNumber;
		/** Count-value or binary hit map of pixels/strips in sequential order
		 *
		 * \sa MpaTransform
		 */
		std::vector<int> data;
	};

	/** \brief Abstract data reader, the work horse
	 *
	 * An implementation of this class is used by the iterators to read data. When incrementing the
	 * iterator, next() is called. The clone() method is used for copy-construction and assigment of the
	 * iterators.
	 *
	 * \sa BaseSensorStreamReader::getReader
	 */
	class reader {
	public:
		reader(const std::string& filename) : _filename(filename) {}
		virtual ~reader() {}
		/** \brief Read next event
		 */
		virtual bool next() = 0;
		/** \brief Create a new reader instance pointing to the same point in the datastream.
		 */
		virtual reader* clone() const = 0;
		/** \brief Get current event number
		 *
		 */
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
	virtual ~BaseSensorStreamReader() {}

	void setFilename(const std::string& filename)
	{
		_filename = filename;
	}

	std::string getFilename() const { return _filename; }

	/** \brief Create new iterator pointing to the first event
	 *
	 * \sa getReader
	 */
	iterator begin()
	{
		return iterator(getReader(_filename), false);
	}

	/** \brief Create new iterator pointing to the first event
	 *
	 * \sa getReader
	 */
	const_iterator begin() const
	{
		return const_iterator(getReader(_filename), false);
	}

	/** \brief Create new beyond-last-element iterator
	 */
	iterator end()
	{
		return iterator(nullptr, true);
	}

	/** \brief Create new beyond-last-element iterator
	 */
	const_iterator end() const
	{
		return const_iterator(nullptr, true);
	}

protected:
	/** \brief Create sub-type specific reader instance
	 *
	 * When constructing a new iterator in begin(), this function provides the sub-type specific read
	 * class.
	 */
	virtual reader* getReader(const std::string& filename) const = 0;

private:
	std::string _filename;
};


#define REGISTER_PIXEL_STREAM_READER_TYPE(type) REGISTER_FACTORY_TYPE(core::BaseSensorStreamReader, type)

}//namespace core

#endif//BASE_SENSOR_STREAM_READER_H
