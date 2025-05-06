// SPDX-License-Identifier: MIT
#include "LinuxSyscalls/Utils/Threads.h"
#include "LinuxSyscalls/Syscalls.h"

#include <FEXCore/Core/Context.h>
#include <FEXCore/Utils/Threads.h>

namespace FEX::LinuxEmulation::Threads {
void* StackTracker::AllocateStackObject() {
  std::lock_guard lk {DeadStackPoolMutex};
  // Keep the first item in the stack pool
  void* Ptr {};

  for (auto it = DeadStackPool.begin(); it != DeadStackPool.end();) {
    auto Ready = std::atomic_ref<bool>(it->ReadyToBeReaped);
    bool ReadyToBeReaped = Ready.load();
    if (Ptr == nullptr && ReadyToBeReaped) {
      Ptr = it->Ptr;
      it = DeadStackPool.erase(it);
      continue;
    }

    if (ReadyToBeReaped) {
      FEXCore::Allocator::munmap(it->Ptr, it->Size);
      it = DeadStackPool.erase(it);
      continue;
    }

    ++it;
  }

  if (Ptr == nullptr) {
    Ptr = FEXCore::Allocator::mmap(nullptr, FEX::LinuxEmulation::Threads::STACK_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  }

  return Ptr;
}

bool* StackTracker::AddStackToDeadPool(void* Ptr) {
  std::lock_guard lk {DeadStackPoolMutex};
  auto& it = DeadStackPool.emplace_back(DeadStackPoolItem {Ptr, FEX::LinuxEmulation::Threads::STACK_SIZE, false});
  return &it.ReadyToBeReaped;
}

void StackTracker::AddStackToLivePool(void* Ptr) {
  std::lock_guard lk {LiveStackPoolMutex};
  LiveStackPool.emplace_back(StackPoolItem {Ptr, FEX::LinuxEmulation::Threads::STACK_SIZE});
}

void StackTracker::RemoveStackFromLivePool(void* Ptr) {
  std::lock_guard lk {LiveStackPoolMutex};
  for (auto it = LiveStackPool.begin(); it != LiveStackPool.end(); ++it) {
    if (it->Ptr == Ptr) {
      LiveStackPool.erase(it);
      return;
    }
  }
}

void StackTracker::CleanupAfterFork_PThread() {
  // We don't need to pull the mutex here
  // After a fork we are the only thread running
  // Just need to make sure not to delete our own stack
  uintptr_t StackLocation = reinterpret_cast<uintptr_t>(alloca(0));

  auto ClearStackPool = [StackLocation](auto& StackPool) {
    for (auto it = StackPool.begin(); it != StackPool.end();) {
      auto& Item = *it;
      uintptr_t ItemStack = reinterpret_cast<uintptr_t>(Item.Ptr);
      if (ItemStack <= StackLocation && (ItemStack + Item.Size) > StackLocation) {
        // This is our stack item, skip it
        ++it;
      } else {
        // Untracked stack. Clean it up
        FEXCore::Allocator::munmap(Item.Ptr, Item.Size);
        it = StackPool.erase(it);
      }
    }
  };

  // Clear both dead stacks and live stacks
  ClearStackPool(DeadStackPool);
  ClearStackPool(LiveStackPool);

  LogMan::Throw::AFmt((DeadStackPool.size() + LiveStackPool.size()) <= 1, "After fork we should only have zero or one tracked stacks!");
}

void StackTracker::Shutdown() {
  std::lock_guard lk {DeadStackPoolMutex};
  std::lock_guard lk2 {LiveStackPoolMutex};
  // Erase all the dead stack pools
  for (auto& Item : DeadStackPool) {
    FEXCore::Allocator::munmap(Item.Ptr, Item.Size);
  }

  // Now clean up any that are considered to still be live
  // We are in shutdown phase, everything in the process is dead
  for (auto& Item : LiveStackPool) {
    FEXCore::Allocator::munmap(Item.Ptr, Item.Size);
  }

  DeadStackPool.clear();
  LiveStackPool.clear();
}

void StackTracker::DeallocateStackObjectImmediately(void* Ptr) {
  if (Ptr) {
    RemoveStackFromLivePool(Ptr);
    auto ReadyToBeReaped = AddStackToDeadPool(Ptr);
    *ReadyToBeReaped = true;
  }
}

[[noreturn]]
void StackTracker::DeallocateStackObjectAndExit(void* Ptr, int Status) {
  if (Ptr) {
    RemoveStackFromLivePool(Ptr);
    auto ReadyToBeReaped = AddStackToDeadPool(Ptr);
    *ReadyToBeReaped = true;
  }

#ifdef _M_ARM_64
  __asm volatile("mov x8, %[SyscallNum];"
                 "mov w0, %w[Result];"
                 "svc #0;" ::[SyscallNum] "i"(SYSCALL_DEF(exit)),
                 [Result] "r"(Status)
                 : "memory", "x0", "x8");
#else
  __asm volatile("mov %[Result], %%edi;"
                 "syscall;" ::"a"(SYSCALL_DEF(exit)),
                 [Result] "r"(Status)
                 : "memory", "rdi");
#endif
  FEX_UNREACHABLE;
}

#ifdef _M_ARM_64
__attribute__((naked)) void StackPivotAndCall(void* Arg, FEXCore::Threads::ThreadFunc Func, uint64_t StackPivot) {
  // x0: Arg
  // x1: Function to call
  // x2: StackPivot
  __asm volatile(R"(
    // Stack pivot.
    mov x3, sp;
    mov sp, x2;

    // Store stack storage location on to current stack
    stp x3, lr, [sp, -16]!;

    // x0 already has argument to pass.
    blr x1

    // Reload stack storage location
    ldp x2, lr, [sp], 16;

    // Stack pivot back
    mov sp, x2;

    ret;
    )" ::
                   : "memory");
}
#else
__attribute__((naked)) void StackPivotAndCall(void* Arg, FEXCore::Threads::ThreadFunc Func, uint64_t StackPivot) {
  // rdi: Arg
  // rsi: Function to call
  // rdx: StackPivot
  __asm volatile(R"(
    // Copy original stack in to RSP.
    movq %%rsp, %%rcx;

    // Store original stack on new stack
    pushq %%rcx;

    // Store stack pivot on new stack.
    pushq %%rdx;

    // rdi already contains function argument.
    callq *%%rsi;

    // Restore original stack
    popq %%rsp;

    ret;

    )" ::
                   : "memory");
}
#endif
namespace PThreads {
  namespace LongJump {
    // This is a custom long jump implementation that avoids the glibc implementation.
    // This is required behaviour because glibc's fortification checks don't understand stack pivots.
    // FEX requires a stack pivot to work through a long jump, so these two features are at odds with each other.
#ifdef _M_ARM_64
    struct JumpBuf {
      // All the registers that are required by AAPCS64 to save.
      // GPRs
      // X19, X20, X21, X22,
      // X23, X24, X25, X26,
      // X27, X28, X29, X30,
      //
      // Lower 64-bits:
      //  V8,  V9, V10, V11,
      // V12, V13, V14, V15,
      //
      // SP,
      uint64_t Registers[21];
    };
    FEX_NAKED uint64_t SetJump(JumpBuf& Buffer) {
      __asm volatile(R"(
        // x0 contains the jumpbuffer
        stp x19, x20, [x0, #( 0 * 8)];
        stp x21, x22, [x0, #( 2 * 8)];
        stp x23, x24, [x0, #( 4 * 8)];
        stp x25, x26, [x0, #( 6 * 8)];
        stp x27, x28, [x0, #( 8 * 8)];
        stp x29, x30, [x0, #(10 * 8)];

        // FPRs
        stp d8,   d9, [x0, #(12 * 8)];
        stp d10, d11, [x0, #(14 * 8)];
        stp d12, d13, [x0, #(16 * 8)];
        stp d14, d15, [x0, #(18 * 8)];

        // Move SP in to a temporary to store.
        mov x1, sp;
        str x1,  [x0, #(19 * 8)];

        // Return zero to signify this is the SetJump.
        mov x0, #0;
        ret;
      )" ::
                       : "memory");
    }

    [[noreturn]]
    FEX_NAKED void LongJump(JumpBuf& Buffer, uint64_t Value) {
      __asm volatile(R"(
        // x0 contains the jumpbuffer
        ldp x19, x20, [x0, #( 0 * 8)];
        ldp x21, x22, [x0, #( 2 * 8)];
        ldp x23, x24, [x0, #( 4 * 8)];
        ldp x25, x26, [x0, #( 6 * 8)];
        ldp x27, x28, [x0, #( 8 * 8)];
        ldp x29, x30, [x0, #(10 * 8)];

        // FPRs
        ldp d8,   d9, [x0, #(12 * 8)];
        ldp d10, d11, [x0, #(14 * 8)];
        ldp d12, d13, [x0, #(16 * 8)];
        ldp d14, d15, [x0, #(18 * 8)];

        // Load SP in to temporary then move
        ldr x0,  [x0, #(19 * 8)];
        mov sp, x0;

        // Move value in to result register
        mov x0, x1;
        ret;
      )" ::
                       : "memory");
    }
#else
    struct JumpBuf {
      // Registers to preserve
      // RBX, RSP, RBP, R12, R13, R14, R15,
      // <return address>
      uint64_t Registers[8];
    };

    __attribute__((naked)) uint64_t SetJump(JumpBuf& Buffer) {
      __asm volatile(R"(
      .intel_syntax noprefix;
      // rdi contains the jumpbuffer
      mov [rdi + (0 * 8)], rbx;
      mov [rdi + (1 * 8)], rsp;
      mov [rdi + (2 * 8)], rbp;
      mov [rdi + (3 * 8)], r12;
      mov [rdi + (4 * 8)], r13;
      mov [rdi + (5 * 8)], r14;
      mov [rdi + (6 * 8)], r15;

      // Return address is on the stack, load it and store
      mov rsi, [rsp];
      mov [rdi + (7 * 8)], rsi;

      // Return zero to signify this is the SetJump.
      mov rax, 0;
      ret;

      .att_syntax prefix;
      )" ::
                       : "memory");
    }

    [[noreturn]]
    __attribute__((naked)) void LongJump(JumpBuf& Buffer, uint64_t Value) {
      __asm volatile(R"(
      .intel_syntax noprefix;
      // rdi contains the jumpbuffer
      mov rbx, [rdi + (0 * 8)];
      mov rsp, [rdi + (1 * 8)];
      mov rbp, [rdi + (2 * 8)];
      mov r12, [rdi + (3 * 8)];
      mov r13, [rdi + (4 * 8)];
      mov r14, [rdi + (5 * 8)];
      mov r15, [rdi + (6 * 8)];

      // Move value in to result register
      mov rax, rsi;

      // Pop the dead return address off the stack
      pop rsi;

      // Load the original return address from the jumpbuffer
      mov rsi, [rdi + (7 * 8)];

      // Return using a jump
      jmp rsi;

      .att_syntax prefix;
      )" ::
                       : "memory");
    }
#endif
  }; // namespace LongJump
  void* InitializeThread(void* Ptr);

  class PThread final : public FEXCore::Threads::Thread {
  public:
    PThread(StackTracker* STracker, FEXCore::Threads::ThreadFunc Func, void* Arg)
      : STracker {STracker}
      , UserFunc {Func}
      , UserArg {Arg} {
      pthread_attr_t Attr {};
      Stack = STracker->AllocateStackObject();
      // pthreads allocates its dtv region behind our back and there is nothing we can do about it.
      FEXCore::Allocator::YesIKnowImNotSupposedToUseTheGlibcAllocator glibc;
      STracker->AddStackToLivePool(Stack);
      pthread_attr_init(&Attr);
      // Allocate a minimum size stack through pthreads, then stack pivot to FEX's allocated stack.
      // This is required due to a race condition with pthread's DTV/TLS regions when a stack is reused before pthreads deletes that thread's
      // DTV/TLS regions.
      // This can be seen as a crash when running Steam fairly easily, but is very confusing when debugging.
      // The cause of this race condition is from glibc associating a DTV/TLS region with a stack region until the kernel clears the
      // `set_tid_address` address construct. If the stack is reused before the address is set to zero, then glibc won't initialize the new thread's
      // DTV/TLS region, resulting in TLS usage crashing.
      pthread_attr_setstacksize(&Attr, PTHREAD_STACK_MIN);
      pthread_create(&Thread, &Attr, InitializeThread, this);
      pthread_attr_destroy(&Attr);
    }

    bool joinable() override {
      pthread_attr_t Attr {};
      if (pthread_getattr_np(Thread, &Attr) == 0) {
        int AttachState {};
        if (pthread_attr_getdetachstate(&Attr, &AttachState) == 0) {
          if (AttachState == PTHREAD_CREATE_JOINABLE) {
            pthread_attr_destroy(&Attr);
            return true;
          }
        }
        pthread_attr_destroy(&Attr);
      }
      return false;
    }

    bool join(void** ret) override {
      return pthread_join(Thread, ret) == 0;
    }

    bool detach() override {
      return pthread_detach(Thread) == 0;
    }

    bool IsSelf() override {
      auto self = pthread_self();
      return self == Thread;
    }

    FEXCore::Threads::ThreadFunc GetUserFunc() const {
      return UserFunc;
    }

    void* GetUserArg() const {
      return UserArg;
    }

    void* GetPivotStack() const {
      return Stack;
    }

    StackTracker* GetStackTracker() const {
      return STracker;
    }

    void SetupLongJump(LongJump::JumpBuf* exit_resolver) {
      _exit_resolver = exit_resolver;
    }

    [[noreturn]]
    void LongJumpExit(FEX::HLE::ThreadStateObject* ThreadObject, uint32_t Status) {
      this->Status = Status;
      this->ThreadObject = ThreadObject;
      LongJump::LongJump(*_exit_resolver, 1);
      FEX_UNREACHABLE;
    }

    uint32_t GetStatus() const {
      return Status;
    }

    FEX::HLE::ThreadStateObject* GetThreadObject() const {
      return ThreadObject;
    }

  private:
    pthread_t Thread;
    StackTracker* STracker;
    FEXCore::Threads::ThreadFunc UserFunc;
    void* UserArg;
    void* Stack {};

    LongJump::JumpBuf* _exit_resolver {};
    FEX::HLE::ThreadStateObject* ThreadObject {};
    uint32_t Status {};
  };

  void* InitializeThread(void* Ptr) {
    void* StackBase {};
    StackTracker* STracker {};
    PThread* Thread {reinterpret_cast<PThread*>(Ptr)};
    StackBase = Thread->GetPivotStack();
    STracker = Thread->GetStackTracker();
    LongJump::JumpBuf exit_resolver {};

    bool LongJumpExit {};

    if (LongJump::SetJump(exit_resolver) == 0) {
      Thread->SetupLongJump(&exit_resolver);
      // Run the user function.
      // `Thread` object is dead after this function returns.
      StackPivotAndCall(Thread->GetUserArg(), Thread->GetUserFunc(),
                        reinterpret_cast<uint64_t>(StackBase) + FEX::LinuxEmulation::Threads::STACK_SIZE);
    } else {
      LongJumpExit = true;
    }

    const auto Status = Thread->GetStatus();
    auto ThreadObject = Thread->GetThreadObject();
    // TLS/DTV teardown is something FEX can't control. Disable glibc checking when we leave a pthread.
    FEXCore::Allocator::YesIKnowImNotSupposedToUseTheGlibcAllocator::HardDisable();

    // Detach to ensure thread teardown occurs.
    Thread->detach();

    if (LongJumpExit) {
      // We have ownership of the thread object. Make sure to clean it up to prevent memory leaks.
      FEX::HLE::_SyscallHandler->TM.DestroyThread(ThreadObject, true);
      if (Status == 0) {
        // If status is zero then we can safely deallocate this thread's pivot stack (Which is no longer used).
        STracker->DeallocateStackObjectImmediately(StackBase);
        StackBase = nullptr;
      }
    }

    if (!LongJumpExit || Status != 0) {
      // If we didn't have a long jump exit (So not a pthread thread) OR status wasn't zero then we need to terminate locally.
      // There is an api limitation in glibc/pthreads where a function's return value is ignored and not passed to SYS_exit.
      // In or to match error condition thread exits, we must call SYS_exit ourselves in this case.
      //
      // This is a memory leak if this is a pthread based thread! We can't work around this.
      // - Leaks 128KB PTHREAD_STACK_MIN
      // - Leaks some glibc internal dtv tracking data.
      STracker->DeallocateStackObjectAndExit(StackBase, Status);
      FEX_UNREACHABLE;
    }

    // Give control back to pthreads.
    // This is required so glibc puts this thread's stack back in the stack cache, preventing a memory leak.
    // We can't use pthread_exit since that requires libgcc_s.so unwinder support which might not be available.
    // We are /expecting/ pthreads to return this status to the _exit syscall.
    return (void*)(uint64_t)Status;
  }

  StackTracker* STracker {};

  fextl::unique_ptr<FEXCore::Threads::Thread> CreateThread_PThread(FEXCore::Threads::ThreadFunc Func, void* Arg) {
    return fextl::make_unique<PThread>(STracker, Func, Arg);
  }

  void CleanupAfterFork_PThread() {
    STracker->CleanupAfterFork_PThread();
  }

}; // namespace PThreads

fextl::unique_ptr<StackTracker> SetupThreadHandlers() {
  FEXCore::Threads::Pointers Ptrs = {
    .CreateThread = PThreads::CreateThread_PThread,
    .CleanupAfterFork = PThreads::CleanupAfterFork_PThread,
  };

  FEXCore::Threads::Thread::SetInternalPointers(Ptrs);

  PThreads::STracker = new StackTracker();
  return fextl::unique_ptr<StackTracker>(PThreads::STracker);
}

void* AllocateStackObject() {
  return PThreads::STracker->AllocateStackObject();
}

[[noreturn]]
void DeallocateStackObjectAndExit(void* Ptr, int Status) {
  PThreads::STracker->DeallocateStackObjectAndExit(Ptr, Status);
  FEX_UNREACHABLE;
}

[[noreturn]]
void LongjumpDeallocateAndExit(FEX::HLE::ThreadStateObject* ThreadObject, int Status) {
  auto ThreadObject_P = reinterpret_cast<PThreads::PThread*>(ThreadObject->ExecutionThread.get());
  ThreadObject_P->LongJumpExit(ThreadObject, Status);
  FEX_UNREACHABLE;
}

void* GetStackBase(FEXCore::Threads::Thread* ThreadObject) {
  auto ThreadObject_P = reinterpret_cast<PThreads::PThread*>(ThreadObject);
  return ThreadObject_P->GetPivotStack();
}

void Shutdown(fextl::unique_ptr<StackTracker> STracker) {
  STracker->Shutdown();
  STracker.reset();
  PThreads::STracker = nullptr;
}
} // namespace FEX::LinuxEmulation::Threads
