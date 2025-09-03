cmake_minimum_required(VERSION 3.15)
project(json_test VERSION 0.01 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -O2")

set(JSON_BuildTests OFF CACHE INTERNAL "")
set(BENCHMARK_ENABLE_TESTING OFF)
set(BENCHMARK_ENABLE_INSTALL OFF)
set(BENCHMARK_INSTALL_DOCS OFF)
set(BENCHMARK_INSTALL_TOOLS OFF)

add_subdirectory(include/nlohhman_json)
add_subdirectory(include/pjh_json)
add_subdirectory(include/benchmark)

add_executable(main test_json.cpp)
target_include_directories(main PRIVATE include/rapid_json/include)
target_link_libraries(main
    PRIVATE nlohmann_json::nlohmann_json
    PRIVATE pjh_json
)

add_executable(test_speed benchmark_test.cpp)
target_include_directories(test_speed PRIVATE include/rapid_json/include)
target_link_libraries(test_speed
    PRIVATE benchmark::benchmark
    PRIVATE nlohmann_json::nlohmann_json
    PRIVATE pjh_json
)
