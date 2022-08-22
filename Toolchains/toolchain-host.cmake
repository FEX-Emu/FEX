# Setup cmake enough so find_package works
set_property(GLOBAL PROPERTY TARGET_SUPPORTS_SHARED_LIBS TRUE)

set(CMAKE_FIND_LIBRARY_PREFIXES "/lib")
set(CMAKE_FIND_LIBRARY_SUFFIXES "")

# This would be sweet, but clang doesn't cross compile correctly when called with the full path, only through the /usr/bin/clang symlink
## find clang
## find_package(Clang REQUIRED CONFIG)

# Instead, this is used for now
set(CLANG_INSTALL_PREFIX /usr)

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

# Default to lld as that is also what the thunks use by default
if (NOT DEFINED ENV{"LD"})
  set(LINKER_OVERRIDE "lld")
else()
  set(LINKER_OVERRIDE ENV{"LD"})
endif()
