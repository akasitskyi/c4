cmake_minimum_required(VERSION 2.8.7)
project( examples )

if(MSVC)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
endif()

if(UNIX)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse -msse2 -msse3 -mssse3 -msse4.1 -msse4.2 -mavx -mavx2")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")
endif()

include_directories(..)

add_executable( sample C4.cpp )

