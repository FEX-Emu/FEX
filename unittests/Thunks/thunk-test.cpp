#include <stdio.h>

#define MAKE_THUNK(lib, name) __attribute__((naked)) int fexthunks_##lib##_##name(void* args) { asm("int $0x7F"); asm(".asciz \"" #lib ":" #name "\""); }

#include "thunks.inl"

int main() {
	printf("guest: test_void\n");
	test_void(63);
	printf("guest: test\n");
	printf("%d\n", test(128));
	printf("guest: add\n");
	printf("%d\n", add(64, 31));
	printf("guest: done\n");

}
