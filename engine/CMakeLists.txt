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

# Testing options
option(LOOKAHEAD_BUILD_TESTS "Build Lookahead Unit Tests" OFF)
if(LOOKAHEAD_BUILD_TESTS)
    enable_testing()
    # GoogleTest integration will be activated in Phase 2
endif()
