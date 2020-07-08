#include <stdio.h>

#include <SDL2/SDL.h>

#include "../Thunk.h"
#include <dlfcn.h>

#include "libSDL2_Forwards.inl"

static ExportEntry exports[] = {
    #include "libSDL2_Thunkmap.inl"
    { nullptr, nullptr }
};

EXPORTS(libSDL2)

