cmake_minimum_required(VERSION 3.15)
project(pjh_json_test VERSION 0.01 LANGUAGES CXX)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    set(PJH_JSON_IS_TOP_LEVEL ON)
else()
    set(PJH_JSON_IS_TOP_LEVEL OFF)
endif()

add_subdirectory(include/pjh_json)

# 如果是独立编译（跑测试/Benchmark的时候），才去搞这些耗时的东西
if(PJH_JSON_IS_TOP_LEVEL)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -O2")

    # ================= 构建控制开关 =================
    # 默认关闭 LTO，以大幅度提升日常编译和链接速度。发布正式版时可开启。
    option(PJH_JSON_ENABLE_LTO "Enable Link Time Optimization (LTO)" OFF)

    # =================================================
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
            /MP # /MP 开启多核编译
            /Zc:char8_t-
        )

        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

        # Windows / MSVC 优化
        add_compile_options(
            $<$<CONFIG:Release>:/O2>
            $<$<CONFIG:Release>:/arch:AVX2>

            $<$<CONFIG:RelWithDebInfo>:/O2>
            $<$<CONFIG:RelWithDebInfo>:/arch:AVX2>
        ) # 开启 O2 优化、全局优化和 AVX2

        # 只有开启 LTO 开关时，才应用导致链接巨慢的全局优化选项
        if(PJH_JSON_ENABLE_LTO)
            add_compile_options(
                $<$<CONFIG:Release>:/GL>
                $<$<CONFIG:RelWithDebInfo>:/GL>
            )
            add_link_options(
                $<$<CONFIG:Release>:/LTCG>
                $<$<CONFIG:RelWithDebInfo>:/LTCG>
            )
        endif()
    else()
        # Linux / macOS (GCC/Clang) 优化
        add_link_options(
            -static-libgcc
            -static-libstdc++
            -static)

        add_compile_options(-O2 -march=native -mtune=native)

        if(PJH_JSON_ENABLE_LTO)
            add_compile_options(-flto=auto)
            add_link_options(-flto)
        endif()
    endif()

    add_subdirectory(include/nlohmann_json)
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
endif()
