#ifndef _VECTOR3_HPP
#define _VECTOR3_HPP

#include <limits>
#include <cmath>
#include <ostream>

#include "Misc/libdesc.hpp"
#include "Misc/hash.hpp"

#undef min
#undef max

LIB_NAMESPACE_START

namespace math
{

template < class V > struct Vector2;
template < class V > struct Vector4;

template < class ValueType >
struct Vector3
{
	typedef Vector3<ValueType> this_type;
	typedef ValueType value_type;
	typedef size_t index_type;

	value_type x;
	value_type y;
	value_type z;

	static const this_type XAxis;
	static const this_type YAxis;
	static const this_type ZAxis;
	static const this_type Zero;
	static const this_type One;
	static const this_type Min;
	static const this_type Max;

	// Construct default vector (0, 0, 0)
	Vector3() : x(static_cast<value_type>(0)), y(static_cast<value_type>(0)), z(static_cast<value_type>(0)) {}
	// Construct vector using specified values
	Vector3(value_type xi, value_type yi, value_type zi) : x(xi), y(yi), z(zi) {}
	// Construct vector using specified values
	template < class OtherType >
	Vector3(OtherType xi, OtherType yi, OtherType zi) : x(xi), y(yi), z(zi) {}
	// Construct vector using specified values array
	template < class OtherType >
	Vector3(const OtherType* const coordinates) 
		: x(coordinates[0]), y(coordinates[1]), z(coordinates[2]) {}
	// Copy constructor
	Vector3(const this_type& vector) : x(vector.x), y(vector.y), z(vector.z) {}

	template < class V >
	explicit Vector3(const Vector3<V>& vector) 
		: x(static_cast<value_type>(vector.x)), 
		y(static_cast<value_type>(vector.y)), 
		z(static_cast<value_type>(vector.z)) {}

	template < class V >
	explicit Vector3(const Vector4<V>& vector) : x(static_cast<value_type>(vector.x)), y(static_cast<value_type>(vector.y)), z(static_cast<value_type>(vector.z)) {}

	template < class V, class V2 >
	explicit Vector3(const Vector2<V>& vector, V2 z_) : x(static_cast<value_type>(vector.x)), y(static_cast<value_type>(vector.y)), z(static_cast<value_type>(z_)) {}

	template < class V >
	explicit Vector3(const Vector2<V>& vector) : x(static_cast<value_type>(vector.x)), y(static_cast<value_type>(vector.y)), z(static_cast<value_type>(0)) {}

	template < class V >
	value_type dotp(const Vector3<V>& v) const
	{
		return static_cast<value_type>((x * v.x) + (y * v.y) + (z * v.z));
	}

	template < class V >
	value_type angle(const Vector3<V>& v) const
	{
		return static_cast<value_type>(std::acos( MY_MAX(MY_MIN(dotp(v), 1.0), -1.0) ));
	}

	template <class V >
	this_type crossp(const Vector3<V>& v) const
	{
		return this_type( 
			y*v.z - z*v.y, 
			z*v.x - x*v.z, 
			x*v.y - y*v.x );
	}

	void normalize()
	{
		value_type len = length();
		if (len != static_cast<value_type>(0))
		{
			value_type invLen = static_cast<value_type>(1) / len;
			x *= invLen;
			y *= invLen;
			z *= invLen;
		}
	}

	this_type normal() const
	{
		this_type norm(*this);
		norm.normalize();
		return norm;
	}

	value_type length() const
	{
		value_type lenSquared = lengthSquared();
		return (lenSquared == static_cast<value_type>(0)) ? static_cast<value_type>(0) : static_cast<value_type>(std::sqrt(static_cast<double>(lenSquared)));
	}

	value_type lengthSquared() const
	{
		return (x * x + y * y + z * z);
	}

	value_type operator[](index_type index) const
	{
		return *(&x + index);
	}

	value_type& operator()(index_type index) 
	{
		return *(&x + index);
	}
};

template<class V> const typename Vector3<V>::this_type Vector3<V>::XAxis = Vector3<V>(V(1), V(0), V(0));
template<class V> const typename Vector3<V>::this_type Vector3<V>::YAxis = Vector3<V>(V(0), V(1), V(0));
template<class V> const typename Vector3<V>::this_type Vector3<V>::ZAxis = Vector3<V>(V(0), V(0), V(1));
template<class V> const typename Vector3<V>::this_type Vector3<V>::Zero = Vector3<V>(V(0), V(0), V(0));
template<class V> const typename Vector3<V>::this_type Vector3<V>::One = Vector3<V>(V(1), V(1), V(1));
template<class V> const typename Vector3<V>::this_type Vector3<V>::Min = Vector3<V>(-std::numeric_limits<V>::max(), -std::numeric_limits<V>::max(), -std::numeric_limits<V>::max());
template<class V> const typename Vector3<V>::this_type Vector3<V>::Max = Vector3<V>(std::numeric_limits<V>::max(), std::numeric_limits<V>::max(), std::numeric_limits<V>::max());

typedef Vector3<float> Vector3f;
typedef Vector3<double> Vector3d;
typedef Vector3<int> Vector3i;
typedef Vector3<unsigned int> Vector3u;

#define DEFINE_V3_UNARY_OPERATOR(op) template <class U> \
	LIB_NAMESPACE::math::Vector3<U> operator##op(const LIB_NAMESPACE::math::Vector3<U>& u) { \
		return LIB_NAMESPACE::math::Vector3<U>(op u.x, op u.y, op u.z); \
	}

