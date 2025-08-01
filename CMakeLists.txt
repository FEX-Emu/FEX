cmake_minimum_required(VERSION 3.14)
project(FEX C CXX ASM)

INCLUDE (CheckIncludeFiles)
CHECK_INCLUDE_FILES ("gdb/jit-reader.h" HAVE_GDB_JIT_READER_H)

option(BUILD_TESTS "Build unit tests to ensure sanity" TRUE)
option(BUILD_FEX_LINUX_TESTS "Build FEXLinuxTests, requires x86 compiler" FALSE)
option(BUILD_THUNKS "Build thunks" FALSE)
option(BUILD_FEXCONFIG "Build FEXConfig" TRUE)
option(ENABLE_CLANG_THUNKS "Build thunks with clang" TRUE)
option(ENABLE_IWYU "Enables include what you use program" FALSE)
option(ENABLE_LTO "Enable LTO with compilation" TRUE)
option(ENABLE_XRAY "Enable building with LLVM X-Ray" FALSE)
set(USE_LINKER "" CACHE STRING "Allow overriding the linker path directly")
option(ENABLE_UBSAN "Enables Clang UBSAN" FALSE)
option(ENABLE_ASAN "Enables Clang ASAN" FALSE)
option(ENABLE_TSAN "Enables Clang TSAN" FALSE)
option(ENABLE_COVERAGE "Enables Coverage" FALSE)
option(ENABLE_ASSERTIONS "Enables assertions in build" FALSE)
option(ENABLE_GDB_SYMBOLS "Enables GDBSymbols integration support" ${HAVE_GDB_JIT_READER_H})
option(ENABLE_STRICT_WERROR "Enables stricter -Werror for CI" FALSE)
option(ENABLE_WERROR "Enables -Werror" FALSE)
option(ENABLE_JEMALLOC "Enables jemalloc allocator" TRUE)
option(ENABLE_JEMALLOC_GLIBC_ALLOC "Enables jemalloc glibc allocator" TRUE)
option(ENABLE_OFFLINE_TELEMETRY "Enables FEX offline telemetry" TRUE)
option(ENABLE_COMPILE_TIME_TRACE "Enables time trace compile option" FALSE)
option(ENABLE_LIBCXX "Enables LLVM libc++" FALSE)
option(ENABLE_CCACHE "Enables ccache for compile caching" TRUE)
option(ENABLE_VIXL_SIMULATOR "Enable use of VIXL simulator for emulation (only useful for CI testing)" FALSE)
option(ENABLE_VIXL_DISASSEMBLER "Enables debug disassembler output with VIXL" FALSE)
option(USE_LEGACY_BINFMTMISC "Uses legacy method of setting up binfmt_misc" FALSE)
option(ENABLE_FEXCORE_PROFILER "Enables use of the FEXCore timeline profiling capabilities" FALSE)
set (FEXCORE_PROFILER_BACKEND "gpuvis" CACHE STRING "Set which backend to use for the FEXCore profiler (gpuvis, tracy)")
option(ENABLE_GLIBC_ALLOCATOR_HOOK_FAULT "Enables glibc memory allocation hooking with fault for CI testing")
option(USE_PDB_DEBUGINFO "Builds debug info in PDB format" FALSE)

