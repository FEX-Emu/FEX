// SPDX-License-Identifier: MIT
#pragma once

#include <FEXCore/Core/X86Enums.h>
#include <FEXCore/IR/IR.h>
#include <FEXCore/Utils/CompilerDefs.h>
#include <FEXCore/Utils/Telemetry.h>

#include <atomic>
#include <cstddef>
#include <cstring>
#include <stdint.h>
#include <string_view>
#include <type_traits>

namespace FEXCore::Core {
// Wrapper around std::atomic using std::memory_order_relaxed.
// This allows compilers to emit more performant code at the expense of visibly tearing.
// In particular, increments/decrements may visibly tear if a signal is received half-way through.
//
// Prefer std::atomic with default memory ordering unless you really know what you're doing.
// Primarily this ensure program ordering when signals are concerned.
template<typename T>
class NonAtomicRefCounter {
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
    auto Current = AtomicVariable.load(std::memory_order_relaxed);
    AtomicVariable.store(Current + Value, std::memory_order_relaxed);
  }

  // Returns original value.
  // x86-64 needs to know the result on decrement.
  T Decrement(T Value) {
    // Specifically avoiding fetch_sub here because that will turn into ldxr+stxr or lock xadd.
    // FEX very specifically wants to use simple loadstore instructions for this
    //
    // ARM64 ex:
    // ldr x0, [x1];
    // sub x0, x0, #1;
    // str x0, [x1];
    //
    // x86-64 ex:
    // dec qword [rax];
    auto Current = AtomicVariable.load(std::memory_order_relaxed);
    AtomicVariable.store(Current - Value, std::memory_order_relaxed);
    return Current;
  }

  T Load() const {
    return AtomicVariable.load(std::memory_order_relaxed);
  }

  void Store(T Value) {
    AtomicVariable.store(Value, std::memory_order_relaxed);
  }

private:
  std::atomic<T> AtomicVariable;
};
static_assert(std::is_standard_layout_v<NonAtomicRefCounter<uint64_t>>, "Needs to be standard layout");
static_assert(std::is_trivially_copyable_v<NonAtomicRefCounter<uint64_t>>, "needs to be trivially copyable");
static_assert(sizeof(NonAtomicRefCounter<uint64_t>) == sizeof(uint64_t), "Needs to be correct size");

struct alignas(64) CPUState {
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

  // Cacheline: 0
  uint64_t InlineJITBlockHeader {};
  // Reference counter for FEX's per-thread deferred signals.
  // Counts the nesting depth of program sections that cause signals to be deferred.
  NonAtomicRefCounter<uint64_t> DeferredSignalRefCount;

  // PF/AF raw values. Really only a byte of each matters, but this layout
  // (32-bits and in the first 256 bytes) is necessary to use ldp/stp to
  // spill/fill these togethers efficiently.
  // pf_raw must be initialized to 1 so that reconstructed PF = 0 (matching x86 reset state).
  // PF reconstruction: popcount(pf_raw ^ 1) & 1, so pf_raw=1 gives PF=0.
  uint32_t pf_raw {1};
  uint32_t af_raw {};

  uint64_t rip {}; ///< Current core's RIP. May not be entirely accurate while JIT is active

  uint64_t gregs[16] {};
  uint64_t L1Pointer {};
  uint64_t L1Mask {};
  uint64_t callret_sp {};
  uint64_t _pad1 {};

  // Cacheline: 1,2,3,4
  // The high 128-bits of AVX registers when not being emulated by SVE256.
  uint64_t avx_high[16][2];

  // Cacheline: 5-12
  XMMRegs xmm {};

  // Cacheline: 13 and onwards.
  // Raw segment register indexes
  uint16_t es_idx {}, cs_idx {}, ss_idx {}, ds_idx {};
  uint16_t gs_idx {}, fs_idx {};
  uint32_t mxcsr {};

  // Segment registers holding base addresses
  uint32_t es_cached {}, cs_cached {}, ss_cached {}, ds_cached {};
  uint64_t gs_cached {};
  uint64_t fs_cached {};
  uint8_t flags[48] {};
  uint64_t mm[8][2] {};