#define DEFINE_V3V3_BINARY_OPERATOR(op) template <class U, class V> \
	LIB_NAMESPACE::math::Vector3<U> operator##op(const LIB_NAMESPACE::math::Vector3<U>& u, const LIB_NAMESPACE::math::Vector3<V>& v) { \
	return LIB_NAMESPACE::math::Vector3<U>(u.x op v.x, u.y op v.y, u.z op v.z); \
}

#define DEFINE_V3S_BINARY_OPERATOR(op) template <class U, class V> \
	LIB_NAMESPACE::math::Vector3<U> operator##op(const LIB_NAMESPACE::math::Vector3<U>& u, V v) { \
	return LIB_NAMESPACE::math::Vector3<U>(u.x op v, u.y op v, u.z op v); \
}

#define DEFINE_SV3_BINARY_OPERATOR(op) template <class U, class V> \
	LIB_NAMESPACE::math::Vector3<U> operator##op(V v, const LIB_NAMESPACE::math::Vector3<U>& u) { \
	return LIB_NAMESPACE::math::Vector3<U>(v op u.x, v op u.y, v op u.z); \
}

#define DEFINE_V3V3_MODIFY_BINARY_OPERATOR(op) template <class U, class V> \
	LIB_NAMESPACE::math::Vector3<U>& operator##op(LIB_NAMESPACE::math::Vector3<U>& u, const LIB_NAMESPACE::math::Vector3<V>& v) { \
	u.x op v.x; u.y op v.y; u.z op v.z; return u; \
}

#define DEFINE_V3S_MODIFY_BINARY_OPERATOR(op) template <class U, class V> \
	LIB_NAMESPACE::math::Vector3<U>& operator##op(LIB_NAMESPACE::math::Vector3<U>& u, V v) { \
	u.x op v; u.y op v; u.z op v; return u; \
}

DEFINE_V3_UNARY_OPERATOR(-)

DEFINE_V3V3_BINARY_OPERATOR(+)
DEFINE_V3V3_BINARY_OPERATOR(-)
DEFINE_V3V3_BINARY_OPERATOR(/)
DEFINE_V3V3_BINARY_OPERATOR(*)

DEFINE_V3S_BINARY_OPERATOR(+)
DEFINE_V3S_BINARY_OPERATOR(-)
DEFINE_V3S_BINARY_OPERATOR(/)
DEFINE_V3S_BINARY_OPERATOR(*)

DEFINE_SV3_BINARY_OPERATOR(+)
DEFINE_SV3_BINARY_OPERATOR(-)
DEFINE_SV3_BINARY_OPERATOR(/)
DEFINE_SV3_BINARY_OPERATOR(*)

DEFINE_V3V3_MODIFY_BINARY_OPERATOR(+=)
DEFINE_V3V3_MODIFY_BINARY_OPERATOR(-=)
DEFINE_V3V3_MODIFY_BINARY_OPERATOR(/=)
DEFINE_V3V3_MODIFY_BINARY_OPERATOR(*=)

DEFINE_V3S_MODIFY_BINARY_OPERATOR(+=)
DEFINE_V3S_MODIFY_BINARY_OPERATOR(-=)
DEFINE_V3S_MODIFY_BINARY_OPERATOR(/=)
DEFINE_V3S_MODIFY_BINARY_OPERATOR(*=)

template <class U, class V> 
bool operator==(const LIB_NAMESPACE::math::Vector3<U>& u, const LIB_NAMESPACE::math::Vector3<V>& v) { 
	return u.x == v.x && u.y == v.y && u.z == v.z;
}

template <class U, class V> 
bool operator!=(const LIB_NAMESPACE::math::Vector3<U>& u, const LIB_NAMESPACE::math::Vector3<V>& v) { 
	return u.x != v.x || u.y != v.y || u.z != v.z;
}

template < class V >
std::ostream& operator<<(std::ostream& str, const LIB_NAMESPACE::math::Vector3<V>& u)
{
	str << u.x << " " << u.y << " " << u.z;
	return str;
}

template < class FTy_ >
math::Vector3<FTy_> clamp(const math::Vector3<FTy_>& val, 
								const math::Vector3<FTy_>& minVal, 
								const math::Vector3<FTy_>& maxVal)
{
	return math::Vector3<FTy_>(
		clamp(val.x, minVal.x, maxVal.x),
		clamp(val.y, minVal.y, maxVal.y),
		clamp(val.z, minVal.z, maxVal.z)
		);
}

}

LIB_NAMESPACE_END;

namespace std {;
template < class FTy_ > struct hash< math::Vector3<FTy_> >
{
	size_t operator()(const math::Vector3<FTy_>& vec) const
	{
		return std::hash<FTy_>()(vec.x) ^ std::hash<FTy_>()(vec.y) ^ std::hash<FTy_>()(vec.z);
	}
};
}

#endif // _VECTOR3_HPP