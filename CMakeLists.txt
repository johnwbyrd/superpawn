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
option(SP_STATIC "Link statically so the binary runs without runtime libraries (release builds)" OFF)
if(SP_STATIC AND MSVC)
  # Must be set before any target is created
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
set(SP_EMBED_NET "${CMAKE_SOURCE_DIR}/nets/default.nnue" CACHE FILEPATH "NNUE network to embed, or OFF for none")

# ---------------------------------------------------------------------------
# Version string from git
# ---------------------------------------------------------------------------
# Release tags are vMAJOR.MINOR.PATCH with MAJOR >= 1; the version is the tag
# without the "v", followed by the distance and commit for builds between
# tags (e.g. 1.9.0-14-g1a2b3c4).
# Other tags (nets, latest, and the old engine's v0.0.1) are skipped.
find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/.git")
  execute_process(
    COMMAND "${GIT_EXECUTABLE}" describe --tags --always --dirty --match "v[1-9]*"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    OUTPUT_VARIABLE SP_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET)
  string(REGEX REPLACE "^v" "" SP_VERSION "${SP_VERSION}")
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
  src/datagen.cpp
  src/eval.cpp
  src/misc.cpp
  src/movegen.cpp
  src/movepick.cpp
  src/nnue.cpp
  src/perft.cpp
  src/position.cpp
  src/search.cpp
  src/thread.cpp
  src/timeman.cpp
  src/tt.cpp
  src/uci.cpp
  src/ucioption.cpp
  ${CMAKE_BINARY_DIR}/generated/embedded_net.cpp)

# The embedded network is turned into a byte array by a step that runs on
# every build, so dropping a file into nets/ (or removing it) takes effect on
# the next `cmake --build` without reconfiguring. With no network file the
# array is empty and the engine uses its hand-crafted evaluation.
if(NOT SP_EMBED_NET)
  set(SP_EMBED_NET "")
endif()

# The released network: nets/NETWORK names the file and its SHA-256. It is
# downloaded from the "nets" GitHub release when not present. A local
# nets/default.nnue (a candidate under test) takes precedence over it.
set(SP_RELEASED_NET "")
if(SP_EMBED_NET STREQUAL "${CMAKE_SOURCE_DIR}/nets/default.nnue" AND EXISTS "${CMAKE_SOURCE_DIR}/nets/NETWORK")
  file(STRINGS "${CMAKE_SOURCE_DIR}/nets/NETWORK" SP_NET_LINES REGEX "^[^#]")
  list(GET SP_NET_LINES 0 SP_NET_LINE)
  separate_arguments(SP_NET_FIELDS UNIX_COMMAND "${SP_NET_LINE}")
  list(GET SP_NET_FIELDS 0 SP_NET_NAME)
  list(GET SP_NET_FIELDS 1 SP_NET_SHA256)
  set(SP_RELEASED_NET "${CMAKE_SOURCE_DIR}/nets/${SP_NET_NAME}")
  if(NOT EXISTS "${SP_RELEASED_NET}")
    set(SP_NET_URL "https://github.com/johnwbyrd/superpawn/releases/download/nets/${SP_NET_NAME}")
    message(STATUS "Downloading network ${SP_NET_NAME}")
    file(DOWNLOAD "${SP_NET_URL}" "${SP_RELEASED_NET}.part" STATUS SP_DL_STATUS TLS_VERIFY ON)
    list(GET SP_DL_STATUS 0 SP_DL_CODE)
    if(SP_DL_CODE EQUAL 0)
      file(SHA256 "${SP_RELEASED_NET}.part" SP_DL_SHA256)
      if(SP_DL_SHA256 STREQUAL SP_NET_SHA256)
        file(RENAME "${SP_RELEASED_NET}.part" "${SP_RELEASED_NET}")
      else()
        message(WARNING "Downloaded ${SP_NET_NAME} has SHA-256 ${SP_DL_SHA256}, expected ${SP_NET_SHA256}; not using it")
        file(REMOVE "${SP_RELEASED_NET}.part")
        set(SP_RELEASED_NET "")
      endif()
    else()
      list(GET SP_DL_STATUS 1 SP_DL_MSG)
      message(WARNING "Could not download ${SP_NET_NAME} (${SP_DL_MSG}); building without a network unless nets/default.nnue exists")
      file(REMOVE "${SP_RELEASED_NET}.part")
      set(SP_RELEASED_NET "")
    endif()
  else()
    file(SHA256 "${SP_RELEASED_NET}" SP_HAVE_SHA256)
    if(NOT SP_HAVE_SHA256 STREQUAL SP_NET_SHA256)
      message(WARNING "${SP_RELEASED_NET} does not match the SHA-256 in nets/NETWORK; not using it")
      set(SP_RELEASED_NET "")
    endif()
  endif()
endif()

set(SP_EMBED_NET_CPP ${CMAKE_BINARY_DIR}/generated/embedded_net.cpp)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/generated)
set(SP_EMBED_ARGS -DINPUT=${SP_EMBED_NET} -DFALLBACK=${SP_RELEASED_NET} -DOUTPUT=${SP_EMBED_NET_CPP})
# Generate once at configure time so the source exists for the first build
execute_process(COMMAND ${CMAKE_COMMAND} ${SP_EMBED_ARGS} -P ${CMAKE_SOURCE_DIR}/cmake/embed_net.cmake)
add_custom_target(superpawn_embed_net ALL
  COMMAND ${CMAKE_COMMAND} ${SP_EMBED_ARGS}
          -P ${CMAKE_SOURCE_DIR}/cmake/embed_net.cmake
  BYPRODUCTS ${SP_EMBED_NET_CPP}
  COMMENT "Checking for an NNUE network to embed"
  VERBATIM)
set_source_files_properties(${SP_EMBED_NET_CPP} PROPERTIES GENERATED TRUE)
add_dependencies(superpawn_core superpawn_embed_net)

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
  # C4324: structure padded due to alignas, which is intentional for the SIMD types
  target_compile_options(superpawn_core PUBLIC /W4 /permissive- /Zc:preprocessor /EHsc /wd4324)
  if(SP_WERROR)
    target_compile_options(superpawn_core PUBLIC /WX)
  endif()
else()
  target_compile_options(superpawn_core PUBLIC -Wall -Wextra -Wpedantic -Wshadow -Wno-unused-parameter)
  if(SP_WERROR)
    target_compile_options(superpawn_core PUBLIC -Werror)
  endif()
endif()

# Static linking for distributed binaries: libstdc++ and glibc on Linux. The
# Windows static runtime is selected above, before the targets exist. macOS
# has no static libc. Android links libc++ statically by the NDK's default and
# bionic is on every device, so -static is neither needed nor wanted there.
if(SP_STATIC AND NOT MSVC AND NOT APPLE AND NOT ANDROID)
  target_link_options(superpawn_core PUBLIC -static)
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
    tests/test_datagen.cpp
    tests/test_eval.cpp
    tests/test_nnue.cpp
    tests/test_position.cpp
    tests/test_perft.cpp
    tests/test_search.cpp
    tests/test_tt.cpp)

  target_include_directories(superpawn_tests PRIVATE tests)
  target_link_libraries(superpawn_tests PRIVATE superpawn_core)
  if(SP_LTO AND SP_IPO_OK)
    # The core objects are LTO bitcode, so every consumer must link with LTO too
    set_property(TARGET superpawn_tests PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
  endif()

  add_test(NAME unit COMMAND superpawn_tests)
  add_test(NAME bench COMMAND superpawn bench 16 1 10)
endif()
