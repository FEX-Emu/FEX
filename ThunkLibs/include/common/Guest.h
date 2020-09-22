#pragma once
#include <stdint.h>

#define MAKE_THUNK(lib, name) static __attribute__((naked)) int fexthunks_##lib##_##name(void *args) { asm(".byte 0xF, 0x3F"); asm(".asciz \"" #lib ":" #name "\""); }

struct LoadlibArgs {
    const char *Name;
    uintptr_t CallbackThunks;
};

#define LOAD_LIB(name) MAKE_THUNK(fex, loadlib) __attribute__((constructor)) static void loadlib() { LoadlibArgs args =  { #name, 0 }; fexthunks_fex_loadlib(&args); }
#define LOAD_LIB_WITH_CALLBACKS(name) MAKE_THUNK(fex, loadlib) __attribute__((constructor)) static void loadlib() { LoadlibArgs args =  { #name, (uintptr_t)&callback_unpacks }; fexthunks_fex_loadlib(&args); }