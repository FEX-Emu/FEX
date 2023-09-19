// SPDX-License-Identifier: MIT
#include <cstddef>
#include <cstdio>
#include <unordered_map>
#include <mutex>
#include <string>

#include <FEXCore/Debug/GDBReaderInterface.h>

GDB_DECLARE_GPL_COMPATIBLE_READER;

#define debugf(...)

extern "C" {
static enum gdb_status read_debug_info(struct gdb_reader_funcs *self, struct gdb_symbol_callbacks *cbs, void *memory, long memory_sz) {
  
  info_t *info = (info_t *)memory;
  blocks_t *blocks = (blocks_t *)(info->blocks_ofs + (long)memory);
  gdb_line_mapping *lines = (gdb_line_mapping *)(info->lines_ofs + (long)memory);
  debugf("info: %p\n", info);
  debugf("info: s %p\n", info->filename);
  debugf("info: s %s\n", info->filename);
  debugf("info: l %d\n", info->nlines);
  debugf("info: b %d\n", info->nblocks);

  struct gdb_object *object = cbs->object_open(cbs);
  struct gdb_symtab *symtab = cbs->symtab_open(cbs, object, info->filename);

  for (int i = 0; i < info->nblocks; i++) {
    debugf("info: %d\n", i);
    debugf("info: %lx\n", blocks[i].start);
    debugf("info: %lx\n", blocks[i].end);
    debugf("info: %s\n", blocks[i].name);
    cbs->block_open(cbs, symtab, NULL, blocks[i].start, blocks[i].end, blocks[i].name);
  }

  debugf("info: lines %d\n", info->nlines);
  debugf("info: lines %p\n", lines);

  for (int i = 0; i < info->nlines; i++) {
    debugf("info: line: %d\n", i);
    debugf("info: line pc: %lx\n", lines[i].pc);
    debugf("info: line file: %d\n", lines[i].line);
  }
  cbs->line_mapping_add(cbs, symtab, info->nlines, lines);

  // don't close here, symtab and object are cached
  cbs->symtab_close(cbs, symtab);
  cbs->object_close(cbs, object);
  return GDB_SUCCESS;
}

enum gdb_status unwind_frame(struct gdb_reader_funcs *self, struct gdb_unwind_callbacks *cbs) { return GDB_SUCCESS; }

struct gdb_frame_id get_frame_id(struct gdb_reader_funcs *self, struct gdb_unwind_callbacks *cbs) {
  struct gdb_frame_id frame = {0x1234000, 0};
  return frame;
}

void destroy_reader(struct gdb_reader_funcs *self) {}

extern struct gdb_reader_funcs *gdb_init_reader(void) {
  static struct gdb_reader_funcs funcs = {GDB_READER_INTERFACE_VERSION, NULL, read_debug_info, unwind_frame, get_frame_id, destroy_reader};
  return &funcs;
}
}
