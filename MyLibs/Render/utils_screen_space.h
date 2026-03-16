#ifndef utils_screen_space_h__
#define utils_screen_space_h__

#include "scenecontext.hpp"

namespace render {;
namespace utils {;

#if 0

template < class SCt_ >
struct HDRData
{
	typedef typename SCt_::RenderStageType RSType;
	typedef HDRData< SCt_ > this_type;
	typedef std::shared_ptr< this_type > ptr;
private:
	typename RSType::ptr _mainRender, _shrinkSplitStage, _compositeBlurStage;
	scene::Material::ptr _shrinkSplitMaterial, _compositeBlurMaterial, _exposureCalcMaterial;
	glbase::VertexSet::ptr _shrinkFSQVerts;
	float _exposure, _currIntensity, _maxIntensity, _minIntensity, _targetIntensity, _intensityChangeRate;
	unsigned int _shrinkFactor, _blurSize;
	glbase::Texture::ptr _exposureCalcTexture;
	float _hdrExposureScale;

	unsigned int _lastt;

public:
	HDRData() {}

	HDRData(typename RSType::ptr mainRender_, typename RSType::ptr exposureCalcStage_, typename RSType::ptr shrinkSplitStage_, typename RSType::ptr compositeBlurStage_, 
		glbase::VertexSet::ptr shrinkFSQVerts, /*scene::Material::ptr exposureCalcMaterial, */scene::Material::ptr shrinkSplitMaterial, 
		scene::Material::ptr compositeBlurMaterial,	glbase::Texture::ptr exposureCalcTexture)
		: _mainRender(mainRender_), _shrinkSplitStage(shrinkSplitStage_), _compositeBlurStage(compositeBlurStage_), _shrinkFSQVerts(shrinkFSQVerts), /*_exposureCalcMaterial(exposureCalcMaterial), */_shrinkSplitMaterial(shrinkSplitMaterial), _compositeBlurMaterial(compositeBlurMaterial), _exposureCalcTexture(exposureCalcTexture), _currIntensity(1.0f), _exposure(0.6f),
		_maxIntensity(5.0f), _minIntensity(0.5f), _targetIntensity(1.0f), _intensityChangeRate(2.0f),
		_lastt(SDL_GetTicks()), _hdrExposureScale(1.1f)
	{
		exposureCalcStage_->set_complete_callback(std::bind(&this_type::calculateExposure, this, std::placeholders::_1));
	}

	void setExposure(float exposure)
	{
		_exposure = exposure;
		setModulatedExposure();
	}

	typename RSType::ptr shrinkSplitStage() const { return _shrinkSplitStage; }
	typename RSType::ptr compositeBlurStage() const { return _compositeBlurStage; }
	float exposure() const { return _exposure; }

private:
	// to calculate exposure
	// render hdr to texture using sampling shader
	// read texture back and determine its brightness
	// alter exposure accordingly
	void calculateExposure(RSType& stage)
	{
		_exposureCalcTexture->bind();
		math::Vector3f intensity;
		_exposureCalcTexture->getData(&intensity);
		_exposureCalcTexture->unbind();
		_targetIntensity = (intensity.x + intensity.y + intensity.z) * 0.3333f;
		//_targetIntensity = MY_MIN(_maxIntensity, MY_MAX(_minIntensity, 5/(2+std::log(_targetIntensity/1.1+0.1))));
		_targetIntensity = static_cast<float>(MY_MIN(_maxIntensity, MY_MAX(_minIntensity, 1.0/-std::log(1.0 - _targetIntensity/_hdrExposureScale))));
		//setModulatedExposure();
		updateExposure();
	}

