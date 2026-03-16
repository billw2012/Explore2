#ifndef _MATH_CONSTANTS_HPP
#define _MATH_CONSTANTS_HPP

namespace math {;
namespace constants {;
const double pi = 3.14159265358979323846;
const float pi_f = 3.14159265358979323846f;
}


template < class V >
V deg_to_rad(V deg)
{
	return (deg / static_cast<V>(180)) * static_cast<V>(constants::pi);
}

template < class V >
V rad_to_deg(V rad)
{
	return (rad / static_cast<V>(constants::pi)) * static_cast<V>(180);
}

}

#endif // _MATH_CONSTANTS_HPP