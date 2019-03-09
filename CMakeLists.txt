cmake_minimum_required(VERSION 2.8.7)
project( C4_LIB )

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

if(UNIX)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -pthread")
    if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm")
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mfpu=neon")
    else()
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
    endif()
endif()

include_directories(.)

add_subdirectory( c4 )
add_subdirectory( examples )
add_subdirectory( tests )
