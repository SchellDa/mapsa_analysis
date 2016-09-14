
#ifndef ABSTRACT_FACTORY_H
#define ABSTRACT_FACTORY_H

#include <map>
#include <memory>
#include <functional>
#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/cat.hpp>

namespace core {

/** \brief Generic factory pattern class
 *
 * This class can be used to dynamicaly generate objects identified by their type name. All objects must be
 * derived from a common base class T. The AbstractFactory is a singleton itself, with one object per class
 * type T and description_T type.
 *
 * The objects are created by a functor object returning an std::shared_ptr<T> that takes no arguments.
 * If the objects can be created by the default constructor, one can use the
 * #REGISTER_FACTORY_TYPE(T, type) and #REGISTER_FACTORY_TYPE_WITH_DESCR(T, type, descr) macros. They
 * invoke RegisterCreator() on the AbstractFactory<T> and pass a functor calling std::make_shared<T>.
 *
 * \tparam T Abstract object interface class, base-type for generated objects
 * \tparam description_T Type of the describing meta-information. std::string by default.
 */
template<typename T, typename description_T=std::string>
class AbstractFactory
{
public:
	/// A functor returning an object of type T
	typedef std::function<std::shared_ptr<T>()> creator_t;

	/// Get the AbstractFactory singleton object
	static AbstractFactory<T>* Instance()
	{
		if(s_instance == nullptr) {
			s_instance = new AbstractFactory;
		}
		return s_instance;
	}

	AbstractFactory() :
	 _creators(), _descriptions()
	{
	}

	/** \brief Register creator function
	 *
	 * A type registration by a unique name and a creator functor returning a new object.
	 * \param type Identifier of the new type. Should be the C++ class name.
	 * \param fct Functor returning a new object with base-type T.
	 */
	bool RegisterCreator(const std::string& type, const creator_t& fct)
	{
		return RegisterCreator(type, fct, description_T{});
	}
	/** \brief Register creator function with description object
	 *
	 * A type registration by a unique name and a creator functor returning a new object. An object
	 * describing the type can be passed.
	 * \param type Identifier of the new type. Should be the C++ class name.
	 * \param fct Functor returning a new object with base-type T.
	 * \param descr Informative description of the new type.
	 */
	bool RegisterCreator(const std::string& type, const creator_t& fct, const description_T& descr)
	{
		_creators[type] = fct;
		_descriptions[type] = descr;
		return true;
	}

	/** \brief Return a vector with all registered type names
	 */
	std::vector<std::string> getTypes() const
	{
		std::vector<std::string> a;
		for(const auto& item: _creators) {
			a.push_back(item.first);
		}
		return a;
	}

	/** \brief Get description object for a specified type
	 *
	 * \param type Identifier of the type to get description about
	 * \throw std::out_of_range The requested type was not registered.
	 */
	description_T getDescription(const std::string& type) const
	{
		return _descriptions.at(type);
	}

	/** \brief Create new object of specified type
	 *
	 * Calls the functor object passed via RegisterCreator() and returns
	 * the created object.
	 * \param type Type-identifier of the object to create
	 * \return Shared pointer to new object of specified type
	 * \throw std::out_of_range The requested type was not registered.
	 */
	std::shared_ptr<T> create(const std::string& type) const
	{
		return _creators.at(type)();
	}

private:
	std::map<std::string, creator_t> _creators;
	std::map<std::string, description_T> _descriptions;
	static AbstractFactory<T, description_T>* s_instance;
};

template<typename T, typename description_T>
AbstractFactory<T, description_T>* AbstractFactory<T, description_T>::s_instance;

/** \brief Register new type to specific factory
 *
 * Adds a new type to the AbstractFactory<base> by invoking RegisterCreator() and passing a functor to
 * std::make_shared<type>. The type name is the C++ identifier passed as type.
 * \param type C++ class identifier of new type class
 * \param base Base class identifier to identify the correct factory
 */
#define REGISTER_FACTORY_TYPE(base, type) static bool BOOST_PP_CAT(type, __regged) = core::AbstractFactory<base>::Instance()->RegisterCreator(BOOST_PP_STRINGIZE(type), std::bind(&std::make_shared<type>));

/** \brief Register new type to specific factory
 *
 * Adds a new type to the AbstractFactory<base> by invoking RegisterCreator() and passing a functor to
 * std::make_shared<type>. The type name is the C++ identifier passed as type.
 *
 * Additionaly, a descriptive object descr is passed.
 *
 * \param type C++ class identifier of new type class
 * \param base Base class identifier to identify the correct factory
 * \param descr Descriptive object of the type specified for the AbstractFactory
 */
#define REGISTER_FACTORY_TYPE_WITH_DESCR(base, type, descr) static bool BOOST_PP_CAT(type, __regged) = core::AbstractFactory<base>::Instance()->RegisterCreator(BOOST_PP_STRINGIZE(type), std::bind(&std::make_shared<type>), descr);

}//namespace core

#endif//ABSTRACT_FACTORY_H
