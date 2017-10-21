cmake_minimum_required(VERSION 2.8)

project(multimethods)
add_executable(${PROJECT_NAME} "examples/example1.cpp")
include_directories(".")
target_link_libraries(multimethods stdc++)
