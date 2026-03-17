
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

target_link_libraries(Explore2Game
    Explore2Engine Effect fbo GLBase HPCLib HPCNoise Math Misc Remote Render Scene Utils Vice hpalib
    OpenCL::OpenCL
    SDL2::SDL2 SDL2::SDL2main
    GLEW::GLEW
    unofficial-tinyxml::unofficial-tinyxml
    ${IL_LIBRARIES} ${ILU_LIBRARIES} ${ILUT_LIBRARIES}
    Boost::thread Boost::filesystem Boost::serialization
)

set(INSTALL_DIR ${CMAKE_BINARY_DIR}/../bin)
install(TARGETS Explore2Game RUNTIME DESTINATION ${INSTALL_DIR})

source_group(Shaders FILES ${shader_files})
