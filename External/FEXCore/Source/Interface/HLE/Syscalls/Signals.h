#pragma once
#include <stdint.h>
#include <signal.h>

namespace FEXCore::Core {
struct InternalThreadState;
}

namespace FEXCore::HLE {
  uint64_t RT_Sigaction(FEXCore::Core::InternalThreadState *Thread, int signum, const struct sigaction *act, struct sigaction *oldact);
  uint64_t RT_Sigprocmask(FEXCore::Core::InternalThreadState *Thread, int how, const sigset_t *set, sigset_t *oldset);
  uint64_t Sigaltstack(FEXCore::Core::InternalThreadState *Thread, const stack_t *ss, stack_t *old_ss);
}
