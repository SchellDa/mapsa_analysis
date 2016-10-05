
#ifndef CORE_UTIL_H
#define CORE_UTIL_H

namespace core {

/** \brief Implementation of C++14 std::max */
template<class T> 
const T& max(const T& a, const T& b)
{
	return (a < b) ? b : a;
}

/** \brief Implementation of a C++14 std::make_unique */
template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args)
{
	    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

}; // core

#endif//CORE_UTIL_H
