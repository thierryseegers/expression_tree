include_directories(${PROJECT_SOURCE_DIR}/include)

if(MSVC)
	add_definitions(-D_SCL_SECURE_NO_WARNINGS -DNOMINMAX)
endif()

if(CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS -std=c++0x)
endif()

add_executable(unit catch.hpp correctness.cpp)
set_property(TARGET unit PROPERTY FOLDER "tests")

add_test(single_leaf_int unit single_leaf_int)
add_test(single_leaf_string unit single_leaf_string)
add_test(add_two_ints unit add_two_ints)
add_test(add_four_ints unit add_four_ints)
add_test(add_two_strings unit add_two_strings)
add_test(add_four_strings unit add_four_strings)