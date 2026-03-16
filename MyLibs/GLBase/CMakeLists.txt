
set(src_files
	sdlgl.cpp
	texture.cpp
	triangleset.cpp
	vertexset.cpp
	vramobject.cpp
	videomemorymanager.cpp
	streamingmanager.cpp
	)

set(header_files
	sdlgl.hpp
	texture.hpp
	texture.inl
	vramobject.hpp	
	videomemorymanager.hpp
	triangleset.hpp
	vertexset.hpp
	vertexspec.hpp
	streamingmanager.h
	)

include_directories(..)

add_library(GLBase ${src_files} ${header_files})
