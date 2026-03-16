
set(src_files
	fontgl.cpp
	rendergl.cpp
	semantic_param_set.cpp
	utils_screen_space.cpp
	)

set(header_files
	composed_geometryset.h
	fontgl.h
	geometryset.h
	rendergl.h
	memorymanager.hpp
	meshtransformset.hpp
	renderstage.hpp
	scenecontext.hpp
	semantic_param_set.hpp
	utils_screen_space.h
	utils_screen_space.inl
	)

include_directories(..)

add_library(Render ${src_files} ${header_files})
