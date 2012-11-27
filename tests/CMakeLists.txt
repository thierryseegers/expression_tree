include_directories(${PROJECT_SOURCE_DIR}/include)

if(MSVC)
	add_definitions(-D_SCL_SECURE_NO_WARNINGS -DNOMINMAX)
endif()

if(CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS -std=c++0x)
endif()

add_executable(unit catch.hpp correctness.cpp)
set_property(TARGET unit PROPERTY FOLDER "tests")

add_test(add_int_2 unit add/int/2)
add_test(add_int_4 unit add/int/4)
add_test(add_string_2 unit add/string/2)
add_test(add_string_4 unit add/string/4)