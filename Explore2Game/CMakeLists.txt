
set(src_files
	Explore2.cpp
	RemoteDebugInterface.cpp
	)
	
set(header_files
	RemoteDebugInterface.h
	)

file(GLOB shader_files ../Data/Shaders/*.*)

include_directories(..)
 
add_executable(Explore2Game ${src_files} ${header_files} ${shader_files} game.rc)

target_link_libraries(Explore2Game Explore2Engine Effect fbo GLBase Vice HPCLib HPCNoise Math Misc Remote Render Scene Utils OpenCL.lib SDL2main.lib SDL2.lib tinyxml.lib glew32.lib)

set(INSTALL_DIR ${CMAKE_BINARY_DIR}/../bin)
install(TARGETS Explore2Game RUNTIME DESTINATION ${INSTALL_DIR})

source_group(Shaders FILES ${shader_files})