  // 32bit x86 state
  struct gdt_segment {
    uint16_t Limit0;
    uint16_t Base0;
    uint16_t Base1  : 8;
    uint16_t Type   : 4;
    uint16_t S      : 1;
    uint16_t DPL    : 2;
    uint16_t P      : 1;
    uint16_t Limit1 : 4;
    uint16_t AVL    : 1;
    uint16_t L      : 1;
    uint16_t D      : 1;
    uint16_t G      : 1;
    uint16_t Base2  : 8;
  };

  // Array of segments (Access offset matches segment selector TI bit)
  // 0 : GDT
  // 1 : LDT
  // Segments are global to the process.
  // GDT segments are only 32-objects in size.
  //   - Kernel allocates a handful of these for various things.
  //   - Three are reserved for user-space to setup TLS segments in
  // LDT segments are entirely controlled by userspace.
  //   - Kernel allocates up to 8192 ldt segments.
  gdt_segment* segment_arrays[2] {};

  static gdt_segment* GetSegmentFromIndex(CPUState& State, uint16_t Selector) {
    auto base = State.segment_arrays[(Selector >> 2) & 1];
    return &base[Selector >> 3];
  }

  static uint32_t CalculateGDTBase(gdt_segment GDT) {
    uint32_t Base {};
    Base |= GDT.Base2 << 24;
    Base |= GDT.Base1 << 16;
    Base |= GDT.Base0;
    return Base;
  }

  static uint32_t CalculateGDTLimit(gdt_segment GDT) {
    uint32_t Limit {};
    Limit |= GDT.Limit1 << 16;
    Limit |= GDT.Limit0;
    return Limit;
  }

  static void SetGDTBase(gdt_segment* GDT, uint32_t Base) {
    GDT->Base0 = Base;
    GDT->Base1 = Base >> 16;
    GDT->Base2 = Base >> 24;
  }

  static void SetGDTLimit(gdt_segment* GDT, uint32_t Limit) {
    GDT->Limit0 = Limit;
    GDT->Limit1 = Limit >> 16;
  }

  uint16_t FCW {0x37F};
  uint8_t AbridgedFTW {};

  uint8_t _pad2[5];
  // PF/AF are statically mapped as-if they were r16/r17 (which do not exist in
  // x86 otherwise). This allows a straightforward mapping for SRA.
  static constexpr uint8_t PF_AS_GREG = 16;
  static constexpr uint8_t AF_AS_GREG = 17;

  static constexpr size_t FLAG_SIZE = sizeof(flags[0]);
  static constexpr size_t GDT_SIZE = sizeof(gdt_segment);
  static_assert(GDT_SIZE == sizeof(uint64_t), "Segments required to be 8-byte in size.");
  static constexpr size_t GPR_REG_SIZE = sizeof(gregs[0]);
  static constexpr size_t XMM_AVX_REG_SIZE = sizeof(xmm.avx.data[0]);
  static constexpr size_t XMM_SSE_REG_SIZE = XMM_AVX_REG_SIZE / 2;
  static constexpr size_t MM_REG_SIZE = sizeof(mm[0]);

  // Only the first 32 bits are defined.
  static constexpr size_t NUM_EFLAG_BITS = 32;
  static constexpr size_t NUM_FLAGS = sizeof(flags) / FLAG_SIZE;
  static constexpr size_t NUM_GPRS = sizeof(gregs) / GPR_REG_SIZE;
  static constexpr size_t NUM_XMMS = sizeof(xmm) / XMM_AVX_REG_SIZE;
  static constexpr size_t NUM_MMS = sizeof(mm) / MM_REG_SIZE;
  CPUState() {
#ifndef NDEBUG
    // Initialize default CPU state
    rip = ~0ULL;
    // Initialize xmm state with garbage to catch spurious incorrect xmm usage.
    for (auto& xmm : xmm.avx.data) {
      xmm[0] = 0xDEADBEEFULL;
      xmm[1] = 0xBAD0DAD1ULL;
      xmm[2] = 0xDEADCAFEULL;
      xmm[3] = 0xBAD2CAD3ULL;
    }
#endif

    flags[X86State::RFLAG_RESERVED_LOC] = 1; ///< Reserved - Always 1.
    flags[X86State::RFLAG_IF_LOC] = 1;       ///< Interrupt flag - Always 1.

    // DF needs to be initialized to 0 to comply with the Linux ABI. However,
    // we encode DF as 1/-1 within the JIT, so we have to write 0x1 here to
    // zero DF.
    flags[X86State::RFLAG_DF_RAW_LOC] = 0x1;

    // Likewise, SF/ZF/CF/OF must be cleared. This would be simply zeroing
    // NZCV... but we invert CF inside the JIT. So set just bit 29 (carry).
    flags[X86State::RFLAG_NZCV_3_LOC] = (1 << (29 - 24));

    // Default mxcsr value
    // All exception masks enabled.
    mxcsr = 0x1F80;
  }

