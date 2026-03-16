#ifndef __SCENE_MESH_HPP__
#define __SCENE_MESH_HPP__

#include "vertexset.hpp"
#include "vertexspec.hpp"
#include "triangleset.hpp"
#include "material.hpp"

namespace scene {;

struct Mesh
{
	typedef std::shared_ptr< Mesh > ptr;

	typedef std::set< TriangleSet::ptr > TriangleSetSet;
	typedef TriangleSetSet::iterator TriangleSetIterator;
	typedef TriangleSetSet::const_iterator ConstTriangleSetIterator;

private:
	TriangleSetSet _tris;
	Material::ptr _material;
	mutable math::AABBf _aabb;
	mutable bool _aabbNeedsRecalc;
	
public:
	Mesh() : _material(), _tris(), _aabbNeedsRecalc(true) {}
	Mesh(Material::ptr material) : _material(material), _tris(), _aabbNeedsRecalc(true) {}

	const math::AABBf& aabb() const 
	{
		if(_aabbNeedsRecalc)
			recalcAABB();
		return _aabb;
	}

	void dirtyAABB()
	{
		_aabbNeedsRecalc = true;
	}

	void addTris(TriangleSet::ptr tris) {	_tris.insert(tris); dirtyAABB(); }
	void removeTris(TriangleSet::ptr tris) { _tris.erase(tris); dirtyAABB();  }
	void clearTris() { _tris.clear(); dirtyAABB();  }
	TriangleSetSet::size_type trisCount() const { return _tris.size(); }

	ConstTriangleSetIterator beginTris() const	{ return _tris.begin(); }
	TriangleSetIterator beginTris()	{ return _tris.begin(); }
	ConstTriangleSetIterator endTris() const { return _tris.end(); }
	TriangleSetIterator endTris() { return _tris.end(); }

	Material::ptr material() const { return _material; }

	void setMaterial(Material::ptr mat) { _material = mat; }

protected:
	math::AABBf& get_aabb() const
	{
		return _aabb;
	}

	void recalcAABB() const
	{
		for(ConstTriangleSetIterator ti = beginTris(); ti != endTris(); ++ti)
		{
			get_aabb().expand((*ti)->aabb());
		}
	}
};

}
#endif // __SCENE_MESH_HPP__