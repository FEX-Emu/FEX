#include <GL/glx.h>
#include <GL/gl.h>

typedef void voidFunc();
#define MAKE_THUNK(lib, name) __attribute__((naked)) int fexthunks_##lib##_##name(void* args) { asm("int $0x7F"); asm(".asciz \"" #lib ":" #name "\""); }

#include "glx-thunks.inl"

extern "C" {
voidFunc *glXGetProcAddress(const GLubyte *procname) {
	return (voidFunc*)&glXCreateContextAttribsARB;
}

}
