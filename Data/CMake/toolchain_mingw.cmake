set(MINGW_TRIPLE "" CACHE STRING "MinGW compiler target architecture triple")

set(CMAKE_RC_COMPILER ${MINGW_TRIPLE}-windres)
set(CMAKE_C_COMPILER ${MINGW_TRIPLE}-clang)
set(CMAKE_CXX_COMPILER ${MINGW_TRIPLE}-clang++)
set(CMAKE_DLLTOOL ${MINGW_TRIPLE}-dlltool)
set(CMAKE_AR ${MINGW_TRIPLE}-ar)

# Compile everything as static to avoid requiring the MinGW runtime libraries, force page aligned sections so that
# debug symbols work correctly, and disable loop alignment to workaround an LLVM bug
# (https://github.com/llvm/llvm-project/issues/47432)
set(CMAKE_SHARED_LINKER_FLAGS_INIT "-static -static-libgcc -static-libstdc++ -Wl,--file-alignment=4096,/mllvm:-align-loops=1")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-static -static-libgcc -static-libstdc++ -Wl,--file-alignment=4096,/mllvm:-align-loops=1")
set(CMAKE_C_STANDARD_LIBRARIES "" CACHE STRING "" FORCE)
set(CMAKE_CXX_STANDARD_LIBRARIES "" CACHE STRING "" FORCE)
set(CMAKE_STANDARD_LIBRARIES "" CACHE STRING "" FORCE)
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR ${MINGW_TRIPLE})

set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
