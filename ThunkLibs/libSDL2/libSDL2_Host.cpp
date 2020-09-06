#include <stdio.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

#include "Thunk.h"
#include <dlfcn.h>

#include "libSDL2_initializers.inl"
#include "libSDL2_forwards.inl"

static ExportEntry exports[] = {
    #include "libSDL2_thunkmap.inl"
    { nullptr, nullptr }
};

EXPORTS(libSDL2)

