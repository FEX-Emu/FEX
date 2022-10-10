option(ENABLE_CLANG_THUNKS "Enable building thunks with clang" FALSE)

set(CMAKE_SYSTEM_PROCESSOR i686)

if (ENABLE_CLANG_THUNKS)
  message(STATUS "Enabling thunk clang building. Force enabling LLD as well")

  set(CMAKE_EXE_LINKER_FLAGS_INIT "-fuse-ld=lld")
  set(CMAKE_MODULE_LINKER_FLAGS_INIT "-fuse-ld=lld")
  set(CMAKE_SHARED_LINKER_FLAGS_INIT "-fuse-ld=lld")
  set(CMAKE_C_COMPILER clang)
  set(CMAKE_CXX_COMPILER clang++)
  set(CLANG_FLAGS "-target i686-linux-gnu -msse2 -mfpmath=sse")

  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CLANG_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CLANG_FLAGS}")
else()
  set(CMAKE_C_COMPILER x86_64-linux-gnu-gcc -m32)
  set(CMAKE_CXX_COMPILER x86_64-linux-gnu-g++ -m32)
endif()
