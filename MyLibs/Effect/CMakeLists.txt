
set(src_files
	effect.cpp
	glstate.cpp
	shadermanager.cpp
	stdafx.cpp
	)
	
set(header_files
	effect.h
	glstate.h
	shadermanager.h
	stdafx.h
	targetver.h
	)

include_directories(..)

add_library(Effect ${src_files} ${header_files})
