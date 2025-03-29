// SPDX-License-Identifier: MIT
#pragma once

#include <FEXCore/Utils/EnumUtils.h>

#include <compare>
#include <cstdint>

namespace ARMEmitter {
class WRegister;
class XRegister;

/* Unsized GPR register class
 * This class doesn't imply a size when used
 */
class Register {
public:
  Register() = delete;
  constexpr explicit Register(uint32_t Idx)
    : Index {Idx} {}

  friend constexpr auto operator<=>(const Register&, const Register&) = default;

  constexpr uint32_t Idx() const {
    return Index;
  }
  constexpr WRegister W() const;
  constexpr XRegister X() const;

private:
  uint32_t Index;
};
static_assert(sizeof(Register) == sizeof(uint32_t));
static_assert(std::is_trivially_copyable_v<Register>);
static_assert(std::is_standard_layout_v<Register>);

/* 32-bit GPR register class.
 * This class will imply a 32-bit register size being used.
 */
class WRegister {
public:
  WRegister() = delete;
  constexpr explicit WRegister(uint32_t Idx)
    : Index {Idx} {}

  friend constexpr auto operator<=>(const WRegister&, const WRegister&) = default;

  constexpr uint32_t Idx() const {
    return Index;
  }

  constexpr operator Register() const {
    return Register(Index);
  }
  constexpr XRegister X() const;
  constexpr Register R() const;

private:
  uint32_t Index;
};
static_assert(sizeof(WRegister) == sizeof(uint32_t));
static_assert(std::is_trivially_copyable_v<WRegister>);
static_assert(std::is_standard_layout_v<WRegister>);

/* 64-bit GPR register class.
 * This class will imply a 64-bit register size being used.
 */
class XRegister {
public:
  XRegister() = delete;
  constexpr explicit XRegister(uint32_t Idx)
    : Index {Idx} {}

  friend constexpr auto operator<=>(const XRegister&, const XRegister&) = default;

  constexpr uint32_t Idx() const {
    return Index;
  }

  constexpr operator Register() const {
    return Register(Index);
  }
  constexpr WRegister W() const;
  constexpr Register R() const;

private:
  uint32_t Index;
};
static_assert(sizeof(XRegister) == sizeof(uint32_t));
static_assert(std::is_trivially_copyable_v<XRegister>);
static_assert(std::is_standard_layout_v<XRegister>);

inline constexpr WRegister Register::W() const {
  return WRegister {Index};
}

inline constexpr XRegister Register::X() const {
  return XRegister {Index};
}

inline constexpr XRegister WRegister::X() const {
  return XRegister {Index};
}

inline constexpr Register WRegister::R() const {
  return *this;
}

inline constexpr WRegister XRegister::W() const {
  return WRegister {Index};
}

inline constexpr Register XRegister::R() const {
  return *this;
}

// Namespace containing all unsized GPR register objects.
namespace Reg {
  constexpr static Register r0(0);
  constexpr static Register r1(1);
  constexpr static Register r2(2);
  constexpr static Register r3(3);
  constexpr static Register r4(4);
  constexpr static Register r5(5);
  constexpr static Register r6(6);
  constexpr static Register r7(7);
  constexpr static Register r8(8);
  constexpr static Register r9(9);
  constexpr static Register r10(10);
  constexpr static Register r11(11);
  constexpr static Register r12(12);
  constexpr static Register r13(13);
  constexpr static Register r14(14);
  constexpr static Register r15(15);
  constexpr static Register r16(16);
  constexpr static Register r17(17);
  constexpr static Register r18(18);
  constexpr static Register r19(19);
  constexpr static Register r20(20);
  constexpr static Register r21(21);
  constexpr static Register r22(22);
  constexpr static Register r23(23);
  constexpr static Register r24(24);
  constexpr static Register r25(25);
  constexpr static Register r26(26);
  constexpr static Register r27(27);
  constexpr static Register r28(28);
  constexpr static Register r29(29);
  constexpr static Register r30(30);
  constexpr static Register r31(31);

