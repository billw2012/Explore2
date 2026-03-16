#include <boost/range/algorithm.hpp>

// solution
#include "Utils/unique_vector.h"

// project
#include "logging.h"
#include "script.h"
#include "template.h"
#include "component.h"

namespace vice {;

inline std::string null_safe_str(const char* str, std::string default = std::string())
{
	if(str == nullptr)
		return std::string();
	return std::string(str);
}

template < class Cont_ >
void load_properties(Cont_& propMap, const TiXmlElement* parentElement)
{
	for(const TiXmlElement* propNode = parentElement->FirstChildElement("property"); 
		propNode; propNode = propNode->NextSiblingElement("property"))
	{
		std::string propName = null_safe_str(propNode->Attribute("name"));
		Object::PropType::type propType = Object::PropType::from_string(null_safe_str(propNode->Attribute("type")));
		propMap[propName] = Object::string_to_property(Object::StringType(
			null_safe_str(propNode->Attribute("value")), propType));
	}
}

template < class Cont_ >
void load_animation(Cont_& cont, const TiXmlElement* parentElement)
{
	for(const TiXmlElement* propNode = parentElement->FirstChildElement("property"); 
		propNode; propNode = propNode->NextSiblingElement("property"))
	{
		std::string objName = null_safe_str(propNode->Attribute("object"));
		std::string propName = null_safe_str(propNode->Attribute("property"));
		Object::PropType::type propType = Object::PropType::from_string(null_safe_str(propNode->Attribute("type")));
		AnimatedProperty& anim = cont[ObjectPropertyID(propName, objName)];
		for(const TiXmlElement* keyNode = parentElement->FirstChildElement("key"); 
			keyNode; keyNode = keyNode->NextSiblingElement("key"))
		{
			anim.insert_key(parse_val<float>(null_safe_str(propNode->Attribute("t"))),
				Object::string_to_property(Object::StringType(
				null_safe_str(propNode->Attribute("value")), propType)));
		}
	}
}

void ComponentTemplate::load( const boost::filesystem::path& file )
{
	TiXmlDocument doc;
	boost::filesystem::path foundPath = ComponentLibrary::resolve_path(file);
	if (!doc.LoadFile(foundPath.string().c_str()))
	{
		Logging::global().log(Logging::Error) << "ERROR: could not load " << foundPath.string();
		return;
	}

	const TiXmlElement* root = doc.RootElement();
	if(std::string("component") != root->Value())
	{
		Logging::global().log(Logging::Error) << "ERROR: component node not found in " << foundPath.string();
		return;
	}

	// Component name
	_name = null_safe_str(root->Attribute("name"));
	if(_name.empty())
	{
		Logging::global().log(Logging::Error) << "ERROR: component does not have a valid name in " << foundPath.string();
		return;
	}

	_path = file;
	_fullPath = foundPath;

	// Component properties
	load_properties(_properties, root);

	int order = 0;
	// Child elements and components, maintaining file ordering...
	for(const TiXmlElement* node = root->FirstChildElement(); node; node = node->NextSiblingElement(), ++order)
	{
		if(std::string(node->Value()) == "element")
		{
			std::string name = null_safe_str(node->Attribute("name"));
			ElementTemplate& elem = _elements[name];
			//elem.parent = null_safe_str(node->Attribute("parent"));
			elem.properties["order"] = order;
			load_properties(elem.properties, node);
		}
		else if(std::string(node->Value()) == "component")
		{
			std::string name = null_safe_str(node->Attribute("name"));
			ChildComponentTemplate& child = _components[name];
			child.path = null_safe_str(node->Attribute("path"));
			child.path.make_preferred();
			child.properties["order"] = order;
			load_properties(child.properties, node);
		}
	}

	// Animations
	for(const TiXmlElement* node = root->FirstChildElement("animation"); node; node = node->NextSiblingElement("animation"))
	{
		std::string name = null_safe_str(node->Attribute("name"));
		auto anim = std::make_shared<Animation>();
		_animations[name] = anim;
		load_animation(anim->properties, node);
	}

	// Script references
	for(const TiXmlElement* node = root->FirstChildElement("script"); node; node = node->NextSiblingElement("script"))
	{
		boost::filesystem::path script = null_safe_str(node->Attribute("path")); // ComponentLibrary::resolve_path(null_safe_str(node->Attribute("path")), file.parent_path());// / null_safe_str(node->Attribute("path"));
		//script.make_preferred();
		unique_push(_scripts, script);
		ScriptLibrary::register_script(script, foundPath.parent_path());
	}
}

void ComponentTemplate::from_instance(const ComponentInstance* inst)
{
	//TiXmlDocument doc;

	//doc.SaveFile()

	//_properties = inst->get_properties();
	//_name = 
}

const ComponentTemplate::AnimationMap& ComponentTemplate::get_animations() const
{
	return _animations;
}

Animation::ptr ComponentTemplate::get_animation(const std::string& name) const
{
	auto fItr = _animations.find(name);
	if (fItr == _animations.end())
		return Animation::ptr();
	return fItr->second;
}

Animation::ptr ComponentTemplate::add_animation(const std::string& name)
{
	auto anim = std::make_shared<Animation>();
	_animations[name] = anim;
	return anim;
}

void ComponentTemplate::add_animation(const std::string& name, const Animation::ptr& anim)
{
	_animations[name] = anim;
}


void ComponentTemplate::remove_animation(const std::string& name)
{
	_animations.erase(name);
}


float smooth_step(float l, float r, float t)
{
	return (t - l) / (r - l);
}

struct InterpolateVisitor : public boost::static_visitor<Object::property_variant>
{
	InterpolateVisitor(float t) : _t(t) {}