set (X86_32_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/Data/CMake/toolchain_x86_32.cmake" CACHE FILEPATH "Toolchain file for the (cross-)compiler targeting i686")
set (X86_64_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/Data/CMake/toolchain_x86_64.cmake" CACHE FILEPATH "Toolchain file for the (cross-)compiler targeting x86_64")
set (X86_DEV_ROOTFS "/" CACHE FILEPATH "Path to the sysroot used for cross-compiling for i686 and x86_64")
set (DATA_DIRECTORY "" CACHE PATH "Global data directory (override)")
set (HOSTLIBS_DATA_DIRECTORY "" CACHE PATH "Global data directory (override)")
if (NOT DATA_DIRECTORY)
  set (DATA_DIRECTORY "${CMAKE_INSTALL_PREFIX}/share/fex-emu")
endif()

include(GNUInstallDirs)
if (NOT HOSTLIBS_DATA_DIRECTORY)
  set(HOSTLIBS_DATA_DIRECTORY "${CMAKE_INSTALL_FULL_LIBDIR}/fex-emu")
endif()

string(FIND ${CMAKE_BASE_NAME} mingw CONTAINS_MINGW)
if (NOT CONTAINS_MINGW EQUAL -1)
  message (STATUS "Mingw build")
  set (MINGW_BUILD TRUE)
  set (ENABLE_JEMALLOC TRUE)
  set (ENABLE_JEMALLOC_GLIBC_ALLOC FALSE)
endif()

if (NOT MINGW_BUILD)
  message (STATUS "Clang version ${CMAKE_CXX_COMPILER_VERSION}")
  set (CLANG_MINIMUM_VERSION 13.0)
  if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS ${CLANG_MINIMUM_VERSION})
    message (FATAL_ERROR "Clang version too old for FEX. Need at least ${CLANG_MINIMUM_VERSION} but has ${CMAKE_CXX_COMPILER_VERSION}")
  endif()
endif()

if (ENABLE_FEXCORE_PROFILER)
  add_definitions(-DENABLE_FEXCORE_PROFILER=1)
  string(TOUPPER "${FEXCORE_PROFILER_BACKEND}" FEXCORE_PROFILER_BACKEND)

  if (FEXCORE_PROFILER_BACKEND STREQUAL "GPUVIS")
    add_definitions(-DFEXCORE_PROFILER_BACKEND=1)
  elseif (FEXCORE_PROFILER_BACKEND STREQUAL "TRACY")
    add_definitions(-DFEXCORE_PROFILER_BACKEND=2)
    add_definitions(-DTRACY_ENABLE=1)
    # Required so that Tracy will only start in the selected guest application
    add_definitions(-DTRACY_MANUAL_LIFETIME=1)
    add_definitions(-DTRACY_DELAYED_INIT=1)
    # This interferes with FEX's signal handling
    add_definitions(-DTRACY_NO_CRASH_HANDLER=1)
    # Tracy can gather call stack samples in regular intervals, but this
    # isn't useful for us since it would usually sample opaque JIT code
    add_definitions(-DTRACY_NO_SAMPLING=1)
    # This pulls in libbacktrace which allocators in global constructors (before FEX can set up its allocator hooks)
    add_definitions(-DTRACY_NO_CALLSTACK=1)
    if (MINGW_BUILD)
      message(FATAL_ERROR "Tracy profiler not supported")
    endif()
  else()
    message(FATAL_ERROR "Unknown FEXCore profiler backend ${FEXCORE_PROFILER_BACKEND}")
  endif()
endif()

if (ENABLE_JEMALLOC_GLIBC_ALLOC AND ENABLE_GLIBC_ALLOCATOR_HOOK_FAULT)
  message(FATAL_ERROR "Can't have both glibc fault allocator and jemalloc glibc allocator enabled at the same time")
endif()

if (ENABLE_GLIBC_ALLOCATOR_HOOK_FAULT)
  add_definitions(-DGLIBC_ALLOCATOR_FAULT=1)
endif()

# uninstall target
if(NOT TARGET uninstall)
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/Data/CMake/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

  add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/cmake_uninstall.cmake)
endif()

# These options are meant for package management
set (TUNE_CPU "native" CACHE STRING "Override the CPU the build is tuned for")
set (TUNE_ARCH "generic" CACHE STRING "Override the Arch the build is tuned for")
set (OVERRIDE_VERSION "detect" CACHE STRING "Override the FEX version in the format of <MMYY>{.<REV>}")

string(TOUPPER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE)
if (CMAKE_BUILD_TYPE MATCHES "DEBUG")
  set(ENABLE_ASSERTIONS TRUE)
endif()

if (ENABLE_ASSERTIONS)
  message(STATUS "Assertions enabled")
  add_definitions(-DASSERTIONS_ENABLED=1)
endif()

if (ENABLE_GDB_SYMBOLS)
  message(STATUS "GDBSymbols support enabled")
  add_definitions(-DGDB_SYMBOLS_ENABLED=1)
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

