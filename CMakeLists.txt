cmake_minimum_required(VERSION 2.8.7)
project( C4_LIB )

if(UNIX)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

include_directories(.)

add_subdirectory( c4 )
add_subdirectory( examples )
add_subdirectory( tests )
