#include <alsa/asoundlib.h>



#include <stdio.h>
#include <cstring>
#include <map>
#include <string>

#include "Thunk.h"
#include <stdarg.h>

LOAD_LIB(libasound)

#include "libasound_thunks.inl"
#include <vector>