if (CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
  option(ENABLE_X86_HOST_DEBUG "Enables compiling on x86_64 host" FALSE)
  if (NOT ENABLE_X86_HOST_DEBUG)
    message(FATAL_ERROR
    " FEX-Emu doesn't support compiling for x86-64 hosts!"
    " This is /only/ a supported configuration for FEX CI and nothing else!")
  endif()
  set(_M_X86_64 1)
  add_definitions(-D_M_X86_64=1)
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcx16")
endif()

if (CMAKE_SYSTEM_PROCESSOR MATCHES "^aarch64|^arm64|^armv8\.*")
  set(_M_ARM_64 1)
  add_definitions(-D_M_ARM_64=1)
endif()

if (CMAKE_SYSTEM_PROCESSOR MATCHES "^arm64ec")
  set(_M_ARM_64EC 1)
  add_definitions(-D_M_ARM_64EC=1)
endif()

include(CheckCXXSourceCompiles)
set(CMAKE_REQUIRED_FLAGS "-std=c++11 -Wattributes -Werror=attributes")
check_cxx_source_compiles(
  "
  __attribute__((preserve_all))
  int Testy(int a, int b, int c, int d, int e, int f) {
  return a + b + c + d + e + f;
  }
  int main() {
  return Testy(0, 1, 2, 3, 4, 5);
  }"
  HAS_CLANG_PRESERVE_ALL)
unset(CMAKE_REQUIRED_FLAGS)
if (HAS_CLANG_PRESERVE_ALL)
  if (MINGW_BUILD)
    message(STATUS "Ignoring broken clang::preserve_all support")
    set(HAS_CLANG_PRESERVE_ALL FALSE)
  else()
    message(STATUS "Has clang::preserve_all")
  endif()
endif ()

if (_M_ARM_64 AND HAS_CLANG_PRESERVE_ALL)
  add_definitions("-DFEX_PRESERVE_ALL_ATTR=__attribute__((preserve_all))" "-DFEX_HAS_PRESERVE_ALL_ATTR=1")
else()
  add_definitions("-DFEX_PRESERVE_ALL_ATTR=" "-DFEX_HAS_PRESERVE_ALL_ATTR=0")
endif()

if (ENABLE_VIXL_SIMULATOR)
  # We can run the simulator on both x86-64 or AArch64 hosts
  add_definitions(-DVIXL_SIMULATOR=1 -DVIXL_INCLUDE_SIMULATOR_AARCH64=1)
endif()

if (ENABLE_CCACHE)
  find_program(CCACHE_PROGRAM ccache)
  if(CCACHE_PROGRAM)
    message(STATUS "CCache enabled")
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
  endif()
endif()

if (ENABLE_XRAY)
  add_compile_options(-fxray-instrument)
  link_libraries(-fxray-instrument)
endif()

if (ENABLE_COMPILE_TIME_TRACE)
  add_compile_options(-ftime-trace)
  link_libraries(-ftime-trace)
endif()

set (PTHREAD_LIB pthread)

if (USE_LINKER)
  message(STATUS "Overriding linker to: ${USE_LINKER}")
  add_link_options("-fuse-ld=${USE_LINKER}")
endif()

if (ENABLE_LIBCXX)
  message(WARNING "This is an unsupported configuration and should only be used for testing")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -stdlib=libc++")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++ -lc++abi")
endif()

if (NOT ENABLE_OFFLINE_TELEMETRY)
  # Disable FEX offline telemetry entirely if asked
  add_definitions(-DFEX_DISABLE_TELEMETRY=1)
endif()

if (ENABLE_UBSAN)
  # See https://github.com/FEX-Emu/FEX/pull/4494#issuecomment-2800608944
  # and related discussion for the use of -fno-sanitize=alignment -fno-sanitize=function
  # with UBSAN.
  # alignment: we don't follow a strict alignment policy, for example IR uses packed structs
  # that are regularly access unaligned.
  # function: syscalls cast function pointers to void (*)(unsigned long...), causing warnings
  # related to this access.
  add_definitions(-DENABLE_UBSAN=1)
  add_compile_options(-fno-omit-frame-pointer -fsanitize=undefined -fno-sanitize=alignment -fno-sanitize=function -fno-sanitize-recover=undefined)
  link_libraries(-fno-omit-frame-pointer -fsanitize=undefined -fno-sanitize=alignment -fno-sanitize=function -fno-sanitize-recover=undefined)
