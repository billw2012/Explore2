#ifndef _MATH_RAY_HPP
#define _MATH_RAY_HPP

#include "Misc/libdesc.hpp"
#include "Misc/hash.hpp"

LIB_NAMESPACE_START

namespace math
{

template < class ValueType >
struct Ray
{
	typedef Vector3< ValueType > VectorType;
	VectorType origin;
	VectorType direction;

	template < class V >
	Ray(const Vector3< V >& _origin, Vector3< V > _direction) : origin(_origin), direction(_direction) {}

};

typedef Ray<float> Rayf;
typedef Ray<double> Rayd;

}

LIB_NAMESPACE_END

#endif // _MATH_RAY_HPP
