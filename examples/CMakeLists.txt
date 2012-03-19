include_directories(${CMAKE_INCLUDE_PATH} ${PROJECT_SOURCE_DIR}/include) 

link_directories(${CMAKE_LIBRARY_PATH})

add_executable(examples main.cpp ${PROJECT_SOURCE_DIR}/include/expression_tree.h)
