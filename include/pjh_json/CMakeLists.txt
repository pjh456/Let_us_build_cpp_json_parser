cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(pjh_json INTERFACE)
target_include_directories(pjh_json INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/..)