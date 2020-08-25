#include <stdio.h>

#include <alsa/asoundlib.h>

#include "common/Host.h"
#include <dlfcn.h>

#include "ldr_ptrs.inl"
#include "function_unpacks.inl"

static ExportEntry exports[] = {
    #include "tab_function_unpacks.inl"
    { nullptr, nullptr }
};

#include "ldr.inl"

EXPORTS(libasound) 

