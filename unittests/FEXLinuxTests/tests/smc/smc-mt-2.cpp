/*
  tests one thread modifying another thread's code

  main thread
  - allocates code buffer
  - starts secondary thread
  - waits to be signaled from secondary thread
  - modifies the code
  - signals secondary thread to claim the code is modified
  - waits for secondary thread to exit, while making sure it doesn't run the old code after modification
  - exits


  secondary thread
  - generates some code and runs it once
  - signals main thread to modify the code
  - waits to be signaled that code was modified
  - calls the to be code and checks if the result is the modified or non modified one
  - exits

*/

#include <cstdio>
#include <cstdlib>
#include <pthread.h>
#include <sys/mman.h>
#include <unistd.h>

#include <atomic>

#include <catch2/catch.hpp>

std::atomic<bool> ready_for_modification;
std::atomic<bool> waiting_for_modification;
std::atomic<bool> thread_unblocked;
std::atomic<int> thread_counter;

char* code;

void* thread(void*) {
  printf("Generating code on thread\n");
  code[0] = 0xB8;
  code[1] = 0xAA;
  code[2] = 0xBB;
  code[3] = 0xCC;
  code[4] = 0xDD;

  code[5] = 0xC3;

  auto fn = (int (*)())code;

  fn();

  ready_for_modification = true;
  printf("Waiting for code to be modified\n");

  while (!waiting_for_modification)
    ;

  while (fn() == 0xDDCCBBAA) {
    thread_counter++;
  }

  thread_unblocked = true;
  printf("Thread exiting\n");

  return 0;
}

void RunIteration() {
  printf("Starting Iteration\n");
  code = (char*)mmap(0, 4096, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANON, 0, 0);
  ready_for_modification = false;
  waiting_for_modification = false;
  thread_unblocked = false;
  thread_counter = 0;

  pthread_t tid;
  pthread_create(&tid, 0, &thread, 0);

  while (!ready_for_modification)
    ;

  printf("Modifying code from another thread\n");

  code[3] = 0xFE;

  waiting_for_modification = true;

  auto counter = thread_counter.load();

  printf("Waiting for thread to get unblocked\n");

  bool once = false;
  while (!thread_unblocked) {
    if (thread_counter != counter) {
      // depending on the patch timing, this might happen once
      if (once) {
        printf("Thread should have been patched to not modify counter here\n");
        exit(1);
      }
      printf("Thread overshoot once, this is non fatal\n");
      once = true;
      counter = thread_counter.load();
    }
  }

  printf("Iteration should finish now\n");
  void* rv;
  pthread_join(tid, &rv);
  printf("Iteration done\n");
  munmap(code, 4096);
}

TEST_CASE("SMC: One thread modifying another thread's code") {
  for (int i = 0; i < 100; i++) {
    RunIteration();
  }
}
