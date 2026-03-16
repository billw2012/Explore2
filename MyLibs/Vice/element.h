#ifndef element_h__
#define element_h__

#include "object.h"
#include "tinyxml.h"

namespace vice {;


/*
 *	The base element of a vice scene.
 */
struct Element : public Object
{
	typedef python_vice_ptr<Element> ptr;

	Element(const std::string& name = std::string()) : Object(name, ElementType) {}

	void set_text(const std::string& text);
	std::string get_text() const;

	void set_color(const Color& color);
	Color get_color() const;

	// design mode functions >>>
	virtual PropertyMap get_edit_properties() const override;
	// << design mode functions

	static void register_type();

private:

};

}

#endif // element_h__
