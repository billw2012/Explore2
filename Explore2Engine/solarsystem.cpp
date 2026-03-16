
#include "solarsystem.h"
#include "commontypes.h"

#include "errorlog.h"

#include "Render/utils_screen_space.h"

#include "GLBase/sdlgl.hpp"

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/random/discrete_distribution.hpp>
#include <boost/random/exponential_distribution.hpp>
#include <boost/random/gamma_distribution.hpp>
#include <boost/random/normal_distribution.hpp>
#include <boost/random/uniform_01.hpp>

using namespace scene::transform;

namespace explore2 { ;


// SolarSystem ----------------------------------------------------------------------------

SolarSystem::SolarSystem() 
	: _root(new scene::transform::Group()),
	_scatteringGeometrySet(new render::GeometrySet()),
	_distantPlanetGeometrySet(new render::GeometrySet()),
	_orbitalPathGeometrySet(new render::GeometrySet())
{
}

void SolarSystem::init(	render::GeometryRenderStage::ptr renderStage,
						render::GeometryRenderStage::ptr distantStage,
						render::LightingRenderStage::ptr lightingStage,
						render::GeometryRenderStage::ptr scatteringStage,
						render::GeometryRenderStage::ptr orbitalPathStage,
						scene::Material::ptr atmosphereAttenuateMaterial,
						effect::Effect::ptr solarLightingShader,
						effect::Effect::ptr atmosphereScatteringShader,
						GBuffer::ptr gbuffer)
{
	_renderStage = renderStage;
	distantStage->get_geometry()->add_geometry_set(_distantPlanetGeometrySet);
	_lightingStage = lightingStage;
	_atmosphereAttenuateMaterial = atmosphereAttenuateMaterial;
	_solarLightingShader = solarLightingShader;
	_atmosphereScatteringShader = atmosphereScatteringShader;
	_scatteringStage = scatteringStage;
	_scatteringStage->get_geometry()->add_geometry_set(_scatteringGeometrySet);
	_gbuffer = gbuffer;
	orbitalPathStage->get_geometry()->add_geometry_set(_orbitalPathGeometrySet);
}

//void SolarSystem::add_planet( const Planet::ptr& planet )
//{
//	_planets.push_back(planet);
//	if(planet->get_geometry())
//		_renderStage->get_geometry()->add_geometry_set(planet->get_geometry()->get_geometry_set());
//	update_lighting();
//}

template < class R_ >
double generate_normal_distribution(R_& rgen, double center, double range)
{
	return boost::random::normal_distribution<>(0, 1)(rgen) * range * 0.3 + center;
}

template < class R_ >
double generate_normal_distribution_clamped(R_& rgen, double center, double range)
{
	return std::min(center + range, std::max(center - range, boost::random::normal_distribution<>(0, 1)(rgen) * range * 0.3 + center));
}

template < class R_ >
double generate_gamma_distribution(R_& rgen, double average, double low)
{
	return (boost::random::gamma_distribution<>(3, 2)(rgen) - 4) * (average - low) * 0.25 + average;
}

template < class R_ >
double generate_exponential_distribution(R_& rgen, double center, double max = std::numeric_limits<double>::max())
{
	return std::min<double>(max, boost::random::exponential_distribution<>(1)(rgen) * center);
}

static const double DISTANCE_SCALE = 93000000 /** WORLD_SCALE*/;

TiXmlElement* create_orbit_element(double orbital_tilt, double long_of_peri, double distance, double orbit_period, double eccentricity,
								   double offset, double axial_tilt, double axial_period)
{
	TiXmlElement* orbit = new TiXmlElement("orbit");
	orbit->SetAttribute("orbital_tilt", std::to_string(orbital_tilt).c_str());
	orbit->SetAttribute("long_of_peri", std::to_string(long_of_peri).c_str());
	orbit->SetAttribute("distance", std::to_string(distance / DISTANCE_SCALE).c_str());
	orbit->SetAttribute("orbit_period", std::to_string(orbit_period).c_str());
	orbit->SetAttribute("eccentricity", std::to_string(eccentricity).c_str());
	orbit->SetAttribute("offset", std::to_string(offset).c_str());
	orbit->SetAttribute("axial_tilt", std::to_string(axial_tilt).c_str());
	orbit->SetAttribute("axial_period", std::to_string(axial_period).c_str());
	return orbit;
}

TiXmlElement* create_light_element(float r, float g, float b)
{
	TiXmlElement* light = new TiXmlElement("light");
	light->SetAttribute("r", std::to_string(r).c_str());
	light->SetAttribute("g", std::to_string(g).c_str());
	light->SetAttribute("b", std::to_string(b).c_str());
	return light;
}

TiXmlElement* create_sun_display_element(double radius, int seed)
{
	TiXmlElement* display = new TiXmlElement("display");
	display->SetAttribute("radius", std::to_string(radius).c_str());
	display->SetAttribute("seed", std::to_string(seed).c_str());
	return display;
}

TiXmlElement* create_atmosphere_element(double atmospheric_depth, double mieU, double scale_height_rayleigh, double scale_height_mie, double n, double Ns)
{
	TiXmlElement* atmosphere = new TiXmlElement("atmosphere");

	atmosphere->SetAttribute("atmospheric_depth",	std::to_string(atmospheric_depth).c_str());
	atmosphere->SetAttribute("mieU",				std::to_string(mieU).c_str());
	atmosphere->SetAttribute("scale_height_rayleigh",std::to_string(scale_height_rayleigh).c_str());
	atmosphere->SetAttribute("scale_height_mie",	std::to_string(scale_height_mie).c_str());
	atmosphere->SetAttribute("n",					std::to_string(n).c_str());
	atmosphere->SetAttribute("Ns",					std::to_string(Ns).c_str());

	return atmosphere;
}

TiXmlElement* create_planet_display_element(double radius, double terrain_height_scale, int seed, double water_radius)
{
	TiXmlElement* display = new TiXmlElement("display");

	display->SetAttribute("radius",				std::to_string(radius).c_str());
	display->SetAttribute("terrain_height_scale",std::to_string(terrain_height_scale).c_str());
	display->SetAttribute("seed",				std::to_string(seed).c_str());
	display->SetAttribute("water_radius",		std::to_string(water_radius).c_str());

	return display;
}

void SolarSystem::generate( size_t seed )
{
	using namespace boost::random;
	mt11213b rgen;
	uniform_int_distribution<> rangeSeed;
	uniform_01<> range01;
	static const float SUN_PROBABILITIES[] = {1/*, 2 , 0.02, 0.001*/};

	int nsuns = discrete_distribution<>(SUN_PROBABILITIES)(rgen) + 1;

	static const double SUN_MASS_AVERAGE = 1.989e30;
	static const double SUN_MASS_LOW = SUN_MASS_AVERAGE * 0.1;
	// for water 1kg = 1 decimeter3, 1000kg = 1 m3, 1000000000000kg = 1 km3
	static const double KG_PER_CUBIC_KM = 1e12;

	auto generate_orbital_tilt = [&]() { return generate_exponential_distribution(rgen, 5, 180); };
	auto create_random_sun_light_element = [&]() { return create_light_element(100, 100, 100); };
	auto radius_from_mass = [](double mass) { return std::pow(mass / (1.25 * math::constants::pi * KG_PER_CUBIC_KM), 1.0/3.0); };
	auto orbital_period_from_mass_distance = [](double massOrbited, double distance) -> double {
		// v = sqrt((GM)/r) where M is the mass of the body being orbited
		static const double G = 6.67e-11;
		// calculate velocity in m/s
		double v = ::sqrt((G * massOrbited) / (distance * 1000));
		// calculate seconds per orbit
		double totalDist = math::constants::pi * ::pow(distance, 2);
		//secondsPerOrbit = totalDist / v
		//orbitsPerSecond = v / totalDist;
		//orbitsPerYear = (60 * 60 * 24 * 365) * (v / totalDist);
		return (60 * 60 * 24 * 365) * (v / totalDist);
	};
	// mass from radius: { return 1.25 * math::constants::pi * std::pow((double)radius, 3) * KG_PER_CUBIC_KM; };
	// orbits are NOT correct for masses...
	// 
	double minPlanetDistance = 0;
	double solarMasses = 0;
	switch(nsuns) 
	{
	case 1: {;
		std::unique_ptr<TiXmlElement> xml(new TiXmlElement("body"));
		double mass = generate_gamma_distribution(rgen, SUN_MASS_AVERAGE, SUN_MASS_LOW);
		double radius = radius_from_mass(mass);
		xml->SetAttribute("type", "sun");
		xml->SetAttribute("mass", std::to_string(mass).c_str());
		xml->LinkEndChild(create_orbit_element(0, 0, 0, 0, 0, 0, 0, 0));
		float solarIntesity = (float)generate_gamma_distribution(rgen, 30, 5);
		xml->LinkEndChild(create_light_element(solarIntesity, solarIntesity, solarIntesity));
		xml->LinkEndChild(create_sun_display_element(radius, rangeSeed(rgen)));
		_planets.push_back(load_planet(xml.get()));
		minPlanetDistance = radius;
		solarMasses = mass;
		break;
			};
	case 2: {;
		double
			orbital_tilt = generate_orbital_tilt(),
			long_of_peri = range01(rgen) * 360.0f, 
			distance = generate_normal_distribution_clamped(rgen, 40000000, 20000000), 
			orbit_period = generate_normal_distribution(rgen, 1, 0.5),
			eccentricity = generate_exponential_distribution(rgen, 0.05, 0.1),
			offset = range01(rgen) * 360.0f;

		{
			std::unique_ptr<TiXmlElement> xml(new TiXmlElement("body"));
			double mass = generate_gamma_distribution(rgen, SUN_MASS_AVERAGE, SUN_MASS_LOW);
			double radius = radius_from_mass(mass);
			xml->SetAttribute("type", "sun");
			xml->SetAttribute("mass", std::to_string(mass).c_str());
			xml->LinkEndChild(create_orbit_element(orbital_tilt, long_of_peri, distance, orbit_period, eccentricity, offset, 0, 0));
			float solarIntesity = (float)generate_gamma_distribution(rgen, 30, 5);
			xml->LinkEndChild(create_light_element(solarIntesity, solarIntesity, solarIntesity));
			xml->LinkEndChild(create_sun_display_element(radius, rangeSeed(rgen)));
			_planets.push_back(load_planet(xml.get()));
			minPlanetDistance = radius + distance;
			solarMasses = mass;
		}
		{
			std::unique_ptr<TiXmlElement> xml(new TiXmlElement("body"));
			double mass = generate_gamma_distribution(rgen, SUN_MASS_AVERAGE, SUN_MASS_LOW);
			double radius = radius_from_mass(mass);
			xml->SetAttribute("type", "sun");
			xml->SetAttribute("mass", std::to_string(mass).c_str());
			xml->LinkEndChild(create_orbit_element(orbital_tilt, long_of_peri, distance, orbit_period, eccentricity, offset + 180.0f, 0, 0));
			float solarIntesity = (float)generate_gamma_distribution(rgen, 30, 5);
			xml->LinkEndChild(create_light_element(solarIntesity, solarIntesity, solarIntesity));
			xml->LinkEndChild(create_sun_display_element(radius, rangeSeed(rgen)));
			_planets.push_back(load_planet(xml.get()));
			minPlanetDistance = std::max<double>(minPlanetDistance, radius + distance);
			solarMasses += mass;
		}
		break;
			};
	};

	// create satellites
	// total mass of satellites based on mass of parent
	// number of satellites based on total mass
	
	//static const double SAT_MASS_SCALAR_CENTER = 0.001f;
	//auto calc_sat_mass = [](double distance, double parentMass) {
	//	return 
	//};

	std::function< void ( Body::ptr, double, int, int, double, double, std::function< int() >, bool ) > gen_sats =
		[&](Body::ptr parent, double parentMass, int countAverage, int countMin, double startDistanceAverage, double startDistanceMin, std::function<int()> typeGen, bool addMoons)
	{
		int nsats = (int)generate_gamma_distribution(rgen, countAverage, countMin);
		double currDist = generate_gamma_distribution(rgen, startDistanceAverage, startDistanceMin);
		for(int idx = 0; idx < nsats; ++idx) 
		{
			int type = typeGen();
			double mass, radius, water_radius = 0, terrain_height_scale = 0.01f /*, atmospheric_depth = 0, mieU = 0, scale_height_rayleigh = 0, scale_height_mie = 0, n = 0, Ns = 0*/;
			TiXmlElement* atmosphereElement = nullptr;
			switch(type)
			{
			case 0: {;// micro
				mass = generate_gamma_distribution(rgen, 1e16, 0.5e16);
				radius = radius_from_mass(mass);
				break;
					}
			case 1: {;// dwarf
				mass = generate_gamma_distribution(rgen, 1e22, 1e21);
				radius = radius_from_mass(mass);
				break;
					}
			case 2: {;// solid
				mass = generate_gamma_distribution(rgen, 1e24, 1e23);
				radius = radius_from_mass(mass);
				//if(range01(rgen) < 0.5) // chance to have an atmosphere
				{
					
					double atmospheric_depth = ::sqrt(radius) * generate_normal_distribution(rgen, 2, 1);
					double scale_height_rayleigh = atmospheric_depth * 0.1f;
					double scale_height_mie = scale_height_rayleigh * 0.2f;
					// aerosol density, on earth 0.70 to 0.85
					double mieU = generate_gamma_distribution(rgen, 0.85, 0);
					// index of refraction n (see http://refractiveindex.info):
					// air: 1.00028
					// ammonia: 1.00038
					// c02: 1.00045
					// methane: 1.00044
					double n = range01(rgen) * 0.00045 + 1;
					// molecular number density:
					// for air: 2.7e25
					double Ns = generate_normal_distribution(rgen, 2.7e25, 2.7e25 * 0.2);
					atmosphereElement = create_atmosphere_element(atmospheric_depth, mieU, scale_height_rayleigh, scale_height_mie, n, Ns);
					if(range01(rgen) < 0.2)
						water_radius = radius;
				}
				break;
					};
			case 3: {;// gas giant
				mass = generate_gamma_distribution(rgen, 1e27, 0.1e27);
				radius = radius_from_mass(mass);
				double atmospheric_depth = 1000;
				double scale_height_rayleigh = atmospheric_depth * 0.5;
				double scale_height_mie = scale_height_rayleigh * 0.2f;
				// aerosol density, on earth 0.70 to 0.85
				double mieU = 0.85f;
				// index of refraction n (see http://refractiveindex.info):
				// air: 1.00028
				// ammonia: 1.00038
				// c02: 1.00045
				// methane: 1.00044
				double n = 1.0001;
				// molecular number density:
				// for air: 2.7e25
				double Ns = 10e25;
				atmosphereElement = create_atmosphere_element(atmospheric_depth, mieU, scale_height_rayleigh, scale_height_mie, n, Ns);
				break;
					};
			};
			double
				orbital_tilt = generate_orbital_tilt(),
				long_of_peri = range01(rgen) * 360.0f, 
				distance = currDist, 
				orbit_period = orbital_period_from_mass_distance(parentMass, distance),
				eccentricity = generate_exponential_distribution(rgen, 0.05, 0.1),
				offset = range01(rgen) * 360.0f;

			std::unique_ptr<TiXmlElement> xml(new TiXmlElement("body"));
			xml->SetAttribute("type", "planet");
			xml->SetAttribute("mass", std::to_string(mass).c_str());
			xml->LinkEndChild(create_orbit_element(orbital_tilt, long_of_peri, distance, orbit_period, eccentricity, offset, 0, 0));
			TiXmlNode* planetElement = xml->LinkEndChild(create_planet_display_element(radius, terrain_height_scale, rangeSeed(rgen), radius/*water_radius*/));
			if(atmosphereElement)
				planetElement->LinkEndChild(atmosphereElement);
			
			Body::ptr planet = load_planet(xml.get());
			if(parent)
				planet->set_parent(parent.get());
			_planets.push_back(planet);
			if(addMoons)
				gen_sats(planet, mass, 1, 0, radius * 100, radius * 10, typeGen, false);
			currDist += currDist * generate_normal_distribution(rgen, 1, 0.25);
		}
	};
	// planet types: 0 = micro, 1 = dwarf, 2 = solid, 3 = gas giant
	static const float PLANET_TYPE_DISTRIBUTION[] = {0, 0, 1, 1};
	discrete_distribution<> planetDistribution(PLANET_TYPE_DISTRIBUTION);
	gen_sats(Body::ptr(), solarMasses, 5, 0, minPlanetDistance + 50000000, minPlanetDistance + 10000000, [&]() -> int { return planetDistribution(rgen); }, true);

	update_lighting();
}

/*	
 *	Solar system rendering:
 *	1/	Update planets.
 *	2/	Gather list of planets that need atmospheric rendering and are ready for it.
 *	3/	Pack atmospheric parameters for each planet in to arrays.
 *	4/	Create or update lights for solar lighting stage for each sun, including light positions.
 *	5/	Assign atmospheric parameter arrays to light materials for lighting stage.
 *	6/	Assign atmospheric parameter arrays to attenuation stage.
 *	7/	Add atmosphere sphere for each planet to scattering phase, and assign atmosphere parameters. 
 *		Flag as transparent for reverse sorting!
 */

static const int NO_ATMOSPHERE_PLANET_INDEX = -1;

void SolarSystem::update( const scene::transform::Camera::ptr& camera, time_type t, const scene::transform::Transform::vec3_type& cameraVelocitySeconds )
{
	std::vector<Body::ptr> planetAtmospheresToRender;
	for(auto itr = _planets.begin(); itr != _planets.end(); ++itr)
	{
		Body::ptr planet = *itr;
		planet->update(camera, t, cameraVelocitySeconds);
		if(planet->is_planet() && planet->is_visible())
		{
			if(planet->has_atmosphere())
			{
				planet->set_planet_index(static_cast<int>(planetAtmospheresToRender.size()));
				planetAtmospheresToRender.push_back(planet);
			}
			else
			{
				planet->set_planet_index(static_cast<int>(NO_ATMOSPHERE_PLANET_INDEX));
			}
		}
	}

	std::vector<math::Matrix4f> cameraToPlanetMatrix;
	std::vector<math::Vector3f> cameraPlanetLocal;
	std::vector<float> outerRadiusSquared;
	std::vector<float> innerRadius;
	std::vector<float> oneOverAtmosphereDepth;
	std::vector<math::Vector3f> beta;
	std::vector<int> opticalDepthRayleighTextureIndex;
	std::vector<int> opticalDepthMieTextureIndex;

	//std::vector<math::Vector3f> v4PiKOverWavelength4;
	std::vector<math::Vector3f> planetCenter;
	std::vector<int> surfaceLightingTextureIndex;

	std::vector<float> scaleHeightRayleigh;
	std::vector<float> scaleHeightMie;
	std::vector<int> kfrMieTextureIndex;
	std::vector<int> kfrRayleighTextureIndex;

	for(size_t idx = 0; idx < planetAtmospheresToRender.size(); ++idx)
	{
		const scene::transform::Group::ptr& planetTransform = planetAtmospheresToRender[idx];
		const AtmosphereParameters::ptr& atmos = planetAtmospheresToRender[idx]->get_atmosphere();
		cameraToPlanetMatrix.push_back(math::Matrix4f(planetTransform->globalTransformInverse() * camera->globalTransform()));
		math::Vector4d cameraPosGlobal(camera->globalTransform().getColumnVector(3));
		cameraPlanetLocal.push_back(math::Vector3f(planetTransform->globalTransformInverse() * cameraPosGlobal));		
		outerRadiusSquared.push_back(std::pow(atmos->get_planet_radius() + atmos->get_atmosphere_depth(), 2));
		innerRadius.push_back(atmos->get_planet_radius());
		oneOverAtmosphereDepth.push_back(1 / atmos->get_atmosphere_depth());
		beta.push_back(atmos->get_beta());
		scaleHeightRayleigh.push_back(atmos->get_parameters().scaleHeightRayleigh);
		scaleHeightMie.push_back(atmos->get_parameters().scaleHeightMie);

		//v4PiKOverWavelength4.push_back(math::Atmospheref::get_beta(atmos->get_n(), atmos->get_Ns()));
		planetCenter.push_back(math::Vector3f(camera->localise(planetTransform->centerGlobal())));
		surfaceLightingTextureIndex.push_back(atmos->get_SL_texture_handle()->get_slot());
		opticalDepthRayleighTextureIndex.push_back(atmos->get_ODRayleigh_texture_handle()->get_slot());
		opticalDepthMieTextureIndex.push_back(atmos->get_ODMie_texture_handle()->get_slot());
		kfrMieTextureIndex.push_back(atmos->get_KFrMie_texture_handle()->get_slot());
		kfrRayleighTextureIndex.push_back(atmos->get_KFrRayleigh_texture_handle()->get_slot());
	}

	_atmosphereAttenuateMaterial->set_parameter("CameraToPlanetMatrix",		cameraToPlanetMatrix);
	_atmosphereAttenuateMaterial->set_parameter("CameraPlanetLocal",		cameraPlanetLocal);
	_atmosphereAttenuateMaterial->set_parameter("OuterRadiusSquared",		outerRadiusSquared);
	_atmosphereAttenuateMaterial->set_parameter("InnerRadius",				innerRadius);
	_atmosphereAttenuateMaterial->set_parameter("OneOverAtmosphereDepth",	oneOverAtmosphereDepth);
	_atmosphereAttenuateMaterial->set_parameter("Beta",						beta);
	_atmosphereAttenuateMaterial->set_parameter("OpticalDepthRayleighTextureIndex", opticalDepthRayleighTextureIndex);
	_atmosphereAttenuateMaterial->set_parameter("OpticalDepthMieTextureIndex", opticalDepthMieTextureIndex);

	std::unordered_set<SunPlanetKey, SunPlanetKey> inactiveScatteringGeometries;
	// assume all are inactive, then remove active ones later
	for(auto itr = _scatteringGeometries.begin(); itr != _scatteringGeometries.end(); ++itr)
		inactiveScatteringGeometries.insert(itr->first);

	// setup lighting: single full screen pass for each sun
	for(auto sunItr = _suns.begin(); sunItr != _suns.end(); ++sunItr)
	{
		const Body::ptr& sun = *sunItr;
		const scene::Material::ptr& lightMat = sun->get_light_params()->get_light_material();

		lightMat->set_parameter("InnerRadius",					innerRadius);
		lightMat->set_parameter("OneOverAtmosphereDepth",		oneOverAtmosphereDepth);
		lightMat->set_parameter("Beta",							beta);
		lightMat->set_parameter("PlanetCenter",					planetCenter);

		lightMat->set_parameter("SurfaceLightingTextureIndex",	surfaceLightingTextureIndex);
		lightMat->set_parameter("OpticalDepthRayleighTextureIndex",	opticalDepthRayleighTextureIndex);
		lightMat->set_parameter("OpticalDepthMieTextureIndex",	opticalDepthMieTextureIndex);

		lightMat->set_parameter("LightColor",					sun->get_light_params()->get_light()->get_color());
		//math::Vector3f cameraLocalLightPos = math::Vector3f(camera->localise(sun->centerGlobal()));
		lightMat->set_parameter("CameraLocalLightPos",			math::Vector3f(camera->localise(sun->centerGlobal())));


		// setup scattering: single quad, multiple pass for each planet
		for(size_t planetIdx = 0; planetIdx < planetAtmospheresToRender.size(); ++planetIdx)
		{
			const Body::ptr& planet = planetAtmospheresToRender[planetIdx];

			math::Rectanglei ssRect = render::utils::get_screenspace_rect(camera, planet->get_bounding_sphere()).
				clamp(math::Rectanglei(0, 0, glbase::SDLGl::width(), glbase::SDLGl::height()));
			SunPlanetKey sunPlanetKey(sun, planet);
			//const AtmosphereParameters::ptr& atmos = planet->get_geometry()->get_atmosphere();
			scene::Geometry::ptr& geom = _scatteringGeometries[sunPlanetKey];
			scene::Material::ptr geomMat;
			if(!geom)
			{
				// vertex/tri sets for the quads are cached by x,y,wid,hgt so this doesn't create a new one unless it didn't exist
				// WARNING: when switching to bounded quads, DON'T DO THIS. It will create one quad for every possible combination...
				geom = render::utils::create_new_screen_quad(ssRect.left, ssRect.bottom, ssRect.width(), ssRect.height());
				geomMat.reset(new scene::Material());
				geomMat->set_effect(_atmosphereScatteringShader);
				geom->set_material(geomMat);
				// set material static parameters
				geomMat->set_parameter("OuterRadiusSquared",				outerRadiusSquared[planetIdx]);
				geomMat->set_parameter("InnerRadius",						innerRadius[planetIdx]);
				geomMat->set_parameter("OneOverAtmosphereDepth",			oneOverAtmosphereDepth[planetIdx]);
				geomMat->set_parameter("Beta",								beta[planetIdx]);
				geomMat->set_parameter("ScaleHeightRayleigh",				scaleHeightRayleigh[planetIdx]);
				geomMat->set_parameter("ScaleHeightMie",					scaleHeightMie[planetIdx]);
				geomMat->set_parameter("Is",								sun->get_light_params()->get_light()->get_color());
				geomMat->set_parameter("OpticalDepthRayleighTexture",		Atmospherics::get_optical_depth_rayleigh_textures());
				geomMat->set_parameter("OpticalDepthMieTexture",			Atmospherics::get_optical_depth_mie_textures());
				geomMat->set_parameter("KFrMieTexture",						Atmospherics::get_kfr_mie_textures());
				geomMat->set_parameter("KFrRayleighTexture",				Atmospherics::get_kfr_rayleigh_textures());
				geomMat->set_parameter("OpticalDepthRayleighTextureIndex",	opticalDepthRayleighTextureIndex[planetIdx]);
				geomMat->set_parameter("OpticalDepthMieTextureIndex",		opticalDepthMieTextureIndex[planetIdx]);
				geomMat->set_parameter("KFrMieTextureIndex",				kfrMieTextureIndex[planetIdx]);
				geomMat->set_parameter("KFrRayleighTextureIndex",			kfrRayleighTextureIndex[planetIdx]);
				geomMat->set_parameter("AlbedoBufferTexture",				_gbuffer->get_albedo());
				geomMat->set_parameter("PositionBufferTexture",				_gbuffer->get_position());
				geomMat->set_parameter("NormalBufferTexture",				_gbuffer->get_normal());

				_scatteringGeometrySet->insert(geom);
			}
			else
			{
				render::utils::update_screen_quad(geom, ssRect.left, ssRect.bottom, ssRect.width(), ssRect.height());
				geomMat = geom->get_material();
			}

			geomMat->set_parameter("PlanetIndex",			(int)planetIdx);
			geomMat->set_parameter("CameraToPlanetMatrix",	cameraToPlanetMatrix[planetIdx]);
			geomMat->set_parameter("CameraPlanetLocal",		cameraPlanetLocal[planetIdx]);
			geomMat->set_parameter("PlanetLocalLightPos",	math::Vector3f(planet->localise(sun->centerGlobal())));

			inactiveScatteringGeometries.erase(sunPlanetKey);
		}
	}

	// remove inactive scattering geometries now
	for(auto itr = inactiveScatteringGeometries.begin(); itr != inactiveScatteringGeometries.end(); ++itr)
	{
		_scatteringGeometrySet->erase(_scatteringGeometries[*itr]);
		_scatteringGeometries.erase(*itr);
	}
}

void SolarSystem::update_lighting()
{
	_lightingStage->remove_all_lights();
	_suns.clear();

	for(auto itr = _planets.begin(); itr != _planets.end(); ++itr)
	{
		Body::ptr planet = *itr;
		if(planet->get_light_params())
		{
			_lightingStage->add_light(planet->get_light_params());
			_suns.push_back(planet);
		}
	}
}

/*
Longitude of Perihelion (p) degrees
states the position in the orbit where the planet is closest to the Sun.
Mean distance (a) generic distance
the value of the semi-major axis of the orbit - measured in Astronomical 
Units for the major planets.
Daily motion (n) degrees
states how far in degrees the planet moves in one (mean solar) day. This 
figure can be used to find the mean anomaly of the planet for a given 
number of days either side of the date of the elements. The figures 
quoted in the Astronomical Almanac do not tally with the period of the 
planet as calculated by applying Kepler's 3rd Law to the semi-major axis.
Eccentricity (e) scalar
eccentricity of the ellipse which describes the orbit
e = 0 		- circle
0 < e < 1 	- eclipse
Mean Longitude (l) degrees
position of the planet in the orbit on the date of the elements.

<solar_system name = "Sol" xsec = "0" ysec = "0" xfrac = "0.5" yfrac = "0.5" civ = "Terran">
	<body name = "Sol" type = "sun1" p = "0" a = "0" n = "0" e = "0" l = "0"/>
	<body name = "Mercury" type = "lava_planet" p = "0" a = "0.25" n = "3" e = "0" l = "0"/>
	<body name = "Venus" type = "rocky_planet" p = "0" a = "0.5" n = "1.5" e = "0" l = "0"/>
	<body name = "Earth" type = "gaia_planet" p = "0" a = "1" n = "1.25" e = "0" l = "0"/>
	<station name = "test_station" type = "test_station_type" 
			distance = "100" angular_offset = "0" angular_velocity = "1" parent = "Earth">
		<trade_type type = "Industrial"/>
	</station> 
	<station name = "test_station2" type = "test_station_type"
			distance = "100" angular_offset = "180" angular_velocity = "1" parent = "Earth">
		<trade_type type = "Agrarian"/>
	</station> 
	<body name = "Mars" type = "rocky_planet" p = "0" a = "1.8" n = "0.625" e = "0" l = "0"/>
	<body name = "Jupiter" type = "gas_planet" p = "0" a = "3.0" n = "0.3125" e = "0" l = "0"/>
	<body name = "Saturn" type = "gas_planet" p = "0" a = "5.5" n = "0.15" e = "0" l = "0"/>
	<body name = "Uranus" type = "gas_planet" p = "0" a = "10" n = "0.075" e = "0" l = "0"/>
	<body name = "Neptune" type = "gas_planet" p = "0" a = "17" n = "0.075" e = "0" l = "0"/>
	<body name = "Pluto" type = "ice_planet" p = "0" a = "30" n = "0.0325" e = "0.1" l = "0"/>
</solar_system>
*/
void SolarSystem::load( const boost::filesystem::path& file )
{
	TiXmlDocument doc;
	if(!doc.LoadFile(file.string().c_str()))
	{
		logfile << "ERROR: could not load " << file.string() << std::endl;
		return;
	}
	const TiXmlElement* root = doc.RootElement();
	if(std::string("solar_system") != root->Value())
	{
		logfile << "ERROR: solar_system node not found in " << file.string() << std::endl;
		return;
	}

	load(root);
}

std::string null_safe_str(const char* str, std::string default = std::string())
{
	if(str == nullptr)
		return std::string();
	return std::string(str);
}

template < class Ty_ >
Ty_ lexical_cast(const std::string& str)
{
	std::stringstream ss(str);
	Ty_ val;
	ss >> val;
	return val;
}

template < class Ty_ >
Ty_ lexical_cast(const std::string& str, Ty_ default_)
{
	std::stringstream ss(str);
	Ty_ val;
	ss >> val;
	if(ss.fail())
		return default_;
	return val;
}

void SolarSystem::load( const TiXmlElement* node )
{
	std::map<Body::ptr, std::string> parenting;
	std::map<std::string, Body::ptr> nameMap;

	_name = null_safe_str(node->Attribute("name"));
	_root->set_name(_name);

	for(const TiXmlElement* bodyNode = node->FirstChildElement("body"); bodyNode != nullptr; bodyNode = bodyNode->NextSiblingElement("body"))
	{
		Body::ptr planet = load_planet(bodyNode);

		std::string parent = null_safe_str(bodyNode->Attribute("parent"));
		if(!parent.empty())
			parenting[planet] = parent;

		_planets.push_back(planet);

		nameMap[planet->get_name()] = planet;
	}

	for(auto itr = parenting.begin(); itr != parenting.end(); ++itr)
	{
		itr->first->set_parent(nameMap[itr->second].get());
	}

	update_lighting();
}

Body::ptr SolarSystem::load_planet( const TiXmlElement* bodyNode )
{
	Body::ptr planet(new Body());

	planet->set_name(null_safe_str(bodyNode->Attribute("name")));
	double mass = lexical_cast<Orbit::real_type>(null_safe_str(bodyNode->Attribute("mass"), "0"));
	planet->set_mass(mass);

	// types:
	// sun, planet
	std::string type = null_safe_str(bodyNode->Attribute("type"));

	const TiXmlElement* orbitNode = bodyNode->FirstChildElement("orbit");
	if(orbitNode)
	{
		Orbit::real_type orbitalTilt	= lexical_cast<Orbit::real_type>(null_safe_str(orbitNode->Attribute("orbital_tilt"), "0"));	// orbital tilt
		Orbit::real_type longOfPeri		= lexical_cast<Orbit::real_type>(null_safe_str(orbitNode->Attribute("long_of_peri"), "0"));	// longitude of perihelion degrees
		Orbit::real_type meanDistance	= lexical_cast<Orbit::real_type>(null_safe_str(orbitNode->Attribute("distance"), "0"));		// mean distance km
		Orbit::real_type orbitalPeriod	= lexical_cast<Orbit::real_type>(null_safe_str(orbitNode->Attribute("orbit_period"), "0"));	// orbital period orbits / earth year
		Orbit::real_type eccentricity	= lexical_cast<Orbit::real_type>(null_safe_str(orbitNode->Attribute("eccentricity"), "0"));	// eccentricity 
		Orbit::real_type meanLongitude	= lexical_cast<Orbit::real_type>(null_safe_str(orbitNode->Attribute("offset"), "0"));		// mean longitude degrees

		Orbit::real_type axialTilt		= lexical_cast<Orbit::real_type>(null_safe_str(orbitNode->Attribute("axial_tilt"), "0"));	// axial tilt in degrees
		Orbit::real_type axialPeriod	= lexical_cast<Orbit::real_type>(null_safe_str(orbitNode->Attribute("axial_period"), "0"));	// axial rotation period rotations / earth day

		planet->set_axial_rotation((axialPeriod == 0)? 0 : (360 * (1 / axialPeriod)), math::rotatex(axialTilt));
		planet->set_orbit(Orbit(math::deg_to_rad(longOfPeri), meanDistance * DISTANCE_SCALE * WORLD_SCALE, 360 * (orbitalPeriod / 365), eccentricity, math::deg_to_rad(meanLongitude)), math::rotatex(orbitalTilt));
		_orbitalPathGeometrySet->insert(planet->get_orbital_path_geometry());
	}

	const TiXmlElement* lightNode = bodyNode->FirstChildElement("light");
	if(lightNode)
	{
		static const float SUN_POWER_MULTIPLIER = 1e20f;
		float sun_r	= lexical_cast<float>(null_safe_str(lightNode->Attribute("r"))) * SUN_POWER_MULTIPLIER;	
		float sun_g	= lexical_cast<float>(null_safe_str(lightNode->Attribute("g"))) * SUN_POWER_MULTIPLIER;	
		float sun_b	= lexical_cast<float>(null_safe_str(lightNode->Attribute("b"))) * SUN_POWER_MULTIPLIER;	

		render::LightingRenderStage::LightParams::ptr lightParams(new render::LightingRenderStage::LightParams());
		scene::transform::Light::ptr light(new scene::transform::Light(planet->get_name()));
		light->set_type(scene::transform::LightType::Solar);
		light->set_parent(planet.get());
		light->set_color(math::Vector3f(sun_r, sun_g, sun_b));
		lightParams->set_light(light);
		scene::Material::ptr lightMat(new scene::Material());
		lightMat->set_effect(_solarLightingShader);
		lightMat->set_parameter("AlbedoBuffer",				_gbuffer->get_albedo());
		lightMat->set_parameter("NormalBuffer",				_gbuffer->get_normal());
		lightMat->set_parameter("PositionBuffer",			_gbuffer->get_position());
		lightMat->set_parameter("DepthBuffer",				_gbuffer->get_depth());
		lightMat->set_parameter("LightColor",				light->get_color());
		lightMat->set_parameter("SurfaceLightingTexture",	Atmospherics::get_surface_lighting_textures());
		lightMat->set_parameter("OpticalDepthRayleighTexture",	Atmospherics::get_optical_depth_rayleigh_textures());
		lightMat->set_parameter("OpticalDepthMieTexture",	Atmospherics::get_optical_depth_mie_textures());

		lightParams->set_light_material(lightMat);

		planet->set_light_params(lightParams);
	}

	const TiXmlElement* displayNode = bodyNode->FirstChildElement("display");
	if(displayNode)
	{
		float radius			= lexical_cast<float>(	null_safe_str(displayNode->Attribute("radius")));				// radius (absolute, average from center) km
		unsigned short seed		= lexical_cast<int>(	null_safe_str(displayNode->Attribute("seed")), 0);

		if(type == "planet")
		{
			float waterRadius		= lexical_cast<float>(null_safe_str(displayNode->Attribute("water_radius")));		// water radius (absolute, from center) km
			float scaleHeightFactor	= lexical_cast<float>(null_safe_str(displayNode->Attribute("terrain_height_scale")), 0.01f);	// atmospheric depth (from radius) km

			AtmosphereParameters::ptr atmosphere;
			const TiXmlElement* atmosphereNode = displayNode->FirstChildElement("atmosphere");
			if(atmosphereNode)
			{
				float atmosphericDepth	= lexical_cast<float>(null_safe_str(atmosphereNode->Attribute("atmospheric_depth")));	// atmospheric depth (from radius) km
				float mieU				= lexical_cast<float>(null_safe_str(atmosphereNode->Attribute("mieU")), 0.80f);
				float scaleHeightRaleigh= lexical_cast<float>(null_safe_str(atmosphereNode->Attribute("scale_height_rayleigh")), atmosphericDepth * 0.08f);
				float scaleHeightMie	= lexical_cast<float>(null_safe_str(atmosphereNode->Attribute("scale_height_mie")), atmosphericDepth * 0.012f);
				float n					= lexical_cast<float>(null_safe_str(atmosphereNode->Attribute("n")), 1.0003f);
				float Ns				= lexical_cast<float>(null_safe_str(atmosphereNode->Attribute("Ns")), 2.5e25f);

				atmosphere = Atmospherics::create_atmosphere(AtmosphericScattering::Parameters(
					AtmosphericScattering::float_type(radius * WORLD_SCALE), AtmosphericScattering::float_type(atmosphericDepth * WORLD_SCALE), 
					AtmosphericScattering::float_type(scaleHeightRaleigh * WORLD_SCALE), AtmosphericScattering::float_type(scaleHeightMie * WORLD_SCALE), mieU, n, Ns));
			}

			render::GeometrySet::ptr geometrySet(new render::GeometrySet());
			_renderStage->get_geometry()->add_geometry_set(geometrySet);
			planet->init_planet(geometrySet, _distantPlanetGeometrySet, atmosphere, float(radius * WORLD_SCALE), waterRadius != 0, float(waterRadius * WORLD_SCALE), scaleHeightFactor, seed);
		}
		else if(type == "sun")
		{
			assert(lightNode); // can't be a sun without a lightsource
			render::GeometrySet::ptr geometrySet(new render::GeometrySet());
			_renderStage->get_geometry()->add_geometry_set(geometrySet);
			planet->init_sun(geometrySet, _distantPlanetGeometrySet, float(radius * WORLD_SCALE), seed);
		}
	}
	return planet;
}

Body::ptr SolarSystem::find_planet( const std::string& name ) const
{
	auto fItr = std::find_if(_planets.begin(), _planets.end(), [&](const Body::ptr& planet) -> bool { return planet->get_name() == name; });
	if(fItr == _planets.end())
		return Body::ptr();
	return *fItr;
}

SolarSystem::DeferredCollision::ptr SolarSystem::collide( const scene::transform::Transform::vec3_type& globalpt_, double minRange /*= 1.8*/ ) const
{
	for(auto itr = _planets.begin(); itr != _planets.end(); ++itr)
	{
		Body::ptr planet = *itr;
		if(planet->is_visible())
		{
			Transform::vec3_type planetLocalPt = planet->get_rotational_reference_frame()->localise(globalpt_);
			if(planetLocalPt.length() <= planet->get_max_height())
			{
				if(planet->is_planet())
					return SolarSystem::DeferredCollision::ptr(new SolarSystem::DeferredCollision(planet->collide_planet(planetLocalPt, minRange), planet->get_rotational_reference_frame()->globalTransform()));
				//else
				//	return SolarSystem::DeferredCollision::ptr(new SolarSystem::DeferredCollision(planet->collide_planet(planetLocalPt, minRange)));				
			}
		}
	}
	return SolarSystem::DeferredCollision::ptr(new SolarSystem::DeferredCollision(globalpt_));
}

std::shared_ptr<void> SolarSystem::static_init()
{
	PlanetGeometry::static_init();
	SunGeometry::static_init();
	AtmosphericScattering::static_init();
	Body::static_init();

	return std::shared_ptr<void>(nullptr, [](void*) { SolarSystem::static_release(); });
}

void SolarSystem::static_release()
{
	Body::static_release();
	SunGeometry::static_release();
	PlanetGeometry::static_release();
}

scene::transform::Transform::vec3_type SolarSystem::DeferredCollision::get() const
{
	if(_planetCollision)
	{
 		return Transform::vec3_type(_globaliseMatrix * Transform::vec4_type(_planetCollision->get(), 1.0));
	}
	else
	{
		return _globalpt;
	}
}

SolarSystem::DeferredCollision::DeferredCollision( const PlanetGeometry::DeferredCollision::ptr& planetCollision, const scene::transform::Transform::matrix4_type& globaliseMatrix )
	: _planetCollision(planetCollision),
	_globaliseMatrix(globaliseMatrix)
{

}

SolarSystem::DeferredCollision::DeferredCollision( const scene::transform::Transform::vec3_type& globalpt )
	: _globalpt(globalpt)
{

}

};