endif()

if (ENABLE_ASAN)
  add_definitions(-DENABLE_ASAN=1)
  add_compile_options(-fno-omit-frame-pointer -fsanitize=address -fsanitize-address-use-after-scope)
  link_libraries(-fno-omit-frame-pointer -fsanitize=address -fsanitize-address-use-after-scope)
endif()

if (ENABLE_TSAN)
  add_compile_options(-fno-omit-frame-pointer -fsanitize=thread)
  link_libraries(-fno-omit-frame-pointer -fsanitize=thread)
endif()

if (ENABLE_COVERAGE)
  add_compile_options(-fprofile-instr-generate -fcoverage-mapping)
  link_libraries(-fprofile-instr-generate -fcoverage-mapping)
endif()

if (ENABLE_JEMALLOC_GLIBC_ALLOC)
  # The glibc jemalloc subproject which hooks the glibc allocator.
  # Required for thunks to work.
  # All host native libraries will use this allocator, while *most* other FEX internal allocations will use the other jemalloc allocator.
  add_subdirectory(External/jemalloc_glibc/)
elseif (NOT MINGW_BUILD)
  message (STATUS
    " jemalloc glibc allocator disabled!\n"
    " This is not a recommended configuration!\n"
    " This will very explicitly break thunk execution!\n"
    " Use at your own risk!")
endif()

if (ENABLE_JEMALLOC)
  # The jemalloc subproject that all FEXCore fextl objects allocate through.
  add_subdirectory(External/jemalloc/)
elseif (NOT MINGW_BUILD)
  message (STATUS
    " jemalloc disabled!\n"
    " This is not a recommended configuration!\n"
    " This will very explicitly break 32-bit application execution!\n"
    " Use at your own risk!")
endif()

if (USE_PDB_DEBUGINFO)
  add_compile_options(-g -gcodeview)
  add_link_options(-g -Wl,--pdb=)
endif()

