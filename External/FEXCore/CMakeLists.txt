cmake_minimum_required(VERSION 3.10)
set (PROJECT_NAME FEXCore)
project(${PROJECT_NAME}
  VERSION 0.01
  LANGUAGES CXX)

option(ENABLE_CLANG_FORMAT "Run clang format over the source" FALSE)
option(FORCE_AARCH64 "Force AArch64 Target for testing" FALSE)
option(ENABLE_JITSYMBOLS "Enable visibility of JITSymbols in profiling tools" FALSE)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(LLVM CONFIG QUIET)
if(LLVM_FOUND AND TARGET LLVM)
  message(STATUS "LLVM found!")
  include_directories(${LLVM_INCLUDE_DIRS})
else()
  message("Couldn't find LLVM and this project requires it")
endif()

include(CheckCXXCompilerFlag)
include(CheckIncludeFileCXX)

if (CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
  set(_M_X86_64 1)
  if (NOT FORCE_AARCH64)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-operator-names")
    set(CMAKE_REQUIRED_DEFINITIONS "-fno-operator-names")
    check_include_file_cxx(xbyak/xbyak.h XBYAK_FOUND)
    if (XBYAK_FOUND)
      message(STATUS "Enabling x86-64 JIT")
      set(ENABLE_JIT 1)
    else()
      message(STATUS "xbyak not found. Not enabling runtime JIT")
    endif()
  endif()
endif()

if (FORCE_AARCH64 OR CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
  message(STATUS "Enabling AArch64 JIT")
  set(_M_ARM_64 1)
  set(ENABLE_JIT 1)
  add_subdirectory(External/vixl/)
  include_directories(External/vixl/src/)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(External/SonicUtils/include/SonicUtils)

add_subdirectory(Source/)
add_subdirectory(Examples/)

install(TARGETS ${PROJECT_NAME} ${PROJECT_NAME}_shared
  LIBRARY
    DESTINATION lib
    COMPONENT Libraries
  ARCHIVE
    DESTINATION lib
    COMPONENT Libraries)

install (DIRECTORY include/FEXCore ${CMAKE_BINARY_DIR}/include/FEXCore
  DESTINATION include
  COMPONENT Development)
