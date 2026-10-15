cmake_minimum_required(VERSION 3.21)

project(superpawn LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# ---------------------------------------------------------------------------
# Options
# ---------------------------------------------------------------------------
# The default target is x86-64-v3 (AVX2, BMI2), which covers every x86 CPU from
# about 2015 onwards. Use x86-64-v4 for AVX-512, x86-64 for a generic binary,
# native for the build machine, or none to pass no architecture flag at all
# (the default on non-x86 hosts).
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64|amd64")
  set(SP_ARCH_DEFAULT "x86-64-v3")
else()
  set(SP_ARCH_DEFAULT "none")
endif()
set(SP_ARCH "${SP_ARCH_DEFAULT}" CACHE STRING "Target architecture: x86-64-v3, x86-64-v4, x86-64, native or none")
option(SP_USE_PEXT "Use the PEXT instruction for sliding attacks (fast on Intel and AMD Zen 3+, slow on Zen 1 and 2)" OFF)
option(SP_LTO "Enable link-time optimisation" ON)
option(SP_BUILD_TESTS "Build the unit tests" ON)
option(SP_WERROR "Treat warnings as errors" OFF)

# ---------------------------------------------------------------------------
# Version string from git
# ---------------------------------------------------------------------------
find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/.git")
  execute_process(
    COMMAND "${GIT_EXECUTABLE}" describe --tags --always --dirty
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    OUTPUT_VARIABLE SP_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET)
endif()
if(NOT SP_VERSION)
  set(SP_VERSION "dev")
endif()
message(STATUS "Superpawn version: ${SP_VERSION}")

# ---------------------------------------------------------------------------
# Core library: everything except main(). An object library rather than a
# static archive so that LTO bitcode objects link without a matching ar.
# ---------------------------------------------------------------------------
add_library(superpawn_core OBJECT
  src/benchmark.cpp
  src/bitboard.cpp
  src/eval.cpp
  src/misc.cpp
  src/movegen.cpp
  src/movepick.cpp
  src/perft.cpp
  src/position.cpp
  src/search.cpp
  src/thread.cpp
  src/timeman.cpp
  src/tt.cpp
  src/uci.cpp
  src/ucioption.cpp)

target_include_directories(superpawn_core PUBLIC src)
target_compile_definitions(superpawn_core PUBLIC SP_VERSION="${SP_VERSION}")

find_package(Threads REQUIRED)
target_link_libraries(superpawn_core PUBLIC Threads::Threads)

if(SP_USE_PEXT)
  target_compile_definitions(superpawn_core PUBLIC SP_USE_PEXT)
endif()

# Architecture flags
if(MSVC)
  if(SP_ARCH STREQUAL "x86-64-v4")
    target_compile_options(superpawn_core PUBLIC /arch:AVX512)
  elseif(SP_ARCH STREQUAL "x86-64-v3" OR SP_ARCH STREQUAL "native")
    target_compile_options(superpawn_core PUBLIC /arch:AVX2)
  endif()
  # MSVC does not define __BMI2__; enable PEXT only when asked for explicitly.
  if(SP_ARCH STREQUAL "x86-64-v4" OR SP_ARCH STREQUAL "x86-64-v3")
    target_compile_definitions(superpawn_core PUBLIC __BMI2__=1)
  endif()
else()
  if(NOT SP_ARCH STREQUAL "none")
    target_compile_options(superpawn_core PUBLIC -march=${SP_ARCH})
  endif()
endif()

# Warnings
if(MSVC)
  target_compile_options(superpawn_core PUBLIC /W4 /permissive- /Zc:preprocessor /EHsc)
  if(SP_WERROR)
    target_compile_options(superpawn_core PUBLIC /WX)
  endif()
else()
  target_compile_options(superpawn_core PUBLIC -Wall -Wextra -Wpedantic -Wshadow -Wno-unused-parameter)
  if(SP_WERROR)
    target_compile_options(superpawn_core PUBLIC -Werror)
  endif()
endif()

# Link-time optimisation in release builds
if(SP_LTO)
  include(CheckIPOSupported)
  check_ipo_supported(RESULT SP_IPO_OK OUTPUT SP_IPO_MSG LANGUAGES CXX)
  if(SP_IPO_OK)
    set_property(TARGET superpawn_core PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
  else()
    message(STATUS "LTO not supported: ${SP_IPO_MSG}")
  endif()
endif()

# ---------------------------------------------------------------------------
# Engine executable
# ---------------------------------------------------------------------------
add_executable(superpawn src/main.cpp)
target_link_libraries(superpawn PRIVATE superpawn_core)
if(SP_LTO AND SP_IPO_OK)
  set_property(TARGET superpawn PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
endif()

install(TARGETS superpawn RUNTIME DESTINATION bin)

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
if(SP_BUILD_TESTS)
  enable_testing()

  add_executable(superpawn_tests
    tests/test_main.cpp
    tests/test_bitboard.cpp
    tests/test_eval.cpp
    tests/test_position.cpp
    tests/test_perft.cpp
    tests/test_search.cpp)

  target_include_directories(superpawn_tests PRIVATE tests)
  target_link_libraries(superpawn_tests PRIVATE superpawn_core)
  if(SP_LTO AND SP_IPO_OK)
    # The core objects are LTO bitcode, so every consumer must link with LTO too
    set_property(TARGET superpawn_tests PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
  endif()

  add_test(NAME unit COMMAND superpawn_tests)
  add_test(NAME bench COMMAND superpawn bench 16 1 10)
endif()