  // Named registers
  constexpr static Register ip0(16);
  constexpr static Register ip1(17);

  constexpr static Register fp(29);
  constexpr static Register lr(30);
  constexpr static Register rsp(31);
  constexpr static Register zr(31);
} // namespace Reg

// Namespace containing all 64-bit GPR register objects.
namespace XReg {
  constexpr static XRegister x0(0);
  constexpr static XRegister x1(1);
  constexpr static XRegister x2(2);
  constexpr static XRegister x3(3);
  constexpr static XRegister x4(4);
  constexpr static XRegister x5(5);
  constexpr static XRegister x6(6);
  constexpr static XRegister x7(7);
  constexpr static XRegister x8(8);
  constexpr static XRegister x9(9);
  constexpr static XRegister x10(10);
  constexpr static XRegister x11(11);
  constexpr static XRegister x12(12);
  constexpr static XRegister x13(13);
  constexpr static XRegister x14(14);
  constexpr static XRegister x15(15);
  constexpr static XRegister x16(16);
  constexpr static XRegister x17(17);
  constexpr static XRegister x18(18);
  constexpr static XRegister x19(19);
  constexpr static XRegister x20(20);
  constexpr static XRegister x21(21);
  constexpr static XRegister x22(22);
  constexpr static XRegister x23(23);
  constexpr static XRegister x24(24);
  constexpr static XRegister x25(25);
  constexpr static XRegister x26(26);
  constexpr static XRegister x27(27);
  constexpr static XRegister x28(28);
  constexpr static XRegister x29(29);
  constexpr static XRegister x30(30);
  constexpr static XRegister x31(31);

  // Named registers
  constexpr static XRegister ip0(16);
  constexpr static XRegister ip1(17);

  constexpr static XRegister fp(29);
  constexpr static XRegister lr(30);
  constexpr static XRegister rsp(31);
  constexpr static XRegister zr(31);
} // namespace XReg

// Namespace containing all 32-bit GPR register objects.
namespace WReg {
  constexpr static WRegister w0(0);
  constexpr static WRegister w1(1);
  constexpr static WRegister w2(2);
  constexpr static WRegister w3(3);
  constexpr static WRegister w4(4);
  constexpr static WRegister w5(5);
  constexpr static WRegister w6(6);
  constexpr static WRegister w7(7);
  constexpr static WRegister w8(8);
  constexpr static WRegister w9(9);
  constexpr static WRegister w10(10);
  constexpr static WRegister w11(11);
  constexpr static WRegister w12(12);
  constexpr static WRegister w13(13);
  constexpr static WRegister w14(14);
  constexpr static WRegister w15(15);
  constexpr static WRegister w16(16);
  constexpr static WRegister w17(17);
  constexpr static WRegister w18(18);
  constexpr static WRegister w19(19);
  constexpr static WRegister w20(20);
  constexpr static WRegister w21(21);
  constexpr static WRegister w22(22);
  constexpr static WRegister w23(23);
  constexpr static WRegister w24(24);
  constexpr static WRegister w25(25);
  constexpr static WRegister w26(26);
  constexpr static WRegister w27(27);
  constexpr static WRegister w28(28);
  constexpr static WRegister w29(29);
  constexpr static WRegister w30(30);
  constexpr static WRegister w31(31);

  // Named registers
  constexpr static WRegister ip0(16);
  constexpr static WRegister ip1(17);

  constexpr static WRegister fp(29);
  constexpr static WRegister lr(30);
  constexpr static WRegister rsp(31);
  constexpr static WRegister zr(31);
} // namespace WReg

class VRegister;
class BRegister;
class HRegister;
class SRegister;
class DRegister;
class QRegister;
class ZRegister;

/* Unsized ASIMD register class
 * This class doesn't imply a size when used, nor implies Vector or Scalar.
 * It does imply that this instruction isn't using the register for SVE.
 */
class VRegister {
public:
  VRegister() = delete;
  constexpr explicit VRegister(uint32_t Idx)
    : Index {Idx} {}

  friend constexpr auto operator<=>(const VRegister&, const VRegister&) = default;

