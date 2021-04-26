cmake_minimum_required(VERSION 3.14)
project(FEX)

option(BUILD_TESTS "Build unit tests to ensure sanity" TRUE)
option(BUILD_THUNKS "Build thunks" FALSE)
option(ENABLE_CLANG_FORMAT "Run clang format over the source" FALSE)
option(ENABLE_IWYU "Enables include what you use program" FALSE)
option(ENABLE_LTO "Enable LTO with compilation" TRUE)
option(ENABLE_XRAY "Enable building with LLVM X-Ray" FALSE)
option(ENABLE_LLD "Enable linking with LLD" FALSE)
option(ENABLE_ASAN "Enables Clang ASAN" FALSE)
option(ENABLE_TSAN "Enables Clang TSAN" FALSE)
option(ENABLE_ASSERTIONS "Enables assertions in build" FALSE)
option(ENABLE_VISUAL_DEBUGGER "Enables the visual debugger for compiling" FALSE)
option(ENABLE_STRICT_WERROR "Enables stricter -Werror for CI" FALSE)
option(ENABLE_WERROR "Enables -Werror" FALSE)

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
  add_compile_options(-fno-omit-frame-pointer -fsanitize=address -fsanitize-address-use-after-scope)
  link_libraries(-fno-omit-frame-pointer -fsanitize=address -fsanitize-address-use-after-scope)
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
  option(ENABLE_X86_HOST_DEBUG "Enables compiling on x86_64 host" FALSE)
  if (NOT ENABLE_X86_HOST_DEBUG)
    message(FATAL_ERROR
      " Be warned: FEX isn't optimized for x86_64 hosts!\n"
      " Support for x86_64 hosts is only for debugging and convenience!\n"
      " Don't expect amazing performance or optimal code generation!\n"
      " Pass -DENABLE_X86_HOST_DEBUG=True to bypass this message!")
  endif()
  set(_M_X86_64 1)
  add_definitions(-D_M_X86_64=1)
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcx16")
endif()

if (CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
  set(_M_ARM_64 1)
  add_definitions(-D_M_ARM_64=1)
endif()

add_subdirectory(External/vixl/)
include_directories(External/vixl/src/)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  # This means we were attempted to get compiled with GCC
  message(FATAL_ERROR "FEX doesn't support getting compiled with GCC!")
endif()

find_package(PkgConfig REQUIRED)
find_package(Python 3.0 REQUIRED COMPONENTS Interpreter)
pkg_check_modules(XXHASH libxxhash>=0.7.3 REQUIRED)

add_definitions(-Wno-trigraphs)
add_definitions(-DGLOBAL_DATA_DIRECTORY="${DATA_DIRECTORY}/")

add_subdirectory(External/jemalloc/)

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

if(ENABLE_WERROR OR ENABLE_STRICT_WERROR)
  add_compile_options(-Werror)
  if (NOT ENABLE_STRICT_WERROR)
    # Disable some Werror that can add frustration when developing
    add_compile_options(-Wno-error=unused-variable)
  endif()
endif()

if(_M_ARM_64)
  if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 12.0)
    # Clang 12.0 fixed the -mcpu=native bug with mixed big.little implementers
    check_cxx_compiler_flag("-mcpu=native" COMPILER_SUPPORTS_CPU_TYPE)
    if(COMPILER_SUPPORTS_CPU_TYPE)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcpu=native")
    endif()
  else()
    # Due to an oversight in llvm, it declares any reasonably new Kryo CPU to only be ARMv8.0
    # Manually detect newer CPU revisions until clang and llvm fixes their bug
    # This script will either provide a supported CPU or 'native'
    # Additionally -march doesn't work under AArch64+Clang, so you have to use -mcpu or -mtune
    execute_process(COMMAND python3 "${PROJECT_SOURCE_DIR}/Scripts/aarch64_fit_native.py" "/proc/cpuinfo"
      OUTPUT_VARIABLE AARCH64_CPU)

    string(STRIP ${AARCH64_CPU} AARCH64_CPU)

    check_cxx_compiler_flag("-mcpu=${AARCH64_CPU}" COMPILER_SUPPORTS_CPU_TYPE)
    if(COMPILER_SUPPORTS_CPU_TYPE)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcpu=${AARCH64_CPU}")
    endif()
  endif()
