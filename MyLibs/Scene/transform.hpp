#ifndef _SCENE_TRANSFORM_HPP
#define _SCENE_TRANSFORM_HPP


#include "matrixtraits.hpp"
#include "Math/vector3.hpp"
#include "Math/vector4.hpp"
#include "Math/matrix4.hpp"
#include "Math/boundingsphere.hpp"
#include "Math/aabb.hpp"
#include "scene_types.hpp"
#include "animation.hpp"
#include "Math/highprec.h"

namespace scene {;

namespace transform {;

struct Transform : public std::enable_shared_from_this<Transform>
{
	typedef std::shared_ptr< Transform > ptr;

	// can't really use HighPrecFloat as it is not thread safe!!!
	typedef double								float_type;
	typedef math::Vector3<float_type>			vec3_type; 
	typedef math::Vector4<float_type>			vec4_type; 
	typedef math::Matrix3<float_type>			matrix3_type; 
	typedef math::Matrix4<float_type>			matrix4_type; 
	typedef math::BoundingSphere<float_type>	bsph_type; 
	typedef math::AABB<float_type>				aabb_type; 

private:
	matrix4_type _transform;
	mutable bool _globalTransformNeedsRecalc, _globalTransformInverseNeedsRecalc;
	mutable matrix4_type _globalTransform;
	mutable matrix4_type _globalTransformInverse;
	Transform* _parent;
	std::string _name;
	AnimatedTransform::ptr _anim;

public:

	Transform(const std::string& name = "unnamedTransform") : _parent(nullptr), _transform(), _globalTransform(), _globalTransformNeedsRecalc(false), _globalTransformInverseNeedsRecalc(false), _name(name) {}

	virtual void clone_data(Transform* to) const 
	{ 
		to->set_parent(nullptr); 
	}

	virtual Transform* clone() const
	{
		Transform* newTransform = new Transform(*this);
		clone_data(newTransform);
		return newTransform;
	}

	void set_name(const std::string& name) { _name = name; }
	const std::string& get_name() const { return _name; }

	const matrix4_type& transform() const { return _transform; }
	virtual void setTransform(const matrix4_type& trans) 
	{ 
		_transform = trans; 
		dirtyGlobalTransform();
	}

	const Transform* parent() const { return _parent; }
	Transform* parent() { return _parent; }

	virtual void set_parent(Transform* parent) 
	{ 
		_parent = parent;
		dirtyGlobalTransform();
	}

	const matrix4_type& globalTransform() const 
	{ 
		//if(_globalTransformNeedsRecalc)
			recalcGlobalTransform();
		return _globalTransform; 
	}

	const matrix4_type& globalTransformInverse() const
	{
		//if(_globalTransformNeedsRecalc)
			recalcGlobalTransform();
		if(_globalTransformInverseNeedsRecalc)
		{
			_globalTransformInverse = _globalTransform.inverse();
			_globalTransformInverseNeedsRecalc = false;
		}
		return _globalTransformInverse;
	}

	virtual void setGlobalTransform(const matrix4_type& trans) 
	{
		 _globalTransform = trans;
		 if(_parent != nullptr)
			_transform = _globalTransform * _parent->globalTransform().inverse();
		 else
			_transform = _globalTransform;
		 _globalTransformInverseNeedsRecalc = true;
	}

	virtual void dirtyGlobalTransform() 
	{
		_globalTransformNeedsRecalc = true;
	}

	vec3_type localise(const vec3_type& pt) const
	{
		return vec3_type(globalTransformInverse() * (vec4_type(pt) + vec4_type::WAxis));
	}

	matrix4_type localise(const matrix4_type& mat) const
	{
		return Transform::matrix4_type(mat * globalTransformInverse());
	}

	math::BoundingSphered localise(const math::BoundingSphered& bsph) const
	{
		return math::BoundingSphered(math::Vector3d(localise(vec3_type(bsph.center()))), bsph.radius());
	}

	vec3_type localiseV(const vec3_type& pt) const
	{
		return vec3_type(globalTransformInverse() * vec4_type(pt));
	}

	vec3_type globalise(const vec3_type& pt) const
	{
		return vec3_type(globalTransform() * (vec4_type(pt) + vec4_type::WAxis));
	}

	matrix4_type globalise(const matrix4_type& mat) const
	{
		return matrix4_type(mat * globalTransform());
	}

	bsph_type globalise(const bsph_type& bsph) const
	{
		return bsph_type(globalise(vec3_type(bsph.center())), bsph.radius());
	}

	bsph_type get_global_bsphere(const aabb_type& aabb) const 
	{
		vec3_type globalCenter = globalise(aabb.center());
		vec3_type globalMin = globalise(aabb.min());
		return bsph_type(globalCenter, (globalMin - globalCenter).length());
	}

	vec3_type globaliseV(const vec3_type& pt) const
	{
		return vec3_type(globalTransform() * vec4_type(pt));
	}

	vec3_type forward() const
	{
		return vec3_type(-transform().getColumnVector(2));
	}

	vec3_type right() const
	{
		return vec3_type(transform().getColumnVector(0));
	}

	vec3_type up() const
	{
		return vec3_type(transform().getColumnVector(1));
	}

	vec3_type forwardGlobal() const
	{
		return vec3_type(-globalTransform().getColumnVector(2));
	}

	vec3_type rightGlobal() const
	{
		return vec3_type(globalTransform().getColumnVector(0));
	}

	vec3_type upGlobal() const
	{
		return vec3_type(globalTransform().getColumnVector(1));
	}

	vec3_type center() const
	{
		return vec3_type(transform().getColumnVector(3));
	}

	vec3_type centerGlobal() const
	{
		return vec3_type(globalTransform().getColumnVector(3));
	}

	virtual NodeType::type get_node_type() const
	{
		return NodeType::Transform;
	}

	void set_anim(AnimatedTransform::ptr anim)
	{
		_anim = anim;
	}

	AnimatedTransform::ptr get_anim() const 
	{
		return _anim;
	}

	virtual void set_t(float t)
	{
		if(_anim)
		{
			_transform = matrix4_type(_anim->get_trans(t));
			_globalTransformNeedsRecalc = true;
		}
	}
	
	virtual AnimatedTransform::TimeRange get_anim_range() const
	{
		if(_anim)
			return _anim->get_range();
		return AnimatedTransform::TimeRange();
	}

private:
	void recalcGlobalTransform() const
	{
		const Transform* pParent = _parent;
		while(pParent)
		{
			_globalTransformNeedsRecalc = _globalTransformNeedsRecalc || pParent->_globalTransformNeedsRecalc;
			pParent = pParent->_parent;
		}
		if(_parent != NULL)
			_globalTransform = _parent->globalTransform() * _transform;
		else
			_globalTransform = _transform;
		_globalTransformNeedsRecalc = false;
		_globalTransformInverseNeedsRecalc = true;
	}
};

}

}
#endif // _SCENE_TRANSFORM_HPP