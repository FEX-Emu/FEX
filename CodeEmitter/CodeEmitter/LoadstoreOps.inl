// SPDX-License-Identifier: MIT
/* Load-store instruction emitters
 *
 * For GPR load-stores that take a `Size` argument as their first argument can be 32-bit or 64-bit.
 * For GPR load-stores that don't take a `Size` argument, then their operating size is determined by the name of the instruction.
 *
 * For Vector load-stores, most take a `SubRegSize` to determine the size of the elements getting loaded or stored.
 * Depending on the instruction it can be an single element or the full instruction, it depends on the instruction.
 *
 * There are some load-store helper functions which take a `ExtendedMemOperand` argument.
 * This helper will select the viable load-store that can work with the provided encapsulated arguments.
 */

#pragma once
#ifndef INCLUDED_BY_EMITTER
#include <CodeEmitter/Emitter.h>
namespace ARMEmitter {
struct EmitterOps : Emitter {
#endif

public:
  // Compare and swap pair
  void casp(ARMEmitter::Size s, ARMEmitter::Register rs, ARMEmitter::Register rs2, ARMEmitter::Register rt, ARMEmitter::Register rt2,
            ARMEmitter::Register rn) {
    LOGMAN_THROW_A_FMT((rs.Idx() + 1) == rs2.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    constexpr uint32_t Op = 0b0000'1000'001 << 21;
    AtomicOp(Op, s, 0, 0, rs, rt, ARMEmitter::Reg::r31, rn);
  }
  void caspa(ARMEmitter::Size s, ARMEmitter::Register rs, ARMEmitter::Register rs2, ARMEmitter::Register rt, ARMEmitter::Register rt2,
             ARMEmitter::Register rn) {
    LOGMAN_THROW_A_FMT((rs.Idx() + 1) == rs2.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    constexpr uint32_t Op = 0b0000'1000'001 << 21;
    AtomicOp(Op, s, 1, 0, rs, rt, ARMEmitter::Reg::r31, rn);
  }
  void caspl(ARMEmitter::Size s, ARMEmitter::Register rs, ARMEmitter::Register rs2, ARMEmitter::Register rt, ARMEmitter::Register rt2,
             ARMEmitter::Register rn) {
    LOGMAN_THROW_A_FMT((rs.Idx() + 1) == rs2.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    constexpr uint32_t Op = 0b0000'1000'001 << 21;
    AtomicOp(Op, s, 0, 1, rs, rt, ARMEmitter::Reg::r31, rn);
  }
  void caspal(ARMEmitter::Size s, ARMEmitter::Register rs, ARMEmitter::Register rs2, ARMEmitter::Register rt, ARMEmitter::Register rt2,
              ARMEmitter::Register rn) {
    LOGMAN_THROW_A_FMT((rs.Idx() + 1) == rs2.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    constexpr uint32_t Op = 0b0000'1000'001 << 21;
    AtomicOp(Op, s, 1, 1, rs, rt, ARMEmitter::Reg::r31, rn);
  }

  // Advanced SIMD load/store multiple structures
  template<SubRegSize size, typename T>
  void ld1(T rt, Register rn) {
    constexpr uint32_t Op = 0b0000'1100'000 << 21;
    constexpr uint32_t Opcode = 0b0111 << 12;
    ASIMDLoadStoreMultipleStructure<size, true>(Op, Opcode, rt, rn, Reg::r0);
  }
  template<SubRegSize size, typename T>
  void ld1(T rt, T rt2, Register rn) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2), "rt and rt2 must be sequential");
    constexpr uint32_t Op = 0b0000'1100'000 << 21;
    constexpr uint32_t Opcode = 0b1010 << 12;
    ASIMDLoadStoreMultipleStructure<size, true>(Op, Opcode, rt, rn, Reg::r0);
  }
  template<SubRegSize size, typename T>
  void ld1(T rt, T rt2, T rt3, Register rn) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2, rt3), "rt, rt2, and rt3 must be sequential");
    constexpr uint32_t Op = 0b0000'1100'000 << 21;
    constexpr uint32_t Opcode = 0b0110 << 12;
    ASIMDLoadStoreMultipleStructure<size, true>(Op, Opcode, rt, rn, Reg::r0);
  }
  template<SubRegSize size, typename T>
  void ld1(T rt, T rt2, T rt3, T rt4, Register rn) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2, rt3, rt4), "rt, rt2, rt3, and rt4 must be sequential");
    constexpr uint32_t Op = 0b0000'1100'000 << 21;
    constexpr uint32_t Opcode = 0b0010 << 12;
    ASIMDLoadStoreMultipleStructure<size, true>(Op, Opcode, rt, rn, Reg::r0);
  }
  template<SubRegSize size, typename T>
  void st1(T rt, Register rn) {
    constexpr uint32_t Op = 0b0000'1100'000 << 21;
    constexpr uint32_t Opcode = 0b0111 << 12;
    ASIMDLoadStoreMultipleStructure<size, false>(Op, Opcode, rt, rn, Reg::r0);
  }
  template<SubRegSize size, typename T>
  void st1(T rt, T rt2, Register rn) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2), "rt and rt2 must be sequential");
    constexpr uint32_t Op = 0b0000'1100'000 << 21;
    constexpr uint32_t Opcode = 0b1010 << 12;
    ASIMDLoadStoreMultipleStructure<size, false>(Op, Opcode, rt, rn, Reg::r0);
  }
  template<SubRegSize size, typename T>
  void st1(T rt, T rt2, T rt3, Register rn) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2, rt3), "rt, rt2, and rt3 must be sequential");
    constexpr uint32_t Op = 0b0000'1100'000 << 21;
    constexpr uint32_t Opcode = 0b0110 << 12;
    ASIMDLoadStoreMultipleStructure<size, false>(Op, Opcode, rt, rn, Reg::r0);
  }
  template<SubRegSize size, typename T>
  void st1(T rt, T rt2, T rt3, T rt4, Register rn) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2, rt3, rt4), "rt, rt2, rt3, and rt4 must be sequential");
    constexpr uint32_t Op = 0b0000'1100'000 << 21;
    constexpr uint32_t Opcode = 0b0010 << 12;
    ASIMDLoadStoreMultipleStructure<size, false>(Op, Opcode, rt, rn, Reg::r0);
  }
  template<SubRegSize size, typename T>
  void ld2(T rt, T rt2, Register rn) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2), "rt and rt2 must be sequential");
    constexpr uint32_t Op = 0b0000'1100'000 << 21;
    constexpr uint32_t Opcode = 0b1000 << 12;
    ASIMDLoadStoreMultipleStructure<size, true>(Op, Opcode, rt, rn, Reg::r0);
  }
  template<SubRegSize size, typename T>
  void st2(T rt, T rt2, Register rn) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2), "rt and rt2 must be sequential");
    constexpr uint32_t Op = 0b0000'1100'000 << 21;
    constexpr uint32_t Opcode = 0b1000 << 12;
    ASIMDLoadStoreMultipleStructure<size, false>(Op, Opcode, rt, rn, Reg::r0);
  }
  template<SubRegSize size, typename T>
  void ld3(T rt, T rt2, T rt3, Register rn) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2, rt3), "rt, rt2, and rt3 must be sequential");
    constexpr uint32_t Op = 0b0000'1100'000 << 21;
    constexpr uint32_t Opcode = 0b0100 << 12;
    ASIMDLoadStoreMultipleStructure<size, true>(Op, Opcode, rt, rn, Reg::r0);
  }
  template<SubRegSize size, typename T>
  void st3(T rt, T rt2, T rt3, Register rn) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2, rt3), "rt, rt2, and rt3 must be sequential");
    constexpr uint32_t Op = 0b0000'1100'000 << 21;
    constexpr uint32_t Opcode = 0b0100 << 12;
    ASIMDLoadStoreMultipleStructure<size, false>(Op, Opcode, rt, rn, Reg::r0);
  }
  template<SubRegSize size, typename T>
  void ld4(T rt, T rt2, T rt3, T rt4, Register rn) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2, rt3, rt4), "rt, rt2, rt3, and rt4 must be sequential");
    constexpr uint32_t Op = 0b0000'1100'000 << 21;
    constexpr uint32_t Opcode = 0b0000 << 12;
    ASIMDLoadStoreMultipleStructure<size, true>(Op, Opcode, rt, rn, Reg::r0);
  }
  template<SubRegSize size, typename T>
  void st4(T rt, T rt2, T rt3, T rt4, Register rn) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2, rt3, rt4), "rt, rt2, rt3, and rt4 must be sequential");
    constexpr uint32_t Op = 0b0000'1100'000 << 21;
    constexpr uint32_t Opcode = 0b0000 << 12;
    ASIMDLoadStoreMultipleStructure<size, false>(Op, Opcode, rt, rn, Reg::r0);
  }
  // Advanced SIMD load/store multiple structures (post-indexed)
  static constexpr uint32_t ASIMDLoadstoreMultiplePost_Op = 0b0000'1100'100 << 21;
  template<SubRegSize size, typename T>
  void ld1(T rt, Register rn, Register rm) {
    constexpr uint32_t Opcode = 0b0111 << 12;
    ASIMDLoadStoreMultipleStructure<size, true>(ASIMDLoadstoreMultiplePost_Op, Opcode, rt, rn, rm);
  }
  template<SubRegSize size, typename T>
  void ld1(T rt, Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_A_FMT((std::is_same_v<QRegister, T> && (PostOffset == 16)) || (std::is_same_v<DRegister, T> && (PostOffset == 8)),
                       "Post-index offset needs to match number of elements times their size");

    constexpr uint32_t Opcode = 0b0111 << 12;
    ASIMDLoadStoreMultipleStructure<size, true>(ASIMDLoadstoreMultiplePost_Op, Opcode, rt, rn, Reg::r31);
  }
  template<SubRegSize size, typename T>
  void ld1(T rt, T rt2, Register rn, Register rm) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2), "rt and rt2 must be sequential");
    constexpr uint32_t Opcode = 0b1010 << 12;
    ASIMDLoadStoreMultipleStructure<size, true>(ASIMDLoadstoreMultiplePost_Op, Opcode, rt, rn, rm);
  }
  template<SubRegSize size, typename T>
  void ld1(T rt, T rt2, Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2), "rt and rt2 must be sequential");
    LOGMAN_THROW_A_FMT((std::is_same_v<QRegister, T> && (PostOffset == 32)) || (std::is_same_v<DRegister, T> && (PostOffset == 16)),
                       "Post-index offset needs to match number of elements times their size");

    constexpr uint32_t Opcode = 0b1010 << 12;
    ASIMDLoadStoreMultipleStructure<size, true>(ASIMDLoadstoreMultiplePost_Op, Opcode, rt, rn, Reg::r31);
  }
  template<SubRegSize size, typename T>
  void ld1(T rt, T rt2, T rt3, Register rn, Register rm) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2, rt3), "rt, rt2, and rt3 must be sequential");
    constexpr uint32_t Opcode = 0b0110 << 12;
    ASIMDLoadStoreMultipleStructure<size, true>(ASIMDLoadstoreMultiplePost_Op, Opcode, rt, rn, rm);
  }
  template<SubRegSize size, typename T>
  void ld1(T rt, T rt2, T rt3, Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2, rt3), "rt, rt2, and rt3 must be sequential");
    LOGMAN_THROW_A_FMT((std::is_same_v<QRegister, T> && (PostOffset == 48)) || (std::is_same_v<DRegister, T> && (PostOffset == 24)),
                       "Post-index offset needs to match number of elements times their size");

    constexpr uint32_t Opcode = 0b0110 << 12;
    ASIMDLoadStoreMultipleStructure<size, true>(ASIMDLoadstoreMultiplePost_Op, Opcode, rt, rn, Reg::r31);
  }
  template<SubRegSize size, typename T>
  void ld1(T rt, T rt2, T rt3, T rt4, Register rn, Register rm) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2, rt3, rt4), "rt, rt2, rt3, and rt4 must be sequential");
    constexpr uint32_t Opcode = 0b0010 << 12;
    ASIMDLoadStoreMultipleStructure<size, true>(ASIMDLoadstoreMultiplePost_Op, Opcode, rt, rn, rm);
  }
  template<SubRegSize size, typename T>
  void ld1(T rt, T rt2, T rt3, T rt4, Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2, rt3, rt4), "rt, rt2, rt3, and rt4 must be sequential");
    LOGMAN_THROW_A_FMT((std::is_same_v<QRegister, T> && (PostOffset == 64)) || (std::is_same_v<DRegister, T> && (PostOffset == 32)),
                       "Post-index offset needs to match number of elements times their size");

    constexpr uint32_t Opcode = 0b0010 << 12;
    ASIMDLoadStoreMultipleStructure<size, true>(ASIMDLoadstoreMultiplePost_Op, Opcode, rt, rn, Reg::r31);
  }

  template<SubRegSize size, typename T>
  void st1(T rt, Register rn, Register rm) {
    constexpr uint32_t Opcode = 0b0111 << 12;
    ASIMDLoadStoreMultipleStructure<size, false>(ASIMDLoadstoreMultiplePost_Op, Opcode, rt, rn, rm);
  }
  template<SubRegSize size, typename T>
  void st1(T rt, Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_A_FMT((std::is_same_v<QRegister, T> && (PostOffset == 16)) || (std::is_same_v<DRegister, T> && (PostOffset == 8)),
                       "Post-index offset needs to match number of elements times their size");

    constexpr uint32_t Opcode = 0b0111 << 12;
    ASIMDLoadStoreMultipleStructure<size, false>(ASIMDLoadstoreMultiplePost_Op, Opcode, rt, rn, Reg::r31);
  }
  template<SubRegSize size, typename T>
  void st1(T rt, T rt2, Register rn, Register rm) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2), "rt and rt2 must be sequential");
    constexpr uint32_t Opcode = 0b1010 << 12;
    ASIMDLoadStoreMultipleStructure<size, false>(ASIMDLoadstoreMultiplePost_Op, Opcode, rt, rn, rm);
  }
  template<SubRegSize size, typename T>
  void st1(T rt, T rt2, Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2), "rt and rt2 must be sequential");
    LOGMAN_THROW_A_FMT((std::is_same_v<QRegister, T> && (PostOffset == 32)) || (std::is_same_v<DRegister, T> && (PostOffset == 16)),
                       "Post-index offset needs to match number of elements times their size");

    constexpr uint32_t Opcode = 0b1010 << 12;
    ASIMDLoadStoreMultipleStructure<size, false>(ASIMDLoadstoreMultiplePost_Op, Opcode, rt, rn, Reg::r31);
  }
  template<SubRegSize size, typename T>
  void st1(T rt, T rt2, T rt3, Register rn, Register rm) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2, rt3), "rt, rt2, and rt3 must be sequential");
    constexpr uint32_t Opcode = 0b0110 << 12;
    ASIMDLoadStoreMultipleStructure<size, false>(ASIMDLoadstoreMultiplePost_Op, Opcode, rt, rn, rm);
  }
  template<SubRegSize size, typename T>
  void st1(T rt, T rt2, T rt3, Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2, rt3), "rt, rt2, and rt3 must be sequential");
    LOGMAN_THROW_A_FMT((std::is_same_v<QRegister, T> && (PostOffset == 48)) || (std::is_same_v<DRegister, T> && (PostOffset == 24)),
                       "Post-index offset needs to match number of elements times their size");

    constexpr uint32_t Opcode = 0b0110 << 12;
    ASIMDLoadStoreMultipleStructure<size, false>(ASIMDLoadstoreMultiplePost_Op, Opcode, rt, rn, Reg::r31);
  }
  template<SubRegSize size, typename T>
  void st1(T rt, T rt2, T rt3, T rt4, Register rn, Register rm) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2, rt3, rt4), "rt, rt2, rt3, and rt4 must be sequential");
    constexpr uint32_t Opcode = 0b0010 << 12;
    ASIMDLoadStoreMultipleStructure<size, false>(ASIMDLoadstoreMultiplePost_Op, Opcode, rt, rn, rm);
  }
  template<SubRegSize size, typename T>
  void st1(T rt, T rt2, T rt3, T rt4, Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2, rt3, rt4), "rt, rt2, rt3, and rt4 must be sequential");
    LOGMAN_THROW_A_FMT((std::is_same_v<QRegister, T> && (PostOffset == 64)) || (std::is_same_v<DRegister, T> && (PostOffset == 32)),
                       "Post-index offset needs to match number of elements times their size");

    constexpr uint32_t Opcode = 0b0010 << 12;
    ASIMDLoadStoreMultipleStructure<size, false>(ASIMDLoadstoreMultiplePost_Op, Opcode, rt, rn, Reg::r31);
  }

  template<SubRegSize size, typename T>
  void ld2(T rt, T rt2, Register rn, Register rm) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2), "rt and rt2 must be sequential");
    constexpr uint32_t Opcode = 0b1000 << 12;
    ASIMDLoadStoreMultipleStructure<size, true>(ASIMDLoadstoreMultiplePost_Op, Opcode, rt, rn, rm);
  }
  template<SubRegSize size, typename T>
  void ld2(T rt, T rt2, Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2), "rt and rt2 must be sequential");
    LOGMAN_THROW_A_FMT((std::is_same_v<QRegister, T> && (PostOffset == 32)) || (std::is_same_v<DRegister, T> && (PostOffset == 16)),
                       "Post-index offset needs to match number of elements times their size");

    constexpr uint32_t Opcode = 0b1000 << 12;
    ASIMDLoadStoreMultipleStructure<size, true>(ASIMDLoadstoreMultiplePost_Op, Opcode, rt, rn, Reg::r31);
  }
  template<SubRegSize size, typename T>
  void st2(T rt, T rt2, Register rn, Register rm) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2), "rt and rt2 must be sequential");
    constexpr uint32_t Opcode = 0b1000 << 12;
    ASIMDLoadStoreMultipleStructure<size, false>(ASIMDLoadstoreMultiplePost_Op, Opcode, rt, rn, rm);
  }
  template<SubRegSize size, typename T>
  void st2(T rt, T rt2, Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2), "rt and rt2 must be sequential");
    LOGMAN_THROW_A_FMT((std::is_same_v<QRegister, T> && (PostOffset == 32)) || (std::is_same_v<DRegister, T> && (PostOffset == 16)),
                       "Post-index offset needs to match number of elements times their size");

    constexpr uint32_t Opcode = 0b1000 << 12;
    ASIMDLoadStoreMultipleStructure<size, false>(ASIMDLoadstoreMultiplePost_Op, Opcode, rt, rn, Reg::r31);
  }
  template<SubRegSize size, typename T>
  void ld3(T rt, T rt2, T rt3, Register rn, Register rm) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2, rt3), "rt, rt2, and rt3 must be sequential");
    constexpr uint32_t Opcode = 0b0100 << 12;
    ASIMDLoadStoreMultipleStructure<size, true>(ASIMDLoadstoreMultiplePost_Op, Opcode, rt, rn, rm);
  }
  template<SubRegSize size, typename T>
  void ld3(T rt, T rt2, T rt3, Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2, rt3), "rt, rt2, and rt3 must be sequential");
    LOGMAN_THROW_A_FMT((std::is_same_v<QRegister, T> && (PostOffset == 48)) || (std::is_same_v<DRegister, T> && (PostOffset == 24)),
                       "Post-index offset needs to match number of elements times their size");

    constexpr uint32_t Opcode = 0b0100 << 12;
    ASIMDLoadStoreMultipleStructure<size, true>(ASIMDLoadstoreMultiplePost_Op, Opcode, rt, rn, Reg::r31);
  }
  template<SubRegSize size, typename T>
  void st3(T rt, T rt2, T rt3, Register rn, Register rm) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2, rt3), "rt, rt2, and rt3 must be sequential");
    constexpr uint32_t Opcode = 0b0100 << 12;
    ASIMDLoadStoreMultipleStructure<size, false>(ASIMDLoadstoreMultiplePost_Op, Opcode, rt, rn, rm);
  }
  template<SubRegSize size, typename T>
  void st3(T rt, T rt2, T rt3, Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2, rt3), "rt, rt2, and rt3 must be sequential");
    LOGMAN_THROW_A_FMT((std::is_same_v<QRegister, T> && (PostOffset == 48)) || (std::is_same_v<DRegister, T> && (PostOffset == 24)),
                       "Post-index offset needs to match number of elements times their size");

    constexpr uint32_t Opcode = 0b0100 << 12;
    ASIMDLoadStoreMultipleStructure<size, false>(ASIMDLoadstoreMultiplePost_Op, Opcode, rt, rn, Reg::r31);
  }
  template<SubRegSize size, typename T>
  void ld4(T rt, T rt2, T rt3, T rt4, Register rn, Register rm) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2, rt3, rt4), "rt, rt2, rt3, and rt4 must be sequential");
    constexpr uint32_t Opcode = 0b0000 << 12;
    ASIMDLoadStoreMultipleStructure<size, true>(ASIMDLoadstoreMultiplePost_Op, Opcode, rt, rn, rm);
  }
  template<SubRegSize size, typename T>
  void ld4(T rt, T rt2, T rt3, T rt4, Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2, rt3, rt4), "rt, rt2, rt3, and rt4 must be sequential");
    LOGMAN_THROW_A_FMT((std::is_same_v<QRegister, T> && (PostOffset == 64)) || (std::is_same_v<DRegister, T> && (PostOffset == 32)),
                       "Post-index offset needs to match number of elements times their size");

    constexpr uint32_t Opcode = 0b0000 << 12;
    ASIMDLoadStoreMultipleStructure<size, true>(ASIMDLoadstoreMultiplePost_Op, Opcode, rt, rn, Reg::r31);
  }
  template<SubRegSize size, typename T>
  void st4(T rt, T rt2, T rt3, T rt4, Register rn, Register rm) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2, rt3, rt4), "rt, rt2, rt3, and rt4 must be sequential");
    constexpr uint32_t Opcode = 0b0000 << 12;
    ASIMDLoadStoreMultipleStructure<size, false>(ASIMDLoadstoreMultiplePost_Op, Opcode, rt, rn, rm);
  }
  template<SubRegSize size, typename T>
  void st4(T rt, T rt2, T rt3, T rt4, Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2, rt3, rt4), "rt, rt2, rt3, and rt4 must be sequential");
    LOGMAN_THROW_A_FMT((std::is_same_v<QRegister, T> && (PostOffset == 64)) || (std::is_same_v<DRegister, T> && (PostOffset == 32)),
                       "Post-index offset needs to match number of elements times their size");

    constexpr uint32_t Opcode = 0b0000 << 12;
    ASIMDLoadStoreMultipleStructure<size, false>(ASIMDLoadstoreMultiplePost_Op, Opcode, rt, rn, Reg::r31);
  }

  // ASIMD loadstore single
  template<SubRegSize size>
  void st1(VRegister rt, uint32_t Index, Register rn) {
    constexpr uint32_t Op = 0b0000'1101'000 << 21;
    constexpr uint32_t Opcode =
        size == SubRegSize::i8Bit  ? 0b000 : // Scale = 0
        size == SubRegSize::i16Bit ? 0b010 : // Scale = 1
        size == SubRegSize::i32Bit ? 0b100 : // Scale = 2
        size == SubRegSize::i64Bit ? 0b100 : // Scale = 2 (Uses size to determine difference between 32-bit).
        0;
    ASIMDSTLD<size, false, 1>(Op, Opcode, rt, Index, rn, Reg::r0);
  }
  template<SubRegSize size>
  void st2(VRegister rt, VRegister rt2, uint32_t Index, Register rn) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2), "rt and rt2 must be sequential");
    constexpr uint32_t Op = 0b0000'1101'000 << 21;
    constexpr uint32_t Opcode =
        size == SubRegSize::i8Bit  ? 0b000 : // Scale = 0
        size == SubRegSize::i16Bit ? 0b010 : // Scale = 1
        size == SubRegSize::i32Bit ? 0b100 : // Scale = 2
        size == SubRegSize::i64Bit ? 0b100 : // Scale = 2 (Uses size to determine difference between 32-bit).
        0;
    ASIMDSTLD<size, false, 2>(Op, Opcode, rt, Index, rn, Reg::r0);
  }
  template<SubRegSize size>
  void st3(VRegister rt, VRegister rt2, VRegister rt3, uint32_t Index, Register rn) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2, rt3), "rt, rt2, and rt3 must be sequential");
    constexpr uint32_t Op = 0b0000'1101'000 << 21;
    constexpr uint32_t Opcode =
        size == SubRegSize::i8Bit  ? 0b000 : // Scale = 0
        size == SubRegSize::i16Bit ? 0b010 : // Scale = 1
        size == SubRegSize::i32Bit ? 0b100 : // Scale = 2
        size == SubRegSize::i64Bit ? 0b100 : // Scale = 2 (Uses size to determine difference between 32-bit).
        0;
    ASIMDSTLD<size, false, 3>(Op, Opcode, rt, Index, rn, Reg::r0);
  }
  template<SubRegSize size>
  void st4(VRegister rt, VRegister rt2, VRegister rt3, VRegister rt4, uint32_t Index, Register rn) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2, rt3, rt4), "rt, rt2, rt3, and rt4 must be sequential");
    constexpr uint32_t Op = 0b0000'1101'000 << 21;
    constexpr uint32_t Opcode =
        size == SubRegSize::i8Bit  ? 0b000 : // Scale = 0
        size == SubRegSize::i16Bit ? 0b010 : // Scale = 1
        size == SubRegSize::i32Bit ? 0b100 : // Scale = 2
        size == SubRegSize::i64Bit ? 0b100 : // Scale = 2 (Uses size to determine difference between 32-bit).
        0;
    ASIMDSTLD<size, false, 4>(Op, Opcode, rt, Index, rn, Reg::r0);
  }
  template<SubRegSize size>
  void ld1(VRegister rt, uint32_t Index, Register rn) {
    constexpr uint32_t Op = 0b0000'1101'000 << 21;
    constexpr uint32_t Opcode =
        size == SubRegSize::i8Bit  ? 0b000 : // Scale = 0
        size == SubRegSize::i16Bit ? 0b010 : // Scale = 1
        size == SubRegSize::i32Bit ? 0b100 : // Scale = 2
        size == SubRegSize::i64Bit ? 0b100 : // Scale = 2 (Uses size to determine difference between 32-bit).
        0;
    ASIMDSTLD<size, true, 1>(Op, Opcode, rt, Index, rn, Reg::r0);
  }
  template<SubRegSize size, IsQOrDRegister T>
  void ld1r(T rt, Register rn) {
    constexpr uint32_t Op = 0b0000'1101'000 << 21;
    constexpr uint32_t Opcode = 0b110;
    ASIMDSTLD<size, true, 1>(Op, Opcode, rt, rn, Reg::r0);
  }
  template<SubRegSize size>
  void ld2(VRegister rt, VRegister rt2, uint32_t Index, Register rn) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2), "rt and rt2 must be sequential");
    constexpr uint32_t Op = 0b0000'1101'000 << 21;
    constexpr uint32_t Opcode =
        size == SubRegSize::i8Bit  ? 0b000 : // Scale = 0
        size == SubRegSize::i16Bit ? 0b010 : // Scale = 1
        size == SubRegSize::i32Bit ? 0b100 : // Scale = 2
        size == SubRegSize::i64Bit ? 0b100 : // Scale = 2 (Uses size to determine difference between 32-bit).
        0;
    ASIMDSTLD<size, true, 2>(Op, Opcode, rt, Index, rn, Reg::r0);
  }
  template<SubRegSize size, IsQOrDRegister T>
  void ld2r(T rt, T rt2, Register rn) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2), "rt and rt2 must be sequential");
    constexpr uint32_t Op = 0b0000'1101'000 << 21;
    constexpr uint32_t Opcode = 0b110;
    ASIMDSTLD<size, true, 2>(Op, Opcode, rt, rn, Reg::r0);
  }
  template<SubRegSize size>
  void ld3(VRegister rt, VRegister rt2, VRegister rt3, uint32_t Index, Register rn) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2, rt3), "rt, rt2, and rt3 must be sequential");
    constexpr uint32_t Op = 0b0000'1101'000 << 21;
    constexpr uint32_t Opcode =
        size == SubRegSize::i8Bit  ? 0b000 : // Scale = 0
        size == SubRegSize::i16Bit ? 0b010 : // Scale = 1
        size == SubRegSize::i32Bit ? 0b100 : // Scale = 2
        size == SubRegSize::i64Bit ? 0b100 : // Scale = 2 (Uses size to determine difference between 32-bit).
        0;
    ASIMDSTLD<size, true, 3>(Op, Opcode, rt, Index, rn, Reg::r0);
  }
  template<SubRegSize size, IsQOrDRegister T>
  void ld3r(T rt, T rt2, T rt3, Register rn) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2, rt3), "rt, rt2, and rt3 must be sequential");
    constexpr uint32_t Op = 0b0000'1101'000 << 21;
    constexpr uint32_t Opcode = 0b110;
    ASIMDSTLD<size, true, 3>(Op, Opcode, rt, rn, Reg::r0);
  }
  template<SubRegSize size>
  void ld4(VRegister rt, VRegister rt2, VRegister rt3, VRegister rt4, uint32_t Index, Register rn) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2, rt3, rt4), "rt, rt2, rt3, and rt4 must be sequential");
    constexpr uint32_t Op = 0b0000'1101'000 << 21;
    constexpr uint32_t Opcode =
        size == SubRegSize::i8Bit  ? 0b000 : // Scale = 0
        size == SubRegSize::i16Bit ? 0b010 : // Scale = 1
        size == SubRegSize::i32Bit ? 0b100 : // Scale = 2
        size == SubRegSize::i64Bit ? 0b100 : // Scale = 2 (Uses size to determine difference between 32-bit).
        0;
    ASIMDSTLD<size, true, 4>(Op, Opcode, rt, Index, rn, Reg::r0);
  }
  template<SubRegSize size, IsQOrDRegister T>
  void ld4r(T rt, T rt2, T rt3, T rt4, Register rn) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2, rt3, rt4), "rt, rt2, rt3, and rt4 must be sequential");
    constexpr uint32_t Op = 0b0000'1101'000 << 21;
    constexpr uint32_t Opcode = 0b110;
    ASIMDSTLD<size, true, 4>(Op, Opcode, rt, rn, Reg::r0);
  }

  // ASIMD loadstore single post-indexed
  template<SubRegSize size>
  void st1(VRegister rt, uint32_t Index, Register rn, Register rm) {
    constexpr uint32_t Op = 0b0000'1101'100 << 21;
    constexpr uint32_t Opcode =
        size == SubRegSize::i8Bit  ? 0b000 : // Scale = 0
        size == SubRegSize::i16Bit ? 0b010 : // Scale = 1
        size == SubRegSize::i32Bit ? 0b100 : // Scale = 2
        size == SubRegSize::i64Bit ? 0b100 : // Scale = 2 (Uses size to determine difference between 32-bit).
        0;
    ASIMDSTLD<size, false, 1>(Op, Opcode, rt, Index, rn, rm);
  }
  template<SubRegSize size>
  void st1(VRegister rt, uint32_t Index, Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_A_FMT((size == SubRegSize::i8Bit && (PostOffset == 1)) || (size == SubRegSize::i16Bit && (PostOffset == 2)) ||
                         (size == SubRegSize::i32Bit && (PostOffset == 4)) || (size == SubRegSize::i64Bit && (PostOffset == 8)),
                       "Post-index offset needs to match number of elements times their size");

    constexpr uint32_t Op = 0b0000'1101'100 << 21;
    constexpr uint32_t Opcode =
        size == SubRegSize::i8Bit  ? 0b000 : // Scale = 0
        size == SubRegSize::i16Bit ? 0b010 : // Scale = 1
        size == SubRegSize::i32Bit ? 0b100 : // Scale = 2
        size == SubRegSize::i64Bit ? 0b100 : // Scale = 2 (Uses size to determine difference between 32-bit).
        0;
    ASIMDSTLD<size, false, 1>(Op, Opcode, rt, Index, rn, Reg::r31);
  }
  template<SubRegSize size>
  void st2(VRegister rt, VRegister rt2, uint32_t Index, Register rn, Register rm) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2), "rt and rt2 must be sequential");
    constexpr uint32_t Op = 0b0000'1101'100 << 21;
    constexpr uint32_t Opcode =
        size == SubRegSize::i8Bit  ? 0b000 : // Scale = 0
        size == SubRegSize::i16Bit ? 0b010 : // Scale = 1
        size == SubRegSize::i32Bit ? 0b100 : // Scale = 2
        size == SubRegSize::i64Bit ? 0b100 : // Scale = 2 (Uses size to determine difference between 32-bit).
        0;
    ASIMDSTLD<size, false, 2>(Op, Opcode, rt, Index, rn, rm);
  }
  template<SubRegSize size>
  void st2(VRegister rt, uint32_t Index, Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_A_FMT((size == SubRegSize::i8Bit && (PostOffset == 2)) || (size == SubRegSize::i16Bit && (PostOffset == 4)) ||
                         (size == SubRegSize::i32Bit && (PostOffset == 8)) || (size == SubRegSize::i64Bit && (PostOffset == 16)),
                       "Post-index offset needs to match number of elements times their size");

    constexpr uint32_t Op = 0b0000'1101'100 << 21;
    constexpr uint32_t Opcode =
        size == SubRegSize::i8Bit  ? 0b000 : // Scale = 0
        size == SubRegSize::i16Bit ? 0b010 : // Scale = 1
        size == SubRegSize::i32Bit ? 0b100 : // Scale = 2
        size == SubRegSize::i64Bit ? 0b100 : // Scale = 2 (Uses size to determine difference between 32-bit).
        0;
    ASIMDSTLD<size, false, 2>(Op, Opcode, rt, Index, rn, Reg::r31);
  }
  template<SubRegSize size>
  void st3(VRegister rt, VRegister rt2, VRegister rt3, uint32_t Index, Register rn, Register rm) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2, rt3), "rt, rt2, and rt3 must be sequential");
    constexpr uint32_t Op = 0b0000'1101'100 << 21;
    constexpr uint32_t Opcode =
        size == SubRegSize::i8Bit  ? 0b000 : // Scale = 0
        size == SubRegSize::i16Bit ? 0b010 : // Scale = 1
        size == SubRegSize::i32Bit ? 0b100 : // Scale = 2
        size == SubRegSize::i64Bit ? 0b100 : // Scale = 2 (Uses size to determine difference between 32-bit).
        0;
    ASIMDSTLD<size, false, 3>(Op, Opcode, rt, Index, rn, rm);
  }
  template<SubRegSize size>
  void st3(VRegister rt, uint32_t Index, Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_A_FMT((size == SubRegSize::i8Bit && (PostOffset == 3)) || (size == SubRegSize::i16Bit && (PostOffset == 6)) ||
                         (size == SubRegSize::i32Bit && (PostOffset == 8)) || (size == SubRegSize::i64Bit && (PostOffset == 24)),
                       "Post-index offset needs to match number of elements times their size");

    constexpr uint32_t Op = 0b0000'1101'100 << 21;
    constexpr uint32_t Opcode =
        size == SubRegSize::i8Bit  ? 0b000 : // Scale = 0
        size == SubRegSize::i16Bit ? 0b010 : // Scale = 1
        size == SubRegSize::i32Bit ? 0b100 : // Scale = 2
        size == SubRegSize::i64Bit ? 0b100 : // Scale = 2 (Uses size to determine difference between 32-bit).
        0;
    ASIMDSTLD<size, false, 3>(Op, Opcode, rt, Index, rn, Reg::r31);
  }
  template<SubRegSize size>
  void st4(VRegister rt, VRegister rt2, VRegister rt3, VRegister rt4, uint32_t Index, Register rn, Register rm) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2, rt3, rt4), "rt, rt2, rt3, and rt4 must be sequential");
    constexpr uint32_t Op = 0b0000'1101'100 << 21;
    constexpr uint32_t Opcode =
        size == SubRegSize::i8Bit  ? 0b000 : // Scale = 0
        size == SubRegSize::i16Bit ? 0b010 : // Scale = 1
        size == SubRegSize::i32Bit ? 0b100 : // Scale = 2
        size == SubRegSize::i64Bit ? 0b100 : // Scale = 2 (Uses size to determine difference between 32-bit).
        0;
    ASIMDSTLD<size, false, 4>(Op, Opcode, rt, Index, rn, rm);
  }
  template<SubRegSize size>
  void st4(VRegister rt, uint32_t Index, Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_A_FMT((size == SubRegSize::i8Bit && (PostOffset == 4)) || (size == SubRegSize::i16Bit && (PostOffset == 8)) ||
                         (size == SubRegSize::i32Bit && (PostOffset == 16)) || (size == SubRegSize::i64Bit && (PostOffset == 32)),
                       "Post-index offset needs to match number of elements times their size");

    constexpr uint32_t Op = 0b0000'1101'100 << 21;
    constexpr uint32_t Opcode =
        size == SubRegSize::i8Bit  ? 0b000 : // Scale = 0
        size == SubRegSize::i16Bit ? 0b010 : // Scale = 1
        size == SubRegSize::i32Bit ? 0b100 : // Scale = 2
        size == SubRegSize::i64Bit ? 0b100 : // Scale = 2 (Uses size to determine difference between 32-bit).
        0;
    ASIMDSTLD<size, false, 4>(Op, Opcode, rt, Index, rn, Reg::r31);
  }
  template<SubRegSize size>
  void ld1(VRegister rt, uint32_t Index, Register rn, Register rm) {
    constexpr uint32_t Op = 0b0000'1101'100 << 21;
    constexpr uint32_t Opcode =
        size == SubRegSize::i8Bit  ? 0b000 : // Scale = 0
        size == SubRegSize::i16Bit ? 0b010 : // Scale = 1
        size == SubRegSize::i32Bit ? 0b100 : // Scale = 2
        size == SubRegSize::i64Bit ? 0b100 : // Scale = 2 (Uses size to determine difference between 32-bit).
        0;
    ASIMDSTLD<size, true, 1>(Op, Opcode, rt, Index, rn, rm);
  }
  template<SubRegSize size>
  void ld1(VRegister rt, uint32_t Index, Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_A_FMT((size == SubRegSize::i8Bit && (PostOffset == 1)) || (size == SubRegSize::i16Bit && (PostOffset == 2)) ||
                         (size == SubRegSize::i32Bit && (PostOffset == 4)) || (size == SubRegSize::i64Bit && (PostOffset == 8)),
                       "Post-index offset needs to match number of elements times their size");
    constexpr uint32_t Op = 0b0000'1101'100 << 21;
    constexpr uint32_t Opcode =
        size == SubRegSize::i8Bit  ? 0b000 : // Scale = 0
        size == SubRegSize::i16Bit ? 0b010 : // Scale = 1
        size == SubRegSize::i32Bit ? 0b100 : // Scale = 2
        size == SubRegSize::i64Bit ? 0b100 : // Scale = 2 (Uses size to determine difference between 32-bit).
        0;
    ASIMDSTLD<size, true, 1>(Op, Opcode, rt, Index, rn, Reg::r31);
  }
  template<SubRegSize size>
  void ld1r(VRegister rt, Register rn, Register rm) {
    constexpr uint32_t Op = 0b0000'1101'100 << 21;
    constexpr uint32_t Opcode = 0b110;
    ASIMDSTLD<size, true, 1>(Op, Opcode, rt, 0, rn, rm);
  }
  template<SubRegSize size>
  void ld1r(VRegister rt, Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_A_FMT((size == SubRegSize::i8Bit && (PostOffset == 1)) || (size == SubRegSize::i16Bit && (PostOffset == 2)) ||
                         (size == SubRegSize::i32Bit && (PostOffset == 4)) || (size == SubRegSize::i64Bit && (PostOffset == 8)),
                       "Post-index offset needs to match number of elements times their size");
    constexpr uint32_t Op = 0b0000'1101'100 << 21;
    constexpr uint32_t Opcode = 0b110;
    ASIMDSTLD<size, true, 1>(Op, Opcode, rt, 0, rn, Reg::r31);
  }
  template<SubRegSize size>
  void ld2(VRegister rt, VRegister rt2, uint32_t Index, Register rn, Register rm) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2), "rt and rt2 must be sequential");
    constexpr uint32_t Op = 0b0000'1101'100 << 21;
    constexpr uint32_t Opcode =
        size == SubRegSize::i8Bit  ? 0b000 : // Scale = 0
        size == SubRegSize::i16Bit ? 0b010 : // Scale = 1
        size == SubRegSize::i32Bit ? 0b100 : // Scale = 2
        size == SubRegSize::i64Bit ? 0b100 : // Scale = 2 (Uses size to determine difference between 32-bit).
        0;
    ASIMDSTLD<size, true, 2>(Op, Opcode, rt, Index, rn, rm);
  }
  template<SubRegSize size>
  void ld2(VRegister rt, uint32_t Index, Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_A_FMT((size == SubRegSize::i8Bit && (PostOffset == 2)) || (size == SubRegSize::i16Bit && (PostOffset == 4)) ||
                         (size == SubRegSize::i32Bit && (PostOffset == 8)) || (size == SubRegSize::i64Bit && (PostOffset == 16)),
                       "Post-index offset needs to match number of elements times their size");
    constexpr uint32_t Op = 0b0000'1101'100 << 21;
    constexpr uint32_t Opcode =
        size == SubRegSize::i8Bit  ? 0b000 : // Scale = 0
        size == SubRegSize::i16Bit ? 0b010 : // Scale = 1
        size == SubRegSize::i32Bit ? 0b100 : // Scale = 2
        size == SubRegSize::i64Bit ? 0b100 : // Scale = 2 (Uses size to determine difference between 32-bit).
        0;
    ASIMDSTLD<size, true, 2>(Op, Opcode, rt, Index, rn, Reg::r31);
  }
  template<SubRegSize size>
  void ld2r(VRegister rt, VRegister rt2, Register rn, Register rm) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2), "rt and rt2 must be sequential");
    constexpr uint32_t Op = 0b0000'1101'100 << 21;
    constexpr uint32_t Opcode = 0b110;
    ASIMDSTLD<size, true, 2>(Op, Opcode, rt, 0, rn, rm);
  }
  template<SubRegSize size>
  void ld2r(VRegister rt, VRegister rt2, Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2), "rt and rt2 must be sequential");
    LOGMAN_THROW_A_FMT((size == SubRegSize::i8Bit && (PostOffset == 2)) || (size == SubRegSize::i16Bit && (PostOffset == 4)) ||
                         (size == SubRegSize::i32Bit && (PostOffset == 8)) || (size == SubRegSize::i64Bit && (PostOffset == 16)),
                       "Post-index offset needs to match number of elements times their size");
    constexpr uint32_t Op = 0b0000'1101'100 << 21;
    constexpr uint32_t Opcode = 0b110;
    ASIMDSTLD<size, true, 2>(Op, Opcode, rt, 0, rn, Reg::r31);
  }
  template<SubRegSize size>
  void ld3(VRegister rt, VRegister rt2, VRegister rt3, uint32_t Index, Register rn, Register rm) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2, rt3), "rt, rt2, and rt3 must be sequential");
    constexpr uint32_t Op = 0b0000'1101'100 << 21;
    constexpr uint32_t Opcode =
        size == SubRegSize::i8Bit  ? 0b000 : // Scale = 0
        size == SubRegSize::i16Bit ? 0b010 : // Scale = 1
        size == SubRegSize::i32Bit ? 0b100 : // Scale = 2
        size == SubRegSize::i64Bit ? 0b100 : // Scale = 2 (Uses size to determine difference between 32-bit).
        0;
    ASIMDSTLD<size, true, 3>(Op, Opcode, rt, Index, rn, rm);
  }
  template<SubRegSize size>
  void ld3(VRegister rt, uint32_t Index, Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_A_FMT((size == SubRegSize::i8Bit && (PostOffset == 3)) || (size == SubRegSize::i16Bit && (PostOffset == 6)) ||
                         (size == SubRegSize::i32Bit && (PostOffset == 12)) || (size == SubRegSize::i64Bit && (PostOffset == 16)),
                       "Post-index offset needs to match number of elements times their size");
    constexpr uint32_t Op = 0b0000'1101'100 << 21;
    constexpr uint32_t Opcode =
        size == SubRegSize::i8Bit  ? 0b000 : // Scale = 0
        size == SubRegSize::i16Bit ? 0b010 : // Scale = 1
        size == SubRegSize::i32Bit ? 0b100 : // Scale = 2
        size == SubRegSize::i64Bit ? 0b100 : // Scale = 2 (Uses size to determine difference between 32-bit).
        0;
    ASIMDSTLD<size, true, 3>(Op, Opcode, rt, Index, rn, Reg::r31);
  }
  template<SubRegSize size>
  void ld3r(VRegister rt, VRegister rt2, VRegister rt3, Register rn, Register rm) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2, rt3), "rt, rt2, and rt3 must be sequential");
    constexpr uint32_t Op = 0b0000'1101'100 << 21;
    constexpr uint32_t Opcode = 0b110;
    ASIMDSTLD<size, true, 3>(Op, Opcode, rt, 0, rn, rm);
  }
  template<SubRegSize size>
  void ld3r(VRegister rt, VRegister rt2, VRegister rt3, Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2, rt3), "rt, rt2, and rt3 must be sequential");
    LOGMAN_THROW_A_FMT((size == SubRegSize::i8Bit && (PostOffset == 3)) || (size == SubRegSize::i16Bit && (PostOffset == 6)) ||
                         (size == SubRegSize::i32Bit && (PostOffset == 12)) || (size == SubRegSize::i64Bit && (PostOffset == 16)),
                       "Post-index offset needs to match number of elements times their size");
    constexpr uint32_t Op = 0b0000'1101'100 << 21;
    constexpr uint32_t Opcode = 0b110;
    ASIMDSTLD<size, true, 3>(Op, Opcode, rt, 0, rn, Reg::r31);
  }
  template<SubRegSize size>
  void ld4(VRegister rt, VRegister rt2, VRegister rt3, VRegister rt4, uint32_t Index, Register rn, Register rm) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2, rt3, rt4), "rt, rt2, rt3, and rt4 must be sequential");
    constexpr uint32_t Op = 0b0000'1101'100 << 21;
    constexpr uint32_t Opcode =
        size == SubRegSize::i8Bit  ? 0b000 : // Scale = 0
        size == SubRegSize::i16Bit ? 0b010 : // Scale = 1
        size == SubRegSize::i32Bit ? 0b100 : // Scale = 2
        size == SubRegSize::i64Bit ? 0b100 : // Scale = 2 (Uses size to determine difference between 32-bit).
        0;
    ASIMDSTLD<size, true, 4>(Op, Opcode, rt, Index, rn, rm);
  }
  template<SubRegSize size>
  void ld4(VRegister rt, uint32_t Index, Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_A_FMT((size == SubRegSize::i8Bit && (PostOffset == 4)) || (size == SubRegSize::i16Bit && (PostOffset == 8)) ||
                         (size == SubRegSize::i32Bit && (PostOffset == 16)) || (size == SubRegSize::i64Bit && (PostOffset == 32)),
                       "Post-index offset needs to match number of elements times their size");
    constexpr uint32_t Op = 0b0000'1101'100 << 21;
    constexpr uint32_t Opcode =
        size == SubRegSize::i8Bit  ? 0b000 : // Scale = 0
        size == SubRegSize::i16Bit ? 0b010 : // Scale = 1
        size == SubRegSize::i32Bit ? 0b100 : // Scale = 2
        size == SubRegSize::i64Bit ? 0b100 : // Scale = 2 (Uses size to determine difference between 32-bit).
        0;
    ASIMDSTLD<size, true, 4>(Op, Opcode, rt, Index, rn, Reg::r31);
  }
  template<SubRegSize size>
  void ld4r(VRegister rt, VRegister rt2, VRegister rt3, VRegister rt4, Register rn, Register rm) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2, rt3, rt4), "rt, rt2, rt3, and rt4 must be sequential");
    constexpr uint32_t Op = 0b0000'1101'100 << 21;
    constexpr uint32_t Opcode = 0b110;
    ASIMDSTLD<size, true, 4>(Op, Opcode, rt, 0, rn, rm);
  }
  template<SubRegSize size>
  void ld4r(VRegister rt, VRegister rt2, VRegister rt3, VRegister rt4, Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2, rt3, rt4), "rt, rt2, rt3, and rt4 must be sequential");
    LOGMAN_THROW_A_FMT((size == SubRegSize::i8Bit && (PostOffset == 4)) || (size == SubRegSize::i16Bit && (PostOffset == 8)) ||
                         (size == SubRegSize::i32Bit && (PostOffset == 16)) || (size == SubRegSize::i64Bit && (PostOffset == 32)),
                       "Post-index offset needs to match number of elements times their size");
    constexpr uint32_t Op = 0b0000'1101'100 << 21;
    constexpr uint32_t Opcode = 0b110;
    ASIMDSTLD<size, true, 4>(Op, Opcode, rt, 0, rn, Reg::r31);
  }

  // Advanced SIMD load/store single structure (post-indexed)
  template<typename T>
  void st1(ARMEmitter::SubRegSize size, T rt, uint32_t Index, ARMEmitter::Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_A_FMT(size == SubRegSize::i8Bit || size == SubRegSize::i16Bit || size == SubRegSize::i32Bit || size == SubRegSize::i64Bit,
                       "Incorrect size");
    LOGMAN_THROW_A_FMT((PostOffset * 8) == SubRegSizeInBits(size), "Post-Index size must match element size");
    constexpr uint32_t Op = 0b0000'1101'1 << 23;
    uint32_t Q;
    uint32_t R = 0;
    uint32_t opcode;
    uint32_t S;
    uint32_t Size;
    if (size == SubRegSize::i8Bit) {
      LOGMAN_THROW_A_FMT(Index < 16, "Index too large");
      Q = Index >> 3;
      S = (Index >> 2) & 1;
      opcode = 0b000;
      Size = Index & 0b11;
    } else if (size == SubRegSize::i16Bit) {
      LOGMAN_THROW_A_FMT(Index < 8, "Index too large");
      Q = Index >> 2;
      S = (Index >> 1) & 1;
      opcode = 0b010;
      Size = (Index & 0b1) << 1;
    } else if (size == SubRegSize::i32Bit) {
      LOGMAN_THROW_A_FMT(Index < 4, "Index too large");
      Q = Index >> 1;
      S = Index & 1;
      opcode = 0b100;
      Size = 0b00;
    } else if (size == SubRegSize::i64Bit) {
      LOGMAN_THROW_A_FMT(Index < 2, "Index too large");
      Q = Index;
      S = 0;
      opcode = 0b100;
      Size = 0b01;
    } else {
      LOGMAN_MSG_A_FMT("Unknown size");
      FEX_UNREACHABLE;
    }

    ASIMDLoadStoreSinglePost(Op, Q, 0, R, opcode, S, Size, ARMEmitter::Reg::r31, rn, rt.Q());
  }
  template<typename T>
  void ld1(ARMEmitter::SubRegSize size, T rt, uint32_t Index, ARMEmitter::Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_A_FMT(size == SubRegSize::i8Bit || size == SubRegSize::i16Bit || size == SubRegSize::i32Bit || size == SubRegSize::i64Bit,
                       "Incorrect size");
    LOGMAN_THROW_A_FMT((PostOffset * 8) == SubRegSizeInBits(size), "Post-Index size must match element size");
    constexpr uint32_t Op = 0b0000'1101'1 << 23;
    uint32_t Q;
    uint32_t R = 0;
    uint32_t opcode;
    uint32_t S;
    uint32_t Size;
    if (size == SubRegSize::i8Bit) {
      LOGMAN_THROW_A_FMT(Index < 16, "Index too large");
      Q = Index >> 3;
      S = (Index >> 2) & 1;
      opcode = 0b001;
      Size = Index & 0b11;
    } else if (size == SubRegSize::i16Bit) {
      LOGMAN_THROW_A_FMT(Index < 8, "Index too large");
      Q = Index >> 2;
      S = (Index >> 1) & 1;
      opcode = 0b011;
      Size = (Index & 0b1) << 1;
    } else if (size == SubRegSize::i32Bit) {
      LOGMAN_THROW_A_FMT(Index < 4, "Index too large");
      Q = Index >> 1;
      S = Index & 1;
      opcode = 0b100;
      Size = 0b00;
    } else if (size == SubRegSize::i64Bit) {
      LOGMAN_THROW_A_FMT(Index < 2, "Index too large");
      Q = Index;
      S = 0;
      opcode = 0b101;
      Size = 0b01;
    } else {
      LOGMAN_MSG_A_FMT("Unknown size");
      FEX_UNREACHABLE;
    }

    ASIMDLoadStoreSinglePost(Op, Q, 1, R, opcode, S, Size, ARMEmitter::Reg::r31, rn, rt.Q());
  }
  template<typename T>
  void ld1r(ARMEmitter::SubRegSize size, T rt, ARMEmitter::Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_A_FMT(PostOffset == 1 || PostOffset == 2 || PostOffset == 4 || PostOffset == 8, "Index too large");
    constexpr uint32_t Op = 0b0000'1101'1 << 23;
    constexpr uint32_t Q = std::is_same_v<ARMEmitter::QRegister, T> ? 1 : 0;
    uint32_t R = 0;
    uint32_t opcode = 0b110;
    uint32_t S = 0;
    uint32_t Size = FEXCore::ToUnderlying(size);
    ASIMDLoadStoreSinglePost<T>(Op, Q, 1, R, opcode, S, Size, ARMEmitter::Reg::r31, rn, rt);
  }

  template<typename T>
  void ld2r(SubRegSize size, T rt, T rt2, Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2), "rt and rt2 must be sequential");
    LOGMAN_THROW_A_FMT(PostOffset == 2 || PostOffset == 4 || PostOffset == 8 || PostOffset == 16, "Index too large");
    constexpr uint32_t Op = 0b0000'1101'1 << 23;
    constexpr uint32_t Q = std::is_same_v<QRegister, T> ? 1 : 0;
    uint32_t R = 1;
    uint32_t opcode = 0b110;
    uint32_t S = 0;
    uint32_t Size = FEXCore::ToUnderlying(size);
    ASIMDLoadStoreSinglePost<T>(Op, Q, 1, R, opcode, S, Size, Reg::r31, rn, rt);
  }

  template<typename T>
  void ld3r(SubRegSize size, T rt, T rt2, T rt3, Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2, rt3), "rt, rt2, and rt3 must be sequential");
    LOGMAN_THROW_A_FMT(PostOffset == 3 || PostOffset == 6 || PostOffset == 12 || PostOffset == 24, "Index too large");
    constexpr uint32_t Op = 0b0000'1101'1 << 23;
    constexpr uint32_t Q = std::is_same_v<QRegister, T> ? 1 : 0;
    uint32_t R = 0;
    uint32_t opcode = 0b111;
    uint32_t S = 0;
    uint32_t Size = FEXCore::ToUnderlying(size);
    ASIMDLoadStoreSinglePost<T>(Op, Q, 1, R, opcode, S, Size, Reg::r31, rn, rt);
  }
  template<typename T>
  void ld4r(SubRegSize size, T rt, T rt2, T rt3, T rt4, Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2, rt3, rt4), "rt, rt2, rt3, and rt4 must be sequential");
    LOGMAN_THROW_A_FMT(PostOffset == 4 || PostOffset == 8 || PostOffset == 16 || PostOffset == 32, "Index too large");
    constexpr uint32_t Op = 0b0000'1101'1 << 23;
    constexpr uint32_t Q = std::is_same_v<QRegister, T> ? 1 : 0;
    uint32_t R = 1;
    uint32_t opcode = 0b111;
    uint32_t S = 0;
    uint32_t Size = FEXCore::ToUnderlying(size);
    ASIMDLoadStoreSinglePost<T>(Op, Q, 1, R, opcode, S, Size, Reg::r31, rn, rt);
  }

  template<typename T>
  void st2(SubRegSize size, T rt, T rt2, uint32_t Index, Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_A_FMT(size == SubRegSize::i8Bit || size == SubRegSize::i16Bit || size == SubRegSize::i32Bit || size == SubRegSize::i64Bit,
                       "Incorrect size");
    LOGMAN_THROW_A_FMT((PostOffset * 8) == (SubRegSizeInBits(size) * 2), "Post-Index size must match element size");
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2), "rt and rt2 must be sequential");

    constexpr uint32_t Op = 0b0000'1101'1 << 23;
    uint32_t Q;
    uint32_t R = 1;
    uint32_t opcode;
    uint32_t S;
    uint32_t Size;
    if (size == SubRegSize::i8Bit) {
      LOGMAN_THROW_A_FMT(Index < 16, "Index too large");
      Q = Index >> 3;
      S = (Index >> 2) & 1;
      opcode = 0b000;
      Size = Index & 0b11;
    } else if (size == SubRegSize::i16Bit) {
      LOGMAN_THROW_A_FMT(Index < 8, "Index too large");
      Q = Index >> 2;
      S = (Index >> 1) & 1;
      opcode = 0b010;
      Size = (Index & 0b1) << 1;
    } else if (size == SubRegSize::i32Bit) {
      LOGMAN_THROW_A_FMT(Index < 4, "Index too large");
      Q = Index >> 1;
      S = Index & 1;
      opcode = 0b100;
      Size = 0b00;
    } else if (size == SubRegSize::i64Bit) {
      LOGMAN_THROW_A_FMT(Index < 2, "Index too large");
      Q = Index;
      S = 0;
      opcode = 0b100;
      Size = 0b01;
    } else {
      LOGMAN_MSG_A_FMT("Unknown size");
      FEX_UNREACHABLE;
    }

    ASIMDLoadStoreSinglePost(Op, Q, 0, R, opcode, S, Size, Reg::r31, rn, rt.Q());
  }
  template<typename T>
  void ld2(SubRegSize size, T rt, T rt2, uint32_t Index, Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_A_FMT(size == SubRegSize::i8Bit || size == SubRegSize::i16Bit || size == SubRegSize::i32Bit || size == SubRegSize::i64Bit,
                       "Incorrect size");
    LOGMAN_THROW_A_FMT((PostOffset * 8) == (SubRegSizeInBits(size) * 2), "Post-Index size must match element size");
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2), "rt and rt2 must be sequential");

    constexpr uint32_t Op = 0b0000'1101'1 << 23;
    uint32_t Q;
    uint32_t R = 1;
    uint32_t opcode;
    uint32_t S;
    uint32_t Size;
    if (size == SubRegSize::i8Bit) {
      LOGMAN_THROW_A_FMT(Index < 16, "Index too large");
      Q = Index >> 3;
      S = (Index >> 2) & 1;
      opcode = 0b000;
      Size = Index & 0b11;
    } else if (size == SubRegSize::i16Bit) {
      LOGMAN_THROW_A_FMT(Index < 8, "Index too large");
      Q = Index >> 2;
      S = (Index >> 1) & 1;
      opcode = 0b010;
      Size = (Index & 0b1) << 1;
    } else if (size == SubRegSize::i32Bit) {
      LOGMAN_THROW_A_FMT(Index < 4, "Index too large");
      Q = Index >> 1;
      S = Index & 1;
      opcode = 0b100;
      Size = 0b00;
    } else if (size == SubRegSize::i64Bit) {
      LOGMAN_THROW_A_FMT(Index < 2, "Index too large");
      Q = Index;
      S = 0;
      opcode = 0b100;
      Size = 0b01;
    } else {
      LOGMAN_MSG_A_FMT("Unknown size");
      FEX_UNREACHABLE;
    }

    ASIMDLoadStoreSinglePost(Op, Q, 1, R, opcode, S, Size, Reg::r31, rn, rt.Q());
  }
  template<typename T>
  void st3(SubRegSize size, T rt, T rt2, T rt3, uint32_t Index, Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_A_FMT(size == SubRegSize::i8Bit || size == SubRegSize::i16Bit || size == SubRegSize::i32Bit || size == SubRegSize::i64Bit,
                       "Incorrect size");
    LOGMAN_THROW_A_FMT((PostOffset * 8) == (SubRegSizeInBits(size) * 3), "Post-Index size must match element size");
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2, rt3), "rt, rt2, and rt3 must be sequential");

    constexpr uint32_t Op = 0b0000'1101'1 << 23;
    uint32_t Q;
    uint32_t R = 0;
    uint32_t opcode;
    uint32_t S;
    uint32_t Size;
    if (size == SubRegSize::i8Bit) {
      LOGMAN_THROW_A_FMT(Index < 16, "Index too large");
      Q = Index >> 3;
      S = (Index >> 2) & 1;
      opcode = 0b001;
      Size = Index & 0b11;
    } else if (size == SubRegSize::i16Bit) {
      LOGMAN_THROW_A_FMT(Index < 8, "Index too large");
      Q = Index >> 2;
      S = (Index >> 1) & 1;
      opcode = 0b011;
      Size = (Index & 0b1) << 1;
    } else if (size == SubRegSize::i32Bit) {
      LOGMAN_THROW_A_FMT(Index < 4, "Index too large");
      Q = Index >> 1;
      S = Index & 1;
      opcode = 0b101;
      Size = 0b00;
    } else if (size == SubRegSize::i64Bit) {
      LOGMAN_THROW_A_FMT(Index < 2, "Index too large");
      Q = Index;
      S = 0;
      opcode = 0b101;
      Size = 0b01;
    } else {
      LOGMAN_MSG_A_FMT("Unknown size");
      FEX_UNREACHABLE;
    }

    ASIMDLoadStoreSinglePost(Op, Q, 0, R, opcode, S, Size, Reg::r31, rn, rt.Q());
  }
  template<typename T>
  void ld3(SubRegSize size, T rt, T rt2, T rt3, uint32_t Index, Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_A_FMT(size == SubRegSize::i8Bit || size == SubRegSize::i16Bit || size == SubRegSize::i32Bit || size == SubRegSize::i64Bit,
                       "Incorrect size");
    LOGMAN_THROW_A_FMT((PostOffset * 8) == (SubRegSizeInBits(size) * 3), "Post-Index size must match element size");
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2, rt3), "rt, rt2, and rt3 must be sequential");

    constexpr uint32_t Op = 0b0000'1101'1 << 23;
    uint32_t Q;
    uint32_t R = 0;
    uint32_t opcode;
    uint32_t S;
    uint32_t Size;
    if (size == SubRegSize::i8Bit) {
      LOGMAN_THROW_A_FMT(Index < 16, "Index too large");
      Q = Index >> 3;
      S = (Index >> 2) & 1;
      opcode = 0b001;
      Size = Index & 0b11;
    } else if (size == SubRegSize::i16Bit) {
      LOGMAN_THROW_A_FMT(Index < 8, "Index too large");
      Q = Index >> 2;
      S = (Index >> 1) & 1;
      opcode = 0b011;
      Size = (Index & 0b1) << 1;
    } else if (size == SubRegSize::i32Bit) {
      LOGMAN_THROW_A_FMT(Index < 4, "Index too large");
      Q = Index >> 1;
      S = Index & 1;
      opcode = 0b101;
      Size = 0b00;
    } else if (size == SubRegSize::i64Bit) {
      LOGMAN_THROW_A_FMT(Index < 2, "Index too large");
      Q = Index;
      S = 0;
      opcode = 0b101;
      Size = 0b01;
    } else {
      LOGMAN_MSG_A_FMT("Unknown size");
      FEX_UNREACHABLE;
    }

    ASIMDLoadStoreSinglePost(Op, Q, 1, R, opcode, S, Size, Reg::r31, rn, rt.Q());
  }
  template<typename T>
  void st4(SubRegSize size, T rt, T rt2, T rt3, T rt4, uint32_t Index, Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_A_FMT(size == SubRegSize::i8Bit || size == SubRegSize::i16Bit || size == SubRegSize::i32Bit || size == SubRegSize::i64Bit,
                       "Incorrect size");
    LOGMAN_THROW_A_FMT((PostOffset * 8) == (SubRegSizeInBits(size) * 4), "Post-Index size must match element size");
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2, rt3, rt4), "rt, rt2, rt3, and rt4 must be sequential");

    constexpr uint32_t Op = 0b0000'1101'1 << 23;
    uint32_t Q;
    uint32_t R = 1;
    uint32_t opcode;
    uint32_t S;
    uint32_t Size;
    if (size == SubRegSize::i8Bit) {
      LOGMAN_THROW_A_FMT(Index < 16, "Index too large");
      Q = Index >> 3;
      S = (Index >> 2) & 1;
      opcode = 0b001;
      Size = Index & 0b11;
    } else if (size == SubRegSize::i16Bit) {
      LOGMAN_THROW_A_FMT(Index < 8, "Index too large");
      Q = Index >> 2;
      S = (Index >> 1) & 1;
      opcode = 0b011;
      Size = (Index & 0b1) << 1;
    } else if (size == SubRegSize::i32Bit) {
      LOGMAN_THROW_A_FMT(Index < 4, "Index too large");
      Q = Index >> 1;
      S = Index & 1;
      opcode = 0b101;
      Size = 0b00;
    } else if (size == SubRegSize::i64Bit) {
      LOGMAN_THROW_A_FMT(Index < 2, "Index too large");
      Q = Index;
      S = 0;
      opcode = 0b101;
      Size = 0b01;
    } else {
      LOGMAN_MSG_A_FMT("Unknown size");
      FEX_UNREACHABLE;
    }

    ASIMDLoadStoreSinglePost(Op, Q, 0, R, opcode, S, Size, Reg::r31, rn, rt.Q());
  }
  template<typename T>
  void ld4(SubRegSize size, T rt, T rt2, T rt3, T rt4, uint32_t Index, Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_A_FMT(size == SubRegSize::i8Bit || size == SubRegSize::i16Bit || size == SubRegSize::i32Bit || size == SubRegSize::i64Bit,
                       "Incorrect size");
    LOGMAN_THROW_A_FMT((PostOffset * 8) == (SubRegSizeInBits(size) * 4), "Post-Index size must match element size");
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2, rt3, rt4), "rt, rt2, rt3, and rt4 must be sequential");

    constexpr uint32_t Op = 0b0000'1101'1 << 23;
    uint32_t Q;
    uint32_t R = 1;
    uint32_t opcode;
    uint32_t S;
    uint32_t Size;
    if (size == SubRegSize::i8Bit) {
      LOGMAN_THROW_A_FMT(Index < 16, "Index too large");
      Q = Index >> 3;
      S = (Index >> 2) & 1;
      opcode = 0b001;
      Size = Index & 0b11;
    } else if (size == SubRegSize::i16Bit) {
      LOGMAN_THROW_A_FMT(Index < 8, "Index too large");
      Q = Index >> 2;
      S = (Index >> 1) & 1;
      opcode = 0b011;
      Size = (Index & 0b1) << 1;
    } else if (size == SubRegSize::i32Bit) {
      LOGMAN_THROW_A_FMT(Index < 4, "Index too large");
      Q = Index >> 1;
      S = Index & 1;
      opcode = 0b101;
      Size = 0b00;
    } else if (size == SubRegSize::i64Bit) {
      LOGMAN_THROW_A_FMT(Index < 2, "Index too large");
      Q = Index;
      S = 0;
      opcode = 0b101;
      Size = 0b01;
    } else {
      LOGMAN_MSG_A_FMT("Unknown size");
      FEX_UNREACHABLE;
    }

    ASIMDLoadStoreSinglePost(Op, Q, 1, R, opcode, S, Size, Reg::r31, rn, rt.Q());
  }

  template<typename T>
  void st1(ARMEmitter::SubRegSize size, T rt, uint32_t Index, ARMEmitter::Register rn, ARMEmitter::Register rm) {
    LOGMAN_THROW_A_FMT(size == SubRegSize::i8Bit || size == SubRegSize::i16Bit || size == SubRegSize::i32Bit || size == SubRegSize::i64Bit,
                       "Incorrect size");
    constexpr uint32_t Op = 0b0000'1101'1 << 23;
    uint32_t Q;
    uint32_t R = 0;
    uint32_t opcode;
    uint32_t S;
    uint32_t Size;
    if (size == SubRegSize::i8Bit) {
      LOGMAN_THROW_A_FMT(Index < 16, "Index too large");
      Q = Index >> 3;
      S = (Index >> 2) & 1;
      opcode = 0b000;
      Size = Index & 0b11;
    } else if (size == SubRegSize::i16Bit) {
      LOGMAN_THROW_A_FMT(Index < 8, "Index too large");
      Q = Index >> 2;
      S = (Index >> 1) & 1;
      opcode = 0b010;
      Size = (Index & 0b1) << 1;
    } else if (size == SubRegSize::i32Bit) {
      LOGMAN_THROW_A_FMT(Index < 4, "Index too large");
      Q = Index >> 1;
      S = Index & 1;
      opcode = 0b100;
      Size = 0b00;
    } else if (size == SubRegSize::i64Bit) {
      LOGMAN_THROW_A_FMT(Index < 2, "Index too large");
      Q = Index;
      S = 0;
      opcode = 0b100;
      Size = 0b01;
    } else {
      LOGMAN_MSG_A_FMT("Unknown size");
      FEX_UNREACHABLE;
    }

    ASIMDLoadStoreSinglePost(Op, Q, 0, R, opcode, S, Size, rm, rn, rt.Q());
  }
  template<typename T>
  void ld1(ARMEmitter::SubRegSize size, T rt, uint32_t Index, ARMEmitter::Register rn, ARMEmitter::Register rm) {
    LOGMAN_THROW_A_FMT(size == SubRegSize::i8Bit || size == SubRegSize::i16Bit || size == SubRegSize::i32Bit || size == SubRegSize::i64Bit,
                       "Incorrect size");
    constexpr uint32_t Op = 0b0000'1101'1 << 23;
    uint32_t Q;
    uint32_t R = 0;
    uint32_t opcode;
    uint32_t S;
    uint32_t Size;
    if (size == SubRegSize::i8Bit) {
      LOGMAN_THROW_A_FMT(Index < 16, "Index too large");
      Q = Index >> 3;
      S = (Index >> 2) & 1;
      opcode = 0b001;
      Size = Index & 0b11;
    } else if (size == SubRegSize::i16Bit) {
      LOGMAN_THROW_A_FMT(Index < 8, "Index too large");
      Q = Index >> 2;
      S = (Index >> 1) & 1;
      opcode = 0b011;
      Size = (Index & 0b1) << 1;
    } else if (size == SubRegSize::i32Bit) {
      LOGMAN_THROW_A_FMT(Index < 4, "Index too large");
      Q = Index >> 1;
      S = Index & 1;
      opcode = 0b100;
      Size = 0b00;
    } else if (size == SubRegSize::i64Bit) {
      LOGMAN_THROW_A_FMT(Index < 2, "Index too large");
      Q = Index;
      S = 0;
      opcode = 0b101;
      Size = 0b01;
    } else {
      LOGMAN_MSG_A_FMT("Unknown size");
      FEX_UNREACHABLE;
    }

    ASIMDLoadStoreSinglePost(Op, Q, 1, R, opcode, S, Size, rm, rn, rt.Q());
  }
  template<typename T>
  void ld1r(SubRegSize size, T rt, Register rn, Register rm) {
    constexpr uint32_t Op = 0b0000'1101'1 << 23;
    constexpr uint32_t Q = std::is_same_v<QRegister, T> ? 1 : 0;
    uint32_t R = 0;
    uint32_t opcode = 0b110;
    uint32_t S = 0;
    uint32_t Size = FEXCore::ToUnderlying(size);
    ASIMDLoadStoreSinglePost<T>(Op, Q, 1, R, opcode, S, Size, rm, rn, rt);
  }

  template<typename T>
  void ld2r(SubRegSize size, T rt, T rt2, Register rn, Register rm) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2), "rt and rt2 must be sequential");
    constexpr uint32_t Op = 0b0000'1101'1 << 23;
    constexpr uint32_t Q = std::is_same_v<QRegister, T> ? 1 : 0;
    uint32_t R = 1;
    uint32_t opcode = 0b110;
    uint32_t S = 0;
    uint32_t Size = FEXCore::ToUnderlying(size);
    ASIMDLoadStoreSinglePost<T>(Op, Q, 1, R, opcode, S, Size, rm, rn, rt);
  }

  template<typename T>
  void ld3r(SubRegSize size, T rt, T rt2, T rt3, Register rn, Register rm) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2, rt3), "rt, rt2, and rt3 must be sequential");
    constexpr uint32_t Op = 0b0000'1101'1 << 23;
    constexpr uint32_t Q = std::is_same_v<QRegister, T> ? 1 : 0;
    uint32_t R = 0;
    uint32_t opcode = 0b111;
    uint32_t S = 0;
    uint32_t Size = FEXCore::ToUnderlying(size);
    ASIMDLoadStoreSinglePost<T>(Op, Q, 1, R, opcode, S, Size, rm, rn, rt);
  }
  template<typename T>
  void ld4r(SubRegSize size, T rt, T rt2, T rt3, T rt4, Register rn, Register rm) {
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2, rt3, rt4), "rt, rt2, rt3, and rt4 must be sequential");
    constexpr uint32_t Op = 0b0000'1101'1 << 23;
    constexpr uint32_t Q = std::is_same_v<QRegister, T> ? 1 : 0;
    uint32_t R = 1;
    uint32_t opcode = 0b111;
    uint32_t S = 0;
    uint32_t Size = FEXCore::ToUnderlying(size);
    ASIMDLoadStoreSinglePost<T>(Op, Q, 1, R, opcode, S, Size, rm, rn, rt);
  }

  template<typename T>
  void st2(SubRegSize size, T rt, T rt2, uint32_t Index, Register rn, Register rm) {
    LOGMAN_THROW_A_FMT(size == SubRegSize::i8Bit || size == SubRegSize::i16Bit || size == SubRegSize::i32Bit || size == SubRegSize::i64Bit,
                       "Incorrect size");
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2), "rt and rt2 must be sequential");

    constexpr uint32_t Op = 0b0000'1101'1 << 23;
    uint32_t Q;
    uint32_t R = 1;
    uint32_t opcode;
    uint32_t S;
    uint32_t Size;
    if (size == SubRegSize::i8Bit) {
      LOGMAN_THROW_A_FMT(Index < 16, "Index too large");
      Q = Index >> 3;
      S = (Index >> 2) & 1;
      opcode = 0b000;
      Size = Index & 0b11;
    } else if (size == SubRegSize::i16Bit) {
      LOGMAN_THROW_A_FMT(Index < 8, "Index too large");
      Q = Index >> 2;
      S = (Index >> 1) & 1;
      opcode = 0b010;
      Size = (Index & 0b1) << 1;
    } else if (size == SubRegSize::i32Bit) {
      LOGMAN_THROW_A_FMT(Index < 4, "Index too large");
      Q = Index >> 1;
      S = Index & 1;
      opcode = 0b100;
      Size = 0b00;
    } else if (size == SubRegSize::i64Bit) {
      LOGMAN_THROW_A_FMT(Index < 2, "Index too large");
      Q = Index;
      S = 0;
      opcode = 0b100;
      Size = 0b01;
    } else {
      LOGMAN_MSG_A_FMT("Unknown size");
      FEX_UNREACHABLE;
    }

    ASIMDLoadStoreSinglePost(Op, Q, 0, R, opcode, S, Size, rm, rn, rt.Q());
  }
  template<typename T>
  void ld2(SubRegSize size, T rt, T rt2, uint32_t Index, Register rn, Register rm) {
    LOGMAN_THROW_A_FMT(size == SubRegSize::i8Bit || size == SubRegSize::i16Bit || size == SubRegSize::i32Bit || size == SubRegSize::i64Bit,
                       "Incorrect size");
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2), "rt and rt2 must be sequential");

    constexpr uint32_t Op = 0b0000'1101'1 << 23;
    uint32_t Q;
    uint32_t R = 1;
    uint32_t opcode;
    uint32_t S;
    uint32_t Size;
    if (size == SubRegSize::i8Bit) {
      LOGMAN_THROW_A_FMT(Index < 16, "Index too large");
      Q = Index >> 3;
      S = (Index >> 2) & 1;
      opcode = 0b000;
      Size = Index & 0b11;
    } else if (size == SubRegSize::i16Bit) {
      LOGMAN_THROW_A_FMT(Index < 8, "Index too large");
      Q = Index >> 2;
      S = (Index >> 1) & 1;
      opcode = 0b010;
      Size = (Index & 0b1) << 1;
    } else if (size == SubRegSize::i32Bit) {
      LOGMAN_THROW_A_FMT(Index < 4, "Index too large");
      Q = Index >> 1;
      S = Index & 1;
      opcode = 0b100;
      Size = 0b00;
    } else if (size == SubRegSize::i64Bit) {
      LOGMAN_THROW_A_FMT(Index < 2, "Index too large");
      Q = Index;
      S = 0;
      opcode = 0b100;
      Size = 0b01;
    } else {
      LOGMAN_MSG_A_FMT("Unknown size");
      FEX_UNREACHABLE;
    }

    ASIMDLoadStoreSinglePost(Op, Q, 1, R, opcode, S, Size, rm, rn, rt.Q());
  }
  template<typename T>
  void st3(SubRegSize size, T rt, T rt2, T rt3, uint32_t Index, Register rn, Register rm) {
    LOGMAN_THROW_A_FMT(size == SubRegSize::i8Bit || size == SubRegSize::i16Bit || size == SubRegSize::i32Bit || size == SubRegSize::i64Bit,
                       "Incorrect size");
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2, rt3), "rt, rt2, and rt3 must be sequential");

    constexpr uint32_t Op = 0b0000'1101'1 << 23;
    uint32_t Q;
    uint32_t R = 0;
    uint32_t opcode;
    uint32_t S;
    uint32_t Size;
    if (size == SubRegSize::i8Bit) {
      LOGMAN_THROW_A_FMT(Index < 16, "Index too large");
      Q = Index >> 3;
      S = (Index >> 2) & 1;
      opcode = 0b001;
      Size = Index & 0b11;
    } else if (size == SubRegSize::i16Bit) {
      LOGMAN_THROW_A_FMT(Index < 8, "Index too large");
      Q = Index >> 2;
      S = (Index >> 1) & 1;
      opcode = 0b011;
      Size = (Index & 0b1) << 1;
    } else if (size == SubRegSize::i32Bit) {
      LOGMAN_THROW_A_FMT(Index < 4, "Index too large");
      Q = Index >> 1;
      S = Index & 1;
      opcode = 0b101;
      Size = 0b00;
    } else if (size == SubRegSize::i64Bit) {
      LOGMAN_THROW_A_FMT(Index < 2, "Index too large");
      Q = Index;
      S = 0;
      opcode = 0b101;
      Size = 0b01;
    } else {
      LOGMAN_MSG_A_FMT("Unknown size");
      FEX_UNREACHABLE;
    }

    ASIMDLoadStoreSinglePost(Op, Q, 0, R, opcode, S, Size, rm, rn, rt.Q());
  }
  template<typename T>
  void ld3(SubRegSize size, T rt, T rt2, T rt3, uint32_t Index, Register rn, Register rm) {
    LOGMAN_THROW_A_FMT(size == SubRegSize::i8Bit || size == SubRegSize::i16Bit || size == SubRegSize::i32Bit || size == SubRegSize::i64Bit,
                       "Incorrect size");
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2, rt3), "rt, rt2, and rt3 must be sequential");

    constexpr uint32_t Op = 0b0000'1101'1 << 23;
    uint32_t Q;
    uint32_t R = 0;
    uint32_t opcode;
    uint32_t S;
    uint32_t Size;
    if (size == SubRegSize::i8Bit) {
      LOGMAN_THROW_A_FMT(Index < 16, "Index too large");
      Q = Index >> 3;
      S = (Index >> 2) & 1;
      opcode = 0b001;
      Size = Index & 0b11;
    } else if (size == SubRegSize::i16Bit) {
      LOGMAN_THROW_A_FMT(Index < 8, "Index too large");
      Q = Index >> 2;
      S = (Index >> 1) & 1;
      opcode = 0b011;
      Size = (Index & 0b1) << 1;
    } else if (size == SubRegSize::i32Bit) {
      LOGMAN_THROW_A_FMT(Index < 4, "Index too large");
      Q = Index >> 1;
      S = Index & 1;
      opcode = 0b101;
      Size = 0b00;
    } else if (size == SubRegSize::i64Bit) {
      LOGMAN_THROW_A_FMT(Index < 2, "Index too large");
      Q = Index;
      S = 0;
      opcode = 0b101;
      Size = 0b01;
    } else {
      LOGMAN_MSG_A_FMT("Unknown size");
      FEX_UNREACHABLE;
    }

    ASIMDLoadStoreSinglePost(Op, Q, 1, R, opcode, S, Size, rm, rn, rt.Q());
  }
  template<typename T>
  void st4(SubRegSize size, T rt, T rt2, T rt3, T rt4, uint32_t Index, Register rn, Register rm) {
    LOGMAN_THROW_A_FMT(size == SubRegSize::i8Bit || size == SubRegSize::i16Bit || size == SubRegSize::i32Bit || size == SubRegSize::i64Bit,
                       "Incorrect size");
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2, rt3, rt4), "rt, rt2, rt3, and rt4 must be sequential");

    constexpr uint32_t Op = 0b0000'1101'1 << 23;
    uint32_t Q;
    uint32_t R = 1;
    uint32_t opcode;
    uint32_t S;
    uint32_t Size;
    if (size == SubRegSize::i8Bit) {
      LOGMAN_THROW_A_FMT(Index < 16, "Index too large");
      Q = Index >> 3;
      S = (Index >> 2) & 1;
      opcode = 0b001;
      Size = Index & 0b11;
    } else if (size == SubRegSize::i16Bit) {
      LOGMAN_THROW_A_FMT(Index < 8, "Index too large");
      Q = Index >> 2;
      S = (Index >> 1) & 1;
      opcode = 0b011;
      Size = (Index & 0b1) << 1;
    } else if (size == SubRegSize::i32Bit) {
      LOGMAN_THROW_A_FMT(Index < 4, "Index too large");
      Q = Index >> 1;
      S = Index & 1;
      opcode = 0b101;
      Size = 0b00;
    } else if (size == SubRegSize::i64Bit) {
      LOGMAN_THROW_A_FMT(Index < 2, "Index too large");
      Q = Index;
      S = 0;
      opcode = 0b101;
      Size = 0b01;
    } else {
      LOGMAN_MSG_A_FMT("Unknown size");
      FEX_UNREACHABLE;
    }

    ASIMDLoadStoreSinglePost(Op, Q, 0, R, opcode, S, Size, rm, rn, rt.Q());
  }
  template<typename T>
  void ld4(SubRegSize size, T rt, T rt2, T rt3, T rt4, uint32_t Index, Register rn, Register rm) {
    LOGMAN_THROW_A_FMT(size == SubRegSize::i8Bit || size == SubRegSize::i16Bit || size == SubRegSize::i32Bit || size == SubRegSize::i64Bit,
                       "Incorrect size");
    LOGMAN_THROW_A_FMT(AreVectorsSequential(rt, rt2, rt3, rt4), "rt, rt2, rt3, and rt4 must be sequential");

    constexpr uint32_t Op = 0b0000'1101'1 << 23;
    uint32_t Q;
    uint32_t R = 1;
    uint32_t opcode;
    uint32_t S;
    uint32_t Size;
    if (size == SubRegSize::i8Bit) {
      LOGMAN_THROW_A_FMT(Index < 16, "Index too large");
      Q = Index >> 3;
      S = (Index >> 2) & 1;
      opcode = 0b001;
      Size = Index & 0b11;
    } else if (size == SubRegSize::i16Bit) {
      LOGMAN_THROW_A_FMT(Index < 8, "Index too large");
      Q = Index >> 2;
      S = (Index >> 1) & 1;
      opcode = 0b011;
      Size = (Index & 0b1) << 1;
    } else if (size == SubRegSize::i32Bit) {
      LOGMAN_THROW_A_FMT(Index < 4, "Index too large");
      Q = Index >> 1;
      S = Index & 1;
      opcode = 0b101;
      Size = 0b00;
    } else if (size == SubRegSize::i64Bit) {
      LOGMAN_THROW_A_FMT(Index < 2, "Index too large");
      Q = Index;
      S = 0;
      opcode = 0b101;
      Size = 0b01;
    } else {
      LOGMAN_MSG_A_FMT("Unknown size");
      FEX_UNREACHABLE;
    }

    ASIMDLoadStoreSinglePost(Op, Q, 1, R, opcode, S, Size, rm, rn, rt.Q());
  }

  template<ARMEmitter::SubRegSize size, typename T>
  void st1(T rt, uint32_t Index, ARMEmitter::Register rn, uint32_t PostOffset) {
    st1(size, rt, Index, rn, PostOffset);
  }
  template<ARMEmitter::SubRegSize size, typename T>
  void ld1(T rt, uint32_t Index, ARMEmitter::Register rn, uint32_t PostOffset) {
    ld1(size, rt, Index, rn, PostOffset);
  }
  template<ARMEmitter::SubRegSize size, typename T>
  void ld1r(T rt, ARMEmitter::Register rn, uint32_t PostOffset) {
    ld1r(size, rt, rn, PostOffset);
  }
  template<ARMEmitter::SubRegSize size, typename T>
  void ld2r(T rt, T rt2, ARMEmitter::Register rn, uint32_t PostOffset) {
    ld2r(size, rt, rt2, rn, PostOffset);
  }
  template<ARMEmitter::SubRegSize size, typename T>
  void ld3r(T rt, T rt2, T rt3, ARMEmitter::Register rn, uint32_t PostOffset) {
    ld3r(size, rt, rt2, rt3, rn, PostOffset);
  }
  template<ARMEmitter::SubRegSize size, typename T>
  void ld4r(T rt, T rt2, T rt3, T rt4, ARMEmitter::Register rn, uint32_t PostOffset) {
    ld4r(size, rt, rt2, rt3, rt4, rn, PostOffset);
  }

  template<ARMEmitter::SubRegSize size, typename T>
  void st2(T rt, T rt2, uint32_t Index, ARMEmitter::Register rn, uint32_t PostOffset) {
    st2(size, rt, rt2, Index, rn, PostOffset);
  }
  template<ARMEmitter::SubRegSize size, typename T>
  void ld2(T rt, T rt2, uint32_t Index, ARMEmitter::Register rn, uint32_t PostOffset) {
    ld2(size, rt, rt2, Index, rn, PostOffset);
  }
  template<ARMEmitter::SubRegSize size, typename T>
  void st3(T rt, T rt2, T rt3, uint32_t Index, ARMEmitter::Register rn, uint32_t PostOffset) {
    st3(size, rt, rt2, rt3, Index, rn, PostOffset);
  }
  template<ARMEmitter::SubRegSize size, typename T>
  void ld3(T rt, T rt2, T rt3, uint32_t Index, ARMEmitter::Register rn, uint32_t PostOffset) {
    ld3(size, rt, rt2, rt3, Index, rn, PostOffset);
  }
  template<ARMEmitter::SubRegSize size, typename T>
  void st4(T rt, T rt2, T rt3, T rt4, uint32_t Index, ARMEmitter::Register rn, uint32_t PostOffset) {
    st4(size, rt, rt2, rt3, rt4, Index, rn, PostOffset);
  }
  template<ARMEmitter::SubRegSize size, typename T>
  void ld4(T rt, T rt2, T rt3, T rt4, uint32_t Index, ARMEmitter::Register rn, uint32_t PostOffset) {
    ld4(size, rt, rt2, rt3, rt4, Index, rn, PostOffset);
  }

  template<ARMEmitter::SubRegSize size, typename T>
  void st1(T rt, uint32_t Index, ARMEmitter::Register rn, ARMEmitter::Register rm) {
    st1(size, rt, Index, rn, rm);
  }
  template<ARMEmitter::SubRegSize size, typename T>
  void ld1(T rt, uint32_t Index, ARMEmitter::Register rn, ARMEmitter::Register rm) {
    ld1(size, rt, Index, rn, rm);
  }
  template<ARMEmitter::SubRegSize size, typename T>
  void ld1r(T rt, ARMEmitter::Register rn, ARMEmitter::Register rm) {
    ld1r(size, rt, rn, rm);
  }
  template<ARMEmitter::SubRegSize size, typename T>
  void ld2r(T rt, T rt2, ARMEmitter::Register rn, ARMEmitter::Register rm) {
    ld2r(size, rt, rt2, rn, rm);
  }
  template<ARMEmitter::SubRegSize size, typename T>
  void ld3r(T rt, T rt2, T rt3, ARMEmitter::Register rn, ARMEmitter::Register rm) {
    ld3r(size, rt, rt2, rt3, rn, rm);
  }
  template<ARMEmitter::SubRegSize size, typename T>
  void ld4r(T rt, T rt2, T rt3, T rt4, ARMEmitter::Register rn, ARMEmitter::Register rm) {
    ld4r(size, rt, rt2, rt3, rt4, rn, rm);
  }

  template<ARMEmitter::SubRegSize size, typename T>
  void st2(T rt, T rt2, uint32_t Index, ARMEmitter::Register rn, ARMEmitter::Register rm) {
    st2(size, rt, rt2, Index, rn, rm);
  }
  template<ARMEmitter::SubRegSize size, typename T>
  void ld2(T rt, T rt2, uint32_t Index, ARMEmitter::Register rn, ARMEmitter::Register rm) {
    ld2(size, rt, rt2, Index, rn, rm);
  }
  template<ARMEmitter::SubRegSize size, typename T>
  void st3(T rt, T rt2, T rt3, uint32_t Index, ARMEmitter::Register rn, ARMEmitter::Register rm) {
    st3(size, rt, rt2, rt3, Index, rn, rm);
  }
  template<ARMEmitter::SubRegSize size, typename T>
  void ld3(T rt, T rt2, T rt3, uint32_t Index, ARMEmitter::Register rn, ARMEmitter::Register rm) {
    ld3(size, rt, rt2, rt3, Index, rn, rm);
  }
  template<ARMEmitter::SubRegSize size, typename T>
  void st4(T rt, T rt2, T rt3, T rt4, uint32_t Index, ARMEmitter::Register rn, ARMEmitter::Register rm) {
    st4(size, rt, rt2, rt3, rt4, Index, rn, rm);
  }
  template<ARMEmitter::SubRegSize size, typename T>
  void ld4(T rt, T rt2, T rt3, T rt4, uint32_t Index, ARMEmitter::Register rn, ARMEmitter::Register rm) {
    ld4(size, rt, rt2, rt3, rt4, Index, rn, rm);
  }

  template<typename T>
  void ASIMDLoadStoreSinglePost(uint32_t Op, uint32_t Q, uint32_t L, uint32_t R, uint32_t opcode, uint32_t S, uint32_t size,
                                ARMEmitter::Register rm, ARMEmitter::Register rn, T rt) {
    LOGMAN_THROW_A_FMT((std::is_same_v<ARMEmitter::QRegister, T> || std::is_same_v<ARMEmitter::DRegister, T>), "Only supports 128-bit and "
                                                                                                               "64-bit vector registers.");
    uint32_t Instr = Op;

    Instr |= Q << 30;
    Instr |= L << 22;
    Instr |= R << 21;
    Instr |= Encode_rm(rm);
    Instr |= opcode << 13;
    Instr |= S << 12;
    Instr |= size << 10;
    Instr |= Encode_rn(rn);
    Instr |= Encode_rt(rt);
    dc32(Instr);
  }
  // Loadstore exclusive pair
  void stxp(ARMEmitter::Size s, ARMEmitter::Register rs, ARMEmitter::Register rt, ARMEmitter::Register rt2, ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b1000'1000'001 << 21;
    AtomicOp(Op, s, 0, 0, rs, rt, rt2, rn);
  }
  void stlxp(ARMEmitter::Size s, ARMEmitter::Register rs, ARMEmitter::Register rt, ARMEmitter::Register rt2, ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b1000'1000'001 << 21;
    AtomicOp(Op, s, 0, 1, rs, rt, rt2, rn);
  }
  void ldxp(ARMEmitter::Size s, ARMEmitter::Register rt, ARMEmitter::Register rt2, ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b1000'1000'001 << 21;
    AtomicOp(Op, s, 1, 0, ARMEmitter::Reg::r31, rt, rt2, rn);
  }
  void ldaxp(ARMEmitter::Size s, ARMEmitter::Register rt, ARMEmitter::Register rt2, ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b1000'1000'001 << 21;
    AtomicOp(Op, s, 1, 1, ARMEmitter::Reg::r31, rt, rt2, rn);
  }
  // Loadstore exclusive register
  void stxrb(ARMEmitter::Register rs, ARMEmitter::Register rt, ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'000 << 21;
    SubAtomicOp(Op, ARMEmitter::SubRegSize::i8Bit, 0, 0, rs, rt, ARMEmitter::Reg::r31, rn);
  }
  void stlxrb(ARMEmitter::Register rs, ARMEmitter::Register rt, ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'000 << 21;
    SubAtomicOp(Op, ARMEmitter::SubRegSize::i8Bit, 0, 1, rs, rt, ARMEmitter::Reg::r31, rn);
  }
  void ldxrb(ARMEmitter::Register rt, ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'000 << 21;
    SubAtomicOp(Op, ARMEmitter::SubRegSize::i8Bit, 1, 0, ARMEmitter::Reg::r31, rt, ARMEmitter::Reg::r31, rn);
  }
  void ldaxrb(ARMEmitter::Register rt, ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'000 << 21;
    SubAtomicOp(Op, ARMEmitter::SubRegSize::i8Bit, 1, 1, ARMEmitter::Reg::r31, rt, ARMEmitter::Reg::r31, rn);
  }
  void stxrh(ARMEmitter::Register rs, ARMEmitter::Register rt, ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'000 << 21;
    SubAtomicOp(Op, ARMEmitter::SubRegSize::i16Bit, 0, 0, rs, rt, ARMEmitter::Reg::r31, rn);
  }
  void stlxrh(ARMEmitter::Register rs, ARMEmitter::Register rt, ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'000 << 21;
    SubAtomicOp(Op, ARMEmitter::SubRegSize::i16Bit, 0, 1, rs, rt, ARMEmitter::Reg::r31, rn);
  }
  void ldxrh(ARMEmitter::Register rt, ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'000 << 21;
    SubAtomicOp(Op, ARMEmitter::SubRegSize::i16Bit, 1, 0, ARMEmitter::Reg::r31, rt, ARMEmitter::Reg::r31, rn);
  }
  void ldaxrh(ARMEmitter::Register rt, ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'000 << 21;
    SubAtomicOp(Op, ARMEmitter::SubRegSize::i16Bit, 1, 1, ARMEmitter::Reg::r31, rt, ARMEmitter::Reg::r31, rn);
  }
  void stxr(ARMEmitter::WRegister rs, ARMEmitter::WRegister rt, ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'000 << 21;
    SubAtomicOp(Op, ARMEmitter::SubRegSize::i32Bit, 0, 0, rs, rt, ARMEmitter::WReg::w31, rn);
  }
  void stlxr(ARMEmitter::WRegister rs, ARMEmitter::WRegister rt, ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'000 << 21;
    SubAtomicOp(Op, ARMEmitter::SubRegSize::i32Bit, 0, 1, rs, rt, ARMEmitter::WReg::w31, rn);
  }
  void ldxr(ARMEmitter::WRegister rt, ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'000 << 21;
    SubAtomicOp(Op, ARMEmitter::SubRegSize::i32Bit, 1, 0, ARMEmitter::WReg::w31, rt, ARMEmitter::WReg::w31, rn);
  }
  void ldaxr(ARMEmitter::WRegister rt, ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'000 << 21;
    SubAtomicOp(Op, ARMEmitter::SubRegSize::i32Bit, 1, 1, ARMEmitter::WReg::w31, rt, ARMEmitter::WReg::w31, rn);
  }
  void stxr(ARMEmitter::XRegister rs, ARMEmitter::XRegister rt, ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'000 << 21;
    SubAtomicOp(Op, ARMEmitter::SubRegSize::i64Bit, 0, 0, rs, rt, ARMEmitter::XReg::x31, rn);
  }
  void stlxr(ARMEmitter::WRegister rs, ARMEmitter::XRegister rt, ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'000 << 21;
    SubAtomicOp(Op, ARMEmitter::SubRegSize::i64Bit, 0, 1, rs.R(), rt.R(), ARMEmitter::Reg::r31, rn);
  }
  void ldxr(ARMEmitter::XRegister rt, ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'000 << 21;
    SubAtomicOp(Op, ARMEmitter::SubRegSize::i64Bit, 1, 0, ARMEmitter::XReg::x31, rt, ARMEmitter::XReg::x31, rn);
  }
  void ldaxr(ARMEmitter::XRegister rt, ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'000 << 21;
    SubAtomicOp(Op, ARMEmitter::SubRegSize::i64Bit, 1, 1, ARMEmitter::XReg::x31, rt, ARMEmitter::XReg::x31, rn);
  }
  void stxr(ARMEmitter::SubRegSize size, ARMEmitter::Register rs, ARMEmitter::Register rt, ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'000 << 21;
    SubAtomicOp(Op, size, 0, 0, rs, rt, ARMEmitter::Reg::r31, rn);
  }
  void stlxr(ARMEmitter::SubRegSize size, ARMEmitter::Register rs, ARMEmitter::Register rt, ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'000 << 21;
    SubAtomicOp(Op, size, 0, 1, rs, rt, ARMEmitter::Reg::r31, rn);
  }
  void ldxr(ARMEmitter::SubRegSize size, ARMEmitter::Register rt, ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'000 << 21;
    SubAtomicOp(Op, size, 1, 0, ARMEmitter::Reg::r31, rt, ARMEmitter::Reg::r31, rn);
  }
  void ldaxr(ARMEmitter::SubRegSize size, ARMEmitter::Register rt, ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'000 << 21;
    SubAtomicOp(Op, size, 1, 1, ARMEmitter::Reg::r31, rt, ARMEmitter::Reg::r31, rn);
  }

  // Load/store ordered
  static constexpr uint32_t LoadStoreOrdered_Op = 0b0000'1000'100 << 21;
  void stllrb(ARMEmitter::Register rt, ARMEmitter::Register rn) {
    SubAtomicOp(LoadStoreOrdered_Op, ARMEmitter::SubRegSize::i8Bit, 0, 0, ARMEmitter::Reg::r31, rt, ARMEmitter::Reg::r31, rn);
  }
  void stlrb(ARMEmitter::Register rt, ARMEmitter::Register rn) {
    SubAtomicOp(LoadStoreOrdered_Op, ARMEmitter::SubRegSize::i8Bit, 0, 1, ARMEmitter::Reg::r31, rt, ARMEmitter::Reg::r31, rn);
  }
  void ldlarb(ARMEmitter::Register rt, ARMEmitter::Register rn) {
    SubAtomicOp(LoadStoreOrdered_Op, ARMEmitter::SubRegSize::i8Bit, 1, 0, ARMEmitter::Reg::r31, rt, ARMEmitter::Reg::r31, rn);
  }
  void ldarb(ARMEmitter::Register rt, ARMEmitter::Register rn) {
    SubAtomicOp(LoadStoreOrdered_Op, ARMEmitter::SubRegSize::i8Bit, 1, 1, ARMEmitter::Reg::r31, rt, ARMEmitter::Reg::r31, rn);
  }
  void stllrh(ARMEmitter::Register rt, ARMEmitter::Register rn) {
    SubAtomicOp(LoadStoreOrdered_Op, ARMEmitter::SubRegSize::i16Bit, 0, 0, ARMEmitter::Reg::r31, rt, ARMEmitter::Reg::r31, rn);
  }
  void stlrh(ARMEmitter::Register rt, ARMEmitter::Register rn) {
    SubAtomicOp(LoadStoreOrdered_Op, ARMEmitter::SubRegSize::i16Bit, 0, 1, ARMEmitter::Reg::r31, rt, ARMEmitter::Reg::r31, rn);
  }
  void ldlarh(ARMEmitter::Register rt, ARMEmitter::Register rn) {
    SubAtomicOp(LoadStoreOrdered_Op, ARMEmitter::SubRegSize::i16Bit, 1, 0, ARMEmitter::Reg::r31, rt, ARMEmitter::Reg::r31, rn);
  }
  void ldarh(ARMEmitter::Register rt, ARMEmitter::Register rn) {
    SubAtomicOp(LoadStoreOrdered_Op, ARMEmitter::SubRegSize::i16Bit, 1, 1, ARMEmitter::Reg::r31, rt, ARMEmitter::Reg::r31, rn);
  }
  void stllr(ARMEmitter::WRegister rt, ARMEmitter::Register rn) {
    SubAtomicOp(LoadStoreOrdered_Op, ARMEmitter::SubRegSize::i32Bit, 0, 0, ARMEmitter::WReg::w31, rt, ARMEmitter::WReg::w31, rn);
  }
  void stlr(ARMEmitter::WRegister rt, ARMEmitter::Register rn) {
    SubAtomicOp(LoadStoreOrdered_Op, ARMEmitter::SubRegSize::i32Bit, 0, 1, ARMEmitter::WReg::w31, rt, ARMEmitter::WReg::w31, rn);
  }
  void ldlar(ARMEmitter::WRegister rt, ARMEmitter::Register rn) {
    SubAtomicOp(LoadStoreOrdered_Op, ARMEmitter::SubRegSize::i32Bit, 1, 0, ARMEmitter::WReg::w31, rt, ARMEmitter::WReg::w31, rn);
  }
  void ldar(ARMEmitter::WRegister rt, ARMEmitter::Register rn) {
    SubAtomicOp(LoadStoreOrdered_Op, ARMEmitter::SubRegSize::i32Bit, 1, 1, ARMEmitter::WReg::w31, rt, ARMEmitter::WReg::w31, rn);
  }
  void stllr(ARMEmitter::XRegister rt, ARMEmitter::Register rn) {
    SubAtomicOp(LoadStoreOrdered_Op, ARMEmitter::SubRegSize::i64Bit, 0, 0, ARMEmitter::XReg::x31, rt, ARMEmitter::XReg::x31, rn);
  }
  void stlr(ARMEmitter::XRegister rt, ARMEmitter::Register rn) {
    SubAtomicOp(LoadStoreOrdered_Op, ARMEmitter::SubRegSize::i64Bit, 0, 1, ARMEmitter::XReg::x31, rt, ARMEmitter::XReg::x31, rn);
  }
  void ldlar(ARMEmitter::XRegister rt, ARMEmitter::Register rn) {
    SubAtomicOp(LoadStoreOrdered_Op, ARMEmitter::SubRegSize::i64Bit, 1, 0, ARMEmitter::XReg::x31, rt, ARMEmitter::XReg::x31, rn);
  }
  void ldar(ARMEmitter::XRegister rt, ARMEmitter::Register rn) {
    SubAtomicOp(LoadStoreOrdered_Op, ARMEmitter::SubRegSize::i64Bit, 1, 1, ARMEmitter::XReg::x31, rt, ARMEmitter::XReg::x31, rn);
  }
  // Compare and swap
  void casb(ARMEmitter::Register rs, ARMEmitter::Register rt, ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'101 << 21;
    SubAtomicOp(Op, ARMEmitter::SubRegSize::i8Bit, 0, 0, rs, rt, ARMEmitter::Reg::r31, rn);
  }
  void caslb(ARMEmitter::Register rs, ARMEmitter::Register rt, ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'101 << 21;
    SubAtomicOp(Op, ARMEmitter::SubRegSize::i8Bit, 0, 1, rs, rt, ARMEmitter::Reg::r31, rn);
  }
  void casab(ARMEmitter::Register rs, ARMEmitter::Register rt, ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'101 << 21;
    SubAtomicOp(Op, ARMEmitter::SubRegSize::i8Bit, 1, 0, rs, rt, ARMEmitter::Reg::r31, rn);
  }
  void casalb(ARMEmitter::Register rs, ARMEmitter::Register rt, ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'101 << 21;
    SubAtomicOp(Op, ARMEmitter::SubRegSize::i8Bit, 1, 1, rs, rt, ARMEmitter::Reg::r31, rn);
  }
  void cash(ARMEmitter::Register rs, ARMEmitter::Register rt, ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'101 << 21;
    SubAtomicOp(Op, ARMEmitter::SubRegSize::i16Bit, 0, 0, rs, rt, ARMEmitter::Reg::r31, rn);
  }
  void caslh(ARMEmitter::Register rs, ARMEmitter::Register rt, ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'101 << 21;
    SubAtomicOp(Op, ARMEmitter::SubRegSize::i16Bit, 0, 1, rs, rt, ARMEmitter::Reg::r31, rn);
  }
  void casah(ARMEmitter::Register rs, ARMEmitter::Register rt, ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'101 << 21;
    SubAtomicOp(Op, ARMEmitter::SubRegSize::i16Bit, 1, 0, rs, rt, ARMEmitter::Reg::r31, rn);
  }
  void casalh(ARMEmitter::Register rs, ARMEmitter::Register rt, ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'101 << 21;
    SubAtomicOp(Op, ARMEmitter::SubRegSize::i16Bit, 1, 1, rs, rt, ARMEmitter::Reg::r31, rn);
  }
  void cas(ARMEmitter::WRegister rs, ARMEmitter::WRegister rt, ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'101 << 21;
    SubAtomicOp(Op, ARMEmitter::SubRegSize::i32Bit, 0, 0, rs.R(), rt.R(), ARMEmitter::Reg::r31, rn);
  }
  void casl(ARMEmitter::WRegister rs, ARMEmitter::WRegister rt, ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'101 << 21;
    SubAtomicOp(Op, ARMEmitter::SubRegSize::i32Bit, 0, 1, rs.R(), rt.R(), ARMEmitter::Reg::r31, rn);
  }
  void casa(ARMEmitter::WRegister rs, ARMEmitter::WRegister rt, ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'101 << 21;
    SubAtomicOp(Op, ARMEmitter::SubRegSize::i32Bit, 1, 0, rs.R(), rt.R(), ARMEmitter::Reg::r31, rn);
  }
  void casal(ARMEmitter::WRegister rs, ARMEmitter::WRegister rt, ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'101 << 21;
    SubAtomicOp(Op, ARMEmitter::SubRegSize::i32Bit, 1, 1, rs.R(), rt.R(), ARMEmitter::Reg::r31, rn);
  }
  void cas(ARMEmitter::XRegister rs, ARMEmitter::XRegister rt, ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'101 << 21;
    SubAtomicOp(Op, ARMEmitter::SubRegSize::i64Bit, 0, 0, rs.R(), rt.R(), ARMEmitter::Reg::r31, rn);
  }
  void casl(ARMEmitter::XRegister rs, ARMEmitter::XRegister rt, ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'101 << 21;
    SubAtomicOp(Op, ARMEmitter::SubRegSize::i64Bit, 0, 1, rs.R(), rt.R(), ARMEmitter::Reg::r31, rn);
  }
  void casa(ARMEmitter::XRegister rs, ARMEmitter::XRegister rt, ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'101 << 21;
    SubAtomicOp(Op, ARMEmitter::SubRegSize::i64Bit, 1, 0, rs.R(), rt.R(), ARMEmitter::Reg::r31, rn);
  }
  void casal(ARMEmitter::XRegister rs, ARMEmitter::XRegister rt, ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'101 << 21;
    SubAtomicOp(Op, ARMEmitter::SubRegSize::i64Bit, 1, 1, rs.R(), rt.R(), ARMEmitter::Reg::r31, rn);
  }

  void cas(ARMEmitter::SubRegSize size, ARMEmitter::Register rs, ARMEmitter::Register rt, ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'101 << 21;
    SubAtomicOp(Op, size, 0, 0, rs, rt, ARMEmitter::Reg::r31, rn);
  }
  void casl(ARMEmitter::SubRegSize size, ARMEmitter::Register rs, ARMEmitter::Register rt, ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'101 << 21;
    SubAtomicOp(Op, size, 0, 1, rs, rt, ARMEmitter::Reg::r31, rn);
  }
  void casa(ARMEmitter::SubRegSize size, ARMEmitter::Register rs, ARMEmitter::Register rt, ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'101 << 21;
    SubAtomicOp(Op, size, 1, 0, rs, rt, ARMEmitter::Reg::r31, rn);
  }
  void casal(ARMEmitter::SubRegSize size, ARMEmitter::Register rs, ARMEmitter::Register rt, ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'101 << 21;
    SubAtomicOp(Op, size, 1, 1, rs, rt, ARMEmitter::Reg::r31, rn);
  }
  // LDAPR/STLR unscaled immediate
  void stlurb(ARMEmitter::Register rt, ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");
    constexpr uint32_t Op = 0b0001'1001'000 << 21;
    SubAtomicImm(Op, ARMEmitter::SubRegSize::i8Bit, 0b00, rt, rn, static_cast<uint32_t>(Imm) & 0x1'FF);
  }
  void ldapurb(ARMEmitter::Register rt, ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");
    constexpr uint32_t Op = 0b0001'1001'000 << 21;
    SubAtomicImm(Op, ARMEmitter::SubRegSize::i8Bit, 0b01, rt, rn, static_cast<uint32_t>(Imm) & 0x1'FF);
  }
  void ldapursb(ARMEmitter::WRegister rt, ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");
    constexpr uint32_t Op = 0b0001'1001'000 << 21;
    SubAtomicImm(Op, ARMEmitter::SubRegSize::i8Bit, 0b11, rt, rn, static_cast<uint32_t>(Imm) & 0x1'FF);
  }
  void ldapursb(ARMEmitter::XRegister rt, ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");
    constexpr uint32_t Op = 0b0001'1001'000 << 21;
    SubAtomicImm(Op, ARMEmitter::SubRegSize::i8Bit, 0b10, rt, rn, static_cast<uint32_t>(Imm) & 0x1'FF);
  }
  void stlurh(ARMEmitter::Register rt, ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");
    constexpr uint32_t Op = 0b0001'1001'000 << 21;
    SubAtomicImm(Op, ARMEmitter::SubRegSize::i16Bit, 0b00, rt, rn, static_cast<uint32_t>(Imm) & 0x1'FF);
  }
  void ldapurh(ARMEmitter::Register rt, ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");
    constexpr uint32_t Op = 0b0001'1001'000 << 21;
    SubAtomicImm(Op, ARMEmitter::SubRegSize::i16Bit, 0b01, rt, rn, static_cast<uint32_t>(Imm) & 0x1'FF);
  }
  void ldapursh(ARMEmitter::WRegister rt, ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");
    constexpr uint32_t Op = 0b0001'1001'000 << 21;
    SubAtomicImm(Op, ARMEmitter::SubRegSize::i16Bit, 0b11, rt, rn, static_cast<uint32_t>(Imm) & 0x1'FF);
  }
  void ldapursh(ARMEmitter::XRegister rt, ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");
    constexpr uint32_t Op = 0b0001'1001'000 << 21;
    SubAtomicImm(Op, ARMEmitter::SubRegSize::i16Bit, 0b10, rt, rn, static_cast<uint32_t>(Imm) & 0x1'FF);
  }
  void stlur(ARMEmitter::WRegister rt, ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");
    constexpr uint32_t Op = 0b0001'1001'000 << 21;
    SubAtomicImm(Op, ARMEmitter::SubRegSize::i32Bit, 0b00, rt, rn, static_cast<uint32_t>(Imm) & 0x1'FF);
  }
  void ldapur(ARMEmitter::WRegister rt, ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");
    constexpr uint32_t Op = 0b0001'1001'000 << 21;
    SubAtomicImm(Op, ARMEmitter::SubRegSize::i32Bit, 0b01, rt, rn, static_cast<uint32_t>(Imm) & 0x1'FF);
  }
  void ldapursw(ARMEmitter::XRegister rt, ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");
    constexpr uint32_t Op = 0b0001'1001'000 << 21;
    SubAtomicImm(Op, ARMEmitter::SubRegSize::i32Bit, 0b10, rt, rn, static_cast<uint32_t>(Imm) & 0x1'FF);
  }
  void stlur(ARMEmitter::XRegister rt, ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");
    constexpr uint32_t Op = 0b0001'1001'000 << 21;
    SubAtomicImm(Op, ARMEmitter::SubRegSize::i64Bit, 0b00, rt, rn, static_cast<uint32_t>(Imm) & 0x1'FF);
  }
  void ldapur(ARMEmitter::XRegister rt, ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");
    constexpr uint32_t Op = 0b0001'1001'000 << 21;
    SubAtomicImm(Op, ARMEmitter::SubRegSize::i64Bit, 0b01, rt, rn, static_cast<uint32_t>(Imm) & 0x1'FF);
  }
  // Load register literal
  void ldr(ARMEmitter::WRegister rt, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -1048576 && Imm <= 1048575 && ((Imm & 0b11) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = 0b0001'1000 << 24;
    LoadStoreLiteral(Op, rt, static_cast<uint32_t>(Imm >> 2) & 0x7'FFFF);
  }
  void ldr(ARMEmitter::SRegister rt, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -1048576 && Imm <= 1048575 && ((Imm & 0b11) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = 0b0001'1100 << 24;
    LoadStoreLiteral(Op, rt, static_cast<uint32_t>(Imm >> 2) & 0x7'FFFF);
  }
  void ldr(ARMEmitter::XRegister rt, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -1048576 && Imm <= 1048575 && ((Imm & 0b11) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = 0b0101'1000 << 24;
    LoadStoreLiteral(Op, rt, static_cast<uint32_t>(Imm >> 2) & 0x7'FFFF);
  }
  void ldr(ARMEmitter::DRegister rt, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -1048576 && Imm <= 1048575 && ((Imm & 0b11) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = 0b0101'1100 << 24;
    LoadStoreLiteral(Op, rt, static_cast<uint32_t>(Imm >> 2) & 0x7'FFFF);
  }
  void ldrs(ARMEmitter::WRegister rt, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -1048576 && Imm <= 1048575 && ((Imm & 0b11) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = 0b1001'1000 << 24;
    LoadStoreLiteral(Op, rt, static_cast<uint32_t>(Imm >> 2) & 0x7'FFFF);
  }
  void ldr(ARMEmitter::QRegister rt, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -1048576 && Imm <= 1048575 && ((Imm & 0b11) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = 0b1001'1100 << 24;
    LoadStoreLiteral(Op, rt, static_cast<uint32_t>(Imm >> 2) & 0x7'FFFF);
  }
  void prfm(ARMEmitter::Prefetch prfop, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -1048576 && Imm <= 1048575 && ((Imm & 0b11) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = 0b1101'1000 << 24;
    LoadStoreLiteral(Op, prfop, static_cast<uint32_t>(Imm >> 2) & 0x7'FFFF);
  }
  void ldr(ARMEmitter::WRegister rt, const BackwardLabel* Label) {
    int32_t Imm = static_cast<int32_t>(Label->Location - GetCursorAddress<uint8_t*>());
    LOGMAN_THROW_A_FMT(Imm >= -1048576 && Imm <= 1048575 && ((Imm & 0b11) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = 0b0001'1000 << 24;
    LoadStoreLiteral(Op, rt, static_cast<uint32_t>(Imm >> 2) & 0x7'FFFF);
  }
  void ldr(ARMEmitter::SRegister rt, const BackwardLabel* Label) {
    int32_t Imm = static_cast<int32_t>(Label->Location - GetCursorAddress<uint8_t*>());
    LOGMAN_THROW_A_FMT(Imm >= -1048576 && Imm <= 1048575 && ((Imm & 0b11) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = 0b0001'1100 << 24;
    LoadStoreLiteral(Op, rt, static_cast<uint32_t>(Imm >> 2) & 0x7'FFFF);
  }
  void ldr(ARMEmitter::XRegister rt, const BackwardLabel* Label) {
    int32_t Imm = static_cast<int32_t>(Label->Location - GetCursorAddress<uint8_t*>());
    LOGMAN_THROW_A_FMT(Imm >= -1048576 && Imm <= 1048575 && ((Imm & 0b11) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = 0b0101'1000 << 24;
    LoadStoreLiteral(Op, rt, static_cast<uint32_t>(Imm >> 2) & 0x7'FFFF);
  }
  void ldr(ARMEmitter::DRegister rt, const BackwardLabel* Label) {
    int32_t Imm = static_cast<int32_t>(Label->Location - GetCursorAddress<uint8_t*>());
    LOGMAN_THROW_A_FMT(Imm >= -1048576 && Imm <= 1048575 && ((Imm & 0b11) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = 0b0101'1100 << 24;
    LoadStoreLiteral(Op, rt, static_cast<uint32_t>(Imm >> 2) & 0x7'FFFF);
  }
  void ldrsw(ARMEmitter::XRegister rt, const BackwardLabel* Label) {
    int32_t Imm = static_cast<int32_t>(Label->Location - GetCursorAddress<uint8_t*>());
    LOGMAN_THROW_A_FMT(Imm >= -1048576 && Imm <= 1048575 && ((Imm & 0b11) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = 0b1001'1000 << 24;
    LoadStoreLiteral(Op, rt, static_cast<uint32_t>(Imm >> 2) & 0x7'FFFF);
  }
  void ldr(ARMEmitter::QRegister rt, const BackwardLabel* Label) {
    int32_t Imm = static_cast<int32_t>(Label->Location - GetCursorAddress<uint8_t*>());
    LOGMAN_THROW_A_FMT(Imm >= -1048576 && Imm <= 1048575 && ((Imm & 0b11) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = 0b1001'1100 << 24;
    LoadStoreLiteral(Op, rt, static_cast<uint32_t>(Imm >> 2) & 0x7'FFFF);
  }
  void prfm(ARMEmitter::Prefetch prfop, const BackwardLabel* Label) {
    int32_t Imm = static_cast<int32_t>(Label->Location - GetCursorAddress<uint8_t*>());
    LOGMAN_THROW_A_FMT(Imm >= -1048576 && Imm <= 1048575 && ((Imm & 0b11) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = 0b1101'1000 << 24;
    LoadStoreLiteral(Op, prfop, static_cast<uint32_t>(Imm >> 2) & 0x7'FFFF);
  }

  void ldr(ARMEmitter::WRegister rt, ForwardLabel* Label) {
    AddLocationToLabel(Label, ForwardLabel::Reference {.Location = GetCursorAddress<uint8_t*>(), .Type = ForwardLabel::InstType::RELATIVE_LOAD});
    constexpr uint32_t Op = 0b0001'1000 << 24;
    LoadStoreLiteral(Op, rt, 0);
  }

  void ldr(ARMEmitter::SRegister rt, ForwardLabel* Label) {
    AddLocationToLabel(Label, ForwardLabel::Reference {.Location = GetCursorAddress<uint8_t*>(), .Type = ForwardLabel::InstType::RELATIVE_LOAD});
    constexpr uint32_t Op = 0b0001'1100 << 24;
    LoadStoreLiteral(Op, rt, 0);
  }

  void ldr(ARMEmitter::XRegister rt, ForwardLabel* Label) {
    AddLocationToLabel(Label, ForwardLabel::Reference {.Location = GetCursorAddress<uint8_t*>(), .Type = ForwardLabel::InstType::RELATIVE_LOAD});
    constexpr uint32_t Op = 0b0101'1000 << 24;
    LoadStoreLiteral(Op, rt, 0);
  }

  void ldr(ARMEmitter::DRegister rt, ForwardLabel* Label) {
    AddLocationToLabel(Label, ForwardLabel::Reference {.Location = GetCursorAddress<uint8_t*>(), .Type = ForwardLabel::InstType::RELATIVE_LOAD});
    constexpr uint32_t Op = 0b0101'1100 << 24;
    LoadStoreLiteral(Op, rt, 0);
  }

  void ldrsw(ARMEmitter::XRegister rt, ForwardLabel* Label) {
    AddLocationToLabel(Label, ForwardLabel::Reference {.Location = GetCursorAddress<uint8_t*>(), .Type = ForwardLabel::InstType::RELATIVE_LOAD});
    constexpr uint32_t Op = 0b1001'1000 << 24;
    LoadStoreLiteral(Op, rt, 0);
  }

  void ldr(ARMEmitter::QRegister rt, ForwardLabel* Label) {
    AddLocationToLabel(Label, ForwardLabel::Reference {.Location = GetCursorAddress<uint8_t*>(), .Type = ForwardLabel::InstType::RELATIVE_LOAD});
    constexpr uint32_t Op = 0b1001'1100 << 24;
    LoadStoreLiteral(Op, rt, 0);
  }

  void prfm(ARMEmitter::Prefetch prfop, ForwardLabel* Label) {
    AddLocationToLabel(Label, ForwardLabel::Reference {.Location = GetCursorAddress<uint8_t*>(), .Type = ForwardLabel::InstType::RELATIVE_LOAD});
    constexpr uint32_t Op = 0b1101'1000 << 24;
    LoadStoreLiteral(Op, prfop, 0);
  }

  void ldr(ARMEmitter::WRegister rt, BiDirectionalLabel* Label) {
    if (Label->Backward.Location) {
      ldr(rt, &Label->Backward);
    } else {
      ldr(rt, &Label->Forward);
    }
  }
  void ldr(ARMEmitter::SRegister rt, BiDirectionalLabel* Label) {
    if (Label->Backward.Location) {
      ldr(rt, &Label->Backward);
    } else {
      ldr(rt, &Label->Forward);
    }
  }
  void ldr(ARMEmitter::XRegister rt, BiDirectionalLabel* Label) {
    if (Label->Backward.Location) {
      ldr(rt, &Label->Backward);
    } else {
      ldr(rt, &Label->Forward);
    }
  }
  void ldr(ARMEmitter::DRegister rt, BiDirectionalLabel* Label) {
    if (Label->Backward.Location) {
      ldr(rt, &Label->Backward);
    } else {
      ldr(rt, &Label->Forward);
    }
  }
  void ldrs(ARMEmitter::WRegister rt, BiDirectionalLabel* Label) {
    if (Label->Backward.Location) {
      ldr(rt, &Label->Backward);
    } else {
      ldr(rt, &Label->Forward);
    }
  }
  void ldr(ARMEmitter::QRegister rt, BiDirectionalLabel* Label) {
    if (Label->Backward.Location) {
      ldr(rt, &Label->Backward);
    } else {
      ldr(rt, &Label->Forward);
    }
  }
  void prfm(ARMEmitter::Prefetch prfop, BiDirectionalLabel* Label) {
    if (Label->Backward.Location) {
      prfm(prfop, &Label->Backward);
    } else {
      prfm(prfop, &Label->Forward);
    }
  }

  // Memory copy/set
  void cpyfp(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 0, 0b00, 0b0000, rs, rn, rd);
  }
  void cpyfm(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 0, 0b01, 0b0000, rs, rn, rd);
  }
  void cpyfe(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 0, 0b10, 0b0000, rs, rn, rd);
  }
  void cpyfpwt(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 0, 0b00, 0b0001, rs, rn, rd);
  }
  void cpyfmwt(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 0, 0b01, 0b0001, rs, rn, rd);
  }
  void cpyfewt(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 0, 0b10, 0b0001, rs, rn, rd);
  }
  void cpyfprt(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 0, 0b00, 0b0010, rs, rn, rd);
  }
  void cpyfmrt(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 0, 0b01, 0b0010, rs, rn, rd);
  }
  void cpyfert(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 0, 0b10, 0b0010, rs, rn, rd);
  }
  void cpyfpt(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 0, 0b00, 0b0011, rs, rn, rd);
  }
  void cpyfmt(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 0, 0b01, 0b0011, rs, rn, rd);
  }
  void cpyfet(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 0, 0b10, 0b0011, rs, rn, rd);
  }
  void cpyfpwn(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 0, 0b00, 0b0100, rs, rn, rd);
  }
  void cpyfmwn(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 0, 0b01, 0b0100, rs, rn, rd);
  }
  void cpyfewn(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 0, 0b10, 0b0100, rs, rn, rd);
  }
  void cpyfpwtwn(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 0, 0b00, 0b0101, rs, rn, rd);
  }
  void cpyfmwtwn(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 0, 0b01, 0b0101, rs, rn, rd);
  }
  void cpyfewtwn(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 0, 0b10, 0b0101, rs, rn, rd);
  }
  void cpyfprtwn(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 0, 0b00, 0b0110, rs, rn, rd);
  }
  void cpyfmrtwn(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 0, 0b01, 0b0110, rs, rn, rd);
  }
  void cpyfertwn(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 0, 0b10, 0b0110, rs, rn, rd);
  }
  void cpyfptwn(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 0, 0b00, 0b0111, rs, rn, rd);
  }
  void cpyfmtwn(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 0, 0b01, 0b0111, rs, rn, rd);
  }
  void cpyfetwn(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 0, 0b10, 0b0111, rs, rn, rd);
  }
  void cpyfprn(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 0, 0b00, 0b1000, rs, rn, rd);
  }
  void cpyfmrn(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 0, 0b01, 0b1000, rs, rn, rd);
  }
  void cpyfern(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 0, 0b10, 0b1000, rs, rn, rd);
  }
  void cpyfpwtrn(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 0, 0b00, 0b1001, rs, rn, rd);
  }
  void cpyfmwtrn(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 0, 0b01, 0b1001, rs, rn, rd);
  }
  void cpyfewtrn(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 0, 0b10, 0b1001, rs, rn, rd);
  }
  void cpyfprtrn(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 0, 0b00, 0b1010, rs, rn, rd);
  }
  void cpyfmrtrn(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 0, 0b01, 0b1010, rs, rn, rd);
  }
  void cpyfertrn(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 0, 0b10, 0b1010, rs, rn, rd);
  }
  void cpyfptrn(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 0, 0b00, 0b1011, rs, rn, rd);
  }
  void cpyfmtrn(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 0, 0b01, 0b1011, rs, rn, rd);
  }
  void cpyfetrn(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 0, 0b10, 0b1011, rs, rn, rd);
  }
  void cpyfpn(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 0, 0b00, 0b1100, rs, rn, rd);
  }
  void cpyfmn(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 0, 0b01, 0b1100, rs, rn, rd);
  }
  void cpyfen(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 0, 0b10, 0b1100, rs, rn, rd);
  }
  void cpyfpwtn(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 0, 0b00, 0b1101, rs, rn, rd);
  }
  void cpyfmwtn(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 0, 0b01, 0b1101, rs, rn, rd);
  }
  void cpyfewtn(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 0, 0b10, 0b1101, rs, rn, rd);
  }
  void cpyfprtn(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 0, 0b00, 0b1110, rs, rn, rd);
  }
  void cpyfmrtn(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 0, 0b01, 0b1110, rs, rn, rd);
  }
  void cpyfertn(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 0, 0b10, 0b1110, rs, rn, rd);
  }
  void cpyfptn(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 0, 0b00, 0b1111, rs, rn, rd);
  }
  void cpyfmtn(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 0, 0b01, 0b1111, rs, rn, rd);
  }
  void cpyfetn(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 0, 0b10, 0b1111, rs, rn, rd);
  }

  void setp(Register rd, Register rn, Register rs) {
    MemoryCopyAndMemorySet(0, 0, 0b11, 0b0000, rs, rn, rd);
  }
  void setm(Register rd, Register rn, Register rs) {
    MemoryCopyAndMemorySet(0, 0, 0b11, 0b0100, rs, rn, rd);
  }
  void sete(Register rd, Register rn, Register rs) {
    MemoryCopyAndMemorySet(0, 0, 0b11, 0b1000, rs, rn, rd);
  }
  void setpt(Register rd, Register rn, Register rs) {
    MemoryCopyAndMemorySet(0, 0, 0b11, 0b0001, rs, rn, rd);
  }
  void setmt(Register rd, Register rn, Register rs) {
    MemoryCopyAndMemorySet(0, 0, 0b11, 0b0101, rs, rn, rd);
  }
  void setet(Register rd, Register rn, Register rs) {
    MemoryCopyAndMemorySet(0, 0, 0b11, 0b1001, rs, rn, rd);
  }
  void setpn(Register rd, Register rn, Register rs) {
    MemoryCopyAndMemorySet(0, 0, 0b11, 0b0010, rs, rn, rd);
  }
  void setmn(Register rd, Register rn, Register rs) {
    MemoryCopyAndMemorySet(0, 0, 0b11, 0b0110, rs, rn, rd);
  }
  void seten(Register rd, Register rn, Register rs) {
    MemoryCopyAndMemorySet(0, 0, 0b11, 0b1010, rs, rn, rd);
  }
  void setptn(Register rd, Register rn, Register rs) {
    MemoryCopyAndMemorySet(0, 0, 0b11, 0b0011, rs, rn, rd);
  }
  void setmtn(Register rd, Register rn, Register rs) {
    MemoryCopyAndMemorySet(0, 0, 0b11, 0b0111, rs, rn, rd);
  }
  void setetn(Register rd, Register rn, Register rs) {
    MemoryCopyAndMemorySet(0, 0, 0b11, 0b1011, rs, rn, rd);
  }

  void cpyp(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 1, 0b00, 0b0000, rs, rn, rd);
  }
  void cpym(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 1, 0b01, 0b0000, rs, rn, rd);
  }
  void cpye(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 1, 0b10, 0b0000, rs, rn, rd);
  }
  void cpypwt(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 1, 0b00, 0b0001, rs, rn, rd);
  }
  void cpymwt(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 1, 0b01, 0b0001, rs, rn, rd);
  }
  void cpyewt(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 1, 0b10, 0b0001, rs, rn, rd);
  }
  void cpyprt(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 1, 0b00, 0b0010, rs, rn, rd);
  }
  void cpymrt(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 1, 0b01, 0b0010, rs, rn, rd);
  }
  void cpyert(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 1, 0b10, 0b0010, rs, rn, rd);
  }
  void cpypt(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 1, 0b00, 0b0011, rs, rn, rd);
  }
  void cpymt(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 1, 0b01, 0b0011, rs, rn, rd);
  }
  void cpyet(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 1, 0b10, 0b0011, rs, rn, rd);
  }
  void cpypwn(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 1, 0b00, 0b0100, rs, rn, rd);
  }
  void cpymwn(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 1, 0b01, 0b0100, rs, rn, rd);
  }
  void cpyewn(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 1, 0b10, 0b0100, rs, rn, rd);
  }
  void cpypwtwn(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 1, 0b00, 0b0101, rs, rn, rd);
  }
  void cpymwtwn(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 1, 0b01, 0b0101, rs, rn, rd);
  }
  void cpyewtwn(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 1, 0b10, 0b0101, rs, rn, rd);
  }
  void cpyprtwn(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 1, 0b00, 0b0110, rs, rn, rd);
  }
  void cpymrtwn(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 1, 0b01, 0b0110, rs, rn, rd);
  }
  void cpyertwn(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 1, 0b10, 0b0110, rs, rn, rd);
  }
  void cpyptwn(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 1, 0b00, 0b0111, rs, rn, rd);
  }
  void cpymtwn(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 1, 0b01, 0b0111, rs, rn, rd);
  }
  void cpyetwn(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 1, 0b10, 0b0111, rs, rn, rd);
  }
  void cpyprn(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 1, 0b00, 0b1000, rs, rn, rd);
  }
  void cpymrn(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 1, 0b01, 0b1000, rs, rn, rd);
  }
  void cpyern(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 1, 0b10, 0b1000, rs, rn, rd);
  }
  void cpypwtrn(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 1, 0b00, 0b1001, rs, rn, rd);
  }
  void cpymwtrn(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 1, 0b01, 0b1001, rs, rn, rd);
  }
  void cpyewtrn(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 1, 0b10, 0b1001, rs, rn, rd);
  }
  void cpyprtrn(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 1, 0b00, 0b1010, rs, rn, rd);
  }
  void cpymrtrn(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 1, 0b01, 0b1010, rs, rn, rd);
  }
  void cpyertrn(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 1, 0b10, 0b1010, rs, rn, rd);
  }
  void cpyptrn(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 1, 0b00, 0b1011, rs, rn, rd);
  }
  void cpymtrn(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 1, 0b01, 0b1011, rs, rn, rd);
  }
  void cpyetrn(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 1, 0b10, 0b1011, rs, rn, rd);
  }
  void cpypn(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 1, 0b00, 0b1100, rs, rn, rd);
  }
  void cpymn(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 1, 0b01, 0b1100, rs, rn, rd);
  }
  void cpyen(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 1, 0b10, 0b1100, rs, rn, rd);
  }
  void cpypwtn(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 1, 0b00, 0b1101, rs, rn, rd);
  }
  void cpymwtn(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 1, 0b01, 0b1101, rs, rn, rd);
  }
  void cpyewtn(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 1, 0b10, 0b1101, rs, rn, rd);
  }
  void cpyprtn(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 1, 0b00, 0b1110, rs, rn, rd);
  }
  void cpymrtn(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 1, 0b01, 0b1110, rs, rn, rd);
  }
  void cpyertn(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 1, 0b10, 0b1110, rs, rn, rd);
  }
  void cpyptn(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 1, 0b00, 0b1111, rs, rn, rd);
  }
  void cpymtn(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 1, 0b01, 0b1111, rs, rn, rd);
  }
  void cpyetn(Register rd, Register rs, Register rn) {
    MemoryCopyAndMemorySet(0, 1, 0b10, 0b1111, rs, rn, rd);
  }

  void setgp(Register rd, Register rn, Register rs) {
    MemoryCopyAndMemorySet(0, 1, 0b11, 0b0000, rs, rn, rd);
  }
  void setgm(Register rd, Register rn, Register rs) {
    MemoryCopyAndMemorySet(0, 1, 0b11, 0b0100, rs, rn, rd);
  }
  void setge(Register rd, Register rn, Register rs) {
    MemoryCopyAndMemorySet(0, 1, 0b11, 0b1000, rs, rn, rd);
  }
  void setgpt(Register rd, Register rn, Register rs) {
    MemoryCopyAndMemorySet(0, 1, 0b11, 0b0001, rs, rn, rd);
  }
  void setgmt(Register rd, Register rn, Register rs) {
    MemoryCopyAndMemorySet(0, 1, 0b11, 0b0101, rs, rn, rd);
  }
  void setget(Register rd, Register rn, Register rs) {
    MemoryCopyAndMemorySet(0, 1, 0b11, 0b1001, rs, rn, rd);
  }
  void setgpn(Register rd, Register rn, Register rs) {
    MemoryCopyAndMemorySet(0, 1, 0b11, 0b0010, rs, rn, rd);
  }
  void setgmn(Register rd, Register rn, Register rs) {
    MemoryCopyAndMemorySet(0, 1, 0b11, 0b0110, rs, rn, rd);
  }
  void setgen(Register rd, Register rn, Register rs) {
    MemoryCopyAndMemorySet(0, 1, 0b11, 0b1010, rs, rn, rd);
  }
  void setgptn(Register rd, Register rn, Register rs) {
    MemoryCopyAndMemorySet(0, 1, 0b11, 0b0011, rs, rn, rd);
  }
  void setgmtn(Register rd, Register rn, Register rs) {
    MemoryCopyAndMemorySet(0, 1, 0b11, 0b0111, rs, rn, rd);
  }
  void setgetn(Register rd, Register rn, Register rs) {
    MemoryCopyAndMemorySet(0, 1, 0b11, 0b1011, rs, rn, rd);
  }

  // Loadstore no-allocate pair
  void stnp(ARMEmitter::WRegister rt, ARMEmitter::WRegister rt2, ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 252 && ((Imm & 0b11) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = 0b0010'1000'00 << 22;
    LoadStoreNoAllocate(Op, rt, rt2, rn, static_cast<uint32_t>(Imm >> 2) & 0b111'1111);
  }
  void ldnp(ARMEmitter::WRegister rt, ARMEmitter::WRegister rt2, ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 252 && ((Imm & 0b11) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = 0b0010'1000'01 << 22;
    LoadStoreNoAllocate(Op, rt, rt2, rn, static_cast<uint32_t>(Imm >> 2) & 0b111'1111);
  }
  void stnp(ARMEmitter::SRegister rt, ARMEmitter::SRegister rt2, ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 252 && ((Imm & 0b11) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = 0b0010'1100'00 << 22;
    LoadStoreNoAllocate(Op, rt, rt2, rn, static_cast<uint32_t>(Imm >> 2) & 0b111'1111);
  }
  void ldnp(ARMEmitter::SRegister rt, ARMEmitter::SRegister rt2, ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 252 && ((Imm & 0b11) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = 0b0010'1100'01 << 22;
    LoadStoreNoAllocate(Op, rt, rt2, rn, static_cast<uint32_t>(Imm >> 2) & 0b111'1111);
  }
  void stnp(ARMEmitter::XRegister rt, ARMEmitter::XRegister rt2, ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -512 && Imm <= 504 && ((Imm & 0b111) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = 0b1010'1000'00 << 22;
    LoadStoreNoAllocate(Op, rt, rt2, rn, static_cast<uint32_t>(Imm >> 3) & 0b111'1111);
  }
  void ldnp(ARMEmitter::XRegister rt, ARMEmitter::XRegister rt2, ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -512 && Imm <= 504 && ((Imm & 0b111) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = 0b1010'1000'01 << 22;
    LoadStoreNoAllocate(Op, rt, rt2, rn, static_cast<uint32_t>(Imm >> 3) & 0b111'1111);
  }
  void stnp(ARMEmitter::DRegister rt, ARMEmitter::DRegister rt2, ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -512 && Imm <= 504 && ((Imm & 0b111) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = 0b0110'1100'00 << 22;
    LoadStoreNoAllocate(Op, rt, rt2, rn, static_cast<uint32_t>(Imm >> 3) & 0b111'1111);
  }
  void ldnp(ARMEmitter::DRegister rt, ARMEmitter::DRegister rt2, ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -512 && Imm <= 504 && ((Imm & 0b111) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = 0b0110'1100'01 << 22;
    LoadStoreNoAllocate(Op, rt, rt2, rn, static_cast<uint32_t>(Imm >> 3) & 0b111'1111);
  }
  void stnp(ARMEmitter::QRegister rt, ARMEmitter::QRegister rt2, ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -1024 && Imm <= 1008 && ((Imm & 0b1111) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = 0b1010'1100'00 << 22;
    LoadStoreNoAllocate(Op, rt, rt2, rn, static_cast<uint32_t>(Imm >> 4) & 0b111'1111);
  }
  void ldnp(ARMEmitter::QRegister rt, ARMEmitter::QRegister rt2, ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -1024 && Imm <= 1008 && ((Imm & 0b1111) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = 0b1010'1100'01 << 22;
    LoadStoreNoAllocate(Op, rt, rt2, rn, static_cast<uint32_t>(Imm >> 4) & 0b111'1111);
  }
  // Loadstore register pair post-indexed
  // Loadstore register pair offset
  // Loadstore register pair pre-indexed
  template<IndexType Index>
  void stp(ARMEmitter::WRegister rt, ARMEmitter::WRegister rt2, ARMEmitter::Register rn, int32_t Imm = 0) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 252 && ((Imm & 0b11) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = (0b0010'1000'00 << 22) | (Index == IndexType::POST   ? (0b01 << 23) :
                                                      Index == IndexType::PRE    ? (0b11 << 23) :
                                                      Index == IndexType::OFFSET ? (0b10 << 23) :
                                                                                   -1);

    LoadStorePair(Op, rt, rt2, rn, (Imm >> 2) & 0b111'1111);
  }
  template<IndexType Index>
  void ldp(ARMEmitter::WRegister rt, ARMEmitter::WRegister rt2, ARMEmitter::Register rn, int32_t Imm = 0) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 252 && ((Imm & 0b11) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = (0b0010'1000'01 << 22) | (Index == IndexType::POST   ? (0b01 << 23) :
                                                      Index == IndexType::PRE    ? (0b11 << 23) :
                                                      Index == IndexType::OFFSET ? (0b10 << 23) :
                                                                                   -1);

    LoadStorePair(Op, rt, rt2, rn, (Imm >> 2) & 0b111'1111);
  }
  template<IndexType Index>
  void ldpsw(ARMEmitter::XRegister rt, ARMEmitter::XRegister rt2, ARMEmitter::Register rn, int32_t Imm = 0) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 252 && ((Imm & 0b11) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = (0b0110'1000'01 << 22) | (Index == IndexType::POST   ? (0b01 << 23) :
                                                      Index == IndexType::PRE    ? (0b11 << 23) :
                                                      Index == IndexType::OFFSET ? (0b10 << 23) :
                                                                                   -1);
    LoadStorePair(Op, rt, rt2, rn, (Imm >> 2) & 0b111'1111);
  }
  template<IndexType Index>
  void stp(ARMEmitter::XRegister rt, ARMEmitter::XRegister rt2, ARMEmitter::Register rn, int32_t Imm = 0) {
    LOGMAN_THROW_A_FMT(Imm >= -512 && Imm <= 504 && ((Imm & 0b111) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = (0b1010'1000'00 << 22) | (Index == IndexType::POST   ? (0b01 << 23) :
                                                      Index == IndexType::PRE    ? (0b11 << 23) :
                                                      Index == IndexType::OFFSET ? (0b10 << 23) :
                                                                                   -1);

    LoadStorePair(Op, rt, rt2, rn, (Imm >> 3) & 0b111'1111);
  }
  template<IndexType Index>
  void ldp(ARMEmitter::XRegister rt, ARMEmitter::XRegister rt2, ARMEmitter::Register rn, int32_t Imm = 0) {
    LOGMAN_THROW_A_FMT(Imm >= -512 && Imm <= 504 && ((Imm & 0b111) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = (0b1010'1000'01 << 22) | (Index == IndexType::POST   ? (0b01 << 23) :
                                                      Index == IndexType::PRE    ? (0b11 << 23) :
                                                      Index == IndexType::OFFSET ? (0b10 << 23) :
                                                                                   -1);

    LoadStorePair(Op, rt, rt2, rn, (Imm >> 3) & 0b111'1111);
  }
  template<IndexType Index>
  void stp(ARMEmitter::SRegister rt, ARMEmitter::SRegister rt2, ARMEmitter::Register rn, int32_t Imm = 0) {
    stp_w<Index>(rt.V(), rt2.V(), rn, Imm);
  }
  template<IndexType Index>
  void ldp(ARMEmitter::SRegister rt, ARMEmitter::SRegister rt2, ARMEmitter::Register rn, int32_t Imm = 0) {
    ldp_w<Index>(rt.V(), rt2.V(), rn, Imm);
  }
  template<IndexType Index>
  void stp(ARMEmitter::DRegister rt, ARMEmitter::DRegister rt2, ARMEmitter::Register rn, int32_t Imm = 0) {
    stp_x<Index>(rt.V(), rt2.V(), rn, Imm);
  }
  template<IndexType Index>
  void ldp(ARMEmitter::DRegister rt, ARMEmitter::DRegister rt2, ARMEmitter::Register rn, int32_t Imm = 0) {
    ldp_x<Index>(rt.V(), rt2.V(), rn, Imm);
  }
  template<IndexType Index>
  void stp(ARMEmitter::QRegister rt, ARMEmitter::QRegister rt2, ARMEmitter::Register rn, int32_t Imm = 0) {
    stp_q<Index>(rt.V(), rt2.V(), rn, Imm);
  }
  template<IndexType Index>
  void ldp(ARMEmitter::QRegister rt, ARMEmitter::QRegister rt2, ARMEmitter::Register rn, int32_t Imm = 0) {
    ldp_q<Index>(rt.V(), rt2.V(), rn, Imm);
  }

  // Loadstore register unscaled immediate
  void sturb(ARMEmitter::Register rt, ARMEmitter::Register rn, int32_t Imm = 0) {
    stXrb<IndexType::OFFSET>(rt, rn, Imm);
  }
  void ldurb(ARMEmitter::Register rt, ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXrb<IndexType::OFFSET>(rt, rn, Imm);
  }
  void sturb(ARMEmitter::VRegister rt, ARMEmitter::Register rn, int32_t Imm = 0) {
    stXrb<IndexType::OFFSET>(rt, rn, Imm);
  }
  void ldurb(ARMEmitter::VRegister rt, ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXrb<IndexType::OFFSET>(rt, rn, Imm);
  }
  void ldursb(ARMEmitter::XRegister rt, ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXrsb<IndexType::OFFSET>(rt, rn, Imm);
  }
  void ldursb(ARMEmitter::WRegister rt, ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXrsb<IndexType::OFFSET>(rt, rn, Imm);
  }
  void sturh(ARMEmitter::Register rt, ARMEmitter::Register rn, int32_t Imm = 0) {
    stXrh<IndexType::OFFSET>(rt, rn, Imm);
  }
  void ldurh(ARMEmitter::Register rt, ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXrh<IndexType::OFFSET>(rt, rn, Imm);
  }
  void sturh(ARMEmitter::VRegister rt, ARMEmitter::Register rn, int32_t Imm = 0) {
    stXrh<IndexType::OFFSET>(rt, rn, Imm);
  }
  void ldurh(ARMEmitter::VRegister rt, ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXrh<IndexType::OFFSET>(rt, rn, Imm);
  }
  void ldursh(ARMEmitter::XRegister rt, ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXrsh<IndexType::OFFSET>(rt, rn, Imm);
  }
  void ldursh(ARMEmitter::WRegister rt, ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXrsh<IndexType::OFFSET>(rt, rn, Imm);
  }
  void stur(ARMEmitter::WRegister rt, ARMEmitter::Register rn, int32_t Imm = 0) {
    stXr<IndexType::OFFSET>(rt, rn, Imm);
  }
  void ldur(ARMEmitter::WRegister rt, ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXr<IndexType::OFFSET>(rt, rn, Imm);
  }
  void stur(ARMEmitter::SRegister rt, ARMEmitter::Register rn, int32_t Imm = 0) {
    stXr<IndexType::OFFSET>(rt, rn, Imm);
  }
  void ldur(ARMEmitter::SRegister rt, ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXr<IndexType::OFFSET>(rt, rn, Imm);
  }
  void ldursw(ARMEmitter::XRegister rt, ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXrsw<IndexType::OFFSET>(rt, rn, Imm);
  }
  void stur(ARMEmitter::XRegister rt, ARMEmitter::Register rn, int32_t Imm = 0) {
    stXr<IndexType::OFFSET>(rt, rn, Imm);
  }
  void ldur(ARMEmitter::XRegister rt, ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXr<IndexType::OFFSET>(rt, rn, Imm);
  }
  void stur(ARMEmitter::DRegister rt, ARMEmitter::Register rn, int32_t Imm = 0) {
    stXr<IndexType::OFFSET>(rt, rn, Imm);
  }
  void ldur(ARMEmitter::DRegister rt, ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXr<IndexType::OFFSET>(rt, rn, Imm);
  }
  void stur(ARMEmitter::QRegister rt, ARMEmitter::Register rn, int32_t Imm = 0) {
    stXr<IndexType::OFFSET>(rt, rn, Imm);
  }
  void ldur(ARMEmitter::QRegister rt, ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXr<IndexType::OFFSET>(rt, rn, Imm);
  }
  template<IndexType Index>
  void prfum(ARMEmitter::Prefetch prfop, ARMEmitter::Register rn, int32_t Imm = 0) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");
    static_assert(Index == IndexType::OFFSET, "Doesn't support another index type");

    constexpr uint32_t Op = 0b1111'1000'10 << 22;
    constexpr uint32_t o2 = 0b00;

    LoadStoreImm(Op, o2, prfop, rn, Imm);
  }

  // Loadstore register immediate post-indexed
  // Loadstore register immediate pre-indexed
  template<IndexType Index>
  requires (Index == IndexType::POST || Index == IndexType::PRE)
  void strb(ARMEmitter::Register rt, ARMEmitter::Register rn, int32_t Imm = 0) {
    stXrb<Index>(rt, rn, Imm);
  }
  template<IndexType Index>
  requires (Index == IndexType::POST || Index == IndexType::PRE)
  void ldrb(ARMEmitter::Register rt, ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXrb<Index>(rt, rn, Imm);
  }
  template<IndexType Index>
  requires (Index == IndexType::POST || Index == IndexType::PRE)
  void strb(ARMEmitter::VRegister rt, ARMEmitter::Register rn, int32_t Imm = 0) {
    stXrb<Index>(rt, rn, Imm);
  }
  template<IndexType Index>
  requires (Index == IndexType::POST || Index == IndexType::PRE)
  void ldrb(ARMEmitter::VRegister rt, ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXrb<Index>(rt, rn, Imm);
  }
  template<IndexType Index>
  requires (Index == IndexType::POST || Index == IndexType::PRE)
  void ldrsb(ARMEmitter::XRegister rt, ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXrsb<Index>(rt, rn, Imm);
  }
  template<IndexType Index>
  requires (Index == IndexType::POST || Index == IndexType::PRE)
  void ldrsb(ARMEmitter::WRegister rt, ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXrsb<Index>(rt, rn, Imm);
  }
  template<IndexType Index>
  requires (Index == IndexType::POST || Index == IndexType::PRE)
  void strh(ARMEmitter::Register rt, ARMEmitter::Register rn, int32_t Imm = 0) {
    stXrh<Index>(rt, rn, Imm);
  }
  template<IndexType Index>
  requires (Index == IndexType::POST || Index == IndexType::PRE)
  void ldrh(ARMEmitter::Register rt, ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXrh<Index>(rt, rn, Imm);
  }
  template<IndexType Index>
  requires (Index == IndexType::POST || Index == IndexType::PRE)
  void strh(ARMEmitter::VRegister rt, ARMEmitter::Register rn, int32_t Imm = 0) {
    stXrh<Index>(rt, rn, Imm);
  }
  template<IndexType Index>
  requires (Index == IndexType::POST || Index == IndexType::PRE)
  void ldrh(ARMEmitter::VRegister rt, ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXrh<Index>(rt, rn, Imm);
  }
  template<IndexType Index>
  requires (Index == IndexType::POST || Index == IndexType::PRE)
  void ldrsh(ARMEmitter::XRegister rt, ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXrsh<Index>(rt, rn, Imm);
  }
  template<IndexType Index>
  requires (Index == IndexType::POST || Index == IndexType::PRE)
  void ldrsh(ARMEmitter::WRegister rt, ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXrsh<Index>(rt, rn, Imm);
  }
  template<IndexType Index>
  requires (Index == IndexType::POST || Index == IndexType::PRE)
  void str(ARMEmitter::WRegister rt, ARMEmitter::Register rn, int32_t Imm = 0) {
    stXr<Index>(rt, rn, Imm);
  }
  template<IndexType Index>
  requires (Index == IndexType::POST || Index == IndexType::PRE)
  void ldr(ARMEmitter::WRegister rt, ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXr<Index>(rt, rn, Imm);
  }
  template<IndexType Index>
  requires (Index == IndexType::POST || Index == IndexType::PRE)
  void str(ARMEmitter::SRegister rt, ARMEmitter::Register rn, int32_t Imm = 0) {
    stXr<Index>(rt, rn, Imm);
  }
  template<IndexType Index>
  requires (Index == IndexType::POST || Index == IndexType::PRE)
  void ldr(ARMEmitter::SRegister rt, ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXr<Index>(rt, rn, Imm);
  }
  template<IndexType Index>
  requires (Index == IndexType::POST || Index == IndexType::PRE)
  void ldrsw(ARMEmitter::XRegister rt, ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXrsw<Index>(rt, rn, Imm);
  }
  template<IndexType Index>
  requires (Index == IndexType::POST || Index == IndexType::PRE)
  void str(ARMEmitter::XRegister rt, ARMEmitter::Register rn, int32_t Imm = 0) {
    stXr<Index>(rt, rn, Imm);
  }
  template<IndexType Index>
  requires (Index == IndexType::POST || Index == IndexType::PRE)
  void ldr(ARMEmitter::XRegister rt, ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXr<Index>(rt, rn, Imm);
  }
  template<IndexType Index>
  requires (Index == IndexType::POST || Index == IndexType::PRE)
  void str(ARMEmitter::DRegister rt, ARMEmitter::Register rn, int32_t Imm = 0) {
    stXr<Index>(rt, rn, Imm);
  }
  template<IndexType Index>
  requires (Index == IndexType::POST || Index == IndexType::PRE)
  void ldr(ARMEmitter::DRegister rt, ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXr<Index>(rt, rn, Imm);
  }
  template<IndexType Index>
  requires (Index == IndexType::POST || Index == IndexType::PRE)
  void str(ARMEmitter::QRegister rt, ARMEmitter::Register rn, int32_t Imm = 0) {
    stXr<Index>(rt, rn, Imm);
  }
  template<IndexType Index>
  requires (Index == IndexType::POST || Index == IndexType::PRE)
  void ldr(ARMEmitter::QRegister rt, ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXr<Index>(rt, rn, Imm);
  }

  // Loadstore register unprivileged
  void sttrb(ARMEmitter::Register rt, ARMEmitter::Register rn, int32_t Imm = 0) {
    stXrb<IndexType::UNPRIVILEGED>(rt, rn, Imm);
  }
  void ldtrb(ARMEmitter::Register rt, ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXrb<IndexType::UNPRIVILEGED>(rt, rn, Imm);
  }
  void ldtrsb(ARMEmitter::XRegister rt, ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXrsb<IndexType::UNPRIVILEGED>(rt, rn, Imm);
  }
  void ldtrsb(ARMEmitter::WRegister rt, ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXrsb<IndexType::UNPRIVILEGED>(rt, rn, Imm);
  }
  void sttrh(ARMEmitter::Register rt, ARMEmitter::Register rn, int32_t Imm = 0) {
    stXrh<IndexType::UNPRIVILEGED>(rt, rn, Imm);
  }
  void ldtrh(ARMEmitter::Register rt, ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXrh<IndexType::UNPRIVILEGED>(rt, rn, Imm);
  }
  void ldtrsh(ARMEmitter::XRegister rt, ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXrsh<IndexType::UNPRIVILEGED>(rt, rn, Imm);
  }
  void ldtrsh(ARMEmitter::WRegister rt, ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXrsh<IndexType::UNPRIVILEGED>(rt, rn, Imm);
  }
  void sttr(ARMEmitter::WRegister rt, ARMEmitter::Register rn, int32_t Imm = 0) {
    stXr<IndexType::UNPRIVILEGED>(rt, rn, Imm);
  }
  void ldtr(ARMEmitter::WRegister rt, ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXr<IndexType::UNPRIVILEGED>(rt, rn, Imm);
  }
  void ldtrsw(ARMEmitter::XRegister rt, ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXrsw<IndexType::UNPRIVILEGED>(rt, rn, Imm);
  }
  void sttr(ARMEmitter::XRegister rt, ARMEmitter::Register rn, int32_t Imm = 0) {
    stXr<IndexType::UNPRIVILEGED>(rt, rn, Imm);
  }
  void ldtr(ARMEmitter::XRegister rt, ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXr<IndexType::UNPRIVILEGED>(rt, rn, Imm);
  }
  // Atomic memory operations
  void stadd(SubRegSize size, Register rs, Register rn) {
    LoadStoreAtomicLSE(size, 0, 0, 0, 0b000, rs, Reg::zr, rn);
  }
  void staddl(SubRegSize size, Register rs, Register rn) {
    LoadStoreAtomicLSE(size, 0, 1, 0, 0b000, rs, Reg::zr, rn);
  }
  void stadda(SubRegSize size, Register rs, Register rn) {
    LoadStoreAtomicLSE(size, 1, 0, 0, 0b000, rs, Reg::zr, rn);
  }
  void staddal(SubRegSize size, Register rs, Register rn) {
    LoadStoreAtomicLSE(size, 1, 1, 0, 0b000, rs, Reg::zr, rn);
  }
  void stclr(SubRegSize size, Register rs, Register rn) {
    LoadStoreAtomicLSE(size, 0, 0, 0, 0b001, rs, Reg::zr, rn);
  }
  void stclrl(SubRegSize size, Register rs, Register rn) {
    LoadStoreAtomicLSE(size, 0, 1, 0, 0b001, rs, Reg::zr, rn);
  }
  void stclra(SubRegSize size, Register rs, Register rn) {
    LoadStoreAtomicLSE(size, 1, 0, 0, 0b001, rs, Reg::zr, rn);
  }
  void stclral(SubRegSize size, Register rs, Register rn) {
    LoadStoreAtomicLSE(size, 1, 1, 0, 0b001, rs, Reg::zr, rn);
  }
  void stset(SubRegSize size, Register rs, Register rn) {
    LoadStoreAtomicLSE(size, 0, 0, 0, 0b011, rs, Reg::zr, rn);
  }
  void stsetl(SubRegSize size, Register rs, Register rn) {
    LoadStoreAtomicLSE(size, 0, 1, 0, 0b011, rs, Reg::zr, rn);
  }
  void stseta(SubRegSize size, Register rs, Register rn) {
    LoadStoreAtomicLSE(size, 1, 0, 0, 0b011, rs, Reg::zr, rn);
  }
  void stsetal(SubRegSize size, Register rs, Register rn) {
    LoadStoreAtomicLSE(size, 1, 1, 0, 0b011, rs, Reg::zr, rn);
  }
  void steor(SubRegSize size, Register rs, Register rn) {
    LoadStoreAtomicLSE(size, 0, 0, 0, 0b010, rs, Reg::zr, rn);
  }
  void steorl(SubRegSize size, Register rs, Register rn) {
    LoadStoreAtomicLSE(size, 0, 1, 0, 0b010, rs, Reg::zr, rn);
  }
  void steora(SubRegSize size, Register rs, Register rn) {
    LoadStoreAtomicLSE(size, 1, 0, 0, 0b010, rs, Reg::zr, rn);
  }
  void steoral(SubRegSize size, Register rs, Register rn) {
    LoadStoreAtomicLSE(size, 1, 1, 0, 0b010, rs, Reg::zr, rn);
  }
  void stsmax(SubRegSize size, Register rs, Register rn) {
    LoadStoreAtomicLSE(size, 0, 0, 0, 0b100, rs, Reg::zr, rn);
  }
  void stsmaxl(SubRegSize size, Register rs, Register rn) {
    LoadStoreAtomicLSE(size, 0, 1, 0, 0b100, rs, Reg::zr, rn);
  }
  void stsmaxa(SubRegSize size, Register rs, Register rn) {
    LoadStoreAtomicLSE(size, 1, 0, 0, 0b100, rs, Reg::zr, rn);
  }
  void stsmaxal(SubRegSize size, Register rs, Register rn) {
    LoadStoreAtomicLSE(size, 1, 1, 0, 0b100, rs, Reg::zr, rn);
  }
  void stsmin(SubRegSize size, Register rs, Register rn) {
    LoadStoreAtomicLSE(size, 0, 0, 0, 0b101, rs, Reg::zr, rn);
  }
  void stsminl(SubRegSize size, Register rs, Register rn) {
    LoadStoreAtomicLSE(size, 0, 1, 0, 0b101, rs, Reg::zr, rn);
  }
  void stsmina(SubRegSize size, Register rs, Register rn) {
    LoadStoreAtomicLSE(size, 1, 0, 0, 0b101, rs, Reg::zr, rn);
  }
  void stsminal(SubRegSize size, Register rs, Register rn) {
    LoadStoreAtomicLSE(size, 1, 1, 0, 0b101, rs, Reg::zr, rn);
  }
  void stumax(SubRegSize size, Register rs, Register rn) {
    LoadStoreAtomicLSE(size, 0, 0, 0, 0b110, rs, Reg::zr, rn);
  }
  void stumaxl(SubRegSize size, Register rs, Register rn) {
    LoadStoreAtomicLSE(size, 0, 1, 0, 0b110, rs, Reg::zr, rn);
  }
  void stumaxa(SubRegSize size, Register rs, Register rn) {
    LoadStoreAtomicLSE(size, 1, 0, 0, 0b110, rs, Reg::zr, rn);
  }
  void stumaxal(SubRegSize size, Register rs, Register rn) {
    LoadStoreAtomicLSE(size, 1, 1, 0, 0b110, rs, Reg::zr, rn);
  }
  void stumin(SubRegSize size, Register rs, Register rn) {
    LoadStoreAtomicLSE(size, 0, 0, 0, 0b111, rs, Reg::zr, rn);
  }
  void stuminl(SubRegSize size, Register rs, Register rn) {
    LoadStoreAtomicLSE(size, 0, 1, 0, 0b111, rs, Reg::zr, rn);
  }
  void stumina(SubRegSize size, Register rs, Register rn) {
    LoadStoreAtomicLSE(size, 1, 0, 0, 0b111, rs, Reg::zr, rn);
  }
  void stuminal(SubRegSize size, Register rs, Register rn) {
    LoadStoreAtomicLSE(size, 1, 1, 0, 0b111, rs, Reg::zr, rn);
  }
  void ldswp(SubRegSize size, Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(size, 0, 0, 1, 0b000, rs, rt, rn);
  }
  void ldswpl(SubRegSize size, Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(size, 0, 1, 1, 0b000, rs, rt, rn);
  }
  void ldswpa(SubRegSize size, Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(size, 1, 0, 1, 0b000, rs, rt, rn);
  }
  void ldswpal(SubRegSize size, Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(size, 1, 1, 1, 0b000, rs, rt, rn);
  }

  void ldadd(SubRegSize size, Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(size, 0, 0, 0, 0b000, rs, rt, rn);
  }
  void ldadda(SubRegSize size, Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(size, 1, 0, 0, 0b000, rs, rt, rn);
  }
  void ldaddl(SubRegSize size, Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(size, 0, 1, 0, 0b000, rs, rt, rn);
  }
  void ldaddal(SubRegSize size, Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(size, 1, 1, 0, 0b000, rs, rt, rn);
  }
  void ldclr(SubRegSize size, Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(size, 0, 0, 0, 0b001, rs, rt, rn);
  }
  void ldclra(SubRegSize size, Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(size, 1, 0, 0, 0b001, rs, rt, rn);
  }
  void ldclrl(SubRegSize size, Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(size, 0, 1, 0, 0b001, rs, rt, rn);
  }
  void ldclral(SubRegSize size, Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(size, 1, 1, 0, 0b001, rs, rt, rn);
  }

  void ldset(SubRegSize size, Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(size, 0, 0, 0, 0b011, rs, rt, rn);
  }
  void ldseta(SubRegSize size, Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(size, 1, 0, 0, 0b011, rs, rt, rn);
  }
  void ldsetl(SubRegSize size, Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(size, 0, 1, 0, 0b011, rs, rt, rn);
  }
  void ldsetal(SubRegSize size, Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(size, 1, 1, 0, 0b011, rs, rt, rn);
  }
  void ldeor(SubRegSize size, Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(size, 0, 0, 0, 0b010, rs, rt, rn);
  }
  void ldeora(SubRegSize size, Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(size, 1, 0, 0, 0b010, rs, rt, rn);
  }
  void ldeorl(SubRegSize size, Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(size, 0, 1, 0, 0b010, rs, rt, rn);
  }
  void ldeoral(SubRegSize size, Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(size, 1, 1, 0, 0b010, rs, rt, rn);
  }


  // 8-bit
  void ldaddb(Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i8Bit, 0, 0, 0, 0b000, rs, rt, rn);
  }
  void ldclrb(Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i8Bit, 0, 0, 0, 0b001, rs, rt, rn);
  }
  void ldeorb(Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i8Bit, 0, 0, 0, 0b010, rs, rt, rn);
  }
  void ldsetb(Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i8Bit, 0, 0, 0, 0b011, rs, rt, rn);
  }
  void ldsmaxb(Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i8Bit, 0, 0, 0, 0b100, rs, rt, rn);
  }
  void ldsminb(Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i8Bit, 0, 0, 0, 0b101, rs, rt, rn);
  }
  void ldumaxb(Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i8Bit, 0, 0, 0, 0b110, rs, rt, rn);
  }
  void lduminb(Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i8Bit, 0, 0, 0, 0b111, rs, rt, rn);
  }
  void ldswpb(Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i8Bit, 0, 0, 1, 0b000, rs, rt, rn);
  }
  void ldaddlb(Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i8Bit, 0, 1, 0, 0b000, rs, rt, rn);
  }
  void ldclrlb(Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i8Bit, 0, 1, 0, 0b001, rs, rt, rn);
  }
  void ldeorlb(Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i8Bit, 0, 1, 0, 0b010, rs, rt, rn);
  }
  void ldsetlb(Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i8Bit, 0, 1, 0, 0b011, rs, rt, rn);
  }
  void ldsmaxlb(Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i8Bit, 0, 1, 0, 0b100, rs, rt, rn);
  }
  void ldsminlb(Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i8Bit, 0, 1, 0, 0b101, rs, rt, rn);
  }
  void ldumaxlb(Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i8Bit, 0, 1, 0, 0b110, rs, rt, rn);
  }
  void lduminlb(Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i8Bit, 0, 1, 0, 0b111, rs, rt, rn);
  }
  void ldswplb(Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i8Bit, 0, 1, 1, 0b000, rs, rt, rn);
  }
  void ldaddab(Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i8Bit, 1, 0, 0, 0b000, rs, rt, rn);
  }
  void ldclrab(Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i8Bit, 1, 0, 0, 0b001, rs, rt, rn);
  }
  void ldeorab(Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i8Bit, 1, 0, 0, 0b010, rs, rt, rn);
  }
  void ldsetab(Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i8Bit, 1, 0, 0, 0b011, rs, rt, rn);
  }
  void ldsmaxab(Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i8Bit, 1, 0, 0, 0b100, rs, rt, rn);
  }
  void ldsminab(Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i8Bit, 1, 0, 0, 0b101, rs, rt, rn);
  }
  void ldumaxab(Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i8Bit, 1, 0, 0, 0b110, rs, rt, rn);
  }
  void lduminab(Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i8Bit, 1, 0, 0, 0b111, rs, rt, rn);
  }
  void ldswpab(Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i8Bit, 1, 0, 1, 0b000, rs, rt, rn);
  }
  void ldaddalb(Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i8Bit, 1, 1, 0, 0b000, rs, rt, rn);
  }
  void ldclralb(Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i8Bit, 1, 1, 0, 0b001, rs, rt, rn);
  }
  void ldeoralb(Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i8Bit, 1, 1, 0, 0b010, rs, rt, rn);
  }
  void ldsetalb(Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i8Bit, 1, 1, 0, 0b011, rs, rt, rn);
  }
  void ldsmaxalb(Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i8Bit, 1, 1, 0, 0b100, rs, rt, rn);
  }
  void ldsminalb(Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i8Bit, 1, 1, 0, 0b101, rs, rt, rn);
  }
  void ldumaxalb(Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i8Bit, 1, 1, 0, 0b110, rs, rt, rn);
  }
  void lduminalb(Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i8Bit, 1, 1, 0, 0b111, rs, rt, rn);
  }
  void ldswpalb(Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i8Bit, 1, 1, 1, 0b000, rs, rt, rn);
  }
  // 16-bit
  void ldaddh(Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i16Bit, 0, 0, 0, 0b000, rs, rt, rn);
  }
  void ldclrh(Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i16Bit, 0, 0, 0, 0b001, rs, rt, rn);
  }
  void ldeorh(Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i16Bit, 0, 0, 0, 0b010, rs, rt, rn);
  }
  void ldseth(Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i16Bit, 0, 0, 0, 0b011, rs, rt, rn);
  }
  void ldsmaxh(Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i16Bit, 0, 0, 0, 0b100, rs, rt, rn);
  }
  void ldsminh(Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i16Bit, 0, 0, 0, 0b101, rs, rt, rn);
  }
  void ldumaxh(Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i16Bit, 0, 0, 0, 0b110, rs, rt, rn);
  }
  void lduminh(Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i16Bit, 0, 0, 0, 0b111, rs, rt, rn);
  }
  void ldswph(Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i16Bit, 0, 0, 1, 0b000, rs, rt, rn);
  }
  void ldaddlh(Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i16Bit, 0, 1, 0, 0b000, rs, rt, rn);
  }
  void ldclrlh(Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i16Bit, 0, 1, 0, 0b001, rs, rt, rn);
  }
  void ldeorlh(Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i16Bit, 0, 1, 0, 0b010, rs, rt, rn);
  }
  void ldsetlh(Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i16Bit, 0, 1, 0, 0b011, rs, rt, rn);
  }
  void ldsmaxlh(Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i16Bit, 0, 1, 0, 0b100, rs, rt, rn);
  }
  void ldsminlh(Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i16Bit, 0, 1, 0, 0b101, rs, rt, rn);
  }
  void ldumaxlh(Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i16Bit, 0, 1, 0, 0b110, rs, rt, rn);
  }
  void lduminlh(Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i16Bit, 0, 1, 0, 0b111, rs, rt, rn);
  }
  void ldswplh(Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i16Bit, 0, 1, 1, 0b000, rs, rt, rn);
  }
  void ldaddah(Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i16Bit, 1, 0, 0, 0b000, rs, rt, rn);
  }
  void ldclrah(Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i16Bit, 1, 0, 0, 0b001, rs, rt, rn);
  }
  void ldeorah(Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i16Bit, 1, 0, 0, 0b010, rs, rt, rn);
  }
  void ldsetah(Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i16Bit, 1, 0, 0, 0b011, rs, rt, rn);
  }
  void ldsmaxah(Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i16Bit, 1, 0, 0, 0b100, rs, rt, rn);
  }
  void ldsminah(Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i16Bit, 1, 0, 0, 0b101, rs, rt, rn);
  }
  void ldumaxah(Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i16Bit, 1, 0, 0, 0b110, rs, rt, rn);
  }
  void lduminah(Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i16Bit, 1, 0, 0, 0b111, rs, rt, rn);
  }
  void ldswpah(Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i16Bit, 1, 0, 1, 0b000, rs, rt, rn);
  }
  void ldaddalh(Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i16Bit, 1, 1, 0, 0b000, rs, rt, rn);
  }
  void ldclralh(Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i16Bit, 1, 1, 0, 0b001, rs, rt, rn);
  }
  void ldeoralh(Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i16Bit, 1, 1, 0, 0b010, rs, rt, rn);
  }
  void ldsetalh(Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i16Bit, 1, 1, 0, 0b011, rs, rt, rn);
  }
  void ldsmaxalh(Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i16Bit, 1, 1, 0, 0b100, rs, rt, rn);
  }
  void ldsminalh(Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i16Bit, 1, 1, 0, 0b101, rs, rt, rn);
  }
  void ldumaxalh(Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i16Bit, 1, 1, 0, 0b110, rs, rt, rn);
  }
  void lduminalh(Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i16Bit, 1, 1, 0, 0b111, rs, rt, rn);
  }
  void ldswpalh(Register rs, Register rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i16Bit, 1, 1, 1, 0b000, rs, rt, rn);
  }
  // 32-bit
  void ldadd(WRegister rs, WRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i32Bit, 0, 0, 0, 0b000, rs, rt, rn);
  }
  void ldclr(WRegister rs, WRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i32Bit, 0, 0, 0, 0b001, rs, rt, rn);
  }
  void ldeor(WRegister rs, WRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i32Bit, 0, 0, 0, 0b010, rs, rt, rn);
  }
  void ldset(WRegister rs, WRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i32Bit, 0, 0, 0, 0b011, rs, rt, rn);
  }
  void ldsmax(WRegister rs, WRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i32Bit, 0, 0, 0, 0b100, rs, rt, rn);
  }
  void ldsmin(WRegister rs, WRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i32Bit, 0, 0, 0, 0b101, rs, rt, rn);
  }
  void ldumax(WRegister rs, WRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i32Bit, 0, 0, 0, 0b110, rs, rt, rn);
  }
  void ldumin(WRegister rs, WRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i32Bit, 0, 0, 0, 0b111, rs, rt, rn);
  }
  void ldswp(WRegister rs, WRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i32Bit, 0, 0, 1, 0b000, rs, rt, rn);
  }
  void ldaddl(WRegister rs, WRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i32Bit, 0, 1, 0, 0b000, rs, rt, rn);
  }
  void ldclrl(WRegister rs, WRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i32Bit, 0, 1, 0, 0b001, rs, rt, rn);
  }
  void ldeorl(WRegister rs, WRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i32Bit, 0, 1, 0, 0b010, rs, rt, rn);
  }
  void ldsetl(WRegister rs, WRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i32Bit, 0, 1, 0, 0b011, rs, rt, rn);
  }
  void ldsmaxl(WRegister rs, WRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i32Bit, 0, 1, 0, 0b100, rs, rt, rn);
  }
  void ldsminl(WRegister rs, WRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i32Bit, 0, 1, 0, 0b101, rs, rt, rn);
  }
  void ldumaxl(WRegister rs, WRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i32Bit, 0, 1, 0, 0b110, rs, rt, rn);
  }
  void lduminl(WRegister rs, WRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i32Bit, 0, 1, 0, 0b111, rs, rt, rn);
  }
  void ldswpl(WRegister rs, WRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i32Bit, 0, 1, 1, 0b000, rs, rt, rn);
  }
  void ldadda(WRegister rs, WRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i32Bit, 1, 0, 0, 0b000, rs, rt, rn);
  }
  void ldclra(WRegister rs, WRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i32Bit, 1, 0, 0, 0b001, rs, rt, rn);
  }
  void ldeora(WRegister rs, WRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i32Bit, 1, 0, 0, 0b010, rs, rt, rn);
  }
  void ldseta(WRegister rs, WRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i32Bit, 1, 0, 0, 0b011, rs, rt, rn);
  }
  void ldsmaxa(WRegister rs, WRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i32Bit, 1, 0, 0, 0b100, rs, rt, rn);
  }
  void ldsmina(WRegister rs, WRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i32Bit, 1, 0, 0, 0b101, rs, rt, rn);
  }
  void ldumaxa(WRegister rs, WRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i32Bit, 1, 0, 0, 0b110, rs, rt, rn);
  }
  void ldumina(WRegister rs, WRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i32Bit, 1, 0, 0, 0b111, rs, rt, rn);
  }
  void ldswpa(WRegister rs, WRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i32Bit, 1, 0, 1, 0b000, rs, rt, rn);
  }
  void ldaddal(WRegister rs, WRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i32Bit, 1, 1, 0, 0b000, rs, rt, rn);
  }
  void ldclral(WRegister rs, WRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i32Bit, 1, 1, 0, 0b001, rs, rt, rn);
  }
  void ldeoral(WRegister rs, WRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i32Bit, 1, 1, 0, 0b010, rs, rt, rn);
  }
  void ldsetal(WRegister rs, WRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i32Bit, 1, 1, 0, 0b011, rs, rt, rn);
  }
  void ldsmaxal(WRegister rs, WRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i32Bit, 1, 1, 0, 0b100, rs, rt, rn);
  }
  void ldsminal(WRegister rs, WRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i32Bit, 1, 1, 0, 0b101, rs, rt, rn);
  }
  void ldumaxal(WRegister rs, WRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i32Bit, 1, 1, 0, 0b110, rs, rt, rn);
  }
  void lduminal(WRegister rs, WRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i32Bit, 1, 1, 0, 0b111, rs, rt, rn);
  }
  void ldswpal(WRegister rs, WRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i32Bit, 1, 1, 1, 0b000, rs, rt, rn);
  }
  // 64-bit
  void ldadd(XRegister rs, XRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i64Bit, 0, 0, 0, 0b000, rs, rt, rn);
  }
  void ldclr(XRegister rs, XRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i64Bit, 0, 0, 0, 0b001, rs, rt, rn);
  }
  void ldeor(XRegister rs, XRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i64Bit, 0, 0, 0, 0b010, rs, rt, rn);
  }
  void ldset(XRegister rs, XRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i64Bit, 0, 0, 0, 0b011, rs, rt, rn);
  }
  void ldsmax(XRegister rs, XRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i64Bit, 0, 0, 0, 0b100, rs, rt, rn);
  }
  void ldsmin(XRegister rs, XRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i64Bit, 0, 0, 0, 0b101, rs, rt, rn);
  }
  void ldumax(XRegister rs, XRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i64Bit, 0, 0, 0, 0b110, rs, rt, rn);
  }
  void ldumin(XRegister rs, XRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i64Bit, 0, 0, 0, 0b111, rs, rt, rn);
  }
  void ldswp(XRegister rs, XRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i64Bit, 0, 0, 1, 0b000, rs, rt, rn);
  }
  void ldaddl(XRegister rs, XRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i64Bit, 0, 1, 0, 0b000, rs, rt, rn);
  }
  void ldclrl(XRegister rs, XRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i64Bit, 0, 1, 0, 0b001, rs, rt, rn);
  }
  void ldeorl(XRegister rs, XRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i64Bit, 0, 1, 0, 0b010, rs, rt, rn);
  }
  void ldsetl(XRegister rs, XRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i64Bit, 0, 1, 0, 0b011, rs, rt, rn);
  }
  void ldsmaxl(XRegister rs, XRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i64Bit, 0, 1, 0, 0b100, rs, rt, rn);
  }
  void ldsminl(XRegister rs, XRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i64Bit, 0, 1, 0, 0b101, rs, rt, rn);
  }
  void ldumaxl(XRegister rs, XRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i64Bit, 0, 1, 0, 0b110, rs, rt, rn);
  }
  void lduminl(XRegister rs, XRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i64Bit, 0, 1, 0, 0b111, rs, rt, rn);
  }
  void ldswpl(XRegister rs, XRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i64Bit, 0, 1, 1, 0b000, rs, rt, rn);
  }
  void ldadda(XRegister rs, XRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i64Bit, 1, 0, 0, 0b000, rs, rt, rn);
  }
  void ldclra(XRegister rs, XRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i64Bit, 1, 0, 0, 0b001, rs, rt, rn);
  }
  void ldeora(XRegister rs, XRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i64Bit, 1, 0, 0, 0b010, rs, rt, rn);
  }
  void ldseta(XRegister rs, XRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i64Bit, 1, 0, 0, 0b011, rs, rt, rn);
  }
  void ldsmaxa(XRegister rs, XRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i64Bit, 1, 0, 0, 0b100, rs, rt, rn);
  }
  void ldsmina(XRegister rs, XRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i64Bit, 1, 0, 0, 0b101, rs, rt, rn);
  }
  void ldumaxa(XRegister rs, XRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i64Bit, 1, 0, 0, 0b110, rs, rt, rn);
  }
  void ldumina(XRegister rs, XRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i64Bit, 1, 0, 0, 0b111, rs, rt, rn);
  }
  void ldswpa(XRegister rs, XRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i64Bit, 1, 0, 1, 0b000, rs, rt, rn);
  }
  void ldaddal(XRegister rs, XRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i64Bit, 1, 1, 0, 0b000, rs, rt, rn);
  }
  void ldclral(XRegister rs, XRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i64Bit, 1, 1, 0, 0b001, rs, rt, rn);
  }
  void ldeoral(XRegister rs, XRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i64Bit, 1, 1, 0, 0b010, rs, rt, rn);
  }
  void ldsetal(XRegister rs, XRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i64Bit, 1, 1, 0, 0b011, rs, rt, rn);
  }
  void ldsmaxal(XRegister rs, XRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i64Bit, 1, 1, 0, 0b100, rs, rt, rn);
  }
  void ldsminal(XRegister rs, XRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i64Bit, 1, 1, 0, 0b101, rs, rt, rn);
  }
  void ldumaxal(XRegister rs, XRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i64Bit, 1, 1, 0, 0b110, rs, rt, rn);
  }
  void lduminal(XRegister rs, XRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i64Bit, 1, 1, 0, 0b111, rs, rt, rn);
  }
  void ldswpal(XRegister rs, XRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i64Bit, 1, 1, 1, 0b000, rs, rt, rn);
  }
  void ldaprb(WRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i8Bit, 1, 0, 1, 0b100, WReg::w31, rt, rn);
  }
  void ldaprh(WRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i16Bit, 1, 0, 1, 0b100, WReg::w31, rt, rn);
  }
  void ldapr(WRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i32Bit, 1, 0, 1, 0b100, WReg::w31, rt, rn);
  }
  void ldapr(XRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i64Bit, 1, 0, 1, 0b100, XReg::x31, rt, rn);
  }
  void st64bv0(XRegister rs, XRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i64Bit, 0, 0, 1, 0b010, rs, rt, rn);
  }
  void st64bv(XRegister rs, XRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i64Bit, 0, 0, 1, 0b011, rs, rt, rn);
  }
  void st64b(XRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i64Bit, 0, 0, 1, 0b001, XReg::x31, rt, rn);
  }
  void ld64b(XRegister rt, Register rn) {
    LoadStoreAtomicLSE(SubRegSize::i64Bit, 0, 0, 1, 0b101, XReg::x31, rt, rn);
  }

  // Loadstore register-register offset
  void strb(ARMEmitter::Register rt, ARMEmitter::Register rn, ARMEmitter::Register rm, ARMEmitter::ExtendedType Option, bool Shift = false) {
    LOGMAN_THROW_A_FMT((FEXCore::ToUnderlying(Option) & 0b010) == 0b010, "Unsupported Extendtype");
    constexpr uint32_t Op = 0b0011'1000'001 << 21 | (0b10 << 10);
    LoadStoreRegisterOffset(Op, 0b00, rt, rn, rm, Option, Shift ? 1 : 0);
  }
  void ldrb(ARMEmitter::Register rt, ARMEmitter::Register rn, ARMEmitter::Register rm, ARMEmitter::ExtendedType Option, bool Shift = false) {
    LOGMAN_THROW_A_FMT((FEXCore::ToUnderlying(Option) & 0b010) == 0b010, "Unsupported Extendtype");
    constexpr uint32_t Op = 0b0011'1000'001 << 21 | (0b10 << 10);
    LoadStoreRegisterOffset(Op, 0b01, rt, rn, rm, Option, Shift ? 1 : 0);
  }
  void ldrsb(ARMEmitter::XRegister rt, ARMEmitter::Register rn, ARMEmitter::Register rm, ARMEmitter::ExtendedType Option, bool Shift = false) {
    LOGMAN_THROW_A_FMT((FEXCore::ToUnderlying(Option) & 0b010) == 0b010, "Unsupported Extendtype");
    constexpr uint32_t Op = 0b0011'1000'001 << 21 | (0b10 << 10);
    LoadStoreRegisterOffset(Op, 0b10, rt, rn, rm, Option, Shift ? 1 : 0);
  }
  void ldrsb(ARMEmitter::WRegister rt, ARMEmitter::Register rn, ARMEmitter::Register rm, ARMEmitter::ExtendedType Option, bool Shift = false) {
    LOGMAN_THROW_A_FMT((FEXCore::ToUnderlying(Option) & 0b010) == 0b010, "Unsupported Extendtype");
    constexpr uint32_t Op = 0b0011'1000'001 << 21 | (0b10 << 10);
    LoadStoreRegisterOffset(Op, 0b11, rt, rn, rm, Option, Shift ? 1 : 0);
  }
  void strh(ARMEmitter::Register rt, ARMEmitter::Register rn, ARMEmitter::Register rm, ARMEmitter::ExtendedType Option, uint32_t Shift) {
    LOGMAN_THROW_A_FMT((FEXCore::ToUnderlying(Option) & 0b010) == 0b010, "Unsupported Extendtype");
    LOGMAN_THROW_A_FMT(Shift == 0 || Shift == 1, "Unsupported shift amount");
    constexpr uint32_t Op = 0b0111'1000'001 << 21 | (0b10 << 10);
    LoadStoreRegisterOffset(Op, 0b00, rt, rn, rm, Option, Shift ? 1 : 0);
  }
  void ldrh(ARMEmitter::Register rt, ARMEmitter::Register rn, ARMEmitter::Register rm, ARMEmitter::ExtendedType Option, uint32_t Shift) {
    LOGMAN_THROW_A_FMT((FEXCore::ToUnderlying(Option) & 0b010) == 0b010, "Unsupported Extendtype");
    LOGMAN_THROW_A_FMT(Shift == 0 || Shift == 1, "Unsupported shift amount");
    constexpr uint32_t Op = 0b0111'1000'001 << 21 | (0b10 << 10);
    LoadStoreRegisterOffset(Op, 0b01, rt, rn, rm, Option, Shift ? 1 : 0);
  }
  void ldrsh(ARMEmitter::XRegister rt, ARMEmitter::Register rn, ARMEmitter::Register rm, ARMEmitter::ExtendedType Option, uint32_t Shift) {
    LOGMAN_THROW_A_FMT((FEXCore::ToUnderlying(Option) & 0b010) == 0b010, "Unsupported Extendtype");
    LOGMAN_THROW_A_FMT(Shift == 0 || Shift == 1, "Unsupported shift amount");
    constexpr uint32_t Op = 0b0111'1000'001 << 21 | (0b10 << 10);
    LoadStoreRegisterOffset(Op, 0b10, rt, rn, rm, Option, Shift ? 1 : 0);
  }
  void ldrsh(ARMEmitter::WRegister rt, ARMEmitter::Register rn, ARMEmitter::Register rm, ARMEmitter::ExtendedType Option, uint32_t Shift) {
    LOGMAN_THROW_A_FMT((FEXCore::ToUnderlying(Option) & 0b010) == 0b010, "Unsupported Extendtype");
    LOGMAN_THROW_A_FMT(Shift == 0 || Shift == 1, "Unsupported shift amount");
    constexpr uint32_t Op = 0b0111'1000'001 << 21 | (0b10 << 10);
    LoadStoreRegisterOffset(Op, 0b11, rt, rn, rm, Option, Shift ? 1 : 0);
  }
  void str(ARMEmitter::WRegister rt, ARMEmitter::Register rn, ARMEmitter::Register rm, ARMEmitter::ExtendedType Option, uint32_t Shift) {
    LOGMAN_THROW_A_FMT((FEXCore::ToUnderlying(Option) & 0b010) == 0b010, "Unsupported Extendtype");
    LOGMAN_THROW_A_FMT(Shift == 0 || Shift == 2, "Unsupported shift amount");
    constexpr uint32_t Op = 0b1011'1000'001 << 21 | (0b10 << 10);
    LoadStoreRegisterOffset(Op, 0b00, rt, rn, rm, Option, Shift ? 1 : 0);
  }
  void ldr(ARMEmitter::WRegister rt, ARMEmitter::Register rn, ARMEmitter::Register rm, ARMEmitter::ExtendedType Option, uint32_t Shift) {
    LOGMAN_THROW_A_FMT((FEXCore::ToUnderlying(Option) & 0b010) == 0b010, "Unsupported Extendtype");
    LOGMAN_THROW_A_FMT(Shift == 0 || Shift == 2, "Unsupported shift amount: {}", Shift);
    constexpr uint32_t Op = 0b1011'1000'001 << 21 | (0b10 << 10);
    LoadStoreRegisterOffset(Op, 0b01, rt, rn, rm, Option, Shift ? 1 : 0);
  }
  void ldrsw(ARMEmitter::XRegister rt, ARMEmitter::Register rn, ARMEmitter::Register rm, ARMEmitter::ExtendedType Option, uint32_t Shift) {
    LOGMAN_THROW_A_FMT((FEXCore::ToUnderlying(Option) & 0b010) == 0b010, "Unsupported Extendtype");
    LOGMAN_THROW_A_FMT(Shift == 0 || Shift == 2, "Unsupported shift amount");
    constexpr uint32_t Op = 0b1011'1000'001 << 21 | (0b10 << 10);
    LoadStoreRegisterOffset(Op, 0b10, rt, rn, rm, Option, Shift ? 1 : 0);
  }
  void str(ARMEmitter::XRegister rt, ARMEmitter::Register rn, ARMEmitter::Register rm, ARMEmitter::ExtendedType Option, uint32_t Shift) {
    LOGMAN_THROW_A_FMT((FEXCore::ToUnderlying(Option) & 0b010) == 0b010, "Unsupported Extendtype");
    LOGMAN_THROW_A_FMT(Shift == 0 || Shift == 3, "Unsupported shift amount");
    constexpr uint32_t Op = 0b1111'1000'001 << 21 | (0b10 << 10);
    LoadStoreRegisterOffset(Op, 0b00, rt, rn, rm, Option, Shift ? 1 : 0);
  }
  void ldr(ARMEmitter::XRegister rt, ARMEmitter::Register rn, ARMEmitter::Register rm, ARMEmitter::ExtendedType Option, uint32_t Shift) {
    LOGMAN_THROW_A_FMT((FEXCore::ToUnderlying(Option) & 0b010) == 0b010, "Unsupported Extendtype");
    LOGMAN_THROW_A_FMT(Shift == 0 || Shift == 3, "Unsupported shift amount");
    constexpr uint32_t Op = 0b1111'1000'001 << 21 | (0b10 << 10);
    LoadStoreRegisterOffset(Op, 0b01, rt, rn, rm, Option, Shift ? 1 : 0);
  }
  void prfm(ARMEmitter::Prefetch prfop, ARMEmitter::Register rn, ARMEmitter::Register rm, ARMEmitter::ExtendedType Option, uint32_t Shift) {
    LOGMAN_THROW_A_FMT((FEXCore::ToUnderlying(Option) & 0b010) == 0b010, "Unsupported Extendtype");
    LOGMAN_THROW_A_FMT(Shift == 0 || Shift == 3, "Unsupported shift amount");
    constexpr uint32_t Op = 0b1111'1000'001 << 21 | (0b10 << 10);
    LoadStoreRegisterOffset(Op, 0b10, prfop, rn, rm, Option, Shift ? 1 : 0);
  }
  void strb(ARMEmitter::VRegister rt, ARMEmitter::Register rn, ARMEmitter::Register rm, ARMEmitter::ExtendedType Option) {
    LOGMAN_THROW_A_FMT((FEXCore::ToUnderlying(Option) & 0b010) == 0b010, "Unsupported Extendtype");
    constexpr uint32_t Op = 0b0011'1100'001 << 21 | (0b10 << 10);
    LoadStoreRegisterOffset(Op, 0b00, rt, rn, rm, Option, 0);
  }
  void ldrb(ARMEmitter::VRegister rt, ARMEmitter::Register rn, ARMEmitter::Register rm, ARMEmitter::ExtendedType Option) {
    LOGMAN_THROW_A_FMT((FEXCore::ToUnderlying(Option) & 0b010) == 0b010, "Unsupported Extendtype");
    constexpr uint32_t Op = 0b0011'1100'001 << 21 | (0b10 << 10);
    LoadStoreRegisterOffset(Op, 0b01, rt, rn, rm, Option, 0);
  }
  void strh(ARMEmitter::VRegister rt, ARMEmitter::Register rn, ARMEmitter::Register rm, ARMEmitter::ExtendedType Option, uint32_t Shift) {
    LOGMAN_THROW_A_FMT((FEXCore::ToUnderlying(Option) & 0b010) == 0b010, "Unsupported Extendtype");
    LOGMAN_THROW_A_FMT(Shift == 0 || Shift == 1, "Unsupported shift amount");
    constexpr uint32_t Op = 0b0111'1100'001 << 21 | (0b10 << 10);
    LoadStoreRegisterOffset(Op, 0b00, rt, rn, rm, Option, Shift ? 1 : 0);
  }
  void ldrh(ARMEmitter::VRegister rt, ARMEmitter::Register rn, ARMEmitter::Register rm, ARMEmitter::ExtendedType Option, uint32_t Shift) {
    LOGMAN_THROW_A_FMT((FEXCore::ToUnderlying(Option) & 0b010) == 0b010, "Unsupported Extendtype");
    LOGMAN_THROW_A_FMT(Shift == 0 || Shift == 1, "Unsupported shift amount");
    constexpr uint32_t Op = 0b0111'1100'001 << 21 | (0b10 << 10);
    LoadStoreRegisterOffset(Op, 0b01, rt, rn, rm, Option, Shift ? 1 : 0);
  }
  void str(ARMEmitter::SRegister rt, ARMEmitter::Register rn, ARMEmitter::Register rm, ARMEmitter::ExtendedType Option, uint32_t Shift) {
    LOGMAN_THROW_A_FMT((FEXCore::ToUnderlying(Option) & 0b010) == 0b010, "Unsupported Extendtype");
    LOGMAN_THROW_A_FMT(Shift == 0 || Shift == 2, "Unsupported shift amount");
    constexpr uint32_t Op = 0b1011'1100'001 << 21 | (0b10 << 10);
    LoadStoreRegisterOffset(Op, 0b00, rt, rn, rm, Option, Shift ? 1 : 0);
  }
  void ldr(ARMEmitter::SRegister rt, ARMEmitter::Register rn, ARMEmitter::Register rm, ARMEmitter::ExtendedType Option, uint32_t Shift) {
    LOGMAN_THROW_A_FMT((FEXCore::ToUnderlying(Option) & 0b010) == 0b010, "Unsupported Extendtype");
    LOGMAN_THROW_A_FMT(Shift == 0 || Shift == 2, "Unsupported shift amount");
    constexpr uint32_t Op = 0b1011'1100'001 << 21 | (0b10 << 10);
    LoadStoreRegisterOffset(Op, 0b01, rt, rn, rm, Option, Shift ? 1 : 0);
  }
  void str(ARMEmitter::DRegister rt, ARMEmitter::Register rn, ARMEmitter::Register rm, ARMEmitter::ExtendedType Option, uint32_t Shift) {
    LOGMAN_THROW_A_FMT((FEXCore::ToUnderlying(Option) & 0b010) == 0b010, "Unsupported Extendtype");
    LOGMAN_THROW_A_FMT(Shift == 0 || Shift == 3, "Unsupported shift amount");
    constexpr uint32_t Op = 0b1111'1100'001 << 21 | (0b10 << 10);
    LoadStoreRegisterOffset(Op, 0b00, rt, rn, rm, Option, Shift ? 1 : 0);
  }
  void ldr(ARMEmitter::DRegister rt, ARMEmitter::Register rn, ARMEmitter::Register rm, ARMEmitter::ExtendedType Option, uint32_t Shift) {
    LOGMAN_THROW_A_FMT((FEXCore::ToUnderlying(Option) & 0b010) == 0b010, "Unsupported Extendtype");
    LOGMAN_THROW_A_FMT(Shift == 0 || Shift == 3, "Unsupported shift amount");
    constexpr uint32_t Op = 0b1111'1100'001 << 21 | (0b10 << 10);
    LoadStoreRegisterOffset(Op, 0b01, rt, rn, rm, Option, Shift ? 1 : 0);
  }
  void str(ARMEmitter::QRegister rt, ARMEmitter::Register rn, ARMEmitter::Register rm, ARMEmitter::ExtendedType Option, uint32_t Shift) {
    LOGMAN_THROW_A_FMT((FEXCore::ToUnderlying(Option) & 0b010) == 0b010, "Unsupported Extendtype");
    LOGMAN_THROW_A_FMT(Shift == 0 || Shift == 4, "Unsupported shift amount");
    constexpr uint32_t Op = 0b0011'1100'001 << 21 | (0b10 << 10);
    LoadStoreRegisterOffset(Op, 0b10, rt, rn, rm, Option, Shift ? 1 : 0);
  }
  void ldr(ARMEmitter::QRegister rt, ARMEmitter::Register rn, ARMEmitter::Register rm, ARMEmitter::ExtendedType Option, uint32_t Shift) {
    LOGMAN_THROW_A_FMT((FEXCore::ToUnderlying(Option) & 0b010) == 0b010, "Unsupported Extendtype");
    LOGMAN_THROW_A_FMT(Shift == 0 || Shift == 4, "Unsupported shift amount");
    constexpr uint32_t Op = 0b0011'1100'001 << 21 | (0b10 << 10);
    LoadStoreRegisterOffset(Op, 0b11, rt, rn, rm, Option, Shift ? 1 : 0);
  }

  void strb(ARMEmitter::Register rt, ARMEmitter::ExtendedMemOperand MemSrc) {
    if (MemSrc.MetaType.Header.MemType == ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED &&
        MemSrc.MetaType.ExtendedType.rm.Idx() != ARMEmitter::Reg::r31.Idx()) {
      strb(rt, MemSrc.rn, MemSrc.MetaType.ExtendedType.rm, MemSrc.MetaType.ExtendedType.Option, MemSrc.MetaType.ExtendedType.Shift);
    } else if (MemSrc.MetaType.Header.MemType == ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED) {
      strb(rt, MemSrc.rn);
    } else {
      if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::OFFSET) {
        if (MemSrc.MetaType.ImmType.Imm < 0) {
          sturb(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        } else {
          strb(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        }
      } else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::POST) {
        strb<ARMEmitter::IndexType::POST>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      } else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::PRE) {
        strb<ARMEmitter::IndexType::PRE>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      } else {
        LOGMAN_MSG_A_FMT("Unexpected loadstore index type");
        FEX_UNREACHABLE;
      }
    }
  }
  void ldrb(ARMEmitter::Register rt, ARMEmitter::ExtendedMemOperand MemSrc) {
    if (MemSrc.MetaType.Header.MemType == ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED &&
        MemSrc.MetaType.ExtendedType.rm.Idx() != ARMEmitter::Reg::r31.Idx()) {
      ldrb(rt, MemSrc.rn, MemSrc.MetaType.ExtendedType.rm, MemSrc.MetaType.ExtendedType.Option, MemSrc.MetaType.ExtendedType.Shift);
    } else if (MemSrc.MetaType.Header.MemType == ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED) {
      ldrb(rt, MemSrc.rn);
    } else {
      if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::OFFSET) {
        if (MemSrc.MetaType.ImmType.Imm < 0) {
          ldurb(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        } else {
          ldrb(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        }
      } else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::POST) {
        ldrb<ARMEmitter::IndexType::POST>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      } else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::PRE) {
        ldrb<ARMEmitter::IndexType::PRE>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      } else {
        LOGMAN_MSG_A_FMT("Unexpected loadstore index type");
        FEX_UNREACHABLE;
      }
    }
  }
  void ldrsb(ARMEmitter::XRegister rt, ARMEmitter::ExtendedMemOperand MemSrc) {
    if (MemSrc.MetaType.Header.MemType == ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED &&
        MemSrc.MetaType.ExtendedType.rm.Idx() != ARMEmitter::Reg::r31.Idx()) {
      ldrsb(rt, MemSrc.rn, MemSrc.MetaType.ExtendedType.rm, MemSrc.MetaType.ExtendedType.Option, MemSrc.MetaType.ExtendedType.Shift);
    } else if (MemSrc.MetaType.Header.MemType == ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED) {
      ldrsb(rt, MemSrc.rn);
    } else {
      if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::OFFSET) {
        if (MemSrc.MetaType.ImmType.Imm < 0) {
          ldursb(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        } else {
          ldrsb(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        }
      } else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::POST) {
        ldrsb<ARMEmitter::IndexType::POST>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      } else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::PRE) {
        ldrsb<ARMEmitter::IndexType::PRE>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      } else {
        LOGMAN_MSG_A_FMT("Unexpected loadstore index type");
        FEX_UNREACHABLE;
      }
    }
  }
  void ldrsb(ARMEmitter::WRegister rt, ARMEmitter::ExtendedMemOperand MemSrc) {
    if (MemSrc.MetaType.Header.MemType == ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED &&
        MemSrc.MetaType.ExtendedType.rm.Idx() != ARMEmitter::Reg::r31.Idx()) {
      ldrsb(rt, MemSrc.rn, MemSrc.MetaType.ExtendedType.rm, MemSrc.MetaType.ExtendedType.Option, MemSrc.MetaType.ExtendedType.Shift);
    } else if (MemSrc.MetaType.Header.MemType == ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED) {
      ldrsb(rt, MemSrc.rn);
    } else {
      if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::OFFSET) {
        if (MemSrc.MetaType.ImmType.Imm < 0) {
          ldursb(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        } else {
          ldrsb(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        }
      } else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::POST) {
        ldrsb<ARMEmitter::IndexType::POST>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      } else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::PRE) {
        ldrsb<ARMEmitter::IndexType::PRE>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      } else {
        LOGMAN_MSG_A_FMT("Unexpected loadstore index type");
        FEX_UNREACHABLE;
      }
    }
  }
  void strh(ARMEmitter::Register rt, ARMEmitter::ExtendedMemOperand MemSrc) {
    if (MemSrc.MetaType.Header.MemType == ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED &&
        MemSrc.MetaType.ExtendedType.rm.Idx() != ARMEmitter::Reg::r31.Idx()) {
      strh(rt, MemSrc.rn, MemSrc.MetaType.ExtendedType.rm, MemSrc.MetaType.ExtendedType.Option, MemSrc.MetaType.ExtendedType.Shift);
    } else if (MemSrc.MetaType.Header.MemType == ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED) {
      strh(rt, MemSrc.rn);
    } else {
      if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::OFFSET) {
        if ((MemSrc.MetaType.ImmType.Imm & 0b1) || MemSrc.MetaType.ImmType.Imm < 0) {
          sturh(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        } else {
          strh(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        }
      } else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::POST) {
        strh<ARMEmitter::IndexType::POST>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      } else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::PRE) {
        strh<ARMEmitter::IndexType::PRE>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      } else {
        LOGMAN_MSG_A_FMT("Unexpected loadstore index type");
        FEX_UNREACHABLE;
      }
    }
  }
  void ldrh(ARMEmitter::Register rt, ARMEmitter::ExtendedMemOperand MemSrc) {
    if (MemSrc.MetaType.Header.MemType == ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED &&
        MemSrc.MetaType.ExtendedType.rm.Idx() != ARMEmitter::Reg::r31.Idx()) {
      ldrh(rt, MemSrc.rn, MemSrc.MetaType.ExtendedType.rm, MemSrc.MetaType.ExtendedType.Option, MemSrc.MetaType.ExtendedType.Shift);
    } else if (MemSrc.MetaType.Header.MemType == ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED) {
      ldrh(rt, MemSrc.rn);
    } else {
      if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::OFFSET) {
        if ((MemSrc.MetaType.ImmType.Imm & 0b1) || MemSrc.MetaType.ImmType.Imm < 0) {
          ldurh(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        } else {
          ldrh(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        }
      } else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::POST) {
        ldrh<ARMEmitter::IndexType::POST>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      } else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::PRE) {
        ldrh<ARMEmitter::IndexType::PRE>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      } else {
        LOGMAN_MSG_A_FMT("Unexpected loadstore index type");
        FEX_UNREACHABLE;
      }
    }
  }
  void ldrsh(ARMEmitter::XRegister rt, ARMEmitter::ExtendedMemOperand MemSrc) {
    if (MemSrc.MetaType.Header.MemType == ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED &&
        MemSrc.MetaType.ExtendedType.rm.Idx() != ARMEmitter::Reg::r31.Idx()) {
      ldrsh(rt, MemSrc.rn, MemSrc.MetaType.ExtendedType.rm, MemSrc.MetaType.ExtendedType.Option, MemSrc.MetaType.ExtendedType.Shift);
    } else if (MemSrc.MetaType.Header.MemType == ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED) {
      ldrsh(rt, MemSrc.rn);
    } else {
      if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::OFFSET) {
        if ((MemSrc.MetaType.ImmType.Imm & 0b1) || MemSrc.MetaType.ImmType.Imm < 0) {
          ldursh(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        } else {
          ldrsh(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        }
      } else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::POST) {
        ldrsh<ARMEmitter::IndexType::POST>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      } else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::PRE) {
        ldrsh<ARMEmitter::IndexType::PRE>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      } else {
        LOGMAN_MSG_A_FMT("Unexpected loadstore index type");
        FEX_UNREACHABLE;
      }
    }
  }
  void ldrsh(ARMEmitter::WRegister rt, ARMEmitter::ExtendedMemOperand MemSrc) {
    if (MemSrc.MetaType.Header.MemType == ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED &&
        MemSrc.MetaType.ExtendedType.rm.Idx() != ARMEmitter::Reg::r31.Idx()) {
      ldrsh(rt, MemSrc.rn, MemSrc.MetaType.ExtendedType.rm, MemSrc.MetaType.ExtendedType.Option, MemSrc.MetaType.ExtendedType.Shift);
    } else if (MemSrc.MetaType.Header.MemType == ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED) {
      ldrsh(rt, MemSrc.rn);
    } else {
      if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::OFFSET) {
        if ((MemSrc.MetaType.ImmType.Imm & 0b1) || MemSrc.MetaType.ImmType.Imm < 0) {
          ldursh(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        } else {
          ldrsh(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        }
      } else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::POST) {
        ldrsh<ARMEmitter::IndexType::POST>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      } else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::PRE) {
        ldrsh<ARMEmitter::IndexType::PRE>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      } else {
        LOGMAN_MSG_A_FMT("Unexpected loadstore index type");
        FEX_UNREACHABLE;
      }
    }
  }
  void str(ARMEmitter::WRegister rt, ARMEmitter::ExtendedMemOperand MemSrc) {
    if (MemSrc.MetaType.Header.MemType == ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED &&
        MemSrc.MetaType.ExtendedType.rm.Idx() != ARMEmitter::Reg::r31.Idx()) {
      str(rt, MemSrc.rn, MemSrc.MetaType.ExtendedType.rm, MemSrc.MetaType.ExtendedType.Option, MemSrc.MetaType.ExtendedType.Shift);
    } else if (MemSrc.MetaType.Header.MemType == ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED) {
      str(rt, MemSrc.rn);
    } else {
      if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::OFFSET) {
        if ((MemSrc.MetaType.ImmType.Imm & 0b11) || MemSrc.MetaType.ImmType.Imm < 0) {
          stur(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        } else {
          str(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        }
      } else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::POST) {
        str<ARMEmitter::IndexType::POST>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      } else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::PRE) {
        str<ARMEmitter::IndexType::PRE>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      } else {
        LOGMAN_MSG_A_FMT("Unexpected loadstore index type");
        FEX_UNREACHABLE;
      }
    }
  }
  void ldr(ARMEmitter::WRegister rt, ARMEmitter::ExtendedMemOperand MemSrc) {
    if (MemSrc.MetaType.Header.MemType == ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED &&
        MemSrc.MetaType.ExtendedType.rm.Idx() != ARMEmitter::Reg::r31.Idx()) {
      ldr(rt, MemSrc.rn, MemSrc.MetaType.ExtendedType.rm, MemSrc.MetaType.ExtendedType.Option, MemSrc.MetaType.ExtendedType.Shift);
    } else if (MemSrc.MetaType.Header.MemType == ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED) {
      ldr(rt, MemSrc.rn);
    } else {
      if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::OFFSET) {
        if ((MemSrc.MetaType.ImmType.Imm & 0b11) || MemSrc.MetaType.ImmType.Imm < 0) {
          ldur(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        } else {
          ldr(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        }
      } else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::POST) {
        ldr<ARMEmitter::IndexType::POST>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      } else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::PRE) {
        ldr<ARMEmitter::IndexType::PRE>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      } else {
        LOGMAN_MSG_A_FMT("Unexpected loadstore index type");
        FEX_UNREACHABLE;
      }
    }
  }
  void ldrsw(ARMEmitter::XRegister rt, ARMEmitter::ExtendedMemOperand MemSrc) {
    if (MemSrc.MetaType.Header.MemType == ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED &&
        MemSrc.MetaType.ExtendedType.rm.Idx() != ARMEmitter::Reg::r31.Idx()) {
      ldrsw(rt, MemSrc.rn, MemSrc.MetaType.ExtendedType.rm, MemSrc.MetaType.ExtendedType.Option, MemSrc.MetaType.ExtendedType.Shift);
    } else if (MemSrc.MetaType.Header.MemType == ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED) {
      ldrsw(rt, MemSrc.rn);
    } else {
      if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::OFFSET) {
        if ((MemSrc.MetaType.ImmType.Imm & 0b11) || MemSrc.MetaType.ImmType.Imm < 0) {
          ldursw(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        } else {
          ldrsw(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        }
      } else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::POST) {
        ldrsw<ARMEmitter::IndexType::POST>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      } else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::PRE) {
        ldrsw<ARMEmitter::IndexType::PRE>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      } else {
        LOGMAN_MSG_A_FMT("Unexpected loadstore index type");
        FEX_UNREACHABLE;
      }
    }
  }
  void str(ARMEmitter::XRegister rt, ARMEmitter::ExtendedMemOperand MemSrc) {
    if (MemSrc.MetaType.Header.MemType == ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED &&
        MemSrc.MetaType.ExtendedType.rm.Idx() != ARMEmitter::Reg::r31.Idx()) {
      str(rt, MemSrc.rn, MemSrc.MetaType.ExtendedType.rm, MemSrc.MetaType.ExtendedType.Option, MemSrc.MetaType.ExtendedType.Shift);
    } else if (MemSrc.MetaType.Header.MemType == ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED) {
      str(rt, MemSrc.rn);
    } else {
      if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::OFFSET) {
        if ((MemSrc.MetaType.ImmType.Imm & 0b111) || MemSrc.MetaType.ImmType.Imm < 0) {
          stur(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        } else {
          str(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        }
      } else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::POST) {
        str<ARMEmitter::IndexType::POST>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      } else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::PRE) {
        str<ARMEmitter::IndexType::PRE>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      } else {
        LOGMAN_MSG_A_FMT("Unexpected loadstore index type");
        FEX_UNREACHABLE;
      }
    }
  }
  void ldr(ARMEmitter::XRegister rt, ARMEmitter::ExtendedMemOperand MemSrc) {
    if (MemSrc.MetaType.Header.MemType == ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED &&
        MemSrc.MetaType.ExtendedType.rm.Idx() != ARMEmitter::Reg::r31.Idx()) {
      ldr(rt, MemSrc.rn, MemSrc.MetaType.ExtendedType.rm, MemSrc.MetaType.ExtendedType.Option, MemSrc.MetaType.ExtendedType.Shift);
    } else if (MemSrc.MetaType.Header.MemType == ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED) {
      ldr(rt, MemSrc.rn);
    } else {
      if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::OFFSET) {
        if ((MemSrc.MetaType.ImmType.Imm & 0b111) || MemSrc.MetaType.ImmType.Imm < 0) {
          ldur(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        } else {
          ldr(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        }
      } else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::POST) {
        ldr<ARMEmitter::IndexType::POST>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      } else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::PRE) {
        ldr<ARMEmitter::IndexType::PRE>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      } else {
        LOGMAN_MSG_A_FMT("Unexpected loadstore index type");
        FEX_UNREACHABLE;
      }
    }
  }
  void prfm(ARMEmitter::Prefetch prfop, ARMEmitter::ExtendedMemOperand MemSrc) {
    if (MemSrc.MetaType.Header.MemType == ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED &&
        MemSrc.MetaType.ExtendedType.rm.Idx() != ARMEmitter::Reg::r31.Idx()) {
      prfm(prfop, MemSrc.rn, MemSrc.MetaType.ExtendedType.rm, MemSrc.MetaType.ExtendedType.Option, MemSrc.MetaType.ExtendedType.Shift);
    } else if (MemSrc.MetaType.Header.MemType == ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED) {
      prfm(prfop, MemSrc.rn);
    } else {
      if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::OFFSET) {
        if ((MemSrc.MetaType.ImmType.Imm & 0b111) || MemSrc.MetaType.ImmType.Imm < 0) {
          prfum<IndexType::OFFSET>(prfop, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        } else {
          prfm(prfop, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        }
      } else {
        LOGMAN_MSG_A_FMT("Unexpected loadstore index type");
        FEX_UNREACHABLE;
      }
    }
  }

  void strb(ARMEmitter::VRegister rt, ARMEmitter::ExtendedMemOperand MemSrc) {
    if (MemSrc.MetaType.Header.MemType == ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED &&
        MemSrc.MetaType.ExtendedType.rm.Idx() != ARMEmitter::Reg::r31.Idx()) {
      LOGMAN_THROW_A_FMT(MemSrc.MetaType.ExtendedType.Shift == false, "Can't shift byte");
      strb(rt, MemSrc.rn, MemSrc.MetaType.ExtendedType.rm, MemSrc.MetaType.ExtendedType.Option);
    } else if (MemSrc.MetaType.Header.MemType == ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED) {
      strb(rt, MemSrc.rn);
    } else {
      if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::OFFSET) {
        if (MemSrc.MetaType.ImmType.Imm < 0) {
          sturb(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        } else {
          strb(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        }
      } else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::POST) {
        strb<ARMEmitter::IndexType::POST>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      } else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::PRE) {
        strb<ARMEmitter::IndexType::PRE>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      } else {
        LOGMAN_MSG_A_FMT("Unexpected loadstore index type");
        FEX_UNREACHABLE;
      }
    }
  }
  void ldrb(ARMEmitter::VRegister rt, ARMEmitter::ExtendedMemOperand MemSrc) {
    if (MemSrc.MetaType.Header.MemType == ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED &&
        MemSrc.MetaType.ExtendedType.rm.Idx() != ARMEmitter::Reg::r31.Idx()) {
      LOGMAN_THROW_A_FMT(MemSrc.MetaType.ExtendedType.Shift == false, "Can't shift byte");
      ldrb(rt, MemSrc.rn, MemSrc.MetaType.ExtendedType.rm, MemSrc.MetaType.ExtendedType.Option);
    } else if (MemSrc.MetaType.Header.MemType == ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED) {
      ldrb(rt, MemSrc.rn);
    } else {
      if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::OFFSET) {
        if (MemSrc.MetaType.ImmType.Imm < 0) {
          ldurb(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        } else {
          ldrb(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        }
      } else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::POST) {
        ldrb<ARMEmitter::IndexType::POST>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      } else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::PRE) {
        ldrb<ARMEmitter::IndexType::PRE>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      } else {
        LOGMAN_MSG_A_FMT("Unexpected loadstore index type");
        FEX_UNREACHABLE;
      }
    }
  }
  void strh(ARMEmitter::VRegister rt, ARMEmitter::ExtendedMemOperand MemSrc) {
    if (MemSrc.MetaType.Header.MemType == ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED &&
        MemSrc.MetaType.ExtendedType.rm.Idx() != ARMEmitter::Reg::r31.Idx()) {
      strh(rt, MemSrc.rn, MemSrc.MetaType.ExtendedType.rm, MemSrc.MetaType.ExtendedType.Option, MemSrc.MetaType.ExtendedType.Shift);
    } else if (MemSrc.MetaType.Header.MemType == ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED) {
      strh(rt, MemSrc.rn);
    } else {
      if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::OFFSET) {
        if ((MemSrc.MetaType.ImmType.Imm & 0b1) || MemSrc.MetaType.ImmType.Imm < 0) {
          sturh(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        } else {
          strh(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        }
      } else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::POST) {
        strh<ARMEmitter::IndexType::POST>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      } else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::PRE) {
        strh<ARMEmitter::IndexType::PRE>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      } else {
        LOGMAN_MSG_A_FMT("Unexpected loadstore index type");
        FEX_UNREACHABLE;
      }
    }
  }
  void ldrh(ARMEmitter::VRegister rt, ARMEmitter::ExtendedMemOperand MemSrc) {
    if (MemSrc.MetaType.Header.MemType == ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED &&
        MemSrc.MetaType.ExtendedType.rm.Idx() != ARMEmitter::Reg::r31.Idx()) {
      ldrh(rt, MemSrc.rn, MemSrc.MetaType.ExtendedType.rm, MemSrc.MetaType.ExtendedType.Option, MemSrc.MetaType.ExtendedType.Shift);
    } else if (MemSrc.MetaType.Header.MemType == ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED) {
      ldrh(rt, MemSrc.rn);
    } else {
      if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::OFFSET) {
        if ((MemSrc.MetaType.ImmType.Imm & 0b1) || MemSrc.MetaType.ImmType.Imm < 0) {
          ldurh(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        } else {
          ldrh(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        }
      } else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::POST) {
        ldrh<ARMEmitter::IndexType::POST>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      } else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::PRE) {
        ldrh<ARMEmitter::IndexType::PRE>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      } else {
        LOGMAN_MSG_A_FMT("Unexpected loadstore index type");
        FEX_UNREACHABLE;
      }
    }
  }
  void str(ARMEmitter::SRegister rt, ARMEmitter::ExtendedMemOperand MemSrc) {
    if (MemSrc.MetaType.Header.MemType == ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED &&
        MemSrc.MetaType.ExtendedType.rm.Idx() != ARMEmitter::Reg::r31.Idx()) {
      str(rt, MemSrc.rn, MemSrc.MetaType.ExtendedType.rm, MemSrc.MetaType.ExtendedType.Option, MemSrc.MetaType.ExtendedType.Shift);
    } else if (MemSrc.MetaType.Header.MemType == ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED) {
      str(rt, MemSrc.rn);
    } else {
      if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::OFFSET) {
        if ((MemSrc.MetaType.ImmType.Imm & 0b11) || MemSrc.MetaType.ImmType.Imm < 0) {
          stur(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        } else {
          str(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        }
      } else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::POST) {
        str<ARMEmitter::IndexType::POST>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      } else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::PRE) {
        str<ARMEmitter::IndexType::PRE>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      } else {
        LOGMAN_MSG_A_FMT("Unexpected loadstore index type");
        FEX_UNREACHABLE;
      }
    }
  }
  void ldr(ARMEmitter::SRegister rt, ARMEmitter::ExtendedMemOperand MemSrc) {
    if (MemSrc.MetaType.Header.MemType == ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED &&
        MemSrc.MetaType.ExtendedType.rm.Idx() != ARMEmitter::Reg::r31.Idx()) {
      ldr(rt, MemSrc.rn, MemSrc.MetaType.ExtendedType.rm, MemSrc.MetaType.ExtendedType.Option, MemSrc.MetaType.ExtendedType.Shift);
    } else if (MemSrc.MetaType.Header.MemType == ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED) {
      ldr(rt, MemSrc.rn);
    } else {
      if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::OFFSET) {
        if ((MemSrc.MetaType.ImmType.Imm & 0b11) || MemSrc.MetaType.ImmType.Imm < 0) {
          ldur(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        } else {
          ldr(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        }
      } else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::POST) {
        ldr<ARMEmitter::IndexType::POST>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      } else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::PRE) {
        ldr<ARMEmitter::IndexType::PRE>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      } else {
        LOGMAN_MSG_A_FMT("Unexpected loadstore index type");
        FEX_UNREACHABLE;
      }
    }
  }
  void str(ARMEmitter::DRegister rt, ARMEmitter::ExtendedMemOperand MemSrc) {
    if (MemSrc.MetaType.Header.MemType == ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED &&
        MemSrc.MetaType.ExtendedType.rm.Idx() != ARMEmitter::Reg::r31.Idx()) {
      str(rt, MemSrc.rn, MemSrc.MetaType.ExtendedType.rm, MemSrc.MetaType.ExtendedType.Option, MemSrc.MetaType.ExtendedType.Shift);
    } else if (MemSrc.MetaType.Header.MemType == ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED) {
      str(rt, MemSrc.rn);
    } else {
      if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::OFFSET) {
        if ((MemSrc.MetaType.ImmType.Imm & 0b111) || MemSrc.MetaType.ImmType.Imm < 0) {
          stur(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        } else {
          str(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        }
      } else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::POST) {
        str<ARMEmitter::IndexType::POST>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      } else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::PRE) {
        str<ARMEmitter::IndexType::PRE>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      } else {
        LOGMAN_MSG_A_FMT("Unexpected loadstore index type");
        FEX_UNREACHABLE;
      }
    }
  }
  void ldr(ARMEmitter::DRegister rt, ARMEmitter::ExtendedMemOperand MemSrc) {
    if (MemSrc.MetaType.Header.MemType == ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED &&
        MemSrc.MetaType.ExtendedType.rm.Idx() != ARMEmitter::Reg::r31.Idx()) {
      ldr(rt, MemSrc.rn, MemSrc.MetaType.ExtendedType.rm, MemSrc.MetaType.ExtendedType.Option, MemSrc.MetaType.ExtendedType.Shift);
    } else if (MemSrc.MetaType.Header.MemType == ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED) {
      ldr(rt, MemSrc.rn);
    } else {
      if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::OFFSET) {
        if ((MemSrc.MetaType.ImmType.Imm & 0b111) || MemSrc.MetaType.ImmType.Imm < 0) {
          ldur(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        } else {
          ldr(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        }
      } else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::POST) {
        ldr<ARMEmitter::IndexType::POST>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      } else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::PRE) {
        ldr<ARMEmitter::IndexType::PRE>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      } else {
        LOGMAN_MSG_A_FMT("Unexpected loadstore index type");
        FEX_UNREACHABLE;
      }
    }
  }
  void str(ARMEmitter::QRegister rt, ARMEmitter::ExtendedMemOperand MemSrc) {
    if (MemSrc.MetaType.Header.MemType == ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED &&
        MemSrc.MetaType.ExtendedType.rm.Idx() != ARMEmitter::Reg::r31.Idx()) {
      str(rt, MemSrc.rn, MemSrc.MetaType.ExtendedType.rm, MemSrc.MetaType.ExtendedType.Option, MemSrc.MetaType.ExtendedType.Shift);
    } else if (MemSrc.MetaType.Header.MemType == ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED) {
      str(rt, MemSrc.rn);
    } else {
      if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::OFFSET) {
        if ((MemSrc.MetaType.ImmType.Imm & 0b1111) || MemSrc.MetaType.ImmType.Imm < 0) {
          stur(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        } else {
          str(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        }
      } else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::POST) {
        str<ARMEmitter::IndexType::POST>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      } else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::PRE) {
        str<ARMEmitter::IndexType::PRE>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      } else {
        LOGMAN_MSG_A_FMT("Unexpected loadstore index type");
        FEX_UNREACHABLE;
      }
    }
  }
  void ldr(ARMEmitter::QRegister rt, ARMEmitter::ExtendedMemOperand MemSrc) {
    if (MemSrc.MetaType.Header.MemType == ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED &&
        MemSrc.MetaType.ExtendedType.rm.Idx() != ARMEmitter::Reg::r31.Idx()) {
      ldr(rt, MemSrc.rn, MemSrc.MetaType.ExtendedType.rm, MemSrc.MetaType.ExtendedType.Option, MemSrc.MetaType.ExtendedType.Shift);
    } else if (MemSrc.MetaType.Header.MemType == ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED) {
      ldr(rt, MemSrc.rn);
    } else {
      if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::OFFSET) {
        if ((MemSrc.MetaType.ImmType.Imm & 0b1111) || MemSrc.MetaType.ImmType.Imm < 0) {
          ldur(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        } else {
          ldr(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        }
      } else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::POST) {
        ldr<ARMEmitter::IndexType::POST>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      } else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::PRE) {
        ldr<ARMEmitter::IndexType::PRE>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      } else {
        LOGMAN_MSG_A_FMT("Unexpected loadstore index type");
        FEX_UNREACHABLE;
      }
    }
  }

  // Loadstore PAC
  void ldraa(XRegister rt, XRegister rn, IndexType type, int32_t offset = 0) {
    LoadStorePAC(0b11, 0, 0, offset, type, rn, rt);
  }
  void ldrab(XRegister rt, XRegister rn, IndexType type, int32_t offset = 0) {
    LoadStorePAC(0b11, 0, 1, offset, type, rn, rt);
  }

  // Loadstore unsigned immediate
  // Maximum values of unsigned immediate offsets for particular data sizes.
  static constexpr uint32_t LSByteMaxUnsignedOffset = 4095;
  static constexpr uint32_t LSHalfMaxUnsignedOffset = 8190;
  static constexpr uint32_t LSWordMaxUnsignedOffset = 16380;
  static constexpr uint32_t LSDWordMaxUnsignedOffset = 32760;
  static constexpr uint32_t LSQWordMaxUnsignedOffset = 65520;

  void strb(Register rt, Register rn, uint32_t Imm = 0) {
    LoadStoreUnsigned(0b00, 0, 0b00, rt, rn, Imm);
  }
  void ldrb(Register rt, Register rn, uint32_t Imm = 0) {
    LoadStoreUnsigned(0b00, 0, 0b01, rt, rn, Imm);
  }
  void ldrsb(XRegister rt, Register rn, uint32_t Imm = 0) {
    LoadStoreUnsigned(0b00, 0, 0b10, rt, rn, Imm);
  }
  void ldrsb(WRegister rt, Register rn, uint32_t Imm = 0) {
    LoadStoreUnsigned(0b00, 0, 0b11, rt, rn, Imm);
  }
  void strb(VRegister rt, Register rn, uint32_t Imm = 0) {
    LoadStoreUnsigned(0b00, 1, 0b00, rt, rn, Imm);
  }
  void ldrb(VRegister rt, Register rn, uint32_t Imm = 0) {
    LoadStoreUnsigned(0b00, 1, 0b01, rt, rn, Imm);
  }
  void strh(Register rt, Register rn, uint32_t Imm = 0) {
    LoadStoreUnsigned(0b01, 0, 0b00, rt, rn, Imm);
  }
  void ldrh(Register rt, Register rn, uint32_t Imm = 0) {
    LoadStoreUnsigned(0b01, 0, 0b01, rt, rn, Imm);
  }
  void ldrsh(XRegister rt, Register rn, uint32_t Imm = 0) {
    LoadStoreUnsigned(0b01, 0, 0b10, rt, rn, Imm);
  }
  void ldrsh(WRegister rt, Register rn, uint32_t Imm = 0) {
    LoadStoreUnsigned(0b01, 0, 0b11, rt, rn, Imm);
  }
  void strh(VRegister rt, Register rn, uint32_t Imm = 0) {
    LoadStoreUnsigned(0b01, 1, 0b00, rt, rn, Imm);
  }
  void ldrh(VRegister rt, Register rn, uint32_t Imm = 0) {
    LoadStoreUnsigned(0b01, 1, 0b01, rt, rn, Imm);
  }
  void str(WRegister rt, Register rn, uint32_t Imm = 0) {
    LoadStoreUnsigned(0b10, 0, 0b00, rt, rn, Imm);
  }
  void ldr(WRegister rt, Register rn, uint32_t Imm = 0) {
    LoadStoreUnsigned(0b10, 0, 0b01, rt, rn, Imm);
  }
  void ldrsw(XRegister rt, Register rn, uint32_t Imm = 0) {
    LoadStoreUnsigned(0b10, 0, 0b10, rt, rn, Imm);
  }
  void str(SRegister rt, Register rn, uint32_t Imm = 0) {
    LoadStoreUnsigned(0b10, 1, 0b00, rt, rn, Imm);
  }
  void ldr(SRegister rt, Register rn, uint32_t Imm = 0) {
    LoadStoreUnsigned(0b10, 1, 0b01, rt, rn, Imm);
  }
  void str(XRegister rt, Register rn, uint32_t Imm = 0) {
    LoadStoreUnsigned(0b11, 0, 0b00, rt, rn, Imm);
  }
  void ldr(XRegister rt, Register rn, uint32_t Imm = 0) {
    LoadStoreUnsigned(0b11, 0, 0b01, rt, rn, Imm);
  }

  void ldr(SubRegSize size, Register rt, Register rn, uint32_t Imm = 0) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i128Bit, "Can't use 128-bit size");
    LoadStoreUnsigned(FEXCore::ToUnderlying(size), 0, 0b01, rt, rn, Imm);
  }
  void str(SubRegSize size, Register rt, Register rn, uint32_t Imm = 0) {
    LOGMAN_THROW_A_FMT(size != SubRegSize::i128Bit, "Can't use 128-bit size");
    LoadStoreUnsigned(FEXCore::ToUnderlying(size), 0, 0b00, rt, rn, Imm);
  }

  void prfm(Prefetch prfop, Register rn, uint32_t Imm = 0) {
    LoadStoreUnsigned(0b11, 0, 0b10, prfop, rn, Imm);
  }
  void str(DRegister rt, Register rn, uint32_t Imm = 0) {
    LoadStoreUnsigned(0b11, 1, 0b00, rt, rn, Imm);
  }
  void ldr(DRegister rt, Register rn, uint32_t Imm = 0) {
    LoadStoreUnsigned(0b11, 1, 0b01, rt, rn, Imm);
  }
  void str(QRegister rt, Register rn, uint32_t Imm = 0) {
    LoadStoreUnsigned(0b00, 1, 0b10, rt, rn, Imm);
  }
  void ldr(QRegister rt, Register rn, uint32_t Imm = 0) {
    LoadStoreUnsigned(0b00, 1, 0b11, rt, rn, Imm);
  }

