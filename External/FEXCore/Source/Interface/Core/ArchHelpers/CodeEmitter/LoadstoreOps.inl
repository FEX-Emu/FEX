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
public:
  // Compare and swap pair
  void casp(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rs2, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rt2, FEXCore::ARMEmitter::Register rn) {
    LOGMAN_THROW_A_FMT((rs.Idx() + 1) == rs2.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    constexpr uint32_t Op = 0b0000'1000'001 << 21;
    AtomicOp(Op, s, 0, 0, rs, rt, FEXCore::ARMEmitter::Reg::r31, rn);
  }
  void caspa(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rs2, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rt2, FEXCore::ARMEmitter::Register rn) {
    LOGMAN_THROW_A_FMT((rs.Idx() + 1) == rs2.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    constexpr uint32_t Op = 0b0000'1000'001 << 21;
    AtomicOp(Op, s, 1, 0, rs, rt, FEXCore::ARMEmitter::Reg::r31, rn);
  }
  void caspl(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rs2, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rt2, FEXCore::ARMEmitter::Register rn) {
    LOGMAN_THROW_A_FMT((rs.Idx() + 1) == rs2.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    constexpr uint32_t Op = 0b0000'1000'001 << 21;
    AtomicOp(Op, s, 0, 1, rs, rt, FEXCore::ARMEmitter::Reg::r31, rn);
  }
  void caspal(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rs2, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rt2, FEXCore::ARMEmitter::Register rn) {
    LOGMAN_THROW_A_FMT((rs.Idx() + 1) == rs2.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    constexpr uint32_t Op = 0b0000'1000'001 << 21;
    AtomicOp(Op, s, 1, 1, rs, rt, FEXCore::ARMEmitter::Reg::r31, rn);
  }

  // Advanced SIMD load/store multiple structures
  template<FEXCore::ARMEmitter::SubRegSize size, typename T>
  void ld1(T rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1100'000 << 21;
    constexpr uint32_t Opcode = 0b0111 << 12;
    ASIMDLoadStoreMultipleStructure<size, true>(Op, Opcode, rt, rn, FEXCore::ARMEmitter::Reg::r0);
  }
  template<FEXCore::ARMEmitter::SubRegSize size, typename T>
  void ld1(T rt, T rt2, FEXCore::ARMEmitter::Register rn) {
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    constexpr uint32_t Op = 0b0000'1100'000 << 21;
    constexpr uint32_t Opcode = 0b1010 << 12;
    ASIMDLoadStoreMultipleStructure<size, true>(Op, Opcode, rt, rn, FEXCore::ARMEmitter::Reg::r0);
  }
  template<FEXCore::ARMEmitter::SubRegSize size, typename T>
  void ld1(T rt, T rt2, T rt3, FEXCore::ARMEmitter::Register rn) {
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt2.Idx() + 1) == rt3.Idx(), "These must be sequential");
    constexpr uint32_t Op = 0b0000'1100'000 << 21;
    constexpr uint32_t Opcode = 0b0110 << 12;
    ASIMDLoadStoreMultipleStructure<size, true>(Op, Opcode, rt, rn, FEXCore::ARMEmitter::Reg::r0);
  }
  template<FEXCore::ARMEmitter::SubRegSize size, typename T>
  void ld1(T rt, T rt2, T rt3, T rt4, FEXCore::ARMEmitter::Register rn) {
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt2.Idx() + 1) == rt3.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt3.Idx() + 1) == rt4.Idx(), "These must be sequential");
    constexpr uint32_t Op = 0b0000'1100'000 << 21;
    constexpr uint32_t Opcode = 0b0010 << 12;
    ASIMDLoadStoreMultipleStructure<size, true>(Op, Opcode, rt, rn, FEXCore::ARMEmitter::Reg::r0);
  }
  template<FEXCore::ARMEmitter::SubRegSize size, typename T>
  void st1(T rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1100'000 << 21;
    constexpr uint32_t Opcode = 0b0111 << 12;
    ASIMDLoadStoreMultipleStructure<size, false>(Op, Opcode, rt, rn, FEXCore::ARMEmitter::Reg::r0);
  }
  template<FEXCore::ARMEmitter::SubRegSize size, typename T>
  void st1(T rt, T rt2, FEXCore::ARMEmitter::Register rn) {
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    constexpr uint32_t Op = 0b0000'1100'000 << 21;
    constexpr uint32_t Opcode = 0b1010 << 12;
    ASIMDLoadStoreMultipleStructure<size, false>(Op, Opcode, rt, rn, FEXCore::ARMEmitter::Reg::r0);
  }
  template<FEXCore::ARMEmitter::SubRegSize size, typename T>
  void st1(T rt, T rt2, T rt3, FEXCore::ARMEmitter::Register rn) {
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt2.Idx() + 1) == rt3.Idx(), "These must be sequential");
    constexpr uint32_t Op = 0b0000'1100'000 << 21;
    constexpr uint32_t Opcode = 0b0110 << 12;
    ASIMDLoadStoreMultipleStructure<size, false>(Op, Opcode, rt, rn, FEXCore::ARMEmitter::Reg::r0);
  }
  template<FEXCore::ARMEmitter::SubRegSize size, typename T>
  void st1(T rt, T rt2, T rt3, T rt4, FEXCore::ARMEmitter::Register rn) {
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt2.Idx() + 1) == rt3.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt3.Idx() + 1) == rt4.Idx(), "These must be sequential");
    constexpr uint32_t Op = 0b0000'1100'000 << 21;
    constexpr uint32_t Opcode = 0b0010 << 12;
    ASIMDLoadStoreMultipleStructure<size, false>(Op, Opcode, rt, rn, FEXCore::ARMEmitter::Reg::r0);
  }
  template<FEXCore::ARMEmitter::SubRegSize size, typename T>
  void ld2(T rt, T rt2, FEXCore::ARMEmitter::Register rn) {
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    constexpr uint32_t Op = 0b0000'1100'000 << 21;
    constexpr uint32_t Opcode = 0b1000 << 12;
    ASIMDLoadStoreMultipleStructure<size, true>(Op, Opcode, rt, rn, FEXCore::ARMEmitter::Reg::r0);
  }
  template<FEXCore::ARMEmitter::SubRegSize size, typename T>
  void st2(T rt, T rt2, FEXCore::ARMEmitter::Register rn) {
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    constexpr uint32_t Op = 0b0000'1100'000 << 21;
    constexpr uint32_t Opcode = 0b1000 << 12;
    ASIMDLoadStoreMultipleStructure<size, false>(Op, Opcode, rt, rn, FEXCore::ARMEmitter::Reg::r0);
  }
  template<FEXCore::ARMEmitter::SubRegSize size, typename T>
  void ld3(T rt, T rt2, T rt3, FEXCore::ARMEmitter::Register rn) {
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt2.Idx() + 1) == rt3.Idx(), "These must be sequential");
    constexpr uint32_t Op = 0b0000'1100'000 << 21;
    constexpr uint32_t Opcode = 0b0100 << 12;
    ASIMDLoadStoreMultipleStructure<size, true>(Op, Opcode, rt, rn, FEXCore::ARMEmitter::Reg::r0);
  }
  template<FEXCore::ARMEmitter::SubRegSize size, typename T>
  void st3(T rt, T rt2, T rt3, FEXCore::ARMEmitter::Register rn) {
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt2.Idx() + 1) == rt3.Idx(), "These must be sequential");
    constexpr uint32_t Op = 0b0000'1100'000 << 21;
    constexpr uint32_t Opcode = 0b0100 << 12;
    ASIMDLoadStoreMultipleStructure<size, false>(Op, Opcode, rt, rn, FEXCore::ARMEmitter::Reg::r0);
  }
  template<FEXCore::ARMEmitter::SubRegSize size, typename T>
  void ld4(T rt, T rt2, T rt3, T rt4, FEXCore::ARMEmitter::Register rn) {
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt2.Idx() + 1) == rt3.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt3.Idx() + 1) == rt4.Idx(), "These must be sequential");
    constexpr uint32_t Op = 0b0000'1100'000 << 21;
    constexpr uint32_t Opcode = 0b0000 << 12;
    ASIMDLoadStoreMultipleStructure<size, true>(Op, Opcode, rt, rn, FEXCore::ARMEmitter::Reg::r0);
  }
  template<FEXCore::ARMEmitter::SubRegSize size, typename T>
  void st4(T rt, T rt2, T rt3, T rt4, FEXCore::ARMEmitter::Register rn) {
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt2.Idx() + 1) == rt3.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt3.Idx() + 1) == rt4.Idx(), "These must be sequential");
    constexpr uint32_t Op = 0b0000'1100'000 << 21;
    constexpr uint32_t Opcode = 0b0000 << 12;
    ASIMDLoadStoreMultipleStructure<size, false>(Op, Opcode, rt, rn, FEXCore::ARMEmitter::Reg::r0);
  }
  // Advanced SIMD load/store multiple structures (post-indexed)
  static constexpr uint32_t ASIMDLoadstoreMultiplePost_Op = 0b0000'1100'100 << 21;
  template<FEXCore::ARMEmitter::SubRegSize size, typename T>
  void ld1(T rt, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    constexpr uint32_t Opcode = 0b0111 << 12;
    ASIMDLoadStoreMultipleStructure<size, true>(ASIMDLoadstoreMultiplePost_Op, Opcode, rt, rn, rm);
  }
  template<FEXCore::ARMEmitter::SubRegSize size, typename T>
  void ld1(T rt, FEXCore::ARMEmitter::Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_A_FMT(
      (std::is_same_v<FEXCore::ARMEmitter::QRegister, T> && (PostOffset == 16)) ||
      (std::is_same_v<FEXCore::ARMEmitter::DRegister, T> && (PostOffset == 8)),
      "Post-index offset needs to match number of elements times their size");

    constexpr uint32_t Opcode = 0b0111 << 12;
    ASIMDLoadStoreMultipleStructure<size, true>(ASIMDLoadstoreMultiplePost_Op, Opcode, rt, rn, FEXCore::ARMEmitter::Reg::r31);
  }
  template<FEXCore::ARMEmitter::SubRegSize size, typename T>
  void ld1(T rt, T rt2, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    constexpr uint32_t Opcode = 0b1010 << 12;
    ASIMDLoadStoreMultipleStructure<size, true>(ASIMDLoadstoreMultiplePost_Op, Opcode, rt, rn, rm);
  }
  template<FEXCore::ARMEmitter::SubRegSize size, typename T>
  void ld1(T rt, T rt2, FEXCore::ARMEmitter::Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT(
      (std::is_same_v<FEXCore::ARMEmitter::QRegister, T> && (PostOffset == 32)) ||
      (std::is_same_v<FEXCore::ARMEmitter::DRegister, T> && (PostOffset == 16)),
      "Post-index offset needs to match number of elements times their size");

    constexpr uint32_t Opcode = 0b1010 << 12;
    ASIMDLoadStoreMultipleStructure<size, true>(ASIMDLoadstoreMultiplePost_Op, Opcode, rt, rn, FEXCore::ARMEmitter::Reg::r31);
  }
  template<FEXCore::ARMEmitter::SubRegSize size, typename T>
  void ld1(T rt, T rt2, T rt3, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt2.Idx() + 1) == rt3.Idx(), "These must be sequential");
    constexpr uint32_t Opcode = 0b0110 << 12;
    ASIMDLoadStoreMultipleStructure<size, true>(ASIMDLoadstoreMultiplePost_Op, Opcode, rt, rn, rm);
  }
  template<FEXCore::ARMEmitter::SubRegSize size, typename T>
  void ld1(T rt, T rt2, T rt3, FEXCore::ARMEmitter::Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt2.Idx() + 1) == rt3.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT(
      (std::is_same_v<FEXCore::ARMEmitter::QRegister, T> && (PostOffset == 48)) ||
      (std::is_same_v<FEXCore::ARMEmitter::DRegister, T> && (PostOffset == 24)),
      "Post-index offset needs to match number of elements times their size");

    constexpr uint32_t Opcode = 0b0110 << 12;
    ASIMDLoadStoreMultipleStructure<size, true>(ASIMDLoadstoreMultiplePost_Op, Opcode, rt, rn, FEXCore::ARMEmitter::Reg::r31);
  }
  template<FEXCore::ARMEmitter::SubRegSize size, typename T>
  void ld1(T rt, T rt2, T rt3, T rt4, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt2.Idx() + 1) == rt3.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt3.Idx() + 1) == rt4.Idx(), "These must be sequential");
    constexpr uint32_t Opcode = 0b0010 << 12;
    ASIMDLoadStoreMultipleStructure<size, true>(ASIMDLoadstoreMultiplePost_Op, Opcode, rt, rn, rm);
  }
  template<FEXCore::ARMEmitter::SubRegSize size, typename T>
  void ld1(T rt, T rt2, T rt3, T rt4, FEXCore::ARMEmitter::Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt2.Idx() + 1) == rt3.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt3.Idx() + 1) == rt4.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT(
      (std::is_same_v<FEXCore::ARMEmitter::QRegister, T> && (PostOffset == 64)) ||
      (std::is_same_v<FEXCore::ARMEmitter::DRegister, T> && (PostOffset == 32)),
      "Post-index offset needs to match number of elements times their size");

    constexpr uint32_t Opcode = 0b0010 << 12;
    ASIMDLoadStoreMultipleStructure<size, true>(ASIMDLoadstoreMultiplePost_Op, Opcode, rt, rn, FEXCore::ARMEmitter::Reg::r31);
  }

  template<FEXCore::ARMEmitter::SubRegSize size, typename T>
  void st1(T rt, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    constexpr uint32_t Opcode = 0b0111 << 12;
    ASIMDLoadStoreMultipleStructure<size, false>(ASIMDLoadstoreMultiplePost_Op, Opcode, rt, rn, rm);
  }
  template<FEXCore::ARMEmitter::SubRegSize size, typename T>
  void st1(T rt, FEXCore::ARMEmitter::Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_A_FMT(
      (std::is_same_v<FEXCore::ARMEmitter::QRegister, T> && (PostOffset == 16)) ||
      (std::is_same_v<FEXCore::ARMEmitter::DRegister, T> && (PostOffset == 8)),
      "Post-index offset needs to match number of elements times their size");

    constexpr uint32_t Opcode = 0b0111 << 12;
    ASIMDLoadStoreMultipleStructure<size, false>(ASIMDLoadstoreMultiplePost_Op, Opcode, rt, rn, FEXCore::ARMEmitter::Reg::r31);
  }
  template<FEXCore::ARMEmitter::SubRegSize size, typename T>
  void st1(T rt, T rt2, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    constexpr uint32_t Opcode = 0b1010 << 12;
    ASIMDLoadStoreMultipleStructure<size, false>(ASIMDLoadstoreMultiplePost_Op, Opcode, rt, rn, rm);
  }
  template<FEXCore::ARMEmitter::SubRegSize size, typename T>
  void st1(T rt, T rt2, FEXCore::ARMEmitter::Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT(
      (std::is_same_v<FEXCore::ARMEmitter::QRegister, T> && (PostOffset == 32)) ||
      (std::is_same_v<FEXCore::ARMEmitter::DRegister, T> && (PostOffset == 16)),
      "Post-index offset needs to match number of elements times their size");

    constexpr uint32_t Opcode = 0b1010 << 12;
    ASIMDLoadStoreMultipleStructure<size, false>(ASIMDLoadstoreMultiplePost_Op, Opcode, rt, rn, FEXCore::ARMEmitter::Reg::r31);
  }
  template<FEXCore::ARMEmitter::SubRegSize size, typename T>
  void st1(T rt, T rt2, T rt3, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt2.Idx() + 1) == rt3.Idx(), "These must be sequential");
    constexpr uint32_t Opcode = 0b0110 << 12;
    ASIMDLoadStoreMultipleStructure<size, false>(ASIMDLoadstoreMultiplePost_Op, Opcode, rt, rn, rm);
  }
  template<FEXCore::ARMEmitter::SubRegSize size, typename T>
  void st1(T rt, T rt2, T rt3, FEXCore::ARMEmitter::Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt2.Idx() + 1) == rt3.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT(
      (std::is_same_v<FEXCore::ARMEmitter::QRegister, T> && (PostOffset == 48)) ||
      (std::is_same_v<FEXCore::ARMEmitter::DRegister, T> && (PostOffset == 24)),
      "Post-index offset needs to match number of elements times their size");

    constexpr uint32_t Opcode = 0b0110 << 12;
    ASIMDLoadStoreMultipleStructure<size, false>(ASIMDLoadstoreMultiplePost_Op, Opcode, rt, rn, FEXCore::ARMEmitter::Reg::r31);
  }
  template<FEXCore::ARMEmitter::SubRegSize size, typename T>
  void st1(T rt, T rt2, T rt3, T rt4, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt2.Idx() + 1) == rt3.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt3.Idx() + 1) == rt4.Idx(), "These must be sequential");
    constexpr uint32_t Opcode = 0b0010 << 12;
    ASIMDLoadStoreMultipleStructure<size, false>(ASIMDLoadstoreMultiplePost_Op, Opcode, rt, rn, rm);
  }
  template<FEXCore::ARMEmitter::SubRegSize size, typename T>
  void st1(T rt, T rt2, T rt3, T rt4, FEXCore::ARMEmitter::Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt2.Idx() + 1) == rt3.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt3.Idx() + 1) == rt4.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT(
      (std::is_same_v<FEXCore::ARMEmitter::QRegister, T> && (PostOffset == 64)) ||
      (std::is_same_v<FEXCore::ARMEmitter::DRegister, T> && (PostOffset == 32)),
      "Post-index offset needs to match number of elements times their size");

    constexpr uint32_t Opcode = 0b0010 << 12;
    ASIMDLoadStoreMultipleStructure<size, false>(ASIMDLoadstoreMultiplePost_Op, Opcode, rt, rn, FEXCore::ARMEmitter::Reg::r31);
  }

  template<FEXCore::ARMEmitter::SubRegSize size, typename T>
  void ld2(T rt, T rt2, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    constexpr uint32_t Opcode = 0b1000 << 12;
    ASIMDLoadStoreMultipleStructure<size, true>(ASIMDLoadstoreMultiplePost_Op, Opcode, rt, rn, rm);
  }
  template<FEXCore::ARMEmitter::SubRegSize size, typename T>
  void ld2(T rt, T rt2, FEXCore::ARMEmitter::Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT(
      (std::is_same_v<FEXCore::ARMEmitter::QRegister, T> && (PostOffset == 32)) ||
      (std::is_same_v<FEXCore::ARMEmitter::DRegister, T> && (PostOffset == 16)),
      "Post-index offset needs to match number of elements times their size");

    constexpr uint32_t Opcode = 0b1000 << 12;
    ASIMDLoadStoreMultipleStructure<size, true>(ASIMDLoadstoreMultiplePost_Op, Opcode, rt, rn, FEXCore::ARMEmitter::Reg::r31);
  }
  template<FEXCore::ARMEmitter::SubRegSize size, typename T>
  void st2(T rt, T rt2, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    constexpr uint32_t Opcode = 0b1000 << 12;
    ASIMDLoadStoreMultipleStructure<size, false>(ASIMDLoadstoreMultiplePost_Op, Opcode, rt, rn, rm);
  }
  template<FEXCore::ARMEmitter::SubRegSize size, typename T>
  void st2(T rt, T rt2, FEXCore::ARMEmitter::Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT(
      (std::is_same_v<FEXCore::ARMEmitter::QRegister, T> && (PostOffset == 32)) ||
      (std::is_same_v<FEXCore::ARMEmitter::DRegister, T> && (PostOffset == 16)),
      "Post-index offset needs to match number of elements times their size");

    constexpr uint32_t Opcode = 0b1000 << 12;
    ASIMDLoadStoreMultipleStructure<size, false>(ASIMDLoadstoreMultiplePost_Op, Opcode, rt, rn, FEXCore::ARMEmitter::Reg::r31);
  }
  template<FEXCore::ARMEmitter::SubRegSize size, typename T>
  void ld3(T rt, T rt2, T rt3, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt2.Idx() + 1) == rt3.Idx(), "These must be sequential");
    constexpr uint32_t Opcode = 0b0100 << 12;
    ASIMDLoadStoreMultipleStructure<size, true>(ASIMDLoadstoreMultiplePost_Op, Opcode, rt, rn, rm);
  }
  template<FEXCore::ARMEmitter::SubRegSize size, typename T>
  void ld3(T rt, T rt2, T rt3, FEXCore::ARMEmitter::Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt2.Idx() + 1) == rt3.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT(
      (std::is_same_v<FEXCore::ARMEmitter::QRegister, T> && (PostOffset == 48)) ||
      (std::is_same_v<FEXCore::ARMEmitter::DRegister, T> && (PostOffset == 24)),
      "Post-index offset needs to match number of elements times their size");

    constexpr uint32_t Opcode = 0b0100 << 12;
    ASIMDLoadStoreMultipleStructure<size, true>(ASIMDLoadstoreMultiplePost_Op, Opcode, rt, rn, FEXCore::ARMEmitter::Reg::r31);
  }
  template<FEXCore::ARMEmitter::SubRegSize size, typename T>
  void st3(T rt, T rt2, T rt3, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt2.Idx() + 1) == rt3.Idx(), "These must be sequential");
    constexpr uint32_t Opcode = 0b0100 << 12;
    ASIMDLoadStoreMultipleStructure<size, false>(ASIMDLoadstoreMultiplePost_Op, Opcode, rt, rn, rm);
  }
  template<FEXCore::ARMEmitter::SubRegSize size, typename T>
  void st3(T rt, T rt2, T rt3, FEXCore::ARMEmitter::Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt2.Idx() + 1) == rt3.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT(
      (std::is_same_v<FEXCore::ARMEmitter::QRegister, T> && (PostOffset == 48)) ||
      (std::is_same_v<FEXCore::ARMEmitter::DRegister, T> && (PostOffset == 24)),
      "Post-index offset needs to match number of elements times their size");

    constexpr uint32_t Opcode = 0b0100 << 12;
    ASIMDLoadStoreMultipleStructure<size, false>(ASIMDLoadstoreMultiplePost_Op, Opcode, rt, rn, FEXCore::ARMEmitter::Reg::r31);
  }
  template<FEXCore::ARMEmitter::SubRegSize size, typename T>
  void ld4(T rt, T rt2, T rt3, T rt4, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt2.Idx() + 1) == rt3.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt3.Idx() + 1) == rt4.Idx(), "These must be sequential");
    constexpr uint32_t Opcode = 0b0000 << 12;
    ASIMDLoadStoreMultipleStructure<size, true>(ASIMDLoadstoreMultiplePost_Op, Opcode, rt, rn, rm);
  }
  template<FEXCore::ARMEmitter::SubRegSize size, typename T>
  void ld4(T rt, T rt2, T rt3, T rt4, FEXCore::ARMEmitter::Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt2.Idx() + 1) == rt3.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt3.Idx() + 1) == rt4.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT(
      (std::is_same_v<FEXCore::ARMEmitter::QRegister, T> && (PostOffset == 64)) ||
      (std::is_same_v<FEXCore::ARMEmitter::DRegister, T> && (PostOffset == 32)),
      "Post-index offset needs to match number of elements times their size");

    constexpr uint32_t Opcode = 0b0000 << 12;
    ASIMDLoadStoreMultipleStructure<size, true>(ASIMDLoadstoreMultiplePost_Op, Opcode, rt, rn, FEXCore::ARMEmitter::Reg::r31);
  }
  template<FEXCore::ARMEmitter::SubRegSize size, typename T>
  void st4(T rt, T rt2, T rt3, T rt4, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt2.Idx() + 1) == rt3.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt3.Idx() + 1) == rt4.Idx(), "These must be sequential");
    constexpr uint32_t Opcode = 0b0000 << 12;
    ASIMDLoadStoreMultipleStructure<size, false>(ASIMDLoadstoreMultiplePost_Op, Opcode, rt, rn, rm);
  }
  template<FEXCore::ARMEmitter::SubRegSize size, typename T>
  void st4(T rt, T rt2, T rt3, T rt4, FEXCore::ARMEmitter::Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt2.Idx() + 1) == rt3.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt3.Idx() + 1) == rt4.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT(
      (std::is_same_v<FEXCore::ARMEmitter::QRegister, T> && (PostOffset == 64)) ||
      (std::is_same_v<FEXCore::ARMEmitter::DRegister, T> && (PostOffset == 32)),
      "Post-index offset needs to match number of elements times their size");

    constexpr uint32_t Opcode = 0b0000 << 12;
    ASIMDLoadStoreMultipleStructure<size, false>(ASIMDLoadstoreMultiplePost_Op, Opcode, rt, rn, FEXCore::ARMEmitter::Reg::r31);
  }

  // ASIMD loadstore single
  template<FEXCore::ARMEmitter::SubRegSize size>
  void st1(FEXCore::ARMEmitter::VRegister rt, uint32_t Index, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1101'000 << 21;
    constexpr uint32_t Opcode =
        size == SubRegSize::i8Bit  ? 0b000 : // Scale = 0
        size == SubRegSize::i16Bit ? 0b010 : // Scale = 1
        size == SubRegSize::i32Bit ? 0b100 : // Scale = 2
        size == SubRegSize::i64Bit ? 0b100 : // Scale = 2 (Uses size to determine difference between 32-bit).
        0;
    ASIMDSTLD<size, false, 1>(Op, Opcode, rt, Index, rn, FEXCore::ARMEmitter::Reg::r0);
  }
  template<FEXCore::ARMEmitter::SubRegSize size>
  void st2(FEXCore::ARMEmitter::VRegister rt, FEXCore::ARMEmitter::VRegister rt2, uint32_t Index, FEXCore::ARMEmitter::Register rn) {
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    constexpr uint32_t Op = 0b0000'1101'000 << 21;
    constexpr uint32_t Opcode =
        size == SubRegSize::i8Bit  ? 0b000 : // Scale = 0
        size == SubRegSize::i16Bit ? 0b010 : // Scale = 1
        size == SubRegSize::i32Bit ? 0b100 : // Scale = 2
        size == SubRegSize::i64Bit ? 0b100 : // Scale = 2 (Uses size to determine difference between 32-bit).
        0;
    ASIMDSTLD<size, false, 2>(Op, Opcode, rt, Index, rn, FEXCore::ARMEmitter::Reg::r0);
  }
  template<FEXCore::ARMEmitter::SubRegSize size>
  void st3(FEXCore::ARMEmitter::VRegister rt, FEXCore::ARMEmitter::VRegister rt2, FEXCore::ARMEmitter::VRegister rt3, uint32_t Index, FEXCore::ARMEmitter::Register rn) {
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt2.Idx() + 1) == rt3.Idx(), "These must be sequential");
    constexpr uint32_t Op = 0b0000'1101'000 << 21;
    constexpr uint32_t Opcode =
        size == SubRegSize::i8Bit  ? 0b000 : // Scale = 0
        size == SubRegSize::i16Bit ? 0b010 : // Scale = 1
        size == SubRegSize::i32Bit ? 0b100 : // Scale = 2
        size == SubRegSize::i64Bit ? 0b100 : // Scale = 2 (Uses size to determine difference between 32-bit).
        0;
    ASIMDSTLD<size, false, 3>(Op, Opcode, rt, Index, rn, FEXCore::ARMEmitter::Reg::r0);
  }
  template<FEXCore::ARMEmitter::SubRegSize size>
  void st4(FEXCore::ARMEmitter::VRegister rt, FEXCore::ARMEmitter::VRegister rt2, FEXCore::ARMEmitter::VRegister rt3, FEXCore::ARMEmitter::VRegister rt4, uint32_t Index, FEXCore::ARMEmitter::Register rn) {
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt2.Idx() + 1) == rt3.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt3.Idx() + 1) == rt4.Idx(), "These must be sequential");
    constexpr uint32_t Op = 0b0000'1101'000 << 21;
    constexpr uint32_t Opcode =
        size == SubRegSize::i8Bit  ? 0b000 : // Scale = 0
        size == SubRegSize::i16Bit ? 0b010 : // Scale = 1
        size == SubRegSize::i32Bit ? 0b100 : // Scale = 2
        size == SubRegSize::i64Bit ? 0b100 : // Scale = 2 (Uses size to determine difference between 32-bit).
        0;
    ASIMDSTLD<size, false, 4>(Op, Opcode, rt, Index, rn, FEXCore::ARMEmitter::Reg::r0);
  }
  template<FEXCore::ARMEmitter::SubRegSize size>
  void ld1(FEXCore::ARMEmitter::VRegister rt, uint32_t Index, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1101'000 << 21;
    constexpr uint32_t Opcode =
        size == SubRegSize::i8Bit  ? 0b000 : // Scale = 0
        size == SubRegSize::i16Bit ? 0b010 : // Scale = 1
        size == SubRegSize::i32Bit ? 0b100 : // Scale = 2
        size == SubRegSize::i64Bit ? 0b100 : // Scale = 2 (Uses size to determine difference between 32-bit).
        0;
    ASIMDSTLD<size, true, 1>(Op, Opcode, rt, Index, rn, FEXCore::ARMEmitter::Reg::r0);
  }
  template<FEXCore::ARMEmitter::SubRegSize size, typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void ld1r(T rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1101'000 << 21;
    constexpr uint32_t Opcode = 0b110;
    ASIMDSTLD<size, true, 1>(Op, Opcode, rt, rn, FEXCore::ARMEmitter::Reg::r0);
  }
  template<FEXCore::ARMEmitter::SubRegSize size>
  void ld2(FEXCore::ARMEmitter::VRegister rt, FEXCore::ARMEmitter::VRegister rt2, uint32_t Index, FEXCore::ARMEmitter::Register rn) {
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    constexpr uint32_t Op = 0b0000'1101'000 << 21;
    constexpr uint32_t Opcode =
        size == SubRegSize::i8Bit  ? 0b000 : // Scale = 0
        size == SubRegSize::i16Bit ? 0b010 : // Scale = 1
        size == SubRegSize::i32Bit ? 0b100 : // Scale = 2
        size == SubRegSize::i64Bit ? 0b100 : // Scale = 2 (Uses size to determine difference between 32-bit).
        0;
    ASIMDSTLD<size, true, 2>(Op, Opcode, rt, Index, rn, FEXCore::ARMEmitter::Reg::r0);
  }
  template<FEXCore::ARMEmitter::SubRegSize size, typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void ld2r(T rt, T rt2, FEXCore::ARMEmitter::Register rn) {
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    constexpr uint32_t Op = 0b0000'1101'000 << 21;
    constexpr uint32_t Opcode = 0b110;
    ASIMDSTLD<size, true, 2>(Op, Opcode, rt, rn, FEXCore::ARMEmitter::Reg::r0);
  }
  template<FEXCore::ARMEmitter::SubRegSize size>
  void ld3(FEXCore::ARMEmitter::VRegister rt, FEXCore::ARMEmitter::VRegister rt2, FEXCore::ARMEmitter::VRegister rt3, uint32_t Index, FEXCore::ARMEmitter::Register rn) {
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt2.Idx() + 1) == rt3.Idx(), "These must be sequential");
    constexpr uint32_t Op = 0b0000'1101'000 << 21;
    constexpr uint32_t Opcode =
        size == SubRegSize::i8Bit  ? 0b000 : // Scale = 0
        size == SubRegSize::i16Bit ? 0b010 : // Scale = 1
        size == SubRegSize::i32Bit ? 0b100 : // Scale = 2
        size == SubRegSize::i64Bit ? 0b100 : // Scale = 2 (Uses size to determine difference between 32-bit).
        0;
    ASIMDSTLD<size, true, 3>(Op, Opcode, rt, Index, rn, FEXCore::ARMEmitter::Reg::r0);
  }
  template<FEXCore::ARMEmitter::SubRegSize size, typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void ld3r(T rt, T rt2, T rt3, FEXCore::ARMEmitter::Register rn) {
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt2.Idx() + 1) == rt3.Idx(), "These must be sequential");
    constexpr uint32_t Op = 0b0000'1101'000 << 21;
    constexpr uint32_t Opcode = 0b110;
    ASIMDSTLD<size, true, 3>(Op, Opcode, rt, rn, FEXCore::ARMEmitter::Reg::r0);
  }
  template<FEXCore::ARMEmitter::SubRegSize size>
  void ld4(FEXCore::ARMEmitter::VRegister rt, FEXCore::ARMEmitter::VRegister rt2, FEXCore::ARMEmitter::VRegister rt3, FEXCore::ARMEmitter::VRegister rt4, uint32_t Index, FEXCore::ARMEmitter::Register rn) {
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt2.Idx() + 1) == rt3.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt3.Idx() + 1) == rt4.Idx(), "These must be sequential");
    constexpr uint32_t Op = 0b0000'1101'000 << 21;
    constexpr uint32_t Opcode =
        size == SubRegSize::i8Bit  ? 0b000 : // Scale = 0
        size == SubRegSize::i16Bit ? 0b010 : // Scale = 1
        size == SubRegSize::i32Bit ? 0b100 : // Scale = 2
        size == SubRegSize::i64Bit ? 0b100 : // Scale = 2 (Uses size to determine difference between 32-bit).
        0;
    ASIMDSTLD<size, true, 4>(Op, Opcode, rt, Index, rn, FEXCore::ARMEmitter::Reg::r0);
  }
  template<FEXCore::ARMEmitter::SubRegSize size, typename T>
  requires(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>)
  void ld4r(T rt, T rt2, T rt3, T rt4, FEXCore::ARMEmitter::Register rn) {
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt2.Idx() + 1) == rt3.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt3.Idx() + 1) == rt4.Idx(), "These must be sequential");
    constexpr uint32_t Op = 0b0000'1101'000 << 21;
    constexpr uint32_t Opcode = 0b110;
    ASIMDSTLD<size, true, 4>(Op, Opcode, rt, rn, FEXCore::ARMEmitter::Reg::r0);
  }

  // ASIMD loadstore single post-indexed
  template<FEXCore::ARMEmitter::SubRegSize size>
  void st1(FEXCore::ARMEmitter::VRegister rt, uint32_t Index, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    constexpr uint32_t Op = 0b0000'1101'100 << 21;
    constexpr uint32_t Opcode =
        size == SubRegSize::i8Bit  ? 0b000 : // Scale = 0
        size == SubRegSize::i16Bit ? 0b010 : // Scale = 1
        size == SubRegSize::i32Bit ? 0b100 : // Scale = 2
        size == SubRegSize::i64Bit ? 0b100 : // Scale = 2 (Uses size to determine difference between 32-bit).
        0;
    ASIMDSTLD<size, false, 1>(Op, Opcode, rt, Index, rn, rm);
  }
  template<FEXCore::ARMEmitter::SubRegSize size>
  void st1(FEXCore::ARMEmitter::VRegister rt, uint32_t Index, FEXCore::ARMEmitter::Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_A_FMT(
      (size == SubRegSize::i8Bit && (PostOffset == 1)) ||
      (size == SubRegSize::i16Bit && (PostOffset == 2)) ||
      (size == SubRegSize::i32Bit && (PostOffset == 4)) ||
      (size == SubRegSize::i64Bit && (PostOffset == 8)), "Post-index offset needs to match number of elements times their size");

    constexpr uint32_t Op = 0b0000'1101'100 << 21;
    constexpr uint32_t Opcode =
        size == SubRegSize::i8Bit  ? 0b000 : // Scale = 0
        size == SubRegSize::i16Bit ? 0b010 : // Scale = 1
        size == SubRegSize::i32Bit ? 0b100 : // Scale = 2
        size == SubRegSize::i64Bit ? 0b100 : // Scale = 2 (Uses size to determine difference between 32-bit).
        0;
    ASIMDSTLD<size, false, 1>(Op, Opcode, rt, Index, rn, FEXCore::ARMEmitter::Reg::r31);
  }
  template<FEXCore::ARMEmitter::SubRegSize size>
  void st2(FEXCore::ARMEmitter::VRegister rt, FEXCore::ARMEmitter::VRegister rt2, uint32_t Index, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    constexpr uint32_t Op = 0b0000'1101'100 << 21;
    constexpr uint32_t Opcode =
        size == SubRegSize::i8Bit  ? 0b000 : // Scale = 0
        size == SubRegSize::i16Bit ? 0b010 : // Scale = 1
        size == SubRegSize::i32Bit ? 0b100 : // Scale = 2
        size == SubRegSize::i64Bit ? 0b100 : // Scale = 2 (Uses size to determine difference between 32-bit).
        0;
    ASIMDSTLD<size, false, 2>(Op, Opcode, rt, Index, rn, rm);
  }
  template<FEXCore::ARMEmitter::SubRegSize size>
  void st2(FEXCore::ARMEmitter::VRegister rt, uint32_t Index, FEXCore::ARMEmitter::Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_A_FMT(
      (size == SubRegSize::i8Bit && (PostOffset == 2)) ||
      (size == SubRegSize::i16Bit && (PostOffset == 4)) ||
      (size == SubRegSize::i32Bit && (PostOffset == 8)) ||
      (size == SubRegSize::i64Bit && (PostOffset == 16)), "Post-index offset needs to match number of elements times their size");

    constexpr uint32_t Op = 0b0000'1101'100 << 21;
    constexpr uint32_t Opcode =
        size == SubRegSize::i8Bit  ? 0b000 : // Scale = 0
        size == SubRegSize::i16Bit ? 0b010 : // Scale = 1
        size == SubRegSize::i32Bit ? 0b100 : // Scale = 2
        size == SubRegSize::i64Bit ? 0b100 : // Scale = 2 (Uses size to determine difference between 32-bit).
        0;
    ASIMDSTLD<size, false, 2>(Op, Opcode, rt, Index, rn, FEXCore::ARMEmitter::Reg::r31);
  }
  template<FEXCore::ARMEmitter::SubRegSize size>
  void st3(FEXCore::ARMEmitter::VRegister rt, FEXCore::ARMEmitter::VRegister rt2, FEXCore::ARMEmitter::VRegister rt3, uint32_t Index, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt2.Idx() + 1) == rt3.Idx(), "These must be sequential");
    constexpr uint32_t Op = 0b0000'1101'100 << 21;
    constexpr uint32_t Opcode =
        size == SubRegSize::i8Bit  ? 0b000 : // Scale = 0
        size == SubRegSize::i16Bit ? 0b010 : // Scale = 1
        size == SubRegSize::i32Bit ? 0b100 : // Scale = 2
        size == SubRegSize::i64Bit ? 0b100 : // Scale = 2 (Uses size to determine difference between 32-bit).
        0;
    ASIMDSTLD<size, false, 3>(Op, Opcode, rt, Index, rn, rm);
  }
  template<FEXCore::ARMEmitter::SubRegSize size>
  void st3(FEXCore::ARMEmitter::VRegister rt, uint32_t Index, FEXCore::ARMEmitter::Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_A_FMT(
      (size == SubRegSize::i8Bit && (PostOffset == 3)) ||
      (size == SubRegSize::i16Bit && (PostOffset == 6)) ||
      (size == SubRegSize::i32Bit && (PostOffset == 8)) ||
      (size == SubRegSize::i64Bit && (PostOffset == 24)), "Post-index offset needs to match number of elements times their size");

    constexpr uint32_t Op = 0b0000'1101'100 << 21;
    constexpr uint32_t Opcode =
        size == SubRegSize::i8Bit  ? 0b000 : // Scale = 0
        size == SubRegSize::i16Bit ? 0b010 : // Scale = 1
        size == SubRegSize::i32Bit ? 0b100 : // Scale = 2
        size == SubRegSize::i64Bit ? 0b100 : // Scale = 2 (Uses size to determine difference between 32-bit).
        0;
    ASIMDSTLD<size, false, 3>(Op, Opcode, rt, Index, rn, FEXCore::ARMEmitter::Reg::r31);
  }
  template<FEXCore::ARMEmitter::SubRegSize size>
  void st4(FEXCore::ARMEmitter::VRegister rt, FEXCore::ARMEmitter::VRegister rt2, FEXCore::ARMEmitter::VRegister rt3, FEXCore::ARMEmitter::VRegister rt4, uint32_t Index, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt2.Idx() + 1) == rt3.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt3.Idx() + 1) == rt4.Idx(), "These must be sequential");
    constexpr uint32_t Op = 0b0000'1101'100 << 21;
    constexpr uint32_t Opcode =
        size == SubRegSize::i8Bit  ? 0b000 : // Scale = 0
        size == SubRegSize::i16Bit ? 0b010 : // Scale = 1
        size == SubRegSize::i32Bit ? 0b100 : // Scale = 2
        size == SubRegSize::i64Bit ? 0b100 : // Scale = 2 (Uses size to determine difference between 32-bit).
        0;
    ASIMDSTLD<size, false, 4>(Op, Opcode, rt, Index, rn, rm);
  }
  template<FEXCore::ARMEmitter::SubRegSize size>
  void st4(FEXCore::ARMEmitter::VRegister rt, uint32_t Index, FEXCore::ARMEmitter::Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_A_FMT(
      (size == SubRegSize::i8Bit && (PostOffset == 4)) ||
      (size == SubRegSize::i16Bit && (PostOffset == 8)) ||
      (size == SubRegSize::i32Bit && (PostOffset == 16)) ||
      (size == SubRegSize::i64Bit && (PostOffset == 32)), "Post-index offset needs to match number of elements times their size");

    constexpr uint32_t Op = 0b0000'1101'100 << 21;
    constexpr uint32_t Opcode =
        size == SubRegSize::i8Bit  ? 0b000 : // Scale = 0
        size == SubRegSize::i16Bit ? 0b010 : // Scale = 1
        size == SubRegSize::i32Bit ? 0b100 : // Scale = 2
        size == SubRegSize::i64Bit ? 0b100 : // Scale = 2 (Uses size to determine difference between 32-bit).
        0;
    ASIMDSTLD<size, false, 4>(Op, Opcode, rt, Index, rn, FEXCore::ARMEmitter::Reg::r31);
  }
  template<FEXCore::ARMEmitter::SubRegSize size>
  void ld1(FEXCore::ARMEmitter::VRegister rt, uint32_t Index, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    constexpr uint32_t Op = 0b0000'1101'100 << 21;
    constexpr uint32_t Opcode =
        size == SubRegSize::i8Bit  ? 0b000 : // Scale = 0
        size == SubRegSize::i16Bit ? 0b010 : // Scale = 1
        size == SubRegSize::i32Bit ? 0b100 : // Scale = 2
        size == SubRegSize::i64Bit ? 0b100 : // Scale = 2 (Uses size to determine difference between 32-bit).
        0;
    ASIMDSTLD<size, true, 1>(Op, Opcode, rt, Index, rn, rm);
  }
  template<FEXCore::ARMEmitter::SubRegSize size>
  void ld1(FEXCore::ARMEmitter::VRegister rt, uint32_t Index, FEXCore::ARMEmitter::Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_A_FMT(
      (size == SubRegSize::i8Bit && (PostOffset == 1)) ||
      (size == SubRegSize::i16Bit && (PostOffset == 2)) ||
      (size == SubRegSize::i32Bit && (PostOffset == 4)) ||
      (size == SubRegSize::i64Bit && (PostOffset == 8)), "Post-index offset needs to match number of elements times their size");
    constexpr uint32_t Op = 0b0000'1101'100 << 21;
    constexpr uint32_t Opcode =
        size == SubRegSize::i8Bit  ? 0b000 : // Scale = 0
        size == SubRegSize::i16Bit ? 0b010 : // Scale = 1
        size == SubRegSize::i32Bit ? 0b100 : // Scale = 2
        size == SubRegSize::i64Bit ? 0b100 : // Scale = 2 (Uses size to determine difference between 32-bit).
        0;
    ASIMDSTLD<size, true, 1>(Op, Opcode, rt, Index, rn, FEXCore::ARMEmitter::Reg::r31);
  }
  template<FEXCore::ARMEmitter::SubRegSize size>
  void ld1r(FEXCore::ARMEmitter::VRegister rt, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    constexpr uint32_t Op = 0b0000'1101'100 << 21;
    constexpr uint32_t Opcode = 0b110;
    ASIMDSTLD<size, true, 1>(Op, Opcode, rt, 0, rn, rm);
  }
  template<FEXCore::ARMEmitter::SubRegSize size>
  void ld1r(FEXCore::ARMEmitter::VRegister rt, FEXCore::ARMEmitter::Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_A_FMT(
      (size == SubRegSize::i8Bit && (PostOffset == 1)) ||
      (size == SubRegSize::i16Bit && (PostOffset == 2)) ||
      (size == SubRegSize::i32Bit && (PostOffset == 4)) ||
      (size == SubRegSize::i64Bit && (PostOffset == 8)), "Post-index offset needs to match number of elements times their size");
    constexpr uint32_t Op = 0b0000'1101'100 << 21;
    constexpr uint32_t Opcode = 0b110;
    ASIMDSTLD<size, true, 1>(Op, Opcode, rt, 0, rn, FEXCore::ARMEmitter::Reg::r31);
  }
  template<FEXCore::ARMEmitter::SubRegSize size>
  void ld2(FEXCore::ARMEmitter::VRegister rt, FEXCore::ARMEmitter::VRegister rt2, uint32_t Index, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    constexpr uint32_t Op = 0b0000'1101'100 << 21;
    constexpr uint32_t Opcode =
        size == SubRegSize::i8Bit  ? 0b000 : // Scale = 0
        size == SubRegSize::i16Bit ? 0b010 : // Scale = 1
        size == SubRegSize::i32Bit ? 0b100 : // Scale = 2
        size == SubRegSize::i64Bit ? 0b100 : // Scale = 2 (Uses size to determine difference between 32-bit).
        0;
    ASIMDSTLD<size, true, 2>(Op, Opcode, rt, Index, rn, rm);
  }
  template<FEXCore::ARMEmitter::SubRegSize size>
  void ld2(FEXCore::ARMEmitter::VRegister rt, uint32_t Index, FEXCore::ARMEmitter::Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_A_FMT(
      (size == SubRegSize::i8Bit && (PostOffset == 2)) ||
      (size == SubRegSize::i16Bit && (PostOffset == 4)) ||
      (size == SubRegSize::i32Bit && (PostOffset == 8)) ||
      (size == SubRegSize::i64Bit && (PostOffset == 16)), "Post-index offset needs to match number of elements times their size");
    constexpr uint32_t Op = 0b0000'1101'100 << 21;
    constexpr uint32_t Opcode =
        size == SubRegSize::i8Bit  ? 0b000 : // Scale = 0
        size == SubRegSize::i16Bit ? 0b010 : // Scale = 1
        size == SubRegSize::i32Bit ? 0b100 : // Scale = 2
        size == SubRegSize::i64Bit ? 0b100 : // Scale = 2 (Uses size to determine difference between 32-bit).
        0;
    ASIMDSTLD<size, true, 2>(Op, Opcode, rt, Index, rn, FEXCore::ARMEmitter::Reg::r31);
  }
  template<FEXCore::ARMEmitter::SubRegSize size>
  void ld2r(FEXCore::ARMEmitter::VRegister rt, FEXCore::ARMEmitter::VRegister rt2, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    constexpr uint32_t Op = 0b0000'1101'100 << 21;
    constexpr uint32_t Opcode = 0b110;
    ASIMDSTLD<size, true, 2>(Op, Opcode, rt, 0, rn, rm);
  }
  template<FEXCore::ARMEmitter::SubRegSize size>
  void ld2r(FEXCore::ARMEmitter::VRegister rt, FEXCore::ARMEmitter::VRegister rt2, FEXCore::ARMEmitter::Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT(
      (size == SubRegSize::i8Bit && (PostOffset == 2)) ||
      (size == SubRegSize::i16Bit && (PostOffset == 4)) ||
      (size == SubRegSize::i32Bit && (PostOffset == 8)) ||
      (size == SubRegSize::i64Bit && (PostOffset == 16)), "Post-index offset needs to match number of elements times their size");
    constexpr uint32_t Op = 0b0000'1101'100 << 21;
    constexpr uint32_t Opcode = 0b110;
    ASIMDSTLD<size, true, 2>(Op, Opcode, rt, 0, rn, FEXCore::ARMEmitter::Reg::r31);
  }
  template<FEXCore::ARMEmitter::SubRegSize size>
  void ld3(FEXCore::ARMEmitter::VRegister rt, FEXCore::ARMEmitter::VRegister rt2, FEXCore::ARMEmitter::VRegister rt3, uint32_t Index, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt2.Idx() + 1) == rt3.Idx(), "These must be sequential");
    constexpr uint32_t Op = 0b0000'1101'100 << 21;
    constexpr uint32_t Opcode =
        size == SubRegSize::i8Bit  ? 0b000 : // Scale = 0
        size == SubRegSize::i16Bit ? 0b010 : // Scale = 1
        size == SubRegSize::i32Bit ? 0b100 : // Scale = 2
        size == SubRegSize::i64Bit ? 0b100 : // Scale = 2 (Uses size to determine difference between 32-bit).
        0;
    ASIMDSTLD<size, true, 3>(Op, Opcode, rt, Index, rn, rm);
  }
  template<FEXCore::ARMEmitter::SubRegSize size>
  void ld3(FEXCore::ARMEmitter::VRegister rt, uint32_t Index, FEXCore::ARMEmitter::Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_A_FMT(
      (size == SubRegSize::i8Bit && (PostOffset == 3)) ||
      (size == SubRegSize::i16Bit && (PostOffset == 6)) ||
      (size == SubRegSize::i32Bit && (PostOffset == 12)) ||
      (size == SubRegSize::i64Bit && (PostOffset == 16)), "Post-index offset needs to match number of elements times their size");
    constexpr uint32_t Op = 0b0000'1101'100 << 21;
    constexpr uint32_t Opcode =
        size == SubRegSize::i8Bit  ? 0b000 : // Scale = 0
        size == SubRegSize::i16Bit ? 0b010 : // Scale = 1
        size == SubRegSize::i32Bit ? 0b100 : // Scale = 2
        size == SubRegSize::i64Bit ? 0b100 : // Scale = 2 (Uses size to determine difference between 32-bit).
        0;
    ASIMDSTLD<size, true, 3>(Op, Opcode, rt, Index, rn, FEXCore::ARMEmitter::Reg::r31);
  }
  template<FEXCore::ARMEmitter::SubRegSize size>
  void ld3r(FEXCore::ARMEmitter::VRegister rt, FEXCore::ARMEmitter::VRegister rt2, FEXCore::ARMEmitter::VRegister rt3, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt2.Idx() + 1) == rt3.Idx(), "These must be sequential");
    constexpr uint32_t Op = 0b0000'1101'100 << 21;
    constexpr uint32_t Opcode = 0b110;
    ASIMDSTLD<size, true, 3>(Op, Opcode, rt, 0, rn, rm);
  }
  template<FEXCore::ARMEmitter::SubRegSize size>
  void ld3r(FEXCore::ARMEmitter::VRegister rt, FEXCore::ARMEmitter::VRegister rt2, FEXCore::ARMEmitter::VRegister rt3, FEXCore::ARMEmitter::Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt2.Idx() + 1) == rt3.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT(
      (size == SubRegSize::i8Bit && (PostOffset == 3)) ||
      (size == SubRegSize::i16Bit && (PostOffset == 6)) ||
      (size == SubRegSize::i32Bit && (PostOffset == 12)) ||
      (size == SubRegSize::i64Bit && (PostOffset == 16)), "Post-index offset needs to match number of elements times their size");
    constexpr uint32_t Op = 0b0000'1101'100 << 21;
    constexpr uint32_t Opcode = 0b110;
    ASIMDSTLD<size, true, 3>(Op, Opcode, rt, 0, rn, FEXCore::ARMEmitter::Reg::r31);
  }
  template<FEXCore::ARMEmitter::SubRegSize size>
  void ld4(FEXCore::ARMEmitter::VRegister rt, FEXCore::ARMEmitter::VRegister rt2, FEXCore::ARMEmitter::VRegister rt3, FEXCore::ARMEmitter::VRegister rt4, uint32_t Index, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt2.Idx() + 1) == rt3.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt3.Idx() + 1) == rt4.Idx(), "These must be sequential");
    constexpr uint32_t Op = 0b0000'1101'100 << 21;
    constexpr uint32_t Opcode =
        size == SubRegSize::i8Bit  ? 0b000 : // Scale = 0
        size == SubRegSize::i16Bit ? 0b010 : // Scale = 1
        size == SubRegSize::i32Bit ? 0b100 : // Scale = 2
        size == SubRegSize::i64Bit ? 0b100 : // Scale = 2 (Uses size to determine difference between 32-bit).
        0;
    ASIMDSTLD<size, true, 4>(Op, Opcode, rt, Index, rn, rm);
  }
  template<FEXCore::ARMEmitter::SubRegSize size>
  void ld4(FEXCore::ARMEmitter::VRegister rt, uint32_t Index, FEXCore::ARMEmitter::Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_A_FMT(
      (size == SubRegSize::i8Bit && (PostOffset == 4)) ||
      (size == SubRegSize::i16Bit && (PostOffset == 8)) ||
      (size == SubRegSize::i32Bit && (PostOffset == 16)) ||
      (size == SubRegSize::i64Bit && (PostOffset == 32)), "Post-index offset needs to match number of elements times their size");
    constexpr uint32_t Op = 0b0000'1101'100 << 21;
    constexpr uint32_t Opcode =
        size == SubRegSize::i8Bit  ? 0b000 : // Scale = 0
        size == SubRegSize::i16Bit ? 0b010 : // Scale = 1
        size == SubRegSize::i32Bit ? 0b100 : // Scale = 2
        size == SubRegSize::i64Bit ? 0b100 : // Scale = 2 (Uses size to determine difference between 32-bit).
        0;
    ASIMDSTLD<size, true, 4>(Op, Opcode, rt, Index, rn, FEXCore::ARMEmitter::Reg::r31);
  }
  template<FEXCore::ARMEmitter::SubRegSize size>
  void ld4r(FEXCore::ARMEmitter::VRegister rt, FEXCore::ARMEmitter::VRegister rt2, FEXCore::ARMEmitter::VRegister rt3, FEXCore::ARMEmitter::VRegister rt4, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt2.Idx() + 1) == rt3.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt3.Idx() + 1) == rt4.Idx(), "These must be sequential");
    constexpr uint32_t Op = 0b0000'1101'100 << 21;
    constexpr uint32_t Opcode = 0b110;
    ASIMDSTLD<size, true, 4>(Op, Opcode, rt, 0, rn, rm);
  }
  template<FEXCore::ARMEmitter::SubRegSize size>
  void ld4r(FEXCore::ARMEmitter::VRegister rt, FEXCore::ARMEmitter::VRegister rt2, FEXCore::ARMEmitter::VRegister rt3, FEXCore::ARMEmitter::VRegister rt4, FEXCore::ARMEmitter::Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt2.Idx() + 1) == rt3.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt3.Idx() + 1) == rt4.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT(
      (size == SubRegSize::i8Bit && (PostOffset == 4)) ||
      (size == SubRegSize::i16Bit && (PostOffset == 8)) ||
      (size == SubRegSize::i32Bit && (PostOffset == 16)) ||
      (size == SubRegSize::i64Bit && (PostOffset == 32)), "Post-index offset needs to match number of elements times their size");
    constexpr uint32_t Op = 0b0000'1101'100 << 21;
    constexpr uint32_t Opcode = 0b110;
    ASIMDSTLD<size, true, 4>(Op, Opcode, rt, 0, rn, FEXCore::ARMEmitter::Reg::r31);
  }

  // Advanced SIMD load/store single structure (post-indexed)
  template<typename T>
  void st1(FEXCore::ARMEmitter::SubRegSize size, T rt, uint32_t Index, FEXCore::ARMEmitter::Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_AA_FMT(size == SubRegSize::i8Bit || size == SubRegSize::i16Bit || size == SubRegSize::i32Bit || size == SubRegSize::i64Bit, "Incorrect size");
    constexpr uint32_t Op = 0b0000'1101'1 << 23;
    uint32_t Q;
    uint32_t R = 0;
    uint32_t opcode;
    uint32_t S;
    uint32_t Size;
    if (size == SubRegSize::i8Bit) {
      LOGMAN_THROW_AA_FMT(Index < 16, "Index too large");
      Q = Index >> 3;
      S = (Index >> 2) & 1;
      opcode = 0b000;
      Size = Index & 0b11;
    }
    else if (size == SubRegSize::i16Bit) {
      LOGMAN_THROW_AA_FMT(Index < 8, "Index too large");
      Q = Index >> 2;
      S = (Index >> 1) & 1;
      opcode = 0b010;
      Size = (Index & 0b1) << 1;
    }
    else if (size == SubRegSize::i32Bit) {
      LOGMAN_THROW_AA_FMT(Index < 4, "Index too large");
      Q = Index >> 1;
      S = Index & 1;
      opcode = 0b100;
      Size = 0b00;
    }
    else if (size == SubRegSize::i64Bit) {
      LOGMAN_THROW_AA_FMT(Index < 2, "Index too large");
      Q = Index;
      S = 0;
      opcode = 0b100;
      Size = 0b01;
    }
    else {
      LOGMAN_MSG_A_FMT("Unknown size");
      FEX_UNREACHABLE;
    }

    ASIMDLoadStoreSinglePost(Op, Q, 0, R, opcode, S, Size, FEXCore::ARMEmitter::Reg::r31, rn, rt.Q());
  }
  template<typename T>
  void ld1(FEXCore::ARMEmitter::SubRegSize size, T rt, uint32_t Index, FEXCore::ARMEmitter::Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_AA_FMT(size == SubRegSize::i8Bit || size == SubRegSize::i16Bit || size == SubRegSize::i32Bit || size == SubRegSize::i64Bit, "Incorrect size");
    constexpr uint32_t Op = 0b0000'1101'1 << 23;
    uint32_t Q;
    uint32_t R = 0;
    uint32_t opcode;
    uint32_t S;
    uint32_t Size;
    if (size == SubRegSize::i8Bit) {
      LOGMAN_THROW_AA_FMT(Index < 16, "Index too large");
      Q = Index >> 3;
      S = (Index >> 2) & 1;
      opcode = 0b001;
      Size = Index & 0b11;
    }
    else if (size == SubRegSize::i16Bit) {
      LOGMAN_THROW_AA_FMT(Index < 8, "Index too large");
      Q = Index >> 2;
      S = (Index >> 1) & 1;
      opcode = 0b011;
      Size = (Index & 0b1) << 1;
    }
    else if (size == SubRegSize::i32Bit) {
      LOGMAN_THROW_AA_FMT(Index < 4, "Index too large");
      Q = Index >> 1;
      S = Index & 1;
      opcode = 0b100;
      Size = 0b00;
    }
    else if (size == SubRegSize::i64Bit) {
      LOGMAN_THROW_AA_FMT(Index < 2, "Index too large");
      Q = Index;
      S = 0;
      opcode = 0b101;
      Size = 0b01;
    }
    else {
      LOGMAN_MSG_A_FMT("Unknown size");
      FEX_UNREACHABLE;
    }

    ASIMDLoadStoreSinglePost(Op, Q, 1, R, opcode, S, Size, FEXCore::ARMEmitter::Reg::r31, rn, rt.Q());
  }
  template<typename T>
  void ld1r(FEXCore::ARMEmitter::SubRegSize size, T rt, FEXCore::ARMEmitter::Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_AA_FMT(PostOffset == 1 || PostOffset == 2 || PostOffset == 4 || PostOffset == 8, "Index too large");
    constexpr uint32_t Op = 0b0000'1101'1 << 23;
    constexpr uint32_t Q = std::is_same_v<FEXCore::ARMEmitter::QRegister, T> ? 1 : 0;
    uint32_t R = 0;
    uint32_t opcode = 0b110;
    uint32_t S = 0;
    uint32_t Size = FEXCore::ToUnderlying(size);
    ASIMDLoadStoreSinglePost<T>(Op, Q, 1, R, opcode, S, Size, FEXCore::ARMEmitter::Reg::r31, rn, rt);
  }

  template<typename T>
  void ld2r(FEXCore::ARMEmitter::SubRegSize size, T rt, T rt2, FEXCore::ARMEmitter::Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    LOGMAN_THROW_AA_FMT(PostOffset == 2 || PostOffset == 4 || PostOffset == 8 || PostOffset == 16, "Index too large");
    constexpr uint32_t Op = 0b0000'1101'1 << 23;
    constexpr uint32_t Q = std::is_same_v<FEXCore::ARMEmitter::QRegister, T> ? 1 : 0;
    uint32_t R = 1;
    uint32_t opcode = 0b110;
    uint32_t S = 0;
    uint32_t Size = FEXCore::ToUnderlying(size);
    ASIMDLoadStoreSinglePost<T>(Op, Q, 1, R, opcode, S, Size, FEXCore::ARMEmitter::Reg::r31, rn, rt);
  }

  template<typename T>
  void ld3r(FEXCore::ARMEmitter::SubRegSize size, T rt, T rt2, T rt3, FEXCore::ARMEmitter::Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt2.Idx() + 1) == rt3.Idx(), "These must be sequential");
    LOGMAN_THROW_AA_FMT(PostOffset == 3 || PostOffset == 6 || PostOffset == 12 || PostOffset == 24, "Index too large");
    constexpr uint32_t Op = 0b0000'1101'1 << 23;
    constexpr uint32_t Q = std::is_same_v<FEXCore::ARMEmitter::QRegister, T> ? 1 : 0;
    uint32_t R = 0;
    uint32_t opcode = 0b111;
    uint32_t S = 0;
    uint32_t Size = FEXCore::ToUnderlying(size);
    ASIMDLoadStoreSinglePost<T>(Op, Q, 1, R, opcode, S, Size, FEXCore::ARMEmitter::Reg::r31, rn, rt);
  }
  template<typename T>
  void ld4r(FEXCore::ARMEmitter::SubRegSize size, T rt, T rt2, T rt3, T rt4, FEXCore::ARMEmitter::Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt2.Idx() + 1) == rt3.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt3.Idx() + 1) == rt4.Idx(), "These must be sequential");
    LOGMAN_THROW_AA_FMT(PostOffset == 4 || PostOffset == 8 || PostOffset == 16 || PostOffset == 32, "Index too large");
    constexpr uint32_t Op = 0b0000'1101'1 << 23;
    constexpr uint32_t Q = std::is_same_v<FEXCore::ARMEmitter::QRegister, T> ? 1 : 0;
    uint32_t R = 1;
    uint32_t opcode = 0b111;
    uint32_t S = 0;
    uint32_t Size = FEXCore::ToUnderlying(size);
    ASIMDLoadStoreSinglePost<T>(Op, Q, 1, R, opcode, S, Size, FEXCore::ARMEmitter::Reg::r31, rn, rt);
  }

  template<typename T>
  void st2(FEXCore::ARMEmitter::SubRegSize size, T rt, T rt2, uint32_t Index, FEXCore::ARMEmitter::Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_AA_FMT(size == SubRegSize::i8Bit || size == SubRegSize::i16Bit || size == SubRegSize::i32Bit || size == SubRegSize::i64Bit, "Incorrect size");
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");

    constexpr uint32_t Op = 0b0000'1101'1 << 23;
    uint32_t Q;
    uint32_t R = 1;
    uint32_t opcode;
    uint32_t S;
    uint32_t Size;
    if (size == SubRegSize::i8Bit) {
      LOGMAN_THROW_AA_FMT(Index < 16, "Index too large");
      Q = Index >> 3;
      S = (Index >> 2) & 1;
      opcode = 0b000;
      Size = Index & 0b11;
    }
    else if (size == SubRegSize::i16Bit) {
      LOGMAN_THROW_AA_FMT(Index < 8, "Index too large");
      Q = Index >> 2;
      S = (Index >> 1) & 1;
      opcode = 0b010;
      Size = (Index & 0b1) << 1;
    }
    else if (size == SubRegSize::i32Bit) {
      LOGMAN_THROW_AA_FMT(Index < 4, "Index too large");
      Q = Index >> 1;
      S = Index & 1;
      opcode = 0b100;
      Size = 0b00;
    }
    else if (size == SubRegSize::i64Bit) {
      LOGMAN_THROW_AA_FMT(Index < 2, "Index too large");
      Q = Index;
      S = 0;
      opcode = 0b100;
      Size = 0b01;
    }
    else {
      LOGMAN_MSG_A_FMT("Unknown size");
      FEX_UNREACHABLE;
    }

    ASIMDLoadStoreSinglePost(Op, Q, 0, R, opcode, S, Size, FEXCore::ARMEmitter::Reg::r31, rn, rt.Q());
  }
  template<typename T>
  void ld2(FEXCore::ARMEmitter::SubRegSize size, T rt, T rt2, uint32_t Index, FEXCore::ARMEmitter::Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_AA_FMT(size == SubRegSize::i8Bit || size == SubRegSize::i16Bit || size == SubRegSize::i32Bit || size == SubRegSize::i64Bit, "Incorrect size");
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");

    constexpr uint32_t Op = 0b0000'1101'1 << 23;
    uint32_t Q;
    uint32_t R = 1;
    uint32_t opcode;
    uint32_t S;
    uint32_t Size;
    if (size == SubRegSize::i8Bit) {
      LOGMAN_THROW_AA_FMT(Index < 16, "Index too large");
      Q = Index >> 3;
      S = (Index >> 2) & 1;
      opcode = 0b000;
      Size = Index & 0b11;
    }
    else if (size == SubRegSize::i16Bit) {
      LOGMAN_THROW_AA_FMT(Index < 8, "Index too large");
      Q = Index >> 2;
      S = (Index >> 1) & 1;
      opcode = 0b010;
      Size = (Index & 0b1) << 1;
    }
    else if (size == SubRegSize::i32Bit) {
      LOGMAN_THROW_AA_FMT(Index < 4, "Index too large");
      Q = Index >> 1;
      S = Index & 1;
      opcode = 0b100;
      Size = 0b00;
    }
    else if (size == SubRegSize::i64Bit) {
      LOGMAN_THROW_AA_FMT(Index < 2, "Index too large");
      Q = Index;
      S = 0;
      opcode = 0b100;
      Size = 0b01;
    }
    else {
      LOGMAN_MSG_A_FMT("Unknown size");
      FEX_UNREACHABLE;
    }

    ASIMDLoadStoreSinglePost(Op, Q, 1, R, opcode, S, Size, FEXCore::ARMEmitter::Reg::r31, rn, rt.Q());
  }
  template<typename T>
  void st3(FEXCore::ARMEmitter::SubRegSize size, T rt, T rt2, T rt3, uint32_t Index, FEXCore::ARMEmitter::Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_AA_FMT(size == SubRegSize::i8Bit || size == SubRegSize::i16Bit || size == SubRegSize::i32Bit || size == SubRegSize::i64Bit, "Incorrect size");
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt2.Idx() + 1) == rt3.Idx(), "These must be sequential");

    constexpr uint32_t Op = 0b0000'1101'1 << 23;
    uint32_t Q;
    uint32_t R = 0;
    uint32_t opcode;
    uint32_t S;
    uint32_t Size;
    if (size == SubRegSize::i8Bit) {
      LOGMAN_THROW_AA_FMT(Index < 16, "Index too large");
      Q = Index >> 3;
      S = (Index >> 2) & 1;
      opcode = 0b001;
      Size = Index & 0b11;
    }
    else if (size == SubRegSize::i16Bit) {
      LOGMAN_THROW_AA_FMT(Index < 8, "Index too large");
      Q = Index >> 2;
      S = (Index >> 1) & 1;
      opcode = 0b011;
      Size = (Index & 0b1) << 1;
    }
    else if (size == SubRegSize::i32Bit) {
      LOGMAN_THROW_AA_FMT(Index < 4, "Index too large");
      Q = Index >> 1;
      S = Index & 1;
      opcode = 0b101;
      Size = 0b00;
    }
    else if (size == SubRegSize::i64Bit) {
      LOGMAN_THROW_AA_FMT(Index < 2, "Index too large");
      Q = Index;
      S = 0;
      opcode = 0b101;
      Size = 0b01;
    }
    else {
      LOGMAN_MSG_A_FMT("Unknown size");
      FEX_UNREACHABLE;
    }

    ASIMDLoadStoreSinglePost(Op, Q, 0, R, opcode, S, Size, FEXCore::ARMEmitter::Reg::r31, rn, rt.Q());
  }
  template<typename T>
  void ld3(FEXCore::ARMEmitter::SubRegSize size, T rt, T rt2, T rt3, uint32_t Index, FEXCore::ARMEmitter::Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_AA_FMT(size == SubRegSize::i8Bit || size == SubRegSize::i16Bit || size == SubRegSize::i32Bit || size == SubRegSize::i64Bit, "Incorrect size");
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt2.Idx() + 1) == rt3.Idx(), "These must be sequential");

    constexpr uint32_t Op = 0b0000'1101'1 << 23;
    uint32_t Q;
    uint32_t R = 0;
    uint32_t opcode;
    uint32_t S;
    uint32_t Size;
    if (size == SubRegSize::i8Bit) {
      LOGMAN_THROW_AA_FMT(Index < 16, "Index too large");
      Q = Index >> 3;
      S = (Index >> 2) & 1;
      opcode = 0b001;
      Size = Index & 0b11;
    }
    else if (size == SubRegSize::i16Bit) {
      LOGMAN_THROW_AA_FMT(Index < 8, "Index too large");
      Q = Index >> 2;
      S = (Index >> 1) & 1;
      opcode = 0b011;
      Size = (Index & 0b1) << 1;
    }
    else if (size == SubRegSize::i32Bit) {
      LOGMAN_THROW_AA_FMT(Index < 4, "Index too large");
      Q = Index >> 1;
      S = Index & 1;
      opcode = 0b101;
      Size = 0b00;
    }
    else if (size == SubRegSize::i64Bit) {
      LOGMAN_THROW_AA_FMT(Index < 2, "Index too large");
      Q = Index;
      S = 0;
      opcode = 0b101;
      Size = 0b01;
    }
    else {
      LOGMAN_MSG_A_FMT("Unknown size");
      FEX_UNREACHABLE;
    }

    ASIMDLoadStoreSinglePost(Op, Q, 1, R, opcode, S, Size, FEXCore::ARMEmitter::Reg::r31, rn, rt.Q());
  }
  template<typename T>
  void st4(FEXCore::ARMEmitter::SubRegSize size, T rt, T rt2, T rt3, T rt4, uint32_t Index, FEXCore::ARMEmitter::Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_AA_FMT(size == SubRegSize::i8Bit || size == SubRegSize::i16Bit || size == SubRegSize::i32Bit || size == SubRegSize::i64Bit, "Incorrect size");
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt2.Idx() + 1) == rt3.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt3.Idx() + 1) == rt4.Idx(), "These must be sequential");

    constexpr uint32_t Op = 0b0000'1101'1 << 23;
    uint32_t Q;
    uint32_t R = 1;
    uint32_t opcode;
    uint32_t S;
    uint32_t Size;
    if (size == SubRegSize::i8Bit) {
      LOGMAN_THROW_AA_FMT(Index < 16, "Index too large");
      Q = Index >> 3;
      S = (Index >> 2) & 1;
      opcode = 0b001;
      Size = Index & 0b11;
    }
    else if (size == SubRegSize::i16Bit) {
      LOGMAN_THROW_AA_FMT(Index < 8, "Index too large");
      Q = Index >> 2;
      S = (Index >> 1) & 1;
      opcode = 0b011;
      Size = (Index & 0b1) << 1;
    }
    else if (size == SubRegSize::i32Bit) {
      LOGMAN_THROW_AA_FMT(Index < 4, "Index too large");
      Q = Index >> 1;
      S = Index & 1;
      opcode = 0b101;
      Size = 0b00;
    }
    else if (size == SubRegSize::i64Bit) {
      LOGMAN_THROW_AA_FMT(Index < 2, "Index too large");
      Q = Index;
      S = 0;
      opcode = 0b101;
      Size = 0b01;
    }
    else {
      LOGMAN_MSG_A_FMT("Unknown size");
      FEX_UNREACHABLE;
    }

    ASIMDLoadStoreSinglePost(Op, Q, 0, R, opcode, S, Size, FEXCore::ARMEmitter::Reg::r31, rn, rt.Q());
  }
  template<typename T>
  void ld4(FEXCore::ARMEmitter::SubRegSize size, T rt, T rt2, T rt3, T rt4, uint32_t Index, FEXCore::ARMEmitter::Register rn, uint32_t PostOffset) {
    LOGMAN_THROW_AA_FMT(size == SubRegSize::i8Bit || size == SubRegSize::i16Bit || size == SubRegSize::i32Bit || size == SubRegSize::i64Bit, "Incorrect size");
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt2.Idx() + 1) == rt3.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt3.Idx() + 1) == rt4.Idx(), "These must be sequential");

    constexpr uint32_t Op = 0b0000'1101'1 << 23;
    uint32_t Q;
    uint32_t R = 1;
    uint32_t opcode;
    uint32_t S;
    uint32_t Size;
    if (size == SubRegSize::i8Bit) {
      LOGMAN_THROW_AA_FMT(Index < 16, "Index too large");
      Q = Index >> 3;
      S = (Index >> 2) & 1;
      opcode = 0b001;
      Size = Index & 0b11;
    }
    else if (size == SubRegSize::i16Bit) {
      LOGMAN_THROW_AA_FMT(Index < 8, "Index too large");
      Q = Index >> 2;
      S = (Index >> 1) & 1;
      opcode = 0b011;
      Size = (Index & 0b1) << 1;
    }
    else if (size == SubRegSize::i32Bit) {
      LOGMAN_THROW_AA_FMT(Index < 4, "Index too large");
      Q = Index >> 1;
      S = Index & 1;
      opcode = 0b101;
      Size = 0b00;
    }
    else if (size == SubRegSize::i64Bit) {
      LOGMAN_THROW_AA_FMT(Index < 2, "Index too large");
      Q = Index;
      S = 0;
      opcode = 0b101;
      Size = 0b01;
    }
    else {
      LOGMAN_MSG_A_FMT("Unknown size");
      FEX_UNREACHABLE;
    }

    ASIMDLoadStoreSinglePost(Op, Q, 1, R, opcode, S, Size, FEXCore::ARMEmitter::Reg::r31, rn, rt.Q());
  }

  template<typename T>
  void st1(FEXCore::ARMEmitter::SubRegSize size, T rt, uint32_t Index, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    LOGMAN_THROW_AA_FMT(size == SubRegSize::i8Bit || size == SubRegSize::i16Bit || size == SubRegSize::i32Bit || size == SubRegSize::i64Bit, "Incorrect size");
    constexpr uint32_t Op = 0b0000'1101'1 << 23;
    uint32_t Q;
    uint32_t R = 0;
    uint32_t opcode;
    uint32_t S;
    uint32_t Size;
    if (size == SubRegSize::i8Bit) {
      LOGMAN_THROW_AA_FMT(Index < 16, "Index too large");
      Q = Index >> 3;
      S = (Index >> 2) & 1;
      opcode = 0b000;
      Size = Index & 0b11;
    }
    else if (size == SubRegSize::i16Bit) {
      LOGMAN_THROW_AA_FMT(Index < 8, "Index too large");
      Q = Index >> 2;
      S = (Index >> 1) & 1;
      opcode = 0b010;
      Size = (Index & 0b1) << 1;
    }
    else if (size == SubRegSize::i32Bit) {
      LOGMAN_THROW_AA_FMT(Index < 4, "Index too large");
      Q = Index >> 1;
      S = Index & 1;
      opcode = 0b100;
      Size = 0b00;
    }
    else if (size == SubRegSize::i64Bit) {
      LOGMAN_THROW_AA_FMT(Index < 2, "Index too large");
      Q = Index;
      S = 0;
      opcode = 0b100;
      Size = 0b01;
    }
    else {
      LOGMAN_MSG_A_FMT("Unknown size");
      FEX_UNREACHABLE;
    }

    ASIMDLoadStoreSinglePost(Op, Q, 0, R, opcode, S, Size, rm, rn, rt.Q());
  }
  template<typename T>
  void ld1(FEXCore::ARMEmitter::SubRegSize size, T rt, uint32_t Index, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    LOGMAN_THROW_AA_FMT(size == SubRegSize::i8Bit || size == SubRegSize::i16Bit || size == SubRegSize::i32Bit || size == SubRegSize::i64Bit, "Incorrect size");
    constexpr uint32_t Op = 0b0000'1101'1 << 23;
    uint32_t Q;
    uint32_t R = 0;
    uint32_t opcode;
    uint32_t S;
    uint32_t Size;
    if (size == SubRegSize::i8Bit) {
      LOGMAN_THROW_AA_FMT(Index < 16, "Index too large");
      Q = Index >> 3;
      S = (Index >> 2) & 1;
      opcode = 0b001;
      Size = Index & 0b11;
    }
    else if (size == SubRegSize::i16Bit) {
      LOGMAN_THROW_AA_FMT(Index < 8, "Index too large");
      Q = Index >> 2;
      S = (Index >> 1) & 1;
      opcode = 0b011;
      Size = (Index & 0b1) << 1;
    }
    else if (size == SubRegSize::i32Bit) {
      LOGMAN_THROW_AA_FMT(Index < 4, "Index too large");
      Q = Index >> 1;
      S = Index & 1;
      opcode = 0b100;
      Size = 0b00;
    }
    else if (size == SubRegSize::i64Bit) {
      LOGMAN_THROW_AA_FMT(Index < 2, "Index too large");
      Q = Index;
      S = 0;
      opcode = 0b101;
      Size = 0b01;
    }
    else {
      LOGMAN_MSG_A_FMT("Unknown size");
      FEX_UNREACHABLE;
    }

    ASIMDLoadStoreSinglePost(Op, Q, 1, R, opcode, S, Size, rm, rn, rt.Q());
  }
  template<typename T>
  void ld1r(FEXCore::ARMEmitter::SubRegSize size, T rt, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    constexpr uint32_t Op = 0b0000'1101'1 << 23;
    constexpr uint32_t Q = std::is_same_v<FEXCore::ARMEmitter::QRegister, T> ? 1 : 0;
    uint32_t R = 0;
    uint32_t opcode = 0b110;
    uint32_t S = 0;
    uint32_t Size = FEXCore::ToUnderlying(size);
    ASIMDLoadStoreSinglePost<T>(Op, Q, 1, R, opcode, S, Size, rm, rn, rt);
  }

  template<typename T>
  void ld2r(FEXCore::ARMEmitter::SubRegSize size, T rt, T rt2, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    constexpr uint32_t Op = 0b0000'1101'1 << 23;
    constexpr uint32_t Q = std::is_same_v<FEXCore::ARMEmitter::QRegister, T> ? 1 : 0;
    uint32_t R = 1;
    uint32_t opcode = 0b110;
    uint32_t S = 0;
    uint32_t Size = FEXCore::ToUnderlying(size);
    ASIMDLoadStoreSinglePost<T>(Op, Q, 1, R, opcode, S, Size, rm, rn, rt);
  }

  template<typename T>
  void ld3r(FEXCore::ARMEmitter::SubRegSize size, T rt, T rt2, T rt3, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt2.Idx() + 1) == rt3.Idx(), "These must be sequential");
    constexpr uint32_t Op = 0b0000'1101'1 << 23;
    constexpr uint32_t Q = std::is_same_v<FEXCore::ARMEmitter::QRegister, T> ? 1 : 0;
    uint32_t R = 0;
    uint32_t opcode = 0b111;
    uint32_t S = 0;
    uint32_t Size = FEXCore::ToUnderlying(size);
    ASIMDLoadStoreSinglePost<T>(Op, Q, 1, R, opcode, S, Size, rm, rn, rt);
  }
  template<typename T>
  void ld4r(FEXCore::ARMEmitter::SubRegSize size, T rt, T rt2, T rt3, T rt4, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt2.Idx() + 1) == rt3.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt3.Idx() + 1) == rt4.Idx(), "These must be sequential");
    constexpr uint32_t Op = 0b0000'1101'1 << 23;
    constexpr uint32_t Q = std::is_same_v<FEXCore::ARMEmitter::QRegister, T> ? 1 : 0;
    uint32_t R = 1;
    uint32_t opcode = 0b111;
    uint32_t S = 0;
    uint32_t Size = FEXCore::ToUnderlying(size);
    ASIMDLoadStoreSinglePost<T>(Op, Q, 1, R, opcode, S, Size, rm, rn, rt);
  }

  template<typename T>
  void st2(FEXCore::ARMEmitter::SubRegSize size, T rt, T rt2, uint32_t Index, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    LOGMAN_THROW_AA_FMT(size == SubRegSize::i8Bit || size == SubRegSize::i16Bit || size == SubRegSize::i32Bit || size == SubRegSize::i64Bit, "Incorrect size");
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");

    constexpr uint32_t Op = 0b0000'1101'1 << 23;
    uint32_t Q;
    uint32_t R = 1;
    uint32_t opcode;
    uint32_t S;
    uint32_t Size;
    if (size == SubRegSize::i8Bit) {
      LOGMAN_THROW_AA_FMT(Index < 16, "Index too large");
      Q = Index >> 3;
      S = (Index >> 2) & 1;
      opcode = 0b000;
      Size = Index & 0b11;
    }
    else if (size == SubRegSize::i16Bit) {
      LOGMAN_THROW_AA_FMT(Index < 8, "Index too large");
      Q = Index >> 2;
      S = (Index >> 1) & 1;
      opcode = 0b010;
      Size = (Index & 0b1) << 1;
    }
    else if (size == SubRegSize::i32Bit) {
      LOGMAN_THROW_AA_FMT(Index < 4, "Index too large");
      Q = Index >> 1;
      S = Index & 1;
      opcode = 0b100;
      Size = 0b00;
    }
    else if (size == SubRegSize::i64Bit) {
      LOGMAN_THROW_AA_FMT(Index < 2, "Index too large");
      Q = Index;
      S = 0;
      opcode = 0b100;
      Size = 0b01;
    }
    else {
      LOGMAN_MSG_A_FMT("Unknown size");
      FEX_UNREACHABLE;
    }

    ASIMDLoadStoreSinglePost(Op, Q, 0, R, opcode, S, Size, rm, rn, rt.Q());
  }
  template<typename T>
  void ld2(FEXCore::ARMEmitter::SubRegSize size, T rt, T rt2, uint32_t Index, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    LOGMAN_THROW_AA_FMT(size == SubRegSize::i8Bit || size == SubRegSize::i16Bit || size == SubRegSize::i32Bit || size == SubRegSize::i64Bit, "Incorrect size");
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");

    constexpr uint32_t Op = 0b0000'1101'1 << 23;
    uint32_t Q;
    uint32_t R = 1;
    uint32_t opcode;
    uint32_t S;
    uint32_t Size;
    if (size == SubRegSize::i8Bit) {
      LOGMAN_THROW_AA_FMT(Index < 16, "Index too large");
      Q = Index >> 3;
      S = (Index >> 2) & 1;
      opcode = 0b000;
      Size = Index & 0b11;
    }
    else if (size == SubRegSize::i16Bit) {
      LOGMAN_THROW_AA_FMT(Index < 8, "Index too large");
      Q = Index >> 2;
      S = (Index >> 1) & 1;
      opcode = 0b010;
      Size = (Index & 0b1) << 1;
    }
    else if (size == SubRegSize::i32Bit) {
      LOGMAN_THROW_AA_FMT(Index < 4, "Index too large");
      Q = Index >> 1;
      S = Index & 1;
      opcode = 0b100;
      Size = 0b00;
    }
    else if (size == SubRegSize::i64Bit) {
      LOGMAN_THROW_AA_FMT(Index < 2, "Index too large");
      Q = Index;
      S = 0;
      opcode = 0b100;
      Size = 0b01;
    }
    else {
      LOGMAN_MSG_A_FMT("Unknown size");
      FEX_UNREACHABLE;
    }

    ASIMDLoadStoreSinglePost(Op, Q, 1, R, opcode, S, Size, rm, rn, rt.Q());
  }
  template<typename T>
  void st3(FEXCore::ARMEmitter::SubRegSize size, T rt, T rt2, T rt3, uint32_t Index, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    LOGMAN_THROW_AA_FMT(size == SubRegSize::i8Bit || size == SubRegSize::i16Bit || size == SubRegSize::i32Bit || size == SubRegSize::i64Bit, "Incorrect size");
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt2.Idx() + 1) == rt3.Idx(), "These must be sequential");

    constexpr uint32_t Op = 0b0000'1101'1 << 23;
    uint32_t Q;
    uint32_t R = 0;
    uint32_t opcode;
    uint32_t S;
    uint32_t Size;
    if (size == SubRegSize::i8Bit) {
      LOGMAN_THROW_AA_FMT(Index < 16, "Index too large");
      Q = Index >> 3;
      S = (Index >> 2) & 1;
      opcode = 0b001;
      Size = Index & 0b11;
    }
    else if (size == SubRegSize::i16Bit) {
      LOGMAN_THROW_AA_FMT(Index < 8, "Index too large");
      Q = Index >> 2;
      S = (Index >> 1) & 1;
      opcode = 0b011;
      Size = (Index & 0b1) << 1;
    }
    else if (size == SubRegSize::i32Bit) {
      LOGMAN_THROW_AA_FMT(Index < 4, "Index too large");
      Q = Index >> 1;
      S = Index & 1;
      opcode = 0b101;
      Size = 0b00;
    }
    else if (size == SubRegSize::i64Bit) {
      LOGMAN_THROW_AA_FMT(Index < 2, "Index too large");
      Q = Index;
      S = 0;
      opcode = 0b101;
      Size = 0b01;
    }
    else {
      LOGMAN_MSG_A_FMT("Unknown size");
      FEX_UNREACHABLE;
    }

    ASIMDLoadStoreSinglePost(Op, Q, 0, R, opcode, S, Size, rm, rn, rt.Q());
  }
  template<typename T>
  void ld3(FEXCore::ARMEmitter::SubRegSize size, T rt, T rt2, T rt3, uint32_t Index, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    LOGMAN_THROW_AA_FMT(size == SubRegSize::i8Bit || size == SubRegSize::i16Bit || size == SubRegSize::i32Bit || size == SubRegSize::i64Bit, "Incorrect size");
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt2.Idx() + 1) == rt3.Idx(), "These must be sequential");

    constexpr uint32_t Op = 0b0000'1101'1 << 23;
    uint32_t Q;
    uint32_t R = 0;
    uint32_t opcode;
    uint32_t S;
    uint32_t Size;
    if (size == SubRegSize::i8Bit) {
      LOGMAN_THROW_AA_FMT(Index < 16, "Index too large");
      Q = Index >> 3;
      S = (Index >> 2) & 1;
      opcode = 0b001;
      Size = Index & 0b11;
    }
    else if (size == SubRegSize::i16Bit) {
      LOGMAN_THROW_AA_FMT(Index < 8, "Index too large");
      Q = Index >> 2;
      S = (Index >> 1) & 1;
      opcode = 0b011;
      Size = (Index & 0b1) << 1;
    }
    else if (size == SubRegSize::i32Bit) {
      LOGMAN_THROW_AA_FMT(Index < 4, "Index too large");
      Q = Index >> 1;
      S = Index & 1;
      opcode = 0b101;
      Size = 0b00;
    }
    else if (size == SubRegSize::i64Bit) {
      LOGMAN_THROW_AA_FMT(Index < 2, "Index too large");
      Q = Index;
      S = 0;
      opcode = 0b101;
      Size = 0b01;
    }
    else {
      LOGMAN_MSG_A_FMT("Unknown size");
      FEX_UNREACHABLE;
    }

    ASIMDLoadStoreSinglePost(Op, Q, 1, R, opcode, S, Size, rm, rn, rt.Q());
  }
  template<typename T>
  void st4(FEXCore::ARMEmitter::SubRegSize size, T rt, T rt2, T rt3, T rt4, uint32_t Index, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    LOGMAN_THROW_AA_FMT(size == SubRegSize::i8Bit || size == SubRegSize::i16Bit || size == SubRegSize::i32Bit || size == SubRegSize::i64Bit, "Incorrect size");
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt2.Idx() + 1) == rt3.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt3.Idx() + 1) == rt4.Idx(), "These must be sequential");

    constexpr uint32_t Op = 0b0000'1101'1 << 23;
    uint32_t Q;
    uint32_t R = 1;
    uint32_t opcode;
    uint32_t S;
    uint32_t Size;
    if (size == SubRegSize::i8Bit) {
      LOGMAN_THROW_AA_FMT(Index < 16, "Index too large");
      Q = Index >> 3;
      S = (Index >> 2) & 1;
      opcode = 0b001;
      Size = Index & 0b11;
    }
    else if (size == SubRegSize::i16Bit) {
      LOGMAN_THROW_AA_FMT(Index < 8, "Index too large");
      Q = Index >> 2;
      S = (Index >> 1) & 1;
      opcode = 0b011;
      Size = (Index & 0b1) << 1;
    }
    else if (size == SubRegSize::i32Bit) {
      LOGMAN_THROW_AA_FMT(Index < 4, "Index too large");
      Q = Index >> 1;
      S = Index & 1;
      opcode = 0b101;
      Size = 0b00;
    }
    else if (size == SubRegSize::i64Bit) {
      LOGMAN_THROW_AA_FMT(Index < 2, "Index too large");
      Q = Index;
      S = 0;
      opcode = 0b101;
      Size = 0b01;
    }
    else {
      LOGMAN_MSG_A_FMT("Unknown size");
      FEX_UNREACHABLE;
    }

    ASIMDLoadStoreSinglePost(Op, Q, 0, R, opcode, S, Size, rm, rn, rt.Q());
  }
  template<typename T>
  void ld4(FEXCore::ARMEmitter::SubRegSize size, T rt, T rt2, T rt3, T rt4, uint32_t Index, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    LOGMAN_THROW_AA_FMT(size == SubRegSize::i8Bit || size == SubRegSize::i16Bit || size == SubRegSize::i32Bit || size == SubRegSize::i64Bit, "Incorrect size");
    LOGMAN_THROW_A_FMT((rt.Idx() + 1) == rt2.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt2.Idx() + 1) == rt3.Idx(), "These must be sequential");
    LOGMAN_THROW_A_FMT((rt3.Idx() + 1) == rt4.Idx(), "These must be sequential");

    constexpr uint32_t Op = 0b0000'1101'1 << 23;
    uint32_t Q;
    uint32_t R = 1;
    uint32_t opcode;
    uint32_t S;
    uint32_t Size;
    if (size == SubRegSize::i8Bit) {
      LOGMAN_THROW_AA_FMT(Index < 16, "Index too large");
      Q = Index >> 3;
      S = (Index >> 2) & 1;
      opcode = 0b001;
      Size = Index & 0b11;
    }
    else if (size == SubRegSize::i16Bit) {
      LOGMAN_THROW_AA_FMT(Index < 8, "Index too large");
      Q = Index >> 2;
      S = (Index >> 1) & 1;
      opcode = 0b011;
      Size = (Index & 0b1) << 1;
    }
    else if (size == SubRegSize::i32Bit) {
      LOGMAN_THROW_AA_FMT(Index < 4, "Index too large");
      Q = Index >> 1;
      S = Index & 1;
      opcode = 0b101;
      Size = 0b00;
    }
    else if (size == SubRegSize::i64Bit) {
      LOGMAN_THROW_AA_FMT(Index < 2, "Index too large");
      Q = Index;
      S = 0;
      opcode = 0b101;
      Size = 0b01;
    }
    else {
      LOGMAN_MSG_A_FMT("Unknown size");
      FEX_UNREACHABLE;
    }

    ASIMDLoadStoreSinglePost(Op, Q, 1, R, opcode, S, Size, rm, rn, rt.Q());
  }

  template<FEXCore::ARMEmitter::SubRegSize size, typename T>
  void st1(T rt, uint32_t Index, FEXCore::ARMEmitter::Register rn, uint32_t PostOffset) {
    st1(size, rt, Index, rn, PostOffset);
  }
  template<FEXCore::ARMEmitter::SubRegSize size, typename T>
  void ld1(T rt, uint32_t Index, FEXCore::ARMEmitter::Register rn, uint32_t PostOffset) {
    ld1(size, rt, Index, rn, PostOffset);
  }
  template<FEXCore::ARMEmitter::SubRegSize size, typename T>
  void ld1r(T rt, FEXCore::ARMEmitter::Register rn, uint32_t PostOffset) {
    ld1r(size, rt, rn, PostOffset);
  }
  template<FEXCore::ARMEmitter::SubRegSize size, typename T>
  void ld2r(T rt, T rt2, FEXCore::ARMEmitter::Register rn, uint32_t PostOffset) {
    ld2r(size, rt, rt2, rn, PostOffset);
  }
  template<FEXCore::ARMEmitter::SubRegSize size, typename T>
  void ld3r(T rt, T rt2, T rt3, FEXCore::ARMEmitter::Register rn, uint32_t PostOffset) {
    ld3r(size, rt, rt2, rt3, rn, PostOffset);
  }
  template<FEXCore::ARMEmitter::SubRegSize size, typename T>
  void ld4r(T rt, T rt2, T rt3, T rt4, FEXCore::ARMEmitter::Register rn, uint32_t PostOffset) {
    ld4r(size, rt, rt2, rt3, rt4, rn, PostOffset);
  }

  template<FEXCore::ARMEmitter::SubRegSize size, typename T>
  void st2(T rt, T rt2, uint32_t Index, FEXCore::ARMEmitter::Register rn, uint32_t PostOffset) {
    st2(size, rt, rt2, Index, rn, PostOffset);
  }
  template<FEXCore::ARMEmitter::SubRegSize size, typename T>
  void ld2(T rt, T rt2, uint32_t Index, FEXCore::ARMEmitter::Register rn, uint32_t PostOffset) {
    ld2(size, rt, rt2, Index, rn, PostOffset);
  }
  template<FEXCore::ARMEmitter::SubRegSize size, typename T>
  void st3(T rt, T rt2, T rt3, uint32_t Index, FEXCore::ARMEmitter::Register rn, uint32_t PostOffset) {
    st3(size, rt, rt2, rt3, Index, rn, PostOffset);
  }
  template<FEXCore::ARMEmitter::SubRegSize size, typename T>
  void ld3(T rt, T rt2, T rt3, uint32_t Index, FEXCore::ARMEmitter::Register rn, uint32_t PostOffset) {
    ld3(size, rt, rt2, rt3, Index, rn, PostOffset);
  }
  template<FEXCore::ARMEmitter::SubRegSize size, typename T>
  void st4(T rt, T rt2, T rt3, T rt4, uint32_t Index, FEXCore::ARMEmitter::Register rn, uint32_t PostOffset) {
    st4(size, rt, rt2, rt3, rt4, Index, rn, PostOffset);
  }
  template<FEXCore::ARMEmitter::SubRegSize size, typename T>
  void ld4(T rt, T rt2, T rt3, T rt4, uint32_t Index, FEXCore::ARMEmitter::Register rn, uint32_t PostOffset) {
    ld4(size, rt, rt2, rt3, rt4, Index, rn, PostOffset);
  }

  template<FEXCore::ARMEmitter::SubRegSize size, typename T>
  void st1(T rt, uint32_t Index, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    st1(size, rt, Index, rn, rm);
  }
  template<FEXCore::ARMEmitter::SubRegSize size, typename T>
  void ld1(T rt, uint32_t Index, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    ld1(size, rt, Index, rn, rm);
  }
  template<FEXCore::ARMEmitter::SubRegSize size, typename T>
  void ld1r(T rt, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    ld1r(size, rt, rn, rm);
  }
  template<FEXCore::ARMEmitter::SubRegSize size, typename T>
  void ld2r(T rt, T rt2, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    ld2r(size, rt, rt2, rn, rm);
  }
  template<FEXCore::ARMEmitter::SubRegSize size, typename T>
  void ld3r(T rt, T rt2, T rt3, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    ld3r(size, rt, rt2, rt3, rn, rm);
  }
  template<FEXCore::ARMEmitter::SubRegSize size, typename T>
  void ld4r(T rt, T rt2, T rt3, T rt4, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    ld4r(size, rt, rt2, rt3, rt4, rn, rm);
  }

  template<FEXCore::ARMEmitter::SubRegSize size, typename T>
  void st2(T rt, T rt2, uint32_t Index, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    st2(size, rt, rt2, Index, rn, rm);
  }
  template<FEXCore::ARMEmitter::SubRegSize size, typename T>
  void ld2(T rt, T rt2, uint32_t Index, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    ld2(size, rt, rt2, Index, rn, rm);
  }
  template<FEXCore::ARMEmitter::SubRegSize size, typename T>
  void st3(T rt, T rt2, T rt3, uint32_t Index, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    st3(size, rt, rt2, rt3, Index, rn, rm);
  }
  template<FEXCore::ARMEmitter::SubRegSize size, typename T>
  void ld3(T rt, T rt2, T rt3, uint32_t Index, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    ld3(size, rt, rt2, rt3, Index, rn, rm);
  }
  template<FEXCore::ARMEmitter::SubRegSize size, typename T>
  void st4(T rt, T rt2, T rt3, T rt4, uint32_t Index, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    st4(size, rt, rt2, rt3, rt4, Index, rn, rm);
  }
  template<FEXCore::ARMEmitter::SubRegSize size, typename T>
  void ld4(T rt, T rt2, T rt3, T rt4, uint32_t Index, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm) {
    ld4(size, rt, rt2, rt3, rt4, Index, rn, rm);
  }

  template<typename T>
  void ASIMDLoadStoreSinglePost(uint32_t Op, uint32_t Q, uint32_t L, uint32_t R, uint32_t opcode, uint32_t S, uint32_t size, FEXCore::ARMEmitter::Register rm, FEXCore::ARMEmitter::Register rn, T rt) {
    LOGMAN_THROW_A_FMT(std::is_same_v<FEXCore::ARMEmitter::QRegister, T> || std::is_same_v<FEXCore::ARMEmitter::DRegister, T>, "Only supports 128-bit and 64-bit vector registers.");
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
  void stxp(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rt2, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b1000'1000'001 << 21;
    AtomicOp(Op, s, 0, 0, rs, rt, rt2, rn);
  }
  void stlxp(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rt2, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b1000'1000'001 << 21;
    AtomicOp(Op, s, 0, 1, rs, rt, rt2, rn);
  }
  void ldxp(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rt2, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b1000'1000'001 << 21;
    AtomicOp(Op, s, 1, 0, FEXCore::ARMEmitter::Reg::r31, rt, rt2, rn);
  }
  void ldaxp(FEXCore::ARMEmitter::Size s, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rt2, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b1000'1000'001 << 21;
    AtomicOp(Op, s, 1, 1, FEXCore::ARMEmitter::Reg::r31, rt, rt2, rn);
  }
  // Loadstore exclusive register
  void stxrb(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'000 << 21;
    SubAtomicOp(Op, FEXCore::ARMEmitter::SubRegSize::i8Bit, 0, 0, rs, rt, FEXCore::ARMEmitter::Reg::r31, rn);
  }
  void stlxrb(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'000 << 21;
    SubAtomicOp(Op, FEXCore::ARMEmitter::SubRegSize::i8Bit, 0, 1, rs, rt, FEXCore::ARMEmitter::Reg::r31, rn);
  }
  void ldxrb(FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'000 << 21;
    SubAtomicOp(Op, FEXCore::ARMEmitter::SubRegSize::i8Bit, 1, 0, FEXCore::ARMEmitter::Reg::r31, rt, FEXCore::ARMEmitter::Reg::r31, rn);
  }
  void ldaxrb(FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'000 << 21;
    SubAtomicOp(Op, FEXCore::ARMEmitter::SubRegSize::i8Bit, 1, 1, FEXCore::ARMEmitter::Reg::r31, rt, FEXCore::ARMEmitter::Reg::r31, rn);
  }
  void stxrh(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'000 << 21;
    SubAtomicOp(Op, FEXCore::ARMEmitter::SubRegSize::i16Bit, 0, 0, rs, rt, FEXCore::ARMEmitter::Reg::r31, rn);
  }
  void stlxrh(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'000 << 21;
    SubAtomicOp(Op, FEXCore::ARMEmitter::SubRegSize::i16Bit, 0, 1, rs, rt, FEXCore::ARMEmitter::Reg::r31, rn);
  }
  void ldxrh(FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'000 << 21;
    SubAtomicOp(Op, FEXCore::ARMEmitter::SubRegSize::i16Bit, 1, 0, FEXCore::ARMEmitter::Reg::r31, rt, FEXCore::ARMEmitter::Reg::r31, rn);
  }
  void ldaxrh(FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'000 << 21;
    SubAtomicOp(Op, FEXCore::ARMEmitter::SubRegSize::i16Bit, 1, 1, FEXCore::ARMEmitter::Reg::r31, rt, FEXCore::ARMEmitter::Reg::r31, rn);
  }
  void stxr(FEXCore::ARMEmitter::WRegister rs, FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'000 << 21;
    SubAtomicOp(Op, FEXCore::ARMEmitter::SubRegSize::i32Bit, 0, 0, rs, rt, FEXCore::ARMEmitter::WReg::w31, rn);
  }
  void stlxr(FEXCore::ARMEmitter::WRegister rs, FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'000 << 21;
    SubAtomicOp(Op, FEXCore::ARMEmitter::SubRegSize::i32Bit, 0, 1, rs, rt, FEXCore::ARMEmitter::WReg::w31, rn);
  }
  void ldxr(FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'000 << 21;
    SubAtomicOp(Op, FEXCore::ARMEmitter::SubRegSize::i32Bit, 1, 0, FEXCore::ARMEmitter::WReg::w31, rt, FEXCore::ARMEmitter::WReg::w31, rn);
  }
  void ldaxr(FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'000 << 21;
    SubAtomicOp(Op, FEXCore::ARMEmitter::SubRegSize::i32Bit, 1, 1, FEXCore::ARMEmitter::WReg::w31, rt, FEXCore::ARMEmitter::WReg::w31, rn);
  }
  void stxr(FEXCore::ARMEmitter::XRegister rs, FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'000 << 21;
    SubAtomicOp(Op, FEXCore::ARMEmitter::SubRegSize::i64Bit, 0, 0, rs, rt, FEXCore::ARMEmitter::XReg::x31, rn);
  }
  void stlxr(FEXCore::ARMEmitter::WRegister rs, FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'000 << 21;
    SubAtomicOp(Op, FEXCore::ARMEmitter::SubRegSize::i64Bit, 0, 1, rs.R(), rt.R(), FEXCore::ARMEmitter::Reg::r31, rn);
  }
  void ldxr(FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'000 << 21;
    SubAtomicOp(Op, FEXCore::ARMEmitter::SubRegSize::i64Bit, 1, 0, FEXCore::ARMEmitter::XReg::x31, rt, FEXCore::ARMEmitter::XReg::x31, rn);
  }
  void ldaxr(FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'000 << 21;
    SubAtomicOp(Op, FEXCore::ARMEmitter::SubRegSize::i64Bit, 1, 1, FEXCore::ARMEmitter::XReg::x31, rt, FEXCore::ARMEmitter::XReg::x31, rn);
  }
  void stxr(ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'000 << 21;
    SubAtomicOp(Op, size, 0, 0, rs, rt, FEXCore::ARMEmitter::Reg::r31, rn);
  }
  void stlxr(ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'000 << 21;
    SubAtomicOp(Op, size, 0, 1, rs, rt, FEXCore::ARMEmitter::Reg::r31, rn);
  }
  void ldxr(ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'000 << 21;
    SubAtomicOp(Op, size, 1, 0, FEXCore::ARMEmitter::Reg::r31, rt, FEXCore::ARMEmitter::Reg::r31, rn);
  }
  void ldaxr(ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'000 << 21;
    SubAtomicOp(Op, size, 1, 1, FEXCore::ARMEmitter::Reg::r31, rt, FEXCore::ARMEmitter::Reg::r31, rn);
  }

  // Load/store ordered
  static constexpr uint32_t LoadStoreOrdered_Op = 0b0000'1000'100 << 21;
  void stllrb(FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    SubAtomicOp(LoadStoreOrdered_Op, FEXCore::ARMEmitter::SubRegSize::i8Bit, 0, 0, FEXCore::ARMEmitter::Reg::r31, rt, FEXCore::ARMEmitter::Reg::r31, rn);
  }
  void stlrb(FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    SubAtomicOp(LoadStoreOrdered_Op, FEXCore::ARMEmitter::SubRegSize::i8Bit, 0, 1, FEXCore::ARMEmitter::Reg::r31, rt, FEXCore::ARMEmitter::Reg::r31, rn);
  }
  void ldlarb(FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    SubAtomicOp(LoadStoreOrdered_Op, FEXCore::ARMEmitter::SubRegSize::i8Bit, 1, 0, FEXCore::ARMEmitter::Reg::r31, rt, FEXCore::ARMEmitter::Reg::r31, rn);
  }
  void ldarb(FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    SubAtomicOp(LoadStoreOrdered_Op, FEXCore::ARMEmitter::SubRegSize::i8Bit, 1, 1, FEXCore::ARMEmitter::Reg::r31, rt, FEXCore::ARMEmitter::Reg::r31, rn);
  }
  void stllrh(FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    SubAtomicOp(LoadStoreOrdered_Op, FEXCore::ARMEmitter::SubRegSize::i16Bit, 0, 0, FEXCore::ARMEmitter::Reg::r31, rt, FEXCore::ARMEmitter::Reg::r31, rn);
  }
  void stlrh(FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    SubAtomicOp(LoadStoreOrdered_Op, FEXCore::ARMEmitter::SubRegSize::i16Bit, 0, 1, FEXCore::ARMEmitter::Reg::r31, rt, FEXCore::ARMEmitter::Reg::r31, rn);
  }
  void ldlarh(FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    SubAtomicOp(LoadStoreOrdered_Op, FEXCore::ARMEmitter::SubRegSize::i16Bit, 1, 0, FEXCore::ARMEmitter::Reg::r31, rt, FEXCore::ARMEmitter::Reg::r31, rn);
  }
  void ldarh(FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    SubAtomicOp(LoadStoreOrdered_Op, FEXCore::ARMEmitter::SubRegSize::i16Bit, 1, 1, FEXCore::ARMEmitter::Reg::r31, rt, FEXCore::ARMEmitter::Reg::r31, rn);
  }
  void stllr(FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn) {
    SubAtomicOp(LoadStoreOrdered_Op, FEXCore::ARMEmitter::SubRegSize::i32Bit, 0, 0, FEXCore::ARMEmitter::WReg::w31, rt, FEXCore::ARMEmitter::WReg::w31, rn);
  }
  void stlr(FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn) {
    SubAtomicOp(LoadStoreOrdered_Op, FEXCore::ARMEmitter::SubRegSize::i32Bit, 0, 1, FEXCore::ARMEmitter::WReg::w31, rt, FEXCore::ARMEmitter::WReg::w31, rn);
  }
  void ldlar(FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn) {
    SubAtomicOp(LoadStoreOrdered_Op, FEXCore::ARMEmitter::SubRegSize::i32Bit, 1, 0, FEXCore::ARMEmitter::WReg::w31, rt, FEXCore::ARMEmitter::WReg::w31, rn);
  }
  void ldar(FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn) {
    SubAtomicOp(LoadStoreOrdered_Op, FEXCore::ARMEmitter::SubRegSize::i32Bit, 1, 1, FEXCore::ARMEmitter::WReg::w31, rt, FEXCore::ARMEmitter::WReg::w31, rn);
  }
  void stllr(FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn) {
    SubAtomicOp(LoadStoreOrdered_Op, FEXCore::ARMEmitter::SubRegSize::i64Bit, 0, 0, FEXCore::ARMEmitter::XReg::x31, rt, FEXCore::ARMEmitter::XReg::x31, rn);
  }
  void stlr(FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn) {
    SubAtomicOp(LoadStoreOrdered_Op, FEXCore::ARMEmitter::SubRegSize::i64Bit, 0, 1, FEXCore::ARMEmitter::XReg::x31, rt, FEXCore::ARMEmitter::XReg::x31, rn);
  }
  void ldlar(FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn) {
    SubAtomicOp(LoadStoreOrdered_Op, FEXCore::ARMEmitter::SubRegSize::i64Bit, 1, 0, FEXCore::ARMEmitter::XReg::x31, rt, FEXCore::ARMEmitter::XReg::x31, rn);
  }
  void ldar(FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn) {
    SubAtomicOp(LoadStoreOrdered_Op, FEXCore::ARMEmitter::SubRegSize::i64Bit, 1, 1, FEXCore::ARMEmitter::XReg::x31, rt, FEXCore::ARMEmitter::XReg::x31, rn);
  }
  // Compare and swap
  void casb(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'101 << 21;
    SubAtomicOp(Op, FEXCore::ARMEmitter::SubRegSize::i8Bit, 0, 0, rs, rt, FEXCore::ARMEmitter::Reg::r31, rn);
  }
  void caslb(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'101 << 21;
    SubAtomicOp(Op, FEXCore::ARMEmitter::SubRegSize::i8Bit, 0, 1, rs, rt, FEXCore::ARMEmitter::Reg::r31, rn);
  }
  void casab(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'101 << 21;
    SubAtomicOp(Op, FEXCore::ARMEmitter::SubRegSize::i8Bit, 1, 0, rs, rt, FEXCore::ARMEmitter::Reg::r31, rn);
  }
  void casalb(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'101 << 21;
    SubAtomicOp(Op, FEXCore::ARMEmitter::SubRegSize::i8Bit, 1, 1, rs, rt, FEXCore::ARMEmitter::Reg::r31, rn);
  }
  void cash(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'101 << 21;
    SubAtomicOp(Op, FEXCore::ARMEmitter::SubRegSize::i16Bit, 0, 0, rs, rt, FEXCore::ARMEmitter::Reg::r31, rn);
  }
  void caslh(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'101 << 21;
    SubAtomicOp(Op, FEXCore::ARMEmitter::SubRegSize::i16Bit, 0, 1, rs, rt, FEXCore::ARMEmitter::Reg::r31, rn);
  }
  void casah(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'101 << 21;
    SubAtomicOp(Op, FEXCore::ARMEmitter::SubRegSize::i16Bit, 1, 0, rs, rt, FEXCore::ARMEmitter::Reg::r31, rn);
  }
  void casalh(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'101 << 21;
    SubAtomicOp(Op, FEXCore::ARMEmitter::SubRegSize::i16Bit, 1, 1, rs, rt, FEXCore::ARMEmitter::Reg::r31, rn);
  }
  void cas(FEXCore::ARMEmitter::WRegister rs, FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'101 << 21;
    SubAtomicOp(Op, FEXCore::ARMEmitter::SubRegSize::i32Bit, 0, 0, rs.R(), rt.R(), FEXCore::ARMEmitter::Reg::r31, rn);
  }
  void casl(FEXCore::ARMEmitter::WRegister rs, FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'101 << 21;
    SubAtomicOp(Op, FEXCore::ARMEmitter::SubRegSize::i32Bit, 0, 1, rs.R(), rt.R(), FEXCore::ARMEmitter::Reg::r31, rn);
  }
  void casa(FEXCore::ARMEmitter::WRegister rs, FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'101 << 21;
    SubAtomicOp(Op, FEXCore::ARMEmitter::SubRegSize::i32Bit, 1, 0, rs.R(), rt.R(), FEXCore::ARMEmitter::Reg::r31, rn);
  }
  void casal(FEXCore::ARMEmitter::WRegister rs, FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'101 << 21;
    SubAtomicOp(Op, FEXCore::ARMEmitter::SubRegSize::i32Bit, 1, 1, rs.R(), rt.R(), FEXCore::ARMEmitter::Reg::r31, rn);
  }
  void cas(FEXCore::ARMEmitter::XRegister rs, FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'101 << 21;
    SubAtomicOp(Op, FEXCore::ARMEmitter::SubRegSize::i64Bit, 0, 0, rs.R(), rt.R(), FEXCore::ARMEmitter::Reg::r31, rn);
  }
  void casl(FEXCore::ARMEmitter::XRegister rs, FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'101 << 21;
    SubAtomicOp(Op, FEXCore::ARMEmitter::SubRegSize::i64Bit, 0, 1, rs.R(), rt.R(), FEXCore::ARMEmitter::Reg::r31, rn);
  }
  void casa(FEXCore::ARMEmitter::XRegister rs, FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'101 << 21;
    SubAtomicOp(Op, FEXCore::ARMEmitter::SubRegSize::i64Bit, 1, 0, rs.R(), rt.R(), FEXCore::ARMEmitter::Reg::r31, rn);
  }
  void casal(FEXCore::ARMEmitter::XRegister rs, FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'101 << 21;
    SubAtomicOp(Op, FEXCore::ARMEmitter::SubRegSize::i64Bit, 1, 1, rs.R(), rt.R(), FEXCore::ARMEmitter::Reg::r31, rn);
  }

  void cas(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'101 << 21;
    SubAtomicOp(Op, size, 0, 0, rs, rt, FEXCore::ARMEmitter::Reg::r31, rn);
  }
  void casl(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'101 << 21;
    SubAtomicOp(Op, size, 0, 1, rs, rt, FEXCore::ARMEmitter::Reg::r31, rn);
  }
  void casa(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'101 << 21;
    SubAtomicOp(Op, size, 1, 0, rs, rt, FEXCore::ARMEmitter::Reg::r31, rn);
  }
  void casal(FEXCore::ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0000'1000'101 << 21;
    SubAtomicOp(Op, size, 1, 1, rs, rt, FEXCore::ARMEmitter::Reg::r31, rn);
  }
  // LDAPR/STLR unscaled immediate
  void stlurb(FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");
    constexpr uint32_t Op = 0b0001'1001'000 << 21;
    SubAtomicImm(Op, FEXCore::ARMEmitter::SubRegSize::i8Bit, 0b00, rt, rn, static_cast<uint32_t>(Imm) & 0x1'FF);
  }
  void ldapurb(FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");
    constexpr uint32_t Op = 0b0001'1001'000 << 21;
    SubAtomicImm(Op, FEXCore::ARMEmitter::SubRegSize::i8Bit, 0b01, rt, rn, static_cast<uint32_t>(Imm) & 0x1'FF);
  }
  void ldapursb(FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");
    constexpr uint32_t Op = 0b0001'1001'000 << 21;
    SubAtomicImm(Op, FEXCore::ARMEmitter::SubRegSize::i8Bit, 0b11, rt, rn, static_cast<uint32_t>(Imm) & 0x1'FF);
  }
  void ldapursb(FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");
    constexpr uint32_t Op = 0b0001'1001'000 << 21;
    SubAtomicImm(Op, FEXCore::ARMEmitter::SubRegSize::i8Bit, 0b10, rt, rn, static_cast<uint32_t>(Imm) & 0x1'FF);
  }
  void stlurh(FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");
    constexpr uint32_t Op = 0b0001'1001'000 << 21;
    SubAtomicImm(Op, FEXCore::ARMEmitter::SubRegSize::i16Bit, 0b00, rt, rn, static_cast<uint32_t>(Imm) & 0x1'FF);
  }
  void ldapurh(FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");
    constexpr uint32_t Op = 0b0001'1001'000 << 21;
    SubAtomicImm(Op, FEXCore::ARMEmitter::SubRegSize::i16Bit, 0b01, rt, rn, static_cast<uint32_t>(Imm) & 0x1'FF);
  }
  void ldapursh(FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");
    constexpr uint32_t Op = 0b0001'1001'000 << 21;
    SubAtomicImm(Op, FEXCore::ARMEmitter::SubRegSize::i16Bit, 0b11, rt, rn, static_cast<uint32_t>(Imm) & 0x1'FF);
  }
  void ldapursh(FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");
    constexpr uint32_t Op = 0b0001'1001'000 << 21;
    SubAtomicImm(Op, FEXCore::ARMEmitter::SubRegSize::i16Bit, 0b10, rt, rn, static_cast<uint32_t>(Imm) & 0x1'FF);
  }
  void stlur(FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");
    constexpr uint32_t Op = 0b0001'1001'000 << 21;
    SubAtomicImm(Op, FEXCore::ARMEmitter::SubRegSize::i32Bit, 0b00, rt, rn, static_cast<uint32_t>(Imm) & 0x1'FF);
  }
  void ldapur(FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");
    constexpr uint32_t Op = 0b0001'1001'000 << 21;
    SubAtomicImm(Op, FEXCore::ARMEmitter::SubRegSize::i32Bit, 0b01, rt, rn, static_cast<uint32_t>(Imm) & 0x1'FF);
  }
  void ldapursw(FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");
    constexpr uint32_t Op = 0b0001'1001'000 << 21;
    SubAtomicImm(Op, FEXCore::ARMEmitter::SubRegSize::i32Bit, 0b10, rt, rn, static_cast<uint32_t>(Imm) & 0x1'FF);
  }
  void stlur(FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");
    constexpr uint32_t Op = 0b0001'1001'000 << 21;
    SubAtomicImm(Op, FEXCore::ARMEmitter::SubRegSize::i64Bit, 0b00, rt, rn, static_cast<uint32_t>(Imm) & 0x1'FF);
  }
  void ldapur(FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");
    constexpr uint32_t Op = 0b0001'1001'000 << 21;
    SubAtomicImm(Op, FEXCore::ARMEmitter::SubRegSize::i64Bit, 0b01, rt, rn, static_cast<uint32_t>(Imm) & 0x1'FF);
  }
  // Load register literal
  void ldr(FEXCore::ARMEmitter::WRegister rt, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -1048576 && Imm <= 1048575 && ((Imm & 0b11) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = 0b0001'1000 << 24;
    LoadStoreLiteral(Op, rt, static_cast<uint32_t>(Imm >> 2) & 0x7'FFFF);
  }
  void ldr(FEXCore::ARMEmitter::SRegister rt, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -1048576 && Imm <= 1048575 && ((Imm & 0b11) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = 0b0001'1100 << 24;
    LoadStoreLiteral(Op, rt, static_cast<uint32_t>(Imm >> 2) & 0x7'FFFF);
  }
  void ldr(FEXCore::ARMEmitter::XRegister rt, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -1048576 && Imm <= 1048575 && ((Imm & 0b11) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = 0b0101'1000 << 24;
    LoadStoreLiteral(Op, rt, static_cast<uint32_t>(Imm >> 2) & 0x7'FFFF);
  }
  void ldr(FEXCore::ARMEmitter::DRegister rt, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -1048576 && Imm <= 1048575 && ((Imm & 0b11) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = 0b0101'1100 << 24;
    LoadStoreLiteral(Op, rt, static_cast<uint32_t>(Imm >> 2) & 0x7'FFFF);
  }
  void ldrs(FEXCore::ARMEmitter::WRegister rt, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -1048576 && Imm <= 1048575 && ((Imm & 0b11) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = 0b1001'1000 << 24;
    LoadStoreLiteral(Op, rt, static_cast<uint32_t>(Imm >> 2) & 0x7'FFFF);
  }
  void ldr(FEXCore::ARMEmitter::QRegister rt, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -1048576 && Imm <= 1048575 && ((Imm & 0b11) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = 0b1001'1100 << 24;
    LoadStoreLiteral(Op, rt, static_cast<uint32_t>(Imm >> 2) & 0x7'FFFF);
  }
  void prfm(FEXCore::ARMEmitter::Prefetch prfop, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -1048576 && Imm <= 1048575 && ((Imm & 0b11) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = 0b1101'1000 << 24;
    LoadStoreLiteral(Op, prfop, static_cast<uint32_t>(Imm >> 2) & 0x7'FFFF);
  }
  void ldr(FEXCore::ARMEmitter::WRegister rt, BackwardLabel const* Label) {
    int32_t Imm = static_cast<int32_t>(Label->Location - GetCursorAddress<uint8_t*>());
    LOGMAN_THROW_A_FMT(Imm >= -1048576 && Imm <= 1048575 && ((Imm & 0b11) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = 0b0001'1000 << 24;
    LoadStoreLiteral(Op, rt, static_cast<uint32_t>(Imm >> 2) & 0x7'FFFF);
  }
  void ldr(FEXCore::ARMEmitter::SRegister rt, BackwardLabel const* Label) {
    int32_t Imm = static_cast<int32_t>(Label->Location - GetCursorAddress<uint8_t*>());
    LOGMAN_THROW_A_FMT(Imm >= -1048576 && Imm <= 1048575 && ((Imm & 0b11) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = 0b0001'1100 << 24;
    LoadStoreLiteral(Op, rt, static_cast<uint32_t>(Imm >> 2) & 0x7'FFFF);
  }
  void ldr(FEXCore::ARMEmitter::XRegister rt, BackwardLabel const* Label) {
    int32_t Imm = static_cast<int32_t>(Label->Location - GetCursorAddress<uint8_t*>());
    LOGMAN_THROW_A_FMT(Imm >= -1048576 && Imm <= 1048575 && ((Imm & 0b11) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = 0b0101'1000 << 24;
    LoadStoreLiteral(Op, rt, static_cast<uint32_t>(Imm >> 2) & 0x7'FFFF);
  }
  void ldr(FEXCore::ARMEmitter::DRegister rt, BackwardLabel const* Label) {
    int32_t Imm = static_cast<int32_t>(Label->Location - GetCursorAddress<uint8_t*>());
    LOGMAN_THROW_A_FMT(Imm >= -1048576 && Imm <= 1048575 && ((Imm & 0b11) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = 0b0101'1100 << 24;
    LoadStoreLiteral(Op, rt, static_cast<uint32_t>(Imm >> 2) & 0x7'FFFF);
  }
  void ldrsw(FEXCore::ARMEmitter::XRegister rt, BackwardLabel const* Label) {
    int32_t Imm = static_cast<int32_t>(Label->Location - GetCursorAddress<uint8_t*>());
    LOGMAN_THROW_A_FMT(Imm >= -1048576 && Imm <= 1048575 && ((Imm & 0b11) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = 0b1001'1000 << 24;
    LoadStoreLiteral(Op, rt, static_cast<uint32_t>(Imm >> 2) & 0x7'FFFF);
  }
  void ldr(FEXCore::ARMEmitter::QRegister rt, BackwardLabel const* Label) {
    int32_t Imm = static_cast<int32_t>(Label->Location - GetCursorAddress<uint8_t*>());
    LOGMAN_THROW_A_FMT(Imm >= -1048576 && Imm <= 1048575 && ((Imm & 0b11) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = 0b1001'1100 << 24;
    LoadStoreLiteral(Op, rt, static_cast<uint32_t>(Imm >> 2) & 0x7'FFFF);
  }
  void prfm(FEXCore::ARMEmitter::Prefetch prfop, BackwardLabel const* Label) {
    int32_t Imm = static_cast<int32_t>(Label->Location - GetCursorAddress<uint8_t*>());
    LOGMAN_THROW_A_FMT(Imm >= -1048576 && Imm <= 1048575 && ((Imm & 0b11) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = 0b1101'1000 << 24;
    LoadStoreLiteral(Op, prfop, static_cast<uint32_t>(Imm >> 2) & 0x7'FFFF);
  }

  void ldr(FEXCore::ARMEmitter::WRegister rt, ForwardLabel *Label) {
    Label->Insts.emplace_back(ForwardLabel::Instructions{ .Location = GetCursorAddress<uint8_t*>(), .Type = ForwardLabel::Instructions::InstType::RELATIVE_LOAD });
    constexpr uint32_t Op = 0b0001'1000 << 24;
    LoadStoreLiteral(Op, rt, 0);
  }
  void ldr(FEXCore::ARMEmitter::SRegister rt, ForwardLabel *Label) {
    Label->Insts.emplace_back(ForwardLabel::Instructions{ .Location = GetCursorAddress<uint8_t*>(), .Type = ForwardLabel::Instructions::InstType::RELATIVE_LOAD });
    constexpr uint32_t Op = 0b0001'1100 << 24;
    LoadStoreLiteral(Op, rt, 0);
  }
  void ldr(FEXCore::ARMEmitter::XRegister rt, ForwardLabel *Label) {
    Label->Insts.emplace_back(ForwardLabel::Instructions{ .Location = GetCursorAddress<uint8_t*>(), .Type = ForwardLabel::Instructions::InstType::RELATIVE_LOAD });
    constexpr uint32_t Op = 0b0101'1000 << 24;
    LoadStoreLiteral(Op, rt, 0);
  }
  void ldr(FEXCore::ARMEmitter::DRegister rt, ForwardLabel *Label) {
    Label->Insts.emplace_back(ForwardLabel::Instructions{ .Location = GetCursorAddress<uint8_t*>(), .Type = ForwardLabel::Instructions::InstType::RELATIVE_LOAD });
    constexpr uint32_t Op = 0b0101'1100 << 24;
    LoadStoreLiteral(Op, rt, 0);
  }
  void ldrsw(FEXCore::ARMEmitter::XRegister rt, ForwardLabel *Label) {
    Label->Insts.emplace_back(ForwardLabel::Instructions{ .Location = GetCursorAddress<uint8_t*>(), .Type = ForwardLabel::Instructions::InstType::RELATIVE_LOAD });
    constexpr uint32_t Op = 0b1001'1000 << 24;
    LoadStoreLiteral(Op, rt, 0);
  }
  void ldr(FEXCore::ARMEmitter::QRegister rt, ForwardLabel *Label) {
    Label->Insts.emplace_back(ForwardLabel::Instructions{ .Location = GetCursorAddress<uint8_t*>(), .Type = ForwardLabel::Instructions::InstType::RELATIVE_LOAD });
    constexpr uint32_t Op = 0b1001'1100 << 24;
    LoadStoreLiteral(Op, rt, 0);
  }
  void prfm(FEXCore::ARMEmitter::Prefetch prfop, ForwardLabel *Label) {
    Label->Insts.emplace_back(ForwardLabel::Instructions{ .Location = GetCursorAddress<uint8_t*>(), .Type = ForwardLabel::Instructions::InstType::RELATIVE_LOAD });
    constexpr uint32_t Op = 0b1101'1000 << 24;
    LoadStoreLiteral(Op, prfop, 0);
  }

  void ldr(FEXCore::ARMEmitter::WRegister rt, BiDirectionalLabel *Label) {
    if (Label->Backward.Location) {
      ldr(rt, &Label->Backward);
    }
    else {
      ldr(rt, &Label->Forward);
    }
  }
  void ldr(FEXCore::ARMEmitter::SRegister rt, BiDirectionalLabel *Label) {
    if (Label->Backward.Location) {
      ldr(rt, &Label->Backward);
    }
    else {
      ldr(rt, &Label->Forward);
    }
  }
  void ldr(FEXCore::ARMEmitter::XRegister rt, BiDirectionalLabel *Label) {
    if (Label->Backward.Location) {
      ldr(rt, &Label->Backward);
    }
    else {
      ldr(rt, &Label->Forward);
    }
  }
  void ldr(FEXCore::ARMEmitter::DRegister rt, BiDirectionalLabel *Label) {
    if (Label->Backward.Location) {
      ldr(rt, &Label->Backward);
    }
    else {
      ldr(rt, &Label->Forward);
    }
  }
  void ldrs(FEXCore::ARMEmitter::WRegister rt, BiDirectionalLabel *Label) {
    if (Label->Backward.Location) {
      ldr(rt, &Label->Backward);
    }
    else {
      ldr(rt, &Label->Forward);
    }
  }
  void ldr(FEXCore::ARMEmitter::QRegister rt, BiDirectionalLabel *Label) {
    if (Label->Backward.Location) {
      ldr(rt, &Label->Backward);
    }
    else {
      ldr(rt, &Label->Forward);
    }
  }
  void prfm(FEXCore::ARMEmitter::Prefetch prfop, BiDirectionalLabel *Label) {
    if (Label->Backward.Location) {
      prfm(prfop, &Label->Backward);
    }
    else {
      prfm(prfop, &Label->Forward);
    }
  }

  // Memory copy/set
  // TODO
  // Loadstore no-allocate pair
  void stnp(FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::WRegister rt2, FEXCore::ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 252 && ((Imm & 0b11) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = 0b0010'1000'00 << 22;
    LoadStoreNoAllocate(Op, rt, rt2, rn, static_cast<uint32_t>(Imm >> 2) & 0b111'1111);
  }
  void ldnp(FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::WRegister rt2, FEXCore::ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 252 && ((Imm & 0b11) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = 0b0010'1000'01 << 22;
    LoadStoreNoAllocate(Op, rt, rt2, rn, static_cast<uint32_t>(Imm >> 2) & 0b111'1111);
  }
  void stnp(FEXCore::ARMEmitter::SRegister rt, FEXCore::ARMEmitter::SRegister rt2, FEXCore::ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 252 && ((Imm & 0b11) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = 0b0010'1100'00 << 22;
    LoadStoreNoAllocate(Op, rt, rt2, rn, static_cast<uint32_t>(Imm >> 2) & 0b111'1111);
  }
  void ldnp(FEXCore::ARMEmitter::SRegister rt, FEXCore::ARMEmitter::SRegister rt2, FEXCore::ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 252 && ((Imm & 0b11) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = 0b0010'1100'01 << 22;
    LoadStoreNoAllocate(Op, rt, rt2, rn, static_cast<uint32_t>(Imm >> 2) & 0b111'1111);
  }
  void stnp(FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::XRegister rt2, FEXCore::ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -512 && Imm <= 504 && ((Imm & 0b111) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = 0b1010'1000'00 << 22;
    LoadStoreNoAllocate(Op, rt, rt2, rn, static_cast<uint32_t>(Imm >> 3) & 0b111'1111);
  }
  void ldnp(FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::XRegister rt2, FEXCore::ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -512 && Imm <= 504 && ((Imm & 0b111) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = 0b1010'1000'01 << 22;
    LoadStoreNoAllocate(Op, rt, rt2, rn, static_cast<uint32_t>(Imm >> 3) & 0b111'1111);
  }
  void stnp(FEXCore::ARMEmitter::DRegister rt, FEXCore::ARMEmitter::DRegister rt2, FEXCore::ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -512 && Imm <= 504 && ((Imm & 0b111) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = 0b0110'1100'00 << 22;
    LoadStoreNoAllocate(Op, rt, rt2, rn, static_cast<uint32_t>(Imm >> 3) & 0b111'1111);
  }
  void ldnp(FEXCore::ARMEmitter::DRegister rt, FEXCore::ARMEmitter::DRegister rt2, FEXCore::ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -512 && Imm <= 504 && ((Imm & 0b111) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = 0b0110'1100'01 << 22;
    LoadStoreNoAllocate(Op, rt, rt2, rn, static_cast<uint32_t>(Imm >> 3) & 0b111'1111);
  }
  void stnp(FEXCore::ARMEmitter::QRegister rt, FEXCore::ARMEmitter::QRegister rt2, FEXCore::ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -1024 && Imm <= 1008 && ((Imm & 0b1111) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = 0b1010'1100'00 << 22;
    LoadStoreNoAllocate(Op, rt, rt2, rn, static_cast<uint32_t>(Imm >> 4) & 0b111'1111);
  }
  void ldnp(FEXCore::ARMEmitter::QRegister rt, FEXCore::ARMEmitter::QRegister rt2, FEXCore::ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -1024 && Imm <= 1008 && ((Imm & 0b1111) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = 0b1010'1100'01 << 22;
    LoadStoreNoAllocate(Op, rt, rt2, rn, static_cast<uint32_t>(Imm >> 4) & 0b111'1111);
  }
  // Loadstore register pair post-indexed
  // Loadstore register pair offset
  // Loadstore register pair pre-indexed
  template<IndexType Index>
  void stp(FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::WRegister rt2, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 252 && ((Imm & 0b11) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = (0b0010'1000'00 << 22) |
      (
        Index == IndexType::POST   ? (0b01 << 23) :
        Index == IndexType::PRE    ? (0b11 << 23) :
        Index == IndexType::OFFSET ? (0b10 << 23) : -1
      );

    LoadStorePair(Op, rt, rt2, rn, (Imm >> 2) & 0b111'1111);
  }
  template<IndexType Index>
  void ldp(FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::WRegister rt2, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 252 && ((Imm & 0b11) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = (0b0010'1000'01 << 22) |
      (
        Index == IndexType::POST   ? (0b01 << 23) :
        Index == IndexType::PRE    ? (0b11 << 23) :
        Index == IndexType::OFFSET ? (0b10 << 23) : -1
      );

    LoadStorePair(Op, rt, rt2, rn, (Imm >> 2) & 0b111'1111);
  }
  template <IndexType Index>
  void ldpsw(FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::XRegister rt2, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 252 && ((Imm & 0b11) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = (0b0110'1000'01 << 22) |
      (
        Index == IndexType::POST   ? (0b01 << 23) :
        Index == IndexType::PRE    ? (0b11 << 23) :
        Index == IndexType::OFFSET ? (0b10 << 23) : -1
      );
    LoadStorePair(Op, rt, rt2, rn, (Imm >> 2) & 0b111'1111);
  }
  template<IndexType Index>
  void stp(FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::XRegister rt2, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    LOGMAN_THROW_A_FMT(Imm >= -512 && Imm <= 504 && ((Imm & 0b111) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = (0b1010'1000'00 << 22) |
      (
        Index == IndexType::POST   ? (0b01 << 23) :
        Index == IndexType::PRE    ? (0b11 << 23) :
        Index == IndexType::OFFSET ? (0b10 << 23) : -1
      );

    LoadStorePair(Op, rt, rt2, rn, (Imm >> 3) & 0b111'1111);
  }
  template<IndexType Index>
  void ldp(FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::XRegister rt2, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    LOGMAN_THROW_A_FMT(Imm >= -512 && Imm <= 504 && ((Imm & 0b111) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = (0b1010'1000'01 << 22) |
      (
        Index == IndexType::POST   ? (0b01 << 23) :
        Index == IndexType::PRE    ? (0b11 << 23) :
        Index == IndexType::OFFSET ? (0b10 << 23) : -1
      );

    LoadStorePair(Op, rt, rt2, rn, (Imm >> 3) & 0b111'1111);
  }
  template <IndexType Index>
  void stp(FEXCore::ARMEmitter::SRegister rt, FEXCore::ARMEmitter::SRegister rt2, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    stp_w<Index>(rt.V(), rt2.V(), rn, Imm);
  }
  template <IndexType Index>
  void ldp(FEXCore::ARMEmitter::SRegister rt, FEXCore::ARMEmitter::SRegister rt2, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    ldp_w<Index>(rt.V(), rt2.V(), rn, Imm);
  }
  template <IndexType Index>
  void stp(FEXCore::ARMEmitter::DRegister rt, FEXCore::ARMEmitter::DRegister rt2, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    stp_x<Index>(rt.V(), rt2.V(), rn, Imm);
  }
  template <IndexType Index>
  void ldp(FEXCore::ARMEmitter::DRegister rt, FEXCore::ARMEmitter::DRegister rt2, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    ldp_x<Index>(rt.V(), rt2.V(), rn, Imm);
  }
  template <IndexType Index>
  void stp(FEXCore::ARMEmitter::QRegister rt, FEXCore::ARMEmitter::QRegister rt2, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    stp_q<Index>(rt.V(), rt2.V(), rn, Imm);
  }
  template <IndexType Index>
  void ldp(FEXCore::ARMEmitter::QRegister rt, FEXCore::ARMEmitter::QRegister rt2, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    ldp_q<Index>(rt.V(), rt2.V(), rn, Imm);
  }

  // Loadstore register unscaled immediate
  void sturb(FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    stXrb<IndexType::OFFSET>(rt, rn, Imm);
  }
  void ldurb(FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXrb<IndexType::OFFSET>(rt, rn, Imm);
  }
  void sturb(FEXCore::ARMEmitter::VRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    stXrb<IndexType::OFFSET>(rt, rn, Imm);
  }
  void ldurb(FEXCore::ARMEmitter::VRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXrb<IndexType::OFFSET>(rt, rn, Imm);
  }
  void ldursb(FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXrsb<IndexType::OFFSET>(rt, rn, Imm);
  }
  void ldursb(FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXrsb<IndexType::OFFSET>(rt, rn, Imm);
  }
  void sturh(FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    stXrh<IndexType::OFFSET>(rt, rn, Imm);
  }
  void ldurh(FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXrh<IndexType::OFFSET>(rt, rn, Imm);
  }
  void sturh(FEXCore::ARMEmitter::VRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    stXrh<IndexType::OFFSET>(rt, rn, Imm);
  }
  void ldurh(FEXCore::ARMEmitter::VRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXrh<IndexType::OFFSET>(rt, rn, Imm);
  }
  void ldursh(FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXrsh<IndexType::OFFSET>(rt, rn, Imm);
  }
  void ldursh(FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXrsh<IndexType::OFFSET>(rt, rn, Imm);
  }
  void stur(FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    stXr<IndexType::OFFSET>(rt, rn, Imm);
  }
  void ldur(FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXr<IndexType::OFFSET>(rt, rn, Imm);
  }
  void stur(FEXCore::ARMEmitter::SRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    stXr<IndexType::OFFSET>(rt, rn, Imm);
  }
  void ldur(FEXCore::ARMEmitter::SRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXr<IndexType::OFFSET>(rt, rn, Imm);
  }
  void ldursw(FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXrsw<IndexType::OFFSET>(rt, rn, Imm);
  }
  void stur(FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    stXr<IndexType::OFFSET>(rt, rn, Imm);
  }
  void ldur(FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXr<IndexType::OFFSET>(rt, rn, Imm);
  }
  void stur(FEXCore::ARMEmitter::DRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    stXr<IndexType::OFFSET>(rt, rn, Imm);
  }
  void ldur(FEXCore::ARMEmitter::DRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXr<IndexType::OFFSET>(rt, rn, Imm);
  }
  void stur(FEXCore::ARMEmitter::QRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    stXr<IndexType::OFFSET>(rt, rn, Imm);
  }
  void ldur(FEXCore::ARMEmitter::QRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXr<IndexType::OFFSET>(rt, rn, Imm);
  }
  template <IndexType Index>
  void prfum(FEXCore::ARMEmitter::Prefetch prfop, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");
    static_assert(Index == IndexType::OFFSET, "Doesn't support another index type");

    constexpr uint32_t Op = 0b1111'1000'10 << 22;
    constexpr uint32_t o2 = 0b00;

    LoadStoreImm(Op, o2, prfop, rn, Imm);
  }

  // Loadstore register immediate post-indexed
  // Loadstore register immediate pre-indexed
  template <IndexType Index>
  requires(Index == IndexType::POST || Index == IndexType::PRE)
  void strb(FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    stXrb<Index>(rt, rn, Imm);
  }
  template <IndexType Index>
  requires(Index == IndexType::POST || Index == IndexType::PRE)
  void ldrb(FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXrb<Index>(rt, rn, Imm);
  }
  template <IndexType Index>
  requires(Index == IndexType::POST || Index == IndexType::PRE)
  void strb(FEXCore::ARMEmitter::VRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    stXrb<Index>(rt, rn, Imm);
  }
  template <IndexType Index>
  requires(Index == IndexType::POST || Index == IndexType::PRE)
  void ldrb(FEXCore::ARMEmitter::VRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXrb<Index>(rt, rn, Imm);
  }
  template <IndexType Index>
  requires(Index == IndexType::POST || Index == IndexType::PRE)
  void ldrsb(FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXrsb<Index>(rt, rn, Imm);
  }
  template <IndexType Index>
  requires(Index == IndexType::POST || Index == IndexType::PRE)
  void ldrsb(FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXrsb<Index>(rt, rn, Imm);
  }
  template <IndexType Index>
  requires(Index == IndexType::POST || Index == IndexType::PRE)
  void strh(FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    stXrh<Index>(rt, rn, Imm);
  }
  template <IndexType Index>
  requires(Index == IndexType::POST || Index == IndexType::PRE)
  void ldrh(FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXrh<Index>(rt, rn, Imm);
  }
  template <IndexType Index>
  requires(Index == IndexType::POST || Index == IndexType::PRE)
  void strh(FEXCore::ARMEmitter::VRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    stXrh<Index>(rt, rn, Imm);
  }
  template <IndexType Index>
  requires(Index == IndexType::POST || Index == IndexType::PRE)
  void ldrh(FEXCore::ARMEmitter::VRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXrh<Index>(rt, rn, Imm);
  }
  template <IndexType Index>
  requires(Index == IndexType::POST || Index == IndexType::PRE)
  void ldrsh(FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXrsh<Index>(rt, rn, Imm);
  }
  template <IndexType Index>
  requires(Index == IndexType::POST || Index == IndexType::PRE)
  void ldrsh(FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXrsh<Index>(rt, rn, Imm);
  }
  template <IndexType Index>
  requires(Index == IndexType::POST || Index == IndexType::PRE)
  void str(FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    stXr<Index>(rt, rn, Imm);
  }
  template <IndexType Index>
  requires(Index == IndexType::POST || Index == IndexType::PRE)
  void ldr(FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXr<Index>(rt, rn, Imm);
  }
  template <IndexType Index>
  requires(Index == IndexType::POST || Index == IndexType::PRE)
  void str(FEXCore::ARMEmitter::SRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    stXr<Index>(rt, rn, Imm);
  }
  template <IndexType Index>
  requires(Index == IndexType::POST || Index == IndexType::PRE)
  void ldr(FEXCore::ARMEmitter::SRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXr<Index>(rt, rn, Imm);
  }
  template <IndexType Index>
  requires(Index == IndexType::POST || Index == IndexType::PRE)
  void ldrsw(FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXrsw<Index>(rt, rn, Imm);
  }
  template <IndexType Index>
  requires(Index == IndexType::POST || Index == IndexType::PRE)
  void str(FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    stXr<Index>(rt, rn, Imm);
  }
  template <IndexType Index>
  requires(Index == IndexType::POST || Index == IndexType::PRE)
  void ldr(FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXr<Index>(rt, rn, Imm);
  }
  template <IndexType Index>
  requires(Index == IndexType::POST || Index == IndexType::PRE)
  void str(FEXCore::ARMEmitter::DRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    stXr<Index>(rt, rn, Imm);
  }
  template <IndexType Index>
  requires(Index == IndexType::POST || Index == IndexType::PRE)
  void ldr(FEXCore::ARMEmitter::DRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXr<Index>(rt, rn, Imm);
  }
  template <IndexType Index>
  requires(Index == IndexType::POST || Index == IndexType::PRE)
  void str(FEXCore::ARMEmitter::QRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    stXr<Index>(rt, rn, Imm);
  }
  template <IndexType Index>
  requires(Index == IndexType::POST || Index == IndexType::PRE)
  void ldr(FEXCore::ARMEmitter::QRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXr<Index>(rt, rn, Imm);
  }

  // Loadstore register unprivileged
  void sttrb(FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    stXrb<IndexType::UNPRIVILEGED>(rt, rn, Imm);
  }
  void ldtrb(FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXrb<IndexType::UNPRIVILEGED>(rt, rn, Imm);
  }
  void ldtrsb(FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXrsb<IndexType::UNPRIVILEGED>(rt, rn, Imm);
  }
  void ldtrsb(FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXrsb<IndexType::UNPRIVILEGED>(rt, rn, Imm);
  }
  void sttrh(FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    stXrh<IndexType::UNPRIVILEGED>(rt, rn, Imm);
  }
  void ldtrh(FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXrh<IndexType::UNPRIVILEGED>(rt, rn, Imm);
  }
  void ldtrsh(FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXrsh<IndexType::UNPRIVILEGED>(rt, rn, Imm);
  }
  void ldtrsh(FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXrsh<IndexType::UNPRIVILEGED>(rt, rn, Imm);
  }
  void sttr(FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    stXr<IndexType::UNPRIVILEGED>(rt, rn, Imm);
  }
  void ldtr(FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXr<IndexType::UNPRIVILEGED>(rt, rn, Imm);
  }
  void ldtrsw(FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXrsw<IndexType::UNPRIVILEGED>(rt, rn, Imm);
  }
  void sttr(FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    stXr<IndexType::UNPRIVILEGED>(rt, rn, Imm);
  }
  void ldtr(FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm = 0) {
    ldXr<IndexType::UNPRIVILEGED>(rt, rn, Imm);
  }
  // Atomic memory operations
  void stadd(ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, size, 0, 0, 0, 0b000, rs, FEXCore::ARMEmitter::Reg::zr, rn);
  }
  void staddl(ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, size, 0, 1, 0, 0b000, rs, FEXCore::ARMEmitter::Reg::zr, rn);
  }
  void stclr(ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, size, 0, 0, 0, 0b001, rs, FEXCore::ARMEmitter::Reg::zr, rn);
  }
  void stclrl(ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, size, 0, 1, 0, 0b001, rs, FEXCore::ARMEmitter::Reg::zr, rn);
  }
  void stset(ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, size, 0, 0, 0, 0b011, rs, FEXCore::ARMEmitter::Reg::zr, rn);
  }
  void stsetl(ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, size, 0, 1, 0, 0b011, rs, FEXCore::ARMEmitter::Reg::zr, rn);
  }
  void steor(ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, size, 0, 0, 0, 0b010, rs, FEXCore::ARMEmitter::Reg::zr, rn);
  }
  void steorl(ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, size, 0, 1, 0, 0b010, rs, FEXCore::ARMEmitter::Reg::zr, rn);
  }
  void ldswp(ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, size, 0, 0, 1, 0b000, rs, rt, rn);
  }
  void ldswpl(ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, size, 0, 1, 1, 0b000, rs, rt, rn);
  }
  void ldswpa(ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, size, 1, 0, 1, 0b000, rs, rt, rn);
  }
  void ldswpal(ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, size, 1, 1, 1, 0b000, rs, rt, rn);
  }

  void ldadd(ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, size, 0, 0, 0, 0b000, rs, rt, rn);
  }
  void ldadda(ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, size, 1, 0, 0, 0b000, rs, rt, rn);
  }
  void ldaddl(ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, size, 0, 1, 0, 0b000, rs, rt, rn);
  }
  void ldaddal(ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, size, 1, 1, 0, 0b000, rs, rt, rn);
  }
  void ldclr(ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, size, 0, 0, 0, 0b001, rs, rt, rn);
  }
  void ldclra(ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, size, 1, 0, 0, 0b001, rs, rt, rn);
  }
  void ldclrl(ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, size, 0, 1, 0, 0b001, rs, rt, rn);
  }
  void ldclral(ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, size, 1, 1, 0, 0b001, rs, rt, rn);
  }

  void ldset(ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, size, 0, 0, 0, 0b011, rs, rt, rn);
  }
  void ldseta(ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, size, 1, 0, 0, 0b011, rs, rt, rn);
  }
  void ldsetl(ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, size, 0, 1, 0, 0b011, rs, rt, rn);
  }
  void ldsetal(ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, size, 1, 1, 0, 0b011, rs, rt, rn);
  }
  void ldeor(ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, size, 0, 0, 0, 0b010, rs, rt, rn);
  }
  void ldeora(ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, size, 1, 0, 0, 0b010, rs, rt, rn);
  }
  void ldeorl(ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, size, 0, 1, 0, 0b010, rs, rt, rn);
  }
  void ldeoral(ARMEmitter::SubRegSize size, FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, size, 1, 1, 0, 0b010, rs, rt, rn);
  }


  // 8-bit
  void ldaddb(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i8Bit, 0, 0, 0, 0b000, rs, rt, rn);
  }
  void ldclrb(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i8Bit, 0, 0, 0, 0b001, rs, rt, rn);
  }
  void ldeorb(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i8Bit, 0, 0, 0, 0b010, rs, rt, rn);
  }
  void ldsetb(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i8Bit, 0, 0, 0, 0b011, rs, rt, rn);
  }
  void ldsmaxb(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i8Bit, 0, 0, 0, 0b100, rs, rt, rn);
  }
  void ldsminb(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i8Bit, 0, 0, 0, 0b101, rs, rt, rn);
  }
  void ldumaxb(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i8Bit, 0, 0, 0, 0b110, rs, rt, rn);
  }
  void lduminb(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i8Bit, 0, 0, 0, 0b111, rs, rt, rn);
  }
  void ldswpb(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i8Bit, 0, 0, 1, 0b000, rs, rt, rn);
  }
  void ldaddlb(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i8Bit, 0, 1, 0, 0b000, rs, rt, rn);
  }
  void ldclrlb(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i8Bit, 0, 1, 0, 0b001, rs, rt, rn);
  }
  void ldeorlb(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i8Bit, 0, 1, 0, 0b010, rs, rt, rn);
  }
  void ldsetlb(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i8Bit, 0, 1, 0, 0b011, rs, rt, rn);
  }
  void ldsmaxlb(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i8Bit, 0, 1, 0, 0b100, rs, rt, rn);
  }
  void ldsminlb(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i8Bit, 0, 1, 0, 0b101, rs, rt, rn);
  }
  void ldumaxlb(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i8Bit, 0, 1, 0, 0b110, rs, rt, rn);
  }
  void lduminlb(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i8Bit, 0, 1, 0, 0b111, rs, rt, rn);
  }
  void ldswplb(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i8Bit, 0, 1, 1, 0b000, rs, rt, rn);
  }
  void ldaddab(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i8Bit, 1, 0, 0, 0b000, rs, rt, rn);
  }
  void ldclrab(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i8Bit, 1, 0, 0, 0b001, rs, rt, rn);
  }
  void ldeorab(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i8Bit, 1, 0, 0, 0b010, rs, rt, rn);
  }
  void ldsetab(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i8Bit, 1, 0, 0, 0b011, rs, rt, rn);
  }
  void ldsmaxab(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i8Bit, 1, 0, 0, 0b100, rs, rt, rn);
  }
  void ldsminab(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i8Bit, 1, 0, 0, 0b101, rs, rt, rn);
  }
  void ldumaxab(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i8Bit, 1, 0, 0, 0b110, rs, rt, rn);
  }
  void lduminab(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i8Bit, 1, 0, 0, 0b111, rs, rt, rn);
  }
  void ldswpab(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i8Bit, 1, 0, 1, 0b000, rs, rt, rn);
  }
  void ldaddalb(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i8Bit, 1, 1, 0, 0b000, rs, rt, rn);
  }
  void ldclralb(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i8Bit, 1, 1, 0, 0b001, rs, rt, rn);
  }
  void ldeoralb(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i8Bit, 1, 1, 0, 0b010, rs, rt, rn);
  }
  void ldsetalb(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i8Bit, 1, 1, 0, 0b011, rs, rt, rn);
  }
  void ldsmaxalb(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i8Bit, 1, 1, 0, 0b100, rs, rt, rn);
  }
  void ldsminalb(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i8Bit, 1, 1, 0, 0b101, rs, rt, rn);
  }
  void ldumaxalb(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i8Bit, 1, 1, 0, 0b110, rs, rt, rn);
  }
  void lduminalb(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i8Bit, 1, 1, 0, 0b111, rs, rt, rn);
  }
  void ldswpalb(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i8Bit, 1, 1, 1, 0b000, rs, rt, rn);
  }
  // 16-bit
  void ldaddh(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i16Bit, 0, 0, 0, 0b000, rs, rt, rn);
  }
  void ldclrh(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i16Bit, 0, 0, 0, 0b001, rs, rt, rn);
  }
  void ldeorh(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i16Bit, 0, 0, 0, 0b010, rs, rt, rn);
  }
  void ldseth(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i16Bit, 0, 0, 0, 0b011, rs, rt, rn);
  }
  void ldsmaxh(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i16Bit, 0, 0, 0, 0b100, rs, rt, rn);
  }
  void ldsminh(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i16Bit, 0, 0, 0, 0b101, rs, rt, rn);
  }
  void ldumaxh(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i16Bit, 0, 0, 0, 0b110, rs, rt, rn);
  }
  void lduminh(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i16Bit, 0, 0, 0, 0b111, rs, rt, rn);
  }
  void ldswph(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i16Bit, 0, 0, 1, 0b000, rs, rt, rn);
  }
  void ldaddlh(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i16Bit, 0, 1, 0, 0b000, rs, rt, rn);
  }
  void ldclrlh(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i16Bit, 0, 1, 0, 0b001, rs, rt, rn);
  }
  void ldeorlh(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i16Bit, 0, 1, 0, 0b010, rs, rt, rn);
  }
  void ldsetlh(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i16Bit, 0, 1, 0, 0b011, rs, rt, rn);
  }
  void ldsmaxlh(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i16Bit, 0, 1, 0, 0b100, rs, rt, rn);
  }
  void ldsminlh(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i16Bit, 0, 1, 0, 0b101, rs, rt, rn);
  }
  void ldumaxlh(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i16Bit, 0, 1, 0, 0b110, rs, rt, rn);
  }
  void lduminlh(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i16Bit, 0, 1, 0, 0b111, rs, rt, rn);
  }
  void ldswplh(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i16Bit, 0, 1, 1, 0b000, rs, rt, rn);
  }
  void ldaddah(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i16Bit, 1, 0, 0, 0b000, rs, rt, rn);
  }
  void ldclrah(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i16Bit, 1, 0, 0, 0b001, rs, rt, rn);
  }
  void ldeorah(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i16Bit, 1, 0, 0, 0b010, rs, rt, rn);
  }
  void ldsetah(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i16Bit, 1, 0, 0, 0b011, rs, rt, rn);
  }
  void ldsmaxah(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i16Bit, 1, 0, 0, 0b100, rs, rt, rn);
  }
  void ldsminah(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i16Bit, 1, 0, 0, 0b101, rs, rt, rn);
  }
  void ldumaxah(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i16Bit, 1, 0, 0, 0b110, rs, rt, rn);
  }
  void lduminah(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i16Bit, 1, 0, 0, 0b111, rs, rt, rn);
  }
  void ldswpah(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i16Bit, 1, 0, 1, 0b000, rs, rt, rn);
  }
  void ldaddalh(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i16Bit, 1, 1, 0, 0b000, rs, rt, rn);
  }
  void ldclralh(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i16Bit, 1, 1, 0, 0b001, rs, rt, rn);
  }
  void ldeoralh(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i16Bit, 1, 1, 0, 0b010, rs, rt, rn);
  }
  void ldsetalh(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i16Bit, 1, 1, 0, 0b011, rs, rt, rn);
  }
  void ldsmaxalh(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i16Bit, 1, 1, 0, 0b100, rs, rt, rn);
  }
  void ldsminalh(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i16Bit, 1, 1, 0, 0b101, rs, rt, rn);
  }
  void ldumaxalh(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i16Bit, 1, 1, 0, 0b110, rs, rt, rn);
  }
  void lduminalh(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i16Bit, 1, 1, 0, 0b111, rs, rt, rn);
  }
  void ldswpalh(FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i16Bit, 1, 1, 1, 0b000, rs, rt, rn);
  }
  // 32-bit
  void ldadd(FEXCore::ARMEmitter::WRegister rs, FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i32Bit, 0, 0, 0, 0b000, rs, rt, rn);
  }
  void ldclr(FEXCore::ARMEmitter::WRegister rs, FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i32Bit, 0, 0, 0, 0b001, rs, rt, rn);
  }
  void ldeor(FEXCore::ARMEmitter::WRegister rs, FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i32Bit, 0, 0, 0, 0b010, rs, rt, rn);
  }
  void ldset(FEXCore::ARMEmitter::WRegister rs, FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i32Bit, 0, 0, 0, 0b011, rs, rt, rn);
  }
  void ldsmax(FEXCore::ARMEmitter::WRegister rs, FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i32Bit, 0, 0, 0, 0b100, rs, rt, rn);
  }
  void ldsmin(FEXCore::ARMEmitter::WRegister rs, FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i32Bit, 0, 0, 0, 0b101, rs, rt, rn);
  }
  void ldumax(FEXCore::ARMEmitter::WRegister rs, FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i32Bit, 0, 0, 0, 0b110, rs, rt, rn);
  }
  void ldumin(FEXCore::ARMEmitter::WRegister rs, FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i32Bit, 0, 0, 0, 0b111, rs, rt, rn);
  }
  void ldswp(FEXCore::ARMEmitter::WRegister rs, FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i32Bit, 0, 0, 1, 0b000, rs, rt, rn);
  }
  void ldaddl(FEXCore::ARMEmitter::WRegister rs, FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i32Bit, 0, 1, 0, 0b000, rs, rt, rn);
  }
  void ldclrl(FEXCore::ARMEmitter::WRegister rs, FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i32Bit, 0, 1, 0, 0b001, rs, rt, rn);
  }
  void ldeorl(FEXCore::ARMEmitter::WRegister rs, FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i32Bit, 0, 1, 0, 0b010, rs, rt, rn);
  }
  void ldsetl(FEXCore::ARMEmitter::WRegister rs, FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i32Bit, 0, 1, 0, 0b011, rs, rt, rn);
  }
  void ldsmaxl(FEXCore::ARMEmitter::WRegister rs, FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i32Bit, 0, 1, 0, 0b100, rs, rt, rn);
  }
  void ldsminl(FEXCore::ARMEmitter::WRegister rs, FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i32Bit, 0, 1, 0, 0b101, rs, rt, rn);
  }
  void ldumaxl(FEXCore::ARMEmitter::WRegister rs, FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i32Bit, 0, 1, 0, 0b110, rs, rt, rn);
  }
  void lduminl(FEXCore::ARMEmitter::WRegister rs, FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i32Bit, 0, 1, 0, 0b111, rs, rt, rn);
  }
  void ldswpl(FEXCore::ARMEmitter::WRegister rs, FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i32Bit, 0, 1, 1, 0b000, rs, rt, rn);
  }
  void ldadda(FEXCore::ARMEmitter::WRegister rs, FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i32Bit, 1, 0, 0, 0b000, rs, rt, rn);
  }
  void ldclra(FEXCore::ARMEmitter::WRegister rs, FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i32Bit, 1, 0, 0, 0b001, rs, rt, rn);
  }
  void ldeora(FEXCore::ARMEmitter::WRegister rs, FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i32Bit, 1, 0, 0, 0b010, rs, rt, rn);
  }
  void ldseta(FEXCore::ARMEmitter::WRegister rs, FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i32Bit, 1, 0, 0, 0b011, rs, rt, rn);
  }
  void ldsmaxa(FEXCore::ARMEmitter::WRegister rs, FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i32Bit, 1, 0, 0, 0b100, rs, rt, rn);
  }
  void ldsmina(FEXCore::ARMEmitter::WRegister rs, FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i32Bit, 1, 0, 0, 0b101, rs, rt, rn);
  }
  void ldumaxa(FEXCore::ARMEmitter::WRegister rs, FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i32Bit, 1, 0, 0, 0b110, rs, rt, rn);
  }
  void ldumina(FEXCore::ARMEmitter::WRegister rs, FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i32Bit, 1, 0, 0, 0b111, rs, rt, rn);
  }
  void ldswpa(FEXCore::ARMEmitter::WRegister rs, FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i32Bit, 1, 0, 1, 0b000, rs, rt, rn);
  }
  void ldaddal(FEXCore::ARMEmitter::WRegister rs, FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i32Bit, 1, 1, 0, 0b000, rs, rt, rn);
  }
  void ldclral(FEXCore::ARMEmitter::WRegister rs, FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i32Bit, 1, 1, 0, 0b001, rs, rt, rn);
  }
  void ldeoral(FEXCore::ARMEmitter::WRegister rs, FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i32Bit, 1, 1, 0, 0b010, rs, rt, rn);
  }
  void ldsetal(FEXCore::ARMEmitter::WRegister rs, FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i32Bit, 1, 1, 0, 0b011, rs, rt, rn);
  }
  void ldsmaxal(FEXCore::ARMEmitter::WRegister rs, FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i32Bit, 1, 1, 0, 0b100, rs, rt, rn);
  }
  void ldsminal(FEXCore::ARMEmitter::WRegister rs, FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i32Bit, 1, 1, 0, 0b101, rs, rt, rn);
  }
  void ldumaxal(FEXCore::ARMEmitter::WRegister rs, FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i32Bit, 1, 1, 0, 0b110, rs, rt, rn);
  }
  void lduminal(FEXCore::ARMEmitter::WRegister rs, FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i32Bit, 1, 1, 0, 0b111, rs, rt, rn);
  }
  void ldswpal(FEXCore::ARMEmitter::WRegister rs, FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i32Bit, 1, 1, 1, 0b000, rs, rt, rn);
  }
  // 64-bit
  void ldadd(FEXCore::ARMEmitter::XRegister rs, FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i64Bit, 0, 0, 0, 0b000, rs, rt, rn);
  }
  void ldclr(FEXCore::ARMEmitter::XRegister rs, FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i64Bit, 0, 0, 0, 0b001, rs, rt, rn);
  }
  void ldeor(FEXCore::ARMEmitter::XRegister rs, FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i64Bit, 0, 0, 0, 0b010, rs, rt, rn);
  }
  void ldset(FEXCore::ARMEmitter::XRegister rs, FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i64Bit, 0, 0, 0, 0b011, rs, rt, rn);
  }
  void ldsmax(FEXCore::ARMEmitter::XRegister rs, FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i64Bit, 0, 0, 0, 0b100, rs, rt, rn);
  }
  void ldsmin(FEXCore::ARMEmitter::XRegister rs, FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i64Bit, 0, 0, 0, 0b101, rs, rt, rn);
  }
  void ldumax(FEXCore::ARMEmitter::XRegister rs, FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i64Bit, 0, 0, 0, 0b110, rs, rt, rn);
  }
  void ldumin(FEXCore::ARMEmitter::XRegister rs, FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i64Bit, 0, 0, 0, 0b111, rs, rt, rn);
  }
  void ldswp(FEXCore::ARMEmitter::XRegister rs, FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i64Bit, 0, 0, 1, 0b000, rs, rt, rn);
  }
  void ldaddl(FEXCore::ARMEmitter::XRegister rs, FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i64Bit, 0, 1, 0, 0b000, rs, rt, rn);
  }
  void ldclrl(FEXCore::ARMEmitter::XRegister rs, FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i64Bit, 0, 1, 0, 0b001, rs, rt, rn);
  }
  void ldeorl(FEXCore::ARMEmitter::XRegister rs, FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i64Bit, 0, 1, 0, 0b010, rs, rt, rn);
  }
  void ldsetl(FEXCore::ARMEmitter::XRegister rs, FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i64Bit, 0, 1, 0, 0b011, rs, rt, rn);
  }
  void ldsmaxl(FEXCore::ARMEmitter::XRegister rs, FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i64Bit, 0, 1, 0, 0b100, rs, rt, rn);
  }
  void ldsminl(FEXCore::ARMEmitter::XRegister rs, FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i64Bit, 0, 1, 0, 0b101, rs, rt, rn);
  }
  void ldumaxl(FEXCore::ARMEmitter::XRegister rs, FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i64Bit, 0, 1, 0, 0b110, rs, rt, rn);
  }
  void lduminl(FEXCore::ARMEmitter::XRegister rs, FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i64Bit, 0, 1, 0, 0b111, rs, rt, rn);
  }
  void ldswpl(FEXCore::ARMEmitter::XRegister rs, FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i64Bit, 0, 1, 1, 0b000, rs, rt, rn);
  }
  void ldadda(FEXCore::ARMEmitter::XRegister rs, FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i64Bit, 1, 0, 0, 0b000, rs, rt, rn);
  }
  void ldclra(FEXCore::ARMEmitter::XRegister rs, FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i64Bit, 1, 0, 0, 0b001, rs, rt, rn);
  }
  void ldeora(FEXCore::ARMEmitter::XRegister rs, FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i64Bit, 1, 0, 0, 0b010, rs, rt, rn);
  }
  void ldseta(FEXCore::ARMEmitter::XRegister rs, FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i64Bit, 1, 0, 0, 0b011, rs, rt, rn);
  }
  void ldsmaxa(FEXCore::ARMEmitter::XRegister rs, FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i64Bit, 1, 0, 0, 0b100, rs, rt, rn);
  }
  void ldsmina(FEXCore::ARMEmitter::XRegister rs, FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i64Bit, 1, 0, 0, 0b101, rs, rt, rn);
  }
  void ldumaxa(FEXCore::ARMEmitter::XRegister rs, FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i64Bit, 1, 0, 0, 0b110, rs, rt, rn);
  }
  void ldumina(FEXCore::ARMEmitter::XRegister rs, FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i64Bit, 1, 0, 0, 0b111, rs, rt, rn);
  }
  void ldswpa(FEXCore::ARMEmitter::XRegister rs, FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i64Bit, 1, 0, 1, 0b000, rs, rt, rn);
  }
  void ldaddal(FEXCore::ARMEmitter::XRegister rs, FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i64Bit, 1, 1, 0, 0b000, rs, rt, rn);
  }
  void ldclral(FEXCore::ARMEmitter::XRegister rs, FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i64Bit, 1, 1, 0, 0b001, rs, rt, rn);
  }
  void ldeoral(FEXCore::ARMEmitter::XRegister rs, FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i64Bit, 1, 1, 0, 0b010, rs, rt, rn);
  }
  void ldsetal(FEXCore::ARMEmitter::XRegister rs, FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i64Bit, 1, 1, 0, 0b011, rs, rt, rn);
  }
  void ldsmaxal(FEXCore::ARMEmitter::XRegister rs, FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i64Bit, 1, 1, 0, 0b100, rs, rt, rn);
  }
  void ldsminal(FEXCore::ARMEmitter::XRegister rs, FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i64Bit, 1, 1, 0, 0b101, rs, rt, rn);
  }
  void ldumaxal(FEXCore::ARMEmitter::XRegister rs, FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i64Bit, 1, 1, 0, 0b110, rs, rt, rn);
  }
  void lduminal(FEXCore::ARMEmitter::XRegister rs, FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i64Bit, 1, 1, 0, 0b111, rs, rt, rn);
  }
  void ldswpal(FEXCore::ARMEmitter::XRegister rs, FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i64Bit, 1, 1, 1, 0b000, rs, rt, rn);
  }
  void ldaprb(FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i8Bit, 1, 0, 1, 0b100, FEXCore::ARMEmitter::WReg::w31, rt, rn);
  }
  void ldaprh(FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i16Bit, 1, 0, 1, 0b100, FEXCore::ARMEmitter::WReg::w31, rt, rn);
  }
  void ldapr(FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i32Bit, 1, 0, 1, 0b100, FEXCore::ARMEmitter::WReg::w31, rt, rn);
  }
  void ldapr(FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i64Bit, 1, 0, 1, 0b100, FEXCore::ARMEmitter::XReg::x31, rt, rn);
  }
  void st64bv0(FEXCore::ARMEmitter::XRegister rs, FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i64Bit, 0, 0, 1, 0b010, rs, rt, rn);
  }
  void st64bv(FEXCore::ARMEmitter::XRegister rs, FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i64Bit, 0, 0, 1, 0b011, rs, rt, rn);
  }
  void st64b(FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i64Bit, 0, 0, 1, 0b001, FEXCore::ARMEmitter::XReg::x31, rt, rn);
  }
  void ld64b(FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn) {
    constexpr uint32_t Op = 0b0011'1000'0010'0000'0000'00 << 10;
    LoadStoreAtomicLSE(Op, SubRegSize::i64Bit, 0, 0, 1, 0b101, FEXCore::ARMEmitter::XReg::x31, rt, rn);
  }

  // Loadstore register-register offset
  void strb(FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm, FEXCore::ARMEmitter::ExtendedType Option, bool Shift = false) {
    LOGMAN_THROW_A_FMT((FEXCore::ToUnderlying(Option) & 0b010) == 0b010, "Unsupported Extendtype");
    constexpr uint32_t Op = 0b0011'1000'001 << 21 | (0b10 << 10);
    LoadStoreRegisterOffset(Op, 0b00, rt, rn, rm, Option, Shift ? 1 : 0);
  }
  void ldrb(FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm, FEXCore::ARMEmitter::ExtendedType Option, bool Shift = false) {
    LOGMAN_THROW_A_FMT((FEXCore::ToUnderlying(Option) & 0b010) == 0b010, "Unsupported Extendtype");
    constexpr uint32_t Op = 0b0011'1000'001 << 21 | (0b10 << 10);
    LoadStoreRegisterOffset(Op, 0b01, rt, rn, rm, Option, Shift ? 1 : 0);
  }
  void ldrsb(FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm, FEXCore::ARMEmitter::ExtendedType Option, bool Shift = false) {
    LOGMAN_THROW_A_FMT((FEXCore::ToUnderlying(Option) & 0b010) == 0b010, "Unsupported Extendtype");
    constexpr uint32_t Op = 0b0011'1000'001 << 21 | (0b10 << 10);
    LoadStoreRegisterOffset(Op, 0b10, rt, rn, rm, Option, Shift ? 1 : 0);
  }
  void ldrsb(FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm, FEXCore::ARMEmitter::ExtendedType Option, bool Shift = false) {
    LOGMAN_THROW_A_FMT((FEXCore::ToUnderlying(Option) & 0b010) == 0b010, "Unsupported Extendtype");
    constexpr uint32_t Op = 0b0011'1000'001 << 21 | (0b10 << 10);
    LoadStoreRegisterOffset(Op, 0b11, rt, rn, rm, Option, Shift ? 1 : 0);
  }
  void strh(FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm, FEXCore::ARMEmitter::ExtendedType Option, uint32_t Shift) {
    LOGMAN_THROW_A_FMT((FEXCore::ToUnderlying(Option) & 0b010) == 0b010, "Unsupported Extendtype");
    LOGMAN_THROW_A_FMT(Shift == 0 || Shift == 1, "Unsupported shift amount");
    constexpr uint32_t Op = 0b0111'1000'001 << 21 | (0b10 << 10);
    LoadStoreRegisterOffset(Op, 0b00, rt, rn, rm, Option, Shift ? 1 : 0);
  }
  void ldrh(FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm, FEXCore::ARMEmitter::ExtendedType Option, uint32_t Shift) {
    LOGMAN_THROW_A_FMT((FEXCore::ToUnderlying(Option) & 0b010) == 0b010, "Unsupported Extendtype");
    LOGMAN_THROW_A_FMT(Shift == 0 || Shift == 1, "Unsupported shift amount");
    constexpr uint32_t Op = 0b0111'1000'001 << 21 | (0b10 << 10);
    LoadStoreRegisterOffset(Op, 0b01, rt, rn, rm, Option, Shift ? 1 : 0);
  }
  void ldrsh(FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm, FEXCore::ARMEmitter::ExtendedType Option, uint32_t Shift) {
    LOGMAN_THROW_A_FMT((FEXCore::ToUnderlying(Option) & 0b010) == 0b010, "Unsupported Extendtype");
    LOGMAN_THROW_A_FMT(Shift == 0 || Shift == 1, "Unsupported shift amount");
    constexpr uint32_t Op = 0b0111'1000'001 << 21 | (0b10 << 10);
    LoadStoreRegisterOffset(Op, 0b10, rt, rn, rm, Option, Shift ? 1 : 0);
  }
  void ldrsh(FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm, FEXCore::ARMEmitter::ExtendedType Option, uint32_t Shift) {
    LOGMAN_THROW_A_FMT((FEXCore::ToUnderlying(Option) & 0b010) == 0b010, "Unsupported Extendtype");
    LOGMAN_THROW_A_FMT(Shift == 0 || Shift == 1, "Unsupported shift amount");
    constexpr uint32_t Op = 0b0111'1000'001 << 21 | (0b10 << 10);
    LoadStoreRegisterOffset(Op, 0b11, rt, rn, rm, Option, Shift ? 1 : 0);
  }
  void str(FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm, FEXCore::ARMEmitter::ExtendedType Option, uint32_t Shift) {
    LOGMAN_THROW_A_FMT((FEXCore::ToUnderlying(Option) & 0b010) == 0b010, "Unsupported Extendtype");
    LOGMAN_THROW_A_FMT(Shift == 0 || Shift == 2, "Unsupported shift amount");
    constexpr uint32_t Op = 0b1011'1000'001 << 21 | (0b10 << 10);
    LoadStoreRegisterOffset(Op, 0b00, rt, rn, rm, Option, Shift ? 1 : 0);
  }
  void ldr(FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm, FEXCore::ARMEmitter::ExtendedType Option, uint32_t Shift) {
    LOGMAN_THROW_A_FMT((FEXCore::ToUnderlying(Option) & 0b010) == 0b010, "Unsupported Extendtype");
    LOGMAN_THROW_A_FMT(Shift == 0 || Shift == 2, "Unsupported shift amount: {}", Shift);
    constexpr uint32_t Op = 0b1011'1000'001 << 21 | (0b10 << 10);
    LoadStoreRegisterOffset(Op, 0b01, rt, rn, rm, Option, Shift ? 1 : 0);
  }
  void ldrsw(FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm, FEXCore::ARMEmitter::ExtendedType Option, uint32_t Shift) {
    LOGMAN_THROW_A_FMT((FEXCore::ToUnderlying(Option) & 0b010) == 0b010, "Unsupported Extendtype");
    LOGMAN_THROW_A_FMT(Shift == 0 || Shift == 2, "Unsupported shift amount");
    constexpr uint32_t Op = 0b1011'1000'001 << 21 | (0b10 << 10);
    LoadStoreRegisterOffset(Op, 0b10, rt, rn, rm, Option, Shift ? 1 : 0);
  }
  void str(FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm, FEXCore::ARMEmitter::ExtendedType Option, uint32_t Shift) {
    LOGMAN_THROW_A_FMT((FEXCore::ToUnderlying(Option) & 0b010) == 0b010, "Unsupported Extendtype");
    LOGMAN_THROW_A_FMT(Shift == 0 || Shift == 3, "Unsupported shift amount");
    constexpr uint32_t Op = 0b1111'1000'001 << 21 | (0b10 << 10);
    LoadStoreRegisterOffset(Op, 0b00, rt, rn, rm, Option, Shift ? 1 : 0);
  }
  void ldr(FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm, FEXCore::ARMEmitter::ExtendedType Option, uint32_t Shift) {
    LOGMAN_THROW_A_FMT((FEXCore::ToUnderlying(Option) & 0b010) == 0b010, "Unsupported Extendtype");
    LOGMAN_THROW_A_FMT(Shift == 0 || Shift == 3, "Unsupported shift amount");
    constexpr uint32_t Op = 0b1111'1000'001 << 21 | (0b10 << 10);
    LoadStoreRegisterOffset(Op, 0b01, rt, rn, rm, Option, Shift ? 1 : 0);
  }
  void prfm(FEXCore::ARMEmitter::Prefetch prfop, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm, FEXCore::ARMEmitter::ExtendedType Option, uint32_t Shift) {
    LOGMAN_THROW_A_FMT((FEXCore::ToUnderlying(Option) & 0b010) == 0b010, "Unsupported Extendtype");
    LOGMAN_THROW_A_FMT(Shift == 0 || Shift == 3, "Unsupported shift amount");
    constexpr uint32_t Op = 0b1111'1000'001 << 21 | (0b10 << 10);
    LoadStoreRegisterOffset(Op, 0b10, prfop, rn, rm, Option, Shift ? 1 : 0);
  }
  void strb(FEXCore::ARMEmitter::VRegister rt, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm, FEXCore::ARMEmitter::ExtendedType Option) {
    LOGMAN_THROW_A_FMT((FEXCore::ToUnderlying(Option) & 0b010) == 0b010, "Unsupported Extendtype");
    constexpr uint32_t Op = 0b0011'1100'001 << 21 | (0b10 << 10);
    LoadStoreRegisterOffset(Op, 0b00, rt, rn, rm, Option, 0);
  }
  void ldrb(FEXCore::ARMEmitter::VRegister rt, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm, FEXCore::ARMEmitter::ExtendedType Option) {
    LOGMAN_THROW_A_FMT((FEXCore::ToUnderlying(Option) & 0b010) == 0b010, "Unsupported Extendtype");
    constexpr uint32_t Op = 0b0011'1100'001 << 21 | (0b10 << 10);
    LoadStoreRegisterOffset(Op, 0b01, rt, rn, rm, Option, 0);
  }
  void strh(FEXCore::ARMEmitter::VRegister rt, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm, FEXCore::ARMEmitter::ExtendedType Option, uint32_t Shift) {
    LOGMAN_THROW_A_FMT((FEXCore::ToUnderlying(Option) & 0b010) == 0b010, "Unsupported Extendtype");
    LOGMAN_THROW_A_FMT(Shift == 0 || Shift == 1, "Unsupported shift amount");
    constexpr uint32_t Op = 0b0111'1100'001 << 21 | (0b10 << 10);
    LoadStoreRegisterOffset(Op, 0b00, rt, rn, rm, Option, Shift ? 1 : 0);
  }
  void ldrh(FEXCore::ARMEmitter::VRegister rt, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm, FEXCore::ARMEmitter::ExtendedType Option, uint32_t Shift) {
    LOGMAN_THROW_A_FMT((FEXCore::ToUnderlying(Option) & 0b010) == 0b010, "Unsupported Extendtype");
    LOGMAN_THROW_A_FMT(Shift == 0 || Shift == 1, "Unsupported shift amount");
    constexpr uint32_t Op = 0b0111'1100'001 << 21 | (0b10 << 10);
    LoadStoreRegisterOffset(Op, 0b01, rt, rn, rm, Option, Shift ? 1 : 0);
  }
  void str(FEXCore::ARMEmitter::SRegister rt, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm, FEXCore::ARMEmitter::ExtendedType Option, uint32_t Shift) {
    LOGMAN_THROW_A_FMT((FEXCore::ToUnderlying(Option) & 0b010) == 0b010, "Unsupported Extendtype");
    LOGMAN_THROW_A_FMT(Shift == 0 || Shift == 2, "Unsupported shift amount");
    constexpr uint32_t Op = 0b1011'1100'001 << 21 | (0b10 << 10);
    LoadStoreRegisterOffset(Op, 0b00, rt, rn, rm, Option, Shift ? 1 : 0);
  }
  void ldr(FEXCore::ARMEmitter::SRegister rt, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm, FEXCore::ARMEmitter::ExtendedType Option, uint32_t Shift) {
    LOGMAN_THROW_A_FMT((FEXCore::ToUnderlying(Option) & 0b010) == 0b010, "Unsupported Extendtype");
    LOGMAN_THROW_A_FMT(Shift == 0 || Shift == 2, "Unsupported shift amount");
    constexpr uint32_t Op = 0b1011'1100'001 << 21 | (0b10 << 10);
    LoadStoreRegisterOffset(Op, 0b01, rt, rn, rm, Option, Shift ? 1 : 0);
  }
  void str(FEXCore::ARMEmitter::DRegister rt, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm, FEXCore::ARMEmitter::ExtendedType Option, uint32_t Shift) {
    LOGMAN_THROW_A_FMT((FEXCore::ToUnderlying(Option) & 0b010) == 0b010, "Unsupported Extendtype");
    LOGMAN_THROW_A_FMT(Shift == 0 || Shift == 3, "Unsupported shift amount");
    constexpr uint32_t Op = 0b1111'1100'001 << 21 | (0b10 << 10);
    LoadStoreRegisterOffset(Op, 0b00, rt, rn, rm, Option, Shift ? 1 : 0);
  }
  void ldr(FEXCore::ARMEmitter::DRegister rt, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm, FEXCore::ARMEmitter::ExtendedType Option, uint32_t Shift) {
    LOGMAN_THROW_A_FMT((FEXCore::ToUnderlying(Option) & 0b010) == 0b010, "Unsupported Extendtype");
    LOGMAN_THROW_A_FMT(Shift == 0 || Shift == 3, "Unsupported shift amount");
    constexpr uint32_t Op = 0b1111'1100'001 << 21 | (0b10 << 10);
    LoadStoreRegisterOffset(Op, 0b01, rt, rn, rm, Option, Shift ? 1 : 0);
  }
  void str(FEXCore::ARMEmitter::QRegister rt, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm, FEXCore::ARMEmitter::ExtendedType Option, uint32_t Shift) {
    LOGMAN_THROW_A_FMT((FEXCore::ToUnderlying(Option) & 0b010) == 0b010, "Unsupported Extendtype");
    LOGMAN_THROW_A_FMT(Shift == 0 || Shift == 4, "Unsupported shift amount");
    constexpr uint32_t Op = 0b0011'1100'001 << 21 | (0b10 << 10);
    LoadStoreRegisterOffset(Op, 0b10, rt, rn, rm, Option, Shift ? 1 : 0);
  }
  void ldr(FEXCore::ARMEmitter::QRegister rt, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm, FEXCore::ARMEmitter::ExtendedType Option, uint32_t Shift) {
    LOGMAN_THROW_A_FMT((FEXCore::ToUnderlying(Option) & 0b010) == 0b010, "Unsupported Extendtype");
    LOGMAN_THROW_A_FMT(Shift == 0 || Shift == 4, "Unsupported shift amount");
    constexpr uint32_t Op = 0b0011'1100'001 << 21 | (0b10 << 10);
    LoadStoreRegisterOffset(Op, 0b11, rt, rn, rm, Option, Shift ? 1 : 0);
  }

  void strb(FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::ExtendedMemOperand MemSrc) {
    if (MemSrc.MetaType.Header.MemType == FEXCore::ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED &&
        MemSrc.MetaType.ExtendedType.rm.Idx() != FEXCore::ARMEmitter::Reg::r31.Idx()) {
      strb(rt, MemSrc.rn, MemSrc.MetaType.ExtendedType.rm, MemSrc.MetaType.ExtendedType.Option, MemSrc.MetaType.ExtendedType.Shift);
    }
    else if (MemSrc.MetaType.Header.MemType == FEXCore::ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED) {
      strb(rt, MemSrc.rn);
    }
    else {
      if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::OFFSET) {
        if (MemSrc.MetaType.ImmType.Imm < 0) {
          sturb(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        }
        else {
          strb(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        }
      }
      else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::POST) {
        strb<ARMEmitter::IndexType::POST>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      }
      else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::PRE) {
        strb<ARMEmitter::IndexType::PRE>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      }
      else {
        LOGMAN_MSG_A_FMT("Unexpected loadstore index type");
        FEX_UNREACHABLE;
      }
    }
  }
  void ldrb(FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::ExtendedMemOperand MemSrc) {
    if (MemSrc.MetaType.Header.MemType == FEXCore::ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED &&
        MemSrc.MetaType.ExtendedType.rm.Idx() != FEXCore::ARMEmitter::Reg::r31.Idx()) {
      ldrb(rt, MemSrc.rn, MemSrc.MetaType.ExtendedType.rm, MemSrc.MetaType.ExtendedType.Option, MemSrc.MetaType.ExtendedType.Shift);
    }
    else if (MemSrc.MetaType.Header.MemType == FEXCore::ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED) {
      ldrb(rt, MemSrc.rn);
    }
    else {
      if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::OFFSET) {
        if (MemSrc.MetaType.ImmType.Imm < 0) {
          ldurb(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        }
        else {
          ldrb(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        }
      }
      else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::POST) {
        ldrb<ARMEmitter::IndexType::POST>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      }
      else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::PRE) {
        ldrb<ARMEmitter::IndexType::PRE>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      }
      else {
        LOGMAN_MSG_A_FMT("Unexpected loadstore index type");
        FEX_UNREACHABLE;
      }
    }
  }
  void ldrsb(FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::ExtendedMemOperand MemSrc) {
    if (MemSrc.MetaType.Header.MemType == FEXCore::ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED &&
        MemSrc.MetaType.ExtendedType.rm.Idx() != FEXCore::ARMEmitter::Reg::r31.Idx()) {
      ldrsb(rt, MemSrc.rn, MemSrc.MetaType.ExtendedType.rm, MemSrc.MetaType.ExtendedType.Option, MemSrc.MetaType.ExtendedType.Shift);
    }
    else if (MemSrc.MetaType.Header.MemType == FEXCore::ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED) {
      ldrsb(rt, MemSrc.rn);
    }
    else {
      if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::OFFSET) {
        if (MemSrc.MetaType.ImmType.Imm < 0) {
          ldursb(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        }
        else {
          ldrsb(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        }
      }
      else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::POST) {
        ldrsb<ARMEmitter::IndexType::POST>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      }
      else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::PRE) {
        ldrsb<ARMEmitter::IndexType::PRE>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      }
      else {
        LOGMAN_MSG_A_FMT("Unexpected loadstore index type");
        FEX_UNREACHABLE;
      }
    }
  }
  void ldrsb(FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::ExtendedMemOperand MemSrc) {
    if (MemSrc.MetaType.Header.MemType == FEXCore::ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED &&
        MemSrc.MetaType.ExtendedType.rm.Idx() != FEXCore::ARMEmitter::Reg::r31.Idx()) {
      ldrsb(rt, MemSrc.rn, MemSrc.MetaType.ExtendedType.rm, MemSrc.MetaType.ExtendedType.Option, MemSrc.MetaType.ExtendedType.Shift);
    }
    else if (MemSrc.MetaType.Header.MemType == FEXCore::ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED) {
      ldrsb(rt, MemSrc.rn);
    }
    else {
      if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::OFFSET) {
        if (MemSrc.MetaType.ImmType.Imm < 0) {
          ldursb(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        }
        else {
          ldrsb(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        }
      }
      else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::POST) {
        ldrsb<ARMEmitter::IndexType::POST>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      }
      else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::PRE) {
        ldrsb<ARMEmitter::IndexType::PRE>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      }
      else {
        LOGMAN_MSG_A_FMT("Unexpected loadstore index type");
        FEX_UNREACHABLE;
      }
    }
  }
  void strh(FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::ExtendedMemOperand MemSrc) {
    if (MemSrc.MetaType.Header.MemType == FEXCore::ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED &&
        MemSrc.MetaType.ExtendedType.rm.Idx() != FEXCore::ARMEmitter::Reg::r31.Idx()) {
      strh(rt, MemSrc.rn, MemSrc.MetaType.ExtendedType.rm, MemSrc.MetaType.ExtendedType.Option, MemSrc.MetaType.ExtendedType.Shift);
    }
    else if (MemSrc.MetaType.Header.MemType == FEXCore::ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED) {
      strh(rt, MemSrc.rn);
    }
    else {
      if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::OFFSET) {
        if ((MemSrc.MetaType.ImmType.Imm & 0b1) || MemSrc.MetaType.ImmType.Imm < 0) {
          sturh(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        }
        else {
          strh(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        }
      }
      else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::POST) {
        strh<ARMEmitter::IndexType::POST>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      }
      else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::PRE) {
        strh<ARMEmitter::IndexType::PRE>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      }
      else {
        LOGMAN_MSG_A_FMT("Unexpected loadstore index type");
        FEX_UNREACHABLE;
      }
    }
  }
  void ldrh(FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::ExtendedMemOperand MemSrc) {
    if (MemSrc.MetaType.Header.MemType == FEXCore::ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED &&
        MemSrc.MetaType.ExtendedType.rm.Idx() != FEXCore::ARMEmitter::Reg::r31.Idx()) {
      ldrh(rt, MemSrc.rn, MemSrc.MetaType.ExtendedType.rm, MemSrc.MetaType.ExtendedType.Option, MemSrc.MetaType.ExtendedType.Shift);
    }
    else if (MemSrc.MetaType.Header.MemType == FEXCore::ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED) {
      ldrh(rt, MemSrc.rn);
    }
    else {
      if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::OFFSET) {
        if ((MemSrc.MetaType.ImmType.Imm & 0b1) || MemSrc.MetaType.ImmType.Imm < 0) {
          ldurh(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        }
        else {
          ldrh(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        }
      }
      else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::POST) {
        ldrh<ARMEmitter::IndexType::POST>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      }
      else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::PRE) {
        ldrh<ARMEmitter::IndexType::PRE>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      }
      else {
        LOGMAN_MSG_A_FMT("Unexpected loadstore index type");
        FEX_UNREACHABLE;
      }
    }
  }
  void ldrsh(FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::ExtendedMemOperand MemSrc) {
    if (MemSrc.MetaType.Header.MemType == FEXCore::ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED &&
        MemSrc.MetaType.ExtendedType.rm.Idx() != FEXCore::ARMEmitter::Reg::r31.Idx()) {
      ldrsh(rt, MemSrc.rn, MemSrc.MetaType.ExtendedType.rm, MemSrc.MetaType.ExtendedType.Option, MemSrc.MetaType.ExtendedType.Shift);
    }
    else if (MemSrc.MetaType.Header.MemType == FEXCore::ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED) {
      ldrsh(rt, MemSrc.rn);
    }
    else {
      if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::OFFSET) {
        if ((MemSrc.MetaType.ImmType.Imm & 0b1) || MemSrc.MetaType.ImmType.Imm < 0) {
          ldursh(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        }
        else {
          ldrsh(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        }
      }
      else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::POST) {
        ldrsh<ARMEmitter::IndexType::POST>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      }
      else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::PRE) {
        ldrsh<ARMEmitter::IndexType::PRE>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      }
      else {
        LOGMAN_MSG_A_FMT("Unexpected loadstore index type");
        FEX_UNREACHABLE;
      }
    }
  }
  void ldrsh(FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::ExtendedMemOperand MemSrc) {
    if (MemSrc.MetaType.Header.MemType == FEXCore::ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED &&
        MemSrc.MetaType.ExtendedType.rm.Idx() != FEXCore::ARMEmitter::Reg::r31.Idx()) {
      ldrsh(rt, MemSrc.rn, MemSrc.MetaType.ExtendedType.rm, MemSrc.MetaType.ExtendedType.Option, MemSrc.MetaType.ExtendedType.Shift);
    }
    else if (MemSrc.MetaType.Header.MemType == FEXCore::ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED) {
      ldrsh(rt, MemSrc.rn);
    }
    else {
      if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::OFFSET) {
        if ((MemSrc.MetaType.ImmType.Imm & 0b1) || MemSrc.MetaType.ImmType.Imm < 0) {
          ldursh(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        }
        else {
          ldrsh(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        }
      }
      else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::POST) {
        ldrsh<ARMEmitter::IndexType::POST>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      }
      else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::PRE) {
        ldrsh<ARMEmitter::IndexType::PRE>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      }
      else {
        LOGMAN_MSG_A_FMT("Unexpected loadstore index type");
        FEX_UNREACHABLE;
      }
    }
  }
  void str(FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::ExtendedMemOperand MemSrc) {
    if (MemSrc.MetaType.Header.MemType == FEXCore::ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED &&
        MemSrc.MetaType.ExtendedType.rm.Idx() != FEXCore::ARMEmitter::Reg::r31.Idx()) {
      str(rt, MemSrc.rn, MemSrc.MetaType.ExtendedType.rm, MemSrc.MetaType.ExtendedType.Option, MemSrc.MetaType.ExtendedType.Shift);
    }
    else if (MemSrc.MetaType.Header.MemType == FEXCore::ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED) {
      str(rt, MemSrc.rn);
    }
    else {
      if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::OFFSET) {
        if ((MemSrc.MetaType.ImmType.Imm & 0b11) || MemSrc.MetaType.ImmType.Imm < 0) {
          stur(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        }
        else {
          str(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        }
      }
      else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::POST) {
        str<ARMEmitter::IndexType::POST>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      }
      else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::PRE) {
        str<ARMEmitter::IndexType::PRE>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      }
      else {
        LOGMAN_MSG_A_FMT("Unexpected loadstore index type");
        FEX_UNREACHABLE;
      }
    }
  }
  void ldr(FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::ExtendedMemOperand MemSrc) {
    if (MemSrc.MetaType.Header.MemType == FEXCore::ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED &&
        MemSrc.MetaType.ExtendedType.rm.Idx() != FEXCore::ARMEmitter::Reg::r31.Idx()) {
      ldr(rt, MemSrc.rn, MemSrc.MetaType.ExtendedType.rm, MemSrc.MetaType.ExtendedType.Option, MemSrc.MetaType.ExtendedType.Shift);
    }
    else if (MemSrc.MetaType.Header.MemType == FEXCore::ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED) {
      ldr(rt, MemSrc.rn);
    }
    else {
      if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::OFFSET) {
        if ((MemSrc.MetaType.ImmType.Imm & 0b11) || MemSrc.MetaType.ImmType.Imm < 0) {
          ldur(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        }
        else {
          ldr(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        }
      }
      else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::POST) {
        ldr<ARMEmitter::IndexType::POST>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      }
      else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::PRE) {
        ldr<ARMEmitter::IndexType::PRE>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      }
      else {
        LOGMAN_MSG_A_FMT("Unexpected loadstore index type");
        FEX_UNREACHABLE;
      }
    }
  }
  void ldrsw(FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::ExtendedMemOperand MemSrc) {
    if (MemSrc.MetaType.Header.MemType == FEXCore::ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED &&
        MemSrc.MetaType.ExtendedType.rm.Idx() != FEXCore::ARMEmitter::Reg::r31.Idx()) {
      ldrsw(rt, MemSrc.rn, MemSrc.MetaType.ExtendedType.rm, MemSrc.MetaType.ExtendedType.Option, MemSrc.MetaType.ExtendedType.Shift);
    }
    else if (MemSrc.MetaType.Header.MemType == FEXCore::ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED) {
      ldrsw(rt, MemSrc.rn);
    }
    else {
      if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::OFFSET) {
        if ((MemSrc.MetaType.ImmType.Imm & 0b11) || MemSrc.MetaType.ImmType.Imm < 0) {
          ldursw(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        }
        else {
          ldrsw(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        }
      }
      else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::POST) {
        ldrsw<ARMEmitter::IndexType::POST>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      }
      else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::PRE) {
        ldrsw<ARMEmitter::IndexType::PRE>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      }
      else {
        LOGMAN_MSG_A_FMT("Unexpected loadstore index type");
        FEX_UNREACHABLE;
      }
    }
  }
  void str(FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::ExtendedMemOperand MemSrc) {
    if (MemSrc.MetaType.Header.MemType == FEXCore::ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED &&
        MemSrc.MetaType.ExtendedType.rm.Idx() != FEXCore::ARMEmitter::Reg::r31.Idx()) {
      str(rt, MemSrc.rn, MemSrc.MetaType.ExtendedType.rm, MemSrc.MetaType.ExtendedType.Option, MemSrc.MetaType.ExtendedType.Shift);
    }
    else if (MemSrc.MetaType.Header.MemType == FEXCore::ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED) {
      str(rt, MemSrc.rn);
    }
    else {
      if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::OFFSET) {
        if ((MemSrc.MetaType.ImmType.Imm & 0b111) || MemSrc.MetaType.ImmType.Imm < 0) {
          stur(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        }
        else {
          str(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        }
      }
      else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::POST) {
        str<ARMEmitter::IndexType::POST>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      }
      else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::PRE) {
        str<ARMEmitter::IndexType::PRE>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      }
      else {
        LOGMAN_MSG_A_FMT("Unexpected loadstore index type");
        FEX_UNREACHABLE;
      }
    }
  }
  void ldr(FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::ExtendedMemOperand MemSrc) {
    if (MemSrc.MetaType.Header.MemType == FEXCore::ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED &&
        MemSrc.MetaType.ExtendedType.rm.Idx() != FEXCore::ARMEmitter::Reg::r31.Idx()) {
      ldr(rt, MemSrc.rn, MemSrc.MetaType.ExtendedType.rm, MemSrc.MetaType.ExtendedType.Option, MemSrc.MetaType.ExtendedType.Shift);
    }
    else if (MemSrc.MetaType.Header.MemType == FEXCore::ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED) {
      ldr(rt, MemSrc.rn);
    }
    else {
      if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::OFFSET) {
        if ((MemSrc.MetaType.ImmType.Imm & 0b111) || MemSrc.MetaType.ImmType.Imm < 0) {
          ldur(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        }
        else {
          ldr(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        }
      }
      else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::POST) {
        ldr<ARMEmitter::IndexType::POST>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      }
      else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::PRE) {
        ldr<ARMEmitter::IndexType::PRE>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      }
      else {
        LOGMAN_MSG_A_FMT("Unexpected loadstore index type");
        FEX_UNREACHABLE;
      }
    }
  }
  void prfm(FEXCore::ARMEmitter::Prefetch prfop, FEXCore::ARMEmitter::ExtendedMemOperand MemSrc) {
    if (MemSrc.MetaType.Header.MemType == FEXCore::ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED &&
        MemSrc.MetaType.ExtendedType.rm.Idx() != FEXCore::ARMEmitter::Reg::r31.Idx()) {
      prfm(prfop, MemSrc.rn, MemSrc.MetaType.ExtendedType.rm, MemSrc.MetaType.ExtendedType.Option, MemSrc.MetaType.ExtendedType.Shift);
    }
    else if (MemSrc.MetaType.Header.MemType == FEXCore::ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED) {
      prfm(prfop, MemSrc.rn);
    }
    else {
      if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::OFFSET) {
        prfm(prfop, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      }
      else {
        LOGMAN_MSG_A_FMT("Unexpected loadstore index type");
        FEX_UNREACHABLE;
      }
    }
  }

  void strb(FEXCore::ARMEmitter::VRegister rt, FEXCore::ARMEmitter::ExtendedMemOperand MemSrc) {
    if (MemSrc.MetaType.Header.MemType == FEXCore::ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED &&
        MemSrc.MetaType.ExtendedType.rm.Idx() != FEXCore::ARMEmitter::Reg::r31.Idx()) {
      LOGMAN_THROW_AA_FMT(MemSrc.MetaType.ExtendedType.Shift == false, "Can't shift byte");
      LOGMAN_MSG_A_FMT("Nope"); // XXX: Implement
      // strb(rt, MemSrc.rn, MemSrc.MetaType.ExtendedType.rm, MemSrc.MetaType.ExtendedType.Option);
    }
    else if (MemSrc.MetaType.Header.MemType == FEXCore::ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED) {
      strb(rt, MemSrc.rn);
    }
    else {
      if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::OFFSET) {
        if (MemSrc.MetaType.ImmType.Imm < 0) {
          sturb(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        }
        else {
          strb(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        }
      }
      else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::POST) {
        strb<ARMEmitter::IndexType::POST>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      }
      else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::PRE) {
        strb<ARMEmitter::IndexType::PRE>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      }
      else {
        LOGMAN_MSG_A_FMT("Unexpected loadstore index type");
        FEX_UNREACHABLE;
      }
    }
  }
  void ldrb(FEXCore::ARMEmitter::VRegister rt, FEXCore::ARMEmitter::ExtendedMemOperand MemSrc) {
    if (MemSrc.MetaType.Header.MemType == FEXCore::ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED &&
        MemSrc.MetaType.ExtendedType.rm.Idx() != FEXCore::ARMEmitter::Reg::r31.Idx()) {
      LOGMAN_THROW_AA_FMT(MemSrc.MetaType.ExtendedType.Shift == false, "Can't shift byte");
      LOGMAN_MSG_A_FMT("Nope"); // XXX: Implement
      // ldrb(rt, MemSrc.rn, MemSrc.MetaType.ExtendedType.rm, MemSrc.MetaType.ExtendedType.Option);
    }
    else if (MemSrc.MetaType.Header.MemType == FEXCore::ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED) {
      ldrb(rt, MemSrc.rn);
    }
    else {
      if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::OFFSET) {
        if (MemSrc.MetaType.ImmType.Imm < 0) {
          ldurb(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        }
        else {
          ldrb(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        }
      }
      else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::POST) {
        ldrb<ARMEmitter::IndexType::POST>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      }
      else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::PRE) {
        ldrb<ARMEmitter::IndexType::PRE>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      }
      else {
        LOGMAN_MSG_A_FMT("Unexpected loadstore index type");
        FEX_UNREACHABLE;
      }
    }
  }
  void strh(FEXCore::ARMEmitter::VRegister rt, FEXCore::ARMEmitter::ExtendedMemOperand MemSrc) {
    if (MemSrc.MetaType.Header.MemType == FEXCore::ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED &&
        MemSrc.MetaType.ExtendedType.rm.Idx() != FEXCore::ARMEmitter::Reg::r31.Idx()) {
      LOGMAN_THROW_AA_FMT(MemSrc.MetaType.ExtendedType.Shift == false, "Can't shift byte");
      LOGMAN_MSG_A_FMT("Nope"); // XXX: Implement
      // strh(rt, MemSrc.rn, MemSrc.MetaType.ExtendedType.rm, MemSrc.MetaType.ExtendedType.Option);
    }
    else if (MemSrc.MetaType.Header.MemType == FEXCore::ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED) {
      strh(rt, MemSrc.rn);
    }
    else {
      if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::OFFSET) {
        if ((MemSrc.MetaType.ImmType.Imm & 0b1) || MemSrc.MetaType.ImmType.Imm < 0) {
          sturh(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        }
        else {
          strh(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        }
      }
      else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::POST) {
        strh<ARMEmitter::IndexType::POST>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      }
      else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::PRE) {
        strh<ARMEmitter::IndexType::PRE>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      }
      else {
        LOGMAN_MSG_A_FMT("Unexpected loadstore index type");
        FEX_UNREACHABLE;
      }
    }
  }
  void ldrh(FEXCore::ARMEmitter::VRegister rt, FEXCore::ARMEmitter::ExtendedMemOperand MemSrc) {
    if (MemSrc.MetaType.Header.MemType == FEXCore::ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED &&
        MemSrc.MetaType.ExtendedType.rm.Idx() != FEXCore::ARMEmitter::Reg::r31.Idx()) {
      LOGMAN_THROW_AA_FMT(MemSrc.MetaType.ExtendedType.Shift == false, "Can't shift byte");
      LOGMAN_MSG_A_FMT("Nope"); // XXX: Implement
      // ldrh(rt, MemSrc.rn, MemSrc.MetaType.ExtendedType.rm, MemSrc.MetaType.ExtendedType.Option);
    }
    else if (MemSrc.MetaType.Header.MemType == FEXCore::ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED) {
      ldrh(rt, MemSrc.rn);
    }
    else {
      if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::OFFSET) {
        if ((MemSrc.MetaType.ImmType.Imm & 0b1) || MemSrc.MetaType.ImmType.Imm < 0) {
          ldurh(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        }
        else {
          ldrh(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        }
      }
      else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::POST) {
        ldrh<ARMEmitter::IndexType::POST>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      }
      else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::PRE) {
        ldrh<ARMEmitter::IndexType::PRE>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      }
      else {
        LOGMAN_MSG_A_FMT("Unexpected loadstore index type");
        FEX_UNREACHABLE;
      }
    }
  }
  void str(FEXCore::ARMEmitter::SRegister rt, FEXCore::ARMEmitter::ExtendedMemOperand MemSrc) {
    if (MemSrc.MetaType.Header.MemType == FEXCore::ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED &&
        MemSrc.MetaType.ExtendedType.rm.Idx() != FEXCore::ARMEmitter::Reg::r31.Idx()) {
      str(rt, MemSrc.rn, MemSrc.MetaType.ExtendedType.rm, MemSrc.MetaType.ExtendedType.Option, MemSrc.MetaType.ExtendedType.Shift);
    }
    else if (MemSrc.MetaType.Header.MemType == FEXCore::ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED) {
      str(rt, MemSrc.rn);
    }
    else {
      if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::OFFSET) {
        if ((MemSrc.MetaType.ImmType.Imm & 0b11) || MemSrc.MetaType.ImmType.Imm < 0) {
          stur(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        }
        else {
          str(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        }
      }
      else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::POST) {
        str<ARMEmitter::IndexType::POST>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      }
      else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::PRE) {
        str<ARMEmitter::IndexType::PRE>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      }
      else {
        LOGMAN_MSG_A_FMT("Unexpected loadstore index type");
        FEX_UNREACHABLE;
      }
    }
  }
  void ldr(FEXCore::ARMEmitter::SRegister rt, FEXCore::ARMEmitter::ExtendedMemOperand MemSrc) {
    if (MemSrc.MetaType.Header.MemType == FEXCore::ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED &&
        MemSrc.MetaType.ExtendedType.rm.Idx() != FEXCore::ARMEmitter::Reg::r31.Idx()) {
      ldr(rt, MemSrc.rn, MemSrc.MetaType.ExtendedType.rm, MemSrc.MetaType.ExtendedType.Option, MemSrc.MetaType.ExtendedType.Shift);
    }
    else if (MemSrc.MetaType.Header.MemType == FEXCore::ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED) {
      ldr(rt, MemSrc.rn);
    }
    else {
      if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::OFFSET) {
        if ((MemSrc.MetaType.ImmType.Imm & 0b11) || MemSrc.MetaType.ImmType.Imm < 0) {
          ldur(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        }
        else {
          ldr(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        }
      }
      else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::POST) {
        ldr<ARMEmitter::IndexType::POST>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      }
      else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::PRE) {
        ldr<ARMEmitter::IndexType::PRE>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      }
      else {
        LOGMAN_MSG_A_FMT("Unexpected loadstore index type");
        FEX_UNREACHABLE;
      }
    }
  }
  void str(FEXCore::ARMEmitter::DRegister rt, FEXCore::ARMEmitter::ExtendedMemOperand MemSrc) {
    if (MemSrc.MetaType.Header.MemType == FEXCore::ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED &&
        MemSrc.MetaType.ExtendedType.rm.Idx() != FEXCore::ARMEmitter::Reg::r31.Idx()) {
      str(rt, MemSrc.rn, MemSrc.MetaType.ExtendedType.rm, MemSrc.MetaType.ExtendedType.Option, MemSrc.MetaType.ExtendedType.Shift);
    }
    else if (MemSrc.MetaType.Header.MemType == FEXCore::ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED) {
      str(rt, MemSrc.rn);
    }
    else {
      if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::OFFSET) {
        if ((MemSrc.MetaType.ImmType.Imm & 0b111) || MemSrc.MetaType.ImmType.Imm < 0) {
          stur(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        }
        else {
          str(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        }
      }
      else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::POST) {
        str<ARMEmitter::IndexType::POST>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      }
      else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::PRE) {
        str<ARMEmitter::IndexType::PRE>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      }
      else {
        LOGMAN_MSG_A_FMT("Unexpected loadstore index type");
        FEX_UNREACHABLE;
      }
    }
  }
  void ldr(FEXCore::ARMEmitter::DRegister rt, FEXCore::ARMEmitter::ExtendedMemOperand MemSrc) {
    if (MemSrc.MetaType.Header.MemType == FEXCore::ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED &&
        MemSrc.MetaType.ExtendedType.rm.Idx() != FEXCore::ARMEmitter::Reg::r31.Idx()) {
      ldr(rt, MemSrc.rn, MemSrc.MetaType.ExtendedType.rm, MemSrc.MetaType.ExtendedType.Option, MemSrc.MetaType.ExtendedType.Shift);
    }
    else if (MemSrc.MetaType.Header.MemType == FEXCore::ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED) {
      ldr(rt, MemSrc.rn);
    }
    else {
      if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::OFFSET) {
        if ((MemSrc.MetaType.ImmType.Imm & 0b111) || MemSrc.MetaType.ImmType.Imm < 0) {
          ldur(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        }
        else {
          ldr(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        }
      }
      else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::POST) {
        ldr<ARMEmitter::IndexType::POST>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      }
      else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::PRE) {
        ldr<ARMEmitter::IndexType::PRE>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      }
      else {
        LOGMAN_MSG_A_FMT("Unexpected loadstore index type");
        FEX_UNREACHABLE;
      }
    }
  }
  void str(FEXCore::ARMEmitter::QRegister rt, FEXCore::ARMEmitter::ExtendedMemOperand MemSrc) {
    if (MemSrc.MetaType.Header.MemType == FEXCore::ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED &&
        MemSrc.MetaType.ExtendedType.rm.Idx() != FEXCore::ARMEmitter::Reg::r31.Idx()) {
      str(rt, MemSrc.rn, MemSrc.MetaType.ExtendedType.rm, MemSrc.MetaType.ExtendedType.Option, MemSrc.MetaType.ExtendedType.Shift);
    }
    else if (MemSrc.MetaType.Header.MemType == FEXCore::ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED) {
      str(rt, MemSrc.rn);
    }
    else {
      if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::OFFSET) {
        if ((MemSrc.MetaType.ImmType.Imm & 0b1111) || MemSrc.MetaType.ImmType.Imm < 0) {
          stur(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        }
        else {
          str(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        }
      }
      else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::POST) {
        str<ARMEmitter::IndexType::POST>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      }
      else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::PRE) {
        str<ARMEmitter::IndexType::PRE>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      }
      else {
        LOGMAN_MSG_A_FMT("Unexpected loadstore index type");
        FEX_UNREACHABLE;
      }
    }
  }
  void ldr(FEXCore::ARMEmitter::QRegister rt, FEXCore::ARMEmitter::ExtendedMemOperand MemSrc) {
    if (MemSrc.MetaType.Header.MemType == FEXCore::ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED &&
        MemSrc.MetaType.ExtendedType.rm.Idx() != FEXCore::ARMEmitter::Reg::r31.Idx()) {
      ldr(rt, MemSrc.rn, MemSrc.MetaType.ExtendedType.rm, MemSrc.MetaType.ExtendedType.Option, MemSrc.MetaType.ExtendedType.Shift);
    }
    else if (MemSrc.MetaType.Header.MemType == FEXCore::ARMEmitter::ExtendedMemOperand::Type::TYPE_EXTENDED) {
      ldr(rt, MemSrc.rn);
    }
    else {
      if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::OFFSET) {
        if ((MemSrc.MetaType.ImmType.Imm & 0b1111) || MemSrc.MetaType.ImmType.Imm < 0) {
          ldur(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        }
        else {
          ldr(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
        }
      }
      else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::POST) {
        ldr<ARMEmitter::IndexType::POST>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      }
      else if (MemSrc.MetaType.ImmType.Index == ARMEmitter::IndexType::PRE) {
        ldr<ARMEmitter::IndexType::PRE>(rt, MemSrc.rn, MemSrc.MetaType.ImmType.Imm);
      }
      else {
        LOGMAN_MSG_A_FMT("Unexpected loadstore index type");
        FEX_UNREACHABLE;
      }
    }
  }

  // Loadstore PAC
  // TODO

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
    LOGMAN_THROW_AA_FMT(size != SubRegSize::i128Bit, "Can't use 128-bit size");
    LoadStoreUnsigned(FEXCore::ToUnderlying(size), 0, 0b01, rt, rn, Imm);
  }
  void str(SubRegSize size, Register rt, Register rn, uint32_t Imm = 0) {
    LOGMAN_THROW_AA_FMT(size != SubRegSize::i128Bit, "Can't use 128-bit size");
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
  void AtomicOp(uint32_t Op, FEXCore::ARMEmitter::Size s, uint32_t L, uint32_t o0, FEXCore::ARMEmitter::Register rs, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rt2, FEXCore::ARMEmitter::Register rn) {
    const uint32_t sz = s == FEXCore::ARMEmitter::Size::i64Bit ? (1U << 30) : 0;
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
  void SubAtomicOp(uint32_t Op, FEXCore::ARMEmitter::SubRegSize s, uint32_t L, uint32_t o0, T rs, T rt, T rt2, FEXCore::ARMEmitter::Register rn) {
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
  void SubAtomicImm(uint32_t Op, FEXCore::ARMEmitter::SubRegSize s, uint32_t opc, T rt, FEXCore::ARMEmitter::Register rn, uint32_t Imm) {
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

  // Loadstore no-allocate pair
  template<typename T>
  void LoadStoreNoAllocate(uint32_t Op, T rt, T rt2, FEXCore::ARMEmitter::Register rn, uint32_t Imm) {
    uint32_t Instr = Op;

    Instr |= Imm << 15;
    Instr |= Encode_rt2(rt2);
    Instr |= Encode_rn(rn);
    Instr |= Encode_rt(rt);
    dc32(Instr);
  }
  // Loadstore register pair post-indexed
  template<typename T>
  void LoadStorePair(uint32_t Op, T rt, T rt2, FEXCore::ARMEmitter::Register rn, uint32_t Imm) {
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
  void LoadStoreImm(uint32_t Op, uint32_t o2, T rt, FEXCore::ARMEmitter::Register rn, uint32_t Imm) {
    uint32_t Instr = Op;

    Instr |= Imm << 12;
    Instr |= o2 << 10;
    Instr |= Encode_rn(rn);
    Instr |= Encode_rt(rt);
    dc32(Instr);
  }

  // Atomic memory operations
  template<typename T>
  void LoadStoreAtomicLSE(uint32_t Op, FEXCore::ARMEmitter::SubRegSize s, uint32_t A, uint32_t R, uint32_t o3, uint32_t opc, T rs, T rt, FEXCore::ARMEmitter::Register rn) {
    const uint32_t sz = FEXCore::ToUnderlying(s) << 30;
    uint32_t Instr = Op;

    Instr |= sz;
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
  void LoadStoreRegisterOffset(uint32_t Op, uint32_t opc, T rt, FEXCore::ARMEmitter::Register rn, FEXCore::ARMEmitter::Register rm, FEXCore::ARMEmitter::ExtendedType Option, uint32_t Shift) {
    uint32_t Instr = Op;

    Instr |= opc << 22;
    Instr |= Encode_rt(rt);
    Instr |= FEXCore::ToUnderlying(Option) << 13;
    Instr |= Shift << 12;
    Instr |= Encode_rn(rn);
    Instr |= Encode_rm(rm);
    dc32(Instr);
  }

  // Loadstore unsigned immediate
  template <typename T>
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
  void ldp_w(FEXCore::ARMEmitter::VRegister rt, FEXCore::ARMEmitter::VRegister rt2, FEXCore::ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 252 && ((Imm & 0b11) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = (0b0010'1100'01 << 22) |
      (
        Index == IndexType::POST   ? (0b01 << 23) :
        Index == IndexType::PRE    ? (0b11 << 23) :
        Index == IndexType::OFFSET ? (0b10 << 23) : -1
      );

    LoadStorePair(Op, rt, rt2, rn, (Imm >> 2) & 0b111'1111);
  }
  template<IndexType Index>
  void ldp_x(FEXCore::ARMEmitter::VRegister rt, FEXCore::ARMEmitter::VRegister rt2, FEXCore::ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -512 && Imm <= 504 && ((Imm & 0b111) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = (0b0110'1100'01 << 22) |
      (
        Index == IndexType::POST   ? (0b01 << 23) :
        Index == IndexType::PRE    ? (0b11 << 23) :
        Index == IndexType::OFFSET ? (0b10 << 23) : -1
      );

    LoadStorePair(Op, rt, rt2, rn, (Imm >> 3) & 0b111'1111);
  }
  template<IndexType Index>
  void stp_w(FEXCore::ARMEmitter::VRegister rt, FEXCore::ARMEmitter::VRegister rt2, FEXCore::ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 252 && ((Imm & 0b11) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = (0b0010'1100'00 << 22) |
      (
        Index == IndexType::POST   ? (0b01 << 23) :
        Index == IndexType::PRE    ? (0b11 << 23) :
        Index == IndexType::OFFSET ? (0b10 << 23) : -1
      );

    LoadStorePair(Op, rt, rt2, rn, (Imm >> 2) & 0b111'1111);
  }
  template<IndexType Index>
  void stp_x(FEXCore::ARMEmitter::VRegister rt, FEXCore::ARMEmitter::VRegister rt2, FEXCore::ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -512 && Imm <= 504 && ((Imm & 0b111) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = (0b0110'1100'00 << 22) |
      (
        Index == IndexType::POST   ? (0b01 << 23) :
        Index == IndexType::PRE    ? (0b11 << 23) :
        Index == IndexType::OFFSET ? (0b10 << 23) : -1
      );

    LoadStorePair(Op, rt, rt2, rn, (Imm >> 3) & 0b111'1111);
  }
  template<IndexType Index>
  void ldp_q(FEXCore::ARMEmitter::VRegister rt, FEXCore::ARMEmitter::VRegister rt2, FEXCore::ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -1024 && Imm <= 1008 && ((Imm & 0b1111) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = (0b1010'1100'01 << 22) |
      (
        Index == IndexType::POST   ? (0b01 << 23) :
        Index == IndexType::PRE    ? (0b11 << 23) :
        Index == IndexType::OFFSET ? (0b10 << 23) : -1
      );

    LoadStorePair(Op, rt, rt2, rn, (Imm >> 4) & 0b111'1111);
  }
  template<IndexType Index>
  void stp_q(FEXCore::ARMEmitter::VRegister rt, FEXCore::ARMEmitter::VRegister rt2, FEXCore::ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -1024 && Imm <= 1008 && ((Imm & 0b1111) == 0), "Unscaled offset too large");
    constexpr uint32_t Op = (0b1010'1100'00 << 22) |
      (
        Index == IndexType::POST   ? (0b01 << 23) :
        Index == IndexType::PRE    ? (0b11 << 23) :
        Index == IndexType::OFFSET ? (0b10 << 23) : -1
      );

    LoadStorePair(Op, rt, rt2, rn, (Imm >> 4) & 0b111'1111);
  }

  template <IndexType Index>
  void stXrb(FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");

    constexpr uint32_t Op = 0b0011'1000'00 << 22;
    constexpr uint32_t o2 =
      Index == IndexType::POST   ? 0b01 :
      Index == IndexType::PRE    ? 0b11 :
      Index == IndexType::OFFSET ? 0b00 :
      Index == IndexType::UNPRIVILEGED ? 0b10 :  -1;

    LoadStoreImm(Op, o2, rt, rn, Imm & 0b1'1111'1111);
  }
  template <IndexType Index>
  void ldXrb(FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");

    constexpr uint32_t Op = 0b0011'1000'01 << 22;
    constexpr uint32_t o2 =
      Index == IndexType::POST   ? 0b01 :
      Index == IndexType::PRE    ? 0b11 :
      Index == IndexType::OFFSET ? 0b00 :
      Index == IndexType::UNPRIVILEGED ? 0b10 :  -1;

    LoadStoreImm(Op, o2, rt, rn, Imm & 0b1'1111'1111);
  }
  template <IndexType Index>
  void stXrb(FEXCore::ARMEmitter::VRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");

    constexpr uint32_t Op = 0b0011'1100'00 << 22;
    constexpr uint32_t o2 =
      Index == IndexType::POST   ? 0b01 :
      Index == IndexType::PRE    ? 0b11 :
      Index == IndexType::OFFSET ? 0b00 :
      Index == IndexType::UNPRIVILEGED ? 0b10 :  -1;

    LoadStoreImm(Op, o2, rt, rn, Imm & 0b1'1111'1111);
  }
  template <IndexType Index>
  void ldXrb(FEXCore::ARMEmitter::VRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");

    constexpr uint32_t Op = 0b0011'1100'01 << 22;
    constexpr uint32_t o2 =
      Index == IndexType::POST   ? 0b01 :
      Index == IndexType::PRE    ? 0b11 :
      Index == IndexType::OFFSET ? 0b00 :
      Index == IndexType::UNPRIVILEGED ? 0b10 :  -1;

    LoadStoreImm(Op, o2, rt, rn, Imm & 0b1'1111'1111);
  }
  template <IndexType Index>
  void ldXrsb(FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");

    constexpr uint32_t Op = 0b0011'1000'10 << 22;
    constexpr uint32_t o2 =
      Index == IndexType::POST   ? 0b01 :
      Index == IndexType::PRE    ? 0b11 :
      Index == IndexType::OFFSET ? 0b00 :
      Index == IndexType::UNPRIVILEGED ? 0b10 :  -1;

    LoadStoreImm(Op, o2, rt, rn, Imm & 0b1'1111'1111);
  }
  template <IndexType Index>
  void ldXrsb(FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");

    constexpr uint32_t Op = 0b0011'1000'11 << 22;
    constexpr uint32_t o2 =
      Index == IndexType::POST   ? 0b01 :
      Index == IndexType::PRE    ? 0b11 :
      Index == IndexType::OFFSET ? 0b00 :
      Index == IndexType::UNPRIVILEGED ? 0b10 :  -1;

    LoadStoreImm(Op, o2, rt, rn, Imm & 0b1'1111'1111);
  }
  template <IndexType Index>
  void stXrh(FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");

    constexpr uint32_t Op = 0b0111'1000'00 << 22;
    constexpr uint32_t o2 =
      Index == IndexType::POST   ? 0b01 :
      Index == IndexType::PRE    ? 0b11 :
      Index == IndexType::OFFSET ? 0b00 :
      Index == IndexType::UNPRIVILEGED ? 0b10 :  -1;

    LoadStoreImm(Op, o2, rt, rn, Imm & 0b1'1111'1111);
  }
  template <IndexType Index>
  void ldXrh(FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");

    constexpr uint32_t Op = 0b0111'1000'01 << 22;
    constexpr uint32_t o2 =
      Index == IndexType::POST   ? 0b01 :
      Index == IndexType::PRE    ? 0b11 :
      Index == IndexType::OFFSET ? 0b00 :
      Index == IndexType::UNPRIVILEGED ? 0b10 :  -1;

    LoadStoreImm(Op, o2, rt, rn, Imm & 0b1'1111'1111);
  }
  template <IndexType Index>
  void stXrh(FEXCore::ARMEmitter::VRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");

    constexpr uint32_t Op = 0b0111'1100'00 << 22;
    constexpr uint32_t o2 =
      Index == IndexType::POST   ? 0b01 :
      Index == IndexType::PRE    ? 0b11 :
      Index == IndexType::OFFSET ? 0b00 :
      Index == IndexType::UNPRIVILEGED ? 0b10 :  -1;

    LoadStoreImm(Op, o2, rt, rn, Imm & 0b1'1111'1111);
  }
  template <IndexType Index>
  void ldXrh(FEXCore::ARMEmitter::VRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");

    constexpr uint32_t Op = 0b0111'1100'01 << 22;
    constexpr uint32_t o2 =
      Index == IndexType::POST   ? 0b01 :
      Index == IndexType::PRE    ? 0b11 :
      Index == IndexType::OFFSET ? 0b00 :
      Index == IndexType::UNPRIVILEGED ? 0b10 :  -1;

    LoadStoreImm(Op, o2, rt, rn, Imm & 0b1'1111'1111);
  }
  template <IndexType Index>
  void ldXrsh(FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");

    constexpr uint32_t Op = 0b0111'1000'10 << 22;
    constexpr uint32_t o2 =
      Index == IndexType::POST   ? 0b01 :
      Index == IndexType::PRE    ? 0b11 :
      Index == IndexType::OFFSET ? 0b00 :
      Index == IndexType::UNPRIVILEGED ? 0b10 :  -1;

    LoadStoreImm(Op, o2, rt, rn, Imm & 0b1'1111'1111);
  }
  template <IndexType Index>
  void ldXrsh(FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");

    constexpr uint32_t Op = 0b0111'1000'11 << 22;
    constexpr uint32_t o2 =
      Index == IndexType::POST   ? 0b01 :
      Index == IndexType::PRE    ? 0b11 :
      Index == IndexType::OFFSET ? 0b00 :
      Index == IndexType::UNPRIVILEGED ? 0b10 :  -1;

    LoadStoreImm(Op, o2, rt, rn, Imm & 0b1'1111'1111);
  }
  template <IndexType Index>
  void stXr(FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");

    constexpr uint32_t Op = 0b1011'1000'00 << 22;
    constexpr uint32_t o2 =
      Index == IndexType::POST   ? 0b01 :
      Index == IndexType::PRE    ? 0b11 :
      Index == IndexType::OFFSET ? 0b00 :
      Index == IndexType::UNPRIVILEGED ? 0b10 :  -1;

    LoadStoreImm(Op, o2, rt, rn, Imm & 0b1'1111'1111);
  }
  template <IndexType Index>
  void ldXr(FEXCore::ARMEmitter::WRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");

    constexpr uint32_t Op = 0b1011'1000'01 << 22;
    constexpr uint32_t o2 =
      Index == IndexType::POST   ? 0b01 :
      Index == IndexType::PRE    ? 0b11 :
      Index == IndexType::OFFSET ? 0b00 :
      Index == IndexType::UNPRIVILEGED ? 0b10 :  -1;

    LoadStoreImm(Op, o2, rt, rn, Imm & 0b1'1111'1111);
  }
  template <IndexType Index>
  void stXr(FEXCore::ARMEmitter::SRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");

    constexpr uint32_t Op = 0b1011'1100'00 << 22;
    constexpr uint32_t o2 =
      Index == IndexType::POST   ? 0b01 :
      Index == IndexType::PRE    ? 0b11 :
      Index == IndexType::OFFSET ? 0b00 :
      Index == IndexType::UNPRIVILEGED ? 0b10 :  -1;

    LoadStoreImm(Op, o2, rt, rn, Imm & 0b1'1111'1111);
  }
  template <IndexType Index>
  void ldXr(FEXCore::ARMEmitter::SRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");

    constexpr uint32_t Op = 0b1011'1100'01 << 22;
    constexpr uint32_t o2 =
      Index == IndexType::POST   ? 0b01 :
      Index == IndexType::PRE    ? 0b11 :
      Index == IndexType::OFFSET ? 0b00 :
      Index == IndexType::UNPRIVILEGED ? 0b10 :  -1;

    LoadStoreImm(Op, o2, rt, rn, Imm & 0b1'1111'1111);
  }
  template <IndexType Index>
  void ldXrsw(FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");

    constexpr uint32_t Op = 0b1011'1000'10 << 22;
    constexpr uint32_t o2 =
      Index == IndexType::POST   ? 0b01 :
      Index == IndexType::PRE    ? 0b11 :
      Index == IndexType::OFFSET ? 0b00 :
      Index == IndexType::UNPRIVILEGED ? 0b10 :  -1;

    LoadStoreImm(Op, o2, rt, rn, Imm & 0b1'1111'1111);
  }
  template <IndexType Index>
  void stXr(FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");

    constexpr uint32_t Op = 0b1111'1000'00 << 22;
    constexpr uint32_t o2 =
      Index == IndexType::POST   ? 0b01 :
      Index == IndexType::PRE    ? 0b11 :
      Index == IndexType::OFFSET ? 0b00 :
      Index == IndexType::UNPRIVILEGED ? 0b10 :  -1;

    LoadStoreImm(Op, o2, rt, rn, Imm & 0b1'1111'1111);
  }
  template <IndexType Index>
  void ldXr(FEXCore::ARMEmitter::XRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");

    constexpr uint32_t Op = 0b1111'1000'01 << 22;
    constexpr uint32_t o2 =
      Index == IndexType::POST   ? 0b01 :
      Index == IndexType::PRE    ? 0b11 :
      Index == IndexType::OFFSET ? 0b00 :
      Index == IndexType::UNPRIVILEGED ? 0b10 :  -1;

    LoadStoreImm(Op, o2, rt, rn, Imm & 0b1'1111'1111);
  }
  template <IndexType Index>
  void stXr(FEXCore::ARMEmitter::DRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");

    constexpr uint32_t Op = 0b1111'1100'00 << 22;
    constexpr uint32_t o2 =
      Index == IndexType::POST   ? 0b01 :
      Index == IndexType::PRE    ? 0b11 :
      Index == IndexType::OFFSET ? 0b00 :
      Index == IndexType::UNPRIVILEGED ? 0b10 :  -1;

    LoadStoreImm(Op, o2, rt, rn, Imm & 0b1'1111'1111);
  }
  template <IndexType Index>
  void ldXr(FEXCore::ARMEmitter::DRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");

    constexpr uint32_t Op = 0b1111'1100'01 << 22;
    constexpr uint32_t o2 =
      Index == IndexType::POST   ? 0b01 :
      Index == IndexType::PRE    ? 0b11 :
      Index == IndexType::OFFSET ? 0b00 :
      Index == IndexType::UNPRIVILEGED ? 0b10 :  -1;

    LoadStoreImm(Op, o2, rt, rn, Imm & 0b1'1111'1111);
  }
  template <IndexType Index>
  void stXr(FEXCore::ARMEmitter::QRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");

    constexpr uint32_t Op = 0b0011'1100'10 << 22;
    constexpr uint32_t o2 =
      Index == IndexType::POST   ? 0b01 :
      Index == IndexType::PRE    ? 0b11 :
      Index == IndexType::OFFSET ? 0b00 :
      Index == IndexType::UNPRIVILEGED ? 0b10 :  -1;

    LoadStoreImm(Op, o2, rt, rn, Imm & 0b1'1111'1111);
  }
  template <IndexType Index>
  void ldXr(FEXCore::ARMEmitter::QRegister rt, FEXCore::ARMEmitter::Register rn, int32_t Imm) {
    LOGMAN_THROW_A_FMT(Imm >= -256 && Imm <= 255, "Unscaled offset too large");

    constexpr uint32_t Op = 0b0011'1100'11 << 22;
    constexpr uint32_t o2 =
      Index == IndexType::POST   ? 0b01 :
      Index == IndexType::PRE    ? 0b11 :
      Index == IndexType::OFFSET ? 0b00 :
      Index == IndexType::UNPRIVILEGED ? 0b10 :  -1;

    LoadStoreImm(Op, o2, rt, rn, Imm & 0b1'1111'1111);
  }


