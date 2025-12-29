# SPDX-License-Identifier: MIT

# This applies some common linker options that reduce code size and linking time in Release mode. Namely:
# --gc-sections: Linktime garbage collection, discards unused sections from the final output
# --strip-all  : Similar to running `strip`, discards the symbol table from the final output
# --as-needed  : Only includes libraries that are actually needed in the final output.

macro(LinkerGC target)
  if (CMAKE_BUILD_TYPE MATCHES "RELEASE")
    target_link_options(${target} PRIVATE
      "LINKER:--gc-sections"
      "LINKER:--strip-all"
      "LINKER:--as-needed")
  endif()
endmacro()
