#pragma once

#include <FEXCore/HLE/Linux/ThreadManagement.h>
#include <FEXCore/Utils/CompilerDefs.h>

#include <atomic>
#include <cstddef>
#include <stdint.h>
#include <string_view>

namespace FEXCore::Core {
  struct FEX_PACKED CPUState {
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
    uint16_t FTW;
  };
  static_assert(offsetof(CPUState, xmm) % 16 == 0, "xmm needs to be 128bit aligned!");

  struct InternalThreadState;

  enum FallbackHandlerIndex {
    OPINDEX_F80LOADFCW = 0,
    OPINDEX_F80CVTTO_4,
    OPINDEX_F80CVTTO_8,
    OPINDEX_F80CVT_4,
    OPINDEX_F80CVT_8,
    OPINDEX_F80CVTINT_2,
    OPINDEX_F80CVTINT_4,
    OPINDEX_F80CVTINT_8,
    OPINDEX_F80CVTINT_TRUNC2,
    OPINDEX_F80CVTINT_TRUNC4,
    OPINDEX_F80CVTINT_TRUNC8,
    OPINDEX_F80CMP_0,
    OPINDEX_F80CMP_1,
    OPINDEX_F80CMP_2,
    OPINDEX_F80CMP_3,
    OPINDEX_F80CMP_4,
    OPINDEX_F80CMP_5,
    OPINDEX_F80CMP_6,
    OPINDEX_F80CMP_7,
    OPINDEX_F80CVTTOINT_2,
    OPINDEX_F80CVTTOINT_4,

    // Unary
    OPINDEX_F80ROUND,
    OPINDEX_F80F2XM1,
    OPINDEX_F80TAN,
    OPINDEX_F80SQRT,
    OPINDEX_F80SIN,
    OPINDEX_F80COS,
    OPINDEX_F80XTRACT_EXP,
    OPINDEX_F80XTRACT_SIG,
    OPINDEX_F80BCDSTORE,
    OPINDEX_F80BCDLOAD,

    // Binary
    OPINDEX_F80ADD,
    OPINDEX_F80SUB,
    OPINDEX_F80MUL,
    OPINDEX_F80DIV,
    OPINDEX_F80FYL2X,
    OPINDEX_F80ATAN,
    OPINDEX_F80FPREM1,
    OPINDEX_F80FPREM,
    OPINDEX_F80SCALE,

    // Double Precision
    OPINDEX_F64SIN,
    OPINDEX_F64COS,
    OPINDEX_F64TAN,
    OPINDEX_F64ATAN,
    OPINDEX_F64F2XM1,
    OPINDEX_F64FYL2X,
    OPINDEX_F64FPREM,
    OPINDEX_F64FPREM1,
    OPINDEX_F64SCALE,
    // Maximum
    OPINDEX_MAX,
  };

  union JITPointers {
    struct {
      // Process specific
      uint64_t LUDIV{};
      uint64_t LDIV{};
      uint64_t LUREM{};
      uint64_t LREM{};
      uint64_t PrintValue{};
      uint64_t PrintVectorValue{};
      uint64_t RemoveThreadCodeEntryFromJIT{};
      uint64_t CPUIDObj{};
      uint64_t CPUIDFunction{};
      uint64_t SyscallHandlerObj{};
      uint64_t SyscallHandlerFunc{};

      uint64_t FallbackHandlerPointers[FallbackHandlerIndex::OPINDEX_MAX];

      // Thread Specific

      /**
       * @name Dispatcher pointers
       * @{ */
      uint64_t DispatcherLoopTop{};
      uint64_t DispatcherLoopTopFillSRA{};
      uint64_t ThreadStopHandlerSpillSRA{};
      uint64_t ThreadPauseHandlerSpillSRA{};
      uint64_t UnimplementedInstructionHandler{};
      uint64_t OverflowExceptionHandler{};
      uint64_t SignalReturnHandler{};
      uint64_t L1Pointer{};
      uint64_t LUDIVHandler{};
      uint64_t LDIVHandler{};
      uint64_t LUREMHandler{};
      uint64_t LREMHandler{};
      /**  @} */
    } AArch64;

    struct {
      // Process specific
      uint64_t PrintValue{};
      uint64_t PrintVectorValue{};
      uint64_t RemoveThreadCodeEntryFromJIT{};
      uint64_t CPUIDObj{};
      uint64_t CPUIDFunction{};
      uint64_t SyscallHandlerObj{};
      uint64_t SyscallHandlerFunc{};

      uint64_t FallbackHandlerPointers[FallbackHandlerIndex::OPINDEX_MAX];

      // Thread Specific

      /**
       * @name Dispatcher pointers
       * @{ */
      uint64_t DispatcherLoopTop{};
      uint64_t DispatcherLoopTopFillSRA{};
      uint64_t ThreadStopHandler{};
      uint64_t ThreadPauseHandler{};
      uint64_t UnimplementedInstructionHandler{};
      uint64_t OverflowExceptionHandler{};
      uint64_t SignalReturnHandler{};
      uint64_t L1Pointer{};
      /**  @} */
    } X86;
  };

  // Each guest JIT frame has one of these
  struct CpuStateFrame {
    CPUState State;

    /**
     * @brief Stack location for the CPU backends to return the stack pointer to
     *
     * Allows the CPU cores to do a long jump out of their execution and safely shut down
     */
    uint64_t ReturningStackLocation{};

    /**
     * @brief If we are in an inline syscall we need to store a bit of additional information about this
     *
     * ARM64:
     *  - Bit 15: In syscall
     *  - Bit 14-0: Number of static registers spilled
    */
    uint64_t InSyscallInfo{};

    uint32_t SignalHandlerRefCounter{};

    InternalThreadState* Thread;

    // Pointers that the JIT needs to load to remove relocations
    JITPointers Pointers;
  };
  static_assert(offsetof(CpuStateFrame, State) == 0, "CPUState must be first member in CpuStateFrame");
  static_assert(offsetof(CpuStateFrame, State.rip) == 0, "rip must be zero offset in CpuStateFrame");
  static_assert(offsetof(CpuStateFrame, Pointers) % 8 == 0, "JITPointers need to be aligned to 8 bytes");
  static_assert(offsetof(CpuStateFrame, Pointers) + sizeof(CpuStateFrame::Pointers) <= 32760, "JITPointers maximum pointer needs to be less than architecture maximum 32768");

  static_assert(std::is_standard_layout<CpuStateFrame>::value, "This needs to be standard layout");

  FEX_DEFAULT_VISIBILITY std::string_view const& GetFlagName(unsigned Flag);
  FEX_DEFAULT_VISIBILITY std::string_view const& GetGRegName(unsigned Reg);
}
