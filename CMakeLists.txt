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
set(BENCHMARK_ENABLE_GTEST_TESTS OFF)
set(BENCHMARK_USE_LIBCXX OFF)
set(BENCHMARK_ENABLE_LIBPFM OFF)

if(MSVC)
    add_compile_options(
        /utf-8
        /MP
        /Zc:char8_t-
    )

    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

    add_compile_options(
        $<$<CONFIG:Release>:/O2>
        $<$<CONFIG:Release>:/GL>
        $<$<CONFIG:Release>:/arch:AVX2>

        $<$<CONFIG:RelWithDebInfo>:/O2>
        $<$<CONFIG:RelWithDebInfo>:/GL>
        $<$<CONFIG:RelWithDebInfo>:/arch:AVX2>
    )
    add_link_options(
        $<$<CONFIG:Release>:/LTCG>
        $<$<CONFIG:RelWithDebInfo>:/LTCG>
    )
else()
    add_link_options(
        -static-libgcc
        -static-libstdc++
        -static)

    add_compile_options(
        -O2
        -march=native
        -mtune=native
        -flto=auto)
endif()

add_subdirectory(include/nlohmann_json)
add_subdirectory(include/pjh_json)
add_subdirectory(include/benchmark)

add_executable(main test_json.cpp)
target_include_directories(main PRIVATE include/rapidjson/include)
target_link_libraries(main
    PRIVATE nlohmann_json::nlohmann_json
    PRIVATE pjh_json
)

add_executable(test_speed benchmark_test.cpp)
target_include_directories(test_speed PRIVATE include/rapidjson/include)
target_link_libraries(test_speed
    PRIVATE benchmark::benchmark
    PRIVATE nlohmann_json::nlohmann_json
    PRIVATE pjh_json
)