endif()

if (ENABLE_IWYU)
  find_program(IWYU_EXE "iwyu")
  if (IWYU_EXE)
    message(STATUS "IWYU enabled")
    set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE "${IWYU_EXE}")
  endif()
endif()

if (ENABLE_CLANG_FORMAT)
  find_program(CLANG_TIDY_EXE "clang-tidy")
  if (NOT CLANG_TIDY_EXE)
    message(FATAL_ERROR "Couldn't find clang-tidy")
  endif()

  set(CLANG_TIDY_FLAGS
    "-checks=*"
    "-fuchsia*"
    "-bugprone-macro-parentheses"
    "-clang-analyzer-core.*"
    "-cppcoreguidelines-pro-type-*"
    "-cppcoreguidelines-pro-bounds-array-to-pointer-decay"
    "-cppcoreguidelines-pro-bounds-pointer-arithmetic"
    "-cppcoreguidelines-avoid-c-arrays"
    "-cppcoreguidelines-avoid-magic-numbers"
    "-cppcoreguidelines-pro-bounds-constant-array-index"
    "-cppcoreguidelines-no-malloc"
    "-cppcoreguidelines-special-member-functions"
    "-cppcoreguidelines-owning-memory"
    "-cppcoreguidelines-macro-usage"
    "-cppcoreguidelines-avoid-goto"
    "-google-readability-function-size"
    "-google-readability-namespace-comments"
    "-google-readability-braces-around-statements"
    "-google-build-using-namespace"
    "-hicpp-*"
    "-llvm-namespace-comment"
    "-llvm-include-order" # Messes up with case sensitivity
    "-llvmlibc-*"
    "-misc-unused-parameters"
    "-modernize-loop-convert"
    "-modernize-use-auto"
    "-modernize-avoid-c-arrays"
    "-modernize-use-nodiscard"
    "readability-*"
    "-readability-function-size"
    "-readability-implicit-bool-conversion"
    "-readability-braces-around-statements"
    "-readability-else-after-return"
    "-readability-magic-numbers"
    "-readability-named-parameter"
    "-readability-uppercase-literal-suffix"
    "-cert-err34-c"
    "-cert-err58-cpp"
    "-bugprone-exception-escape"
    )
  string(REPLACE ";" "," CLANG_TIDY_FLAGS "${CLANG_TIDY_FLAGS}")
  set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY_EXE} "${CLANG_TIDY_FLAGS}")
endif()

add_compile_options(-Wall)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/include/Config.h.in
  ${CMAKE_BINARY_DIR}/generated/Config.h)

if (BUILD_TESTS)
  include(CTest)
  enable_testing()
  message(STATUS "Unit tests are enabled")
endif()
add_subdirectory(External/FEXCore)

add_subdirectory(Source/)
add_subdirectory(Data/AppConfig/)

if (BUILD_TESTS)
  add_subdirectory(unittests/)
endif()

if (BUILD_THUNKS)
  include(ExternalProject)

  ExternalProject_Add(host-libs
    PREFIX host-libs
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ThunkLibs/HostLibs"
    BINARY_DIR "Host"
    CMAKE_ARGS "-DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}"
    INSTALL_COMMAND ""
    BUILD_ALWAYS ON
  )

  install(
    CODE "MESSAGE(\"-- Installing: host-libs\")"
    CODE "
    EXECUTE_PROCESS(COMMAND ${CMAKE_COMMAND} --build . --target ThunkHostsInstall
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/Host
    )"
    DEPENDS host-libs
  )

  ExternalProject_Add(guest-libs
    PREFIX guest-libs
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ThunkLibs/GuestLibs"
    BINARY_DIR "Guest"
    CMAKE_ARGS "-DX86_C_COMPILER:STRING=${X86_C_COMPILER}" "-DX86_CXX_COMPILER:STRING=${X86_CXX_COMPILER}" "-DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}"
    INSTALL_COMMAND ""
    BUILD_ALWAYS ON
  )

  install(
    CODE "MESSAGE(\"-- Installing: guest-libs\")"
    CODE "
    EXECUTE_PROCESS(COMMAND ${CMAKE_COMMAND} --build . --target ThunkGuestsInstall
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/Guest
    )"
    DEPENDS guest-libs
  )
endif()
