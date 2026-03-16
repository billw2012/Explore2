
#include "atmospherics.h"

namespace explore2 {;

static const AtmosphericScattering::float_type ATMOSPHERE_WORLD_SCALE = AtmosphericScattering::float_type(1.0);

TextureArrayManager::ptr _opticalDepthRayleighTextures;
TextureArrayManager::ptr _opticalDepthMieTextures;
TextureArrayManager::ptr _KFrMieTextures;
TextureArrayManager::ptr _KFrRayleighTextures;
TextureArrayManager::ptr _surfaceLightingTextures;
int _samples = 0;
int _textureSizes = 0;

// quality	| texture size	| samples
// 1		| 16			| 3
// 4		| 128			| 12
std::shared_ptr<void> Atmospherics::init(int maxAtmospheres, int maxLights, int quality)
{
	_textureSizes = (int)std::pow(2, quality+3);
	_samples = quality * 3;

	_opticalDepthRayleighTextures.reset(new TextureArrayManager());
	_opticalDepthRayleighTextures->init2D("optical depth rayleigh", _textureSizes, _textureSizes, maxAtmospheres, 1, GL_RED, GL_FLOAT, GL_R32F);
	_opticalDepthRayleighTextures->get_texture()->set_wrap(glbase::WrapMode::ClampToEdge, glbase::WrapMode::ClampToEdge);

	_opticalDepthMieTextures.reset(new TextureArrayManager());
	_opticalDepthMieTextures->init2D("optical depth mie", _textureSizes, _textureSizes, maxAtmospheres, 1, GL_RED, GL_FLOAT, GL_R32F);
	_opticalDepthMieTextures->get_texture()->set_wrap(glbase::WrapMode::ClampToEdge, glbase::WrapMode::ClampToEdge);

	_KFrMieTextures.reset(new TextureArrayManager());
	_KFrMieTextures->init1D("KFrThetaOverLambda4Mie", _textureSizes, maxAtmospheres, 3, GL_RGB, GL_FLOAT, GL_RGB32F);
	_KFrMieTextures->get_texture()->set_wrap(glbase::WrapMode::ClampToEdge);
	_KFrRayleighTextures.reset(new TextureArrayManager());
	_KFrRayleighTextures->init1D("KFrThetaOverLambda4Rayleigh", _textureSizes, maxAtmospheres, 3, GL_RGB, GL_FLOAT, GL_RGB32F);
	_KFrRayleighTextures->get_texture()->set_wrap(glbase::WrapMode::ClampToEdge);

	_surfaceLightingTextures.reset(new TextureArrayManager());
	_surfaceLightingTextures->init2D("surface lighting", _textureSizes, _textureSizes, maxAtmospheres * maxLights, 3, GL_RGB, GL_FLOAT, GL_RGB32F);
	_surfaceLightingTextures->get_texture()->set_wrap(glbase::WrapMode::ClampToEdge, glbase::WrapMode::ClampToEdge);

	return std::shared_ptr<void>(nullptr, [](void*) { Atmospherics::destroy(); });
}

void Atmospherics::destroy()
{
	_opticalDepthRayleighTextures.reset();
	_opticalDepthMieTextures.reset();
	_KFrMieTextures.reset();
	_KFrRayleighTextures.reset();
	_surfaceLightingTextures.reset();
}

glbase::Texture::ptr Atmospherics::get_optical_depth_rayleigh_textures()
{
	return _opticalDepthRayleighTextures->get_texture();
}

glbase::Texture::ptr Atmospherics::get_optical_depth_mie_textures()
{
	return _opticalDepthMieTextures->get_texture();
}

glbase::Texture::ptr Atmospherics::get_kfr_mie_textures()
{
	return _KFrMieTextures->get_texture();
}

glbase::Texture::ptr Atmospherics::get_kfr_rayleigh_textures()
{
	return _KFrRayleighTextures->get_texture();
}

glbase::Texture::ptr Atmospherics::get_surface_lighting_textures()
{
	return _surfaceLightingTextures->get_texture();
}

AtmosphereParameters::ptr Atmospherics::create_atmosphere(const AtmosphericScattering::Parameters& params)
{
	AtmosphereParameters::ptr atmos(new AtmosphereParameters(params));

	return atmos;
}

//void Atmospherics::dump()
//{
//	_opticalDepthTextures.get_texture()->save("atmospherics_od.png");
//	_KFrThetaOverLambda4MieTextures.get_texture()->save("atmospherics_krm.png");
//	_KFrThetaOverLambda4RayleighTextures.get_texture()->save("atmospherics_krr.png");
//	_surfaceLightingTextures.get_texture()->save("atmospherics_sl.png");
//}


AtmosphereParameters::AtmosphereParameters(const AtmosphericScattering::Parameters& params) 
	: _params(params),
	_readyToDraw(false),
	_beta(AtmosphericScattering::calculate_beta(params.n, params.Ns))
{
}

struct AtmosphereBuildData : public BuildThreadData
{
	AtmosphereBuildData() {}

	void init(const AtmosphericScattering::Parameters& params, int textureSizes, int samples);

	virtual void process(size_t step);
	virtual size_t get_total_work() const;
	virtual size_t get_work_remaining() const;

	size_t get_texture_sizes() const { return _textureSizes; }

