
#ifndef CORE_UTIL_H
#define CORE_UTIL_H

namespace core {

/** \brief Implementation of C++14 std::max */
template<class T> 
const T& max(const T& a, const T& b)
{
	return (a < b) ? b : a;
}

}; // core

#endif//CORE_UTIL_H
