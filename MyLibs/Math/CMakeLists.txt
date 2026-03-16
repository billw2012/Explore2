
#set(src_files
#	)

set(header_files
	highprec.h
	atmospheric.inl
	perlin.inl
	ridgedmultifractal.inl
	spatialnoise.inl
	aabb.hpp
	atmospheric.hpp
	boundingsphere.hpp
	constants.hpp
	frustum.hpp
	hermite.hpp
	intersection.hpp
	matrix3.hpp
	matrix4.hpp
	misc.hpp
	perlin.hpp
	plane.hpp
	quaternion.hpp
	ray.hpp
	rectangle.hpp
	ridgedmultifractal.hpp
	spatialnoise.hpp
	sse_utils.hpp
	transformation.hpp
	vector2.hpp
	vector3.hpp
	vector4.hpp
	utils.h
	)

include_directories(..)

add_library(Math ${header_files})
