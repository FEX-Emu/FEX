cmake_minimum_required(VERSION 3.14)
set (PROJECT_NAME FEXCore)
project(${PROJECT_NAME}
  VERSION 0.01
  LANGUAGES CXX)

option(ENABLE_CLANG_FORMAT "Run clang format over the source" FALSE)
option(ENABLE_JITSYMBOLS "Enable visibility of JITSymbols in profiling tools" FALSE)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
cmake_policy(SET CMP0083 NEW) # Follow new PIE policy
include(CheckPIESupported)
check_pie_supported()

set(CMAKE_INCLUDE_CURRENT_DIR ON)

include(CheckCXXCompilerFlag)
include(CheckIncludeFileCXX)

if (CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
  set(_M_X86_64 1)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-operator-names -mcx16")
  set(CMAKE_REQUIRED_DEFINITIONS "-fno-operator-names")
  message(STATUS "Enabling x86-64 JIT")
  set(ENABLE_JIT 1)
endif()

if (CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
  message(STATUS "Enabling AArch64 JIT")
  set(_M_ARM_64 1)
  set(ENABLE_JIT 1)
  if (EXISTS ${CMAKE_CURRENT_DIR}/External/vixl/)
    # Useful to have for freestanding libFEXCore
    add_subdirectory(External/vixl/)
    include_directories(External/vixl/src/)
  endif()
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(${CMAKE_SOURCE_DIR}/External/SonicUtils/include/SonicUtils)

# Find our git hash
find_package(Git)

set(GIT_SHORT_HASH "Unknown")

if (GIT_FOUND)
  execute_process(
    COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    OUTPUT_VARIABLE GIT_SHORT_HASH
    ERROR_QUIET
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
endif()

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/include/git_version.h.in
  ${CMAKE_BINARY_DIR}/generated/git_version.h)

include_directories(${CMAKE_BINARY_DIR}/generated)

add_subdirectory(Source/)

install (DIRECTORY include/FEXCore ${CMAKE_BINARY_DIR}/include/FEXCore
  DESTINATION include
  COMPONENT Development)
