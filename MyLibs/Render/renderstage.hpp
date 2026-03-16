#if !defined(__RENDER_RENDERSTAGE_H__)
#define __RENDER_RENDERSTAGE_H__


#include "Math/vector4.hpp"
#include "Math/rectangle.hpp"
#include "GLBase/texture.hpp"
#include "Scene/camera.hpp"
#include "fbo/framebufferObject.h"
#include "Scene/param_binder.hpp"
#include "Scene/viewport.hpp"
#include "Scene/light.hpp"
#include "Vice/component.h"

#include "composed_geometryset.h"

#define DEFAULT_TARGET 0xffffffff

namespace render {;

struct RenderStageFlags { enum type {
	None				= 0,
	ClearColour			= 1 << 0,
	ClearDepth			= 1 << 1,
	CameraToViewport	= 1 << 2,
	FrustumCull			= 1 << 3,
	OptimizeZBuffer		= 1 << 4,
	WireFrame			= 1 << 5,
};};

inline RenderStageFlags::type operator|(const RenderStageFlags::type& first, const RenderStageFlags::type& second)
{
	return static_cast<RenderStageFlags::type>(
		static_cast<unsigned int>(first) | static_cast<unsigned int>(second));
}


struct RenderStage
{
	typedef std::shared_ptr< RenderStage > ptr;

	typedef std::function<void (RenderStage&)> RenderStageCallback;

	struct StageType { enum type {
		Geometry,
		Lighting,
		Atmosphere,
		UI
	};};

	typedef std::set<RenderStage::ptr> RenderStageSet;
	typedef RenderStageSet::iterator RenderStageIterator;
	typedef RenderStageSet::const_iterator ConstRenderStageIterator;

	RenderStage(StageType::type stageType, const std::string& name = std::string()) : _stageType(stageType), _name(name), _flags(RenderStageFlags::None) {}
	virtual ~RenderStage() {}

	StageType::type get_type() const { return _stageType; }

	void set_flag(RenderStageFlags::type flag)
	{
		_flags = _flags | static_cast<unsigned int>(flag);
	}

	void clear_flag(RenderStageFlags::type flag)
	{
		_flags = _flags & ~(static_cast<unsigned int>(flag));
	}

	bool is_flag_set(RenderStageFlags::type flag) const
	{
		return (_flags & static_cast<unsigned int>(flag)) != 0;
	}

	unsigned int get_flags() const { return _flags; }

	void set_complete_callback(RenderStageCallback callback) { _renderStageComplete = callback; }

	void call_render_stage_complete()
	{
		if(_renderStageComplete)
			_renderStageComplete(*this);
	}

	const std::string& get_name() const { return _name; }

	void add_dependancy(RenderStage::ptr rs) { _dependancies.insert(rs); }

	RenderStageIterator begin_dependancies() { return _dependancies.begin(); }
	ConstRenderStageIterator begin_dependancies() const { return _dependancies.begin(); }

	RenderStageIterator end_dependancies() { return _dependancies.end(); }
	ConstRenderStageIterator end_dependancies() const { return _dependancies.end(); }

private:
	StageType::type _stageType;
	RenderStageSet _dependancies;

	RenderStageCallback _renderStageComplete;

	unsigned int _flags;

	std::string _name;
};

struct GeometryRenderStage : public RenderStage
{
	typedef std::shared_ptr< GeometryRenderStage > ptr;

	GeometryRenderStage(const std::string& name) : RenderStage(StageType::Geometry, name), _geometry(new ComposedGeometrySet()) {}

	scene::transform::Camera::ptr get_camera() const { return _camera; }
	void set_camera(scene::transform::Camera::ptr camera) { _camera = camera; }

	void set_fbo_target(FramebufferObject::ptr fboTarget) { _fboTarget = fboTarget; }
	FramebufferObject::ptr get_fbo_target() const { return _fboTarget; }

	void add_render_target(GLenum target) { _renderTargets.push_back(target); }
	const std::vector<GLenum>& get_render_targets() const { return _renderTargets; }

