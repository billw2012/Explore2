
set(src_files
	framebufferObject.cpp
	renderbuffer.cpp
	)
	
set(header_files
	framebufferObject.h
	renderbuffer.h
	)

include_directories(..)

add_library(fbo ${src_files} ${header_files})
