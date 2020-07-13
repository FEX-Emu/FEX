#pragma once

namespace FEXCore::Context {
  struct Context;
}

#define MAKE_THUNK(lib, name) static __attribute__((naked)) int fexthunks_##lib##_##name(void *args) { asm("int $0x7F"); asm(".asciz \"" #lib ":" #name "\""); }

#define LOAD_LIB(name) MAKE_THUNK(fex, loadlib) __attribute__((constructor)) static void loadlib() { char libname[] = #name; fexthunks_fex_loadlib(libname); }

struct ExportEntry { const char* name; void(*fn)(FEXCore::Context::Context *, void *); };

#define EXPORTS(name) extern "C" { ExportEntry* fexthunks_exports_##name() { fexthunks_init_##name(); return exports; } }