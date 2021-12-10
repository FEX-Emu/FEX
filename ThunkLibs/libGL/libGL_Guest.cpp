/*
$info$
tags: thunklibs|GL
desc: Handles glXGetProcAddress
$end_info$
*/

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

extern "C" {
	voidFunc *glXGetProcAddress(const GLubyte *procname) {

        for (int i = 0; internal_symtable[i].name; i++) {
            if (strcmp(internal_symtable[i].name, (const char*)procname) == 0) {
				// for debugging
                //printf("glXGetProcAddress: looked up %s %s %p %p\n", procname, internal_symtable[i].name, internal_symtable[i].fn, &glXGetProcAddress);
                return internal_symtable[i].fn;
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