  constexpr uint32_t Idx() const {
    return Index;
  }

  constexpr BRegister B() const;
  constexpr HRegister H() const;
  constexpr SRegister S() const;
  constexpr DRegister D() const;
  constexpr QRegister Q() const;
  constexpr ZRegister Z() const;

private:
  uint32_t Index;
};
static_assert(sizeof(VRegister) == sizeof(uint32_t));
static_assert(std::is_trivially_copyable_v<VRegister>);
static_assert(std::is_standard_layout_v<VRegister>);

/* 8-bit ASIMD register class
 * This class implies 8-bit scalar register.
 */
class BRegister {
public:
  BRegister() = delete;
  constexpr explicit BRegister(uint32_t Idx)
    : Index {Idx} {}

  friend constexpr auto operator<=>(const BRegister&, const BRegister&) = default;

  constexpr uint32_t Idx() const {
    return Index;
  }

  constexpr operator VRegister() const {
    return VRegister(Index);
  }
  constexpr BRegister V() const;
  constexpr HRegister H() const;
  constexpr SRegister S() const;
  constexpr DRegister D() const;
  constexpr QRegister Q() const;
  constexpr ZRegister Z() const;

private:
  uint32_t Index;
};
static_assert(sizeof(BRegister) == sizeof(uint32_t));
static_assert(std::is_trivially_copyable_v<BRegister>);
static_assert(std::is_standard_layout_v<BRegister>);

/* 16-bit ASIMD register class
 * This class implies 16-bit scalar register.
 */
class HRegister {
public:
  HRegister() = delete;
  constexpr explicit HRegister(uint32_t Idx)
    : Index {Idx} {}

  friend constexpr auto operator<=>(const HRegister&, const HRegister&) = default;

  constexpr uint32_t Idx() const {
    return Index;
  }

  constexpr operator VRegister() const {
    return VRegister(Index);
  }
  constexpr HRegister V() const;
  constexpr BRegister B() const;
  constexpr SRegister S() const;
  constexpr DRegister D() const;
  constexpr QRegister Q() const;
  constexpr ZRegister Z() const;

private:
  uint32_t Index;
};
static_assert(sizeof(HRegister) == sizeof(uint32_t));
static_assert(std::is_trivially_copyable_v<HRegister>);
static_assert(std::is_standard_layout_v<HRegister>);

/* 32-bit ASIMD register class
 * This class implies 32-bit scalar register.
 */
class SRegister {
public:
  SRegister() = delete;
  constexpr explicit SRegister(uint32_t Idx)
    : Index {Idx} {}

  friend constexpr auto operator<=>(const SRegister&, const SRegister&) = default;

  constexpr uint32_t Idx() const {
    return Index;
  }

  constexpr operator VRegister() const {
    return VRegister(Index);
  }
  constexpr SRegister V() const;
  constexpr BRegister B() const;
  constexpr HRegister H() const;
  constexpr DRegister D() const;
  constexpr QRegister Q() const;
  constexpr ZRegister Z() const;

private:
  uint32_t Index;
};
static_assert(sizeof(SRegister) == sizeof(uint32_t));
static_assert(std::is_trivially_copyable_v<SRegister>);
static_assert(std::is_standard_layout_v<SRegister>);

/* 64-bit ASIMD register class
 * This class doesn't imply Vector or Scalar.
 * Associated with operating the instruction at 64-bit.
 */
class DRegister {
public:
  DRegister() = delete;
  constexpr explicit DRegister(uint32_t Idx)
    : Index {Idx} {}

  friend constexpr auto operator<=>(const DRegister&, const DRegister&) = default;

  constexpr uint32_t Idx() const {
    return Index;
  }