	void set_clear_screen_colour(const math::Vector4f& colour) { _clearColor = colour; }
	const math::Vector4f& get_clear_colour() const { return _clearColor; }

// 	void set_viewport(scene::Viewport::ptr viewport) { _viewport = viewport; }
// 	scene::Viewport::ptr get_viewport() const { return _viewport; }

	void set_geometry(ComposedGeometrySet::ptr geometry) { _geometry = geometry; }
	ComposedGeometrySet::ptr get_geometry() const { return _geometry; }

private:
	FramebufferObject::ptr _fboTarget;
	std::vector<GLenum> _renderTargets;

	ComposedGeometrySet::ptr _geometry;
	scene::transform::Camera::ptr _camera;

	//scene::Viewport::ptr _viewport;
	math::Vector4f _clearColor;
};

//struct DirectionalLightingRenderStage : public RenderStage
//{
//	typedef std::shared_ptr< DirectionalLightingRenderStage > ptr;
//
//	DirectionalLightingRenderStage() : RenderStage(StageType::Lighting), _cascadeSplits(4) {}
//
//	void set_g_buffer(FramebufferObject::ptr gBuffer) { _gBuffer = gBuffer; }
//	FramebufferObject::ptr get_g_buffer() const { return _gBuffer; }
//
//	void set_p_buffer(FramebufferObject::ptr pBuffer) { _pBuffer = pBuffer; }
//	FramebufferObject::ptr get_p_buffer() const { return _pBuffer; }
//
//	void set_viewport(scene::Viewport::ptr viewport) { _pBufferViewport = viewport; }
//	scene::Viewport::ptr get_viewport() const { return _pBufferViewport; }
//
//	// for use with directional lighting
//	void set_cascade_shadow_depth_texture(glbase::Texture::ptr shadowDepthTexture) { _cascadeShadowDepthTexture = shadowDepthTexture; }
//	glbase::Texture::ptr get_cascade_shadow_depth_texture() const { return _cascadeShadowDepthTexture; }
//
//	void set_shadow_fbo(FramebufferObject::ptr shadowFBO) { _shadowFBO = shadowFBO; }
//	FramebufferObject::ptr get_shadow_fbo() const { return _shadowFBO; }
//
//	void set_geometry_stage(GeometryRenderStage::ptr geometryStage) { _geometryStage = geometryStage; }
//	GeometryRenderStage::ptr get_geometry_stage() const { return _geometryStage; }
//
//	void set_cascade_splits(unsigned int cascadeSplits) { _cascadeSplits = cascadeSplits; }
//	unsigned int get_cascade_splits() const { return _cascadeSplits; }
//
//	void set_solar_light_material(scene::Material::ptr solarLightMaterial) { _solarLightMaterial = solarLightMaterial; }
//	scene::Material::ptr get_solar_light_material() const { return _solarLightMaterial; }
//private:
//	FramebufferObject::ptr _gBuffer;
//	FramebufferObject::ptr _pBuffer;
//	FramebufferObject::ptr _shadowFBO;
//	glbase::Texture::ptr _cascadeShadowDepthTexture;
//
//	GeometryRenderStage::ptr _geometryStage;
//	//scene::Material::ptr _solarLightMaterial;
//
//	scene::Viewport::ptr _pBufferViewport;
//
//	unsigned int _cascadeSplits;
//};

/*
light rendering:
sun/planet - a point light but effectively directional, doubles as ambient lighting
spot lights - standard cone light, possibly with shadow map
point lights - standard point light, possibly with shadow map(s), parabolic or 6 separate frustum spot lights
*/
struct LightingRenderStage : public RenderStage
{
	struct LightParams
	{
		typedef std::shared_ptr< LightParams > ptr;

		LightParams(scene::transform::Light::ptr light, 
			scene::Material::ptr lightMaterial) 
			: _light(light), _lightMaterial(lightMaterial) {}

		LightParams() {}

