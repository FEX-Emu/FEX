#pragma once

#include "FEXCore/Config/Config.h"
#include <FEXCore/Utils/LogManager.h>

#include <biscuit/assembler.hpp>

#include <array>
#include <cstddef>
#include <cstdint>

namespace FEXCore::Context {
struct Context;
}

namespace FEXCore::CPU {
using namespace biscuit;

// All are caller saved
const std::array<biscuit::GPR, 16> SRA64 = {
  x1,
  x5, x6, x7,
  x10, x11,
  x12, x13, x14, x15, x16, x17,
  x28, x29, x30, x31,
};

// All are callee saved
const std::array<biscuit::GPR, 7> RA64 = {
  x20, x21, x22, x23, x24,
  x25, x26,
};

// Remaining GPRs not sitting in RA
// x0 - Zero
// x2 - Stack pointer
// x3 - Global pointer
// x4 - Thread pointer
// x27 - State

const std::array<std::pair<biscuit::GPR, biscuit::GPR>, 5>  RA64Pair = {{
  {x8, x9},
  {x18, x19},
  {x20, x21},
  {x22, x23},
  {x24, x25},
}};

// ABI isn't documented, assume all callee saved
const std::array<biscuit::Vec, 16> SRAFPR = {
  v0, v1, v2, v3, v4, v5, v6, v7, v8, v9,
  v10, v11, v12, v13, v14, v15,
};

const std::array<biscuit::Vec, 13> RAFPR = {
  // v16-v18 are temps
  v19, v20, v21, v22, v23, v24, v25,
  v26, v27, v28, v29, v30, v31,
};

class RISCVEmitter : public biscuit::Assembler {
public:
  // Arbitrarily chosen callee saved register
  constexpr static biscuit::GPR STATE = x27;
  constexpr static biscuit::GPR FPRSTATE = x3;
  constexpr static biscuit::GPR TMP1 = x8;
  constexpr static biscuit::GPR TMP2 = x9;
  constexpr static biscuit::GPR TMP3 = x18;
  constexpr static biscuit::GPR TMP4 = x19;
  constexpr static biscuit::FPR VTMP1 = f16;
  constexpr static biscuit::FPR VTMP2 = f17;
  constexpr static biscuit::FPR VTMP3 = f18;

protected:
  RISCVEmitter(FEXCore::Context::Context *ctx, uint8_t* Buffer, size_t size);
  RISCVEmitter(FEXCore::Context::Context *ctx, size_t size);

  void LoadConstant(biscuit::GPR Reg, uint64_t Constant, bool NOPPad = false);
  void SXTB(biscuit::GPR Dst, biscuit::GPR Src) {
    SEXTB(Dst, Src);
  }
  void SXTH(biscuit::GPR Dst, biscuit::GPR Src) {
    SEXTH(Dst, Src);
  }
  void SXTW(biscuit::GPR Dst, biscuit::GPR Src) {
    // Sign extends the 32-bit operation to 64-bit register
    ADDIW(Dst, Src, 0);
  }

  void UXTB(biscuit::GPR Dst, biscuit::GPR Src) {
    SLLI64(Dst, Src, 56);
    SRLI64(Dst, Dst, 56);
  }

  void UXTH(biscuit::GPR Dst, biscuit::GPR Src) {
    ZEXTH_64(Dst, Src);
  }

  void UXTW(biscuit::GPR Dst, biscuit::GPR Src) {
    ZEXTW(Dst, Src);
  }

  void SBFX(biscuit::GPR Dst, biscuit::GPR Src, uint32_t lsb, uint32_t width) {
    LOGMAN_THROW_A_FMT(width > 0, "Can't extract zero width");
    LOGMAN_THROW_A_FMT((lsb + width) <= 64, "Can't extract past 64-bits");
    SLLI64(Dst, Src, 64 - (lsb + width));
    SRAI64(Dst, Dst, 64 - width);
  }

  void UBFX(biscuit::GPR Dst, biscuit::GPR Src, uint32_t lsb, uint32_t width) {
    LOGMAN_THROW_A_FMT(width > 0, "Can't extract zero width");
    LOGMAN_THROW_A_FMT((lsb + width) <= 64, "Can't extract past 64-bits");
    SLLI64(Dst, Src, 64 - (lsb + width));
    SRLI64(Dst, Dst, 64 - width);
  }

  void LDSize(uint32_t Size, biscuit::GPR Dst, uint32_t Offset, biscuit::GPR Addr) {
    switch (Size) {
      case 1:
        LB(Dst, Offset, Addr);
        break;
      case 2:
        LH(Dst, Offset, Addr);
        break;
      case 4:
        LW(Dst, Offset, Addr);
        break;
      case 8:
        LD(Dst, Offset, Addr);
        break;
      default: LOGMAN_MSG_A_FMT("Unknown load size: {}", Size);
    }
  }

  void LDUSize(uint32_t Size, biscuit::GPR Dst, uint32_t Offset, biscuit::GPR Addr) {
    switch (Size) {
      case 1:
        LBU(Dst, Offset, Addr);
        break;
      case 2:
        LHU(Dst, Offset, Addr);
        break;
      case 4:
        LWU(Dst, Offset, Addr);
        break;
      case 8:
        LD(Dst, Offset, Addr);
        break;
      default: LOGMAN_MSG_A_FMT("Unknown load size: {}", Size);
    }
  }


  void STSize(uint32_t Size, biscuit::GPR Src, uint32_t Offset, biscuit::GPR Addr) {
    switch (Size) {
      case 1:
        SB(Src, Offset, Addr);
        break;
      case 2:
        SH(Src, Offset, Addr);
        break;
      case 4:
        SW(Src, Offset, Addr);
        break;
      case 8:
        SD(Src, Offset, Addr);
        break;
      default: LOGMAN_MSG_A_FMT("Unknown load size: {}", Size);
    }
  }

  void LDSize(uint32_t Size, biscuit::FPR Dst, uint32_t Offset, biscuit::GPR Addr) {
    switch (Size) {
      case 4:
        FLW(Dst, Offset, Addr);
        break;
      case 8:
        FLD(Dst, Offset, Addr);
        break;
      default: LOGMAN_MSG_A_FMT("Unknown load size: {}", Size);
    }
  }

  void STSize(uint32_t Size, biscuit::FPR Src, uint32_t Offset, biscuit::GPR Addr) {
    switch (Size) {
      case 4:
        FSW(Src, Offset, Addr);
        break;
      case 8:
        FSD(Src, Offset, Addr);
        break;
      default: LOGMAN_MSG_A_FMT("Unknown load size: {}", Size);
    }
  }

  void SpillStaticRegs(bool FPRs = true, uint32_t GPRSpillMask = ~0U, uint32_t FPRSpillMask = ~0U);
  void FillStaticRegs(bool FPRs = true, uint32_t GPRFillMask = ~0U, uint32_t FPRFillMask = ~0U);

  void PushDynamicRegsAndLR();
  void PopDynamicRegsAndLR();

  void PushCalleeSavedRegisters();
  void PopCalleeSavedRegisters();

  void ResetStack();

  uint32_t SpillSlots{};
  FEX_CONFIG_OPT(StaticRegisterAllocation, SRA);
};

}