  constexpr operator VRegister() const {
    return VRegister(Index);
  }
  constexpr DRegister V() const;
  constexpr BRegister B() const;
  constexpr HRegister H() const;
  constexpr SRegister S() const;
  constexpr QRegister Q() const;
  constexpr ZRegister Z() const;

private:
  uint32_t Index;
};
static_assert(sizeof(DRegister) == sizeof(uint32_t));
static_assert(std::is_trivially_copyable_v<DRegister>);
static_assert(std::is_standard_layout_v<DRegister>);

/* 128-bit ASIMD register class
 * This class doesn't imply Vector or Scalar.
 * Associated with operating the instruction at 128-bit.
 */
class QRegister {
public:
  QRegister() = delete;
  constexpr explicit QRegister(uint32_t Idx)
    : Index {Idx} {}

  friend constexpr auto operator<=>(const QRegister&, const QRegister&) = default;

  constexpr uint32_t Idx() const {
    return Index;
  }

  constexpr operator VRegister() const {
    return VRegister(Index);
  }
  constexpr QRegister V() const;
  constexpr BRegister B() const;
  constexpr HRegister H() const;
  constexpr SRegister S() const;
  constexpr DRegister D() const;
  constexpr ZRegister Z() const;

private:
  uint32_t Index;
};
static_assert(sizeof(QRegister) == sizeof(uint32_t));
static_assert(std::is_trivially_copyable_v<QRegister>);
static_assert(std::is_standard_layout_v<QRegister>);

/* Unsized SVE register class.
 * This class explicitly implies the instruction will operate using SVE.
 */
class ZRegister {
public:
  ZRegister() = delete;
  constexpr explicit ZRegister(uint32_t Idx)
    : Index {Idx} {}

  friend constexpr auto operator<=>(const ZRegister&, const ZRegister&) = default;

  constexpr uint32_t Idx() const {
    return Index;
  }

