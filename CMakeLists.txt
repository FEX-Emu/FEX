cmake_minimum_required(VERSION 3.14)
project(FEX)

option(BUILD_TESTS "Build unit tests to ensure sanity" TRUE)
option(ENABLE_CLANG_FORMAT "Run clang format over the source" FALSE)
option(ENABLE_IWYU "Enables include what you use program" FALSE)
option(ENABLE_LTO "Enable LTO with compilation" TRUE)
option(ENABLE_XRAY "Enable building with LLVM X-Ray" FALSE)
option(ENABLE_LLD "Enable linking with LLD" FALSE)
option(ENABLE_ASAN "Enables Clang ASAN" FALSE)
option(ENABLE_TSAN "Enables Clang TSAN" FALSE)
option(ENABLE_ASSERTIONS "Enables assertions in build" FALSE)
option(ENABLE_VISUAL_DEBUGGER "Enables the visual debugger for compiling" FALSE)

set (X86_C_COMPILER "x86_64-linux-gnu-gcc" CACHE STRING "c compiler for compiling x86 guest libs")
set (X86_CXX_COMPILER "x86_64-linux-gnu-g++" CACHE STRING "c++ compiler for compiling x86 guest libs")
set (DATA_DIRECTORY "${CMAKE_INSTALL_PREFIX}/share/fex-emu" CACHE PATH "global data directory")

string(TOUPPER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE)
if (CMAKE_BUILD_TYPE MATCHES "DEBUG")
  set(ENABLE_ASSERTIONS TRUE)
endif()

if (ENABLE_ASSERTIONS)
  message(STATUS "Assertions enabled")
  add_definitions(-DASSERTIONS_ENABLED=1)
endif()

if (ENABLE_IWYU)
  find_program(IWYU_EXE "iwyu")
  if (IWYU_EXE)
    message(STATUS "IWYU enabled")
    set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE "${IWYU_EXE}")
  endif()
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Bin)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
cmake_policy(SET CMP0083 NEW) # Follow new PIE policy
include(CheckPIESupported)
check_pie_supported()

if (ENABLE_LTO)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
else()
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION FALSE)
endif()

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
  message(STATUS "CCache enabled")
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
endif()

if (ENABLE_XRAY)
  add_compile_options(-fxray-instrument)
  link_libraries(-fxray-instrument)
endif()

if (ENABLE_LLD)
  link_libraries(-fuse-ld=lld)
endif()

if (ENABLE_ASAN)
  add_definitions(-DENABLE_ASAN=1)
  add_compile_options(-fno-omit-frame-pointer -fsanitize=address)
  link_libraries(-fno-omit-frame-pointer -fsanitize=address)
endif()

if (ENABLE_TSAN)
  add_compile_options(-fno-omit-frame-pointer -fsanitize=thread)
  link_libraries(-fno-omit-frame-pointer -fsanitize=thread)
endif()

set (CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -fno-omit-frame-pointer")
set (CMAKE_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_LINKER_FLAGS_RELWITHDEBINFO} -fno-omit-frame-pointer")

set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fomit-frame-pointer")
set (CMAKE_LINKER_FLAGS_RELEASE "${CMAKE_LINKER_FLAGS_RELEASE} -fomit-frame-pointer")

if (CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
  set(_M_X86_64 1)
  add_definitions(-D_M_X86_64=1)
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcx16")
endif()

if (CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
  set(_M_ARM_64 1)
  add_definitions(-D_M_ARM_64=1)
  add_subdirectory(External/vixl/)
  include_directories(External/vixl/src/)
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  # This means we were attempted to get compiled with GCC
  message(FATAL_ERROR "FEX doesn't support getting compiled with GCC!")
endif()

add_definitions(-Wno-trigraphs)
add_definitions(-DGLOBAL_DATA_DIRECTORY="${DATA_DIRECTORY}/")

add_subdirectory(External/cpp-optparse/)
include_directories(External/cpp-optparse/)

add_subdirectory(External/imgui/)
include_directories(External/imgui/)

add_subdirectory(External/json-maker/)
include_directories(External/json-maker/)

add_subdirectory(External/tiny-json/)
include_directories(External/tiny-json/)

include_directories(External/xbyak/)

include_directories(Source/)
include_directories("${CMAKE_BINARY_DIR}/Source/")

include(CheckCXXCompilerFlag)

# Add in diagnostic colours if the option is available.
# Ninja code generator will kill colours if this isn't here
check_cxx_compiler_flag(-fdiagnostics-color=always GCC_COLOR)
check_cxx_compiler_flag(-fcolor-diagnostics CLANG_COLOR)
check_cxx_compiler_flag(-Wno-deprecated-enum-enum-conversion ENUM_ENUM_WARNING)

if (GCC_COLOR)
  add_compile_options(-fdiagnostics-color=always)
endif()
if (CLANG_COLOR)
  add_compile_options(-fcolor-diagnostics)
endif()

if(ENUM_ENUM_WARNING)
  add_compile_options(-Wno-deprecated-enum-enum-conversion)
endif()

check_cxx_compiler_flag("-march=native" COMPILER_SUPPORTS_MARCH_NATIVE)
if(COMPILER_SUPPORTS_MARCH_NATIVE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
endif()

add_compile_options(-Wall)

add_subdirectory(External/FEXCore)

add_subdirectory(Source/)

if (BUILD_TESTS)
  enable_testing()
  message(STATUS "Unit tests are enabled")
  add_subdirectory(unittests/)
endif()

include(ExternalProject)

ExternalProject_Add(host-libs
	PREFIX host-libs
	SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ThunkLibs/HostLibs"
  BINARY_DIR "Host"
	INSTALL_COMMAND ""
  BUILD_ALWAYS ON
)

ExternalProject_Add(guest-libs
	PREFIX guest-libs
	SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ThunkLibs/GuestLibs"
  BINARY_DIR "Guest"
  CMAKE_ARGS "-DX86_C_COMPILER:STRING=${X86_C_COMPILER}" "-DX86_CXX_COMPILER:STRING=${X86_CXX_COMPILER}"
	INSTALL_COMMAND ""
  BUILD_ALWAYS ON
)
