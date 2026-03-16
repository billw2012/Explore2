
set(src_files
	WaitHandle.cpp
	Profiler.cpp
	time_type.cpp
	)

set(header_files
	WaitHandle.h
	HRTimer.h
	stl_aligned_allocator.h
	Profiler.h
	time_type.h
	)

include_directories(..)

add_library(Utils ${src_files} ${header_files})
