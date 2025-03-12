cmake_minimum_required(VERSION 3.28)
project( c4 )

set (CMAKE_CXX_STANDARD 20)

include_directories(..)
file(GLOB SOURCES "*.cpp")
file(GLOB HEADERS "*.hpp")
add_library( c4 STATIC ${SOURCES} ${HEADERS} )
