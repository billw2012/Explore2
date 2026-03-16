
set(src_files
	perlin.cpp
	)

set(header_files
	perlin.inl
	perlin.hpp
	shader_base.cl
	)

include_directories(..)

add_library(HPCNoise ${src_files} ${header_files})
