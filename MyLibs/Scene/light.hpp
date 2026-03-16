#ifndef _SCENE_LIGHT_H
#define _SCENE_LIGHT_H


#include "matrixtraits.hpp"
#include "coordtraits.hpp"

namespace scene {;

namespace transform {;

struct LightType { enum type {
	Solar,
	Directional,
	Point,
	Spot
};};

struct Light : public Group
{
	typedef std::shared_ptr< Light > ptr;

private:
	LightType::type _type;
	math::Vector3f _color/*, _wavelength*/;
	math::Vector3f _dir;
	//float _brightness;
	unsigned int _shadowMapSize;
	float _maxShadowDistance, _maxOccluderDistance;

public:
	Light(const std::string& name = "unnamedLight") : Group(name), _type(LightType::Point), _shadowMapSize(256), _maxShadowDistance(5000.0f), _maxOccluderDistance(1000.0f) {}

	virtual Transform* clone() const
	{
		Light* newLight = new Light(*this);
		clone_data(newLight);
		return newLight;
	}

	void set_type(LightType::type type_) { _type = type_; }
	LightType::type get_type() const { return _type; }
	
	//void set_dir(const math::Vector3f& dir_) { _dir = dir_; }
	//const math::Vector3f& get_dir() const { return _dir; }

	void set_color(const math::Vector3f& col) { _color = col; }
	const math::Vector3f& get_color() const { return _color; }

	//void set_wavelength(const math::Vector3f& wavelength) { _wavelength = wavelength; }
	//const math::Vector3f& get_wavelength() const { return _wavelength; }

	//void set_brightness(float b) { _brightness = b; }
	//float get_brightness() const { return _brightness; }

	virtual NodeType::type get_node_type() const
	{
		return NodeType::Light;
	}

	unsigned int get_shadow_map_size() const { return _shadowMapSize; }
	void set_shadow_map_size(unsigned int shadowMapSize) { _shadowMapSize = shadowMapSize; }


	void set_max_shadow_distance(float maxShadowDistance) { _maxShadowDistance = maxShadowDistance; }
	float get_max_shadow_distance() const { return _maxShadowDistance; }

	void set_max_shadow_occluder_distance(float maxOccluderDistance) { _maxOccluderDistance = maxOccluderDistance; }
	float get_max_shadow_occluder_distance() const { return _maxOccluderDistance; }
};
}
}

#endif // _SCENE_CAMERA_H
