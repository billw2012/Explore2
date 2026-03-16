#ifndef _MATH_FRUSTUM_HPP
#define _MATH_FRUSTUM_HPP

#include "Misc/libdesc.hpp"
#include "Misc/hash.hpp"
#include "matrix4.hpp"
#include "vector3.hpp"
#include "plane.hpp"

namespace math {;

template < class ValueType >
struct Frustum 
{
	typedef Frustum<ValueType> this_type;
	typedef ValueType value_type;

	typedef Vector3<value_type> vec3_type;
	typedef Plane<value_type> plane_type;

	enum PlaneIndex 
	{
		Left = 0,
		Right,
		Bottom,
		Top,
		Near,
		Far
	};

public:

	Frustum() {}

	Frustum(const math::Matrix4<value_type>& mat) 
	{
		_planes[Left]	= plane_type(mat.m41 + mat.m11, mat.m42 + mat.m12, mat.m43 + mat.m13, mat.m44 + mat.m14);
		_planes[Right]	= plane_type(mat.m41 - mat.m11, mat.m42 - mat.m12, mat.m43 - mat.m13, mat.m44 - mat.m14);
		_planes[Bottom]	= plane_type(mat.m41 + mat.m21, mat.m42 + mat.m22, mat.m43 + mat.m23, mat.m44 + mat.m24);
		_planes[Top]	= plane_type(mat.m41 - mat.m21, mat.m42 - mat.m22, mat.m43 - mat.m23, mat.m44 - mat.m24);
		_planes[Near]	= plane_type(mat.m41 + mat.m31, mat.m42 + mat.m32, mat.m43 + mat.m33, mat.m44 + mat.m34);
		_planes[Far]	= plane_type(mat.m41 - mat.m31, mat.m42 - mat.m32, mat.m43 - mat.m33, mat.m44 - mat.m34);
	}

	const plane_type& get_plane(size_t idx) const { return _planes[idx]; }

private:
	plane_type _planes[6];
};

//template < class ValueType >
//struct Frustum 
//{
//	typedef Frustum<ValueType> this_type;
//	typedef ValueType value_type;
//
//	typedef Vector3<value_type> VectorType;
//
//private:
//	VectorType _forward;
//	VectorType _right;
//	VectorType _up;
//	VectorType _eyePos;
//	value_type _rFactor;
//	value_type _uFactor;
//	value_type _nearZ;
//	value_type _farZ;
//
//public:
//
//	Frustum() : _rFactor(0.0), _uFactor(0.0), _nearZ(0.0), _farZ(0.0) {}
//
//	Frustum(const VectorType& forward, const VectorType& right, const VectorType& up, 
//		const VectorType& eyePos, value_type fovv, value_type fovh, /*value_type viewAspect, */value_type nearZ, value_type farZ) 
//		: _forward(forward), _right(right), _up(up), _eyePos(eyePos), 
//		_uFactor(static_cast<value_type>(tan(deg_to_rad(fovv * value_type(0.5))))), 
//		_rFactor(static_cast<value_type>(tan(deg_to_rad(fovh * value_type(0.5))))), 
//		_nearZ(nearZ), _farZ(farZ)
//	{
//	}
//
//	// for an eye space fustrum
//	Frustum(value_type fovv, value_type fovh, value_type nearZ, value_type farZ) 
//		: _forward(-VectorType::ZAxis), _right(VectorType::XAxis), _up(VectorType::YAxis), 
//		_uFactor(static_cast<value_type>(tan(deg_to_rad(fovv * value_type(0.5))))), 
//		_rFactor(static_cast<value_type>(tan(deg_to_rad(fovh * value_type(0.5))))), 
//		_nearZ(nearZ), _farZ(farZ)
//	{
//	}
//
//	const VectorType& forward() const { return _forward; }
//	const VectorType& right() const { return _right; }
//	const VectorType& up() const { return _up; }
//	const VectorType& eyePos() const { return _eyePos; }
//	value_type rFactor() const { return _rFactor; }
//	value_type uFactor() const { return _uFactor; }
//	value_type nearZ() const { return _nearZ; }
//	value_type farZ() const { return _farZ; }
//};

typedef Frustum<float> Frustumf;
typedef Frustum<double> Frustumd;

}

#endif // _MATH_FRUSTUM_HPP
