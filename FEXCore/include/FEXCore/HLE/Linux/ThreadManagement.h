// SPDX-License-Identifier: MIT
#pragma once
#include <atomic>
#include <cstdint>

namespace FEXCore::HLE {
// XXX: This should map multiple IDs correctly
// Tracking relationships between thread IDs and such
class ThreadManagement {
public:
  uint64_t GetUID()  const { return UID; }
  uint64_t GetGID()  const { return GID; }
  uint64_t GetEUID() const { return EUID; }
  uint64_t GetEGID() const { return EGID; }
  uint64_t GetTID()  const { return TID; }
  uint64_t GetPID()  const { return PID; }

  uint64_t UID{1000};
  uint64_t GID{1000};
  uint64_t EUID{1000};
  uint64_t EGID{1000};
  std::atomic<uint64_t> TID{1};
  uint64_t PID{1};
  int32_t *set_child_tid{0};
  int32_t *clear_child_tid{0};
  uint64_t parent_tid{0};
  uint64_t robust_list_head{0};
};
}