  constexpr VRegister V() const;
  constexpr BRegister B() const;
  constexpr HRegister H() const;
  constexpr SRegister S() const;
  constexpr DRegister D() const;
  constexpr QRegister Q() const;

private:
  uint32_t Index;
};
static_assert(sizeof(ZRegister) == sizeof(uint32_t));
static_assert(std::is_trivially_copyable_v<ZRegister>);
static_assert(std::is_standard_layout_v<ZRegister>);

// VRegister
inline constexpr BRegister VRegister::B() const {
  return BRegister {Index};
}
inline constexpr HRegister VRegister::H() const {
  return HRegister {Index};
}
inline constexpr SRegister VRegister::S() const {
  return SRegister {Index};
}
inline constexpr DRegister VRegister::D() const {
  return DRegister {Index};
}
inline constexpr QRegister VRegister::Q() const {
  return QRegister {Index};
}
inline constexpr ZRegister VRegister::Z() const {
  return ZRegister {Index};
}

// BRegister
inline constexpr BRegister BRegister::V() const {
  return *this;
}
inline constexpr HRegister BRegister::H() const {
  return HRegister {Index};
}
inline constexpr SRegister BRegister::S() const {
  return SRegister {Index};
}
inline constexpr DRegister BRegister::D() const {
  return DRegister {Index};
}
inline constexpr QRegister BRegister::Q() const {
  return QRegister {Index};
}
inline constexpr ZRegister BRegister::Z() const {
  return ZRegister {Index};
}

// HRegister
inline constexpr HRegister HRegister::V() const {
  return *this;
}
inline constexpr BRegister HRegister::B() const {
  return BRegister {Index};
}
inline constexpr SRegister HRegister::S() const {
  return SRegister {Index};
}
inline constexpr DRegister HRegister::D() const {
  return DRegister {Index};
}
inline constexpr QRegister HRegister::Q() const {
  return QRegister {Index};
}
inline constexpr ZRegister HRegister::Z() const {
  return ZRegister {Index};
}

// SRegister
inline constexpr SRegister SRegister::V() const {
  return *this;
}
inline constexpr BRegister SRegister::B() const {
  return BRegister {Index};
}
inline constexpr HRegister SRegister::H() const {
  return HRegister {Index};
}
inline constexpr DRegister SRegister::D() const {
  return DRegister {Index};
}
inline constexpr QRegister SRegister::Q() const {
  return QRegister {Index};
}
inline constexpr ZRegister SRegister::Z() const {
  return ZRegister {Index};
}

// DRegister
inline constexpr DRegister DRegister::V() const {
  return DRegister {Index};
}
inline constexpr BRegister DRegister::B() const {
  return BRegister {Index};
}
inline constexpr HRegister DRegister::H() const {
  return HRegister {Index};
}
inline constexpr SRegister DRegister::S() const {
  return SRegister {Index};
}
inline constexpr QRegister DRegister::Q() const {
  return QRegister {Index};
}
inline constexpr ZRegister DRegister::Z() const {
  return ZRegister {Index};
}

// QRegister
inline constexpr QRegister QRegister::V() const {
  return *this;
}
inline constexpr BRegister QRegister::B() const {
  return BRegister {Index};
}
inline constexpr HRegister QRegister::H() const {
  return HRegister {Index};
}
inline constexpr SRegister QRegister::S() const {
  return SRegister {Index};
}
inline constexpr DRegister QRegister::D() const {
  return DRegister {Index};
}
inline constexpr ZRegister QRegister::Z() const {
  return ZRegister {Index};
}

// ZRegister
inline constexpr VRegister ZRegister::V() const {
  return VRegister(Index);
}
inline constexpr BRegister ZRegister::B() const {
  return BRegister(Index);
}
inline constexpr HRegister ZRegister::H() const {
  return HRegister(Index);
}
inline constexpr SRegister ZRegister::S() const {
  return SRegister(Index);
}
inline constexpr DRegister ZRegister::D() const {
  return DRegister(Index);
}
inline constexpr QRegister ZRegister::Q() const {
  return QRegister(Index);
}

// Namespace containing all unsized ASIMD register objects.
namespace VReg {
  constexpr static VRegister v0(0);
  constexpr static VRegister v1(1);
  constexpr static VRegister v2(2);
  constexpr static VRegister v3(3);
  constexpr static VRegister v4(4);
  constexpr static VRegister v5(5);
  constexpr static VRegister v6(6);
  constexpr static VRegister v7(7);
  constexpr static VRegister v8(8);
  constexpr static VRegister v9(9);
  constexpr static VRegister v10(10);
  constexpr static VRegister v11(11);
  constexpr static VRegister v12(12);
  constexpr static VRegister v13(13);
  constexpr static VRegister v14(14);
  constexpr static VRegister v15(15);
  constexpr static VRegister v16(16);
  constexpr static VRegister v17(17);
  constexpr static VRegister v18(18);
  constexpr static VRegister v19(19);
  constexpr static VRegister v20(20);
  constexpr static VRegister v21(21);
  constexpr static VRegister v22(22);
  constexpr static VRegister v23(23);
  constexpr static VRegister v24(24);
  constexpr static VRegister v25(25);
  constexpr static VRegister v26(26);
  constexpr static VRegister v27(27);
  constexpr static VRegister v28(28);
  constexpr static VRegister v29(29);
  constexpr static VRegister v30(30);
  constexpr static VRegister v31(31);
} // namespace VReg

// Namespace containing all 8-bit ASIMD register objects.
namespace BReg {
  constexpr static BRegister b0(0);
  constexpr static BRegister b1(1);
  constexpr static BRegister b2(2);
  constexpr static BRegister b3(3);
  constexpr static BRegister b4(4);
  constexpr static BRegister b5(5);
  constexpr static BRegister b6(6);
  constexpr static BRegister b7(7);
  constexpr static BRegister b8(8);
  constexpr static BRegister b9(9);
  constexpr static BRegister b10(10);
  constexpr static BRegister b11(11);
  constexpr static BRegister b12(12);
  constexpr static BRegister b13(13);
  constexpr static BRegister b14(14);
  constexpr static BRegister b15(15);
  constexpr static BRegister b16(16);
  constexpr static BRegister b17(17);
  constexpr static BRegister b18(18);
  constexpr static BRegister b19(19);
  constexpr static BRegister b20(20);
  constexpr static BRegister b21(21);
  constexpr static BRegister b22(22);
  constexpr static BRegister b23(23);
  constexpr static BRegister b24(24);
  constexpr static BRegister b25(25);
  constexpr static BRegister b26(26);
  constexpr static BRegister b27(27);
  constexpr static BRegister b28(28);
  constexpr static BRegister b29(29);
  constexpr static BRegister b30(30);
  constexpr static BRegister b31(31);
} // namespace BReg

// Namespace containing all 16-bit ASIMD register objects.
namespace HReg {
  constexpr static HRegister h0(0);
  constexpr static HRegister h1(1);
  constexpr static HRegister h2(2);
  constexpr static HRegister h3(3);
  constexpr static HRegister h4(4);
  constexpr static HRegister h5(5);
  constexpr static HRegister h6(6);
  constexpr static HRegister h7(7);
  constexpr static HRegister h8(8);
  constexpr static HRegister h9(9);
  constexpr static HRegister h10(10);
  constexpr static HRegister h11(11);
  constexpr static HRegister h12(12);
  constexpr static HRegister h13(13);
  constexpr static HRegister h14(14);
  constexpr static HRegister h15(15);
  constexpr static HRegister h16(16);
  constexpr static HRegister h17(17);
  constexpr static HRegister h18(18);
  constexpr static HRegister h19(19);
  constexpr static HRegister h20(20);
  constexpr static HRegister h21(21);
  constexpr static HRegister h22(22);
  constexpr static HRegister h23(23);
  constexpr static HRegister h24(24);
  constexpr static HRegister h25(25);
  constexpr static HRegister h26(26);
  constexpr static HRegister h27(27);
  constexpr static HRegister h28(28);
  constexpr static HRegister h29(29);
  constexpr static HRegister h30(30);
  constexpr static HRegister h31(31);
} // namespace HReg

// Namespace containing all 32-bit ASIMD register objects.
namespace SReg {
  constexpr static SRegister s0(0);
  constexpr static SRegister s1(1);
  constexpr static SRegister s2(2);
  constexpr static SRegister s3(3);
  constexpr static SRegister s4(4);
  constexpr static SRegister s5(5);
  constexpr static SRegister s6(6);
  constexpr static SRegister s7(7);
  constexpr static SRegister s8(8);
  constexpr static SRegister s9(9);
  constexpr static SRegister s10(10);
  constexpr static SRegister s11(11);
  constexpr static SRegister s12(12);
  constexpr static SRegister s13(13);
  constexpr static SRegister s14(14);
  constexpr static SRegister s15(15);
  constexpr static SRegister s16(16);
  constexpr static SRegister s17(17);
  constexpr static SRegister s18(18);
  constexpr static SRegister s19(19);
  constexpr static SRegister s20(20);
  constexpr static SRegister s21(21);
  constexpr static SRegister s22(22);
  constexpr static SRegister s23(23);
  constexpr static SRegister s24(24);
  constexpr static SRegister s25(25);
  constexpr static SRegister s26(26);
  constexpr static SRegister s27(27);
  constexpr static SRegister s28(28);
  constexpr static SRegister s29(29);
  constexpr static SRegister s30(30);
  constexpr static SRegister s31(31);
} // namespace SReg

// Namespace containing all 64-bit ASIMD register objects.
namespace DReg {
  constexpr static DRegister d0(0);
  constexpr static DRegister d1(1);
  constexpr static DRegister d2(2);
  constexpr static DRegister d3(3);
  constexpr static DRegister d4(4);
  constexpr static DRegister d5(5);
  constexpr static DRegister d6(6);
  constexpr static DRegister d7(7);
  constexpr static DRegister d8(8);
  constexpr static DRegister d9(9);
  constexpr static DRegister d10(10);
  constexpr static DRegister d11(11);
  constexpr static DRegister d12(12);
  constexpr static DRegister d13(13);
  constexpr static DRegister d14(14);
  constexpr static DRegister d15(15);
  constexpr static DRegister d16(16);
  constexpr static DRegister d17(17);
  constexpr static DRegister d18(18);
  constexpr static DRegister d19(19);
  constexpr static DRegister d20(20);
  constexpr static DRegister d21(21);
  constexpr static DRegister d22(22);
  constexpr static DRegister d23(23);
  constexpr static DRegister d24(24);
  constexpr static DRegister d25(25);
  constexpr static DRegister d26(26);
  constexpr static DRegister d27(27);
  constexpr static DRegister d28(28);
  constexpr static DRegister d29(29);
  constexpr static DRegister d30(30);
  constexpr static DRegister d31(31);
} // namespace DReg

// Namespace containing all 128-bit ASIMD register objects.
namespace QReg {
  constexpr static QRegister q0(0);
  constexpr static QRegister q1(1);
  constexpr static QRegister q2(2);
  constexpr static QRegister q3(3);
  constexpr static QRegister q4(4);
  constexpr static QRegister q5(5);
  constexpr static QRegister q6(6);
  constexpr static QRegister q7(7);
  constexpr static QRegister q8(8);
  constexpr static QRegister q9(9);
  constexpr static QRegister q10(10);
  constexpr static QRegister q11(11);
  constexpr static QRegister q12(12);
  constexpr static QRegister q13(13);
  constexpr static QRegister q14(14);
  constexpr static QRegister q15(15);
  constexpr static QRegister q16(16);
  constexpr static QRegister q17(17);
  constexpr static QRegister q18(18);
  constexpr static QRegister q19(19);
  constexpr static QRegister q20(20);
  constexpr static QRegister q21(21);
  constexpr static QRegister q22(22);
  constexpr static QRegister q23(23);
  constexpr static QRegister q24(24);
  constexpr static QRegister q25(25);
  constexpr static QRegister q26(26);
  constexpr static QRegister q27(27);
  constexpr static QRegister q28(28);
  constexpr static QRegister q29(29);
  constexpr static QRegister q30(30);
  constexpr static QRegister q31(31);
} // namespace QReg

// Namespace containing all unsigned SVE register objects.
namespace ZReg {
  constexpr static ZRegister z0(0);
  constexpr static ZRegister z1(1);
  constexpr static ZRegister z2(2);
  constexpr static ZRegister z3(3);
  constexpr static ZRegister z4(4);
  constexpr static ZRegister z5(5);
  constexpr static ZRegister z6(6);
  constexpr static ZRegister z7(7);
  constexpr static ZRegister z8(8);
  constexpr static ZRegister z9(9);
  constexpr static ZRegister z10(10);
  constexpr static ZRegister z11(11);
  constexpr static ZRegister z12(12);
  constexpr static ZRegister z13(13);
  constexpr static ZRegister z14(14);
  constexpr static ZRegister z15(15);
  constexpr static ZRegister z16(16);
  constexpr static ZRegister z17(17);
  constexpr static ZRegister z18(18);
  constexpr static ZRegister z19(19);
  constexpr static ZRegister z20(20);
  constexpr static ZRegister z21(21);
  constexpr static ZRegister z22(22);
  constexpr static ZRegister z23(23);
  constexpr static ZRegister z24(24);
  constexpr static ZRegister z25(25);
  constexpr static ZRegister z26(26);
  constexpr static ZRegister z27(27);
  constexpr static ZRegister z28(28);
  constexpr static ZRegister z29(29);
  constexpr static ZRegister z30(30);
  constexpr static ZRegister z31(31);
} // namespace ZReg

// Zero-cost FPR->GPR
inline constexpr Register ToReg(HRegister Reg) {
  return Register(Reg.Idx());
}
inline constexpr Register ToReg(SRegister Reg) {
  return Register(Reg.Idx());
}
inline constexpr Register ToReg(DRegister Reg) {
  return Register(Reg.Idx());
}
inline constexpr Register ToReg(VRegister Reg) {
  return Register(Reg.Idx());
}

// Zero-cost GPR->FPR
inline constexpr VRegister ToVReg(Register Reg) {
  return VRegister(Reg.Idx());
}
inline constexpr VRegister ToVReg(XRegister Reg) {
  return VRegister(Reg.Idx());
}
inline constexpr VRegister ToVReg(WRegister Reg) {
  return VRegister(Reg.Idx());
}

class PRegisterZero;
class PRegisterMerge;

/* Unsized predicate register for SVE.
 * This is unsized because of how SVE operates.
 */
class PRegister {
public:
  PRegister() = delete;
  constexpr PRegister(uint32_t Idx)
    : Index {Idx} {}

