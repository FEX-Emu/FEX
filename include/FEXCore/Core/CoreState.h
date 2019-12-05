#pragma once
#include <FEXCore/HLE/Linux/ThreadManagement.h>
#include <atomic>
#include <cstddef>
#include <stdint.h>
#include <string_view>

namespace FEXCore::Core {
  struct __attribute__((packed)) CPUState {
    uint64_t rip; ///< Current core's RIP. May not be entirely accurate while JIT is active
    uint64_t gregs[16];
    uint64_t : 64;
    uint64_t xmm[16][2];
    uint64_t gs;
    uint64_t fs;
    uint8_t flags[48];
    uint64_t mm[8][2];
  };
  static_assert(offsetof(CPUState, xmm) % 16 == 0, "xmm needs to be 128bit aligned!");

  struct __attribute__((packed)) ThreadState {
    CPUState State{};

    struct {
      std::atomic_bool Running {false};
      std::atomic_bool ShouldStop {false};
      std::atomic_bool ShouldPause {false};
      std::atomic_bool WaitingToStart {false};
    } RunningEvents;

    FEXCore::HLE::ThreadManagement ThreadManager;
  };
  static_assert(offsetof(ThreadState, State) == 0, "CPUState must be first member in threadstate");

  constexpr uint64_t PAGE_SIZE = 4096;

  std::string_view const& GetFlagName(unsigned Flag);
  std::string_view const& GetGRegName(unsigned Reg);
}