	void updateExposure()
	{
		unsigned int t = SDL_GetTicks();
		float dt = static_cast<float>(t - _lastt) / 1000.0f;
		_lastt = t;
		_intensityChangeRate = std::abs(_targetIntensity - _currIntensity);
		if(_currIntensity < _targetIntensity)
		{
			_currIntensity = MY_MIN(_targetIntensity, _currIntensity + dt * _intensityChangeRate);
			setModulatedExposure();
		}
		else if(_currIntensity > _targetIntensity)
		{
			_currIntensity = MY_MAX(_targetIntensity, _currIntensity - dt * _intensityChangeRate);
			setModulatedExposure();
		}
	}

	void setModulatedExposure()
	{
		//_shrinkSplitMaterial->floatInputMap()["Exposure"] = _currIntensity * _exposure;
		//_compositeBlurMaterial->floatInputMap()["Exposure"] = _currIntensity * _exposure;
		_shrinkSplitMaterial->setParameter<float>("Exposure", _currIntensity * _exposure);
		_compositeBlurMaterial->setParameter<float>("Exposure", _currIntensity * _exposure);
	}
};

template < class SCt_ >
typename HDRData<SCt_>::ptr addHDRStages( int screenWidth, int screenHeight, typename SCt_::RenderStageType::ptr mainRender, typename SCt_::CameraType::ptr camera, typename SCt_::ptr sceneContext )
{
	typedef SCt_::RenderStageType RSType;
	typedef typename SCt_::TriangleMeshType TriangleMeshType;
	int shrinkFactor = 32, filterSize = 3;
	float exposure = 0.6f;

	FramebufferObject::ptr hdrfbo(new FramebufferObject());
	hdrfbo->Bind();
	// add primary render target
	glbase::Texture::ptr hdrfsq(new glbase::Texture());
	hdrfsq->create2D(GL_TEXTURE_RECTANGLE_ARB, screenWidth, screenHeight, 4, GL_RGBA, GL_FLOAT, NULL, GL_RGBA32F_ARB);
	//hdrfsq->bind();
	//hdrfsq->setFilter(FilterMode::Nearest, FilterMode::Nearest);
	hdrfbo->AttachTexture(hdrfsq->handle(), GL_COLOR_ATTACHMENT0);
	glbase::Texture::ptr horizontalconvolvefsq(new glbase::Texture());
	horizontalconvolvefsq->create2D(GL_TEXTURE_RECTANGLE, screenWidth, screenHeight, 4, GL_RGBA, GL_FLOAT, NULL, GL_RGBA32F_ARB);
	//hdrfsq->bind();
	//hdrfsq->setFilter(glbase::FilterMode::Nearest, glbase::FilterMode::Nearest);
	hdrfbo->AttachTexture(horizontalconvolvefsq->handle(), GL_COLOR_ATTACHMENT1);


	// create the depth render-buffer and add it
	Renderbuffer::ptr depthBuff(new Renderbuffer());
	depthBuff->Set(GL_DEPTH_COMPONENT32, screenWidth, screenHeight);
	hdrfbo->AttachRenderBuffer(depthBuff->GetId(), GL_DEPTH_ATTACHMENT);
	// hand the lifetime management of the render-buffer over to the fbo
	hdrfbo->manageRenderBuffer(depthBuff);
	assert(hdrfbo->IsValid());
	FramebufferObject::Disable();

	mainRender->setFBOTarget(hdrfbo);
	mainRender->setPriority(4);
	// attach camera to the correct attachment and the fbo to the render stage
	mainRender->addCamera(camera, GL_COLOR_ATTACHMENT0);

	// create a simple ortho2d camera for drawing fsqs
	typename SCt_::CameraType::ptr fsqCam(new SCt_::CameraType());
	fsqCam->setType(ProjectionType::Ortho2D);
	fsqCam->setNearPlane(-1.0);	fsqCam->setFarPlane(1.0);

	// create exposure calculation fbo
	FramebufferObject::ptr exposureCalcfbo(new FramebufferObject());
	exposureCalcfbo->Bind();
	// add shrink and split target
	glbase::Texture::ptr exposureCalcTex(new glbase::Texture());
	exposureCalcTex->create2D(GL_TEXTURE_2D, 1, 1, 3, GL_RGB, GL_FLOAT, NULL, GL_RGB32F_ARB);
	exposureCalcfbo->AttachTexture(exposureCalcTex->handle(), GL_COLOR_ATTACHMENT0);
	assert(exposureCalcfbo->IsValid());
	FramebufferObject::Disable();

	// create a render stage for the calculating exposure
	RSType::ptr exposureCalcStage = createFSQRenderStage<SCt_>(1, 1, exposureCalcfbo);
	exposureCalcStage->setPriority(3);
	sceneContext->addStage(exposureCalcStage);
	exposureCalcStage->addCamera(fsqCam, GL_COLOR_ATTACHMENT0);

	// create the materials and meshes for calculating exposure
	Shader::ptr exposureCalcShader(ShaderManager::globalManager().createShader("calcexposure", "../Data/Shaders/ExposureSample.cgfx"));
	Material::ptr exposureCalcMaterial(new Material());
	exposureCalcMaterial->setParameter("FSQTexture", hdrfsq);
	exposureCalcMaterial->setParameter("FSQTextureSize", math::Vector2f(hdrfsq->width(), hdrfsq->height()));
	exposureCalcMaterial->setParameter<float>("Resolution", 20);
	//exposureCalcMaterial->textureInputMap()["FSQTexture"] = hdrfsq;
	//exposureCalcMaterial->vector2fInputMap()["FSQTextureSize"] = math::Vector2f(hdrfsq->width(), hdrfsq->height());
	//exposureCalcMaterial->floatInputMap()["Resolution"] = 20;
	exposureCalcMaterial->setShader(exposureCalcShader);
	TriangleMeshType::ptr exposureCalcMesh = createFSQ<SCt_>(1, 1);
	exposureCalcMesh->setMaterial(exposureCalcMaterial);
	exposureCalcStage->includeMesh(exposureCalcMesh);

	// create shrink/split and blur fbo
	int shrinkWidth = screenWidth/shrinkFactor, shrinkHeight = screenHeight/shrinkFactor;
	FramebufferObject::ptr shrinkfbo(new FramebufferObject());
	shrinkfbo->Bind();
	// add shrink and split target
	glbase::Texture::ptr shrinkfsq(new glbase::Texture());
	shrinkfsq->create2D(GL_TEXTURE_RECTANGLE, shrinkWidth, shrinkHeight, 3, GL_RGB, GL_FLOAT, NULL, GL_RGB32F_ARB);
	//// add shrink and split target
	//glbase::Texture::ptr shrinkfsq(new glbase::Texture());
	//shrinkfsq->create(GL_TEXTURE_RECTANGLE_ARB, shrinkWidth, shrinkHeight, 3, GL_RGB, GL_FLOAT, NULL, GL_RGB32F_ARB);

	//shrinkfsq->bind();
	//shrinkfsq->setFilter(glbase::FilterMode::Linear, glbase::FilterMode::Linear);
	shrinkfbo->AttachTexture(shrinkfsq->handle(), GL_COLOR_ATTACHMENT0);

	// add blur target
	//glbase::Texture::ptr blurfsq(new glbase::Texture());
	//blurfsq->create(GL_TEXTURE_RECTANGLE_ARB, shrinkWidth, shrinkHeight, 3, GL_RGB, GL_FLOAT, NULL, GL_RGB32F_ARB);
	//shrinkfbo->AttachTexture(GL_TEXTURE_RECTANGLE_ARB, blurfsq->handle(), GL_COLOR_ATTACHMENT1_EXT);

	assert(shrinkfbo->IsValid());
	FramebufferObject::Disable();


	// create a render stage for the shrinking/splitting
	RSType::ptr shrinkSplitStage = createFSQRenderStage<SCt_>(shrinkWidth, shrinkHeight, shrinkfbo);
	shrinkSplitStage->setPriority(2);
	sceneContext->addStage(shrinkSplitStage);
	shrinkSplitStage->addCamera(fsqCam, GL_COLOR_ATTACHMENT0);
	// create the materials and meshes for shrinking/splitting and blurring
	Shader::ptr shrinkSplitShader(ShaderManager::globalManager().createShader("shrinksplit", "../Data/Shaders/ShrinkSplitFSQ.cgfx"));
	Material::ptr shrinkSplitMaterial(new Material());
	shrinkSplitMaterial->setParameter("FSQTexture", hdrfsq);
	//shrinkSplitMaterial->textureInputMap()["FSQTexture"] = hdrfsq;
	shrinkSplitMaterial->setShader(shrinkSplitShader);
	TriangleMeshType::ptr shrinkSplitMesh = createFSQ<SCt_>(shrinkWidth, shrinkHeight);
	shrinkSplitMesh->setMaterial(shrinkSplitMaterial);
	shrinkSplitStage->includeMesh(shrinkSplitMesh);

	// create a render stage for the horizontalConvolution
	RSType::ptr horizontalConvolveStage = createFSQRenderStage<SCt_>(screenWidth, screenHeight, hdrfbo);
	horizontalConvolveStage->setPriority(1);
	sceneContext->addStage(horizontalConvolveStage);
	horizontalConvolveStage->addCamera(fsqCam, GL_COLOR_ATTACHMENT1);
	// create the materials and meshes for shrinking/splitting and blurring
	Shader::ptr horizontalConvolveShader(ShaderManager::globalManager().createShader("horizontalConvolveShader", "../Data/Shaders/HorizontalConvolve.cgfx"));
	Material::ptr horizontalConvolveMaterial(new Material());
	horizontalConvolveMaterial->setParameter("BloomTexture", shrinkfsq);
	//horizontalConvolveMaterial->textureInputMap()["BloomTexture"] = shrinkfsq;
	horizontalConvolveMaterial->setShader(horizontalConvolveShader);
	TriangleMeshType::ptr horizontalConvolveMesh = createFSQ<SCt_>(screenWidth, screenHeight);
	horizontalConvolveMesh->setMaterial(horizontalConvolveMaterial);
	horizontalConvolveStage->includeMesh(horizontalConvolveMesh);

	//RSType::ptr blurStage = createFSQRenderStage<SCt_>(shrinkWidth, shrinkHeight, hdrfbo);
	//hdrRender->add_camera(fsqCam, GL_COLOR_ATTACHMENT2_EXT);

	//Shader::ptr blurShader(ShaderManager::globalManager().createShader("shrinksplit", "../Data/Shaders/ShrinkSplitFSQ.cgfx"));
	//Material::ptr blurMaterial(new Material());
	//blurMaterial->textureInputMap()["FSQTexture"] = shrinkfsq;
	//blurMaterial->floatInputMap()["KernelSize"] = ;
	//blurMaterial->setShader(blurShader);

	// add cameras with other targets etc blah blah here.

	// create a render stage for the exposure 
	RSType::ptr expRender = createFSQRenderStage<SCt_>(screenWidth, screenHeight/*, hdrfbo*/);//(new RSType());
	expRender->setPriority(0);
	sceneContext->addStage(expRender);
	// no point clearing the screen for full screen quad renders
	expRender->addCamera(fsqCam);
	Shader::ptr exposureShader(ShaderManager::globalManager().createShader("exposure", "../Data/Shaders/VerticalConvolveExposureComposite.cgfx"));
	Material::ptr exposureMaterial(new Material());
	exposureMaterial->setParameter("FSQTexture", hdrfsq);
	exposureMaterial->setParameter("BloomTexture", horizontalconvolvefsq);
	//exposureMaterial->textureInputMap()["FSQTexture"] = hdrfsq;
	//exposureMaterial->textureInputMap()["BloomTexture"] = horizontalconvolvefsq;
	//exposureMaterial->floatInputMap()["ShrinkFactor"] = 1.0/shrinkFactor;
	//exposureMaterial->floatInputMap()["Exposure"] = exposure;
	//exposureMaterial->intInputMap()["BoxFilterSize"] = filterSize;
	exposureMaterial->setShader(exposureShader);
	TriangleMeshType::ptr fsqMesh = createFSQ<SCt_>(screenWidth, screenHeight);

	fsqMesh->setMaterial(exposureMaterial);
	expRender->includeMesh(fsqMesh);

	HDRData<SCt_>::ptr hdrData(new HDRData<SCt_>(mainRender, exposureCalcStage, shrinkSplitStage, expRender, (*shrinkSplitMesh->beginTris())->getVerts(), shrinkSplitMaterial, exposureMaterial, exposureCalcTex));
	hdrData->setExposure(exposure);
	//hdrData.setShrinkFactor(shrinkFactor);
	//hdrData.setBlurSize(filterSize);

	return hdrData;
}

template < class SCt_ > 
typename SCt_::RenderStageType::ptr createFSQRenderStage( int width, int height, FramebufferObject::ptr hdrfbo = FramebufferObject::ptr())
{
	typename SCt_::RenderStageType::ptr fsqStage(new SCt_::RenderStageType());
	//fsqStage->setClearScreen(false);
	fsqStage->setViewport(math::Rectangle<unsigned int>(0, 0, width, height));
	fsqStage->setIncludedMeshesOnly(true);
	fsqStage->setFBOTarget(hdrfbo);

	return fsqStage;
}
#endif
//template < class SCt_ > 
//typename SCt_::RenderStageType::ptr addRenderToFBOStage(
//	typename SCt_::ptr sc, 
//	typename SCt_::CameraType::ptr camera, 
//	FramebufferObject::ptr fbo)
//{
//
//}

struct QuadVert
{
	QuadVert(const math::Vector2f& pos_, const math::Vector2f& uv_) : pos(pos_), uv(uv_) {}

