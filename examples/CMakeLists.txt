include_directories(${CMAKE_INCLUDE_PATH} ${PROJECT_SOURCE_DIR}/include) 

link_directories(${CMAKE_LIBRARY_PATH})

if(CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS -std=c++0x)
endif()

add_executable(examples main.cpp ${PROJECT_SOURCE_DIR}/include/expression_tree.h)

if(CMAKE_COMPILER_IS_GNUCXX)
	target_link_libraries(examples pthread rt)
endif()
