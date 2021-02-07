#pragma once
#include <stdint.h>

struct ExportEntry { uint8_t* sha256; void(*fn)(void *); };

typedef void fex_call_callback_t(uintptr_t callback, void *arg0, void* arg1);

static fex_call_callback_t* call_guest;

#define EXPORTS(name) extern "C" { ExportEntry* fexthunks_exports_##name(void *a0, uintptr_t a1) { call_guest = (fex_call_callback_t*)a0; fexldr_init_##name(); return exports; } }
#define EXPORTS_WITH_CALLBACKS(name) extern "C" { ExportEntry* fexthunks_exports_##name(void *a0, uintptr_t a1) { call_guest = (fex_call_callback_t*)a0; (uintptr_t&)callback_unpacks = a1; fexldr_init_##name(); return exports; } }
