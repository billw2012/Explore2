#ifndef _SCENE_DRAWABLE_H
#define _SCENE_DRAWABLE_H


#include "Math/aabb.hpp"
#include "matrixtraits.hpp"
#include "coordtraits.hpp"
#include "group.hpp"

namespace scene
{

namespace transform
{

struct Drawable : Group
{
	typedef std::shared_ptr< Drawable > ptr;

private:
	//bool _aabbDirty;
	bool _encloseChildren;

	int _transparencySortHint;

	bool _isFar;

	mutable math::AABBf _aabb;
	mutable math::BoundingSpheref _bsphere;
	mutable bool _aabbNeedsRecalc;

public:
	Drawable(const std::string& name = "unnamedDrawable") 
		: Group(name), _encloseChildren(false), _aabb(), _transparencySortHint(0), _isFar(false), _aabbNeedsRecalc(true) {}

	virtual void clone_data(Group* to) const
	{
		Group::clone_data(to);
		Drawable* typedTo = dynamic_cast<Drawable*>(to);
		if (typedTo != NULL)
			typedTo->dirtyAABB();
	}

	virtual Transform* clone() const
	{
		Drawable* newDrawable = new Drawable(*this);
		clone_data(newDrawable);
		return newDrawable;
	}

	void setEncloseChildren(bool flag)
	{
		_encloseChildren = flag;
	}

	bool encloseChildren() const
	{
		return _encloseChildren;
	}

	void set_far(bool isnear) { _isFar = isnear; }

	bool is_far() const { return _isFar; }

	void setTransparencySortHint(int val) { _transparencySortHint = val; }

	int transparencySortHint() const { return _transparencySortHint; }

	//virtual void recalcAABB() const = 0;
	
	const math::AABBf& aabb() const 
	{
		if(_aabbNeedsRecalc)
			recalcAABB();
		return _aabb;
	}

	const math::BoundingSpheref& bsphere() const
	{
		if(_aabbNeedsRecalc)
			recalcAABB();
		return _bsphere;
	}

	math::BoundingSpheref bsphereGlobal() const
	{
		math::BoundingSpheref bsph = bsphere();
		bsph.create(math::Vector3f(globalise(Transform::vec3_type(bsph.center()))), bsph.radius());
		return bsph;
	}

	void dirtyAABB()
	{
		_aabbNeedsRecalc = true;
	}

	virtual NodeType::type get_node_type() const
	{
		return NodeType::Drawable;
	}

protected:
	math::AABBf& get_aabb() const
	{
		return _aabb;
	}

	virtual void recalcAABB() const
	{
		_aabb.reset();
		for(ConstChildIterator itr = begin(); itr != end(); ++itr)
		{
			Drawable* drawable = dynamic_cast<Drawable*>(itr->get());
			if(drawable)
				_aabb.expand(drawable->aabb());
		}
		_aabbNeedsRecalc = false;
		updateBSphere();
	}

	virtual void updateBSphere() const
	{
		_bsphere.create(_aabb);
	}

};

}

}

#endif // _SCENE_DRAWABLE_H
