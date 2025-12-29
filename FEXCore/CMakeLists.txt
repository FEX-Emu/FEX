cmake_minimum_required(VERSION 3.14)
set(PROJECT_NAME FEXCore)
project(${PROJECT_NAME}
  VERSION 0.01
  LANGUAGES CXX)

if (CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
  set(ARCHITECTURE_x86_64 1)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcx16")
endif()

if (CMAKE_SYSTEM_PROCESSOR MATCHES "^aarch64|^arm64|^armv8\.*")
  set(ARCHITECTURE_arm64 1)
endif()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
cmake_policy(SET CMP0083 NEW) # Follow new PIE policy
include(CheckPIESupported)
check_pie_supported()

set(CMAKE_INCLUDE_CURRENT_DIR ON)

include(CheckCXXCompilerFlag)
include(CheckIncludeFileCXX)
include(CheckCXXSourceCompiles)

if (EXISTS ${CMAKE_CURRENT_DIR}/External/vixl/)
  # Useful to have for freestanding libFEXCore
  add_subdirectory(External/vixl/)
  include_directories(External/vixl/src/)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/git_version.h.in
  ${CMAKE_BINARY_DIR}/generated/git_version.h)

include_directories(${CMAKE_BINARY_DIR}/generated)

# Disable strict aliasing for all build modes
# See discussion in https://github.com/FEX-Emu/FEX/pull/4494#issuecomment-2800608944
# for background context.
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-fno-strict-aliasing> $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>)

add_subdirectory(Source/)

if (NOT BUILD_STEAM_SUPPORT)
  install (DIRECTORY include/FEXCore ${CMAKE_BINARY_DIR}/include/FEXCore
    DESTINATION include
    COMPONENT Development)
endif()

if (BUILD_TESTING)
  add_subdirectory(unittests/)
endif()
