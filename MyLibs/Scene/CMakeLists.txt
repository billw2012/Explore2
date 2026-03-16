
set(src_files
	material.cpp
	)

set(header_files
	animation.hpp
	camera.hpp
	coordtraits.hpp
	drawable.hpp
	geometry.hpp
	group.hpp
	light.hpp
	material.hpp
	matrixtraits.hpp
	mesh.hpp
	meshtransform.hpp
	param_binder.hpp
	scene_types.hpp
	scene_utils.hpp
	sgclipmap.hpp
	targetver.h
	transform.hpp
	viewport.hpp
	)

include_directories(..)

add_library(Scene ${src_files} ${header_files})
