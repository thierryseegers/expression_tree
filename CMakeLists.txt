cmake_minimum_required(VERSION 2.6)

project(expression_tree)

include_directories(${CMAKE_INCLUDE_PATH}) 

link_directories(${CMAKE_LIBRARY_PATH})

add_executable(examples examples.cpp)
 