	math::Vector2f pos;
	math::Vector2f uv;
};

// calculate screen space quad to cover a sphere
math::Rectanglei get_screenspace_rect( const scene::transform::Camera::ptr& camera, const math::BoundingSphered& bsphere);

struct UVType { enum type {
	Pixel,
	Relative
};};
#if 0
// get a shared screen space quad geometry, DO NOT MODIFY THE VERTS OR TRIS, someone else might be sharing them, remember to apply a material.
scene::Geometry::ptr get_shared_screen_quad( int x, int y, int width, int height, UVType::type uvType = UVType::Pixel );
#endif
// create a new screen space quad you don't have to share with anyone.
template < class Ty_ >
scene::Geometry::ptr create_new_screen_quad( Ty_ x, Ty_ y, Ty_ width, Ty_ height, UVType::type uvType = UVType::Pixel );

// update the vert positions of a screen space quad provided by create_new_screen_quad
template < class Ty_ >
void update_screen_quad( const scene::Geometry::ptr& geom, Ty_ x, Ty_ y, Ty_ width, Ty_ height, UVType::type uvType = UVType::Pixel );

render::GeometryRenderStage::ptr create_2D_render_stage( unsigned int width, unsigned int height, const std::string& name, FramebufferObject::ptr fbo = FramebufferObject::ptr());
render::GeometryRenderStage::ptr create_ui_render_stage( unsigned int width, unsigned int height, const std::string& name, FramebufferObject::ptr fbo = FramebufferObject::ptr());
render::GeometryRenderStage::ptr create_fsq_render_stage( unsigned int width, unsigned int height, scene::Material::ptr material, const std::string& name, FramebufferObject::ptr fbo = FramebufferObject::ptr());

}
}

#include "utils_screen_space.inl"

#endif // utils_screen_space_h__