  // TODO: This should be moved to the frontend.
  constexpr static uint32_t DEFAULT_USER_CS = 6;

  // Follows encoding of the TI bit in segment selector encoding.
  constexpr static uint32_t SEGMENT_ARRAY_INDEX_GDT = 0;
  constexpr static uint32_t SEGMENT_ARRAY_INDEX_LDT = 1;
  Core::CPUState::gdt_segment private_gdt[32] {};
};
static_assert(std::is_trivially_copyable_v<CPUState>, "Needs to be trivial");
static_assert(std::is_standard_layout_v<CPUState>, "This needs to be standard layout");
static_assert(alignof(CPUState) == 64, "CPUState needs to be 64-byte aligned!");
static_assert(offsetof(CPUState, avx_high) % 64 == 0, "avx_high needs to be 64-byte aligned!");
static_assert(offsetof(CPUState, xmm) % 32 == 0, "xmm needs to be 256-bit aligned!");
static_assert(offsetof(CPUState, mm) % 16 == 0, "mm needs to be 128-bit aligned!");
static_assert(offsetof(CPUState, gregs[15]) <= 504, "gregs maximum offset must be <= 504 for ldp/stp to work");
static_assert(offsetof(CPUState, DeferredSignalRefCount) % 8 == 0, "Needs to be 8-byte aligned");
static_assert(offsetof(CPUState, L1Pointer) <= 504, "This needs to be <= 504 for ldp");
static_assert(offsetof(CPUState, L1Mask) == (offsetof(CPUState, L1Pointer) + 8), "These two variables are paired");
static_assert(offsetof(CPUState, pf_raw) <= 252, "pf_raw must be within ldp imm offset range");
static_assert((offsetof(CPUState, pf_raw) + 4) == offsetof(CPUState, af_raw), "pf_raw and af_raw must be sequential");

// Some CPU architectures have a penalty for alignment of ldp/stp not being 2 * <element_size>.
static_assert(offsetof(CPUState, gregs[0]) % 16 == 0, "gregs should be 16-byte aligned");
static_assert(offsetof(CPUState, pf_raw) % 8 == 0, "pf_raw must be 8-byte aligned.");

struct InternalThreadState;

enum FallbackHandlerIndex {
  OPINDEX_F80CVTTO_4 = 0,
  OPINDEX_F80CVTTO_8,
  OPINDEX_F80CVT_4,
  OPINDEX_F80CVT_8,
  OPINDEX_F80CVTINT_2,
  OPINDEX_F80CVTINT_4,
  OPINDEX_F80CVTINT_8,
  OPINDEX_F80CVTINT_TRUNC2,
  OPINDEX_F80CVTINT_TRUNC4,
  OPINDEX_F80CVTINT_TRUNC8,
  OPINDEX_F80CMP,
  OPINDEX_F80CVTTOINT_2,
  OPINDEX_F80CVTTOINT_4,

  // Unary
  OPINDEX_F80ROUND,
  OPINDEX_F80F2XM1,
  OPINDEX_F80TAN,
  OPINDEX_F80SQRT,
  OPINDEX_F80SIN,
  OPINDEX_F80COS,
  OPINDEX_F80SINCOS,
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
  OPINDEX_F64SINCOS,
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

struct FallbackABIInfo {
  uint64_t ABIHandler;
  uint64_t Func;
};

struct JITPointers {

  // Process specific
  uint64_t PrintValue {};
  uint64_t PrintVectorValue {};
  uint64_t ThreadRemoveCodeEntryFromJIT {};
  uint64_t CPUIDObj {};
  uint64_t CPUIDFunction {};
  uint64_t XCRFunction {};
  uint64_t SyscallHandlerObj {};
  uint64_t SyscallHandlerFunc {};
  uint64_t ExitFunctionLink {};
  uint64_t MonoBackpatcherWrite {};
  uint64_t LUDIV {};
  uint64_t LDIV {};
  uint64_t ThunkCallbackRet {};

