#include <atomic>
#include <catch2/catch.hpp>
#include <fstream>
#include <sys/syscall.h>
#include <linux/futex.h>
#include <signal.h>
#include <optional>
#include <thread>

#if __SIZEOF_POINTER__ == 4
#define DO_ASM(x, y) \
  __asm volatile( \
  x \
  /* Need to late move syscall number since incoming asm will overwrite eax */ \
  " mov eax, %[Syscall];" \
  /* Notify we are ready (Without touching flags) */ \
  "mov dword ptr [%[ReadyNotify]], 1;" \
  /* Do a futex */ \
  "int 0x80;" \
  y \
  : \
  : [Syscall] "i" (SYS_futex) \
  , "b" (Futex) \
  , "c" (FUTEX_WAIT) \
  , "d" (0) \
  , "S" (0) \
  , [ReadyNotify] "r" (ReadyNotify) \
  : "cc", "memory", "eax")
#else

#define DO_ASM(x, y) \
  __asm volatile( \
  x \
  /* Do a futex */ \
  " mov rax, %[Syscall];" \
  " mov rdi, %[FutexAddr];" \
  " mov rsi, %[FutexOp];" \
  " mov rdx, %[ExpectedValue];" \
  " mov r10, %[TimeoutAddr];" \
  /* Notify we are ready (Without touching flags) */ \
  "mov dword ptr [%[ReadyNotify]], 1;" \
  "syscall;" \
  y \
  : \
  : [Syscall] "i" (SYS_futex) \
  , [FutexAddr] "r" (Futex) \
  , [FutexOp] "i" (FUTEX_WAIT) \
  , [ExpectedValue] "i" (0) \
  , [TimeoutAddr] "i" (0) \
  , [ReadyNotify] "r" (ReadyNotify) \
  : "cc", "memory", "rax", "rdi", "rsi", "rdx", "r10")
#endif

static void ClearCFAndWait(std::atomic<uint32_t> *Futex, std::atomic<uint32_t> *ReadyNotify) {
  DO_ASM(
    // Clear CF
    "clc;"
  ,
  // CF should still be cleared.
  "jnc 1f;"
  "int3;"
  "1:"
  );
}

static void SetCFAndWait(std::atomic<uint32_t> *Futex, std::atomic<uint32_t> *ReadyNotify) {
  DO_ASM(
    // Set CF
    "stc;"
  ,
  // CF should still be set.
  "jc 1f;"
  "int3;"
  "1:"
  );
}

static void ClearPFAndWait(std::atomic<uint32_t> *Futex, std::atomic<uint32_t> *ReadyNotify) {
  DO_ASM(
    // Clear PF
    "mov eax, 0;"
    "inc eax;"
  ,

  // PF should still be cleared.
  "jnp 1f;"
  "int3;"
  "1:"
  );
}

static void SetPFAndWait(std::atomic<uint32_t> *Futex, std::atomic<uint32_t> *ReadyNotify) {
  DO_ASM(
    // Set PF
    "mov eax, 0x80;"
    "inc eax;"
  ,
  // PF should still be set.
  "jp 1f;"
  "int3;"
  "1:"
  );
}

static void ClearZFAndWait(std::atomic<uint32_t> *Futex, std::atomic<uint32_t> *ReadyNotify) {
  DO_ASM(
    // Clear ZF
    "mov eax, 2;"
    "dec eax;"
  ,
  // ZF should still be cleared.
  "jnz 1f;"
  "int3;"
  "1:"
  );
}

static void SetZFAndWait(std::atomic<uint32_t> *Futex, std::atomic<uint32_t> *ReadyNotify) {
  DO_ASM(
    // Set ZF
    "mov eax, 1;"
    "dec eax;"
  ,
  // ZF should still be set.
  "jz 1f;"
  "int3;"
  "1:"
  );
}

static void ClearSFAndWait(std::atomic<uint32_t> *Futex, std::atomic<uint32_t> *ReadyNotify) {
  DO_ASM(
    // Clear SF
    "mov eax, 1;"
    "dec eax;"
  ,
  // SF should still be cleared.
  "jns 1f;"
  "int3;"
  "1:"
  );
}

static void SetSFAndWait(std::atomic<uint32_t> *Futex, std::atomic<uint32_t> *ReadyNotify) {
  DO_ASM(
    // Set SF
    "mov eax, 0;"
    "dec eax;"
  ,
  // SF should still be set.
  "js 1f;"
  "int3;"
  "1:"
  );
}

static void ClearOFAndWait(std::atomic<uint32_t> *Futex, std::atomic<uint32_t> *ReadyNotify) {
  DO_ASM(
    // Clear OF
    "mov eax, 0;"
    "inc eax;"
  ,
  // OF should still be cleared.
  "jno 1f;"
  "int3;"
  "1:"
  );
}

