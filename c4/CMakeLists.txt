cmake_minimum_required(VERSION 2.8.7)
project( c4 )

include_directories(..)
file(GLOB SOURCES "*.cpp")
file(GLOB HEADERS "*.hpp")
add_library( c4 STATIC ${SOURCES} ${HEADERS} )
