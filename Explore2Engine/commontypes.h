
#ifndef commontypes_h__
#define commontypes_h__

// #include "Math/vector2.hpp"
// #include "Math/vector3.hpp"
// #include "Math/vector4.hpp"
// #include "Math/matrix4.hpp"

namespace explore2 {;

// time_type is in ms (int64 can represent 7 billion years at ms resolution).
typedef unsigned __int64 time_type;
double time_type_to_earth_days(time_type t);
double time_type_to_seconds(time_type t);

//typedef double real_type;
// typedef math::Vector2<real_type> Vector2r;
// typedef math::Vector3<real_type> Vector3r;
// typedef math::Vector4<real_type> Vector4r;
// typedef math::Matrix4<real_type> Matrix4r;

};

#endif // commontypes_h__