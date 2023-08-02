#pragma once

#include <cstdint>

namespace FEXCore::X86State {
/**
 * @name The ordered of the GPRs from name to index
 * @{ */
enum X86Reg : uint32_t {
  REG_RAX     = 0,
  REG_RCX     = 1,
  REG_RDX     = 2,
  REG_RBX     = 3,
  REG_RSP     = 4,
  REG_RBP     = 5,
  REG_RSI     = 6,
  REG_RDI     = 7,
  REG_R8      = 8,
  REG_R9      = 9,
  REG_R10     = 10,
  REG_R11     = 11,
  REG_R12     = 12,
  REG_R13     = 13,
  REG_R14     = 14,
  REG_R15     = 15,
  REG_XMM_0   = 16,
  REG_XMM_1   = 17,
  REG_XMM_2   = 18,
  REG_XMM_3   = 19,
  REG_XMM_4   = 20,
  REG_XMM_5   = 21,
  REG_XMM_6   = 22,
  REG_XMM_7   = 23,
  REG_XMM_8   = 24,
  REG_XMM_9   = 25,
  REG_XMM_10  = 26,
  REG_XMM_11  = 27,
  REG_XMM_12  = 28,
  REG_XMM_13  = 29,
  REG_XMM_14  = 30,
  REG_XMM_15  = 31,
  REG_MM_0    = 32,
  REG_MM_1    = 33,
  REG_MM_2    = 34,
  REG_MM_3    = 35,
  REG_MM_4    = 36,
  REG_MM_5    = 37,
  REG_MM_6    = 38,
  REG_MM_7    = 39,
  REG_INVALID = 255,
};
/**  @} */

/**
 * @name RFLAG register bit locations
 * @{ */
enum X86RegLocation : uint32_t {
  RFLAG_CF_LOC    = 0,
  RFLAG_RESERVED_LOC = 1, // Reserved Bit, Read-as-1
  RFLAG_PF_LOC    = 2,
  RFLAG_AF_LOC    = 4,
  RFLAG_ZF_LOC    = 6,
  RFLAG_SF_LOC    = 7,
  RFLAG_TF_LOC    = 8,
  RFLAG_IF_LOC    = 9,
  RFLAG_DF_LOC    = 10,
  RFLAG_OF_LOC    = 11,
  RFLAG_IOPL_LOC  = 12,
  RFLAG_NT_LOC    = 14,
  RFLAG_RF_LOC    = 16,
  RFLAG_VM_LOC    = 17,
  RFLAG_AC_LOC    = 18,
  RFLAG_VIF_LOC   = 19,
  RFLAG_VIP_LOC   = 20,
  RFLAG_ID_LOC    = 21,

  // So we can implement arm64-like flag manipulaton on the interpreter/x86 jit..
  // SF/ZF/CF/OF packed into a 32-bit word, matching arm64's NZCV structure (not semantics).
  RFLAG_NZCV_LOC   = 24,
  RFLAG_NZCV_1_LOC = 25,
  RFLAG_NZCV_2_LOC = 26,
  RFLAG_NZCV_3_LOC = 27,

// So we can share flag handling logic, we put x87 flags after RFLAGS
  X87FLAG_BASE    = 32,
  X87FLAG_IE_LOC  = 32,
  X87FLAG_DE_LOC  = 33,
  X87FLAG_ZE_LOC  = 34,
  X87FLAG_OE_LOC  = 35,
  X87FLAG_UE_LOC  = 36,
  X87FLAG_PE_LOC  = 37,
  X87FLAG_SF_LOC  = 38,
  X87FLAG_ES_LOC  = 39,
  X87FLAG_C0_LOC  = 40,
  X87FLAG_C1_LOC  = 41,
  X87FLAG_C2_LOC  = 42,
  X87FLAG_TOP_LOC = 43, // 3 Bits wide
  X87FLAG_C3_LOC  = 46,
  X87FLAG_B_LOC   = 47,
};

// X86 trap number definitions
enum X86TrapNo : uint32_t {
  X86_TRAPNO_DE       = 0,  // Divide-by-zero
  X86_TRAPNO_DB       = 1,  // Debug
  X86_TRAPNO_NMI      = 2,  // Non-maskable interrupt
  X86_TRAPNO_BP       = 3,  // Breakpoint
  X86_TRAPNO_OF       = 4,  // Overflow
  X86_TRAPNO_BR       = 5,  // Bound range exceeded
  X86_TRAPNO_UD       = 6,  // Invalid opcode
  X86_TRAPNO_NM       = 7,  // Device not available
  X86_TRAPNO_DF       = 8,  // Double fault
  X86_TRAPNO_OLD_MF   = 9,  // Coprocessor segment overrun
  X86_TRAPNO_TS       = 10, // Invalid TSS
  X86_TRAPNO_NP       = 11, // Segment not present
  X86_TRAPNO_SS       = 12, // Stack segmentation fault
  X86_TRAPNO_GP       = 13, // General Protection fault
  X86_TRAPNO_PF       = 14, // Page fault
  X86_TRAPNO_SPURIOUS = 15, // Spurious interrupt
  X86_TRAPNO_MF       = 16, // X87 float exception
  X86_TRAPNO_AC       = 17, // Alignment check
  X86_TRAPNO_MC       = 18, // Machine check
  X86_TRAPNO_XF       = 19, // SIMD floating point exception
  X86_TRAPNO_VE       = 20, // Virtualization exception
  X86_TRAPNO_CP       = 21, // Control protection exception
  X86_TRAPNO_VC       = 29, // VMM communication exception
  X86_TRAPNO_IRET     = 32, // IRET exception
};

// X86 page fault error code bits
// Populates siginfo gregs[REG_ERR]
enum X86PageFaultBit : uint32_t {
  X86_PF_PROT  = (1 << 0), // 0: No page found 1: protection fault
  X86_PF_WRITE = (1 << 1), // 0: Access was read 1: Access was write
  X86_PF_USER  = (1 << 2), // 0: Kernel mode access 1: user-mode access
  X86_PF_RSV   = (1 << 3), // 1: Reserved bit?
  X86_PF_INSTR = (1 << 4), // 1: Fault from instruction fetch
  X86_PF_PK    = (1 << 5), // 1: Protection keys block access
  X86_PF_SGX   = (1 << 6), // 1: SGX MMU fault
};

} // namespace FEXCore::X86State
