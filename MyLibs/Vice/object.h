#ifndef object_h__
#define object_h__

// standard
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <iostream>

// boost
#include <boost/variant.hpp>

// solution
#include "tinyxml.h"
#include "Math/matrix4.hpp"
#include "Math/vector2.hpp"

// project
#include "boost_python_wrapper.h"
#include "xml_utils.h"

namespace vice {;

struct Color
{
	Color(float r_ = 1, float g_ = 1, float b_ = 1, float a_ = 1) : r(r_), g(g_), b(b_), a(a_) {}

	static void register_type();

	float r, g, b, a;
};

template <>
inline Color parse_val<Color>(const std::string& str)
{
	std::stringstream ss(str);
	Color c;
	ss >> c.r >> c.g >> c.b >> c.a;
	return c;
}

template < class Ty_ >
struct python_vice_ptr
{
	typedef Ty_ value_type;
	typedef value_type* ptr_type;
	typedef std::shared_ptr<value_type> shared_ptr_type;
	typedef python_vice_ptr<value_type> this_type;

	template < class OtherTy_ >
	friend struct python_vice_ptr;

	python_vice_ptr() : _viceObj(nullptr) {}

	explicit python_vice_ptr(ptr_type viceObj) : _viceObj(viceObj) {}

	python_vice_ptr(const shared_ptr_type& viceObj, const boost::python::object& pythonObj = boost::python::object())
		: _viceObj(viceObj)
		, _pythonObj(pythonObj) {}

	template < class OtherTy_ >
	python_vice_ptr(const python_vice_ptr<OtherTy_>& other)
		: _viceObj(other._viceObj)
		, _pythonObj(other._pythonObj) {}

	ptr_type operator->() const
	{
		return _viceObj.get();
	}

	value_type& operator*() const
	{
		return *_viceObj;
	}

	operator bool() const 
	{
		return _viceObj != nullptr;
	}

	bool operator==(const this_type& other) const
	{
		return _viceObj == other._viceObj;
	}

	ptr_type get() const 
	{
		return _viceObj.get();
	}

	const shared_ptr_type& ptr() const 
	{
		return _viceObj;
	}

private:
	shared_ptr_type _viceObj;
	boost::python::object _pythonObj;
};

struct ComponentInstance;

struct Object : public std::enable_shared_from_this<Object>
{
	friend struct ComponentInstance;
	friend struct ComponentLibrary;

	enum type 
	{
		ObjectType,
		ElementType,
		ComponentType
	};

	// layout style
	struct PinType { enum type {
		PinMin,
		PinMax,
		Stretch
	};};

	typedef python_vice_ptr<Object> ptr;

	typedef boost::variant < 
		std::string, int, float, Color, PinType::type > property_variant;

	typedef std::unordered_map<std::string, property_variant> PropertyMap;

	Object(const std::string& name = std::string(), type type_ = ObjectType);
	~Object();

	type get_type() const { return _type; }

	void set_name(const std::string& name);
	std::string get_name() const;

	void set_parent(const std::string& str);
	std::string get_parent() const;

	int get_order() const;
	void set_order(int order);

	void set_position(float x, float y);
	void set_x(float x);
	void set_y(float y);
	void set_x_layout(float x);
	void set_y_layout(float y);
	float get_x() const;
	float get_y() const;

	void set_size(float width, float height);
	void set_width(float width);
	void set_height(float height);
	void set_width_layout(float width);
	void set_height_layout(float height);
	float get_width() const;
	float get_height() const;

	PinType::type get_layout_left() const;
	PinType::type get_layout_right() const;
	PinType::type get_layout_top() const;
	PinType::type get_layout_bottom() const;

	void set_rotation(float rotation);
	float get_rotation() const;

	void set_pivot(float pivotX, float pivotY);
	float get_pivot_x() const;
	float get_pivot_y() const;

	void set_property(const std::string& name, const property_variant& value);
	void set_animated_property(const std::string& name, const property_variant& value);
	void set_layout_property(const std::string& name, const property_variant& value);

	property_variant get_property(const std::string& name) const;
	property_variant get_property_base(const std::string& name) const;

