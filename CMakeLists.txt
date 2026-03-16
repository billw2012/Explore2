cmake_minimum_required (VERSION 2.6)

add_definitions(-D_VARIADIC_MAX=10)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")

project (Explore2)

set (CMAKE_LIBRARY_PATH "C:\\Program Files (x86)\\Windows Kits\\8.0\\Lib\\win8\\um\\x64")

cmake_policy(SET CMP0020 OLD)

find_package(Qt5 REQUIRED Core Gui Widgets)

set(QT_UIC_EXECUTABLE ${Qt5Widgets_UIC_EXECUTABLE})

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

add_subdirectory(MyLibs/Effect)
add_subdirectory(MyLibs/fbo)
add_subdirectory(MyLibs/GLBase)
add_subdirectory(MyLibs/HPCLib)
add_subdirectory(MyLibs/HPCNoise)
add_subdirectory(MyLibs/Math)
add_subdirectory(MyLibs/Misc)
add_subdirectory(MyLibs/Remote)
add_subdirectory(MyLibs/Render)
add_subdirectory(MyLibs/Scene)
add_subdirectory(MyLibs/Ugly)
add_subdirectory(MyLibs/Utils)
add_subdirectory(MyLibs/Vice)
add_subdirectory(MyLibs/Vice/ViceTest)

add_subdirectory(Explore2Engine)

add_subdirectory(Explore2Remote)

add_subdirectory(Explore2Game)

#add_executable(DogeMarkets mainwindow.cpp)