static void SetOFAndWait(std::atomic<uint32_t> *Futex, std::atomic<uint32_t> *ReadyNotify) {
  DO_ASM(
    // Set OF
    "mov eax, 0x7fffffff;"
    "inc eax;"
  ,
  // OF should still be set.
  "jo 1f;"
  "int3;"
  "1:"
  );
}

struct CapturingData {
  int Signal;
  uint64_t eflags;
};

std::optional<CapturingData> from_handler;
constexpr uint32_t EFL_CF = 0;
constexpr uint32_t EFL_PF = 2;
constexpr uint32_t EFL_ZF = 6;
constexpr uint32_t EFL_SF = 7;
constexpr uint32_t EFL_OF = 11;

static void CapturingHandler(int signal, siginfo_t *siginfo, void* context) {
  ucontext_t* _context = (ucontext_t*)context;

  from_handler = {
    .Signal = signal,
    .eflags = static_cast<uint64_t>(_context->uc_mcontext.gregs[REG_EFL]),
  };
}

using TestHandler = std::function<void(std::atomic<uint32_t> *Futex, std::atomic<uint32_t> *ReadyNotify)>;

static void ThreadHandler(std::atomic<uint32_t> *Mutex, std::atomic<uint32_t> *ReadyNotify, std::atomic<uint32_t> *ThreadID, TestHandler Test) {
  // Unblock SIGTERM.
  sigset_t BlockMask{};
  sigemptyset(&BlockMask);
  sigaddset(&BlockMask, SIGTERM);
  sigprocmask(SIG_UNBLOCK, &BlockMask, nullptr);

  // Set up a signal handler for SIGTERM
  struct sigaction act{};
  act.sa_sigaction = CapturingHandler;
  act.sa_flags = SA_SIGINFO;
  sigaction(SIGTERM, &act, nullptr);

  *ThreadID = ::gettid();
  Test(Mutex, ReadyNotify);
}

void WaitForThreadAsleep(uint32_t tid) {
  std::string Path = "/proc/" + std::to_string(::getpid()) + "/task/" + std::to_string(tid) + "/status";
  std::ifstream fs{Path, std::fstream::binary};
  std::string Line;

  while (true) {
    fs.clear();
    fs.seekg(0);
    while (std::getline(fs, Line)) {
      if (fs.eof()) break;

      if (Line.find("State") == Line.npos) {
        continue;
      }

      char State{};
      if (sscanf(Line.c_str(), "State: %c", &State) == 1) {
        if (State == 'S') {
          return;
        }
        break;
      }
    }
  }
}
void RunTest(uint32_t FlagLocation, uint32_t FlagValue, TestHandler Test) {
  // Block SIGTERM.
  sigset_t BlockMask{};
  sigemptyset(&BlockMask);
  sigaddset(&BlockMask, SIGTERM);
  sigprocmask(SIG_BLOCK, &BlockMask, nullptr);
  std::atomic<uint32_t> Mutex{};
  std::atomic<uint32_t> ReadyNotify{};
  std::atomic<uint32_t> ThreadID{};

  std::thread t(ThreadHandler, &Mutex, &ReadyNotify, &ThreadID, Test);

  while (ReadyNotify.load() == 0);
  // Wait for thread to get in to the futex.
  WaitForThreadAsleep(ThreadID.load());

  tgkill(::getpid(), ThreadID.load(), SIGTERM);

  t.join();

  REQUIRE(from_handler.has_value());
  CHECK(from_handler.value().Signal == SIGTERM);
  CHECK(((from_handler.value().eflags >> FlagLocation) & 1) == FlagValue);
  from_handler.reset();
}

TEST_CASE("Signal-Flags-CF-0") {
  RunTest(EFL_CF, 0, ClearCFAndWait);
}
TEST_CASE("Signal-Flags-CF-1") {
  RunTest(EFL_CF, 1, SetCFAndWait);
}
TEST_CASE("Signal-Flags-PF-0") {
  RunTest(EFL_PF, 0, ClearPFAndWait);
}
TEST_CASE("Signal-Flags-PF-1") {
  RunTest(EFL_PF, 1, SetPFAndWait);
}
TEST_CASE("Signal-Flags-ZF-0") {
  RunTest(EFL_ZF, 0, ClearZFAndWait);
}
TEST_CASE("Signal-Flags-ZF-1") {
  RunTest(EFL_ZF, 1, SetZFAndWait);
}
TEST_CASE("Signal-Flags-SF-0") {
  RunTest(EFL_SF, 0, ClearSFAndWait);
}
TEST_CASE("Signal-Flags-SF-1") {
  RunTest(EFL_SF, 1, SetSFAndWait);
}
TEST_CASE("Signal-Flags-OF-0") {
  RunTest(EFL_OF, 0, ClearOFAndWait);
}
TEST_CASE("Signal-Flags-OF-1") {
  RunTest(EFL_OF, 1, SetOFAndWait);
}
