include(ExternalProject)
ExternalProject_Add(FEXMingwTests
  PREFIX FEXMingwTests
  SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tests"
  BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/FEXMingwTests_64"
  CMAKE_ARGS
  "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
  "-DLLVM_MINGW=${LLVM_MINGW}"
  "-DCMAKE_TOOLCHAIN_FILE:FILEPATH=${X86_64_MINGW_TOOLCHAIN_FILE}"
  "-DBITNESS=64"
  INSTALL_COMMAND ""
  BUILD_ALWAYS ON)

ExternalProject_Add(FEXMingwTests_32
  PREFIX FEXMingwTests_32
  SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tests"
  BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/FEXMingwTests_32"
  CMAKE_ARGS
  "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
  "-DLLVM_MINGW=${LLVM_MINGW}"
  "-DCMAKE_TOOLCHAIN_FILE:FILEPATH=${X86_32_MINGW_TOOLCHAIN_FILE}"
  "-DBITNESS=32"
  INSTALL_COMMAND ""
  BUILD_ALWAYS ON)

# this kind of sucks, but reglob
file(GLOB_RECURSE TESTS CONFIGURE_DEPENDS tests/*.cpp)
file(GLOB_RECURSE TESTS_64_ONLY CONFIGURE_DEPENDS tests/*.64.cpp)
file(GLOB_RECURSE TESTS_32_ONLY CONFIGURE_DEPENDS tests/*.32.cpp)

# Apply bitness-specific exclude lists
list(REMOVE_ITEM TESTS ${TESTS_64_ONLY})
list(REMOVE_ITEM TESTS ${TESTS_32_ONLY})

function(AddTests Tests BinDirectory Bitness)
  # TODO: Implement some form of test harness for mingw tests.
endfunction()

# Execute combined 32-bit and 64-bit tests.
AddTests("${TESTS}" "FEXMingwTests_64" 64)
AddTests("${TESTS}" "FEXMingwTests_32" 32)
# Execute tests that are only 64-bit.
AddTests("${TESTS_64_ONLY}" "FEXMingwTests_64" 64)
# Execute tests that are only 32-bit.
AddTests("${TESTS_32_ONLY}" "FEXMingwTests_32" 32)
