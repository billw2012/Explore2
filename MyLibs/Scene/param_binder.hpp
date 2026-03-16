#if !defined(__RENDER_PARAM_BINDER_HPP__)
#define __RENDER_PARAM_BINDER_HPP__

#include "material.hpp"
// #include "shader.hpp"

using namespace std::placeholders;

namespace scene
{

namespace transform
{
	struct MeshTransform;
	struct Camera;
};

struct ParamBinder
{
	typedef std::shared_ptr< ParamBinder > ptr;

	virtual void apply(transform::MeshTransform* meshTrans, transform::Camera* camera) = 0;
};

struct ParamBinders
{
	typedef std::shared_ptr< ParamBinders > ptr;

	typedef std::set< ParamBinder::ptr > ParamBinderSet;
	typedef ParamBinderSet::iterator ParamBinderIterator;
	typedef ParamBinderSet::const_iterator ConstParamBinderIterator;
	
	void insert(ParamBinder::ptr binder) { _binders.insert(binder); }
	void remove(ParamBinder::ptr binder) { _binders.erase(binder); }

	ConstParamBinderIterator begin() const { return _binders.begin(); }
	ConstParamBinderIterator end() const { return _binders.end(); }

	ParamBinderIterator begin() { return _binders.begin(); }
	ParamBinderIterator end() { return _binders.end(); }

	void apply(transform::MeshTransform* meshTrans, transform::Camera* camera)
	{
		std::for_each(_binders.begin(), _binders.end(), std::bind(&ParamBinder::apply, std::placeholders::_1, meshTrans, camera));
	}

private:
	ParamBinderSet _binders;
};

}

#endif // __RENDER_PARAM_BINDER_HPP__