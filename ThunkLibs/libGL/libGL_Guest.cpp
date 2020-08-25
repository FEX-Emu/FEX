#define GL_GLEXT_PROTOTYPES 1
#define GLX_GLXEXT_PROTOTYPES 1

#include "glcorearb.h"

#include <GL/glx.h>
#include <GL/glxext.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include <stdio.h>
#include <cstring>

#include "common/Guest.h"

#include "thunks.inl"
#include "function_packs.inl"
#include "function_packs_public.inl"

typedef void voidFunc();

static struct { const char* name; voidFunc* fn; } symtab[] = {
	#include "tab_function_packs.inl"
	{ nullptr, nullptr }
};

extern "C" {
	voidFunc *glXGetProcAddress(const GLubyte *procname) {

		for (int i = 0; symtab[i].name; i++) {
			if (strcmp(symtab[i].name, (const char*)procname) == 0) {
				// for debugging
				//printf("glXGetProcAddress: looked up %s %s %p %p\n", procname, symtab[i].name, symtab[i].fn, &glXGetProcAddress);
				return symtab[i].fn;
			}
		}

		printf("glXGetProcAddress: not found %s\n", procname);
		return nullptr;
	}

	voidFunc *glXGetProcAddressARB(const GLubyte *procname) {
		return glXGetProcAddress(procname);
	}
}

LOAD_LIB(libGL)