  friend constexpr auto operator<=>(const PRegister&, const PRegister&) = default;

  constexpr uint32_t Idx() const {
    return Index;
  }

  constexpr PRegisterZero Zeroing() const;
  constexpr PRegisterMerge Merging() const;

private:
  uint32_t Index;
};
static_assert(sizeof(PRegister) == sizeof(uint32_t));
static_assert(std::is_trivially_copyable_v<PRegister>);
static_assert(std::is_standard_layout_v<PRegister>);

// Unsized predicate register for SVE with zeroing semantics.
class PRegisterZero {
public:
  PRegisterZero() = delete;
  constexpr PRegisterZero(uint32_t Idx)
    : Index {Idx} {}

  friend constexpr auto operator<=>(const PRegisterZero&, const PRegisterZero&) = default;

  constexpr uint32_t Idx() const {
    return Index;
  }

  constexpr operator PRegister() const {
    return PRegister(Index);
  }
  constexpr PRegister P() const {
    return PRegister(Index);
  }
  constexpr PRegisterMerge Merging() const;

private:
  uint32_t Index;
};
static_assert(sizeof(PRegisterZero) == sizeof(uint32_t));
static_assert(std::is_trivially_copyable_v<PRegisterZero>);
static_assert(std::is_standard_layout_v<PRegisterZero>);

// Unsized predicate register for SVE with merging semantics.
class PRegisterMerge {
public:
  PRegisterMerge() = delete;
  constexpr PRegisterMerge(uint32_t Idx)
    : Index {Idx} {}

