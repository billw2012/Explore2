#ifndef atmospherics_h__
#define atmospherics_h__

#include "texturearraymanager.h"
#include "atmospheric_scattering.h"
#include "databuilderthread.h"
#include "engine_constants.h"

namespace explore2 {;

/*
 * Atmospheric parameters.
 * Material.
 * Sun
 */

struct AtmosphereBuildData;

struct AtmosphereParameters
{
	friend struct Atmospherics;

	typedef std::shared_ptr< AtmosphereParameters > ptr;

	// spawn lookup texture creation threads etc.
	void prepare_for_draw();
	// are the lookup textures calculated?
	bool is_ready_to_draw();
	bool is_building() const;

	//int opticalDepthTextureSize;
	//int surfaceLightingTextureSize;
	//int opticalDepthSamples;
	//int surfaceLightingSamples;
	//int KFrThetaOverLambda4Samples;

	const AtmosphericScattering::Parameters& get_parameters() const { return _params; }

	TextureArraySlotHandle get_ODRayleigh_texture_handle() const { return _opticalDepthRayleighTextureHandle; }
	TextureArraySlotHandle get_ODMie_texture_handle() const { return _opticalDepthMieTextureHandle; }
	TextureArraySlotHandle get_KFrMie_texture_handle() const { return _KFrThetaOverLambda4MieTextureHandle; }
	TextureArraySlotHandle get_KFrRayleigh_texture_handle()  const { return _KFrThetaOverLambda4RayleighTextureHandle; }
	TextureArraySlotHandle get_SL_texture_handle() const { return _surfaceLightingTextureHandle; }


	const AtmosphericScattering::vec3_type& get_beta() const { return _beta; }
	float get_planet_radius() const { return _params.innerRadius; }
	float get_atmosphere_depth() const { return _params.depth; }

private:
	AtmosphereParameters(const AtmosphericScattering::Parameters& params);

	//std::vector<render::LightingRenderStage::LightParams::ptr> _suns;

	AtmosphericScattering::Parameters _params;
	TextureArraySlotHandle _opticalDepthRayleighTextureHandle;
	TextureArraySlotHandle _opticalDepthMieTextureHandle;
	TextureArraySlotHandle _KFrThetaOverLambda4MieTextureHandle;
	TextureArraySlotHandle _KFrThetaOverLambda4RayleighTextureHandle;
	TextureArraySlotHandle _surfaceLightingTextureHandle;

	AtmosphericScattering::vec3_type _beta;
	//scene::Material::ptr atmosphereAttenuateMaterial;
	//scene::Material::ptr atmosphereScatteringMaterial;


	bool _readyToDraw;

	BuildThreadData::ptr _buildData;
};

struct Atmospherics 
{
	// load shaders etc.
	static std::shared_ptr<void> init(int maxAtmospheres, int maxLights, int quality);
	static void destroy();

	static glbase::Texture::ptr get_optical_depth_rayleigh_textures();
	static glbase::Texture::ptr get_optical_depth_mie_textures();
	static glbase::Texture::ptr get_kfr_mie_textures();
	static glbase::Texture::ptr get_kfr_rayleigh_textures();
	static glbase::Texture::ptr get_surface_lighting_textures();

	//static void dump();

	struct AtmosphereType { enum type {
		Oxygen,
		Nitrogen,
	};};

	static AtmosphereParameters::ptr create_atmosphere(const AtmosphericScattering::Parameters& params);
};


}

#endif // atmospherics_h__
