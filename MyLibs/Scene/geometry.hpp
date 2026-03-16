#if !defined __SCENE_TRIANGLESET_VERTS_PAIR_HPP__
#define __SCENE_TRIANGLESET_VERTS_PAIR_HPP__

#include "glbase/triangleset.hpp"
#include "glbase/vertexset.hpp"
#include "transform.hpp"
#include "material.hpp"
#include "math/boundingsphere.hpp"
#include "math/aabb.hpp"

namespace scene {;

struct Geometry
{
	typedef std::shared_ptr< Geometry > ptr;

	struct Flags { enum type {
		None			= 0,
		CastShadows		= 1 << 0
	};};

	Geometry() : _flags(Flags::None), _triSetChangeNum(0), _vertsChangeNum(0) {}

	Geometry(const glbase::TriangleSet::ptr& triSet, const glbase::VertexSet::ptr& verts, 
		const Material::ptr& material, const transform::Transform::ptr& trans)
		: _verts(verts), 
		_triSet(triSet), 
		_material(material), 
		_transform(trans),
		_flags(Flags::None), 
		_triSetChangeNum(0), 
		_vertsChangeNum(0)
	{}

	// set chunks to shadow, add fade of shadowing to light shader, add ambient pass, requiring
	// float 32 bit pbuffer target, add @expose@ stage
	void set_verts(const glbase::VertexSet::ptr& verts)
	{
		_verts = verts;
	}

	const glbase::VertexSet::ptr& get_verts() const
	{
		return _verts;
	}

	void set_tris(const glbase::TriangleSet::ptr& triSet)
	{
		_triSet = triSet;
	}

	const glbase::TriangleSet::ptr& get_tris() const
	{
		return _triSet;
	}

	void set_material(const Material::ptr& material)
	{
		_material = material;
	}

	const Material::ptr& get_material() const
	{
		return _material;
	}

	void set_transform(const transform::Transform::ptr& trans)
	{
		_transform = trans;
	}

	const transform::Transform::ptr& get_transform() const
	{
		return _transform;
	}

	const math::AABBf& get_aabb() const 
	{
		if(does_aabb_need_recalc())
			recalc_aabb();
		return _aabb;
	}

	const math::BoundingSpheref& get_bsphere() const
	{
		if(does_aabb_need_recalc())
			recalc_aabb();
		return _bsph;
	}

	bool is_valid() const
	{
		return _material && _transform && _triSet && _verts &&
			_triSet->get_count() > 0;
	}

	void recalc_aabb() const
	{
		if(!_triSet || !_verts) 
		{
			_aabb = math::AABBf(math::Vector3f::Zero, math::Vector3f::Zero);
		}
		else
		{
			_aabb.reset();
			for(size_t idx = 0; idx < _triSet->get_count(); ++idx)
			{
				_aabb.expand(_verts->getPositionf((*_triSet)[idx]));
			}
		}
		_bsph.create(_aabb);

		if(!_triSet)
			_triSetChangeNum = 0;
		else
			_triSetChangeNum = _triSet->get_change_num();

		if(!_verts)
			_vertsChangeNum = 0;
		else
			_vertsChangeNum = _verts->get_change_num();
	}

	bool does_aabb_need_recalc() const
	{
		return (!_triSet && _triSetChangeNum != 0) ||
			(!_verts && _vertsChangeNum != 0) || 
			(_triSet && _triSetChangeNum != _triSet->get_change_num()) ||
			(_verts && _vertsChangeNum != _verts->get_change_num());
	}


	Flags::type get_flags() const { return _flags; }
	void set_flag(Flags::type flag) { _flags = static_cast<Flags::type>(static_cast<unsigned int>(_flags) | static_cast<unsigned int>(flag)); }
	void unset_Flag(Flags::type flag) { _flags = static_cast<Flags::type>(static_cast<unsigned int>(_flags) & ~static_cast<unsigned int>(flag)); }
	bool is_flag_set(Flags::type flag) const { return (static_cast<unsigned int>(_flags) & static_cast<unsigned int>(flag)) != 0; }

private:	
	glbase::TriangleSet::ptr _triSet;
	glbase::VertexSet::ptr _verts;
	transform::Transform::ptr _transform;
	Material::ptr _material;
	mutable math::AABBf _aabb;
	mutable math::BoundingSpheref _bsph;
	mutable size_t _triSetChangeNum, _vertsChangeNum;
	Flags::type _flags;
};

inline Geometry::Flags::type operator|(Geometry::Flags::type f1, Geometry::Flags::type f2)
{
	return static_cast<Geometry::Flags::type>(static_cast<unsigned int>(f1) | static_cast<unsigned int>(f2));
}

}
#endif // __SCENE_TRIANGLESET_VERTS_PAIR_HPP__
