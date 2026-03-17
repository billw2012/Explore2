
#include "player.h"

namespace explore2 {;


using namespace scene::transform;

void Player::change_frame_of_reference( const Body::ptr& body )
{
	Transform::matrix4_type frameTransform;

	if(_camera->parent())
		frameTransform = _camera->parent()->globalTransform();
	if(body)
		frameTransform = frameTransform * body->get_rotational_reference_frame()->globalTransformInverse();

	_velocity = Transform::vec3_type(frameTransform * Transform::vec4_type(_velocity, Transform::vec4_type::value_type(0)));
	_camera->set_parent(body->get_rotational_reference_frame().get());
	_camera->setTransform(frameTransform * _camera->transform());
	_pendingCameraMatrix = frameTransform * _pendingCameraMatrix;
	// clear any pending collision so we don't mess up the next update with positions in incorrect frame of reference
	_pendingCollision.reset();
}

// camera update:
// 
// update_
void Player::update( double dt )
{
	//Transform::matrix4_type cameraMat = _camera->transform();
	if(_pendingCollision)
	{
		// get the camera pos localized back to the 
		Transform::vec3_type colpos = Transform::vec3_type(_collisionMatrixInverse * Transform::vec4_type(_pendingCollision->get(), Transform::vec4_type::value_type(1)));
		_pendingCameraMatrix.setColumnVector(3, Transform::vec4_type(colpos, Transform::vec4_type::value_type(1)));
		_pendingCameraMatrix.orthoNormalize();
	}
	_camera->setTransform(_pendingCameraMatrix);
	Transform::vec3_type pos = Transform::vec3_type(_pendingCameraMatrix.getColumnVector(3));
	pos += _velocity;
	_pendingCameraMatrix.setColumnVector(3, Transform::vec4_type(pos, Transform::vec4_type::value_type(1)));
	if(_camera->parent())
	{
		_collisionMatrixInverse = _camera->parent()->globalTransformInverse();
		pos = _camera->parent()->globalise(Transform::vec3_type(pos));
	}

	_pendingCollision = _solarSystem->collide(pos, 0.1);
	//_ 
	//if(_pendingCollision)
	//{
	//	// get the camera pos localized back to the 
	//	pos = Transform::vec3_type(_collisionMatrixInverse * Transform::vec4_type(_pendingCollision->get(), Transform::vec4_type::value_type(1)));
	//	_pendingCameraMatrix.setColumnVector(3, Transform::vec4_type(pos, Transform::vec4_type::value_type(1)));
	//	_pendingCameraMatrix.orthoNormalize();
	//	_camera->setTransform(_pendingCameraMatrix);
	//}
	//else
	//{
	//	pos = Transform::vec3_type(cameraMat.getColumnVector(3));
	//}
	//pos += _velocity;

	//if(_camera->parent())
	//{
	//	_collisionMatrixInverse = _camera->parent()->globalTransformInverse();
	//	pos = _camera->parent()->globalise(Transform::vec3_type(pos));
	//}

	//_pendingCollision = _solarSystem->collide(pos);
}

void Player::accelerate_relative( const math::Vector3d& acceleration )
{
	_velocity += 
		acceleration.x * Transform::vec3_type(_pendingCameraMatrix.getColumnVector(0)) + 
		acceleration.y * Transform::vec3_type(_pendingCameraMatrix.getColumnVector(1)) + 
		acceleration.z * Transform::vec3_type(_pendingCameraMatrix.getColumnVector(2));
}

void Player::rotate_relative(const scene::transform::Transform::vec3_type& rot)
{
	// save translate
	Transform::vec4_type trans = _pendingCameraMatrix.getColumnVector(3);
	// calculate new rotate
	Transform::matrix4_type mat = math::rotate_axis_angle(Transform::vec3_type(_pendingCameraMatrix.getColumnVector(0)), rot.x) * 
		math::rotate_axis_angle(Transform::vec3_type(_pendingCameraMatrix.getColumnVector(1)), rot.y) * 
		math::rotate_axis_angle(Transform::vec3_type(_pendingCameraMatrix.getColumnVector(2)), rot.z); 
	// apply rotation
	_pendingCameraMatrix = mat * _pendingCameraMatrix;
	// restore translate
	_pendingCameraMatrix.setColumnVector(3, trans);
}

void Player::set_camera( const scene::transform::Camera::ptr& camera )
{
	_camera = camera;
	_pendingCameraMatrix = _camera->transform();
}

}