set (CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -fno-omit-frame-pointer")
set (CMAKE_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_LINKER_FLAGS_RELWITHDEBINFO} -fno-omit-frame-pointer")

set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fomit-frame-pointer")
set (CMAKE_LINKER_FLAGS_RELEASE "${CMAKE_LINKER_FLAGS_RELEASE} -fomit-frame-pointer")

include_directories(External/robin-map/include/)

if (BUILD_TESTS OR ENABLE_VIXL_DISASSEMBLER OR ENABLE_VIXL_SIMULATOR)
  add_subdirectory(External/vixl/)
  include_directories(SYSTEM External/vixl/src/)
endif()

if (ENABLE_FEXCORE_PROFILER AND FEXCORE_PROFILER_BACKEND STREQUAL "TRACY")
  add_subdirectory(External/tracy)
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  # This means we were attempted to get compiled with GCC
  message(FATAL_ERROR "FEX doesn't support getting compiled with GCC!")
endif()

find_package(PkgConfig REQUIRED)
find_package(Python 3.0 REQUIRED COMPONENTS Interpreter)

set(BUILD_SHARED_LIBS OFF)

pkg_search_module(xxhash IMPORTED_TARGET xxhash libxxhash)
if (TARGET PkgConfig::xxhash AND NOT CMAKE_CROSSCOMPILING)
  add_library(xxHash::xxhash ALIAS PkgConfig::xxhash)
else()
  set(XXHASH_BUNDLED_MODE TRUE)
  set(XXHASH_BUILD_XXHSUM FALSE)
  add_subdirectory(External/xxhash/cmake_unofficial/)
endif()

add_definitions(-Wno-trigraphs)
add_definitions(-DGLOBAL_DATA_DIRECTORY="${DATA_DIRECTORY}/")

if (BUILD_TESTS)
  find_package(Catch2 3 QUIET)
  if (NOT Catch2_FOUND)
    add_subdirectory(External/Catch2/)

    # Pull in catch_discover_tests definition
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/External/Catch2/contrib/")
  endif()

  include(Catch)
endif()

find_package(fmt QUIET)
if (NOT fmt_FOUND)
  # Disable fmt install
  set(FMT_INSTALL OFF)
  add_subdirectory(External/fmt/)
endif()

add_subdirectory(External/tiny-json/)
include_directories(External/tiny-json/)

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

if(ENABLE_WERROR OR ENABLE_STRICT_WERROR)
  add_compile_options(-Werror)
  if (NOT ENABLE_STRICT_WERROR)
    # Disable some Werror that can add frustration when developing
    add_compile_options(-Wno-error=unused-variable)
  endif()
endif()

set(FEX_TUNE_COMPILE_FLAGS)
if (NOT TUNE_ARCH STREQUAL "generic")
  check_cxx_compiler_flag("-march=${TUNE_ARCH}" COMPILER_SUPPORTS_ARCH_TYPE)
  if(COMPILER_SUPPORTS_ARCH_TYPE)
    list(APPEND FEX_TUNE_COMPILE_FLAGS "-march=${TUNE_ARCH}")
  else()
    message(FATAL_ERROR "Trying to compile arch type '${TUNE_ARCH}' but the compiler doesn't support this")
  endif()
endif()

if (TUNE_CPU STREQUAL "native")
  if(_M_ARM_64)
    if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 999999.0)
      # Clang 12.0 fixed the -mcpu=native bug with mixed big.little implementers
      # Clang can not currently check for native Apple M1 type in hypervisor. Currently disabled
      check_cxx_compiler_flag("-mcpu=native" COMPILER_SUPPORTS_CPU_TYPE)
      if(COMPILER_SUPPORTS_CPU_TYPE)
        list(APPEND FEX_TUNE_COMPILE_FLAGS "-mcpu=native")
      endif()
    else()
      execute_process(COMMAND python3 "${PROJECT_SOURCE_DIR}/Scripts/aarch64_fit_native.py" "/proc/cpuinfo" "${CMAKE_CXX_COMPILER_VERSION}"
        OUTPUT_VARIABLE AARCH64_CPU)

      string(STRIP ${AARCH64_CPU} AARCH64_CPU)

      execute_process(COMMAND python3 "${PROJECT_SOURCE_DIR}/Scripts/NeedDisabledSVE.py"
        RESULT_VARIABLE NEEDS_SVE_DISABLED)
      if (NEEDS_SVE_DISABLED)
        message(STATUS "Platform has bugged SVE. Disabling")
        set(AARCH64_CPU "cortex-a78")
      endif()

      check_cxx_compiler_flag("-mcpu=${AARCH64_CPU}" COMPILER_SUPPORTS_CPU_TYPE)
      if(COMPILER_SUPPORTS_CPU_TYPE)
        list(APPEND FEX_TUNE_COMPILE_FLAGS "-mcpu=${AARCH64_CPU}")
      endif()
    endif()
  else()
    check_cxx_compiler_flag("-march=native" COMPILER_SUPPORTS_MARCH_NATIVE)
    if(COMPILER_SUPPORTS_MARCH_NATIVE)
      list(APPEND FEX_TUNE_COMPILE_FLAGS "-march=native")
    endif()
  endif()
elseif (NOT TUNE_CPU STREQUAL "none")
  check_cxx_compiler_flag("-mcpu=${TUNE_CPU}" COMPILER_SUPPORTS_CPU_TYPE)
  if(COMPILER_SUPPORTS_CPU_TYPE)
    list(APPEND FEX_TUNE_COMPILE_FLAGS "-mcpu=${TUNE_CPU}")
  else()
    message(FATAL_ERROR "Trying to compile cpu type '${TUNE_CPU}' but the compiler doesn't support this")
  endif()
endif()

if (ENABLE_IWYU)
  find_program(IWYU_EXE "iwyu")
  if (IWYU_EXE)
    message(STATUS "IWYU enabled")
    set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE "${IWYU_EXE}")
  endif()
endif()

add_compile_options(-Wall)

