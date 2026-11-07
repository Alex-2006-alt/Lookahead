cmake_minimum_required(VERSION 3.25)
project(lookahead VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for language servers
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Compiler warnings and options
if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Sanitizer configuration
option(LOOKAHEAD_ENABLE_SANITIZERS "Enable Address and Undefined Behavior Sanitizers" OFF)
if(LOOKAHEAD_ENABLE_SANITIZERS AND NOT MSVC)
    message(STATUS "Enabling AddressSanitizer and UndefinedBehaviorSanitizer")
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

# Core engine library (to be populated starting Phase 2)
file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS
    src/core/*.cpp
    src/movegen/*.cpp
    src/eval/*.cpp
    src/search/*.cpp
    src/interface/*.cpp
)

if(ENGINE_SOURCES)
    add_library(lookahead_lib STATIC ${ENGINE_SOURCES})
    target_include_directories(lookahead_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

    # Native UCI executable
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)
        add_executable(lookahead src/main.cpp)
        target_link_libraries(lookahead PRIVATE lookahead_lib)
    endif()
else()
    message(STATUS "Phase 1: Engine sources not yet created. Skeleton configured.")
endif()

# Testing configuration
option(LOOKAHEAD_BUILD_TESTS "Build Lookahead Unit Tests" ON)
if(LOOKAHEAD_BUILD_TESTS)
    enable_testing()

    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp
    )

    if(TEST_SOURCES)
        add_executable(lookahead_tests ${TEST_SOURCES})
        target_include_directories(lookahead_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
        target_link_libraries(lookahead_tests PRIVATE GTest::gtest_main)
        if(TARGET lookahead_lib)
            target_link_libraries(lookahead_tests PRIVATE lookahead_lib)
        endif()
        add_test(NAME AllTests COMMAND lookahead_tests)
    endif()
endif()
