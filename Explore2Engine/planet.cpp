
#include "planet.h"

namespace explore2 {;
// Planet ---------------------------------------------------------------------------------

void Body::init_planet(const render::GeometrySet::ptr& geometry,
						 const render::GeometrySet::ptr& distantGeometry,
						 const AtmosphereParameters::ptr& atmosphere,
						 float radius, bool water, float waterRadius, 
						 float heightScaleFactor,
						 unsigned short seed)
{
	_planetGeometry.reset(new PlanetGeometry());
	_planetGeometry->init(_rotatedTransform, geometry, distantGeometry, atmosphere, radius,
		waterRadius != 0, waterRadius, 33, 32, 1024, 256, heightScaleFactor, seed);
}

void Body::init_sun(const render::GeometrySet::ptr& geometry,
					const render::GeometrySet::ptr& distantGeometry,
					float radius, unsigned short seed)
{
	_sunGeometry.reset(new SunGeometry());
	_sunGeometry->init(radius, _lightParams->get_light()->get_color(), shared_from_this(), geometry, distantGeometry);
}

void Body::update( const scene::transform::Camera::ptr& camera, time_type t, const scene::transform::Transform::vec3_type& cameraVelocitySeconds )
{
	double tDays = time_type_to_earth_days(t);
	math::Vector3<Orbit::real_type> pos = _orbit.get_position(tDays, _orbitalTilt);
	// final local transform:
	//	rotate to axial tilt
	//	rotate around axis for days
	//	transform to orbital position
	//	rotate by orbital tilt

	// our transform is positional only, so we do not rotationally offset our orbiting bodies.
	setTransform(math::translate(pos) * _orbitalTilt);

	// geometry of course needs to have axial rotation applied as well
	_rotatedTransform->setTransform(_axialTilt * math::rotatez(_axialDailyRotation * tDays));

	if(_planetGeometry)
	{
		_planetGeometry->update(camera, cameraVelocitySeconds);
	}
}

void Body::set_light_params( render::LightingRenderStage::LightParams::ptr lightParams )
{
	_lightParams = lightParams;
}

render::LightingRenderStage::LightParams::ptr Body::get_light_params() const
{
	return _lightParams;
}

void Body::set_planet_index( int index )
{
	if(_planetGeometry)
	{
		render::GeometrySet::ptr& geomSet = _planetGeometry->get_geometry_set();

		for(auto itr = geomSet->begin(); itr != geomSet->end(); ++itr)
		{
			(*itr)->get_material()->set_parameter("PlanetIndex", index);
		}
	}
}

math::BoundingSphered Body::get_bounding_sphere() const
{
	if(_planetGeometry)
		return math::BoundingSphered(_rotatedTransform->centerGlobal(), _planetGeometry->get_atmosphere()->get_planet_radius() + _planetGeometry->get_atmosphere()->get_atmosphere_depth());
	if(_sunGeometry)
		return math::BoundingSphered(_rotatedTransform->centerGlobal(), _sunGeometry->get_radius());
	return math::BoundingSphered();
}

bool Body::is_visible() const
{
	if(_planetGeometry)
		return _planetGeometry->is_visible();
	else if(_sunGeometry)
		return _sunGeometry->is_visible();
	return false;
}

AtmosphereParameters::ptr Body::get_atmosphere() const
{
	if(_planetGeometry)
		return _planetGeometry->get_atmosphere();
	return AtmosphereParameters::ptr();
}

PlanetGeometry::DeferredCollision::ptr Body::collide_planet( scene::transform::Transform::vec3_type pt, double minRange ) const
{
	assert(_planetGeometry);
	return _planetGeometry->collide(pt, minRange);
}

float Body::get_max_height() const
{
	if(_planetGeometry)
		return _planetGeometry->get_max_height();
	return 0.0f;
}

scene::Geometry::ptr Body::get_orbital_path_geometry()
{
	if(!_orbitalGeometry)
		create_orbital_path_geometry();
	return _orbitalGeometry;
}

glbase::VertexSpec::ptr _orbitalPathVertSpec;
effect::Effect::ptr _orbitalPathEffect;

void Body::create_orbital_path_geometry()
{
	static const int ORBIT_PATH_PARTS = 10000;

	glbase::TriangleSet::ptr tris(new glbase::TriangleSet(glbase::TrianglePrimitiveType::LINE_LOOP, ORBIT_PATH_PARTS));
	glbase::VertexSet::ptr verts(new glbase::VertexSet(_orbitalPathVertSpec, ORBIT_PATH_PARTS));
	for(size_t idx = 0; idx < ORBIT_PATH_PARTS; ++idx)
	{
		(*tris)(idx) = (glbase::TriangleSet::value_type)idx;
		*(verts->extract<math::Vector3f>(idx)) = math::Vector3f(_orbit.get_position(0, _orbitalTilt, (idx / (float)ORBIT_PATH_PARTS) * math::constants::pi_f * 2));
	}
	tris->sync_all();
	verts->sync_all();

	scene::Material::ptr material(new scene::Material());
	material->set_effect(_orbitalPathEffect);

	scene::transform::Transform::ptr trans(new scene::transform::Transform());
	if(parent())
		trans->set_parent(parent());
	_orbitalGeometry.reset(new scene::Geometry(tris, verts, material, trans));
}

void Body::static_init()
{
	_orbitalPathVertSpec.reset(new glbase::VertexSpec());
	_orbitalPathVertSpec->append(glbase::VertexData::PositionData, 0, sizeof(float), 3, glbase::VertexElementType::Float);
	
	_orbitalPathEffect.reset(new effect::Effect());
	_orbitalPathEffect->load("../Data/Shaders/orbit_path.xml");
}

void Body::static_release()
{
	_orbitalPathEffect.reset();
	_orbitalPathVertSpec.reset();
}

};