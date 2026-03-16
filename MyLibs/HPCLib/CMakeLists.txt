
set(src_files
	OpenCLWrappers.cpp
	)

set(header_files
	OpenCLWrappers.h
	OpenCLWrappers.inl
	)

include_directories(..)

add_library(HPCLib ${src_files} ${header_files})
