
set(src_files
	vice.cpp
	layer.cpp
	template.cpp
	component.cpp
	logging.cpp
	context.cpp
	script.cpp
	element.cpp
	object.cpp
	xml_utils.cpp
	renderer.cpp
	renderer_base.cpp
	)

set(header_files
	vice.h
	layer.h
	template.h
	component.h
	logging.h
	context.h
	script.h
	element.h
	object.h
	xml_utils.h
	renderer.h
	renderer_base.h
	)

set(shader_files
	shaders/element_shader.xml	
	shaders/element_shader_vs.glsl
	shaders/element_shader_fs.glsl
	shaders/stencil_shader.xml	
	shaders/stencil_shader_vs.glsl
	shaders/stencil_shader_fs.glsl
	shaders/selection_shader.xml	
	shaders/selection_shader_vs.glsl
	shaders/selection_shader_fs.glsl
	)

include_directories(..)


add_library(ViceDesigner ${src_files} ${header_files} ${shader_files})
set_target_properties(ViceDesigner PROPERTIES COMPILE_FLAGS -DVICE_DESIGNER)
source_group(Shaders FILES ${shader_files})
set(INSTALL_DIR ${CMAKE_BINARY_DIR}/../bin)
install(FILES ${shader_files} DESTINATION ${INSTALL_DIR}/../data/vice/shaders)

add_library(Vice ${src_files} ${header_files} ${shader_files})
target_link_libraries(Vice PUBLIC Boost::python Boost::filesystem)
source_group(Shaders FILES ${shader_files})
set(INSTALL_DIR ${CMAKE_BINARY_DIR}/../bin)
install(FILES ${shader_files} DESTINATION ${INSTALL_DIR}/../data/vice/shaders)