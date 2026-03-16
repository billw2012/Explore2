
set(src_files
	RemoteDebug.cpp
	)
	
set(header_files
	RemoteDebug.h
	)

include_directories(..)

add_library(Remote ${src_files} ${header_files})