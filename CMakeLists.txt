cmake_minimum_required(VERSION 3.0.0)
project(void VERSION 0.1.0)

include(CTest)
enable_testing()

add_library(void INTERFACE)
add_library(void::void ALIAS void)
target_include_directories(void INTERFACE include)

add_subdirectory(test)
