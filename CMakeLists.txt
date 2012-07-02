project(ascrubber)

cmake_minimum_required(VERSION 2.4)

list(APPEND CMAKE_CXX_FLAGS "-std=c++0x ${CMAKE_CXX_FLAGS}")

SET(CMAKE_MODULE_PATH ${ascrubber_SOURCE_DIR}/CMake)

find_package(FLAC REQUIRED)
include_directories(${FLAC_INCLUDE_DIR})
set(LIBS ${LIBS} ${FLAC_LIBRARIES})

add_executable(ascrubber flacscrubber.cpp main.cpp)

target_link_libraries(ascrubber ${LIBS})
