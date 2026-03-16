#ifndef _MATH_PLANE_HPP
#define _MATH_PLANE_HPP

#include "Misc/libdesc.hpp"
#include "Misc/hash.hpp"
#include "vector3.hpp"
#include "vector4.hpp"
#include "misc.hpp"

LIB_NAMESPACE_START

namespace math {;

template < class ValueType >
struct Plane
{
	typedef ValueType value_type;
	typedef Plane< value_type > this_type;
	typedef Vector3< value_type > VectorType;

	static const this_type XNormalPlane;
	static const this_type YNormalPlane;
	static const this_type ZNormalPlane;

	VectorType normal;
	value_type distanceConstant;

	Plane() : distanceConstant(0) {}

	template < class V, class OtherValueType >
	Plane(const Vector3< V >& _normal, OtherValueType _constant) : normal(_normal), distanceConstant(static_cast<value_type>(_constant)) {}

	template < class V, class V2 >
	Plane(const Vector3< V >& _normal, const Vector3< V2 >& _point) : normal(_normal), distanceConstant(static_cast<value_type>(_normal.dotp(_point))) {}

	template < class ATy_, class BTy_, class CTy_, class DTy_ >
	Plane(ATy_ A, BTy_ B, CTy_ C, DTy_ D) : normal(A, B, C)
	{
		ValueType nlen = static_cast<ValueType>(1) / static_cast<ValueType>(normal.length());
		distanceConstant = -D * nlen;
		// renormalize normal
		normal = normal * nlen;
	}

	template < class V, class V2, class V3 >
	Plane(const Vector3< V >& p0, const Vector3< V2 >& p1, const Vector3< V3 >& p2) : normal((p2-p1).cross(p0-p1)) 
	{
		distanceConstant = normal.dotp(p0);
	}

	template < class V >
	value_type distance(const Vector3< V >& p) const
	{
		return value_type(p.dotp(normal)) - distanceConstant;
	}

	Vector4d plane_eq() const
	{
		Vector4d eq;
		double s = 1.0 / normal.length();//sqrt( normal.x * normal.x + normal.y * normal.y + normal.z * normal.z ); 

		Vector3d v(normal * distanceConstant);
		eq.x = normal.x * s; 
		eq.y = normal.y * s; 
		eq.z = normal.z * s; 
		eq.w = -( eq.x * v.x + eq.y * v.y + eq.z * v.z ); 

		return eq;
	}
};

template<class V> const typename Plane<V>::this_type Plane<V>::XNormalPlane = Plane<V>(VectorType::XAxis, VectorType::Zero);
template<class V> const typename Plane<V>::this_type Plane<V>::YNormalPlane = Plane<V>(VectorType::YAxis, VectorType::Zero);
template<class V> const typename Plane<V>::this_type Plane<V>::ZNormalPlane = Plane<V>(VectorType::ZAxis, VectorType::Zero);

typedef Plane<float> Planef;
typedef Plane<double> Planed;

}

LIB_NAMESPACE_END

#endif // _MATH_PLANE_HPP