  // Handles returning/calling ARM64EC code from the JIT, expects the target PC in TMP3
  uint64_t ExitFunctionEC {};

  FallbackABIInfo FallbackHandlerPointers[FallbackHandlerIndex::OPINDEX_MAX];
  uint64_t NamedVectorConstantPointers[FEXCore::IR::NamedVectorConstant::NAMED_VECTOR_CONST_POOL_MAX];
  uint64_t IndexedNamedVectorConstantPointers[FEXCore::IR::IndexNamedVectorConstant::INDEXED_NAMED_VECTOR_MAX];
  uint64_t TelemetryValueAddresses[FEXCore::Telemetry::TYPE_LAST];

  /**
   * @name Dispatcher pointers
   * @{ */
  uint64_t DispatcherLoopTop {};
  uint64_t DispatcherLoopTopFillSRA {};
  uint64_t DispatcherLoopTopEnterEC {};
  uint64_t DispatcherLoopTopEnterECFillSRA {};
  uint64_t ExitFunctionLinker {};
  uint64_t ThreadStopHandlerSpillSRA {};
  uint64_t ThreadPauseHandlerSpillSRA {};
  uint64_t GuestSignal_SIGILL {};
  uint64_t GuestSignal_SIGTRAP {};
  uint64_t GuestSignal_SIGSEGV {};
  uint64_t SignalReturnHandler {};
  uint64_t SignalReturnHandlerRT {};
  uint64_t L2Pointer {};
  uint64_t LUDIVHandler {};
  uint64_t LDIVHandler {};
  /**  @} */

  // Copy of process-wide named vector constants data.
  alignas(16) uint64_t NamedVectorConstants[FEXCore::IR::NamedVectorConstant::NAMED_VECTOR_CONST_POOL_MAX][2];
};

// Each guest JIT frame has one of these
struct CpuStateFrame {
  CPUState State;

  /**
   * @brief Stack location for the CPU backends to return the stack pointer to
   *
   * Allows the CPU cores to do a long jump out of their execution and safely shut down
   */
  uint64_t ReturningStackLocation {};

  /**
   * @brief If we are in an inline syscall we need to store a bit of additional information about this
   *
   * ARM64:
   *  - Bit 15: In syscall
   *  - Bit 14-0: Number of static registers spilled
   */
  uint64_t InSyscallInfo {};

  uint32_t SignalHandlerRefCounter {};

  struct alignas(8) SynchronousFaultDataStruct {
    bool FaultToTopAndGeneratedException {};
    uint8_t Signal;
    uint8_t TrapNo;
    uint8_t si_code;
    uint16_t err_code;
    uint16_t _pad : 16;
  } SynchronousFaultData;

  InternalThreadState* Thread;

#ifdef ARCHITECTURE_arm64ec
  // Set by the kernel on ARM64EC whenever the JIT should cooperatively suspend running guest code.
  uint32_t SuspendDoorbell {};
#endif

  // Pointers that the JIT needs to load to remove relocations
  JITPointers Pointers;
};
static_assert(offsetof(CpuStateFrame, State) == 0, "CPUState must be first member in CpuStateFrame");
static_assert(offsetof(CpuStateFrame, Pointers) % 8 == 0, "JITPointers need to be aligned to 8 bytes");
static_assert(offsetof(CpuStateFrame, Pointers) + sizeof(CpuStateFrame::Pointers) <= 32760, "JITPointers maximum pointer needs to be less "
                                                                                            "than architecture maximum 32768");

static_assert(std::is_standard_layout<CpuStateFrame>::value, "This needs to be standard layout");
static_assert(sizeof(CpuStateFrame::SynchronousFaultData) == 8, "This needs to be 8 bytes");
static_assert(alignof(CpuStateFrame::SynchronousFaultDataStruct) == 8, "This needs to be 8 bytes");
static_assert(offsetof(CpuStateFrame, SynchronousFaultData) % 8 == 0, "This needs to be aligned");
} // namespace FEXCore::Core
