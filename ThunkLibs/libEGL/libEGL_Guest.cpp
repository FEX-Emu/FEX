#include <GL/glx.h>
#include <EGL/egl.h>

#include <stdio.h>
#include <cstring>

#include "common/Guest.h"

#include "thunks.inl"
#include "function_packs.inl"
#include "function_packs_public.inl"

typedef void voidFunc();


extern "C" {
	voidFunc *eglGetProcAddress(const char *procname) {
		// TODO: Fix this HACK
		return glXGetProcAddress((const GLubyte*)procname);
	}
}

LOAD_LIB(libEGL)