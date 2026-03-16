
set(src_files
	main.cpp
	)

set(header_files
	)

set(test_files
	test.xml
	test.py
	test2.xml
	test2.py
	)

include_directories(.. ../..)

add_executable(ViceTest ${src_files} ${header_files} ${test_files})

source_group(TestData FILES ${test_files})

target_link_libraries(ViceTest Effect fbo GLBase Vice Math Misc Render Scene Utils SDL2main.lib SDL2.lib tinyxml.lib glew32.lib python27.lib)

set(INSTALL_DIR ${CMAKE_BINARY_DIR}/../bin)

install(TARGETS ViceTest RUNTIME DESTINATION ${INSTALL_DIR})

install(FILES ${test_files} DESTINATION ${INSTALL_DIR}/../data/vice)
