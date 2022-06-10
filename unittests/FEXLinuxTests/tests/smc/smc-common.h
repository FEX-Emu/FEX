#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <cstring>

#include <unistd.h>
#include <fcntl.h>

#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/wait.h>


int test(char *code, const char *name) {
  // mov eax, imm32
  code[0] = 0xB8;
  code[1] = 0xAA;
  code[2] = 0xBB;
  code[3] = 0xCC;
  code[4] = 0xDD;

  // ret
  code[5] = 0xC3;

  auto fn = (int (*)())code;
  auto e1 = fn();
  
  // patch imm
  code[3] = 0xFE;
  auto e2 = fn();

  mprotect(code, 4096, PROT_READ | PROT_EXEC);

  mprotect(code, 4096, PROT_READ | PROT_WRITE | PROT_EXEC);

  // patch imm
  code[3] = 0xF3;

  mprotect(code, 4096, PROT_READ | PROT_EXEC);

  auto e3 = fn();

  mprotect(code, 4096, PROT_READ | PROT_WRITE | PROT_EXEC);

  // patch imm
  code[3] = 0xF1;

  auto e4 = fn();

  int failure_set = 0;

  failure_set |= (e1 != 0xDDCCBBAA) << 0;
  printf("%s-1: %X, %s\n", name, e1, e1 != 0xDDCCBBAA ? "FAIL" : "PASS");
  failure_set |= (e2 != 0xDDFEBBAA) << 1;
  printf("%s-2: %X, %s\n", name, e2, e2 != 0xDDFEBBAA ? "FAIL" : "PASS");
  failure_set |= (e3 != 0xDDF3BBAA) << 2;
  printf("%s-3: %X, %s\n", name, e3, e3 != 0xDDF3BBAA ? "FAIL" : "PASS");
  failure_set |= (e4 != 0xDDF1BBAA) << 3;
  printf("%s-4: %X, %s\n", name, e4, e4 != 0xDDF1BBAA ? "FAIL" : "PASS");

  return failure_set;
}

int test_shared(char* code, char* codeexec, const char* name) {
	assert(code != codeexec);
	code[0] = 0xB8;
	code[1] = 0xAA;
	code[2] = 0xBB;
	code[3] = 0xCC;
	code[4] = 0xDD;

	code[5] = 0xC3;

	auto fn = (int(*)())codeexec;
	auto e1 = fn();
	code[3]=0xFE;
	auto e2 = fn();

    int failure_set = 0;

    failure_set |= (e1 != 0xDDCCBBAA) << 0;
	printf("%s-1: %X, %s\n", name, e1, e1 != 0xDDCCBBAA? "FAIL" : "PASS");
    failure_set |= (e2 != 0xDDFEBBAA) << 1;
	printf("%s-2: %X, %s\n", name, e2, e2 != 0xDDFEBBAA? "FAIL" : "PASS");

    return failure_set;
}

int test_forked(char* code, char* codeexec, const char* name) {
	code[0] = 0xB8;
	code[1] = 0xAA;
	code[2] = 0xBB;
	code[3] = 0xCC;
	code[4] = 0xDD;

	code[5] = 0xC3;

	auto fn = (int(*)())codeexec;
	auto e1 = fn();
	auto pid = fork();
	if (pid == 0) {
		code[3]=0xFE;
		exit(0);
	} else {
        int status;
        wait(&status);
        return WEXITSTATUS(status);
	}
	auto e2 = fn();

	printf("%s-1: %X, %s\n", name, e1, e1 != 0xDDCCBBAA? "FAIL" : "PASS");
	printf("%s-2: %X, %s\n", name, e2, e2 != 0xDDFEBBAA? "FAIL" : "PASS");
}
