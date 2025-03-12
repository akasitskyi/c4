cmake_minimum_required(VERSION 3.28)
project( C4_LIB )

set (CMAKE_CXX_STANDARD 20)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

if(UNIX)
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
