#pragma once

#include <FEXCore/HLE/Linux/ThreadManagement.h>
#include <FEXCore/Utils/CompilerDefs.h>
#include <FEXCore/Core/CPUBackend.h>

#include <atomic>
#include <cstddef>
#include <stdint.h>
#include <string_view>
#include <type_traits>

namespace FEXCore::Core {
  // This is defined here instead of a helper because it is a massive footgun in behaviour.
  // Don't use this instead of std::atomic unless you really know what you're doing.
  // Increment and decrement can visibily tear and only uses relaxed atomic behaviour internally.
  // In particular decrement and increment can tear if a signal is received half-way through
  // and you need to be aware of that when using this.
  //
  // Primarily this ensure program ordering when signals are concerned.
  //
  // This casts its internal object to std::atomic internally so that compilers can't reorder operations around it.
  // This is necessary with how it is used with deferring signals.
  template<typename T>
  class MoveableNonatomicRefCounter {
    public:
      void Increment(T Value) {
        // Specifically avoiding fetch_add here because that will turn in to ldxr+stxr or lock xadd.
        // FEX very specifically wants to use simple loadstore instructions for this
        //
        // ARM64 ex:
        // ldr x0, [x1];
        // add x0, x0, #1;
        // str x0, [x1];
        //
        // x86-64 ex:
        // inc qword [rax];
        auto AtomicVariable = std::atomic_ref<T>(Variable);
        auto Current = AtomicVariable.load(std::memory_order_relaxed);
        AtomicVariable.store(Current + Value, std::memory_order_relaxed);
      }

      // Returns original value.
      // x86-64 needs to know the result on decrement.
      T Decrement(T Value) {
        // Specifically avoiding fetch_sub here because that will turn in to ldxr+stxr or lock xadd.
        // FEX very specifically wants to use simple loadstore instructions for this
        //
        // ARM64 ex:
        // ldr x0, [x1];
        // sub x0, x0, #1;
        // str x0, [x1];
        //
        // x86-64 ex:
        // dec qword [rax];
        auto AtomicVariable = std::atomic_ref<T>(Variable);
        auto Current = AtomicVariable.load(std::memory_order_relaxed);
        AtomicVariable.store(Current - Value, std::memory_order_relaxed);
        return Current;
      }

      T Load() const {
        auto AtomicVariable = std::atomic_ref<const T>(Variable);
        return AtomicVariable.load(std::memory_order_relaxed);
      }

      void Store(T Value) {
        auto AtomicVariable = std::atomic_ref<T>(Variable);
        AtomicVariable.store(Value, std::memory_order_relaxed);
      }

    private:
      // Internal variable can't be std::atomic because that doesn't have a move constructor and is non-pod.
      T Variable;
  };
  static_assert(std::is_standard_layout_v<MoveableNonatomicRefCounter<uint64_t>>, "Needs to be standard layout");
  static_assert(std::is_trivially_copyable_v<MoveableNonatomicRefCounter<uint64_t>>, "needs to be trivially copyable");
  static_assert(sizeof(MoveableNonatomicRefCounter<uint64_t>) == sizeof(uint64_t), "Needs to be correct size");

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
    uint64_t InlineJITBlockHeader;
    XMMRegs xmm;
    uint8_t flags[48];
    uint64_t mm[8][2];

    // 32bit x86 state
    struct {
      uint32_t base;
    } gdt[32];
    uint16_t FCW;
    uint16_t FTW;

    uint32_t _pad2[1];
    // Reference counter for FEX's per-thread deferred signals.
    MoveableNonatomicRefCounter<uint64_t> DeferredSignalRefCount;
    // Since this memory region is thread local, it should only be accessed with relaxed atomics.
    MoveableNonatomicRefCounter<uint64_t> *DeferredSignalFaultAddress;

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
  static_assert(offsetof(CPUState, DeferredSignalRefCount) % 8 == 0, "Needs to be 8-byte aligned");

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

    // SSE4.2 string instructions
    OPINDEX_VPCMPESTRX,
    OPINDEX_VPCMPISTRX,

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
      uint64_t XCRFunction{};
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
      uint64_t SignalReturnHandler{};
      uint64_t SignalReturnHandlerRT{};
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
      uint16_t _pad : 16;
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