		void set_light(scene::transform::Light::ptr light) { _light = light; }
		scene::transform::Light::ptr get_light() const { return _light; }

		void set_light_material(scene::Material::ptr lightMaterial) { _lightMaterial = lightMaterial; }
		scene::Material::ptr get_light_material() const { return _lightMaterial; }

		//void set_atmosphere_center_transform(scene::transform::Transform::ptr atmosphereCenterTransform) { _atmosphereCenterTransform = atmosphereCenterTransform; }
		//scene::transform::Transform::ptr get_atmosphere_center_transform() const { return _atmosphereCenterTransform; }

	private:
		scene::transform::Light::ptr _light; 
		scene::Material::ptr _lightMaterial;
		//scene::transform::Transform::ptr _atmosphereCenterTransform;
	};

	typedef std::shared_ptr< LightingRenderStage > ptr;

	typedef std::set<LightParams::ptr> LightSet;
	typedef LightSet::iterator LightIterator;
	typedef LightSet::const_iterator ConstLightIterator;

	LightingRenderStage() : RenderStage(StageType::Lighting), _cascadeSplits(4) {}

	void remove_all_lights() { _lights.clear(); }
	void add_light(LightParams::ptr light) { _lights.insert(light); }
	void remove_light(LightParams::ptr light) { _lights.erase(light); }

	LightIterator begin_lights() { return _lights.begin(); }
	LightIterator end_lights() { return _lights.end(); }

	ConstLightIterator begin_lights() const { return _lights.begin(); }
	ConstLightIterator end_lights() const { return _lights.end(); }

	void set_g_buffer(FramebufferObject::ptr gBuffer) { _gBuffer = gBuffer; }
	FramebufferObject::ptr get_g_buffer() const { return _gBuffer; }

	// p buffer is output target where lighting is accumulated
	void set_p_buffer(FramebufferObject::ptr pBuffer) { _pBuffer = pBuffer; }
	FramebufferObject::ptr get_p_buffer() const { return _pBuffer; }

	void set_geometry_stage(GeometryRenderStage::ptr geometryStage) { _geometryStage = geometryStage; }
	GeometryRenderStage::ptr get_geometry_stage() const { return _geometryStage; }

	// for use with directional lighting
	void set_cascade_shadow_depth_texture(glbase::Texture::ptr shadowDepthTexture) { _cascadeShadowDepthTexture = shadowDepthTexture; }
	glbase::Texture::ptr get_cascade_shadow_depth_texture() const { return _cascadeShadowDepthTexture; }
	void set_shadow_fbo(FramebufferObject::ptr shadowFBO) { _shadowFBO = shadowFBO; }
	FramebufferObject::ptr get_shadow_fbo() const { return _shadowFBO; }
	void set_cascade_splits(unsigned int cascadeSplits) { _cascadeSplits = cascadeSplits; }
	unsigned int get_cascade_splits() const { return _cascadeSplits; }

private:
	std::set<LightParams::ptr> _lights;
	FramebufferObject::ptr _gBuffer;
	FramebufferObject::ptr _pBuffer;
	FramebufferObject::ptr _shadowFBO;
	glbase::Texture::ptr _cascadeShadowDepthTexture;

	GeometryRenderStage::ptr _geometryStage;

	unsigned int _cascadeSplits;
};

/*
post process effects stage:
atmospheric attenuation - 
atmospheric scattering -
	-light direction / location
	-atmosphere parameters
	-planet location + radius
	Calculate screen space rect including entire atmosphere and render it using appropriate shader+shader params


*/
struct AtmosphericsRenderStage : public RenderStage
{
	struct AtmosphereParams
	{
		typedef std::shared_ptr< AtmosphereParams > ptr;

		AtmosphereParams(scene::transform::Light::ptr light, 
			scene::Material::ptr material,
			scene::transform::Transform::ptr atmosphereCenter,
			double radius) 
			: _light(light), 
			_material(material), 
			_atmosphereCenter(atmosphereCenter),
			_radius(radius)
		{}

