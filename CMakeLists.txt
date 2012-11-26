cmake_minimum_required(VERSION 2.6)

project(expression_tree)

set(EXPRESSION_TREE_VERSION_MAJOR 3)
set(EXPRESSION_TREE_VERSION_MINOR 2)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(CMAKE_GENERATOR STREQUAL Xcode)
	set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++11")
	set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")
endif()

add_subdirectory(examples)
add_subdirectory(documentation)

install(FILES ${PROJECT_SOURCE_DIR}/include/expression_tree.h DESTINATION include)
install(DIRECTORY ${PROJECT_BINARY_DIR}/documentation/htdocs DESTINATION documentation)

set(CPACK_GENERATOR ZIP)
set(CPACK_PACKAGE_FILE_NAME expression_tree_${EXPRESSION_TREE_VERSION_MAJOR}_${EXPRESSION_TREE_VERSION_MINOR}) 
include(CPack)