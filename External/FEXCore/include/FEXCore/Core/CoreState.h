#pragma once

#include <FEXCore/HLE/Linux/ThreadManagement.h>
#include <FEXCore/Utils/CompilerDefs.h>
#include <FEXCore/Core/CPUBackend.h>

#include <atomic>
#include <cstddef>
#include <stdint.h>
#include <string_view>

namespace FEXCore::Core {
  struct FEX_PACKED CPUState {
    // Allows more efficient handling of the register
    // file in the event AVX is not supported.
    union XMMRegs {
      struct AVX {
        uint64_t data[16][4];
      };
      struct SSE {
        uint64_t data[16][2];
        uint64_t pad[16][2];
      };

      AVX avx;
      SSE sse;
    };

    uint64_t rip; ///< Current core's RIP. May not be entirely accurate while JIT is active
    uint64_t gregs[16];
    // Raw segment register indexes
    uint16_t es_idx, cs_idx, ss_idx, ds_idx;
    uint16_t gs_idx, fs_idx;
    uint16_t _pad[2];

    // Segment registers holding base addresses
    uint32_t es_cached, cs_cached, ss_cached, ds_cached;
    uint64_t gs_cached;
    uint64_t fs_cached;
    uint64_t _pad2[1];
    XMMRegs xmm;
    uint8_t flags[48];
    uint64_t mm[8][2];

    // 32bit x86 state
    struct {
      uint32_t base;
    } gdt[32];
    uint16_t FCW;
    uint16_t FTW;

    static constexpr size_t FLAG_SIZE = sizeof(flags[0]);
    static constexpr size_t GDT_SIZE = sizeof(gdt[0]);
    static constexpr size_t GPR_REG_SIZE = sizeof(gregs[0]);
    static constexpr size_t XMM_AVX_REG_SIZE = sizeof(xmm.avx.data[0]);
    static constexpr size_t XMM_SSE_REG_SIZE = XMM_AVX_REG_SIZE / 2;
    static constexpr size_t MM_REG_SIZE = sizeof(mm[0]);

    // Only the first 32 bits are defined.
    static constexpr size_t NUM_EFLAG_BITS = 32;
    static constexpr size_t NUM_FLAGS = sizeof(flags) / FLAG_SIZE;
    static constexpr size_t NUM_GDTS = sizeof(gdt) / GDT_SIZE;
    static constexpr size_t NUM_GPRS = sizeof(gregs) / GPR_REG_SIZE;
    static constexpr size_t NUM_XMMS = sizeof(xmm) / XMM_AVX_REG_SIZE;
    static constexpr size_t NUM_MMS = sizeof(mm) / MM_REG_SIZE;
  };
  static_assert(offsetof(CPUState, xmm) % 32 == 0, "xmm needs to be 256-bit aligned!");
  static_assert(offsetof(CPUState, mm) % 16 == 0, "mm needs to be 128-bit aligned!");

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

  struct JITPointers {

    struct {
      // Process specific

      uint64_t PrintValue{};
      uint64_t PrintVectorValue{};
      uint64_t ThreadRemoveCodeEntryFromJIT{};
      uint64_t CPUIDObj{};
      uint64_t CPUIDFunction{};
      uint64_t SyscallHandlerObj{};
      uint64_t SyscallHandlerFunc{};
      uint64_t ExitFunctionLink{};

      uint64_t FallbackHandlerPointers[FallbackHandlerIndex::OPINDEX_MAX];

      // Thread Specific
      /**
       * @name Dispatcher pointers
       * @{ */
      uint64_t DispatcherLoopTop{};
      uint64_t DispatcherLoopTopFillSRA{};
      uint64_t ExitFunctionLinker{};
      uint64_t ThreadStopHandlerSpillSRA{};
      uint64_t ThreadPauseHandlerSpillSRA{};
      uint64_t UnimplementedInstructionHandler{};
      uint64_t GuestSignal_SIGILL{};
      uint64_t GuestSignal_SIGTRAP{};
      uint64_t GuestSignal_SIGSEGV{};
      uint64_t SignalHandlerReturnAddressRT{};
      uint64_t SignalHandlerReturnAddress{};
      uint64_t L1Pointer{};
      uint64_t L2Pointer{};
      /**  @} */
    } Common;

    union {
      struct {
        // Process specific
        uint64_t LUDIV{};
        uint64_t LDIV{};
        uint64_t LUREM{};
        uint64_t LREM{};

        // Thread Specific

        /**
         * @name Dispatcher pointers
         * @{ */
        uint64_t LUDIVHandler{};
        uint64_t LDIVHandler{};
        uint64_t LUREMHandler{};
        uint64_t LREMHandler{};
        /**  @} */
      } AArch64;

      struct {
        // None so far
      } X86;

      struct {
        uint64_t FragmentExecuter;
        using IntCallbackReturn =  void(*)(FEXCore::Core::InternalThreadState *Thread, volatile void *Host_RSP);
        IntCallbackReturn CallbackReturn;
      } Interpreter;
    };
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

    struct alignas(8) SynchronousFaultDataStruct {
      bool FaultToTopAndGeneratedException{};
      uint8_t Signal;
      uint8_t TrapNo;
      uint8_t si_code;
      uint16_t err_code;
      uint32_t _pad : 16;
    } SynchronousFaultData;

    InternalThreadState* Thread;

    // Pointers that the JIT needs to load to remove relocations
    JITPointers Pointers;
  };
  static_assert(offsetof(CpuStateFrame, State) == 0, "CPUState must be first member in CpuStateFrame");
  static_assert(offsetof(CpuStateFrame, State.rip) == 0, "rip must be zero offset in CpuStateFrame");
  static_assert(offsetof(CpuStateFrame, Pointers) % 8 == 0, "JITPointers need to be aligned to 8 bytes");
  static_assert(offsetof(CpuStateFrame, Pointers) + sizeof(CpuStateFrame::Pointers) <= 32760, "JITPointers maximum pointer needs to be less than architecture maximum 32768");

  static_assert(std::is_standard_layout<CpuStateFrame>::value, "This needs to be standard layout");
  static_assert(sizeof(CpuStateFrame::SynchronousFaultData) == 8, "This needs to be 8 bytes");
  static_assert(std::alignment_of_v<CpuStateFrame::SynchronousFaultDataStruct> == 8, "This needs to be 8 bytes");
  static_assert(offsetof(CpuStateFrame, SynchronousFaultData) % 8 == 0, "This needs to be aligned");

  FEX_DEFAULT_VISIBILITY std::string_view const& GetFlagName(unsigned Flag);
  FEX_DEFAULT_VISIBILITY std::string_view const& GetGRegName(unsigned Reg);
}
