#ifndef __SCENE_MESH_TRANSFORM_HPP__
#define __SCENE_MESH_TRANSFORM_HPP__

#include "vertexset.hpp"
#include "vertexspec.hpp"
#include "triangleset.hpp"
#include "material.hpp"
#include "mesh.hpp"
#include "param_binder.hpp"
#include "drawable.hpp"

namespace scene
{

namespace transform
{

struct MeshTransform : Drawable
{
	typedef std::shared_ptr< MeshTransform > ptr;

	MeshTransform(const std::string& name = "unnamedMeshTransform") : Drawable(name), _mesh() {}
	MeshTransform(Mesh::ptr mesh, const std::string& name = "unnamedMeshTransform") : Drawable(name), _mesh(mesh) {}
	
	virtual Transform* clone() const
	{
		MeshTransform* newMeshTransform = new MeshTransform(*this);
		clone_data(newMeshTransform);
		return newMeshTransform;
	}

	Mesh::ptr get_mesh() const { return _mesh; }
	void set_mesh(Mesh::ptr mesh) { _mesh = mesh; dirtyAABB(); }

	virtual NodeType::type get_node_type() const { return NodeType::MeshTransform; }

	template < class InitCloneFn >
	void clone_meshes_r(InitCloneFn fn)
	{
		if(_mesh != NULL)
		{
			Mesh* clone = new Mesh(*_mesh);
			fn(this, _mesh.get(), clone);
			_mesh.reset(clone);
		}
		for(ConstChildIterator cItr = begin(); cItr != end(); ++cItr)
		{
			MeshTransform* meshTransformChild = dynamic_cast<MeshTransform*>(cItr->get());
			if(meshTransformChild != NULL)
			{
				meshTransformChild->clone_meshes_r(fn);
			}
		}
	}

	void set_param_binders(ParamBinders::ptr binders)
	{
		_paramBinders = binders;
	}

	ParamBinders::ptr get_param_binders() const
	{
		return _paramBinders;
	}

protected:
	void recalcAABB() const
	{
		Drawable::recalcAABB();
		if(_mesh != NULL)
			get_aabb().expand(_mesh->aabb());
		updateBSphere();
	}

private:
	Mesh::ptr _mesh;
	ParamBinders::ptr _paramBinders;
};

}

}
#endif // __SCENE_MESH_TRANSFORM_HPP__