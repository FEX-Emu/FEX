/*
  tests for smc changes in .text, stack and bss
*/

char data_sym[16384];
char text_sym[16384] __attribute__((section(".text")));

#include "smc-common.h"

int main(int argc, char *argv[]) {

  if (argc == 2) {

    if (strcmp(argv[1], "stack") == 0) {
      // stack, depends on -z execstack or mprotect
      char stack[16384];
      auto code = (char *)(((uintptr_t)stack + 4095) & ~4095);

#if !defined(EXECSTACK)
      mprotect(code, 4096, PROT_READ | PROT_WRITE | PROT_EXEC);
#endif

      return test(code, "stack");
    } else if (strcmp(argv[1], "data_sym") == 0) {
      // data_sym, must use mprotect
      auto code = (char *)(((uintptr_t)data_sym + 4095) & ~4095);
      mprotect(code, 4096, PROT_READ | PROT_WRITE | PROT_EXEC);
      return test(code, "data_sym");
    } else if (strcmp(argv[1], "text_sym") == 0) {
      // text_sym, depends on -Wl,omagic or mprotect
      auto code = (char *)(((uintptr_t)text_sym + 4095) & ~4095);

#if !defined(OMAGIC)
      mprotect(code, 4096, PROT_READ | PROT_WRITE | PROT_EXEC);
#endif

      return test(code, "text_sym");
    }
  }

  printf("Invalid arguments\n");
  printf("please specify one of %s\n", args);

  return -1;
}
