cmake_minimum_required(VERSION 2.6)

project(expression_tree)

add_subdirectory(examples)
add_subdirectory(documentation)

install(FILES ${PROJECT_SOURCE_DIR}/include/expression_tree.h DESTINATION include)
install(DIRECTORY ${PROJECT_BINARY_DIR}/documentation/htdocs DESTINATION documentation)

set(CPACK_GENERATOR ZIP)
set(CPACK_PACKAGE_FILE_NAME expression_tree) 
include(CPack)