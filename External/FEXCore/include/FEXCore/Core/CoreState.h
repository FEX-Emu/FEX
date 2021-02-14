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
    uint16_t es, cs, ss, ds;
    uint64_t gs;
    uint64_t fs;
    uint8_t flags[48];
    uint64_t : 64; // Ensures mm is aligned
    uint64_t mm[8][2];

    // 32bit x86 state
    struct {
      uint32_t base;
    } gdt[32];
    uint16_t FCW;
  };
  static_assert(offsetof(CPUState, xmm) % 16 == 0, "xmm needs to be 128bit aligned!");

  struct ThreadState {
    CPUState State{};

    struct {
      std::atomic_bool Running {false};
      std::atomic_bool WaitingToStart {false};
    } RunningEvents;

    /**
     * @brief Stack location for the CPU backends to return the stack pointer to
     *
     * Allows the CPU cores to do a long jump out of their execution and safely shut down
     */
    uint64_t ReturningStackLocation{};

    FEXCore::HLE::ThreadManagement ThreadManager;
  };
  static_assert(offsetof(ThreadState, State) == 0, "CPUState must be first member in threadstate");
  static_assert(offsetof(ThreadState, State.rip) == 0, "rip must be zero offset in threadstate");

  static_assert(std::is_standard_layout<ThreadState>::value, "This needs to be standard layout");

#ifdef PAGE_SIZE
  static_assert(PAGE_SIZE == 4096, "FEX only supports 4k pages");
#undef PAGE_SIZE
#endif

  constexpr uint64_t PAGE_SIZE = 4096;

  std::string_view const& GetFlagName(unsigned Flag);
  std::string_view const& GetGRegName(unsigned Reg);
}
