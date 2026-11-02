set(CMAKE_SYSTEM_PROCESSOR i686)

set(LLVM_MINGW "" CACHE FILEPATH "Path to LLVM_MINGW")
if (LLVM_MINGW)
  set(LLVM_MINGW_BIN "${LLVM_MINGW}/bin/")
else()
  set(LLVM_MINGW_BIN "")
endif()

set(MINGW_TRIPLE "i686-w64-mingw32")

set(CMAKE_RC_COMPILER ${LLVM_MINGW_BIN}${MINGW_TRIPLE}-windres)
set(CMAKE_C_COMPILER ${LLVM_MINGW_BIN}${MINGW_TRIPLE}-clang)
set(CMAKE_CXX_COMPILER ${LLVM_MINGW_BIN}${MINGW_TRIPLE}-clang++)
set(CMAKE_DLLTOOL ${LLVM_MINGW_BIN}${MINGW_TRIPLE}-dlltool)
set(CMAKE_AR ${LLVM_MINGW_BIN}${MINGW_TRIPLE}-ar)

# Compile everything as static to avoid requiring the MinGW runtime libraries, force page aligned sections so that
# debug symbols work correctly, and disable loop alignment to workaround an LLVM bug
# (https://github.com/llvm/llvm-project/issues/47432)
set(CMAKE_SHARED_LINKER_FLAGS_INIT "-static -Wl,--file-alignment=4096,/mllvm:-align-loops=1")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-static -Wl,--file-alignment=4096,/mllvm:-align-loops=1")
set(CMAKE_C_STANDARD_LIBRARIES "" CACHE STRING "" FORCE)
set(CMAKE_CXX_STANDARD_LIBRARIES "" CACHE STRING "" FORCE)
set(CMAKE_STANDARD_LIBRARIES "" CACHE STRING "" FORCE)
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR ${MINGW_TRIPLE})

set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

