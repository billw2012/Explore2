#pragma once

#include <boost/filesystem/path.hpp>
#include <string>
#include <unordered_map>
#include "object.h"

namespace vice {;

struct ComponentInstance;

struct AnimatedProperty 
{
	friend struct Animation;

	Object::property_variant get(float t) const;
	void insert_key(float t, const Object::property_variant& val);
	//void erase_key(float t);

	struct Key
	{
		Key(float t_ = 0.0f) : t(t_) {}
		Key(float t_, const Object::property_variant& val_) : val(val_), t(t_) {}

		bool operator<(const Key& other) const
		{
			return t < other.t;
		}

		float t;
		Object::property_variant val;
	};

	std::vector<Key> keys;
};

struct ComponentLibrary;

struct ObjectPropertyID
{
	ObjectPropertyID(const std::string& prop_ = std::string(), const std::string& obj_ = std::string())
		: obj(obj_)
		, prop(prop_) {}

	bool operator==(const ObjectPropertyID& other) const
	{
		return obj == other.obj && prop == other.prop;
	}

	std::string obj;
	std::string prop;
};

struct PropIDHash
{
	size_t operator()(const ObjectPropertyID& id) const
	{
		return std::hash<std::string>()(id.obj) ^ std::hash<std::string>()(id.prop);
	}
};

typedef std::unordered_map<ObjectPropertyID, AnimatedProperty, PropIDHash> AnimatedPropertyMap;
struct Animation
{
	typedef std::shared_ptr<Animation> ptr;

	bool is_key_at(const ObjectPropertyID& propID, float t) const;
	bool is_animated(const ObjectPropertyID& propID) const;

	AnimatedPropertyMap properties;
};

struct ComponentTemplate
{
	friend struct ComponentLibrary;
	friend struct ComponentInstance;

	typedef std::shared_ptr<ComponentTemplate> ptr;

	typedef std::unordered_map<std::string, Animation::ptr> AnimationMap;

	void from_instance(const ComponentInstance* inst);

	const AnimationMap& get_animations() const;
	Animation::ptr get_animation(const std::string& name) const;
	Animation::ptr add_animation(const std::string& name);
	void add_animation(const std::string& name, const Animation::ptr& anim);
	void remove_animation(const std::string& name);

private:
	void load(const boost::filesystem::path& file);
	//ComponentInstance* create_instance() const;

	//typedef std::unordered_map<std::string, Object::property_variant> PropertyMap;

	std::string _name;
	boost::filesystem::path _path, _fullPath;

	// Elements
	struct ElementTemplate
	{
		Object::PropertyMap properties;
	};
	std::unordered_map<std::string, ElementTemplate> _elements;

	// Child components
	struct ChildComponentTemplate
	{
		boost::filesystem::path path;
		Object::PropertyMap properties;
	};
	std::unordered_map<std::string, ChildComponentTemplate> _components;

	// Script file references
	std::vector<boost::filesystem::path> _scripts;

	// Properties
	Object::PropertyMap _properties;

	// Animations
	// These use an addressing system to reference any property in the component or 
	// its direct children.

	AnimationMap _animations;
};

}