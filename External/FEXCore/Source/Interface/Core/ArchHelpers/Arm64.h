#pragma once

#include <stdint.h>

namespace FEXCore::ArchHelpers::Arm64 {
  constexpr uint32_t CASPAL_MASK = 0xBF'E0'FC'00;
  constexpr uint32_t CASPAL_INST = 0x08'60'FC'00;

  constexpr uint32_t CASAL_MASK = 0x3F'E0'FC'00;
  constexpr uint32_t CASAL_INST = 0x08'E0'FC'00;

  constexpr uint32_t ATOMIC_MEM_MASK = 0x3B200C00;
  constexpr uint32_t ATOMIC_MEM_INST = 0x38200000;

  constexpr uint32_t RCPC2_MASK  = 0x3F'E0'0C'00;
  constexpr uint32_t LDAPUR_INST = 0x19'40'00'00;
  constexpr uint32_t STLUR_INST  = 0x19'00'00'00;

  constexpr uint32_t LDAXP_MASK = 0xBF'FF'80'00;
  constexpr uint32_t LDAXP_INST = 0x88'7F'80'00;

  constexpr uint32_t STLXP_MASK = 0xBF'E0'80'00;
  constexpr uint32_t STLXP_INST = 0x88'20'80'00;

  constexpr uint32_t LDAXR_MASK = 0x3F'FF'FC'00;
  constexpr uint32_t LDAXR_INST = 0x08'5F'FC'00;

  constexpr uint32_t STLXR_MASK = 0x3F'E0'FC'00;
  constexpr uint32_t STLXR_INST = 0x08'00'FC'00;

  constexpr uint32_t CBNZ_MASK = 0x7F'00'00'00;
  constexpr uint32_t CBNZ_INST = 0x35'00'00'00;

  constexpr uint32_t ALU_OP_MASK = 0x7F'00'00'00;
  constexpr uint32_t ADD_INST    = 0x0B'00'00'00;
  constexpr uint32_t SUB_INST    = 0x4B'00'00'00;
  constexpr uint32_t CMP_INST    = 0x6B'00'00'00;
  constexpr uint32_t AND_INST    = 0x0A'00'00'00;
  constexpr uint32_t OR_INST     = 0x2A'00'00'00;
  constexpr uint32_t EOR_INST    = 0x4A'00'00'00;

  constexpr uint32_t CCMP_MASK   = 0x7F'E0'0C'10;
  constexpr uint32_t CCMP_INST   = 0x7A'40'00'00;

  constexpr uint32_t CLREX_MASK = 0xFF'FF'F0'FF;
  constexpr uint32_t CLREX_INST = 0xD5'03'30'5F;

  enum ExclusiveAtomicPairType {
    TYPE_SWAP,
    TYPE_ADD,
    TYPE_SUB,
    TYPE_AND,
    TYPE_OR,
    TYPE_EOR,
    TYPE_NEG, // This is just a sub with zero. Need to know the differences
  };

  // Load ops are 4 bits
  // Acquire and release bits are independent on the instruction
  constexpr uint32_t ATOMIC_ADD_OP  = 0b0000;
  constexpr uint32_t ATOMIC_CLR_OP  = 0b0001;
  constexpr uint32_t ATOMIC_EOR_OP  = 0b0010;
  constexpr uint32_t ATOMIC_SET_OP  = 0b0011;
  constexpr uint32_t ATOMIC_SMAX_OP = 0b0100;
  constexpr uint32_t ATOMIC_SMIN_OP = 0b0101;
  constexpr uint32_t ATOMIC_UMAX_OP = 0b0110;
  constexpr uint32_t ATOMIC_UMIN_OP = 0b0111;
  constexpr uint32_t ATOMIC_SWAP_OP = 0b1000;

  constexpr uint32_t REGISTER_MASK = 0b11111;
  constexpr uint32_t RD_OFFSET = 0;
  constexpr uint32_t RN_OFFSET = 5;
  constexpr uint32_t RM_OFFSET = 16;

  constexpr uint32_t DMB_ALL = 0b1101'0101'0000'0011'0011'0000'1011'1111 |
    0b1011'0000'0000; // Inner shareable all

  constexpr uint32_t DMB_READ = 0b1101'0101'0000'0011'0011'0000'1011'1111 |
    0b1010'0000'0000; // Inner shareable reads

  constexpr uint32_t DMB_WRITE = 0b1101'0101'0000'0011'0011'0000'1011'1111 |
    0b1001'0000'0000; // Inner shareable Writes

  constexpr uint32_t NOP = 0b1101'0101'0000'0011'0010'0000'0001'1111;

  inline uint32_t GetRdReg(uint32_t Instr) {
    return (Instr >> RD_OFFSET) & REGISTER_MASK;
  }

  inline uint32_t GetRnReg(uint32_t Instr) {
    return (Instr >> RN_OFFSET) & REGISTER_MASK;
  }

  inline uint32_t GetRmReg(uint32_t Instr) {
    return (Instr >> RM_OFFSET) & REGISTER_MASK;
  }

  bool HandleAtomicLoad(void *_ucontext, void *_info, uint32_t Instr, int64_t Offset);
  bool HandleAtomicStore(void *_ucontext, void *_info, uint32_t Instr, int64_t Offset);
  bool HandleAtomicLoad128(void *_ucontext, void *_info, uint32_t Instr);
  uint64_t HandleAtomicLoadstoreExclusive(void *_ucontext, void *_info);
  bool HandleCASPAL(void *_ucontext, void *_info, uint32_t Instr);
  uint64_t HandleCASPAL_ARMv8(void *_ucontext, void *_info, uint32_t Instr);
  bool HandleAtomicVectorStore(void *_ucontext, void *_info, uint32_t Instr);
  bool HandleCASAL(void *_ucontext, void *_info, uint32_t Instr);
  bool HandleAtomicMemOp(void *_ucontext, void *_info, uint32_t Instr);
  [[nodiscard]] bool HandleSIGBUS(bool ParanoidTSO, int Signal, void *info, void *ucontext);
}
