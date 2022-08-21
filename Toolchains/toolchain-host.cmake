# Setup cmake enough so find_package works
set_property(GLOBAL PROPERTY TARGET_SUPPORTS_SHARED_LIBS TRUE)

set(CMAKE_FIND_LIBRARY_PREFIXES "/lib")
set(CMAKE_FIND_LIBRARY_SUFFIXES "")

# find clang
find_package(Clang REQUIRED CONFIG)

unset(CMAKE_FIND_LIBRARY_PREFIXES)
unset(CMAKE_FIND_LIBRARY_SUFFIXES)

# Set default c compiler based on located clang install

if (NOT DEFINED ENV{"CC"})
  set(CMAKE_C_COMPILER "${CLANG_INSTALL_PREFIX}/bin/clang")
endif()

# Set default c++ compiler
if (NOT DEFINED ENV{"CXX"})
  set(CMAKE_CXX_COMPILER "${CLANG_INSTALL_PREFIX}/bin/clang++")
endif()

if (NOT DEFINED ENV{"LD"})
  set(LINKER_OVERRIDE "${CLANG_INSTALL_PREFIX}/bin/ld.lld")
else()
  set(LINKER_OVERRIDE ENV{"LD"})
endif()