	//const math::Atmospheref::vec3_type& get_beta() const { return _atmosphere.get_beta(); }
	const std::vector< float >& get_optical_depth_rayleigh_texture() { return _opticalDepthRayleigh; }
	const std::vector< float >& get_optical_depth_mie_texture() { return _opticalDepthMie; }
	const std::vector< math::Vector3f >& get_surface_lighting_texture() { return _surfaceLighting; }
	const std::vector< math::Vector3f >& get_KFrThetaOverLambda4Mie_texture() { return _KFrThetaOverLambda4Mie; }
	const std::vector< math::Vector3f >& get_KFrThetaOverLambda4Rayleigh_texture() { return _KFrThetaOverLambda4Rayleigh; }

private:
	size_t _workRemaining, _totalWork;
	std::vector< float > _opticalDepthRayleigh, _opticalDepthMie;
	std::vector< math::Vector3f > _surfaceLighting;
	std::vector< math::Vector3f > _KFrThetaOverLambda4Mie, _KFrThetaOverLambda4Rayleigh;
	int _textureSizes;
	int _samples;
	AtmosphericScattering::Parameters _params;
};

void AtmosphereParameters::prepare_for_draw()
{
	if(is_building())
		return ;

	//math::Atmospheref atmos;
	//atmos.set_radii(_planetRadius, _planetRadius + _atmosphereDepth);
	//atmos.set_scale_heights(_scaleHeightRayleigh, _scaleHeightMie);
	//atmos.set_k(_n, _Ns);

	AtmosphereBuildData* buildData = new AtmosphereBuildData();

	_buildData.reset(buildData);
	buildData->init(_params, _textureSizes, _samples);

	DataDistributeThread::get_instance()->queue_data(_buildData);
}

bool AtmosphereParameters::is_ready_to_draw()
{
	if(!is_building())
		return false;

	if(_buildData->is_finished_build())
	{
		AtmosphereBuildData* buildData = static_cast<AtmosphereBuildData*>(_buildData.get());
		_opticalDepthRayleighTextureHandle = _opticalDepthRayleighTextures->reserve(
			static_cast<const GLvoid*>(&(buildData->get_optical_depth_rayleigh_texture()[0])));
		_opticalDepthMieTextureHandle = _opticalDepthMieTextures->reserve(
			static_cast<const GLvoid*>(&(buildData->get_optical_depth_mie_texture()[0])));
		_KFrThetaOverLambda4MieTextureHandle = _KFrMieTextures->reserve(
			static_cast<const GLvoid*>(&(buildData->get_KFrThetaOverLambda4Mie_texture()[0])));
		_KFrThetaOverLambda4RayleighTextureHandle = _KFrRayleighTextures->reserve(
			static_cast<const GLvoid*>(&(buildData->get_KFrThetaOverLambda4Rayleigh_texture()[0])));
		_surfaceLightingTextureHandle = _surfaceLightingTextures->reserve(
			static_cast<const GLvoid*>(&(buildData->get_surface_lighting_texture()[0])));

		_readyToDraw = true;

		_buildData.reset();
	}
	return _readyToDraw;
}

bool AtmosphereParameters::is_building() const
{
	return static_cast<bool>(_buildData);
}

void AtmosphereBuildData::init(const AtmosphericScattering::Parameters& params, int textureSizes, int samples)
{
	reset_flags();
	_params = params;
	_params.depth *= ATMOSPHERE_WORLD_SCALE ;
	_params.innerRadius *= ATMOSPHERE_WORLD_SCALE ;
	_params.scaleHeightRayleigh *= ATMOSPHERE_WORLD_SCALE ;
	_params.scaleHeightMie *= ATMOSPHERE_WORLD_SCALE ;
	_textureSizes = textureSizes;
	_samples = samples;
	_totalWork = _workRemaining = textureSizes * textureSizes * 3 + textureSizes * 2; // not really accurate but whatevs
}

void AtmosphereBuildData::process(size_t step)
{
	using namespace math;

	AtmosphericScattering::calculate_tables(_samples, _textureSizes, _textureSizes, _textureSizes, _params, 
		_opticalDepthRayleigh, _opticalDepthMie, _KFrThetaOverLambda4Rayleigh, _KFrThetaOverLambda4Mie, _surfaceLighting);
	//_atmosphere.calculate_optical_depth_table(_opticalDepthSamples, _opticalDepthTextureSize, _opticalDepthTextureSize);
	//_opticalDepthRayleigh = _atmosphere.get_optical_depth_rayleigh_table();
	//_opticalDepthMie = _atmosphere.get_optical_depth_mie_table();


	//_atmosphere.set_light_wavelength(_wavelength);
	//_atmosphere.calculate_KFrThetaOverLambda4Rayleigh_table(_wavelength, _KFrThetaOverLambda4Samples);
	//_atmosphere.calculate_KFrThetaOverLambda4Mie_table(_atmosphere.get_g(_mieU), _wavelength, _KFrThetaOverLambda4Samples);
	//_KFrThetaOverLambda4Rayleigh = _atmosphere.get_KFrThetaOverLambda4Rayleigh_table();
	//_KFrThetaOverLambda4Mie = _atmosphere.get_KFrThetaOverLambda4Mie_table();
	//_opticalDepth = _atmosphere.get_optical_depth_table();

	//_atmosphere.calculate_surface_lighting_table(_mieU, _surfaceLightingSamples, 
	//	_surfaceLightingSamples, _surfaceLightingSamples, 
	//	_surfaceLightingTextureSize, _surfaceLightingTextureSize, _surfaceLighting);

	_workRemaining = 0;
}

size_t AtmosphereBuildData::get_total_work() const
{
	return _totalWork;
}

size_t AtmosphereBuildData::get_work_remaining() const
{
	return _workRemaining;
}

}