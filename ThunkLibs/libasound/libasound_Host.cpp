#include <stdio.h>

#include <alsa/asoundlib.h>

#include "Thunk.h"
#include <dlfcn.h>

#include "libasound_initializers.inl"
#include "libasound_forwards.inl"

static ExportEntry exports[] = {
    #include "libasound_thunkmap.inl"
    { nullptr, nullptr }
};

EXPORTS(libasound) 

