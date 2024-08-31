# This is a reference AArch64 cross compile script
# Pass in to cmake when building:
# eg: cmake -DCMAKE_TOOLCHAIN_FILE=../CMakeToolchains/AArch64.cmake ..
if (NOT DEFINED ENV{SYSROOT})
  message(FATAL_ERROR "Need to have SYSROOT environment variable set")
endif()

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)
set(CMAKE_CROSSCOMPILING TRUE)

# Target triple needs to match the binutils exactly
set(TARGET_TRIPLE aarch64-linux-gnu)
set(CMAKE_C_COMPILER "clang")
set(CMAKE_CXX_COMPILER "clang++")
set(CMAKE_C_COMPILER_AR "llvm-ar")
set(CMAKE_CXX_COMPILER_AR "llvm-ar")
set(CMAKE_C_COMPILER_RANLIB "llvm-ranlib")
set(CMAKE_CXX_COMPILER_RANLIB "llvm-ranlib")
set(CMAKE_LINKER "ld.lld")

set(CMAKE_C_COMPILER_TARGET ${TARGET_TRIPLE})
set(CMAKE_CXX_COMPILER_TARGET ${TARGET_TRIPLE})

# Set the environment variable SYSROOT to the aarch64 rootfs
set(CMAKE_FIND_ROOT_PATH "$ENV{SYSROOT}")
set(CMAKE_SYSROOT "$ENV{SYSROOT}")

list(APPEND CMAKE_PREFIX_PATH "$ENV{SYSROOT}/usr/lib/${TARGET_TRIPLE}/cmake/")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
