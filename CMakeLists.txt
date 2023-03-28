cmake_minimum_required(VERSION 3.12)
project(phawd)

include_directories(pybind11/include)
include_directories(phawd/include)

set(CMAKE_CXX_STANDARD 11)
add_subdirectory(pybind11)

file(GLOB SOURCES phawd/src/*.cpp)
pybind11_add_module(phawd ${SOURCES})

# EXAMPLE_VERSION_INFO is defined by setup.py and passed into the C++ code as a
# define (VERSION_INFO) here.
target_compile_definitions(phawd PRIVATE VERSION_INFO=${PHAWD_VERSION_INFO})

#if(WIN32)
#	target_link_libraries(phawd ws2_32)
#elseif(UNIX)
#	target_link_libraries(phawd rt)
#endif()
