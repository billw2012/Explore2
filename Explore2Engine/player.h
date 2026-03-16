#ifndef player_h__
#define player_h__

#include "Scene/camera.hpp"
#include "solarsystem.h"

namespace explore2 {;

struct Player
{
	typedef std::shared_ptr<Player> ptr;

	void set_camera(const scene::transform::Camera::ptr& camera);
	void set_solar_system(const SolarSystem::ptr& solarSystem)		{ _solarSystem = solarSystem; }
	void change_frame_of_reference(const Body::ptr& body);
	void update(double dt);
	void accelerate_relative(const scene::transform::Transform::vec3_type& acceleration);
	void stop() { _velocity = scene::transform::Transform::vec3_type::Zero; }
	const scene::transform::Transform::vec3_type& get_velocity() const { return _velocity; }
	void rotate_relative(const scene::transform::Transform::vec3_type& rot);

private:
	SolarSystem::ptr _solarSystem;
	scene::transform::Camera::matrix4_type _pendingCameraMatrix;
	scene::transform::Transform::vec3_type _velocity;
	scene::transform::Camera::ptr _camera;
	scene::transform::Transform::matrix4_type _collisionMatrixInverse;
	SolarSystem::DeferredCollision::ptr _pendingCollision;
};

}

#endif // player_h__