include(CTest)
if (BUILD_TESTS)
  message(STATUS "Unit tests are enabled")
  if (NOT BUILD_TESTING)
    # CMake checks this variable before generating CTestTestfile.cmake
    message(SEND_ERROR "Unit tests require BUILD_TESTING to be enabled")
  endif()

  set (TEST_JOB_COUNT "" CACHE STRING "Override number of parallel jobs to use while running tests")
  if (TEST_JOB_COUNT)
    message(STATUS "Running tests with ${TEST_JOB_COUNT} jobs")
  elseif(CMAKE_VERSION VERSION_LESS "3.29")
    execute_process(COMMAND "nproc" OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE TEST_JOB_COUNT)
  endif()
  set(TEST_JOB_FLAG "-j${TEST_JOB_COUNT}")
endif()

add_subdirectory(External/SoftFloat-3e/)
add_subdirectory(External/cephes/)
add_subdirectory(FEXHeaderUtils/)
add_subdirectory(CodeEmitter/)
add_subdirectory(FEXCore/)

if (_M_ARM_64 AND NOT MINGW_BUILD)
  # Binfmt_misc files must be installed prior to Source/ installs
  add_subdirectory(Data/binfmts/)
endif()

add_subdirectory(Source/)
add_subdirectory(Data/AppConfig/)