  friend constexpr auto operator<=>(const PRegisterMerge&, const PRegisterMerge&) = default;

  constexpr uint32_t Idx() const {
    return Index;
  }

  constexpr operator PRegister() const {
    return PRegister(Index);
  }
  constexpr PRegister P() const {
    return PRegister(Index);
  }
  constexpr PRegisterZero Zeroing() const;

private:
  uint32_t Index;
};
static_assert(sizeof(PRegisterMerge) == sizeof(uint32_t));
static_assert(std::is_trivially_copyable_v<PRegisterMerge>);
static_assert(std::is_standard_layout_v<PRegisterMerge>);

// PRegister
inline constexpr PRegisterZero PRegister::Zeroing() const {
  return PRegisterZero(Idx());
}
inline constexpr PRegisterMerge PRegister::Merging() const {
  return PRegisterMerge(Idx());
}

// PRegisterZero
inline constexpr PRegisterMerge PRegisterZero::Merging() const {
  return PRegisterMerge(Idx());
}

// PRegisterMerge
inline constexpr PRegisterZero PRegisterMerge::Zeroing() const {
  return PRegisterZero(Idx());
}

// Namespace containing all unsigned SVE predicate register objects.
namespace PReg {
  constexpr static PRegister p0(0);
  constexpr static PRegister p1(1);
  constexpr static PRegister p2(2);
  constexpr static PRegister p3(3);
  constexpr static PRegister p4(4);
  constexpr static PRegister p5(5);
  constexpr static PRegister p6(6);
  constexpr static PRegister p7(7);
  constexpr static PRegister p8(8);
  constexpr static PRegister p9(9);
  constexpr static PRegister p10(10);
  constexpr static PRegister p11(11);
  constexpr static PRegister p12(12);
  constexpr static PRegister p13(13);
  constexpr static PRegister p14(14);
  constexpr static PRegister p15(15);
} // namespace PReg

/* `OpType` enum describes how some SVE instructions operate if they support both forms.
 * Not all SVE instructions support this.
 */
enum class OpType : uint32_t {
  Destructive = 0,
  Constructive,
};
} // namespace ARMEmitter
