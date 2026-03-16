
#if !defined(__RENDER_COMPOSED_GEOMETRYSET_H__)
#define __RENDER_COMPOSED_GEOMETRYSET_H__


#include <vector>
#include "geometryset.h"

namespace render {;

struct ComposedGeometrySet
{
	typedef std::shared_ptr< ComposedGeometrySet > ptr;

	struct GeometrySetOp { enum type {
		Include,
		Exclude
	};};

	struct GeometrySetAndOp
	{
		GeometrySetAndOp(GeometrySet::ptr geometrySet_, GeometrySetOp::type op_)
			: geometrySet(geometrySet_), op(op_) {}

		GeometrySet::ptr geometrySet;
		GeometrySetOp::type op;

		bool operator<(const GeometrySetAndOp& other) const { return geometrySet < other.geometrySet; }
		bool operator==(const GeometrySetAndOp& other) const { return geometrySet == other.geometrySet; }
	};

	typedef std::vector<GeometrySetAndOp> GeometrySetAndOpSet;
	typedef GeometrySetAndOpSet::iterator GeometrySetAndOpIterator;
	typedef GeometrySetAndOpSet::const_iterator GeometrySetAndOpConstIterator;

	void add_geometry_set(GeometrySet::ptr geometrySet, GeometrySetOp::type op = GeometrySetOp::Include)
	{
		assert(std::find(_geometrySets.begin(), _geometrySets.end(), GeometrySetAndOp(geometrySet, op)) == _geometrySets.end());

		_geometrySets.push_back(GeometrySetAndOp(geometrySet, op));
	}

	GeometrySetAndOpIterator begin_geometry_sets() { return _geometrySets.begin(); }
	GeometrySetAndOpIterator end_geometry_sets() { return _geometrySets.end(); }

	GeometrySetAndOpConstIterator begin_geometry_sets() const { return _geometrySets.begin(); }
	GeometrySetAndOpConstIterator end_geometry_sets() const { return _geometrySets.end(); }

private:
	std::vector<GeometrySetAndOp> _geometrySets;
};

}

#endif // __RENDER_COMPOSED_GEOMETRYSET_H__