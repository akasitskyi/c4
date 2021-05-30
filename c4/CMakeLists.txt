cmake_minimum_required(VERSION 3.8.2)
project( c4 )

set (CMAKE_CXX_STANDARD 17)

include_directories(..)
file(GLOB SOURCES "*.cpp")
file(GLOB HEADERS "*.hpp")
add_library( c4 STATIC ${SOURCES} ${HEADERS} )
