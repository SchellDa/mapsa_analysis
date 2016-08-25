
#ifndef ABSTRACT_FACTORY_H
#define ABSTRACT_FACTORY_H

#include <memory>
#include <functional>
#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/cat.hpp>

namespace core {

template<typename T, typename description_T=std::string>
class AbstractFactory
{
public:
	typedef std::function<std::shared_ptr<T>()> creator_t;
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

	bool RegisterCreator(const std::string& type, const creator_t& fct)
	{
		return RegisterCreator(type, fct, description_T{});
	}

	bool RegisterCreator(const std::string& type, const creator_t& fct, const description_T& descr)
	{
		_creators[type] = fct;
		_descriptions[type] = descr;
		return true;
	}

	std::vector<std::string> getTypes() const
	{
		std::vector<std::string> a;
		for(const auto& item: _creators) {
			a.push_back(item.first);
		}
		return a;
	}

	description_T getDescription(const std::string& type) const
	{
		return _descriptions.at(type);
	}

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

#define REGISTER_FACTORY_TYPE(base, type) static bool BOOST_PP_CAT(type, __regged) = core::AbstractFactory<base>::Instance()->RegisterCreator(BOOST_PP_STRINGIZE(type), std::bind(&std::make_shared<type>));

#define REGISTER_FACTORY_TYPE_WITH_DESCR(base, type, descr) static bool BOOST_PP_CAT(type, __regged) = core::AbstractFactory<base>::Instance()->RegisterCreator(BOOST_PP_STRINGIZE(type), std::bind(&std::make_shared<type>), descr);

}//namespace core

#endif//ABSTRACT_FACTORY_H
