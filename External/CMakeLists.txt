# This subproject fetches all externals as needed

set(BUILD_SHARED_LIBS OFF)

if (BUILD_TESTING OR ENABLE_VIXL_DISASSEMBLER OR ENABLE_VIXL_SIMULATOR)
  if (USE_CPM)
    FEXAddPackage(
      NAME vixl
      REPO FEX-Emu/vixl
      COMMIT ed690c9eca)
  else()
    add_subdirectory(vixl/)
    include_directories(SYSTEM vixl/src/)
  endif()

  if (NOT TARGET vixl::vixl)
    add_library(vixl::vixl ALIAS vixl)
  endif()
endif()

if (ENABLE_FEXCORE_PROFILER AND FEXCORE_PROFILER_BACKEND STREQUAL "TRACY")
  if (USE_CPM)
    FEXAddPackage(
      NAME tracy
      REPO wolfpld/tracy
      TAG v0.12.1)
  else()
    add_subdirectory(tracy)
  endif()
endif()

set(XXHASH_BUNDLED_MODE TRUE)
set(XXHASH_BUILD_XXHSUM FALSE)

if (USE_CPM)
  FEXAddPackage(
    NAME xxhash
    REPO Cyan4973/xxHash
    TAG v0.8.3
    SOURCE_SUBDIR cmake_unofficial)

  FEXAddPackage(
    NAME fmt
    REPO fmtlib/fmt
    TAG 12.1.0)

  # TODO: is the system package viable?
  FEXAddPackage(
    NAME robin-map
    REPO crueter/robin-map
    COMMIT 1b6c00afe1)

  FEXAddPackage(
    NAME range-v3
    REPO ericniebler/range-v3
    COMMIT ca1388fb9d)
else()
  find_package(xxhash MODULE QUIET GLOBAL)
  if (NOT TARGET xxhash::xxhash)
    add_subdirectory(xxhash/cmake_unofficial/)
  endif()

  find_package(fmt QUIET GLOBAL)
  if (NOT fmt_FOUND)
    # Disable fmt install
    set(FMT_INSTALL OFF)
    add_subdirectory(fmt/)
  endif()

  add_subdirectory(robin-map)

  find_package(range-v3 QUIET GLOBAL)
  if (NOT range-v3_FOUND)
    add_subdirectory(range-v3/)
    target_compile_definitions(range-v3 INTERFACE RANGES_DISABLE_DEPRECATED_WARNINGS)
  endif()
endif()

if (ENABLE_JEMALLOC_GLIBC_ALLOC)
  # The glibc jemalloc subproject which hooks the glibc allocator.
  # Required for thunks to work.
  # All host native libraries will use this allocator, while *most*
  # other FEX internal allocations will use the other jemalloc allocator.
  if (USE_CPM)
    FEXAddPackage(
      NAME jemalloc_glibc
      REPO FEX-Emu/jemalloc
      COMMIT 8436195ad5
    )
  else()
    add_subdirectory(jemalloc_glibc/)
  endif()
elseif (NOT MINGW)
  message (STATUS
    " jemalloc glibc allocator disabled!\n"
    " This is not a recommended configuration!\n"
    " This will very explicitly break thunk execution!\n"
    " Use at your own risk!")
endif()

if (ENABLE_JEMALLOC)
  # The jemalloc subproject that all FEXCore fextl objects allocate through.
    if (USE_CPM)
    FEXAddPackage(
      NAME jemalloc
      REPO FEX-Emu/jemalloc
      COMMIT ce24593018
    )
  else()
    add_subdirectory(jemalloc)
  endif()
elseif (NOT MINGW)
  message (STATUS
    " jemalloc disabled!\n"
    " This is not a recommended configuration!\n"
    " This will very explicitly break 32-bit application execution!\n"
    " Use at your own risk!")
endif()

add_library(jemalloc::jemalloc ALIAS FEX_jemalloc)

if (BUILD_TESTING)
  if (USE_CPM)
    FEXAddPackage(
      NAME Catch2
      REPO catchorg/Catch2
      TAG v3.11.0)

    if (NOT Catch2_ADDED)
      set(Catch2_FOUND ON)
    endif()
  else()
    find_package(Catch2 3 QUIET GLOBAL)
    if (NOT Catch2_FOUND)
      add_subdirectory(Catch2/)
    endif()
  endif()

  if (Catch2_FOUND)
    list(APPEND CMAKE_MODULE_PATH "${Catch2_DIR}")
  else()
    # Pull in catch_discover_tests definition
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Catch2/contrib/")
  endif()
  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} PARENT_SCOPE)
else ()
  # Override any previously generated test list to avoid running stale test binaries
  file(GENERATE OUTPUT CTestTestfile.cmake CONTENT "# No tests since BUILD_TESTING is disabled")
endif()

if (BUILD_THUNKS)
  if (USE_CPM)
    FEXAddPackage(
      NAME Vulkan-Headers
      REPO KhronosGroup/Vulkan-Headers
      TAG v1.4.310)

    set(vulkan_dir ${Vulkan-Headers_SOURCE_DIR}/include)
  else()
    add_subdirectory(Vulkan-Headers)
    if (NOT TARGET Vulkan::Headers)
      add_library(Vulkan::Headers ALIAS Vulkan-Headers)
    endif()

    set(vulkan_dir Vulkan-Headers/include)
  endif()
endif()

set(VULKAN_HEADERS_DIR ${vulkan_dir} PARENT_SCOPE)

add_subdirectory(tiny-json/)

add_subdirectory(SoftFloat-3e/)
add_subdirectory(cephes/)
