// standard
#include <unordered_set>

// boost
#include <boost/range/algorithm.hpp>
#include <boost/variant/static_visitor.hpp>

// solution
#include "Utils/unique_vector.h"
#include "Math/vector4.hpp"

// project
#include "boost_python_wrapper.h"
#include "object.h"
#include "xml_utils.h"
#include "logging.h"
#include "component.h"


namespace vice {;

struct ObjectReg
{
	~ObjectReg()
	{
		if(!activeObjects.empty())
		{
			Logging::global().log(Logging::Error) << activeObjects.size() << " Vice objects leaked.";
		}
	}

	std::unordered_set<Object*> activeObjects;
};

static ObjectReg gReg;

Object::Object(const std::string& name /*= std::string()*/, type type_ /*= ObjectType*/) 
	: _name(name)
	, _owner(nullptr)
	, _type(type_)
	, _highlighted(false)
	, _selected(false)
{
	gReg.activeObjects.insert(this);
}

Object::~Object()
{
	// don't do this, if this is last ref then we ALREADY are being removed from the last child list...
	//set_owner(nullptr);
	gReg.activeObjects.erase(this);
}

void Object::set_name( const std::string& name )
{
	_name = name;
}

std::string Object::get_name() const
{
	return _name;
}

void Object::set_parent(const std::string& str)
{
	set_property("parent", str);
}

int Object::get_order() const
{
	return get_property_or_default("order", 0);
}

void Object::set_order(int order)
{
	set_property("order", order);
}

std::string Object::get_parent() const
{
	return get_property_or_default("parent", std::string());
}

void Object::set_position( float x, float y )
{
	set_x(x);
	set_y(y);
}

void Object::set_x(float x)
{
	set_property("x", x);
}

void Object::set_y(float y)
{
	set_property("y", y);
}

void Object::set_x_layout(float x)
{
	set_layout_property("x", x);
}

void Object::set_y_layout(float y)
{
	set_layout_property("y", y);
}

float Object::get_x() const
{
	return get_property_or_default("x", 0.0f);
}

float Object::get_y() const
{
	return get_property_or_default("y", 0.0f);
}

void Object::set_size( float width, float height )
{
	set_width(width);
	set_height(height);
}

void Object::set_width(float width)
{
	set_property("width", width);
}

void Object::set_height(float height)
{
	set_property("height", height);
}

void Object::set_width_layout(float width)
{
	set_layout_property("width", width);
}

void Object::set_height_layout(float height)
{
	set_layout_property("height", height);
}

float Object::get_width() const
{
	return get_property_or_default("width", 0.0f);
}

float Object::get_height() const
{
	return get_property_or_default("height", 0.0f);
}


Object::PinType::type Object::get_layout_left() const
{
	return get_property_or_default("layout_left", PinType::PinMin);
}

Object::PinType::type Object::get_layout_right() const
{
	return get_property_or_default("layout_right", PinType::PinMin);
}

Object::PinType::type Object::get_layout_top() const
{
	return get_property_or_default("layout_top", PinType::PinMin);
}

Object::PinType::type Object::get_layout_bottom() const
{
	return get_property_or_default("layout_bottom", PinType::PinMin);
}

void Object::set_property( const std::string& name, const property_variant& value )
{
	_properties[name] = value;
}

void Object::set_animated_property( const std::string& name, const property_variant& value )
{
	_animatedProperties[name] = value;
}

void Object::set_layout_property(const std::string& name, const property_variant& value)
{
	_layoutProperties[name] = value;
}

Object::property_variant Object::get_property_base(const std::string& name) const
{
	auto nonAnimatied = _properties.find(name);
	if (nonAnimatied != _properties.end())
		return nonAnimatied->second;
	return property_variant();
}

Object::property_variant Object::get_property(const std::string& name) const
{
	auto animated = _animatedProperties.find(name);
	if (animated != _animatedProperties.end())
		return animated->second;
	auto nonAnimatied = _properties.find(name);
	if (nonAnimatied != _properties.end())
		return nonAnimatied->second;
	return property_variant();
}

void Object::register_type()
{
	using namespace boost::python;
	class_<Object/*, boost::noncopyable*/, std::shared_ptr<Object> >("Object", init<std::string>())
		.add_property("name", &Object::get_name, &Object::set_name, "Object name")
		.add_property("parent", &Object::get_parent, &Object::set_parent, "Parent name, containing element")
		.add_property("x", &Object::get_x, &Object::set_x, "Object x coordinate")
		.add_property("y", &Object::get_y, &Object::set_y, "Object y coordinate")
		.add_property("width", &Object::get_width, &Object::set_width, "Object width")
		.add_property("height", &Object::get_height, &Object::set_height, "Object height")
		.add_property("owner", make_function(&Object::get_owner, return_value_policy<reference_existing_object>()), &Object::set_owner, "Owning component")
	;
}

void Object::set_owner(ComponentInstance* other)
{
	auto oldOwner = _owner;
	_owner = other;
	if(_owner == oldOwner)
		return ;
	if(oldOwner)
	{
		auto itr = std::find_if(oldOwner->_children.begin(), oldOwner->_children.end(), 
			[this](const Object::ptr& obj) -> bool {
				return obj.get() == this;
		});

		//ERROR HERE: Object erasure causes shared_ptr to deref for a second time, double delete etc..
		//circular dependency
		if(itr != oldOwner->_children.end()) 
			oldOwner->_children.erase(itr);
		//unique_erase(_parent->_children, shared_from_this());
	}
	if(_owner) 
		unique_push(_owner->_children, shared_from_this());
}

const ComponentInstance* Object::get_owner_const() const
{
	return _owner;
}

ComponentInstance* Object::get_owner()
{
	return _owner;
}

bool Object::is_hit_global(float x, float y) const
{
	math::Vector2f localPos = from_global(math::Vector2f(x, y));
	
	return localPos.x >= 0.0f && localPos.x <= get_width() &&
		localPos.y >= 0.0f && localPos.y <= get_height();
}

math::Vector2f Object::from_global(const math::Vector2f& pt) const
{
	return math::Vector2f(globalTransform.inverse() * math::Vector4f(pt, 0.0f, 1.0f));
}

math::Vector2f Object::from_component(const math::Vector2f& pt) const
{
	return math::Vector2f(componentTransform.inverse() * math::Vector4f(pt, 0.0f, 1.0f));
}

math::Vector2f Object::to_global(const math::Vector2f& pt) const
{
	return math::Vector2f(globalTransform * math::Vector4f(pt, 0.0f, 1.0f));
}

math::Vector2f Object::to_component(const math::Vector2f& pt) const
{
	return math::Vector2f(componentTransform * math::Vector4f(pt, 0.0f, 1.0f));
}

void Object::clear_animated_properties()
{
	_animatedProperties.clear();
}

void Object::clear_layout_properties()
{
	_layoutProperties.clear();
}

void Color::register_type()
{
	using namespace boost::python;
	class_<Color>("Color")
		.def_readwrite("r", &Color::r)
		.def_readwrite("g", &Color::g)
		.def_readwrite("b", &Color::b)
		.def_readwrite("a", &Color::a);
}

struct ToStringVisitor : public boost::static_visitor < >
{
	ToStringVisitor(Object::StringType& strType) : _strType(strType) {}

