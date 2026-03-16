#ifndef _VECTOR2_HPP
#define _VECTOR2_HPP

#include "Misc/libdesc.hpp"
#include "Misc/hash.hpp"
#include <ostream>

namespace math {;

template < class V > struct Vector3;
template < class V > struct Vector4;

template < class ValueType >
struct Vector2
{
	typedef Vector2<ValueType> this_type;
	typedef ValueType value_type;
	typedef unsigned int index_type;

	value_type x;
	value_type y;

	static const this_type XAxis;
	static const this_type YAxis;
	static const this_type Zero;
	static const this_type One;
	static const this_type Min;
	static const this_type Max;

	// Construct default vector (0, 0)
	Vector2() : x(static_cast<value_type>(0)), y(static_cast<value_type>(0)) {}
	// Construct vector using specified values
	template < class OtherType >
	Vector2(OtherType xi, OtherType yi) : x(static_cast<value_type>(xi)), y(static_cast<value_type>(yi)) {}
	// Construct vector using specified values array
	template < class OtherType >
	Vector2(const OtherType* const coordinates) 
		: x(coordinates[0]), y(coordinates[1]) {}
	// Copy constructor
	Vector2(const this_type& vector) : x(vector.x), y(vector.y) {}
	template < class V >
	explicit Vector2(const Vector2<V>& vector) : x(static_cast<value_type>(vector.x)), y(static_cast<value_type>(vector.y)) {}
	template < class V >
	explicit Vector2(const Vector3<V>& vector) : x(static_cast<value_type>(vector.x)), y(static_cast<value_type>(vector.y)) {}
	template < class V >
	explicit Vector2(const Vector4<V>& vector) : x(static_cast<value_type>(vector.x)), y(static_cast<value_type>(vector.y)) {}

	template < class V >
	value_type dotp(const Vector2<V>& v) const
	{
		return (x * v.x) + (y * v.y);
	}

	void normalize()
	{
		value_type len = length();
		if (len != 0)
		{
			value_type invLen = static_cast<value_type>(1) / len;
			x *= invLen;
			y *= invLen;
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
		return (x * x + y * y);
	}

	value_type length_manhattan() const
	{
		return std::abs(x) + std::abs(y);
	}

	value_type operator[](index_type index) const
	{
		return *(&x + index);
	}

	value_type& operator()(index_type index) 
	{
		return *(&x + index);
	}

	bool operator<(const this_type& other) const
	{
		if(x < other.x)
			return true;
		if(x > other.x)
			return false;
		return y < other.y;
	}

};

template< class V > const typename Vector2<V>::this_type Vector2<V>::XAxis = Vector2<V>(V(1), V(0));
template< class V > const typename Vector2<V>::this_type Vector2<V>::YAxis = Vector2<V>(V(0), V(1));
template< class V > const typename Vector2<V>::this_type Vector2<V>::Zero = Vector2<V>(V(0), V(0));
template< class V > const typename Vector2<V>::this_type Vector2<V>::One = Vector2<V>(V(1), V(1));
template< class V > const typename Vector2<V>::this_type Vector2<V>::Min = Vector2<V>(-std::numeric_limits<V>::max(), -std::numeric_limits<V>::max());
template< class V > const typename Vector2<V>::this_type Vector2<V>::Max = Vector2<V>(std::numeric_limits<V>::max(), std::numeric_limits<V>::max());

typedef Vector2<float> Vector2f;
typedef Vector2<double> Vector2d;
typedef Vector2<int> Vector2i;
typedef Vector2<unsigned int> Vector2u;

#define DEFINE_V2_UNARY_OPERATOR(op) template <class U> \
	LIB_NAMESPACE::math::Vector2<U> operator##op(const LIB_NAMESPACE::math::Vector2<U>& u) { \
		return LIB_NAMESPACE::math::Vector2<U>(op u.x, op u.y); \
	}

#define DEFINE_V2V2_BINARY_OPERATOR(op)	template <class U, class V> \
	LIB_NAMESPACE::math::Vector2<U> operator##op(const LIB_NAMESPACE::math::Vector2<U>& u, const LIB_NAMESPACE::math::Vector2<V>& v) { \
	return LIB_NAMESPACE::math::Vector2<U>(u.x op v.x, u.y op v.y); \
}

