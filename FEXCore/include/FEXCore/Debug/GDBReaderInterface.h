// SPDX-License-Identifier: MIT
#include <cstddef>
#include <cstdint>

#include <gdb/jit-reader.h>

// everything is stored inline as it is marshaled cross process by gdb

struct blocks_t {
  char name[512];
  GDB_CORE_ADDR start;
  GDB_CORE_ADDR end;
};

struct info_t {
  char filename[512];

  ptrdiff_t blocks_ofs;
  ptrdiff_t lines_ofs;

  int nblocks;
  int nlines;
};