	template <typename T, typename U>
	Object::property_variant operator()( const T &, const U & ) const
	{
		return Object::property_variant();
	}

	template <typename T>
	Object::property_variant operator()( const T & lhs, const T & rhs ) const
	{
		return interpolate(lhs, rhs);
	}

	std::string interpolate(const std::string& l, const std::string& r) const 
	{
		return _t < 0.5f? l : r;
	}

	int interpolate(int l, int r) const 
	{
		return static_cast<int>(static_cast<float>(r - l) * _t + l);
	}

	float interpolate(float l, float r) const
	{
		return (r - l) * _t + l;
	}

	Object::property_variant interpolate(const Color& l, const Color& r) const 
	{
		return Color(
			interpolate(l.r, r.r),
			interpolate(l.g, r.g),
			interpolate(l.b, r.b),
			interpolate(l.a, r.a)
			);
	}

private:
	float _t;
};

Object::property_variant interpolate(const Object::property_variant& l, const Object::property_variant& r, float t)
{
	return boost::apply_visitor(InterpolateVisitor(t), l, r);
}

Object::property_variant AnimatedProperty::get( float t ) const
{
	if(keys.empty())
		return Object::property_variant();
	auto rItr = std::lower_bound(std::begin(keys), std::end(keys), Key(t));
	if(rItr == std::begin(keys))
		return rItr->val;
	if(rItr == std::end(keys))
		return (--rItr)->val;
	auto lItr = rItr - 1;
	float amount = smooth_step(lItr->t, rItr->t, t);
	return interpolate(lItr->val, rItr->val, amount);
}

void AnimatedProperty::insert_key(float t, const Object::property_variant& val)
{
	keys.insert(std::lower_bound(std::begin(keys), std::end(keys), Key(t)), Key(t, val));
}


bool Animation::is_key_at(const ObjectPropertyID& propID, float t) const
{
	auto keys = properties.find(propID);
	if (keys == properties.end())
		return false;
	auto fItr = boost::range::find_if(keys->second.keys, [t](const AnimatedProperty::Key& key) -> bool {
		return key.t == t;
	});
	return fItr != keys->second.keys.end();
}

bool Animation::is_animated(const ObjectPropertyID& propID) const
{
	return properties.find(propID) != properties.end();
}

}