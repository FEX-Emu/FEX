#pragma once

namespace FEXCore::X86State {
/**
 * @name The ordered of the GPRs from name to index
 * @{ */
constexpr unsigned REG_RAX = 0;
constexpr unsigned REG_RBX = 1;
constexpr unsigned REG_RCX = 2;
constexpr unsigned REG_RDX = 3;
constexpr unsigned REG_RSI = 4;
constexpr unsigned REG_RDI = 5;
constexpr unsigned REG_RBP = 6;
constexpr unsigned REG_RSP = 7;
constexpr unsigned REG_R8  = 8;
constexpr unsigned REG_R9  = 9;
constexpr unsigned REG_R10 = 10;
constexpr unsigned REG_R11 = 11;
constexpr unsigned REG_R12 = 12;
constexpr unsigned REG_R13 = 13;
constexpr unsigned REG_R14 = 14;
constexpr unsigned REG_R15 = 15;
constexpr unsigned REG_XMM_0  = 16;
constexpr unsigned REG_XMM_1  = 17;
constexpr unsigned REG_XMM_2  = 18;
constexpr unsigned REG_XMM_3  = 19;
constexpr unsigned REG_XMM_4  = 20;
constexpr unsigned REG_XMM_5  = 21;
constexpr unsigned REG_XMM_6  = 22;
constexpr unsigned REG_XMM_7  = 23;
constexpr unsigned REG_XMM_8  = 24;
constexpr unsigned REG_XMM_9  = 25;
constexpr unsigned REG_XMM_10 = 26;
constexpr unsigned REG_XMM_11 = 27;
constexpr unsigned REG_XMM_12 = 28;
constexpr unsigned REG_XMM_13 = 29;
constexpr unsigned REG_XMM_14 = 30;
constexpr unsigned REG_XMM_15 = 31;
constexpr unsigned REG_MM_0   = 32;
constexpr unsigned REG_MM_1   = 33;
constexpr unsigned REG_MM_2   = 34;
constexpr unsigned REG_MM_3   = 35;
constexpr unsigned REG_MM_4   = 36;
constexpr unsigned REG_MM_5   = 37;
constexpr unsigned REG_MM_6   = 38;
constexpr unsigned REG_MM_7   = 39;
constexpr unsigned REG_INVALID = 255;
/**  @} */

/**
 * @name RFLAG register bit locations
 * @{ */
constexpr unsigned RFLAG_CF_LOC   = 0;
constexpr unsigned RFLAG_PF_LOC   = 2;
constexpr unsigned RFLAG_AF_LOC   = 4;
constexpr unsigned RFLAG_ZF_LOC   = 6;
constexpr unsigned RFLAG_SF_LOC   = 7;
constexpr unsigned RFLAG_TF_LOC   = 8;
constexpr unsigned RFLAG_IF_LOC   = 9;
constexpr unsigned RFLAG_DF_LOC   = 10;
constexpr unsigned RFLAG_OF_LOC   = 11;
constexpr unsigned RFLAG_IOPL_LOC = 12;
constexpr unsigned RFLAG_NT_LOC   = 14;
constexpr unsigned RFLAG_RF_LOC   = 16;
constexpr unsigned RFLAG_VM_LOC   = 17;
constexpr unsigned RFLAG_AC_LOC   = 18;
constexpr unsigned RFLAG_VIF_LOC  = 19;
constexpr unsigned RFLAG_VIP_LOC  = 20;
constexpr unsigned RFLAG_ID_LOC   = 21;


// So we can share flag handling logic, we put x87 flags after RFLAGS
constexpr unsigned X87FLAG_BASE   = 32;
constexpr unsigned X87FLAG_IE_LOC = 32;
constexpr unsigned X87FLAG_DE_LOC = 33;
constexpr unsigned X87FLAG_ZE_LOC = 34;
constexpr unsigned X87FLAG_OE_LOC = 35;
constexpr unsigned X87FLAG_UE_LOC = 36;
constexpr unsigned X87FLAG_PE_LOC = 37;
constexpr unsigned X87FLAG_SF_LOC = 38;
constexpr unsigned X87FLAG_ES_LOC = 39;
constexpr unsigned X87FLAG_C0_LOC = 40;
constexpr unsigned X87FLAG_C1_LOC = 41;
constexpr unsigned X87FLAG_C2_LOC = 42;
constexpr unsigned X87FLAG_TOP_LOC = 43; // 3 Bits wide
constexpr unsigned X87FLAG_C3_LOC = 46;
constexpr unsigned X87FLAG_B_LOC = 47;




}
