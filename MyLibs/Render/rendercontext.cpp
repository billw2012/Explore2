
#include "rendercontext.h"

namespace render {;

// RenderContextFactory* RenderContextFactory::_instance = NULL;
// 
// void RenderContextFactory::create_instance()
// {
// 	_instance = new RenderContextFactory();
// }
// 
// RenderContextFactory* RenderContextFactory::get_instance()
// {
// 	return _instance;
// }

void GBuffer::init(GLuint width, GLuint height)
{
	fbo.reset(new FramebufferObject());

	albedoBuffer.reset(new Texture("albedoBuffer"));
	albedoBuffer->create(GL_TEXTURE_RECTANGLE, width, height, 4, GL_FLOAT_RGBA_NV, GL_FLOAT, NULL, GL_FLOAT_RGBA32_NV);
	fbo->AttachTexture(GL_TEXTURE_RECTANGLE, albedoBuffer->handle(), GL_COLOR_ATTACHMENT0_EXT);

	normalBuffer.reset(new Texture("normalBuffer"));
	normalBuffer->create(GL_TEXTURE_RECTANGLE, width, normalBuffer->handle(), GL_COLOR_ATTACHMENT1_EXT);

	specularBuffer.reset(new Texture("specularBuffer"));
	specularBuffer->create(GL_TEXTURE_RECTANGLE, width, height, 4, GL_FLOAT_RGBA_NV, GL_FLOAT, NULL, GL_FLOAT_RGBA32_NV);
	fbo->AttachTexture(GL_TEXTURE_RECTANGLE, specularBuffer->handle(), GL_COLOR_ATTACHMENT2_EXT);

	positionBuffer.reset(new Texture("positionBuffer"));
	positionBuffer->create(GL_TEXTURE_RECTANGLE, width, height, 4, GL_FLOAT_RGBA_NV, GL_FLOAT, NULL, GL_FLOAT_RGBA32_NV);
	fbo->AttachTexture(GL_TEXTURE_RECTANGLE, positionBuffer->handle(), GL_COLOR_ATTACHMENT3_EXT);

	depthBuffer.reset(new Texture("depthBuffer"));
	depthBuffer->create(GL_TEXTURE_RECTANGLE, width, height, 1, GL_DEPTH_COMPONENT, GL_FLOAT, NULL, GL_DEPTH_COMPONENT32F);
	fbo->AttachTexture(GL_TEXTURE_RECTANGLE, depthBuffer->handle(), GL_DEPTH_ATTACHMENT);

	assert(fbo->IsValid(std::cout));
}

void PBuffer::init(GLuint width, GLuint height)
{
	fbo.reset(new FramebufferObject());
	texture.reset(new Texture());
	texture->create(GL_TEXTURE_RECTANGLE, width, height, 4, GL_FLOAT_RGBA_NV, GL_FLOAT, NULL, GL_FLOAT_RGBA32_NV);
	fbo->AttachTexture(GL_TEXTURE_RECTANGLE, texture->handle(), GL_COLOR_ATTACHMENT0_EXT);

	assert(fbo->IsValid(std::cout));
}

void RenderContext::init(int width, int height, double fov) const
{
	// init back plane
	_backPlaneMaterial.reset(new scene::Material());
	_backPlaneEffect.reset(new effect::Effect());
	if(!_backPlaneEffect->load(SHADER_DIR "background.xml"))
	{
		std::cout << "Error: " << _backPlaneEffect->get_last_error() << std::endl;
		return false;
	}
	_backPlaneMaterial->set_effect(_backPlaneEffect);

	_backPlaneVertexSpec(new scene::VertexSpec());
	_backPlaneVertexSpec->append(
		scene::VertexData::PositionData, sizeof(float), 3, scene::VertexElementType::Float);

	// load lighting effects
	_solarLightEffect.reset(new effect::Effect());
	if(!_solarLightEffect->load(SHADER_DIR "solar_light.xml"))
	{
		std::cout << "Error: " << _solarLightEffect->get_last_error() << std::endl;
		return false;
	}

	_solarLightShadowEffect.reset(new effect::Effect());
	if(!_solarLightShadowEffect->load(SHADER_DIR "solar_light_shadow.xml"))
	{
		std::cout << "Error: " << _solarLightShadowEffect->get_last_error() << std::endl;
		return false;
	}

	// load atmosphere effects
	_atmosphereAttenuateEffect.reset(new effect::Effect());
	if(!_atmosphereAttenuateEffect->load(SHADER_DIR "atmosphere_attenuate.xml"))
	{
		std::cout << "Error: " << _atmosphereAttenuateEffect->get_last_error() << std::endl;
		return false;
	}

	_atmosphereScatteringEffect.reset(new effect::Effect());
	if(!_atmosphereScatteringEffect->load(SHADER_DIR "atmosphere_scatter.xml"))
	{
		std::cout << "Error: " << _atmosphereScatteringEffect->get_last_error() << std::endl;
		return false;
	}

	// load other post process effects
	_renderPBufferEffect.reset(new effect::Effect());
	if(!_renderPBufferEffect->load(SHADER_DIR "render_pbuffer.xml"))
	{
		std::cout << "Error: " << _renderPBufferEffect->get_last_error() << std::endl;
		return false;
	}

	// init lighting
	_shadowDepthBuffer.reset(new Texture());
	_shadowDepthBuffer->create3D(GL_TEXTURE_2D_ARRAY_EXT, shadowBufferSize, shadowBufferSize, 
		cascadeSplits, 1, GL_DEPTH_COMPONENT, GL_FLOAT, NULL, GL_DEPTH_COMPONENT32F);
	_shadowingFBO.reset(new FramebufferObject());

	// init scene context
	_sceneContext.reset(new SceneContext());

	// init camera
	_mainCamera.reset(new transform::Camera());
	_viewport.reset(new Viewport(0, 0, width, height));
	_mainCamera->set_viewport(_viewport);

	// init gBuffer
	_gBuffer.reset(new GBuffer());
	_gBuffer->init(width, height);

	// init pBuffers
	_pBuffer0.reset(new PBuffer());
	_pBuffer0->init(width, height);
	_pBuffer1.reset(new PBuffer());
	_pBuffer1->init(width, height);

	// init main render stage
	_mainRender.reset(new GeometryRenderStage());

	_mainRender->set_fbo_target(_gBuffer->fbo);

	_mainRender->set_camera(_mainCamera);

	_mainRender->set_flag(
		RenderStageFlags::FrustumCull |
		RenderStageFlags::ClearColour |
		RenderStageFlags::ClearDepth);
	_mainRender->set_clear_screen_colour(math::Vector4f(0.0f, 0.0f, 0.0f, 1.0f));

	// setup back plane
	scene::VertexSet::ptr backPlaneVerts(new scene::VertexSet(4, backPlaneVertexSpec->vertexSize()));
	backPlaneVerts->setSpec(backPlaneVertexSpec);
	camera->set_far_plane(100000000);
	math::Vector3f backPlanebl(camera->unproject_global_pt(math::Vector3d(0, 0, 1)));
	math::Vector3f backPlanebr(camera->unproject_global_pt(math::Vector3d(0, screenHeight, 1)));
	math::Vector3f backPlanetl(camera->unproject_global_pt(math::Vector3d(screenWidth, 0, 1)));
	math::Vector3f backPlanetr(camera->unproject_global_pt(math::Vector3d(screenWidth, screenHeight, 1)));
	// top left vert
	*backPlaneVerts->extract<math::Vector3f>(0) = backPlanebl; 
	// bottom left vert
	*backPlaneVerts->extract<math::Vector3f>(1) = backPlanebr; 
	// top right
	*backPlaneVerts->extract<math::Vector3f>(2) = backPlanetl; 
	// bottom right
	*backPlaneVerts->extract<math::Vector3f>(3) = backPlanetr;
	// create the tri set for mesh
	scene::TriangleSet::ptr backPlaneTriSet(new TriangleSet(TrianglePrimitiveType::TRIANGLE_STRIP));
	backPlaneTriSet->push_back(0);
	backPlaneTriSet->push_back(1);
	backPlaneTriSet->push_back(2);
	backPlaneTriSet->push_back(3);
	scene::Material::ptr backPlaneMaterial(new scene::Material());
	effect::Effect::ptr backPlaneEffect(new effect::Effect());
	if(!backPlaneEffect->load("../Data/Shaders/background.xml"))
	{
		std::cout << "Error: " << backPlaneEffect->get_last_error() << std::endl;
		return 1;
	}
	backPlaneMaterial->set_effect(backPlaneEffect);
	scene::Geometry::ptr backPlaneGeometry(new scene::Geometry(backPlaneTriSet, backPlaneVerts, backPlaneMaterial, camera));

	ComposedGeometrySet::ptr cGeometrySet(new ComposedGeometrySet());
	GeometrySet::ptr geometrySet(new GeometrySet());
	cGeometrySet->add_geometry_set(geometrySet);
	mainRender->set_geometry(cGeometrySet);
	//GeometrySet::ptr geometrySet2(new GeometrySet());
	geometrySet->insert(backPlaneGeometry);
	//cGeometrySet->add_geometry_set(geometrySet2);

	camera->setTransform(math::transform(Transform::vec3_type(0.0f, 0.0f, 0.0f), 
		Transform::vec3_type(0.0f, 0.0f, radius * 3.0f)));

	// ------------------------------------------------------------------

	// ------------------------------------------------------------------
	// setup lighting state
	FramebufferObject::ptr pBuffer(new FramebufferObject());
	Texture::ptr pTexture0(new Texture());
	pTexture0->create(GL_TEXTURE_RECTANGLE, screenWidth, screenHeight, 4, GL_FLOAT_RGBA_NV, GL_FLOAT, NULL, GL_FLOAT_RGBA32_NV);
	pBuffer->AttachTexture(GL_TEXTURE_RECTANGLE, pTexture0->handle(), GL_COLOR_ATTACHMENT0_EXT);

	LightingRenderStage::ptr lightingStage(new LightingRenderStage());

	static unsigned int shadowBufferSize = 512;
	static unsigned int cascadeSplits = 4;
	lightingStage->set_cascade_splits(cascadeSplits);
	lightingStage->set_g_buffer(gBuffer);
	lightingStage->set_p_buffer(pBuffer);
	lightingStage->set_p_buffer_viewport(viewport);
	lightingStage->set_flag(RenderStageFlags::ClearColour);
	lightingStage->set_geometry_stage(mainRender);

	FramebufferObject::ptr shadowingFBO(new FramebufferObject());

	lightingStage->set_shadow_fbo(shadowingFBO);


	return rc;
}

#define SHADER_DIR "../Data/Shaders/"

bool RenderContextFactory::init(RenderGL* renderer, GLuint shadowBufferSize, GLuint cascadeSplits)
{
	_renderer = renderer;

	return true;
}


}