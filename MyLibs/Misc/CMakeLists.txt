
#set(src_files
#	)

set(header_files
	platform.h
	timer.h
	atomic.hpp
	filefuncs.hpp
	hash.hpp
	libdesc.hpp
	rangelist.hpp
	simdtypes.hpp
	statemachine.hpp
	waithandle.hpp
	functions.hpp
	)

include_directories(..)

add_library(Misc ${header_files})
