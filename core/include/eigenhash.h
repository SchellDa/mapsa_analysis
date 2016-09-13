#ifndef EIGEN_HASH_H
#define EIGEN_HASH_H

#include <functional>
#include <Eigen/Dense>

namespace std {

template<> struct hash<Eigen::Vector2i>
{
	typedef Eigen::Vector2i argument_type;
	typedef std::size_t result_type;
	result_type operator()(argument_type const& s) const
	{
		result_type const h1 ( std::hash<int>()(s(0)) );
		result_type const h2 ( std::hash<int>()(s(1)) );
		return h1 ^ (h2 << 1);
	}
};

} // namespace std

#endif//EIGEN_HASH_H
