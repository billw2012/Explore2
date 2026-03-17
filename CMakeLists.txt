cmake_minimum_required (VERSION 3.15)

add_definitions(-D_VARIADIC_MAX=10 -D_SILENCE_STDEXT_HASH_DEPRECATION_WARNINGS)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /EHsc /std:c++17")

project (Explore2)

cmake_policy(SET CMP0020 OLD)

find_package(Qt5 REQUIRED Core Gui Widgets)

set(QT_UIC_EXECUTABLE ${Qt5Widgets_UIC_EXECUTABLE})

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

find_package(OpenCL REQUIRED)
find_package(SDL2 CONFIG REQUIRED)
find_package(GLEW REQUIRED)
find_package(tinyxml CONFIG REQUIRED)
find_package(DevIL REQUIRED)
find_package(Boost REQUIRED COMPONENTS thread filesystem serialization python)

# Make external headers available to all subprojects
include_directories(LegacyMyLibs)
include_directories(${OpenCL_INCLUDE_DIRS})
include_directories(${IL_INCLUDE_DIR})
include_directories(${Boost_INCLUDE_DIRS})
get_target_property(_SDL2_INC SDL2::SDL2 INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(_GLEW_INC GLEW::GLEW INTERFACE_INCLUDE_DIRECTORIES)
include_directories(${_SDL2_INC})
include_directories(${_GLEW_INC})
# Python headers for boost-python (Python 3.12 installed by vcpkg as boost-python dependency)
include_directories(${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows/include/python3.12)

add_subdirectory(LegacyMyLibs/hpalib)
add_subdirectory(LegacyMyLibs/Effect)
add_subdirectory(LegacyMyLibs/fbo)
add_subdirectory(LegacyMyLibs/GLBase)
add_subdirectory(LegacyMyLibs/HPCLib)
add_subdirectory(LegacyMyLibs/HPCNoise)
add_subdirectory(LegacyMyLibs/Math)
add_subdirectory(LegacyMyLibs/Misc)
add_subdirectory(LegacyMyLibs/Remote)
add_subdirectory(LegacyMyLibs/Render)
add_subdirectory(LegacyMyLibs/Scene)
add_subdirectory(LegacyMyLibs/Ugly)
add_subdirectory(LegacyMyLibs/Utils)
add_subdirectory(LegacyMyLibs/Vice)
#add_subdirectory(LegacyMyLibs/Vice/ViceTest)   # ViceTest needs boost-python at runtime

add_subdirectory(Explore2Engine)

#add_subdirectory(Explore2Remote)    # requires Qwt — skipped for now

add_subdirectory(Explore2Game)
