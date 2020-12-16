#pragma once
#include <stdint.h>
#include <sys/types.h>

namespace FEXCore::Core {
struct InternalThreadState;
struct CPUState;
}

namespace FEX::HLE {
  FEXCore::Core::InternalThreadState *CreateNewThread(FEXCore::Core::InternalThreadState *Thread, uint32_t flags, void *stack, pid_t *parent_tid, pid_t *child_tid, void *tls);
  uint64_t ForkGuest(FEXCore::Core::InternalThreadState *Thread, uint32_t flags, void *stack, pid_t *parent_tid, pid_t *child_tid, void *tls);
}
