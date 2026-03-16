#ifndef _RENDER_RENDERCONTEXT_HPP
#define _RENDER_RENDERCONTEXT_HPP

#include <boost/shared_ptr.hpp>
#include "Scene/camera.hpp"
#include "Math/rectangle.hpp"
#include "GLBase/videomemorymanager.hpp"

namespace render
{

template <
	class _ConstMeshIterator,
	class MatrixFloatType
>
struct DefaultRenderContextTraits
{
	typedef _ConstMeshIterator ConstMeshIterator;
	typedef scene::transform::Camera CameraType;
	typedef math::Rectangle< unsigned int > ViewportType;
};

template < 
	class _ConstMeshIterator,
	class MatrixFloatType,
	class RenderContextTraits = DefaultRenderContextTraits<
		_ConstMeshIterator, MatrixFloatType >
>
struct RenderContext
{
	typedef typename RenderContextTraits::ConstMeshIterator ConstMeshIterator;
	typedef typename RenderContextTraits::CameraType CameraType;
	typedef typename RenderContextTraits::ViewportType ViewportType;

private:
	//typename CameraType::ptr _camera;
	//ConstMeshIterator _beginMeshes, _endMeshes;
	//ViewportType _viewport;
	//VideoMemoryManager::ptr _vmm;


public:

	RenderContext(typename CameraType::ptr camera, 
		const ViewportType& viewport, 
		//VideoMemoryManager::ptr vmm, 
		ConstMeshIterator beginMeshes = ConstMeshIterator(), 
		ConstMeshIterator endMeshes = ConstMeshIterator())
		: _camera(camera), _viewport(viewport),
		_beginMeshes(beginMeshes), _endMeshes(endMeshes) {}

	typename CameraType::ptr camera() const { return _camera; }
	ViewportType viewport() const { return _viewport; }
	ConstMeshIterator beginMeshes() const { return _beginMeshes; }
	ConstMeshIterator endMeshes() const { return _endMeshes; }
	//VideoMemoryManager::ptr vmm() const { return _vmm; }
};

}

#endif // _RENDER_RENDERCONTEXT_HPP