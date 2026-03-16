#ifndef _RENDER_RENDERCONTEXT_HPP
#define _RENDER_RENDERCONTEXT_HPP

#include <boost/shared_ptr.hpp>
#include "Scene/camera.hpp"
#include "Math/rectangle.hpp"
#include "videomemorymanager.hpp"

namespace render {;

struct GBuffer
{
	typedef boost::shared_ptr<GBuffer> ptr;

	FramebufferObject::ptr fbo;
	Texture::ptr albedoBuffer, normalBuffer, specularBuffer, positionBuffer,
		depthBuffer;

	void init(GLuint width, GLuint height);
};

struct PBuffer
{
	typedef boost::shared_ptr<PBuffer> ptr;

	FramebufferObject::ptr fbo;
	Texture::ptr texture;

	void init(GLuint width, GLuint height);
};

struct RenderContextFactory;

// struct RenderContext
// {
// 	friend struct RenderContextFactory;
// 
// 	typedef boost::shared_ptr<RenderContext> ptr;
// 
// 	scene::transform::Camera::ptr get_main_camera() const 
// 	{
// 		return _mainCamera;
// 	}
// 
// 	void init(int width, int height, double fov);
// private:
// 	GBuffer::ptr _gBuffer;
// 	PBuffer::ptr _pBuffer0, _pBuffer1;
// 
// 	render::GeometryRenderStage::ptr _mainRender, _shadowingFBO,
// 		_renderAtmosphereAttenuateBufferStage, 
// 		_renderAtmosphereScatteringBufferStage, _renderPBufferStage;
// 	render::LightingRenderStage::ptr _lightingStage;
// 
// 	Texture::ptr _gAlbedoBuffer, _gNormalBuffer, _gSpecularBuffer,
// 		_gPositionBuffer, _gDepthBuffer;
// 	render::SceneContext::ptr _sceneContext;
// 	scene::transform::Camera::ptr _mainCamera;
// 	scene::Viewport::ptr _viewport;
// };

struct RenderContext
{
	friend struct RenderContextFactory;

	typedef boost::shared_ptr<RenderContext> ptr;

	scene::transform::Camera::ptr get_main_camera() const 
	{
		return _mainCamera;
	}

	void init(int width, int height, double fov);

private:
	GBuffer::ptr _gBuffer;
	PBuffer::ptr _pBuffer0, _pBuffer1;

	render::GeometryRenderStage::ptr _mainRender, 
		_renderAtmosphereAttenuateBufferStage, 
		_renderAtmosphereScatteringBufferStage, _renderPBufferStage;
	render::LightingRenderStage::ptr _lightingStage;

	FramebufferObject::ptr _shadowingFBO;
	glbase::Texture::ptr _shadowDepthBuffer;

	Texture::ptr _gAlbedoBuffer, _gNormalBuffer, _gSpecularBuffer,
		_gPositionBuffer, _gDepthBuffer;

	render::SceneContext::ptr _sceneContext;
	scene::transform::Camera::ptr _mainCamera;
	scene::Viewport::ptr _viewport;

	scene::VertexSpec::ptr _backPlaneVertexSpec;
	scene::TriangleSet::ptr _backPlaneTriSet;
	scene::Material::ptr _backPlaneMaterial;

	effect::Effect::ptr _backPlaneEffect;

	effect::Effect::ptr _solarLightEffect, _solarLightShadowEffect,
		_atmosphereAttenuateEffect, _atmosphereScatteringEffect;

	effect::Effect::ptr _renderPBufferEffect;

	RenderGL* _renderer;
};

// struct RenderContextFactory
// {
// 	static void create_instance();
// 
// 	static RenderContextFactory* get_instance();
// 
// 	RenderContext::ptr create_context() const;
// 
// 	bool init(RenderGL* renderer);
// 
// private:
// 	static RenderContextFactory* _instance;
// 
// 	scene::TriangleSet::ptr _backPlaneTriSet;
// 	scene::Material::ptr _backPlaneMaterial;
// 	effect::Effect::ptr _backPlaneEffect, _solarLightEffect,
// 		_atmosphereAttenuateEffect, _atmosphereScatteringEffect,
// 		_renderPBufferEffect;
// 
// 	glbase::Texture::ptr _shadowDepthBuffer;
// 
// 	scene::VertexSpec::ptr _backPlaneVertexSpec;
// 
// 	RenderGL* _renderer;
// 
// };

}

#endif // _RENDER_RENDERCONTEXT_HPP