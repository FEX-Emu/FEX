// SPDX-License-Identifier: MIT
#include "GDBJIT.h"

#include <FEXCore/Debug/InternalThreadState.h>
#include <FEXCore/HLE/SourcecodeResolver.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/fextl/vector.h>

#if defined(GDB_SYMBOLS_ENABLED)

#include <FEXCore/Debug/GDBReaderInterface.h>

extern "C" {
enum jit_actions_t { JIT_NOACTION = 0, JIT_REGISTER_FN, JIT_UNREGISTER_FN };

struct jit_code_entry {
  jit_code_entry* next_entry;
  jit_code_entry* prev_entry;
  const char* symfile_addr;
  uint64_t symfile_size;
};

struct jit_descriptor {
  uint32_t version;
  /* This type should be jit_actions_t, but we use uint32_t
     to be explicit about the bitwidth.  */
  uint32_t action_flag;
  jit_code_entry* relevant_entry;
  jit_code_entry* first_entry;
};

/* Make sure to specify the version statically, because the
   debugger may check the version before we can set it.  */

constinit jit_descriptor __jit_debug_descriptor = {.version = 1};

/* GDB puts a breakpoint in this function.  */
void __attribute__((noinline)) __jit_debug_register_code() {
  asm volatile("" ::"r"(&__jit_debug_descriptor));
};
}

namespace FEXCore {

void GDBJITRegister(FEXCore::IR::AOTIRCacheEntry* Entry, uintptr_t VAFileStart, uint64_t GuestRIP, uintptr_t HostEntry,
                    FEXCore::Core::DebugData* DebugData) {
  auto map = Entry->SourcecodeMap.get();

  if (map) {
    auto FileOffset = GuestRIP - VAFileStart;

    auto Sym = map->FindSymbolMapping(FileOffset);

    auto SymName = HLE::SourcecodeSymbolMapping::SymName(Sym, Entry->Filename, HostEntry, FileOffset);

    fextl::vector<gdb_line_mapping> Lines;
    for (const auto& GuestOpcode : DebugData->GuestOpcodes) {
      auto Line = map->FindLineMapping(GuestRIP + GuestOpcode.GuestEntryOffset - VAFileStart);
      if (Line) {
        Lines.push_back({Line->LineNumber, HostEntry + GuestOpcode.HostEntryOffset});
      }
    }

    size_t size = sizeof(info_t) + 1 * sizeof(blocks_t) + Lines.size() * sizeof(gdb_line_mapping);

    auto mem = (uint8_t*)malloc(size);
    auto base = mem;
    info_t* info = (info_t*)mem;
    mem += sizeof(info_t);

    strncpy(info->filename, map->SourceFile.c_str(), 511);

    info->nblocks = 1;

    auto blocks = (blocks_t*)mem;
    info->blocks_ofs = mem - base;

    mem += info->nblocks * sizeof(blocks_t);

    for (int i = 0; i < info->nblocks; i++) {
      strncpy(blocks[i].name, SymName.c_str(), 511);
      blocks[i].start = HostEntry;
      blocks[i].end = HostEntry + DebugData->HostCodeSize;
    }

    info->nlines = Lines.size();

    auto lines = (gdb_line_mapping*)mem;
    info->lines_ofs = mem - base;
    mem += info->nlines * sizeof(gdb_line_mapping);

    if (info->nlines) {
      memcpy(lines, &Lines.at(0), info->nlines * sizeof(gdb_line_mapping));
    }

    auto entry = new jit_code_entry {0, 0, 0, 0};

    entry->symfile_addr = (const char*)info;
    entry->symfile_size = size;

    if (__jit_debug_descriptor.first_entry) {
      __jit_debug_descriptor.relevant_entry->next_entry = entry;
      entry->prev_entry = __jit_debug_descriptor.relevant_entry;
    } else {
      __jit_debug_descriptor.first_entry = entry;
    }

    __jit_debug_descriptor.relevant_entry = entry;
    __jit_debug_descriptor.action_flag = JIT_REGISTER_FN;
    __jit_debug_register_code();
  }
}
} // namespace FEXCore
#else
namespace FEXCore {
void GDBJITRegister(FEXCore::IR::AOTIRCacheEntry*, uintptr_t, uint64_t, uintptr_t, FEXCore::Core::DebugData*) {
  ERROR_AND_DIE_FMT("GDBSymbols support not compiled in");
}
} // namespace FEXCore
#endif