		AtmosphereParams() {}

		void set_light(scene::transform::Light::ptr light) { _light = light; }
		scene::transform::Light::ptr get_light() const { return _light; }

		void set_material(scene::Material::ptr attenuateMaterial) { _material = attenuateMaterial; }
		scene::Material::ptr get_material() const { return _material; }

		void set_atmosphere_center_transform(scene::transform::Transform::ptr atmosphereCenter) { _atmosphereCenter = atmosphereCenter; }
		scene::transform::Transform::ptr get_atmosphere_center_transform() const { return _atmosphereCenter; }

		void set_radius(double radius) { _radius = radius; }
		double get_radius() const { return _radius; }

	private:
		scene::transform::Light::ptr _light; 
		scene::Material::ptr _material;
		scene::transform::Transform::ptr _atmosphereCenter;
		double _radius;
	};

	typedef std::shared_ptr< AtmosphericsRenderStage > ptr;

	typedef std::set<AtmosphereParams::ptr> AtmosphereSet;
	typedef AtmosphereSet::iterator AtmosphereIterator;
	typedef AtmosphereSet::const_iterator ConstAtmosphereIterator;

	AtmosphericsRenderStage() : RenderStage(StageType::Atmosphere) {}

	void add_attenuate(AtmosphereParams::ptr atmosphere)	{ _attenuates.insert(atmosphere); }
	void remove_attenuate(AtmosphereParams::ptr atmosphere) { _attenuates.erase(atmosphere); }
	ConstAtmosphereIterator begin_attenuates() const { return _attenuates.begin(); }
	ConstAtmosphereIterator end_attenuates() const { return _attenuates.end(); }

	void add_scatter(AtmosphereParams::ptr atmosphere)	{ _scatters.insert(atmosphere); }
	void remove_scatter(AtmosphereParams::ptr atmosphere) { _scatters.erase(atmosphere); }
	ConstAtmosphereIterator begin_scatters() const { return _scatters.begin(); }
	ConstAtmosphereIterator end_scatters() const { return _scatters.end(); }

	void set_g_buffer(FramebufferObject::ptr gBuffer) { _gBuffer = gBuffer; }
	FramebufferObject::ptr get_g_buffer() const { return _gBuffer; }

	// p buffer is input
	void set_p_buffer(FramebufferObject::ptr pBuffer) { _pBuffer = pBuffer; }
	FramebufferObject::ptr get_p_buffer() const { return _pBuffer; }

	void set_fbo_target(FramebufferObject::ptr fboTarget) { _fboTarget = fboTarget; }
	FramebufferObject::ptr get_fbo_target() const { return _fboTarget; }

	void set_geometry_stage(GeometryRenderStage::ptr geometryStage) { _geometryStage = geometryStage; }
	GeometryRenderStage::ptr get_geometry_stage() const { return _geometryStage; }

private:
	std::set<AtmosphereParams::ptr> _attenuates;
	std::set<AtmosphereParams::ptr> _scatters;
	FramebufferObject::ptr _gBuffer;
	FramebufferObject::ptr _fboTarget;
	FramebufferObject::ptr _pBuffer;

	GeometryRenderStage::ptr _geometryStage;
};

struct UIRenderStage : public RenderStage
{
	typedef std::shared_ptr< UIRenderStage > ptr;

	UIRenderStage(const std::string& name) : RenderStage(StageType::UI, name) {}

	struct UIContext
	{
		UIContext(const vice::ComponentInstance::ptr& component_)
			: component(component_) {}

		vice::ComponentInstance::ptr component;
	};

	void clear_contexts()
	{
		_contexts.clear();
	}

	void add_context(const vice::ComponentInstance::ptr& component)
	{ 
		_contexts.push_back(UIContext(component)); 
	}

	const std::vector<UIContext>& get_contexts() const { return _contexts; }

private:
	std::vector<UIContext> _contexts;
};

}

#endif // __RENDER_RENDERSTAGE_H__