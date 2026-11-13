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

    # Build configuration branches: WebAssembly (Emscripten) vs Native Executable
    option(LOOKAHEAD_BUILD_WASM "Build Lookahead WebAssembly Artifacts" OFF)
    if(EMSCRIPTEN OR LOOKAHEAD_BUILD_WASM)
        add_executable(lookahead_wasm src/interface/wasm_api.cpp)
        target_link_libraries(lookahead_wasm PRIVATE lookahead_lib)
        set_target_properties(lookahead_wasm PROPERTIES
            OUTPUT_NAME "lookahead"
            SUFFIX ".js"
        )
        target_link_options(lookahead_wasm PRIVATE
            "-O3"
            "-sWASM=1"
            "-sMODULARIZE=1"
            "-sEXPORT_NAME=LookaheadModule"
            "-sALLOW_TABLE_GROWTH=1"
            "-sEXPORTED_FUNCTIONS=['_lookahead_init','_lookahead_send_command','_lookahead_set_output_callback','_lookahead_read_output','_lookahead_shutdown','_malloc','_free']"
            "-sEXPORTED_RUNTIME_METHODS=['ccall','cwrap','addFunction','removeFunction','UTF8ToString','stringToUTF8','allocateUTF8','lengthBytesUTF8']"
            "-sALLOW_MEMORY_GROWTH=1"
            "-sINITIAL_MEMORY=67108864"
            "-sENVIRONMENT=web,worker,node"
        )
    elseif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)
        # Native UCI executable
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
