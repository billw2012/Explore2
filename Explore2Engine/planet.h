#ifndef planet_h__
#define planet_h__

#include "planetgeometry.h"
#include "sungeometry.h"
#include "orbit.h"
#include "commontypes.h"

namespace explore2 {;

struct Body : public scene::transform::Group
{
	typedef std::shared_ptr<Body> ptr;

	static void static_init();
	static void static_release();

	Body() 
		: _mass(0)
		, _axialDailyRotation(0)
		, _rotatedTransform(new scene::transform::Group())
	{
		_rotatedTransform->set_parent(this);
	}

	void set_name(const std::string& name) { _name = name; }
	const std::string& get_name() const { return _name; }

	void set_mass(double mass) { _mass = mass; }
	double get_mass() const { return _mass; }

	void set_axial_rotation(Orbit::real_type axialDailyRotation, const math::Matrix4<Orbit::real_type>& axialTilt) { _axialDailyRotation = axialDailyRotation; _axialTilt = axialTilt; }
	void set_orbit(const Orbit& orbit, const math::Matrix4<Orbit::real_type>& orbitalTilt) { _orbit = orbit; _orbitalTilt = orbitalTilt; }
	void set_parent(const Body::ptr& parent);

	void init_planet(const render::GeometrySet::ptr& geometry,
		const render::GeometrySet::ptr& distantGeometry,
		const AtmosphereParameters::ptr& atmosphere,
		float radius, bool water, float waterRadius, 
		float heightScaleFactor,
		unsigned short seed);
	void init_sun(const render::GeometrySet::ptr& geometry,
		const render::GeometrySet::ptr& distantGeometry,
		float radius, unsigned short seed);

	scene::Geometry::ptr get_orbital_path_geometry();

	bool is_planet() const { return (bool)_planetGeometry; }
	bool has_atmosphere() const { return _planetGeometry && _planetGeometry->get_atmosphere(); }
	bool is_visible() const;
	//void set_geometry(const PlanetGeometry::ptr& geometry) { _geometry = geometry; }
	//PlanetGeometry::ptr get_geometry() const { return _geometry; }

	void set_light_params(render::LightingRenderStage::LightParams::ptr lightParams);
	render::LightingRenderStage::LightParams::ptr get_light_params() const;

	AtmosphereParameters::ptr get_atmosphere() const;

	PlanetGeometry::DeferredCollision::ptr collide_planet(scene::transform::Transform::vec3_type pt, double minRange) const;
	float get_max_height() const;

	void update(const scene::transform::Camera::ptr& camera, time_type t, const scene::transform::Transform::vec3_type& cameraVelocitySeconds);

	void load(const TiXmlElement* node);

	void set_planet_index( int index );

	const scene::transform::Group::ptr& get_rotational_reference_frame() const { return _rotatedTransform; }

	math::BoundingSphered get_bounding_sphere() const;

	virtual void set_parent(Transform* parent) 
	{ 
		scene::transform::Group::set_parent(parent);
		if(_orbitalGeometry)
			_orbitalGeometry->get_transform()->set_parent(parent);
	}

private:
	void create_orbital_path_geometry();

	// tilt of orbit
	math::Matrix4<Orbit::real_type> _orbitalTilt;
	// tilt of planets rotational axis
	math::Matrix4<Orbit::real_type> _axialTilt;
	// axial rotational speed
	Orbit::real_type _axialDailyRotation;

	Orbit _orbit;

	// if it has geometry
	PlanetGeometry::ptr _planetGeometry;
	SunGeometry::ptr _sunGeometry;

	std::string _name;
	// if its a sun
	render::LightingRenderStage::LightParams::ptr _lightParams;

	// mass in kg
	double _mass;

	scene::transform::Group::ptr _rotatedTransform;

	scene::Geometry::ptr _orbitalGeometry;
};

}

#endif // planet_h__
