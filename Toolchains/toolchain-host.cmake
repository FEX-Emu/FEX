# Setup cmake enough so find_package works
SET_PROPERTY(GLOBAL PROPERTY TARGET_SUPPORTS_SHARED_LIBS TRUE)

find_package(Clang REQUIRED CONFIG)

# Set default c compiler based on located clang install

if (NOT DEFINED ENV{"CC"})
  set(CMAKE_C_COMPILER "${CLANG_INSTALL_PREFIX}/bin/clang")
endif()

# Set default c++ compiler
if (NOT DEFINED ENV{"CXX"})
  set(CMAKE_CXX_COMPILER "${CLANG_INSTALL_PREFIX}/bin/clang++")
endif()

if (NOT DEFINED ENV{"LD"})
  set(LINKER_OVERRIDE "${CLANG_INSTALL_PREFIX}/bin/lld")
else()
  set(LINKER_OVERRIDE ENV{"LD"})
endif()