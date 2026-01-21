# SPDX-License-Identifier: MIT

# vixl
if (BUILD_TESTING OR ENABLE_VIXL_DISASSEMBLER OR ENABLE_VIXL_SIMULATOR)
    FEXAddPackage(
        NAME vixl
        REPO FEX-Emu/vixl
        COMMIT da1ad9fb02
        BUNDLED ON)
endif()

# tracy
if (ENABLE_FEXCORE_PROFILER AND FEXCORE_PROFILER_BACKEND STREQUAL "TRACY")
  FEXAddPackage(
      NAME tracy
      REPO wolfpld/tracy
      TAG v0.12.1)
endif()

# xxhash
FEXAddPackage(
    NAME xxhash
    REPO Cyan4973/xxHash
    TAG v0.8.3
    SOURCE_SUBDIR cmake_unofficial
    BUNDLED ${CMAKE_CROSSCOMPILING})

# Catch2
if (BUILD_TESTING)
    FEXAddPackage(
        NAME Catch2
        REPO catchorg/Catch2
        VERSION 3
        TAG v3.11.0)

    if (Catch2_ADDED)
        list(APPEND CMAKE_MODULE_PATH "${Catch2_SOURCE_DIR}/extras/")
    endif()
endif()

# jemalloc
if (ENABLE_JEMALLOC_GLIBC_ALLOC)
    # The glibc jemalloc subproject which hooks the glibc allocator.
    # Required for thunks to work.
    # All host native libraries will use this allocator, while *most* other FEX internal allocations will use the other jemalloc allocator.
    FEXAddPackage(
        NAME jemalloc_glibc
        REPO FEX-Emu/jemalloc
        COMMIT 8436195ad5
        BUNDLED ON)
elseif (NOT MINGW)
    message(STATUS
        " jemalloc glibc allocator disabled!\n"
        " This is not a recommended configuration!\n"
        " This will very explicitly break thunk execution!\n"
        " Use at your own risk!")
endif()

if (ENABLE_JEMALLOC)
    # The jemalloc subproject that all FEXCore fextl objects allocate through.
    FEXAddPackage(
        NAME jemalloc
        REPO FEX-Emu/jemalloc
        COMMIT 97d986993d
        BUNDLED ON)
elseif (NOT MINGW)
    message(STATUS
        " jemalloc disabled!\n"
        " This is not a recommended configuration!\n"
        " This will very explicitly break 32-bit application execution!\n"
        " Use at your own risk!")
endif()

# zydis
if (ENABLE_ZYDIS)
    FEXAddPackage(
        NAME Zycore
        REPO zyantific/zycore-c
        VERSION 1.5
        COMMIT 75a36c45ae)

    set(ZYDIS_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
    set(ZYDIS_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(ZYAN_SYSTEM_ZYCORE ON)

    FEXAddPackage(
        NAME Zydis
        REPO zyantific/zydis
        VERSION 4.0
        COMMIT 9bfadd6a55)
endif()

# unordered_dense
FEXAddPackage(
    NAME unordered_dense
    REPO martinus/unordered_dense
    VERSION 4
    TAG v4.8.1)

# fmt
FEXAddPackage(
    NAME fmt
    REPO fmtlib/fmt
    TAG 12.1.0
    VERSION 8)

# range-v3
FEXAddPackage(
    NAME range-v3
    REPO ericniebler/range-v3
    COMMIT ca1388fb9d)

if (range-v3_ADDED)
    target_compile_definitions(range-v3 INTERFACE RANGES_DISABLE_DEPRECATED_WARNINGS)
endif()

# Vulkan Headers
FEXAddPackage(
    NAME VulkanHeaders
    REPO KhronosGroup/Vulkan-Headers
    TAG v1.4.337
    BUNDLED ON)

Propagate(VulkanHeaders_SOURCE_DIR)

# propagate any modules to root cmakelists
Propagate(CMAKE_MODULE_PATH)
