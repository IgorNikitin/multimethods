cmake_minimum_required(VERSION 2.8)

project(multimethods)
add_executable(${PROJECT_NAME} "benches/bench2.cpp")
include_directories(".")
target_link_libraries(multimethods stdc++)
SET(CMAKE_CXX_COMPILER clang++)
SET(CMAKE_CXX_FLAGS "-std=c++17 -fsanitize=undefined" )
