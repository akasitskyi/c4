cmake_minimum_required(VERSION 2.8.7)
project( examples )

include(../dlib/cmake)

add_executable( example example.cpp )

add_executable( ulz_example ulz_example.cpp )

add_executable( matrix_regression_example matrix_regression_example.cpp )

add_executable( object_detection_example object_detection_example.cpp )

target_link_libraries( object_detection_example dlib )