#define DEFINE_V2S_BINARY_OPERATOR(op) template <class U, class V> \
	LIB_NAMESPACE::math::Vector2<U> operator##op(const LIB_NAMESPACE::math::Vector2<U>& u, V v) { \
	return LIB_NAMESPACE::math::Vector2<U>(u.x op v, u.y op v); \
}

#define DEFINE_SV2_BINARY_OPERATOR(op) template <class U, class V> \
	LIB_NAMESPACE::math::Vector2<U> operator##op(V v, const LIB_NAMESPACE::math::Vector2<U>& u) { \
	return LIB_NAMESPACE::math::Vector2<U>(v op u.x, v op u.y); \
}

#define DEFINE_V2V2_MODIFY_BINARY_OPERATOR(op) template <class U, class V> \
	LIB_NAMESPACE::math::Vector2<U>& operator##op(LIB_NAMESPACE::math::Vector2<U>& u, const LIB_NAMESPACE::math::Vector2<V>& v) { \
	u.x op v.x; u.y op v.y; return u; \
}

#define DEFINE_V2S_MODIFY_BINARY_OPERATOR(op) template <class U, class V> \
	LIB_NAMESPACE::math::Vector2<U>& operator##op(LIB_NAMESPACE::math::Vector2<U>& u, V v) { \
	u.x op v; u.y op v; return u; \
}

DEFINE_V2_UNARY_OPERATOR(-)

DEFINE_V2V2_BINARY_OPERATOR(+)
DEFINE_V2V2_BINARY_OPERATOR(-)
DEFINE_V2V2_BINARY_OPERATOR(/)
DEFINE_V2V2_BINARY_OPERATOR(*)

DEFINE_V2S_BINARY_OPERATOR(+)
DEFINE_V2S_BINARY_OPERATOR(-)
DEFINE_V2S_BINARY_OPERATOR(/)
DEFINE_V2S_BINARY_OPERATOR(*)

DEFINE_SV2_BINARY_OPERATOR(+)
DEFINE_SV2_BINARY_OPERATOR(-)
DEFINE_SV2_BINARY_OPERATOR(/)
DEFINE_SV2_BINARY_OPERATOR(*)

DEFINE_V2V2_MODIFY_BINARY_OPERATOR(+=)
DEFINE_V2V2_MODIFY_BINARY_OPERATOR(-=)
DEFINE_V2V2_MODIFY_BINARY_OPERATOR(/=)
DEFINE_V2V2_MODIFY_BINARY_OPERATOR(*=)

DEFINE_V2S_MODIFY_BINARY_OPERATOR(+=)
DEFINE_V2S_MODIFY_BINARY_OPERATOR(-=)
DEFINE_V2S_MODIFY_BINARY_OPERATOR(/=)
DEFINE_V2S_MODIFY_BINARY_OPERATOR(*=)

template <class U, class V> 
bool operator==(const LIB_NAMESPACE::math::Vector2<U>& u, const LIB_NAMESPACE::math::Vector2<V>& v) { 
	return u.x == v.x && u.y == v.y;
}

template <class U, class V> 
bool operator!=(const LIB_NAMESPACE::math::Vector2<U>& u, const LIB_NAMESPACE::math::Vector2<V>& v) { 
	return u.x != v.x || u.y != v.y;
}

template < class V >
std::ostream& operator<<(std::ostream& str, const LIB_NAMESPACE::math::Vector2<V>& u)
{
	str << u.x << " " << u.y;
	return str;
}


template < class FTy_ >
math::Vector2<FTy_> clamp(const math::Vector2<FTy_>& val, const math::Vector2<FTy_>& minVal, const math::Vector2<FTy_>& maxVal)
{
	return math::Vector2<FTy_>(
		clamp(val.x, minVal.x, maxVal.x),
		clamp(val.y, minVal.y, maxVal.y)
		);
}

}

namespace std {;
template < class FTy_ > struct hash< math::Vector2<FTy_> >
{
	size_t operator()(const math::Vector2<FTy_>& vec) const
	{
		return std::hash<FTy_>()(vec.x) ^ std::hash<FTy_>()(vec.y);
	}
};
}

#endif // _VECTOR2_HPP