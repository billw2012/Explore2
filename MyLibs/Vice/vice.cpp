
#include "object.h"
#include "component.h"
#include "element.h"
#include "vice.h"

#include "boost_python_wrapper.h"


namespace vice {;

BOOST_PYTHON_MODULE(Vice)
{
	using namespace vice;
	Color::register_type();
	Object::register_type();
	ComponentInstance::register_type();
	Element::register_type();
}

void init_vice()
{
	initVice();
}

}