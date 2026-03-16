#ifndef _MATH_RECTANGLE_HPP
#define _MATH_RECTANGLE_HPP

#include "Misc/libdesc.hpp"
#include "Misc/hash.hpp"

LIB_NAMESPACE_START

namespace math
{

// simple rectangle type
template < class ValueType >
struct Rectangle
{
	typedef ValueType value_type;
private:
	typedef Rectangle< ValueType > this_type;
public:
	value_type left, right, top, bottom;

	Rectangle(value_type l = static_cast<value_type>(0), value_type b = static_cast<value_type>(0), value_type r = static_cast<value_type>(0), value_type t = static_cast<value_type>(0)) 
		: left(l), right(r), top(t), bottom(b) {}

	template < class OtherType >
	Rectangle(const Rectangle<OtherType>& other) : left(static_cast<value_type>(other.left)), right(static_cast<value_type>(other.right)), top(static_cast<value_type>(other.top)), bottom(static_cast<value_type>(other.bottom)) {}

	bool operator==(const math::Rectangle<value_type>& other) const 
	{
		return left == other.left && right == other.right && top == other.top && bottom == other.bottom;
	}

	value_type width() const { return static_cast<value_type>(std::abs(static_cast<double>(right) - static_cast<double>(left))); }
	value_type height() const { return static_cast<value_type>(std::abs(static_cast<double>(bottom) - static_cast<double>(top))); }

	this_type clamp(const this_type& bounds) const 
	{
		return this_type(std::min(bounds.right, std::max(bounds.left, left)), 
			std::min(bounds.top, std::max(bounds.bottom, bottom)), 
			std::min(bounds.right, std::max(bounds.left, right)), 
			std::min(bounds.top, std::max(bounds.bottom, top)));
	}
};

typedef Rectangle< int > Rectanglei;
typedef Rectangle< float > Rectanglef;
typedef Rectangle< double > Rectangled;

}

LIB_NAMESPACE_END

#endif // _MATH_RECTANGLE_HPP
