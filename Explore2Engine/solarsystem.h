#ifndef solarsystem_h__
#define solarsystem_h__


#include <memory>
#include <vector>
#include <unordered_map>

#include <boost/filesystem/path.hpp>

#include "tinyxml.h"

#include "engine_constants.h"

#include "Scene/group.hpp"
#include "Render/geometryset.h"

#include "commontypes.h"

#include "gbuffer.h"

#include "planet.h"



//struct PlanetType { enum type {
//	Sun,
//	Planet
//};};

namespace explore2 { ;

struct SolarSystem
{
	typedef std::shared_ptr<SolarSystem> ptr;

	static std::shared_ptr<void> static_init();
	static void static_release();

	SolarSystem();

	void init(render::GeometryRenderStage::ptr renderStage,
		render::GeometryRenderStage::ptr distantStage,
		render::LightingRenderStage::ptr lightingStage,
		render::GeometryRenderStage::ptr scatteringStage,
		render::GeometryRenderStage::ptr orbitalPathStage,
		scene::Material::ptr atmosphereAttenuateMaterial,
		effect::Effect::ptr solarLightingShader,
		effect::Effect::ptr atmosphereScatteringShader,
		GBuffer::ptr gbuffer);

	//void add_planet(const Planet::ptr& planet);
	void generate(size_t seed);
	void load(const boost::filesystem::path& file);
	void load(const TiXmlElement* node);

	Body::ptr load_planet( const TiXmlElement* bodyNode );

	void update(const scene::transform::Camera::ptr& camera, time_type t, const scene::transform::Transform::vec3_type& cameraVelocitySeconds);

	Body::ptr find_planet(const std::string& name) const;
	const std::vector<Body::ptr>& get_planets() const { return _planets; }

	struct DeferredCollision
	{
		friend struct SolarSystem;
		typedef std::shared_ptr<DeferredCollision> ptr;

		scene::transform::Transform::vec3_type get() const;

	private:
		DeferredCollision(const PlanetGeometry::DeferredCollision::ptr& planetCollision, const scene::transform::Transform::matrix4_type& globaliseMatrix);
		DeferredCollision(const scene::transform::Transform::vec3_type& globalpt);

		scene::transform::Transform::vec3_type _globalpt;
		scene::transform::Transform::matrix4_type _globaliseMatrix;
		PlanetGeometry::DeferredCollision::ptr _planetCollision;
	};

	DeferredCollision::ptr collide(const scene::transform::Transform::vec3_type& globalpt_, double minRange = 1.8) const;

private:
	void update_lighting();

private:
	std::vector<Body::ptr> _planets;
	std::vector<Body::ptr> _suns;
	std::string _name;
	scene::transform::Group::ptr _root;

	render::GeometryRenderStage::ptr _renderStage;
	render::LightingRenderStage::ptr _lightingStage;
	render::GeometryRenderStage::ptr _scatteringStage;
	scene::Material::ptr _atmosphereAttenuateMaterial;
	effect::Effect::ptr _solarLightingShader;
	effect::Effect::ptr _atmosphereScatteringShader;
	render::GeometrySet::ptr _distantPlanetGeometrySet;
	render::GeometrySet::ptr _orbitalPathGeometrySet;
	render::GeometrySet::ptr _scatteringGeometrySet;
	GBuffer::ptr _gbuffer;

	struct SunPlanetKey
	{
		Body::ptr sun;
		Body::ptr planet;

		SunPlanetKey(Body::ptr sun_ = Body::ptr(), Body::ptr planet_ = Body::ptr()) : sun(sun_), planet(planet_) {}

		bool operator==(const SunPlanetKey& other) const 
		{
			return sun == other.sun && planet == other.planet;
		}
		// hasher
		size_t operator()(const SunPlanetKey& val) const { return std::hash< Body::ptr > ()(val.sun) & std::hash< Body::ptr > ()(val.planet); }
	};
	typedef std::unordered_map< SunPlanetKey, scene::Geometry::ptr, SunPlanetKey > SunPlanetGeomMap;
	SunPlanetGeomMap _scatteringGeometries;
};

};

#endif // solarsystem_h__