	void operator()(const std::string& val) const
	{
		_strType = Object::StringType(val, Object::PropType::String);
	}

	void operator()(int val) const
	{
		std::stringstream ss;
		ss << val;
		_strType = Object::StringType(ss.str(), Object::PropType::Int);
	}


	void operator()(float val) const
	{
		std::stringstream ss;
		ss << val;
		_strType = Object::StringType(ss.str(), Object::PropType::Float);
	}


	void operator()(const Color& val) const
	{
		std::stringstream ss;
		ss << val.r << " " << val.g << " " << val.b << " " << val.a;
		_strType = Object::StringType(ss.str(), Object::PropType::Color);
	}

	void operator()(Object::PinType::type val) const
	{
		std::stringstream ss;
		ss << val;
		_strType = Object::StringType(ss.str(), Object::PropType::Layout);
	}


private:
	Object::StringType& _strType;
};

Object::StringType Object::property_to_string(const property_variant& prop)
{
	StringType str;
	boost::apply_visitor(ToStringVisitor(str), prop);
	return str;
}


Object::property_variant Object::string_to_property(const StringType& strType)
{
	switch (strType.type)
	{
	case PropType::Int:
		return parse_val<int>(strType.val);
	case PropType::Float:
		return parse_val<float>(strType.val);
	case PropType::Color:
		return parse_val<Color>(strType.val);
	case PropType::Layout:
		return parse_val<Object::PinType::type>(strType.val);
	default:
		return strType.val;
	};
}


std::string Object::PropType::to_string(type val)
{
	switch (val)
	{
	case PropType::Int:		return "int";
	case PropType::Float:	return "float";
	case PropType::Color:	return "color";
	case PropType::Layout:	return "layout";
	default:				return "string";
	};
}

vice::Object::PropType::type Object::PropType::from_string(const std::string& str)
{
	if (str == "int")		return PropType::Int;
	if (str == "float")		return PropType::Float;
	if (str == "color")		return PropType::Color;
	if (str == "layout")	return PropType::Layout;
	return String;
}

Object::PropertyMap Object::get_edit_properties() const
{
	PropertyMap props;
	props["parent"] = std::string();
	props["order"] = 0;
	props["x"] = props["y"] = props["width"] = props["height"] = 0.0f;
	props["layout_left"] = props["layout_right"] = props["layout_top"] = props["layout_bottom"] = PinType::PinMin;
	for (auto prop : _properties)
	{
		props[prop.first] = prop.second;
	}
	return std::move(props);
}

}