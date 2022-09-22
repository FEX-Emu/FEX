#define EXECSTACK

/*
We cannot test the omagic or the static version of this, due to cross compiling issues
//#define OMAGIC // when the g++ driver is used to link, -Wl,--omagic breaks -static, so this can't be tested
*/
/*
  tests for smc changes in .text, stack and bss
*/

char data_sym[16384];
char text_sym[16384] __attribute__((section(".text")));

#include "smc-common.h"

#include <catch2/catch.hpp>

TEST_CASE("SMC: Changes in stack") {
  // stack, depends on -z execstack or mprotect
  char stack[16384];
  auto code = (char *)(((uintptr_t)stack + 4095) & ~4095);

#if !defined(EXECSTACK)
  mprotect(code, 4096, PROT_READ | PROT_WRITE | PROT_EXEC);
#endif

  CHECK(test(code, "stack") == 0);
}

TEST_CASE("SMC: Changes in data section") {
  // data_sym, must use mprotect
  auto code = (char *)(((uintptr_t)data_sym + 4095) & ~4095);
  mprotect(code, 4096, PROT_READ | PROT_WRITE | PROT_EXEC);
  CHECK(test(code, "data_sym") == 0);
}

TEST_CASE("SMC: Changes in text section") {
  // text_sym, depends on -Wl,omagic or mprotect
  auto code = (char *)(((uintptr_t)text_sym + 4095) & ~4095);

#if !defined(OMAGIC)
  mprotect(code, 4096, PROT_READ | PROT_WRITE | PROT_EXEC);
#endif

  CHECK(test(code, "text_sym") == 0);
}
