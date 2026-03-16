
#include <boost/variant/get.hpp>

#include "boost_python_wrapper.h"


#include "element.h"
#include "xml_utils.h"

namespace vice {;

void Element::set_text( const std::string& text )
{
	set_property("text", text);
}

std::string Element::get_text() const
{
	return get_property_or_default("text", std::string());
}

void Element::set_color( const Color& color )
{
	set_property("color", color);
}

Color Element::get_color() const
{
	return get_property_or_default("color", Color());
}

void Element::register_type()
{
	using namespace boost::python;
	class_<Element, /*boost::noncopyable,*/ bases<Object>, std::shared_ptr<Element> >("Element", init<std::string>())
		.add_property("text", &Element::get_text, &Element::set_text)
		.add_property("color", &Element::get_color, &Element::set_color)
		;
	//implicitly_convertible<std::shared_ptr<Element>, std::shared_ptr<Object> >();
}

Object::PropertyMap Element::get_edit_properties() const
{
	PropertyMap ourProps;
	ourProps["text"] = std::string();
	ourProps["color"] = Color();
	PropertyMap props = Object::get_edit_properties();
	for (auto prop : props)
	{
		ourProps[prop.first] = prop.second;
	}
	return std::move(ourProps);
}

}
