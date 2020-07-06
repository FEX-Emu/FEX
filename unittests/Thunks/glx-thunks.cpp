#define GL_GLEXT_PROTOTYPES 1
#include "glcorearb.h"

#include <GL/glx.h>
#include <GL/gl.h>
#include <stdio.h>
#include <cstring>

typedef void voidFunc();
#define MAKE_THUNK(lib, name) __attribute__((naked)) int fexthunks_##lib##_##name(void* args) { asm("int $0x7F"); asm(".asciz \"" #lib ":" #name "\""); }

typedef int XSetErrorHandlerFN(Display*, XErrorEvent*);
typedef int XIfEventFN(Display*, XEvent*, XPointer);

#include "glx-thunks.inl"

static char temp[] = "asd";
#include <map>
#include <string>

std::map<std::string, voidFunc*> symtab = {
#include "glx-syms.inl"
};

extern "C" {
voidFunc *glXGetProcAddress(const GLubyte *procname) {
	auto sym = std::string("libGL:") + (const char*)procname;

	if (symtab.count(sym) == 1) {
		return symtab[sym];
	}
	else {
		printf("glXGetProcAddress: not found %s\n", sym.c_str());
		return nullptr;
	}
}

voidFunc *glXGetProcAddressARB(const GLubyte *procname) {
return glXGetProcAddress(procname);
}

char* XGetICValues(XIC, ...) {
    return temp;
}
_XIC* XCreateIC(XIM, ...) {
    return nullptr;
}
}