# Install the ThunksDB file
file(GLOB CONFIG_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Data/*.json)

# Any application configuration json file gets installed
foreach(CONFIG_SRC ${CONFIG_SOURCES})
  install(FILES ${CONFIG_SRC}
    DESTINATION ${DATA_DIRECTORY}/)
endforeach()

if (BUILD_TESTS)
  add_subdirectory(unittests/)
endif()

if (BUILD_THUNKS)
  set (FEX_PROJECT_SOURCE_DIR ${PROJECT_SOURCE_DIR})
  add_subdirectory(ThunkLibs/Generator)

  # Thunk targets for both host libraries and IDE integration
  add_subdirectory(ThunkLibs/HostLibs)

  # Thunk targets for IDE integration of guest code, only
  add_subdirectory(ThunkLibs/GuestLibs)

  # Thunk targets for guest libraries
  include(ExternalProject)
  ExternalProject_Add(guest-libs
    PREFIX guest-libs
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ThunkLibs/GuestLibs"
    BINARY_DIR "Guest"
    CMAKE_ARGS
      "-DBITNESS=64"
      "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
      "-DBUILD_FEX_LINUX_TESTS=${BUILD_FEX_LINUX_TESTS}"
      "-DENABLE_CLANG_THUNKS=${ENABLE_CLANG_THUNKS}"
      "-DCMAKE_TOOLCHAIN_FILE:FILEPATH=${X86_64_TOOLCHAIN_FILE}"
      "-DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}"
      "-DFEX_PROJECT_SOURCE_DIR=${FEX_PROJECT_SOURCE_DIR}"
      "-DGENERATOR_EXE=$<TARGET_FILE:thunkgen>"
      "-DX86_DEV_ROOTFS=${X86_DEV_ROOTFS}"
    INSTALL_COMMAND ""
    BUILD_ALWAYS ON
    DEPENDS thunkgen
  )

  ExternalProject_Add(guest-libs-32
    PREFIX guest-libs-32
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ThunkLibs/GuestLibs"
    BINARY_DIR "Guest_32"
    CMAKE_ARGS
      "-DBITNESS=32"
      "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
      "-DBUILD_FEX_LINUX_TESTS=${BUILD_FEX_LINUX_TESTS}"
      "-DENABLE_CLANG_THUNKS=${ENABLE_CLANG_THUNKS}"
      "-DCMAKE_TOOLCHAIN_FILE:FILEPATH=${X86_32_TOOLCHAIN_FILE}"
      "-DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}"
      "-DFEX_PROJECT_SOURCE_DIR=${FEX_PROJECT_SOURCE_DIR}"
      "-DGENERATOR_EXE=$<TARGET_FILE:thunkgen>"
      "-DX86_DEV_ROOTFS=${X86_DEV_ROOTFS}"
    INSTALL_COMMAND ""
    BUILD_ALWAYS ON
    DEPENDS thunkgen
  )

  install(
    CODE "MESSAGE(\"-- Installing: guest-libs\")"
    CODE "
    EXECUTE_PROCESS(COMMAND ${CMAKE_COMMAND} --build . --target install
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/Guest
    )"
    DEPENDS guest-libs
  )

  install(
    CODE "MESSAGE(\"-- Installing: guest-libs-32\")"
    CODE "
    EXECUTE_PROCESS(COMMAND ${CMAKE_COMMAND} --build . --target install
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/Guest_32
    )"
    DEPENDS guest-libs-32
  )

  add_custom_target(uninstall_guest-libs
    COMMAND ${CMAKE_COMMAND} "--build" "." "--target" "uninstall"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/Guest
  )

  add_custom_target(uninstall_guest-libs-32
    COMMAND ${CMAKE_COMMAND} "--build" "." "--target" "uninstall"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/Guest_32
  )

  add_dependencies(uninstall uninstall_guest-libs)
  add_dependencies(uninstall uninstall_guest-libs-32)
endif()

set(FEX_VERSION_MAJOR "0")
set(FEX_VERSION_MINOR "0")
set(FEX_VERSION_PATCH "0")

if (OVERRIDE_VERSION STREQUAL "detect")
  find_package(Git)
  if (GIT_FOUND)
    execute_process(
      COMMAND ${GIT_EXECUTABLE} describe --abbrev=0
      WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
      OUTPUT_VARIABLE GIT_DESCRIBE_STRING
      RESULT_VARIABLE GIT_ERROR
      ERROR_QUIET
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if (NOT ${GIT_ERROR} EQUAL 0)
      # Likely built in a way that doesn't have tags
      # Setup a version tag that is unknown
      set(GIT_DESCRIBE_STRING "FEX-0000")
    endif()
  endif()
else()
  set(GIT_DESCRIBE_STRING "FEX-${OVERRIDE_VERSION}")
endif()

# Parse the version here
# Change something like `FEX-2106.1-76-<hash>` in to a list
string(REPLACE "-" ";" DESCRIBE_LIST ${GIT_DESCRIBE_STRING})

# Extract the `2106.1` element
list(GET DESCRIBE_LIST 1 DESCRIBE_LIST)

# Change `2106.1` in to a list
string(REPLACE "." ";" DESCRIBE_LIST ${DESCRIBE_LIST})

# Calculate list size
list(LENGTH DESCRIBE_LIST LIST_SIZE)

# Pull out the major version
list(GET DESCRIBE_LIST 0 FEX_VERSION_MAJOR)

# Minor version only exists if there is a .1 at the end
# eg: 2106 versus 2106.1
if (LIST_SIZE GREATER 1)
  list(GET DESCRIBE_LIST 1 FEX_VERSION_MINOR)
endif()

# Package creation
set (CPACK_GENERATOR "DEB")
set (CPACK_PACKAGE_NAME fex-emu)
set (CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${GIT_DESCRIBE_STRING}_${CMAKE_SYSTEM_PROCESSOR}")
set (CPACK_PACKAGE_CONTACT "FEX-Emu Maintainers <team@fex-emu.com>")
set (CPACK_PACKAGE_VERSION_MAJOR "${FEX_VERSION_MAJOR}")
set (CPACK_PACKAGE_VERSION_MINOR "${FEX_VERSION_MINOR}")
set (CPACK_PACKAGE_VERSION_PATCH "${FEX_VERSION_PATCH}")
set (CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/Data/CMake/CPack/Description.txt")

# Debian defines
set (CPACK_DEBIAN_PACKAGE_DEPENDS "libc6, libstdc++6, libepoxy0, libsdl2-2.0-0, libegl1, libx11-6, squashfuse")
set (CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
  "${CMAKE_CURRENT_SOURCE_DIR}/Data/CMake/CPack/postinst;${CMAKE_CURRENT_SOURCE_DIR}/Data/CMake/CPack/prerm;${CMAKE_CURRENT_SOURCE_DIR}/Data/CMake/CPack/triggers")
if (CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
  # binfmt_misc conflicts with qemu-user-static
  # We also only install binfmt_misc on aarch64 hosts
  set (CPACK_DEBIAN_PACKAGE_CONFLICTS "${CPACK_DEBIAN_PACKAGE_CONFLICTS}, qemu-user-static")
endif()
include (CPack)
