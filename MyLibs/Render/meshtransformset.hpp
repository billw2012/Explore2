#ifndef _RENDER_MESHTRANSFORMSET_H
#define _RENDER_MESHTRANSFORMSET_H


#include <set>
//#include "Scene/meshtransform.hpp"

//namespace render
//{
//
//struct MeshTransformSet
//{
//	typedef std::shared_ptr< MeshTransformSet > ptr;
//
//	typedef std::set< scene::transform::MeshTransform::ptr > MeshTransformSet;
//	typedef MeshTransformSet::size_type size_type;
//	typedef MeshTransformSet::iterator MeshTransformIterator;
//	typedef MeshTransformSet::const_iterator ConstMeshTransformIterator;
//
//private:
//	MeshTransformSet _meshes;
//
//public:
//	template < class Itr >
//	void set(Itr beginItr, Itr endItr)
//	{
//		_meshes.clear();
//		_meshes.insert(beginItr, endItr);
//	}
//
//	MeshTransformIterator begin() { return _meshes.begin(); }
//	MeshTransformIterator end() { return _meshes.end(); }
//	ConstMeshTransformIterator begin() const { return _meshes.begin(); }
//	ConstMeshTransformIterator end() const { return _meshes.end(); }
//
//	void add(scene::transform::MeshTransform::ptr mesh) { _meshes.insert(mesh); }
//
//	void remove(scene::transform::MeshTransform::ptr mesh) { _meshes.erase(mesh); }
//
//	template < class MItr > 
//	void add(MItr beginM, MItr endM) { _meshes.insert(beginM, endM); }
//
//	template < class MItr > 
//	void remove(MItr beginM, MItr endM) { std::for_each(beginM, endM, std::bind(&MeshTransformSet::erase, &_meshes, std::placeholders::_1)); }
//
//	size_type count() const { return _meshes.count(); }
//
//	bool contains(scene::transform::MeshTransform::ptr mesh) const 
//	{
//		return find(mesh) != end();
//	}
//
//	MeshTransformIterator find(scene::transform::MeshTransform::ptr mesh)
//	{
//		return _meshes.find(mesh);
//	}
//	
//	ConstMeshTransformIterator find(scene::transform::MeshTransform::ptr mesh) const
//	{
//		return _meshes.find(mesh);
//	}
//
//	void clear() { _meshes.clear(); }
//};
//
//}

#endif // _RENDER_MESHTRANSFORMSET_H