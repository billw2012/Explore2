#ifndef _AABB_HPP
#define _AABB_HPP

#include <vector>
#include "Misc/libdesc.hpp"
#include "Misc/hash.hpp"
#include "vector3.hpp"
#include "misc.hpp"

#undef max
#undef min

LIB_NAMESPACE_START

namespace math
{

template < class ValueType >
struct AABB 
{
	typedef AABB<ValueType> this_type;
	typedef ValueType value_type;

	typedef Vector3<value_type> VectorType;
	template<class U> friend struct AABB;
private:
	VectorType _min, _max;

public:

	AABB() : _min(VectorType::Max), _max(VectorType::Min) {}

	template < class V >
	AABB(const Vector3<V>& min, const Vector3<V>& max) : _min(min), _max(max) { }

	template < class V >
	AABB(const AABB<V>& other) : _min(VectorType(other._min)), _max(VectorType(other._max)) { }

	// Create box from min and max vectors
	template < class V >
	void create(const Vector3<V>& min, const Vector3<V>& max)
	{
		_min = min;
		_max = max;
	}

	void reset()
	{
		_min = VectorType::Max;
		_max = VectorType::Min;
	}

	// Expand the box to contain vec if required
	template < class V >
	void expand(const Vector3<V>& vec)
	{
		if(vec.x < _min.x)	_min.x = vec.x;
		if(vec.x >  _max.x)	_max.x = vec.x;
		if(vec.y < _min.y)	_min.y = vec.y;
		if(vec.y >  _max.y)	_max.y = vec.y;
		if(vec.z < _min.z)	_min.z = vec.z;
		if(vec.z >  _max.z)	_max.z = vec.z;
	}

	// Expand the box to contain vec if required
	template < class V >
	void expand(const AABB<V>& aabb)
	{
		expand(aabb.min());
		expand(aabb.max());
	}

	const VectorType& min() const
	{
		return _min;
	}

	const VectorType& max() const
	{
		return _max;
	}

	VectorType center() const
	{
		return (_min + _max) * static_cast<value_type>(0.5);
	}

	VectorType extents() const
	{
		return _max - _min;
	}

	template < class V > 
	bool contains(const Vector3<V>& vec) const
	{
		return vec.x >= _min.x && vec.x <= _max.x &&
			vec.y >= _min.y && vec.y <= _max.y &&
			vec.z >= _min.z && vec.z <= _max.z;
	}

	template < class V > 
	bool contains_conservative(V x, V y, V z) const
	{
		return x >= _min.x && x < _max.x &&
			y >= _min.y && y < _max.y &&
			z >= _min.z && z < _max.z;
	}

	template < class V > 
	bool contains(const AABB<V>& aabb) const
	{
		return contains(aabb._min) && contains(aabb._max);
	}

	template < class V >
	void getVertices(std::vector< Vector3<V> > &vertices) const
	{
		vertices.resize(8);
		vertices[0] = _min;
		vertices[1] = Vector3<V>(_min.x, _min.y, _max.z);
		vertices[2] = Vector3<V>(_min.x, _max.y, _max.z);
		vertices[3] = Vector3<V>(_min.x, _max.y, _min.z);
		vertices[4] = Vector3<V>(_max.x, _min.y, _min.z);
		vertices[5] = Vector3<V>(_max.x, _min.y, _max.z);
		vertices[6] = _max;
		vertices[7] = Vector3<V>(_max.x, _max.y, _min.z);
	}

	template < class V >
	Vector3<V> closest_point(const Vector3<V>& vert) const
	{
		Vector3<V> closePt;

		for(size_t idx = 0; idx < 3; ++idx)
			closePt(idx) = clamp(vert[idx], _min[idx], _max[idx]);

		return closePt;
	}
};

typedef AABB<float> AABBf;
typedef AABB<double> AABBd;
}

LIB_NAMESPACE_END

#endif // _AABB_HPP