	void set_owner(ComponentInstance* other);
	const ComponentInstance* get_owner_const() const;
	ComponentInstance* get_owner();

	const math::Matrix4f& get_transform() const { return transform; }
	const math::Matrix4f& get_global_transform() const { return globalTransform; }
	const math::Matrix4f& get_component_transform() const { return componentTransform; }

	bool is_hit_global(float x, float y) const;
	math::Vector2f from_global(const math::Vector2f& pt) const;
	math::Vector2f from_component(const math::Vector2f& pt) const;
	math::Vector2f to_global(const math::Vector2f& pt) const;
	math::Vector2f to_component(const math::Vector2f& pt) const;

	// design mode functions >>>
	void set_selected(bool selected) { _selected = selected; }
	bool is_selected() const { return _selected; }
	void set_highlighted(bool highlighted) { _highlighted = highlighted; }
	bool is_highlighted() const { return _highlighted; }
	virtual PropertyMap get_edit_properties() const;
	// << design mode functions

	const PropertyMap& get_properties() const { return _properties; }

	static void register_type();

	struct PropType {
		enum type {
			String, Int, Float, Color, Layout
		};
		static std::string to_string(type val);
		static type from_string(const std::string& str);
	};

	struct StringType
	{
		StringType() {}
		StringType(const std::string& val_, PropType::type type_)
			: val(val_), type(type_) {}

		std::string val;
		PropType::type type;
	};

	static StringType property_to_string(const property_variant& prop);
	static property_variant string_to_property(const StringType& strType);

protected:
	virtual void clear_animated_properties();
	virtual void clear_layout_properties();

	template < class Ty_ >
	Ty_ get_property_or_default(const std::string& name, Ty_ default_) const
	{
		auto layout = _layoutProperties.find(name);
		if (layout != _layoutProperties.end())
			return boost::get<Ty_>(layout->second);

		auto animated = _animatedProperties.find(name);
		if(animated != _animatedProperties.end())
			return boost::get<Ty_>(animated->second);

		auto nonAnimatied = _properties.find(name);
		if(nonAnimatied != _properties.end())
			return boost::get<Ty_>(nonAnimatied->second);
		return default_;
	}

	template < class Cont_, class Ty_ >
	static Ty_ get_property_or_default(const Cont_& cont, const std::string& name, Ty_ default_)
	{
		auto layout = cont.find(name);
		if (layout != cont.end())
			return boost::get<Ty_>(layout->second);
		return default_;
	}

private:
	PropertyMap _properties, _animatedProperties, _layoutProperties;
	std::string _name;
	ComponentInstance* _owner;

	// transform relative to parent
	math::Matrix4f transform;
	// absolute global transform
	math::Matrix4f globalTransform;
	// transform relative to owner transform
	math::Matrix4f componentTransform;

	// design mode >>
	bool _highlighted, _selected;
	// << design mode

protected:
	type _type;
};

}

namespace std {
	inline std::ostream& operator<< (std::ostream& stream, const vice::Object::PinType::type& val)
	{
		switch (val)
		{
		case vice::Object::PinType::PinMin:	return (stream << "min");
		case vice::Object::PinType::PinMax:	return (stream << "max");
		case vice::Object::PinType::Stretch:
		default:							return (stream << "stretch");
		};
	}

	inline std::istream& operator>> (std::istream& stream, vice::Object::PinType::type& val)
	{
		std::string pinStr;
		stream >> pinStr;
		if (stream.fail()) return stream;
		if (pinStr == "min")
			val = vice::Object::PinType::PinMin;
		else if (pinStr == "max")
			val = vice::Object::PinType::PinMax;
		else if (pinStr == "stretch")
			val = vice::Object::PinType::Stretch;
		else
			stream.setstate(std::ios_base::failbit);

		return stream;
	}
}

namespace std { ;

template < typename Ty_ >
struct hash< vice::python_vice_ptr<Ty_> >
{
	std::size_t operator()(const vice::python_vice_ptr<Ty_>& val) const
	{
		return std::hash<Ty_*>()(val.get());
	}
};

}
#endif // object_h__
