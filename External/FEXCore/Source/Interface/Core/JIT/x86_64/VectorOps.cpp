/*
$info$
tags: backend|x86-64
$end_info$
*/

#include "Interface/Core/JIT/x86_64/JITClass.h"

#include <FEXCore/IR/IR.h>
#include <FEXCore/Utils/LogManager.h>

#include <array>
#include <stddef.h>
#include <stdint.h>
#include <xbyak/xbyak.h>

namespace FEXCore::CPU {

#define DEF_OP(x) void X86JITCore::Op_##x(IR::IROp_Header *IROp, IR::NodeID Node)
DEF_OP(VectorZero) {
  auto Dst = GetDst(Node);
  vpxor(Dst, Dst, Dst);
}

DEF_OP(VectorImm) {
  auto Op = IROp->C<IR::IROp_VectorImm>();

  const uint8_t OpSize = IROp->Size;
  const uint64_t Imm = Op->Immediate;

  const auto Dst = GetDst(Node);

  uint64_t Element{};
  switch (Op->Header.ElementSize) {
    case 1:
      Element =
        (Imm << (0 * 8)) |
        (Imm << (1 * 8)) |
        (Imm << (2 * 8)) |
        (Imm << (3 * 8)) |
        (Imm << (4 * 8)) |
        (Imm << (5 * 8)) |
        (Imm << (6 * 8)) |
        (Imm << (7 * 8));
      break;
    case 2:
      Element =
        (Imm << (0 * 16)) |
        (Imm << (1 * 16)) |
        (Imm << (2 * 16)) |
        (Imm << (3 * 16));
      break;
    case 4:
      Element =
        (Imm << (0 * 32)) |
        (Imm << (1 * 32));
      break;
    case 8:
      Element = Imm;
      break;
  }

  mov(TMP1, Element);
  vmovq(Dst, TMP1);

  if (OpSize >= 16) {
    LOGMAN_THROW_AA_FMT(OpSize == 16 || OpSize == 32,
                        "Can't handle a vector of size: {}", OpSize);

    // Duplicate into upper elements
    vbroadcastsd(ToYMM(Dst), Dst);
  }
}

DEF_OP(VMov) {
  auto Op = IROp->C<IR::IROp_VMov>();
  const uint8_t OpSize = IROp->Size;

  const auto Dst = GetDst(Node);
  const auto Source = GetSrc(Op->Source.ID());

  switch (OpSize) {
    case 1: {
      vpxor(xmm15, xmm15, xmm15);
      pextrb(eax, Source, 0);
      pinsrb(xmm15, eax, 0);
      vmovapd(Dst, xmm15);
      break;
    }
    case 2: {
      vpxor(xmm15, xmm15, xmm15);
      pextrw(eax, Source, 0);
      pinsrw(xmm15, eax, 0);
      vmovapd(Dst, xmm15);
      break;
    }
    case 4: {
      vpxor(xmm15, xmm15, xmm15);
      pextrd(eax, Source, 0);
      pinsrd(xmm15, eax, 0);
      vmovapd(Dst, xmm15);
      break;
    }
    case 8: {
      vmovq(Dst, Source);
      break;
    }
    case 16: {
      vmovaps(Dst, Source);
      break;
    }
    case 32: {
      vmovaps(ToYMM(Dst), ToYMM(Source));
      break;
    }
    default:
      LOGMAN_MSG_A_FMT("Unknown Op Size: {}", OpSize);
      break;
  }
}

DEF_OP(VAnd) {
  auto Op = IROp->C<IR::IROp_VAnd>();

  const auto Dst = ToYMM(GetDst(Node));
  const auto Vector1 = ToYMM(GetSrc(Op->Vector1.ID()));
  const auto Vector2 = ToYMM(GetSrc(Op->Vector2.ID()));

  vpand(Dst, Vector1, Vector2);
}

DEF_OP(VBic) {
  auto Op = IROp->C<IR::IROp_VBic>();

  const auto Dst = ToYMM(GetDst(Node));
  const auto Vector1 = ToYMM(GetSrc(Op->Vector1.ID()));
  const auto Vector2 = ToYMM(GetSrc(Op->Vector2.ID()));

  // This doesn't map directly to ARM
  vpcmpeqd(ymm15, ymm15, ymm15);
  vpxor(ymm15, Vector2, ymm15);
  vpand(Dst, Vector1, ymm15);
}

DEF_OP(VOr) {
  auto Op = IROp->C<IR::IROp_VOr>();

  const auto Dst = ToYMM(GetDst(Node));
  const auto Vector1 = ToYMM(GetSrc(Op->Vector1.ID()));
  const auto Vector2 = ToYMM(GetSrc(Op->Vector2.ID()));

  vpor(Dst, Vector1, Vector2);
}

DEF_OP(VXor) {
  auto Op = IROp->C<IR::IROp_VXor>();

  const auto Dst = ToYMM(GetDst(Node));
  const auto Vector1 = ToYMM(GetSrc(Op->Vector1.ID()));
  const auto Vector2 = ToYMM(GetSrc(Op->Vector2.ID()));

  vpxor(Dst, Vector1, Vector2);
}

DEF_OP(VAdd) {
  auto Op = IROp->C<IR::IROp_VAdd>();

  const auto ElementSize = Op->Header.ElementSize;

  const auto Dst = ToYMM(GetDst(Node));
  const auto Vector1 = ToYMM(GetSrc(Op->Vector1.ID()));
  const auto Vector2 = ToYMM(GetSrc(Op->Vector2.ID()));

  switch (ElementSize) {
    case 1: {
      vpaddb(Dst, Vector1, Vector2);
      break;
    }
    case 2: {
      vpaddw(Dst, Vector1, Vector2);
      break;
    }
    case 4: {
      vpaddd(Dst, Vector1, Vector2);
      break;
    }
    case 8: {
      vpaddq(Dst, Vector1, Vector2);
      break;
    }
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
}

DEF_OP(VSub) {
  auto Op = IROp->C<IR::IROp_VSub>();

  const auto ElementSize = Op->Header.ElementSize;

  const auto Dst = ToYMM(GetDst(Node));
  const auto Vector1 = ToYMM(GetSrc(Op->Vector1.ID()));
  const auto Vector2 = ToYMM(GetSrc(Op->Vector2.ID()));

  switch (ElementSize) {
    case 1: {
      vpsubb(Dst, Vector1, Vector2);
      break;
    }
    case 2: {
      vpsubw(Dst, Vector1, Vector2);
      break;
    }
    case 4: {
      vpsubd(Dst, Vector1, Vector2);
      break;
    }
    case 8: {
      vpsubq(Dst, Vector1, Vector2);
      break;
    }
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
}

DEF_OP(VUQAdd) {
  auto Op = IROp->C<IR::IROp_VUQAdd>();

  const auto ElementSize = Op->Header.ElementSize;

  const auto Dst = ToYMM(GetDst(Node));
  const auto Vector1 = ToYMM(GetSrc(Op->Vector1.ID()));
  const auto Vector2 = ToYMM(GetSrc(Op->Vector2.ID()));

  switch (ElementSize) {
    case 1: {
      vpaddusb(Dst, Vector1, Vector2);
      break;
    }
    case 2: {
      vpaddusw(Dst, Vector1, Vector2);
      break;
    }
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
}

DEF_OP(VUQSub) {
  auto Op = IROp->C<IR::IROp_VUQSub>();

  const auto ElementSize = Op->Header.ElementSize;

  const auto Dst = ToYMM(GetDst(Node));
  const auto Vector1 = ToYMM(GetSrc(Op->Vector1.ID()));
  const auto Vector2 = ToYMM(GetSrc(Op->Vector2.ID()));

  switch (ElementSize) {
    case 1: {
      vpsubusb(Dst, Vector1, Vector2);
      break;
    }
    case 2: {
      vpsubusw(Dst, Vector1, Vector2);
      break;
    }
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
}

DEF_OP(VSQAdd) {
  auto Op = IROp->C<IR::IROp_VSQAdd>();

  const auto ElementSize = Op->Header.ElementSize;

  const auto Dst = ToYMM(GetDst(Node));
  const auto Vector1 = ToYMM(GetSrc(Op->Vector1.ID()));
  const auto Vector2 = ToYMM(GetSrc(Op->Vector2.ID()));

  switch (ElementSize) {
    case 1: {
      vpaddsb(Dst, Vector1, Vector2);
      break;
    }
    case 2: {
      vpaddsw(Dst, Vector1, Vector2);
      break;
    }
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
}

DEF_OP(VSQSub) {
  auto Op = IROp->C<IR::IROp_VSQSub>();

  const auto ElementSize = Op->Header.ElementSize;

  const auto Dst = ToYMM(GetDst(Node));
  const auto Vector1 = ToYMM(GetSrc(Op->Vector1.ID()));
  const auto Vector2 = ToYMM(GetSrc(Op->Vector2.ID()));

  switch (ElementSize) {
    case 1: {
      vpsubsb(Dst, Vector1, Vector2);
      break;
    }
    case 2: {
      vpsubsw(Dst, Vector1, Vector2);
      break;
    }
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
}

DEF_OP(VAddP) {
  const auto Op = IROp->C<IR::IROp_VAddP>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;

  const auto Dst = GetDst(Node);
  const auto VectorLower = GetSrc(Op->VectorLower.ID());
  const auto VectorUpper = GetSrc(Op->VectorUpper.ID());

  if (OpSize == 8) {
    // Can't handle this natively without dropping to MMX
    // Emulate
    vpxor(xmm14, xmm14, xmm14);
    movq(xmm15, VectorLower);
    vshufpd(xmm15, xmm15, VectorUpper, 0b00);
    vpaddw(Dst, xmm15, xmm14);
    switch (ElementSize) {
      case 1:
        vpunpcklbw(xmm0, xmm15, xmm14);
        vpunpckhbw(xmm12, xmm15, xmm14);

        vpunpcklbw(xmm15, xmm0, xmm12);
        vpunpckhbw(xmm14, xmm0, xmm12);

        vpunpcklbw(xmm0, xmm15, xmm14);
        vpunpckhbw(xmm12, xmm15, xmm14);

        vpunpcklbw(xmm15, xmm0, xmm12);
        vpunpckhbw(xmm14, xmm0, xmm12);

        vpaddb(Dst, xmm15, xmm14);
        break;
      case 2:
        vphaddw(Dst, xmm15, xmm14);
        break;
      case 4:
        vphaddd(Dst, xmm15, xmm14);
        break;
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        break;
    }
  }
  else {
    const auto DstYMM = ToYMM(Dst);
    const auto VectorLowerYMM = ToYMM(VectorLower);
    const auto VectorUpperYMM = ToYMM(VectorUpper);

    switch (ElementSize) {
      case 1:
        vmovdqu(ymm15, VectorLowerYMM);
        vmovdqu(ymm14, VectorUpperYMM);

        vpunpcklbw(ymm0, ymm15, ymm14);
        vpunpckhbw(ymm12, ymm15, ymm14);

        vpunpcklbw(ymm15, ymm0, ymm12);
        vpunpckhbw(ymm14, ymm0, ymm12);

        vpunpcklbw(ymm0, ymm15, ymm14);
        vpunpckhbw(ymm12, ymm15, ymm14);

        vpunpcklbw(ymm15, ymm0, ymm12);
        vpunpckhbw(ymm14, ymm0, ymm12);

        vpaddb(DstYMM, ymm15, ymm14);
        break;
      case 2:
        vphaddw(DstYMM, VectorLowerYMM, VectorUpperYMM);
        break;
      case 4:
        vphaddd(DstYMM, VectorLowerYMM, VectorUpperYMM);
        break;
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        break;
    }
  }
}

DEF_OP(VAddV) {
  auto Op = IROp->C<IR::IROp_VAddV>();
  const uint8_t OpSize = IROp->Size;

  auto Src = GetSrc(Op->Vector.ID());
  auto Dest = GetDst(Node);
  vpxor(xmm15, xmm15, xmm15);

  const uint8_t Elements = OpSize / Op->Header.ElementSize;
  switch (Op->Header.ElementSize) {
    case 2: {
      for (int i = Elements; i > 1; i >>= 1) {
        vphaddw(Dest, Src, Dest);
        Src = Dest;
      }
      pextrw(eax, Dest, 0);
      pinsrw(xmm15, eax, 0);
    break;
    }
    case 4: {
      for (int i = Elements; i > 1; i >>= 1) {
        vphaddd(Dest, Src, Dest);
        Src = Dest;
      }
      pextrd(eax, Dest, 0);
      pinsrd(xmm15, eax, 0);
    break;
    }
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }

  movaps(Dest, xmm15);
}

DEF_OP(VUMinV) {
  auto Op = IROp->C<IR::IROp_VUMinV>();

  const auto OpSize = Op->Header.Size;
  const auto ElementSize = Op->Header.ElementSize;

  LOGMAN_THROW_AA_FMT(OpSize == 16 || OpSize == 32,
                      "Can't handle a vector of size: {}", OpSize);

  const auto Src = GetSrc(Op->Vector.ID());
  const auto Dest = GetDst(Node);

  switch (ElementSize) {
    case 2: {
      if (OpSize == 32) {
        // vphminposuw, unfortunately, doesn't traverse the entire
        // 256-bit ymm register (it just sets the top 128 bits to zero)
        // so we pull the top 128 bits down into its own vector,
        // get the horizontal minimum in both vectors, then
        // take the minimum between the two.
        vextractf128(xmm15, ToYMM(Src), 1);
        vphminposuw(xmm15, xmm15);
        vphminposuw(xmm14, Src);
        vpminuw(Dest, xmm14, xmm15);
      } else {
        vphminposuw(Dest, Src);
      }

      // Extract the upper bits which are zero, overwriting position
      pextrw(eax, Dest, 2);
      pinsrw(Dest, eax, 1);
      break;
    }
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
}

DEF_OP(VURAvg) {
  const auto Op = IROp->C<IR::IROp_VURAvg>();

  const auto ElementSize = Op->Header.ElementSize;

  const auto Dst = ToYMM(GetDst(Node));
  const auto Vector1 = ToYMM(GetSrc(Op->Vector1.ID()));
  const auto Vector2 = ToYMM(GetSrc(Op->Vector2.ID()));

  switch (ElementSize) {
    case 1: {
      vpavgb(Dst, Vector1, Vector2);
      break;
    }
    case 2: {
      vpavgw(Dst, Vector1, Vector2);
      break;
    }
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
}

DEF_OP(VAbs) {
  const auto Op = IROp->C<IR::IROp_VAbs>();

  const auto ElementSize = Op->Header.ElementSize;

  const auto Dst = ToYMM(GetDst(Node));
  const auto Src = ToYMM(GetSrc(Op->Vector.ID()));

  switch (ElementSize) {
    case 1: {
      vpabsb(Dst, Src);
      break;
    }
    case 2: {
      vpabsw(Dst, Src);
      break;
    }
    case 4: {
      vpabsd(Dst, Src);
      break;
    }
    case 8: {
      vpabsq(Dst, Src);
      break;
    }
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
}

// This only supports 8bit popcount on 8byte to 32byte registers
DEF_OP(VPopcount) {
  const auto Op = IROp->C<IR::IROp_VPopcount>();
  const uint8_t OpSize = IROp->Size;
  const bool Is256Bit = OpSize == 32;

  const auto Src = GetSrc(Op->Vector.ID());
  const auto Dest = GetDst(Node);

  const uint8_t ElementSize = Op->Header.ElementSize;
  const uint8_t Elements = OpSize / ElementSize;

  vpxor(xmm15, xmm15, xmm15);

  // This is disgustingly bad on x86-64 but we only need it for compatibility
  // NOTE: If, in the distant future, we ever use AVX-512, consider
  //       using vpopcnt{b, d, q, w} to shorten all of this down to one
  //       instruction.
  switch (ElementSize) {
    case 1: {
      const uint8_t NumElements = Is256Bit ? Elements / 2
                                           : Elements;

      const auto VectorPopcount = [this, NumElements](const Xbyak::Xmm& src, const Xbyak::Xmm& dst) {
        for (size_t i = 0; i < NumElements; ++i) {
          pextrb(eax, src, i);
          popcnt(eax, eax);
          pinsrb(dst, eax, i);
        }
      };

      // Bottom 128 bits
      VectorPopcount(Src, xmm15);
      movaps(Dest, xmm15);

      // Now do the top 128 bits, if necessary
      if (Is256Bit) {
        vextracti128(xmm14, ToYMM(Src), 1);
        VectorPopcount(xmm14, xmm14);
        vinserti128(ToYMM(Dest), ToYMM(Dest), xmm14, 1);
      }
      break;
    }
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
}

DEF_OP(VFAdd) {
  const auto Op = IROp->C<IR::IROp_VFAdd>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto IsScalar = ElementSize == OpSize;

  const auto Dst = GetDst(Node);
  const auto Vector1 = GetSrc(Op->Vector1.ID());
  const auto Vector2 = GetSrc(Op->Vector2.ID());

  if (IsScalar) {
    switch (ElementSize) {
      case 4: {
        vaddss(Dst, Vector1, Vector2);
        break;
      }
      case 8: {
        vaddsd(Dst, Vector1, Vector2);
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        break;
    }
  } else {
    const auto DstYMM = ToYMM(Dst);
    const auto Vector1YMM = ToYMM(Vector1);
    const auto Vector2YMM = ToYMM(Vector2);

    switch (ElementSize) {
      case 4: {
        vaddps(DstYMM, Vector1YMM, Vector2YMM);
        break;
      }
      case 8: {
        vaddpd(DstYMM, Vector1YMM, Vector2YMM);
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        break;
    }
  }
}

DEF_OP(VFAddP) {
  const auto Op = IROp->C<IR::IROp_VFAddP>();

  const auto ElementSize = Op->Header.ElementSize;

  const auto Dst = ToYMM(GetDst(Node));
  const auto VectorLower = ToYMM(GetSrc(Op->VectorLower.ID()));
  const auto VectorUpper = ToYMM(GetSrc(Op->VectorUpper.ID()));

  switch (ElementSize) {
    case 4:
      vhaddps(Dst, VectorLower, VectorUpper);
      break;
    case 8:
      vhaddpd(Dst, VectorLower, VectorUpper);
      break;
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
}

DEF_OP(VFSub) {
  const auto Op = IROp->C<IR::IROp_VFSub>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto IsScalar = ElementSize == OpSize;

  const auto Dst = GetDst(Node);
  const auto Vector1 = GetSrc(Op->Vector1.ID());
  const auto Vector2 = GetSrc(Op->Vector2.ID());

  if (IsScalar) {
    switch (ElementSize) {
      case 4: {
        vsubss(Dst, Vector1, Vector2);
        break;
      }
      case 8: {
        vsubsd(Dst, Vector1, Vector2);
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        break;
    }
  } else {
    const auto DstYMM = ToYMM(Dst);
    const auto Vector1YMM = ToYMM(Vector1);
    const auto Vector2YMM = ToYMM(Vector2);

    switch (ElementSize) {
      case 4: {
        vsubps(DstYMM, Vector1YMM, Vector2YMM);
        break;
      }
      case 8: {
        vsubpd(DstYMM, Vector1YMM, Vector2YMM);
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        break;
    }
  }
}

DEF_OP(VFMul) {
  const auto Op = IROp->C<IR::IROp_VFMul>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto IsScalar = Op->Header.ElementSize == OpSize;

  const auto Dst = GetDst(Node);
  const auto Vector1 = GetSrc(Op->Vector1.ID());
  const auto Vector2 = GetSrc(Op->Vector2.ID());

  if (IsScalar) {
    switch (ElementSize) {
      case 4: {
        vmulss(Dst, Vector1, Vector2);
        break;
      }
      case 8: {
        vmulsd(Dst, Vector1, Vector2);
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        break;
    }
  } else {
    const auto DstYMM = ToYMM(Dst);
    const auto Vector1YMM = ToYMM(Vector1);
    const auto Vector2YMM = ToYMM(Vector2);

    switch (ElementSize) {
      case 4: {
        vmulps(DstYMM, Vector1YMM, Vector2YMM);
        break;
      }
      case 8: {
        vmulpd(DstYMM, Vector1YMM, Vector2YMM);
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        break;
    }
  }
}

DEF_OP(VFDiv) {
  const auto Op = IROp->C<IR::IROp_VFDiv>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto IsScalar = ElementSize == OpSize;

  const auto Dst = GetDst(Node);
  const auto Vector1 = GetSrc(Op->Vector1.ID());
  const auto Vector2 = GetSrc(Op->Vector2.ID());

  if (IsScalar) {
    switch (ElementSize) {
      case 4: {
        vdivss(Dst, Vector1, Vector2);
        break;
      }
      case 8: {
        vdivsd(Dst, Vector1, Vector2);
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        break;
    }
  } else {
    const auto DstYMM = ToYMM(Dst);
    const auto Vector1YMM = ToYMM(Vector1);
    const auto Vector2YMM = ToYMM(Vector2);

    switch (ElementSize) {
      case 4: {
        vdivps(DstYMM, Vector1YMM, Vector2YMM);
        break;
      }
      case 8: {
        vdivpd(DstYMM, Vector1YMM, Vector2YMM);
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        break;
    }
  }
}

DEF_OP(VFMin) {
  const auto Op = IROp->C<IR::IROp_VFMin>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto IsScalar = ElementSize == OpSize;

  const auto Dst = GetDst(Node);
  const auto Vector1 = GetSrc(Op->Vector1.ID());
  const auto Vector2 = GetSrc(Op->Vector2.ID());

  if (IsScalar) {
    switch (Op->Header.ElementSize) {
      case 4: {
        vminss(Dst, Vector1, Vector2);
        break;
      }
      case 8: {
        vminsd(Dst, Vector1, Vector2);
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        break;
    }
  }
  else {
    const auto DstYMM = ToYMM(Dst);
    const auto Vector1YMM = ToYMM(Vector1);
    const auto Vector2YMM = ToYMM(Vector2);

    switch (ElementSize) {
      case 4: {
        vminps(DstYMM, Vector1YMM, Vector2YMM);
        break;
      }
      case 8: {
        vminpd(DstYMM, Vector1YMM, Vector2YMM);
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        break;
    }
  }
}

DEF_OP(VFMax) {
  const auto Op = IROp->C<IR::IROp_VFMax>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto IsScalar = ElementSize == OpSize;

  const auto Dst = GetDst(Node);
  const auto Vector1 = GetSrc(Op->Vector1.ID());
  const auto Vector2 = GetSrc(Op->Vector2.ID());

  if (IsScalar) {
    switch (ElementSize) {
      case 4: {
        vmaxss(Dst, Vector1, Vector2);
        break;
      }
      case 8: {
        vmaxsd(Dst, Vector1, Vector2);
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        break;
    }
  }
  else {
    const auto DstYMM = ToYMM(Dst);
    const auto Vector1YMM = ToYMM(Vector1);
    const auto Vector2YMM = ToYMM(Vector2);

    switch (ElementSize) {
      case 4: {
        vmaxps(DstYMM, Vector1YMM, Vector2YMM);
        break;
      }
      case 8: {
        vmaxpd(DstYMM, Vector1YMM, Vector2YMM);
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        break;
    }
  }
}

DEF_OP(VFRecp) {
  const auto Op = IROp->C<IR::IROp_VFRecp>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto IsScalar = ElementSize == OpSize;

  const auto Dst = GetDst(Node);
  const auto Vector = GetSrc(Op->Vector.ID());

  if (IsScalar) {
    switch (ElementSize) {
      case 4: {
        mov(eax, 0x3f800000); // 1.0f
        vmovd(xmm15, eax);
        vdivss(Dst, xmm15, Vector);
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        break;
    }
  }
  else {
    switch (ElementSize) {
      case 4: {
        mov(eax, 0x3f800000); // 1.0f
        vmovd(xmm15, eax);
        vbroadcastss(ymm15, xmm15);
        vdivps(ToYMM(Dst), ymm15, ToYMM(Vector));
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        break;
    }
  }
}

DEF_OP(VFSqrt) {
  const auto Op = IROp->C<IR::IROp_VFSqrt>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto IsScalar = ElementSize == OpSize;

  const auto Dst = GetDst(Node);
  const auto Vector = GetSrc(Op->Vector.ID());

  if (IsScalar) {
    switch (ElementSize) {
      case 4: {
        vsqrtss(Dst, Vector, Vector);
        break;
      }
      case 8: {
        vsqrtsd(Dst, Vector, Vector);
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        break;
    }
  }
  else {
    const auto DstYMM = ToYMM(Dst);
    const auto VectorYMM = ToYMM(Vector);

    switch (ElementSize) {
      case 4: {
        vsqrtps(DstYMM, VectorYMM);
        break;
      }
      case 8: {
        vsqrtpd(DstYMM, VectorYMM);
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        break;
    }
  }
}

DEF_OP(VFRSqrt) {
  const auto Op = IROp->C<IR::IROp_VFRSqrt>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto IsScalar = ElementSize == OpSize;

  const auto Dst = GetDst(Node);
  const auto Vector = GetSrc(Op->Vector.ID());

  if (IsScalar) {
    switch (ElementSize) {
      case 4: {
        mov(eax, 0x3f800000); // 1.0f
        sqrtss(xmm15, Vector);
        vmovd(Dst, eax);
        divss(Dst, xmm15);
        break;
      }
      case 8: {
        mov(eax, 0x3f800000); // 1.0f
        sqrtsd(xmm15, Vector);
        vmovd(Dst, eax);
        divsd(Dst, xmm15);
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        break;
    }
  }
  else {
    const auto DstYMM = ToYMM(Dst);
    const auto VectorYMM = ToYMM(Vector);

    switch (ElementSize) {
      case 4: {
        mov(rax, 0x3f800000); // 1.0f
        vsqrtps(ymm15, VectorYMM);
        vmovd(Dst, eax);
        vbroadcastss(DstYMM, Dst);
        vdivps(Dst, ymm15);
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        break;
    }
  }
}

DEF_OP(VNeg) {
  const auto Op = IROp->C<IR::IROp_VNeg>();

  const auto ElementSize = Op->Header.ElementSize;

  const auto Dst = ToYMM(GetDst(Node));
  const auto Vector = ToYMM(GetSrc(Op->Vector.ID()));

  vpxor(xmm15, xmm15, xmm15);
  switch (ElementSize) {
    case 1: {
      vpsubb(Dst, ymm15, Vector);
      break;
    }
    case 2: {
      vpsubw(Dst, ymm15, Vector);
      break;
    }
    case 4: {
      vpsubd(Dst, ymm15, Vector);
      break;
    }
    case 8: {
      vpsubq(Dst, ymm15, Vector);
      break;
    }
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
}

DEF_OP(VFNeg) {
  const auto Op = IROp->C<IR::IROp_VNeg>();

  const auto ElementSize = Op->Header.ElementSize;

  const auto Dst = ToYMM(GetDst(Node));
  const auto Vector = ToYMM(GetSrc(Op->Vector.ID()));

  switch (ElementSize) {
    case 2: {
      mov(rax, 0x80008000);
      vmovd(xmm15, eax);
      vbroadcastss(ymm15, xmm15);
      vxorps(Dst, ymm15, Vector);
      break;
    }
    case 4: {
      mov(rax, 0x80000000);
      vmovd(xmm15, eax);
      vbroadcastss(ymm15, xmm15);
      vxorps(Dst, ymm15, Vector);
      break;
    }
    case 8: {
      mov(rax, 0x8000000000000000ULL);
      vmovq(xmm15, rax);
      vbroadcastsd(ymm15, xmm15);
      vxorpd(Dst, ymm15, Vector);
      break;
    }
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
}

DEF_OP(VNot) {
  const auto Op = IROp->C<IR::IROp_VNot>();

  const auto Dst = ToYMM(GetDst(Node));
  const auto Vector = ToYMM(GetSrc(Op->Vector.ID()));

  vpcmpeqd(ymm15, ymm15, ymm15);
  vpxor(Dst, ymm15, Vector);
}

DEF_OP(VUMin) {
  const auto Op = IROp->C<IR::IROp_VUMin>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto IsScalar = OpSize == ElementSize;

  const auto Dst = GetDst(Node);
  const auto Vector1 = GetSrc(Op->Vector1.ID());
  const auto Vector2 = GetSrc(Op->Vector2.ID());

  if (IsScalar) {
    switch (ElementSize) {
      case 8: {
        // This isn't very nice on x86 until AVX-512
        pextrq(TMP1, Vector1, 0);
        pextrq(TMP2, Vector2, 0);
        cmp(TMP1, TMP2);
        cmovb(TMP2, TMP1);
        pinsrq(Dst, TMP2, 0);
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        break;
    }
  }
  else {
    const auto DstYMM = ToYMM(Dst);
    const auto Vector1YMM = ToYMM(Vector1);
    const auto Vector2YMM = ToYMM(Vector2);

    switch (ElementSize) {
      case 1: {
        vpminub(DstYMM, Vector1YMM, Vector2YMM);
        break;
      }
      case 2: {
        vpminuw(DstYMM, Vector1YMM, Vector2YMM);
        break;
      }
      case 4: {
        vpminud(DstYMM, Vector1YMM, Vector2YMM);
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        break;
    }
  }
}

DEF_OP(VSMin) {
  const auto Op = IROp->C<IR::IROp_VSMin>();

  const auto ElementSize = Op->Header.ElementSize;

  const auto Dst = ToYMM(GetDst(Node));
  const auto Vector1 = ToYMM(GetSrc(Op->Vector1.ID()));
  const auto Vector2 = ToYMM(GetSrc(Op->Vector2.ID()));

  switch (ElementSize) {
    case 1: {
      vpminsb(Dst, Vector1, Vector2);
      break;
    }
    case 2: {
      vpminsw(Dst, Vector1, Vector2);
      break;
    }
    case 4: {
      vpminsd(Dst, Vector1, Vector2);
      break;
    }
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
}

DEF_OP(VUMax) {
  const auto Op = IROp->C<IR::IROp_VUMax>();

  const auto ElementSize = Op->Header.ElementSize;

  const auto Dst = ToYMM(GetDst(Node));
  const auto Vector1 = ToYMM(GetSrc(Op->Vector1.ID()));
  const auto Vector2 = ToYMM(GetSrc(Op->Vector2.ID()));

  switch (ElementSize) {
    case 1: {
      vpmaxub(Dst, Vector1, Vector2);
      break;
    }
    case 2: {
      vpmaxuw(Dst, Vector1, Vector2);
      break;
    }
    case 4: {
      vpmaxud(Dst, Vector1, Vector2);
      break;
    }
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
}

DEF_OP(VSMax) {
  const auto Op = IROp->C<IR::IROp_VSMax>();

  const auto ElementSize = Op->Header.ElementSize;

  const auto Dst = ToYMM(GetDst(Node));
  const auto Vector1 = ToYMM(GetSrc(Op->Vector1.ID()));
  const auto Vector2 = ToYMM(GetSrc(Op->Vector2.ID()));

  switch (ElementSize) {
    case 1: {
      vpmaxsb(Dst, Vector1, Vector2);
      break;
    }
    case 2: {
      vpmaxsw(Dst, Vector1, Vector2);
      break;
    }
    case 4: {
      vpmaxsd(Dst, Vector1, Vector2);
      break;
    }
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
}

DEF_OP(VZip) {
  auto Op = IROp->C<IR::IROp_VZip>();
  movapd(xmm15, GetSrc(Op->VectorLower.ID()));

  switch (Op->Header.ElementSize) {
    case 1: {
      punpcklbw(xmm15, GetSrc(Op->VectorUpper.ID()));
      break;
    }
    case 2: {
      punpcklwd(xmm15, GetSrc(Op->VectorUpper.ID()));
      break;
    }
    case 4: {
      punpckldq(xmm15, GetSrc(Op->VectorUpper.ID()));
      break;
    }
    case 8: {
      punpcklqdq(xmm15, GetSrc(Op->VectorUpper.ID()));
      break;
    }
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
  movapd(GetDst(Node), xmm15);
}

DEF_OP(VZip2) {
  auto Op = IROp->C<IR::IROp_VZip2>();
  const uint8_t OpSize = IROp->Size;

  movapd(xmm15, GetSrc(Op->VectorLower.ID()));

  if (OpSize == 8) {
    vpslldq(xmm15, GetSrc(Op->VectorLower.ID()), 4);
    vpslldq(xmm14, GetSrc(Op->VectorUpper.ID()), 4);
    switch (Op->Header.ElementSize) {
    case 1: {
      vpunpckhbw(GetDst(Node), xmm15, xmm14);
    break;
    }
    case 2: {
      vpunpckhwd(GetDst(Node), xmm15, xmm14);
    break;
    }
    case 4: {
      vpunpckhdq(GetDst(Node), xmm15, xmm14);
    break;
    }
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
    }
  }
  else {
    switch (Op->Header.ElementSize) {
    case 1: {
      punpckhbw(xmm15, GetSrc(Op->VectorUpper.ID()));
    break;
    }
    case 2: {
      punpckhwd(xmm15, GetSrc(Op->VectorUpper.ID()));
    break;
    }
    case 4: {
      punpckhdq(xmm15, GetSrc(Op->VectorUpper.ID()));
    break;
    }
    case 8: {
      punpckhqdq(xmm15, GetSrc(Op->VectorUpper.ID()));
    break;
    }
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
    }
    movapd(GetDst(Node), xmm15);
  }
}

DEF_OP(VUnZip) {
  auto Op = IROp->C<IR::IROp_VUnZip>();
  const uint8_t OpSize = IROp->Size;

  if (OpSize == 8) {
    LOGMAN_MSG_A_FMT("Unsupported register size on VUnZip");
  }
  else {
    switch (Op->Header.ElementSize) {
      case 1: {
        // Shuffle low bits
        mov(rax, 0x0E'0C'0A'08'06'04'02'00); // Lower
        mov(rcx, 0x80'80'80'80'80'80'80'80); // Upper
        vmovq(xmm15, rax);
        pinsrq(xmm15, rcx, 1);
        vpshufb(xmm14, GetSrc(Op->VectorLower.ID()), xmm15);
        vpshufb(xmm13, GetSrc(Op->VectorUpper.ID()), xmm15);
        // movlhps back to combine
        vmovlhps(GetDst(Node), xmm14, xmm13);
        break;
      }
      case 2: {
        // Shuffle low bits
        mov(rax, 0x0D'0C'09'08'05'04'01'00); // Lower
        mov(rcx, 0x80'80'80'80'80'80'80'80); // Upper
        vmovq(xmm15, rax);
        pinsrq(xmm15, rcx, 1);
        vpshufb(xmm14, GetSrc(Op->VectorLower.ID()), xmm15);
        vpshufb(xmm13, GetSrc(Op->VectorUpper.ID()), xmm15);
        // movlhps back to combine
        vmovlhps(GetDst(Node), xmm14, xmm13);
        break;
      }
      case 4: {
        vshufps(GetDst(Node),
          GetSrc(Op->VectorLower.ID()),
          GetSrc(Op->VectorUpper.ID()),
          0b10'00'10'00);
        break;
      }
      case 8: {
        vshufpd(GetDst(Node),
          GetSrc(Op->VectorLower.ID()),
          GetSrc(Op->VectorUpper.ID()),
          0b0'0);
        break;
      }
      default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
    }
  }
}

DEF_OP(VUnZip2) {
  auto Op = IROp->C<IR::IROp_VUnZip2>();
  const uint8_t OpSize = IROp->Size;

  if (OpSize == 8) {
    LOGMAN_MSG_A_FMT("Unsupported register size on VUnZip2");
  }
  else {
    switch (Op->Header.ElementSize) {
      case 1: {
        // Shuffle low bits
        mov(rax, 0x0F'0D'0B'09'07'05'03'01); // Lower
        mov(rcx, 0x80'80'80'80'80'80'80'80); // Upper
        vmovq(xmm15, rax);
        pinsrq(xmm15, rcx, 1);
        vpshufb(xmm14, GetSrc(Op->VectorLower.ID()), xmm15);
        vpshufb(xmm13, GetSrc(Op->VectorUpper.ID()), xmm15);
        // movlhps back to combine
        vmovlhps(GetDst(Node), xmm14, xmm13);
        break;
      }
      case 2: {
        // Shuffle low bits
        mov(rax, 0x0F'0E'0B'0A'07'06'03'02); // Lower
        mov(rcx, 0x80'80'80'80'80'80'80'80); // Upper
        vmovq(xmm15, rax);
        pinsrq(xmm15, rcx, 1);
        vpshufb(xmm14, GetSrc(Op->VectorLower.ID()), xmm15);
        vpshufb(xmm13, GetSrc(Op->VectorUpper.ID()), xmm15);
        // movlhps back to combine
        vmovlhps(GetDst(Node), xmm14, xmm13);
        break;
      }
      case 4: {
        vshufps(GetDst(Node),
          GetSrc(Op->VectorLower.ID()),
          GetSrc(Op->VectorUpper.ID()),
          0b11'01'11'01);
        break;
      }
      case 8: {
        vshufpd(GetDst(Node),
          GetSrc(Op->VectorLower.ID()),
          GetSrc(Op->VectorUpper.ID()),
          0b1'1);
        break;
      }
      default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
    }
  }
}


DEF_OP(VBSL) {
  const auto Op = IROp->C<IR::IROp_VBSL>();

  const auto Dst = ToYMM(GetDst(Node));
  const auto VectorFalse = ToYMM(GetSrc(Op->VectorFalse.ID()));
  const auto VectorTrue = ToYMM(GetSrc(Op->VectorTrue.ID()));
  const auto VectorMask = ToYMM(GetSrc(Op->VectorMask.ID()));

  vpand(ymm0, VectorMask, VectorTrue);
  vpandn(ymm12, VectorMask, VectorFalse);
  vpor(Dst, ymm0, ymm12);
}

DEF_OP(VCMPEQ) {
  const auto Op = IROp->C<IR::IROp_VCMPEQ>();

  const auto ElementSize = Op->Header.ElementSize;

  const auto Dst = ToYMM(GetDst(Node));
  const auto Vector1 = ToYMM(GetSrc(Op->Vector1.ID()));
  const auto Vector2 = ToYMM(GetSrc(Op->Vector2.ID()));

  switch (ElementSize) {
    case 1:
      vpcmpeqb(Dst, Vector1, Vector2);
      break;
    case 2:
      vpcmpeqw(Dst, Vector1, Vector2);
      break;
    case 4:
      vpcmpeqd(Dst, Vector1, Vector2);
      break;
    case 8:
      vpcmpeqq(Dst, Vector1, Vector2);
      break;
    default:
      LOGMAN_MSG_A_FMT("Unsupported element size: {}", ElementSize);
      break;
  }
}

DEF_OP(VCMPEQZ) {
  const auto Op = IROp->C<IR::IROp_VCMPEQZ>();

  const auto ElementSize = Op->Header.ElementSize;

  const auto Dst = ToYMM(GetDst(Node));
  const auto Vector = ToYMM(GetSrc(Op->Vector.ID()));
  const auto ZeroVector = ymm15;

  vpxor(ZeroVector, ZeroVector, ZeroVector);
  switch (ElementSize) {
    case 1:
      vpcmpeqb(Dst, Vector, ZeroVector);
      break;
    case 2:
      vpcmpeqw(Dst, Vector, ZeroVector);
      break;
    case 4:
      vpcmpeqd(Dst, Vector, ZeroVector);
      break;
    case 8:
      vpcmpeqq(Dst, Vector, ZeroVector);
      break;
    default:
      LOGMAN_MSG_A_FMT("Unsupported element size: {}", ElementSize);
      break;
  }
}

DEF_OP(VCMPGT) {
  const auto Op = IROp->C<IR::IROp_VCMPGT>();

  const auto ElementSize = Op->Header.ElementSize;

  const auto Dst = ToYMM(GetDst(Node));
  const auto Vector1 = ToYMM(GetSrc(Op->Vector1.ID()));
  const auto Vector2 = ToYMM(GetSrc(Op->Vector2.ID()));

  switch (ElementSize) {
    case 1:
      vpcmpgtb(Dst, Vector1, Vector2);
      break;
    case 2:
      vpcmpgtw(Dst, Vector1, Vector2);
      break;
    case 4:
      vpcmpgtd(Dst, Vector1, Vector2);
      break;
    case 8:
      vpcmpgtq(Dst, Vector1, Vector2);
      break;
    default:
      LOGMAN_MSG_A_FMT("Unsupported element size: {}", ElementSize);
      break;
  }
}

DEF_OP(VCMPGTZ) {
  const auto Op = IROp->C<IR::IROp_VCMPGTZ>();

  const auto ElementSize = Op->Header.ElementSize;

  const auto Dst = ToYMM(GetDst(Node));
  const auto Vector = ToYMM(GetSrc(Op->Vector.ID()));
  const auto ZeroVector = ymm15;

  vpxor(ZeroVector, ZeroVector, ZeroVector);
  switch (ElementSize) {
    case 1:
      vpcmpgtb(Dst, Vector, ZeroVector);
      break;
    case 2:
      vpcmpgtw(Dst, Vector, ZeroVector);
      break;
    case 4:
      vpcmpgtd(Dst, Vector, ZeroVector);
      break;
    case 8:
      vpcmpgtq(Dst, Vector, ZeroVector);
      break;
    default:
      LOGMAN_MSG_A_FMT("Unsupported element size: {}", ElementSize);
      break;
  }
}

DEF_OP(VCMPLTZ) {
  const auto Op = IROp->C<IR::IROp_VCMPLTZ>();

  const auto ElementSize = Op->Header.ElementSize;

  const auto Dst = ToYMM(GetDst(Node));
  const auto Vector = ToYMM(GetSrc(Op->Vector.ID()));
  const auto ZeroVector = ymm15;

  vpxor(ZeroVector, ZeroVector, ZeroVector);
  switch (ElementSize) {
    case 1:
      vpcmpgtb(Dst, ZeroVector, Vector);
      break;
    case 2:
      vpcmpgtw(Dst, ZeroVector, Vector);
      break;
    case 4:
      vpcmpgtd(Dst, ZeroVector, Vector);
      break;
    case 8:
      vpcmpgtq(Dst, ZeroVector, Vector);
      break;
    default:
      LOGMAN_MSG_A_FMT("Unsupported element size: {}", ElementSize);
      break;
  }
}

DEF_OP(VFCMPEQ) {
  const auto Op = IROp->C<IR::IROp_VFCMPEQ>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto IsScalar = ElementSize == OpSize;

  const auto Dst = GetDst(Node);
  const auto Vector1 = GetSrc(Op->Vector1.ID());
  const auto Vector2 = GetSrc(Op->Vector2.ID());

  if (IsScalar) {
    switch (ElementSize) {
    case 4:
      vcmpss(Dst, Vector1, Vector2, 0);
      break;
    case 8:
      vcmpsd(Dst, Vector1, Vector2, 0);
      break;
    default:
      LOGMAN_MSG_A_FMT("Unsupported element size: {}", ElementSize);
      break;
    }
  }
  else {
    const auto DstYMM = ToYMM(Dst);
    const auto Vector1YMM = ToYMM(Vector1);
    const auto Vector2YMM = ToYMM(Vector2);

    switch (ElementSize) {
    case 4:
      vcmpps(DstYMM, Vector1YMM, Vector2YMM, 0);
      break;
    case 8:
      vcmppd(DstYMM, Vector1YMM, Vector2YMM, 0);
      break;
    default:
      LOGMAN_MSG_A_FMT("Unsupported element size: {}", ElementSize);
      break;
    }
  }
}

DEF_OP(VFCMPNEQ) {
  const auto Op = IROp->C<IR::IROp_VFCMPNEQ>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto IsScalar = ElementSize == OpSize;

  const auto Dst = GetDst(Node);
  const auto Vector1 = GetSrc(Op->Vector1.ID());
  const auto Vector2 = GetSrc(Op->Vector2.ID());

  if (IsScalar) {
    switch (ElementSize) {
    case 4:
      vcmpss(Dst, Vector1, Vector2, 4);
      break;
    case 8:
      vcmpsd(Dst, Vector1, Vector2, 4);
      break;
    default:
      LOGMAN_MSG_A_FMT("Unsupported element size: {}", ElementSize);
      break;
    }
  }
  else {
    const auto DstYMM = ToYMM(Dst);
    const auto Vector1YMM = ToYMM(Vector1);
    const auto Vector2YMM = ToYMM(Vector2);

    switch (ElementSize) {
    case 4:
      vcmpps(DstYMM, Vector1YMM, Vector2YMM, 4);
      break;
    case 8:
      vcmppd(DstYMM, Vector1YMM, Vector2YMM, 4);
      break;
    default:
      LOGMAN_MSG_A_FMT("Unsupported element size: {}", ElementSize);
      break;
    }
  }
}

DEF_OP(VFCMPLT) {
  const auto Op = IROp->C<IR::IROp_VFCMPLT>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto IsScalar = ElementSize == OpSize;

  const auto Dst = GetDst(Node);
  const auto Vector1 = GetSrc(Op->Vector1.ID());
  const auto Vector2 = GetSrc(Op->Vector2.ID());

  if (IsScalar) {
    switch (ElementSize) {
    case 4:
      vcmpss(Dst, Vector1, Vector2, 1);
      break;
    case 8:
      vcmpsd(Dst, Vector1, Vector2, 1);
      break;
    default:
      LOGMAN_MSG_A_FMT("Unsupported element size: {}", ElementSize);
      break;
    }
  }
  else {
    const auto DstYMM = ToYMM(Dst);
    const auto Vector1YMM = ToYMM(Vector1);
    const auto Vector2YMM = ToYMM(Vector2);

    switch (ElementSize) {
    case 4:
      vcmpps(DstYMM, Vector1YMM, Vector2YMM, 1);
      break;
    case 8:
      vcmppd(DstYMM, Vector1YMM, Vector2YMM, 1);
      break;
    default:
      LOGMAN_MSG_A_FMT("Unsupported element size: {}", ElementSize);
      break;
    }
  }
}

DEF_OP(VFCMPGT) {
  const auto Op = IROp->C<IR::IROp_VFCMPGT>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto IsScalar = ElementSize == OpSize;

  const auto Dst = GetDst(Node);
  const auto Vector1 = GetSrc(Op->Vector1.ID());
  const auto Vector2 = GetSrc(Op->Vector2.ID());

  if (IsScalar) {
    switch (ElementSize) {
    case 4:
      vcmpss(Dst, Vector2, Vector1, 1);
      break;
    case 8:
      vcmpsd(Dst, Vector2, Vector1, 1);
      break;
    default:
      LOGMAN_MSG_A_FMT("Unsupported element size: {}", ElementSize);
      break;
    }
  }
  else {
    const auto DstYMM = ToYMM(Dst);
    const auto Vector1YMM = ToYMM(Vector1);
    const auto Vector2YMM = ToYMM(Vector2);

    switch (ElementSize) {
    case 4:
      vcmpps(DstYMM, Vector2YMM, Vector1YMM, 1);
      break;
    case 8:
      vcmppd(DstYMM, Vector2YMM, Vector1YMM, 1);
      break;
    default:
      LOGMAN_MSG_A_FMT("Unsupported element size: {}", ElementSize);
      break;
    }
  }
}

DEF_OP(VFCMPLE) {
  const auto Op = IROp->C<IR::IROp_VFCMPLE>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto IsScalar = ElementSize == OpSize;

  const auto Dst = GetDst(Node);
  const auto Vector1 = GetSrc(Op->Vector1.ID());
  const auto Vector2 = GetSrc(Op->Vector2.ID());

  if (IsScalar) {
    switch (ElementSize) {
    case 4:
      vcmpss(Dst, Vector1, Vector2, 2);
      break;
    case 8:
      vcmpsd(Dst, Vector1, Vector2, 2);
      break;
    default:
      LOGMAN_MSG_A_FMT("Unsupported element size: {}", ElementSize);
      break;
    }
  }
  else {
    const auto DstYMM = ToYMM(Dst);
    const auto Vector1YMM = ToYMM(Vector1);
    const auto Vector2YMM = ToYMM(Vector2);

    switch (ElementSize) {
    case 4:
      vcmpps(DstYMM, Vector1YMM, Vector2YMM, 2);
      break;
    case 8:
      vcmppd(DstYMM, Vector1YMM, Vector2YMM, 2);
      break;
    default:
      LOGMAN_MSG_A_FMT("Unsupported element size: {}", ElementSize);
      break;
    }
  }
}

DEF_OP(VFCMPORD) {
  const auto Op = IROp->C<IR::IROp_VFCMPORD>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto IsScalar = ElementSize == OpSize;

  const auto Dst = GetDst(Node);
  const auto Vector1 = GetSrc(Op->Vector1.ID());
  const auto Vector2 = GetSrc(Op->Vector2.ID());

  if (IsScalar) {
    switch (ElementSize) {
    case 4:
      vcmpss(Dst, Vector1, Vector2, 7);
      break;
    case 8:
      vcmpsd(Dst, Vector1, Vector2, 7);
      break;
    default:
      LOGMAN_MSG_A_FMT("Unsupported element size: {}", ElementSize);
      break;
    }
  }
  else {
    const auto DstYMM = ToYMM(Dst);
    const auto Vector1YMM = ToYMM(Vector1);
    const auto Vector2YMM = ToYMM(Vector2);

    switch (ElementSize) {
    case 4:
      vcmpps(DstYMM, Vector1YMM, Vector2YMM, 7);
      break;
    case 8:
      vcmppd(DstYMM, Vector1YMM, Vector2YMM, 7);
      break;
    default:
      LOGMAN_MSG_A_FMT("Unsupported element size: {}", ElementSize);
      break;
    }
  }
}

DEF_OP(VFCMPUNO) {
  const auto Op = IROp->C<IR::IROp_VFCMPUNO>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto IsScalar = ElementSize == OpSize;

  const auto Dst = GetDst(Node);
  const auto Vector1 = GetSrc(Op->Vector1.ID());
  const auto Vector2 = GetSrc(Op->Vector2.ID());

  if (IsScalar) {
    switch (ElementSize) {
    case 4:
      vcmpss(Dst, Vector1, Vector2, 3);
      break;
    case 8:
      vcmpsd(Dst, Vector1, Vector2, 3);
      break;
    default:
      LOGMAN_MSG_A_FMT("Unsupported element size: {}", ElementSize);
      break;
    }
  }
  else {
    const auto DstYMM = ToYMM(Dst);
    const auto Vector1YMM = ToYMM(Vector1);
    const auto Vector2YMM = ToYMM(Vector2);

    switch (ElementSize) {
    case 4:
      vcmpps(DstYMM, Vector1YMM, Vector2YMM, 3);
      break;
    case 8:
      vcmppd(DstYMM, Vector1YMM, Vector2YMM, 3);
      break;
    default:
      LOGMAN_MSG_A_FMT("Unsupported element size: {}", ElementSize);
      break;
    }
  }
}

DEF_OP(VUShl) {
  LOGMAN_MSG_A_FMT("Unimplemented");
}

DEF_OP(VUShr) {
  LOGMAN_MSG_A_FMT("Unimplemented");
}

DEF_OP(VSShr) {
  LOGMAN_MSG_A_FMT("Unimplemented");
}

DEF_OP(VUShlS) {
  auto Op = IROp->C<IR::IROp_VUShlS>();

  switch (Op->Header.ElementSize) {
    case 2: {
      vpsllw(GetDst(Node), GetSrc(Op->Vector.ID()), GetSrc(Op->ShiftScalar.ID()));
      break;
    }
    case 4: {
      vpslld(GetDst(Node), GetSrc(Op->Vector.ID()), GetSrc(Op->ShiftScalar.ID()));
      break;
    }
    case 8: {
      vpsllq(GetDst(Node), GetSrc(Op->Vector.ID()), GetSrc(Op->ShiftScalar.ID()));
      break;
    }
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
}

DEF_OP(VUShrS) {
  auto Op = IROp->C<IR::IROp_VUShrS>();

  switch (Op->Header.ElementSize) {
    case 2: {
      vpsrlw(GetDst(Node), GetSrc(Op->Vector.ID()), GetSrc(Op->ShiftScalar.ID()));
      break;
    }
    case 4: {
      vpsrld(GetDst(Node), GetSrc(Op->Vector.ID()), GetSrc(Op->ShiftScalar.ID()));
      break;
    }
    case 8: {
      vpsrlq(GetDst(Node), GetSrc(Op->Vector.ID()), GetSrc(Op->ShiftScalar.ID()));
      break;
    }
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
}

DEF_OP(VSShrS) {
  auto Op = IROp->C<IR::IROp_VSShrS>();

  switch (Op->Header.ElementSize) {
    case 2: {
      vpsraw(GetDst(Node), GetSrc(Op->Vector.ID()), GetSrc(Op->ShiftScalar.ID()));
      break;
    }
    case 4: {
      vpsrad(GetDst(Node), GetSrc(Op->Vector.ID()), GetSrc(Op->ShiftScalar.ID()));
      break;
    }
    case 8: // Doesn't exist on x86
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
}

DEF_OP(VInsElement) {
  auto Op = IROp->C<IR::IROp_VInsElement>();
  movapd(xmm15, GetSrc(Op->DestVector.ID()));

  // Dst_d[Op->DestIdx] = Src2_d[Op->SrcIdx];

  // pextrq reg64/mem64, xmm, imm
  // pinsrq xmm, reg64/mem64, imm8
  switch (Op->Header.ElementSize) {
  case 1: {
    pextrb(eax, GetSrc(Op->SrcVector.ID()), Op->SrcIdx);
    pinsrb(xmm15, eax, Op->DestIdx);
  break;
  }
  case 2: {
    pextrw(eax, GetSrc(Op->SrcVector.ID()), Op->SrcIdx);
    pinsrw(xmm15, eax, Op->DestIdx);
  break;
  }
  case 4: {
    pextrd(eax, GetSrc(Op->SrcVector.ID()), Op->SrcIdx);
    pinsrd(xmm15, eax, Op->DestIdx);
  break;
  }
  case 8: {
    pextrq(rax, GetSrc(Op->SrcVector.ID()), Op->SrcIdx);
    pinsrq(xmm15, rax, Op->DestIdx);
  break;
  }
  default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }

  movapd(GetDst(Node), xmm15);
}

DEF_OP(VDupElement) {
  auto Op = IROp->C<IR::IROp_VDupElement>();

  switch (Op->Header.ElementSize) {
    case 1: {
      // First extract the index
      pextrb(eax, GetSrc(Op->Vector.ID()), Op->Index);
      // Insert it in to the first element of the destination
      pinsrb(GetDst(Node), eax, 0);
      pinsrb(GetDst(Node), eax, 1);
      // Shuffle low elements
      vpshuflw(GetDst(Node), GetSrc(Op->Vector.ID()), 0);
      // Insert element in to the first upper 64bit element
      pinsrb(GetDst(Node), eax, 8);
      pinsrb(GetDst(Node), eax, 9);
      // Shuffle high elements
      vpshufhw(GetDst(Node), GetSrc(Op->Vector.ID()), 0);
      break;
    }
    case 2: {
      // First extract the index
      pextrw(eax, GetSrc(Op->Vector.ID()), Op->Index);
      // Insert it in to the first element of the destination
      pinsrw(GetDst(Node), eax, 0);
      // Shuffle low elements
      vpshuflw(GetDst(Node), GetSrc(Op->Vector.ID()), 0);
      // Insert element in to the first upper 64bit element
      pinsrw(GetDst(Node), eax, 4);
      // Shuffle high elements
      vpshufhw(GetDst(Node), GetSrc(Op->Vector.ID()), 0);
      break;
    }
    case 4: {
      vpshufd(GetDst(Node),
        GetSrc(Op->Vector.ID()),
        (Op->Index << 0) |
        (Op->Index << 2) |
        (Op->Index << 4) |
        (Op->Index << 6));
      break;
    }
    case 8: {
      vshufpd(GetDst(Node),
        GetSrc(Op->Vector.ID()),
        GetSrc(Op->Vector.ID()),
        (Op->Index << 0) |
        (Op->Index << 1));
      break;
    }
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
}



DEF_OP(VExtr) {
  auto Op = IROp->C<IR::IROp_VExtr>();
  const uint8_t OpSize = IROp->Size;

  if (OpSize == 8) {
    // No way to do this with 64bit source without dropping to MMX
    // So emulate it
    vpxor(xmm14, xmm14, xmm14);
    movq(xmm15, GetSrc(Op->VectorUpper.ID()));
    vshufpd(xmm15, xmm15, GetSrc(Op->VectorLower.ID()), 0b00);
    vpalignr(GetDst(Node), xmm14, xmm15, Op->Index);
  }
  else {
    vpalignr(GetDst(Node), GetSrc(Op->VectorLower.ID()), GetSrc(Op->VectorUpper.ID()), Op->Index);
  }
}

DEF_OP(VSLI) {
  auto Op = IROp->C<IR::IROp_VSLI>();
  movapd(xmm15, GetSrc(Op->Vector.ID()));
  pslldq(xmm15, Op->ByteShift);
  movapd(GetDst(Node), xmm15);
}

DEF_OP(VSRI) {
  auto Op = IROp->C<IR::IROp_VSRI>();
  movapd(xmm15, GetSrc(Op->Vector.ID()));
  psrldq(xmm15, Op->ByteShift);
  movapd(GetDst(Node), xmm15);
}

DEF_OP(VUShrI) {
  const auto Op = IROp->C<IR::IROp_VUShrI>();

  const auto BitShift = Op->BitShift;
  const auto ElementSize = Op->Header.ElementSize;

  const auto Dst = ToYMM(GetDst(Node));
  const auto Vector = ToYMM(GetSrc(Op->Vector.ID()));

  switch (ElementSize) {
    case 2: {
      vpsrlw(Dst, Vector, BitShift);
      break;
    }
    case 4: {
      vpsrld(Dst, Vector, BitShift);
      break;
    }
    case 8: {
      vpsrlq(Dst, Vector, BitShift);
      break;
    }
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
}

DEF_OP(VSShrI) {
  auto Op = IROp->C<IR::IROp_VSShrI>();
  auto Dest = GetDst(Node);
  movapd(Dest, GetSrc(Op->Vector.ID()));
  switch (Op->Header.ElementSize) {
    case 1: {
      // This isn't a native instruction on x86
      const uint8_t OpSize = IROp->Size;
      const uint8_t Elements = OpSize / Op->Header.ElementSize;
      for (int i = 0; i < Elements; ++i) {
        pextrb(eax, Dest, i);
        movsx(eax, al);
        sar(al, Op->BitShift);
        pinsrb(Dest, eax, i);
      }
      break;
    }
    case 2: {
      psraw(Dest, Op->BitShift);
      break;
    }
    case 4: {
      psrad(Dest, Op->BitShift);
      break;
    }
    case 8: {
      // This isn't a native instruction on x86
      for (int i = 0; i < 2; ++i) {
        pextrq(rax, Dest, i);
        sar(rax, Op->BitShift);
        pinsrq(Dest, rax, i);
      }
      break;
    }
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
}

DEF_OP(VShlI) {
  auto Op = IROp->C<IR::IROp_VShlI>();
  movapd(GetDst(Node), GetSrc(Op->Vector.ID()));
  switch (Op->Header.ElementSize) {
    case 2: {
      psllw(GetDst(Node), Op->BitShift);
      break;
    }
    case 4: {
      pslld(GetDst(Node), Op->BitShift);
      break;
    }
    case 8: {
      psllq(GetDst(Node), Op->BitShift);
      break;
    }
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
}

DEF_OP(VUShrNI) {
  auto Op = IROp->C<IR::IROp_VUShrNI>();
  movapd(GetDst(Node), GetSrc(Op->Vector.ID()));
  vpxor(xmm15, xmm15, xmm15);
  switch (Op->Header.ElementSize) {
    case 1: {
      psrlw(GetDst(Node), Op->BitShift);
      // <8 x i16> -> <8 x i8>
      mov(rax, 0x0E'0C'0A'08'06'04'02'00); // Lower
      mov(rcx, 0x80'80'80'80'80'80'80'80); // Upper
      break;
    }
    case 2: {
      psrld(GetDst(Node), Op->BitShift);
      // <4 x i32> -> <4 x i16>
      mov(rax, 0x0D'0C'09'08'05'04'01'00); // Lower
      mov(rcx, 0x80'80'80'80'80'80'80'80); // Upper
      break;
    }
    case 4: {
      psrlq(GetDst(Node), Op->BitShift);
      // <2 x i64> -> <2 x i32>
      mov(rax, 0x0B'0A'09'08'03'02'01'00); // Lower
      mov(rcx, 0x80'80'80'80'80'80'80'80); // Upper
      break;
    }
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }

  vmovq(xmm15, rax);
  vmovq(xmm14, rcx);
  punpcklqdq(xmm15, xmm14);
  pshufb(GetDst(Node), xmm15);
}

DEF_OP(VUShrNI2) {
  // Src1 = Lower results
  // Src2 = Upper Results
  auto Op = IROp->C<IR::IROp_VUShrNI2>();
  movapd(xmm13, GetSrc(Op->VectorUpper.ID()));
  switch (Op->Header.ElementSize) {
    case 1: {
      psrlw(xmm13, Op->BitShift);
      // <8 x i16> -> <8 x i8>
      mov(rax, 0x80'80'80'80'80'80'80'80); // Lower
      mov(rcx, 0x0E'0C'0A'08'06'04'02'00); // Upper
      break;
    }
    case 2: {
      psrld(xmm13, Op->BitShift);
      // <4 x i32> -> <4 x i16>
      mov(rax, 0x80'80'80'80'80'80'80'80); // Lower
      mov(rcx, 0x0D'0C'09'08'05'04'01'00); // Upper
      break;
    }
    case 4: {
      psrlq(xmm13, Op->BitShift);
      // <2 x i64> -> <2 x i32>
      mov(rax, 0x80'80'80'80'80'80'80'80); // Lower
      mov(rcx, 0x0B'0A'09'08'03'02'01'00); // Upper
      break;
    }
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }

  vmovq(xmm15, rax);
  vmovq(xmm14, rcx);
  punpcklqdq(xmm15, xmm14);
  vpshufb(xmm14, xmm13, xmm15);
  vpor(GetDst(Node), xmm14, GetSrc(Op->VectorLower.ID()));
}

DEF_OP(VSXTL) {
  auto Op = IROp->C<IR::IROp_VSXTL>();
  switch (Op->Header.ElementSize) {
    case 2:
      pmovsxbw(GetDst(Node), GetSrc(Op->Vector.ID()));
    break;
    case 4:
      pmovsxwd(GetDst(Node), GetSrc(Op->Vector.ID()));
    break;
    case 8:
      pmovsxdq(GetDst(Node), GetSrc(Op->Vector.ID()));
    break;
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize);
  }
}

DEF_OP(VSXTL2) {
  auto Op = IROp->C<IR::IROp_VSXTL2>();
  uint8_t OpSize = IROp->Size;

  vpsrldq(GetDst(Node), GetSrc(Op->Vector.ID()), OpSize / 2);
  switch (Op->Header.ElementSize) {
    case 2:
      pmovsxbw(GetDst(Node), GetDst(Node));
    break;
    case 4:
      pmovsxwd(GetDst(Node), GetDst(Node));
    break;
    case 8:
      pmovsxdq(GetDst(Node), GetDst(Node));
    break;
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize);
  }
}

DEF_OP(VUXTL) {
  auto Op = IROp->C<IR::IROp_VUXTL>();
  switch (Op->Header.ElementSize) {
    case 2:
      pmovzxbw(GetDst(Node), GetSrc(Op->Vector.ID()));
    break;
    case 4:
      pmovzxwd(GetDst(Node), GetSrc(Op->Vector.ID()));
    break;
    case 8:
      pmovzxdq(GetDst(Node), GetSrc(Op->Vector.ID()));
    break;
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize);
  }
}

DEF_OP(VUXTL2) {
  auto Op = IROp->C<IR::IROp_VUXTL2>();
  uint8_t OpSize = IROp->Size;

  vpsrldq(GetDst(Node), GetSrc(Op->Vector.ID()), OpSize / 2);
  switch (Op->Header.ElementSize) {
    case 2:
      pmovzxbw(GetDst(Node), GetDst(Node));
    break;
    case 4:
      pmovzxwd(GetDst(Node), GetDst(Node));
    break;
    case 8:
      pmovzxdq(GetDst(Node), GetDst(Node));
    break;
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize);
  }
}

DEF_OP(VSQXTN) {
  auto Op = IROp->C<IR::IROp_VSQXTN>();
  switch (Op->Header.ElementSize) {
    case 1:
      packsswb(xmm15, GetSrc(Op->Vector.ID()));
    break;
    case 2:
      packssdw(xmm15, GetSrc(Op->Vector.ID()));
    break;
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize);
  }
  psrldq(xmm15, 8);
  movaps(GetDst(Node), xmm15);
}

DEF_OP(VSQXTN2) {
  auto Op = IROp->C<IR::IROp_VSQXTN2>();
  const uint8_t OpSize = IROp->Size;

  // Zero the lower bits
  vpxor(xmm15, xmm15, xmm15);
  switch (Op->Header.ElementSize) {
    case 1:
      packsswb(xmm15, GetSrc(Op->VectorUpper.ID()));
    break;
    case 2:
      packssdw(xmm15, GetSrc(Op->VectorUpper.ID()));
    break;
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize);
  }

  if (OpSize == 8) {
    psrldq(xmm15, OpSize / 2);
  }
  vpor(GetDst(Node), GetSrc(Op->VectorLower.ID()), xmm15);
}

DEF_OP(VSQXTUN) {
  auto Op = IROp->C<IR::IROp_VSQXTUN>();
  switch (Op->Header.ElementSize) {
    case 1:
      packuswb(xmm15, GetSrc(Op->Vector.ID()));
    break;
    case 2:
      packusdw(xmm15, GetSrc(Op->Vector.ID()));
    break;
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize);
  }
  psrldq(xmm15, 8);
  movaps(GetDst(Node), xmm15);
}

DEF_OP(VSQXTUN2) {
  auto Op = IROp->C<IR::IROp_VSQXTUN2>();
  const uint8_t OpSize = IROp->Size;

  // Zero the lower bits
  vpxor(xmm15, xmm15, xmm15);
  switch (Op->Header.ElementSize) {
    case 1:
      packuswb(xmm15, GetSrc(Op->VectorUpper.ID()));
    break;
    case 2:
      packusdw(xmm15, GetSrc(Op->VectorUpper.ID()));
    break;
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize);
  }
  if (OpSize == 8) {
    psrldq(xmm15, OpSize / 2);
  }

  vpor(GetDst(Node), GetSrc(Op->VectorLower.ID()), xmm15);
}

DEF_OP(VMul) {
  auto Op = IROp->C<IR::IROp_VUMul>();
  switch (Op->Header.ElementSize) {
    case 2: {
      vpmullw(GetDst(Node), GetSrc(Op->Vector1.ID()), GetSrc(Op->Vector2.ID()));
      break;
    }
    case 4: {
      vpmulld(GetDst(Node), GetSrc(Op->Vector1.ID()), GetSrc(Op->Vector2.ID()));
      break;
    }
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
}

DEF_OP(VUMull) {
  auto Op = IROp->C<IR::IROp_VUMull>();
  switch (Op->Header.ElementSize) {
    case 4: {
      // IR operation:
      // [31:00 ] = src1[15:00] * src2[15:00]
      // [63:32 ] = src1[31:16] * src2[31:16]
      // [95:64 ] = src1[47:32] * src2[47:32]
      // [127:96] = src1[63:48] * src2[63:48]
      //
      vpxor(xmm15, xmm15, xmm15);
      vpxor(xmm14, xmm14, xmm14);
      vpunpcklwd(xmm15, GetSrc(Op->Vector1.ID()), xmm15);
      vpunpcklwd(xmm14, GetSrc(Op->Vector2.ID()), xmm14);
      vpmulld(GetDst(Node), xmm14, xmm15);
      break;
    }
    case 8: {
      // We need to shuffle the data for this one
      // x86 PMULUDQ wants the 32bit values in [31:0] and [95:64]
      // Which then extends out to [63:0] and [127:64]
      vpshufd(xmm14, GetSrc(Op->Vector1.ID()), 0b10'10'00'00);
      vpshufd(xmm15, GetSrc(Op->Vector2.ID()), 0b10'10'00'00);

      vpmuludq(GetDst(Node), xmm14, xmm15);
    break;
    }
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
}

DEF_OP(VSMull) {
  auto Op = IROp->C<IR::IROp_VSMull>();
  switch (Op->Header.ElementSize) {
    case 4: {
      // IR operation:
      // [31:00 ] = src1[15:00] * src2[15:00]
      // [63:32 ] = src1[31:16] * src2[31:16]
      // [95:64 ] = src1[47:32] * src2[47:32]
      // [127:96] = src1[63:48] * src2[63:48]
      //
      vpxor(xmm15, xmm15, xmm15);
      vpxor(xmm14, xmm14, xmm14);
      vpunpcklwd(xmm15, GetSrc(Op->Vector1.ID()), xmm15);
      vpunpcklwd(xmm14, GetSrc(Op->Vector2.ID()), xmm14);
      pslld(xmm15, 16);
      pslld(xmm14, 16);
      psrad(xmm15, 16);
      psrad(xmm14, 16);
      vpmulld(GetDst(Node), xmm14, xmm15);
      break;
    }
    case 8: {
      // We need to shuffle the data for this one
      // x86 PMULDQ wants the 32bit values in [31:0] and [95:64]
      // Which then extends out to [63:0] and [127:64]
      vpshufd(xmm14, GetSrc(Op->Vector1.ID()), 0b10'10'00'00);
      vpshufd(xmm15, GetSrc(Op->Vector2.ID()), 0b10'10'00'00);

      vpmuldq(GetDst(Node), xmm14, xmm15);
    break;
    }
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
}

DEF_OP(VUMull2) {
  auto Op = IROp->C<IR::IROp_VUMull2>();
  switch (Op->Header.ElementSize) {
    case 4: {
      // IR operation:
      // [31:00 ] = src1[79:64  ] * src2[79:64  ]
      // [63:32 ] = src1[95:80  ] * src2[95:80  ]
      // [95:64 ] = src1[111:96 ] * src2[111:96 ]
      // [127:96] = src1[127:112] * src2[127:112]
      //
      vpxor(xmm15, xmm15, xmm15);
      vpxor(xmm14, xmm14, xmm14);
      vpunpckhwd(xmm15, GetSrc(Op->Vector1.ID()), xmm15);
      vpunpckhwd(xmm14, GetSrc(Op->Vector2.ID()), xmm14);
      vpmulld(GetDst(Node), xmm14, xmm15);
      break;
    }
    case 8: {
      // IR operation:
      // [63:00 ] = src1[95:64 ] * src2[95:64 ]
      // [127:64] = src1[127:96] * src2[127:96]
      //
      // x86 vpmuludq
      // [63:00 ] = src1[31:0 ] * src2[31:0 ]
      // [127:64] = src1[95:64] * src2[95:64]

      vpshufd(xmm14, GetSrc(Op->Vector1.ID()), 0b11'11'10'10);
      vpshufd(xmm15, GetSrc(Op->Vector2.ID()), 0b11'11'10'10);

      vpmuludq(GetDst(Node), xmm14, xmm15);
    break;
    }
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
}

DEF_OP(VSMull2) {
  auto Op = IROp->C<IR::IROp_VSMull2>();
  switch (Op->Header.ElementSize) {
    case 4: {
      // IR operation:
      // [31:00 ] = src1[79:64  ] * src2[79:64  ]
      // [63:32 ] = src1[95:80  ] * src2[95:80  ]
      // [95:64 ] = src1[111:96 ] * src2[111:96 ]
      // [127:96] = src1[127:112] * src2[127:112]
      //
      vpxor(xmm15, xmm15, xmm15);
      vpxor(xmm14, xmm14, xmm14);
      vpunpckhwd(xmm15, GetSrc(Op->Vector1.ID()), xmm15);
      vpunpckhwd(xmm14, GetSrc(Op->Vector2.ID()), xmm14);
      pslld(xmm15, 16);
      pslld(xmm14, 16);
      psrad(xmm15, 16);
      psrad(xmm14, 16);
      vpmulld(GetDst(Node), xmm14, xmm15);
      break;
    }
    case 8: {
      // IR operation:
      // [63:00 ] = src1[95:64 ] * src2[95:64 ]
      // [127:64] = src1[127:96] * src2[127:96]
      //
      // x86 vpmuludq
      // [63:00 ] = src1[31:0 ] * src2[31:0 ]
      // [127:64] = src1[95:64] * src2[95:64]

      vpshufd(xmm14, GetSrc(Op->Vector1.ID()), 0b11'11'10'10);
      vpshufd(xmm15, GetSrc(Op->Vector2.ID()), 0b11'11'10'10);

      vpmuldq(GetDst(Node), xmm14, xmm15);
    break;
    }
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
}

DEF_OP(VUABDL) {
  auto Op = IROp->C<IR::IROp_VUABDL>();
  switch (Op->Header.ElementSize) {
    case 2: {
      pmovzxbw(xmm14, GetSrc(Op->Vector1.ID()));
      pmovzxbw(xmm15, GetSrc(Op->Vector2.ID()));
      vpsubw(GetDst(Node), xmm14, xmm15);
      vpabsw(GetDst(Node), GetDst(Node));
      break;
    }
    case 4: {
      pmovzxwd(xmm14, GetSrc(Op->Vector1.ID()));
      pmovzxwd(xmm15, GetSrc(Op->Vector2.ID()));
      vpsubd(GetDst(Node), xmm14, xmm15);
      vpabsd(GetDst(Node), GetDst(Node));
      break;
    }
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
}

DEF_OP(VTBL1) {
  auto Op = IROp->C<IR::IROp_VTBL1>();
  const uint8_t OpSize = IROp->Size;

  switch (OpSize) {
    case 8: {
      vpshufb(GetDst(Node), GetSrc(Op->VectorTable.ID()), GetSrc(Op->VectorIndices.ID()));
      movq(GetDst(Node), GetDst(Node));
      break;
    }
    case 16: {
      vpshufb(GetDst(Node), GetSrc(Op->VectorTable.ID()), GetSrc(Op->VectorIndices.ID()));
      break;
    }
    default: LOGMAN_MSG_A_FMT("Unknown OpSize: {}", OpSize); break;
  }
}

DEF_OP(VRev64) {
  auto Op = IROp->C<IR::IROp_VRev64>();

  switch (Op->Header.ElementSize) {
    case 1: {
      mov(rax, 0x00'01'02'03'04'05'06'07); // Lower
      vmovq(xmm15, rax);
      if (IROp->Size == 16) {
        // Full 8bit byteswap in each 64-bit element
        mov(rcx, 0x08'09'0A'0B'0C'0D'0E'0F); // Upper
        pinsrq(xmm15, rcx, 1);
      }
      else {
        // 8byte, upper bits get zero
        // Full 8bit byteswap in each 64-bit element
        mov(rcx, 0x80'80'80'80'80'80'80'80); // Upper
        pinsrq(xmm15, rcx, 1);
      }

      vpshufb(GetDst(Node), GetSrc(Op->Vector.ID()), xmm15);
      break;
    }
    case 2: {
      // Full 16-bit byteswap in each 64-bit element
      mov(rax, 0x01'00'03'02'05'04'07'06); // Lower
      vmovq(xmm15, rax);
      if (IROp->Size == 16) {
        mov(rcx, 0x09'08'0B'0A'0D'0C'0F'0E); // Upper
        pinsrq(xmm15, rcx, 1);
      }
      else {
        // 8byte, upper bits get zero
        // Full 8bit byteswap in each 64-bit element
        mov(rcx, 0x80'80'80'80'80'80'80'80); // Upper
        pinsrq(xmm15, rcx, 1);
      }
      vpshufb(GetDst(Node), GetSrc(Op->Vector.ID()), xmm15);
      break;
    }
    case 4: {
      if (IROp->Size == 16) {
      vpshufd(GetDst(Node),
        GetSrc(Op->Vector.ID()),
        (0b11 << 0) |
        (0b10 << 2) |
        (0b01 << 4) |
        (0b00 << 6));
      }
      else {

      vpshufd(GetDst(Node),
        GetSrc(Op->Vector.ID()),
        (0b01 << 0) |
        (0b00 << 2) |
        (0b11 << 4) | // Last two don't matter, will be overwritten with zero
        (0b11 << 6));

        // Zero upper 64-bits
        mov(rcx, 0);
        pinsrq(GetDst(Node), rcx, 1);
      }
      break;
    }
    default: LOGMAN_MSG_A_FMT("Unknown Element Size: {}", Op->Header.ElementSize); break;
  }
}


#undef DEF_OP
void X86JITCore::RegisterVectorHandlers() {
#define REGISTER_OP(op, x) OpHandlers[FEXCore::IR::IROps::OP_##op] = &X86JITCore::Op_##x
  REGISTER_OP(VECTORZERO,        VectorZero);
  REGISTER_OP(VECTORIMM,         VectorImm);
  REGISTER_OP(VMOV,              VMov);
  REGISTER_OP(VAND,              VAnd);
  REGISTER_OP(VBIC,              VBic);
  REGISTER_OP(VOR,               VOr);
  REGISTER_OP(VXOR,              VXor);
  REGISTER_OP(VADD,              VAdd);
  REGISTER_OP(VSUB,              VSub);
  REGISTER_OP(VUQADD,            VUQAdd);
  REGISTER_OP(VUQSUB,            VUQSub);
  REGISTER_OP(VSQADD,            VSQAdd);
  REGISTER_OP(VSQSUB,            VSQSub);
  REGISTER_OP(VADDP,             VAddP);
  REGISTER_OP(VADDV,             VAddV);
  REGISTER_OP(VUMINV,            VUMinV);
  REGISTER_OP(VURAVG,            VURAvg);
  REGISTER_OP(VABS,              VAbs);
  REGISTER_OP(VPOPCOUNT,         VPopcount);
  REGISTER_OP(VFADD,             VFAdd);
  REGISTER_OP(VFADDP,            VFAddP);
  REGISTER_OP(VFSUB,             VFSub);
  REGISTER_OP(VFMUL,             VFMul);
  REGISTER_OP(VFDIV,             VFDiv);
  REGISTER_OP(VFMIN,             VFMin);
  REGISTER_OP(VFMAX,             VFMax);
  REGISTER_OP(VFRECP,            VFRecp);
  REGISTER_OP(VFSQRT,            VFSqrt);
  REGISTER_OP(VFRSQRT,           VFRSqrt);
  REGISTER_OP(VNEG,              VNeg);
  REGISTER_OP(VFNEG,             VFNeg);
  REGISTER_OP(VNOT,              VNot);
  REGISTER_OP(VUMIN,             VUMin);
  REGISTER_OP(VSMIN,             VSMin);
  REGISTER_OP(VUMAX,             VUMax);
  REGISTER_OP(VSMAX,             VSMax);
  REGISTER_OP(VZIP,              VZip);
  REGISTER_OP(VZIP2,             VZip2);
  REGISTER_OP(VUNZIP,            VUnZip);
  REGISTER_OP(VUNZIP2,           VUnZip2);
  REGISTER_OP(VBSL,              VBSL);
  REGISTER_OP(VCMPEQ,            VCMPEQ);
  REGISTER_OP(VCMPEQZ,           VCMPEQZ);
  REGISTER_OP(VCMPGT,            VCMPGT);
  REGISTER_OP(VCMPGTZ,           VCMPGTZ);
  REGISTER_OP(VCMPLTZ,           VCMPLTZ);
  REGISTER_OP(VFCMPEQ,           VFCMPEQ);
  REGISTER_OP(VFCMPNEQ,          VFCMPNEQ);
  REGISTER_OP(VFCMPLT,           VFCMPLT);
  REGISTER_OP(VFCMPGT,           VFCMPGT);
  REGISTER_OP(VFCMPLE,           VFCMPLE);
  REGISTER_OP(VFCMPORD,          VFCMPORD);
  REGISTER_OP(VFCMPUNO,          VFCMPUNO);
  REGISTER_OP(VUSHL,             VUShl);
  REGISTER_OP(VUSHR,             VUShr);
  REGISTER_OP(VSSHR,             VSShr);
  REGISTER_OP(VUSHLS,            VUShlS);
  REGISTER_OP(VUSHRS,            VUShrS);
  REGISTER_OP(VSSHRS,            VSShrS);
  REGISTER_OP(VINSELEMENT,       VInsElement);
  REGISTER_OP(VDUPELEMENT,       VDupElement);
  REGISTER_OP(VEXTR,             VExtr);
  REGISTER_OP(VSLI,              VSLI);
  REGISTER_OP(VSRI,              VSRI);
  REGISTER_OP(VUSHRI,            VUShrI);
  REGISTER_OP(VSSHRI,            VSShrI);
  REGISTER_OP(VSHLI,             VShlI);
  REGISTER_OP(VUSHRNI,           VUShrNI);
  REGISTER_OP(VUSHRNI2,          VUShrNI2);
  REGISTER_OP(VSXTL,             VSXTL);
  REGISTER_OP(VSXTL2,            VSXTL2);
  REGISTER_OP(VUXTL,             VUXTL);
  REGISTER_OP(VUXTL2,            VUXTL2);
  REGISTER_OP(VSQXTN,            VSQXTN);
  REGISTER_OP(VSQXTN2,           VSQXTN2);
  REGISTER_OP(VSQXTUN,           VSQXTUN);
  REGISTER_OP(VSQXTUN2,          VSQXTUN2);
  REGISTER_OP(VUMUL,             VMul);
  REGISTER_OP(VSMUL,             VMul);
  REGISTER_OP(VUMULL,            VUMull);
  REGISTER_OP(VSMULL,            VSMull);
  REGISTER_OP(VUMULL2,           VUMull2);
  REGISTER_OP(VSMULL2,           VSMull2);
  REGISTER_OP(VUABDL,            VUABDL);
  REGISTER_OP(VTBL1,             VTBL1);
  REGISTER_OP(VREV64,            VRev64);
#undef REGISTER_OP
}
}

