
#if !defined (__RENDER_GEOMETRYSET_H__)
#define __RENDER_GEOMETRYSET_H__


#include <unordered_set>
#include "Scene/geometry.hpp"

namespace render {;

struct GeometrySet : public std::unordered_set< scene::Geometry::ptr >
{
	typedef std::shared_ptr< GeometrySet > ptr;
	bool contains(scene::Geometry::ptr geometry) const
	{
		return find(geometry) != end();
	}
};
typedef GeometrySet::iterator GeometryIterator;
typedef GeometrySet::const_iterator ConstGeometryIterator;

}

#endif // __RENDER_GEOMETRYSET_H__