private:
  void AtomicOp(uint32_t Op, ARMEmitter::Size s, uint32_t L, uint32_t o0, ARMEmitter::Register rs, ARMEmitter::Register rt,
                ARMEmitter::Register rt2, ARMEmitter::Register rn) {
    const uint32_t sz = s == ARMEmitter::Size::i64Bit ? (1U << 30) : 0;
    uint32_t Instr = Op;

    Instr |= sz;
    Instr |= L << 22;
    Instr |= Encode_rs(rs);
    Instr |= o0 << 15;
    Instr |= Encode_rt2(rt2);
    Instr |= Encode_rn(rn);
    Instr |= Encode_rt(rt);

    dc32(Instr);
  }

  template<typename T>
  void SubAtomicOp(uint32_t Op, ARMEmitter::SubRegSize s, uint32_t L, uint32_t o0, T rs, T rt, T rt2, ARMEmitter::Register rn) {
    const uint32_t sz = FEXCore::ToUnderlying(s) << 30;
    uint32_t Instr = Op;

    Instr |= sz;
    Instr |= L << 22;
    Instr |= Encode_rs(rs);
    Instr |= o0 << 15;
    Instr |= Encode_rt2(rt2);
    Instr |= Encode_rn(rn);
    Instr |= Encode_rt(rt);

    dc32(Instr);
  }

  template<typename T>
  void SubAtomicImm(uint32_t Op, ARMEmitter::SubRegSize s, uint32_t opc, T rt, ARMEmitter::Register rn, uint32_t Imm) {
    const uint32_t sz = FEXCore::ToUnderlying(s) << 30;
    uint32_t Instr = Op;

    Instr |= sz;
    Instr |= opc << 22;
    Instr |= Imm << 12;
    Instr |= Encode_rn(rn);
    Instr |= Encode_rt(rt);

    dc32(Instr);
  }
  // Load register literal
  template<typename T>
  void LoadStoreLiteral(uint32_t Op, T rt, uint32_t Imm) {
    uint32_t Instr = Op;

    Instr |= Imm << 5;
    Instr |= Encode_rt(rt);
    dc32(Instr);
  }

  void MemoryCopyAndMemorySet(uint32_t sz, uint32_t o0, uint32_t op1, uint32_t op2, Register rs, Register rn, Register rd) {
    uint32_t Instr = 0b0001'1001'0000'0000'0000'0100'0000'0000;

    Instr |= sz << 30;
    Instr |= o0 << 26;
    Instr |= op1 << 22;
    Instr |= rs.Idx() << 16;
    Instr |= op2 << 12;
    Instr |= rn.Idx() << 5;
    Instr |= rd.Idx();

    dc32(Instr);
  }

  // Loadstore no-allocate pair
  template<typename T>
  void LoadStoreNoAllocate(uint32_t Op, T rt, T rt2, ARMEmitter::Register rn, uint32_t Imm) {
    uint32_t Instr = Op;

    Instr |= Imm << 15;
    Instr |= Encode_rt2(rt2);
    Instr |= Encode_rn(rn);
    Instr |= Encode_rt(rt);
    dc32(Instr);
  }
  // Loadstore register pair post-indexed
  template<typename T>
  void LoadStorePair(uint32_t Op, T rt, T rt2, ARMEmitter::Register rn, uint32_t Imm) {
    uint32_t Instr = Op;
    Instr |= Imm << 15;
    Instr |= Encode_rt2(rt2);
    Instr |= Encode_rn(rn);
    Instr |= Encode_rt(rt);
    dc32(Instr);
  }

  // Loadstore register unscaled immediate
  // Loadstore register immediate post-indexed
  // Loadstore register unprivileged
  // Loadstore register immediate pre-indexed
  template<typename T>
  void LoadStoreImm(uint32_t Op, uint32_t o2, T rt, ARMEmitter::Register rn, uint32_t Imm) {
    uint32_t Instr = Op;

    Instr |= Imm << 12;
    Instr |= o2 << 10;
    Instr |= Encode_rn(rn);
    Instr |= Encode_rt(rt);
    dc32(Instr);
  }

  // Atomic memory operations
  void LoadStoreAtomicLSE(SubRegSize s, uint32_t A, uint32_t R, uint32_t o3, uint32_t opc, Register rs, Register rt, Register rn) {
    uint32_t Instr = 0b0011'1000'0010'0000'0000'0000'0000'0000;
    Instr |= FEXCore::ToUnderlying(s) << 30;
    Instr |= A << 23;
    Instr |= R << 22;
    Instr |= Encode_rs(rs);
    Instr |= o3 << 15;
    Instr |= opc << 12;
    Instr |= Encode_rn(rn);
    Instr |= Encode_rt(rt);
    dc32(Instr);
  }

  // Loadstore register-register offset
  template<typename T>
  void LoadStoreRegisterOffset(uint32_t Op, uint32_t opc, T rt, ARMEmitter::Register rn, ARMEmitter::Register rm,
                               ARMEmitter::ExtendedType Option, uint32_t Shift) {
    uint32_t Instr = Op;

    Instr |= opc << 22;
    Instr |= Encode_rt(rt);
    Instr |= FEXCore::ToUnderlying(Option) << 13;
    Instr |= Shift << 12;
    Instr |= Encode_rn(rn);
    Instr |= Encode_rm(rm);
    dc32(Instr);
  }

  void LoadStorePAC(uint32_t size, uint32_t VR, uint32_t M, int32_t imm, IndexType type, Register rn, Register rt) {
    LOGMAN_THROW_A_FMT((imm % 8) == 0, "Immediate ({}) must be divisible by 8", imm);
    LOGMAN_THROW_A_FMT(imm >= -4096 && imm <= 4088, "Immediate ({}) must be within [-4096, 4088]", imm);
    LOGMAN_THROW_A_FMT(type == IndexType::OFFSET || type == IndexType::PRE, "PAC may only use offset or pre-indexed values");

    // The immediate is scaled down in order to fit within the available 10 immediate bits.
    const auto scaled_imm = static_cast<uint32_t>(imm / 8);
    const auto imm9 = scaled_imm & 0b1'1111'1111;
    const auto S = (scaled_imm >> 9) & 1;

    const auto W = type == IndexType::OFFSET ? 0U : 1U;

    uint32_t Instr = 0b0011'1000'0010'0000'0000'0100'0000'0000;
    Instr |= size << 30;
    Instr |= VR << 26;
    Instr |= M << 23;
    Instr |= S << 22;
    Instr |= imm9 << 12;
    Instr |= W << 11;
    Instr |= rn.Idx() << 5;
    Instr |= rt.Idx();
    dc32(Instr);
  }

  // Loadstore unsigned immediate
  template<typename T>
  void LoadStoreUnsigned(uint32_t size, uint32_t V, uint32_t opc, T rt, Register rn, uint32_t Imm) {
    uint32_t SizeShift = size;
    if constexpr (std::is_same_v<T, QRegister>) {
      // 128-bit variant is specified via size=0b00, V=1, opc=0b1x
      // so we need to special case this one based on whether or not
      // rt indicates a 128-bit vector. Nice thing is this can be
      // checked at compile-time.
      SizeShift = 4;
    }

    [[maybe_unused]] const uint32_t MaxImm = LSByteMaxUnsignedOffset << SizeShift;
    [[maybe_unused]] const uint32_t ElementSize = 1U << SizeShift;

    LOGMAN_THROW_A_FMT(Imm <= MaxImm, "{}: Offset not valid: Imm: 0x{:x} Max: 0x{:x}", __func__, Imm, MaxImm);
    LOGMAN_THROW_A_FMT((Imm % ElementSize) == 0, "{}: Offset must be a multiple of {}. Offset: 0x{:x}", __func__, ElementSize, Imm);

    const uint32_t ShiftedImm = Imm >> SizeShift;

    uint32_t Instr = 0b0011'1001'0000'0000'0000'0000'0000'0000;
    Instr |= size << 30;
    Instr |= V << 26;
    Instr |= opc << 22;
    Instr |= ShiftedImm << 10;
    Instr |= Encode_rn(rn);
    Instr |= Encode_rt(rt);
    dc32(Instr);
  }

  template<IndexType Index>
  void ldp_w(ARMEmitter::VRegister rt, ARMEmitter::VRegister rt2, ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 252 && ((Imm & 0b11) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = (0b0010'1100'01 << 22) | (Index == IndexType::POST   ? (0b01 << 23) :
                                                      Index == IndexType::PRE    ? (0b11 << 23) :
                                                      Index == IndexType::OFFSET ? (0b10 << 23) :
                                                                                   -1);

    LoadStorePair(Op, rt, rt2, rn, (Imm >> 2) & 0b111'1111);
  }
  template<IndexType Index>
  void ldp_x(ARMEmitter::VRegister rt, ARMEmitter::VRegister rt2, ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -512 && Imm <= 504 && ((Imm & 0b111) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = (0b0110'1100'01 << 22) | (Index == IndexType::POST   ? (0b01 << 23) :
                                                      Index == IndexType::PRE    ? (0b11 << 23) :
                                                      Index == IndexType::OFFSET ? (0b10 << 23) :
                                                                                   -1);

    LoadStorePair(Op, rt, rt2, rn, (Imm >> 3) & 0b111'1111);
  }
  template<IndexType Index>
  void stp_w(ARMEmitter::VRegister rt, ARMEmitter::VRegister rt2, ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 252 && ((Imm & 0b11) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = (0b0010'1100'00 << 22) | (Index == IndexType::POST   ? (0b01 << 23) :
                                                      Index == IndexType::PRE    ? (0b11 << 23) :
                                                      Index == IndexType::OFFSET ? (0b10 << 23) :
                                                                                   -1);

    LoadStorePair(Op, rt, rt2, rn, (Imm >> 2) & 0b111'1111);
  }
  template<IndexType Index>
  void stp_x(ARMEmitter::VRegister rt, ARMEmitter::VRegister rt2, ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -512 && Imm <= 504 && ((Imm & 0b111) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = (0b0110'1100'00 << 22) | (Index == IndexType::POST   ? (0b01 << 23) :
                                                      Index == IndexType::PRE    ? (0b11 << 23) :
                                                      Index == IndexType::OFFSET ? (0b10 << 23) :
                                                                                   -1);

    LoadStorePair(Op, rt, rt2, rn, (Imm >> 3) & 0b111'1111);
  }
  template<IndexType Index>
  void ldp_q(ARMEmitter::VRegister rt, ARMEmitter::VRegister rt2, ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -1024 && Imm <= 1008 && ((Imm & 0b1111) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = (0b1010'1100'01 << 22) | (Index == IndexType::POST   ? (0b01 << 23) :
                                                      Index == IndexType::PRE    ? (0b11 << 23) :
                                                      Index == IndexType::OFFSET ? (0b10 << 23) :
                                                                                   -1);

    LoadStorePair(Op, rt, rt2, rn, (Imm >> 4) & 0b111'1111);
  }
  template<IndexType Index>
  void stp_q(ARMEmitter::VRegister rt, ARMEmitter::VRegister rt2, ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -1024 && Imm <= 1008 && ((Imm & 0b1111) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = (0b1010'1100'00 << 22) | (Index == IndexType::POST   ? (0b01 << 23) :
                                                      Index == IndexType::PRE    ? (0b11 << 23) :
                                                      Index == IndexType::OFFSET ? (0b10 << 23) :
                                                                                   -1);

    LoadStorePair(Op, rt, rt2, rn, (Imm >> 4) & 0b111'1111);
  }

  template<IndexType Index>
  void stXrb(ARMEmitter::Register rt, ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");

    constexpr uint32_t Op = 0b0011'1000'00 << 22;
    constexpr uint32_t o2 = Index == IndexType::POST         ? 0b01 :
                            Index == IndexType::PRE          ? 0b11 :
                            Index == IndexType::OFFSET       ? 0b00 :
                            Index == IndexType::UNPRIVILEGED ? 0b10 :
                                                               -1;

    LoadStoreImm(Op, o2, rt, rn, Imm & 0b1'1111'1111);
  }
  template<IndexType Index>
  void ldXrb(ARMEmitter::Register rt, ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");

    constexpr uint32_t Op = 0b0011'1000'01 << 22;
    constexpr uint32_t o2 = Index == IndexType::POST         ? 0b01 :
                            Index == IndexType::PRE          ? 0b11 :
                            Index == IndexType::OFFSET       ? 0b00 :
                            Index == IndexType::UNPRIVILEGED ? 0b10 :
                                                               -1;

    LoadStoreImm(Op, o2, rt, rn, Imm & 0b1'1111'1111);
  }
  template<IndexType Index>
  void stXrb(ARMEmitter::VRegister rt, ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");

    constexpr uint32_t Op = 0b0011'1100'00 << 22;
    constexpr uint32_t o2 = Index == IndexType::POST         ? 0b01 :
                            Index == IndexType::PRE          ? 0b11 :
                            Index == IndexType::OFFSET       ? 0b00 :
                            Index == IndexType::UNPRIVILEGED ? 0b10 :
                                                               -1;

    LoadStoreImm(Op, o2, rt, rn, Imm & 0b1'1111'1111);
  }
  template<IndexType Index>
  void ldXrb(ARMEmitter::VRegister rt, ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");

    constexpr uint32_t Op = 0b0011'1100'01 << 22;
    constexpr uint32_t o2 = Index == IndexType::POST         ? 0b01 :
                            Index == IndexType::PRE          ? 0b11 :
                            Index == IndexType::OFFSET       ? 0b00 :
                            Index == IndexType::UNPRIVILEGED ? 0b10 :
                                                               -1;

    LoadStoreImm(Op, o2, rt, rn, Imm & 0b1'1111'1111);
  }
  template<IndexType Index>
  void ldXrsb(ARMEmitter::XRegister rt, ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");

    constexpr uint32_t Op = 0b0011'1000'10 << 22;
    constexpr uint32_t o2 = Index == IndexType::POST         ? 0b01 :
                            Index == IndexType::PRE          ? 0b11 :
                            Index == IndexType::OFFSET       ? 0b00 :
                            Index == IndexType::UNPRIVILEGED ? 0b10 :
                                                               -1;

    LoadStoreImm(Op, o2, rt, rn, Imm & 0b1'1111'1111);
  }
  template<IndexType Index>
  void ldXrsb(ARMEmitter::WRegister rt, ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");

    constexpr uint32_t Op = 0b0011'1000'11 << 22;
    constexpr uint32_t o2 = Index == IndexType::POST         ? 0b01 :
                            Index == IndexType::PRE          ? 0b11 :
                            Index == IndexType::OFFSET       ? 0b00 :
                            Index == IndexType::UNPRIVILEGED ? 0b10 :
                                                               -1;

    LoadStoreImm(Op, o2, rt, rn, Imm & 0b1'1111'1111);
  }
  template<IndexType Index>
  void stXrh(ARMEmitter::Register rt, ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");

    constexpr uint32_t Op = 0b0111'1000'00 << 22;
    constexpr uint32_t o2 = Index == IndexType::POST         ? 0b01 :
                            Index == IndexType::PRE          ? 0b11 :
                            Index == IndexType::OFFSET       ? 0b00 :
                            Index == IndexType::UNPRIVILEGED ? 0b10 :
                                                               -1;

    LoadStoreImm(Op, o2, rt, rn, Imm & 0b1'1111'1111);
  }
  template<IndexType Index>
  void ldXrh(ARMEmitter::Register rt, ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");

    constexpr uint32_t Op = 0b0111'1000'01 << 22;
    constexpr uint32_t o2 = Index == IndexType::POST         ? 0b01 :
                            Index == IndexType::PRE          ? 0b11 :
                            Index == IndexType::OFFSET       ? 0b00 :
                            Index == IndexType::UNPRIVILEGED ? 0b10 :
                                                               -1;

    LoadStoreImm(Op, o2, rt, rn, Imm & 0b1'1111'1111);
  }
  template<IndexType Index>
  void stXrh(ARMEmitter::VRegister rt, ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");

    constexpr uint32_t Op = 0b0111'1100'00 << 22;
    constexpr uint32_t o2 = Index == IndexType::POST         ? 0b01 :
                            Index == IndexType::PRE          ? 0b11 :
                            Index == IndexType::OFFSET       ? 0b00 :
                            Index == IndexType::UNPRIVILEGED ? 0b10 :
                                                               -1;

    LoadStoreImm(Op, o2, rt, rn, Imm & 0b1'1111'1111);
  }
  template<IndexType Index>
  void ldXrh(ARMEmitter::VRegister rt, ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");

    constexpr uint32_t Op = 0b0111'1100'01 << 22;
    constexpr uint32_t o2 = Index == IndexType::POST         ? 0b01 :
                            Index == IndexType::PRE          ? 0b11 :
                            Index == IndexType::OFFSET       ? 0b00 :
                            Index == IndexType::UNPRIVILEGED ? 0b10 :
                                                               -1;

    LoadStoreImm(Op, o2, rt, rn, Imm & 0b1'1111'1111);
  }
  template<IndexType Index>
  void ldXrsh(ARMEmitter::XRegister rt, ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");

    constexpr uint32_t Op = 0b0111'1000'10 << 22;
    constexpr uint32_t o2 = Index == IndexType::POST         ? 0b01 :
                            Index == IndexType::PRE          ? 0b11 :
                            Index == IndexType::OFFSET       ? 0b00 :
                            Index == IndexType::UNPRIVILEGED ? 0b10 :
                                                               -1;

    LoadStoreImm(Op, o2, rt, rn, Imm & 0b1'1111'1111);
  }
  template<IndexType Index>
  void ldXrsh(ARMEmitter::WRegister rt, ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");

    constexpr uint32_t Op = 0b0111'1000'11 << 22;
    constexpr uint32_t o2 = Index == IndexType::POST         ? 0b01 :
                            Index == IndexType::PRE          ? 0b11 :
                            Index == IndexType::OFFSET       ? 0b00 :
                            Index == IndexType::UNPRIVILEGED ? 0b10 :
                                                               -1;

    LoadStoreImm(Op, o2, rt, rn, Imm & 0b1'1111'1111);
  }
  template<IndexType Index>
  void stXr(ARMEmitter::WRegister rt, ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");

    constexpr uint32_t Op = 0b1011'1000'00 << 22;
    constexpr uint32_t o2 = Index == IndexType::POST         ? 0b01 :
                            Index == IndexType::PRE          ? 0b11 :
                            Index == IndexType::OFFSET       ? 0b00 :
                            Index == IndexType::UNPRIVILEGED ? 0b10 :
                                                               -1;

    LoadStoreImm(Op, o2, rt, rn, Imm & 0b1'1111'1111);
  }
  template<IndexType Index>
  void ldXr(ARMEmitter::WRegister rt, ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");

    constexpr uint32_t Op = 0b1011'1000'01 << 22;
    constexpr uint32_t o2 = Index == IndexType::POST         ? 0b01 :
                            Index == IndexType::PRE          ? 0b11 :
                            Index == IndexType::OFFSET       ? 0b00 :
                            Index == IndexType::UNPRIVILEGED ? 0b10 :
                                                               -1;

    LoadStoreImm(Op, o2, rt, rn, Imm & 0b1'1111'1111);
  }
  template<IndexType Index>
  void stXr(ARMEmitter::SRegister rt, ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");

    constexpr uint32_t Op = 0b1011'1100'00 << 22;
    constexpr uint32_t o2 = Index == IndexType::POST         ? 0b01 :
                            Index == IndexType::PRE          ? 0b11 :
                            Index == IndexType::OFFSET       ? 0b00 :
                            Index == IndexType::UNPRIVILEGED ? 0b10 :
                                                               -1;

    LoadStoreImm(Op, o2, rt, rn, Imm & 0b1'1111'1111);
  }
  template<IndexType Index>
  void ldXr(ARMEmitter::SRegister rt, ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");

    constexpr uint32_t Op = 0b1011'1100'01 << 22;
    constexpr uint32_t o2 = Index == IndexType::POST         ? 0b01 :
                            Index == IndexType::PRE          ? 0b11 :
                            Index == IndexType::OFFSET       ? 0b00 :
                            Index == IndexType::UNPRIVILEGED ? 0b10 :
                                                               -1;

    LoadStoreImm(Op, o2, rt, rn, Imm & 0b1'1111'1111);
  }
  template<IndexType Index>
  void ldXrsw(ARMEmitter::XRegister rt, ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");

    constexpr uint32_t Op = 0b1011'1000'10 << 22;
    constexpr uint32_t o2 = Index == IndexType::POST         ? 0b01 :
                            Index == IndexType::PRE          ? 0b11 :
                            Index == IndexType::OFFSET       ? 0b00 :
                            Index == IndexType::UNPRIVILEGED ? 0b10 :
                                                               -1;

    LoadStoreImm(Op, o2, rt, rn, Imm & 0b1'1111'1111);
  }
  template<IndexType Index>
  void stXr(ARMEmitter::XRegister rt, ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");

    constexpr uint32_t Op = 0b1111'1000'00 << 22;
    constexpr uint32_t o2 = Index == IndexType::POST         ? 0b01 :
                            Index == IndexType::PRE          ? 0b11 :
                            Index == IndexType::OFFSET       ? 0b00 :
                            Index == IndexType::UNPRIVILEGED ? 0b10 :
                                                               -1;

    LoadStoreImm(Op, o2, rt, rn, Imm & 0b1'1111'1111);
  }
  template<IndexType Index>
  void ldXr(ARMEmitter::XRegister rt, ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");

    constexpr uint32_t Op = 0b1111'1000'01 << 22;
    constexpr uint32_t o2 = Index == IndexType::POST         ? 0b01 :
                            Index == IndexType::PRE          ? 0b11 :
                            Index == IndexType::OFFSET       ? 0b00 :
                            Index == IndexType::UNPRIVILEGED ? 0b10 :
                                                               -1;

    LoadStoreImm(Op, o2, rt, rn, Imm & 0b1'1111'1111);
  }
  template<IndexType Index>
  void stXr(ARMEmitter::DRegister rt, ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");

    constexpr uint32_t Op = 0b1111'1100'00 << 22;
    constexpr uint32_t o2 = Index == IndexType::POST         ? 0b01 :
                            Index == IndexType::PRE          ? 0b11 :
                            Index == IndexType::OFFSET       ? 0b00 :
                            Index == IndexType::UNPRIVILEGED ? 0b10 :
                                                               -1;

    LoadStoreImm(Op, o2, rt, rn, Imm & 0b1'1111'1111);
  }
  template<IndexType Index>
  void ldXr(ARMEmitter::DRegister rt, ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");

    constexpr uint32_t Op = 0b1111'1100'01 << 22;
    constexpr uint32_t o2 = Index == IndexType::POST         ? 0b01 :
                            Index == IndexType::PRE          ? 0b11 :
                            Index == IndexType::OFFSET       ? 0b00 :
                            Index == IndexType::UNPRIVILEGED ? 0b10 :
                                                               -1;

    LoadStoreImm(Op, o2, rt, rn, Imm & 0b1'1111'1111);
  }
  template<IndexType Index>
  void stXr(ARMEmitter::QRegister rt, ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");

    constexpr uint32_t Op = 0b0011'1100'10 << 22;
    constexpr uint32_t o2 = Index == IndexType::POST         ? 0b01 :
                            Index == IndexType::PRE          ? 0b11 :
                            Index == IndexType::OFFSET       ? 0b00 :
                            Index == IndexType::UNPRIVILEGED ? 0b10 :
                                                               -1;

    LoadStoreImm(Op, o2, rt, rn, Imm & 0b1'1111'1111);
  }
  template<IndexType Index>
  void ldXr(ARMEmitter::QRegister rt, ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");

    constexpr uint32_t Op = 0b0011'1100'11 << 22;
    constexpr uint32_t o2 = Index == IndexType::POST         ? 0b01 :
                            Index == IndexType::PRE          ? 0b11 :
                            Index == IndexType::OFFSET       ? 0b00 :
                            Index == IndexType::UNPRIVILEGED ? 0b10 :
                                                               -1;

    LoadStoreImm(Op, o2, rt, rn, Imm & 0b1'1111'1111);
  }

#ifndef INCLUDED_BY_EMITTER
}; // struct LoadstoreEmitterOps
} // namespace ARMEmitter
#endif
