// libs: pthread

/*
  tests concurrent invalidation of different code from different threads

  creates 10 threads
  each thread does an smc test 10 times
  
*/
#include <cstdio>
#include <pthread.h>
#include <sys/mman.h>

#include <atomic>

std::atomic<int> result;
std::atomic<bool> go;

void *thread(void *) {

  auto code = (char *)mmap(0, 4096, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANON, 0, 0);

  for (int k = 0; k < 10; k++) {
    code[0] = 0xB8;
    code[1] = 0xAA;
    code[2] = 0xBB;
    code[3] = 0xCC;
    code[4] = 0xDD;

    code[5] = 0xC3;

    while(!go) ;

    auto fn = (int (*)())code;
    auto e1 = fn();
    code[3] = 0xFE;
    auto e2 = fn();

    mprotect(code, 4096, PROT_READ | PROT_EXEC);

    mprotect(code, 4096, PROT_READ | PROT_WRITE | PROT_EXEC);

    code[3] = 0xF3;

    mprotect(code, 4096, PROT_READ | PROT_EXEC);

    auto e3 = fn();

    mprotect(code, 4096, PROT_READ | PROT_WRITE | PROT_EXEC);

    code[3] = 0xF1;

    auto e4 = fn();

    result |= e1 != 0xDDCCBBAA;
    printf("Exec1: %X, %s\n", e1, e1 != 0xDDCCBBAA ? "FAIL" : "PASS");
    result |= e2 != 0xDDFEBBAA;
    printf("Exec2: %X, %s\n", e2, e2 != 0xDDFEBBAA ? "FAIL" : "PASS");
    result |= e3 != 0xDDF3BBAA;
    printf("Exec3: %X, %s\n", e3, e3 != 0xDDF3BBAA ? "FAIL" : "PASS");
    result |= e4 != 0xDDF1BBAA;
    printf("Exec4: %X, %s\n", e4, e4 != 0xDDF1BBAA ? "FAIL" : "PASS");
  }

  return 0;
}

int main() {
  pthread_t tid[10];
  for (int i = 0; i < 10; i++) {
    pthread_create(&tid[i], 0, &thread, 0);
  }

  go = true;

  for (int i = 0; i < 10; i++) {
    void *rv;
    pthread_join(tid[i], &rv);
  }

  return result;
}
