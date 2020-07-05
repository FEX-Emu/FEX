#include <GL/glx.h>
#include <GL/gl.h>
#include <stdio.h>
#include <cstring>

typedef void voidFunc();
#define MAKE_THUNK(lib, name) __attribute__((naked)) int fexthunks_##lib##_##name(void* args) { asm("int $0x7F"); asm(".asciz \"" #lib ":" #name "\""); }

#include "glx-thunks.inl"

extern "C" {
voidFunc *glXGetProcAddress(const GLubyte *procname) {
	printf("glXGetProcAddress: %s\n", procname);
	if (strcmp("glXCreateContextAttribsARB", (const char*)procname) == 0)
		return (voidFunc*)&glXCreateContextAttribsARB;
	else if (strcmp("glXGetSwapIntervalMESA", (const char*)procname) == 0)
		return (voidFunc*)&glXGetSwapIntervalMESA;
	else
		return nullptr;
}

voidFunc *glXGetProcAddressARB(const GLubyte *procname) {
return glXGetProcAddress(procname);
}
}
