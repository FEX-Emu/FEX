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
  const auto Op = IROp->C<IR::IROp_VectorImm>();
  const auto OpSize = IROp->Size;

  const auto Is128Bit = OpSize == Core::CPUState::XMM_SSE_REG_SIZE;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

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

  if (Is256Bit) {
    vpbroadcastq(ToYMM(Dst), Dst);
  } else if (Is128Bit) {
    vpbroadcastq(Dst, Dst);
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
  const auto Op = IROp->C<IR::IROp_VAddV>();
  const auto OpSize = IROp->Size;

  const auto Src = GetSrc(Op->Vector.ID());
  const auto Dest = GetDst(Node);

  const auto ElementSize = Op->Header.ElementSize;
  const auto Elements = OpSize / ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  vpxor(xmm15, xmm15, xmm15);

  switch (ElementSize) {
    case 2: {
      const auto HorizontalAdd = [this, Elements](const Xbyak::Xmm& dst, Xbyak::Xmm src, const Xbyak::Xmm& tmp) {
        for (int i = Elements; i > 1; i >>= 1) {
          vphaddw(dst, src, dst);
          src = dst;
        }
        pextrw(eax, dst, 0);
        pinsrw(tmp, eax, 0);
      };

      if (Is256Bit) {
        vpxor(xmm13, xmm13, xmm13);
        vextracti128(xmm14, ToYMM(Src), 1);

        HorizontalAdd(Dest, Src, xmm15);
        HorizontalAdd(Dest, xmm14, xmm13);

        vpaddw(Dest, xmm13, xmm15);
      } else {
        HorizontalAdd(Dest, Src, xmm15);
        vmovaps(Dest, xmm15);
      }
      break;
    }
    case 4: {
      const auto HorizontalAdd = [this, Elements](const Xbyak::Xmm& dst, Xbyak::Xmm src, const Xbyak::Xmm& tmp) {
        for (int i = Elements; i > 1; i >>= 1) {
          vphaddd(dst, src, dst);
          src = dst;
        }
        pextrd(eax, dst, 0);
        pinsrd(tmp, eax, 0);
      };

      if (Is256Bit) {
        vpxor(xmm13, xmm13, xmm13);
        vextracti128(xmm14, ToYMM(Src), 1);

        HorizontalAdd(Dest, Src, xmm15);
        HorizontalAdd(Dest, xmm14, xmm13);

        vpaddd(Dest, xmm13, xmm15);
      } else {
        HorizontalAdd(Dest, Src, xmm15);
        vmovaps(Dest, xmm15);
      }
      break;
    }
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
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
  const auto Op = IROp->C<IR::IROp_VZip>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetDst(Node);
  const auto VectorLower = GetSrc(Op->VectorLower.ID());
  const auto VectorUpper = GetSrc(Op->VectorUpper.ID());

  if (Is256Bit) {
    switch (ElementSize) {
      case 1: {
        vpunpcklbw(xmm15, VectorLower, VectorUpper);
        vpunpckhbw(xmm14, VectorLower, VectorUpper);
        break;
      }
      case 2: {
        vpunpcklwd(xmm15, VectorLower, VectorUpper);
        vpunpckhwd(xmm14, VectorLower, VectorUpper);
        break;
      }
      case 4: {
        vpunpckldq(xmm15, VectorLower, VectorUpper);
        vpunpckhdq(xmm14, VectorLower, VectorUpper);
        break;
      }
      case 8: {
        vpunpcklqdq(xmm15, VectorLower, VectorUpper);
        vpunpckhqdq(xmm14, VectorLower, VectorUpper);
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        return;
    }

    vinserti128(ymm15, ymm15, xmm14, 1);
    vmovapd(ToYMM(Dst), ymm15);
  } else {
    switch (ElementSize) {
      case 1: {
        vpunpcklbw(Dst, VectorLower, VectorUpper);
        break;
      }
      case 2: {
        vpunpcklwd(Dst, VectorLower, VectorUpper);
        break;
      }
      case 4: {
        vpunpckldq(Dst, VectorLower, VectorUpper);
        break;
      }
      case 8: {
        vpunpcklqdq(Dst, VectorLower, VectorUpper);
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        break;
    }
  }
}

DEF_OP(VZip2) {
  const auto Op = IROp->C<IR::IROp_VZip2>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetDst(Node);
  const auto VectorLower = GetSrc(Op->VectorLower.ID());
  const auto VectorUpper = GetSrc(Op->VectorUpper.ID());

  if (Is256Bit) {
     vextracti128(xmm15, ToYMM(VectorLower), 1);
     vextracti128(xmm14, ToYMM(VectorUpper), 1);

     switch (ElementSize) {
       case 1: {
         vpunpcklbw(Dst, xmm15, xmm14);
         vpunpckhbw(xmm13, xmm15, xmm14);
         break;
       }
       case 2: {
         vpunpcklwd(Dst, xmm15, xmm14);
         vpunpckhwd(xmm13, xmm15, xmm14);
         break;
       }
       case 4: {
         vpunpckldq(Dst, xmm15, xmm14);
         vpunpckhdq(xmm13, xmm15, xmm14);
         break;
       }
       case 8: {
         vpunpcklqdq(Dst, xmm15, xmm14);
         vpunpckhqdq(xmm13, xmm15, xmm14);
         break;
       }
       default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        return;
     }

     vinserti128(ToYMM(Dst), ToYMM(Dst), xmm13, 1);
  } else {
    if (OpSize == 8) {
      vpslldq(xmm15, VectorLower, 4);
      vpslldq(xmm14, VectorUpper, 4);
      switch (ElementSize) {
      case 1: {
        vpunpckhbw(Dst, xmm15, xmm14);
        break;
      }
      case 2: {
        vpunpckhwd(Dst, xmm15, xmm14);
        break;
      }
      case 4: {
        vpunpckhdq(Dst, xmm15, xmm14);
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        break;
      }
    } else {
      switch (ElementSize) {
      case 1: {
        vpunpckhbw(Dst, VectorLower, VectorUpper);
        break;
      }
      case 2: {
        vpunpckhwd(Dst, VectorLower, VectorUpper);
        break;
      }
      case 4: {
        vpunpckhdq(Dst, VectorLower, VectorUpper);
        break;
      }
      case 8: {
        vpunpckhqdq(Dst, VectorLower, VectorUpper);
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        break;
      }
    }
  }
}

DEF_OP(VUnZip) {
  const auto Op = IROp->C<IR::IROp_VUnZip>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetDst(Node);
  const auto VectorLower = GetSrc(Op->VectorLower.ID());
  const auto VectorUpper = GetSrc(Op->VectorUpper.ID());

  if (OpSize == 8) {
    LOGMAN_MSG_A_FMT("Unsupported register size on VUnZip");
  }
  else {
    switch (ElementSize) {
      case 1: {
        // Shuffle low bits
        mov(rax, 0x0E'0C'0A'08'06'04'02'00); // Lower
        mov(rcx, 0x80'80'80'80'80'80'80'80); // Upper
        vmovq(xmm15, rax);
        pinsrq(xmm15, rcx, 1);
        if (Is256Bit) {
          vinserti128(ymm15, ymm15, xmm15, 1);
          vpshufb(ymm14, ToYMM(VectorLower), ymm15);
          vpshufb(ymm13, ToYMM(VectorUpper), ymm15);
          vpunpcklqdq(ToYMM(Dst), ymm14, ymm13);
        } else {
          vpshufb(xmm14, VectorLower, xmm15);
          vpshufb(xmm13, VectorUpper, xmm15);
          // movlhps back to combine
          vmovlhps(Dst, xmm14, xmm13);
        }
        break;
      }
      case 2: {
        // Shuffle low bits
        mov(rax, 0x0D'0C'09'08'05'04'01'00); // Lower
        mov(rcx, 0x80'80'80'80'80'80'80'80); // Upper
        vmovq(xmm15, rax);
        pinsrq(xmm15, rcx, 1);
        if (Is256Bit) {
          vinserti128(ymm15, ymm15, xmm15, 1);
          vpshufb(ymm14, ToYMM(VectorLower), ymm15);
          vpshufb(ymm13, ToYMM(VectorUpper), ymm15);
          vpunpcklqdq(ToYMM(Dst), ymm14, ymm13);
        } else {
          vpshufb(xmm14, VectorLower, xmm15);
          vpshufb(xmm13, VectorUpper, xmm15);
          // movlhps back to combine
          vmovlhps(Dst, xmm14, xmm13);
        }
        break;
      }
      case 4: {
        if (Is256Bit) {
          vshufps(ToYMM(Dst), ToYMM(VectorLower), ToYMM(VectorUpper), 0b10'00'10'00);
        } else {
          vshufps(Dst, VectorLower, VectorUpper, 0b10'00'10'00);
        }
        break;
      }
      case 8: {
        if (Is256Bit) {
          vshufpd(ToYMM(Dst), ToYMM(VectorLower), ToYMM(VectorUpper), 0b0'0);
        } else {
          vshufpd(Dst, VectorLower, VectorUpper, 0b0'0);
        }
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        break;
    }
  }
}

DEF_OP(VUnZip2) {
  const auto Op = IROp->C<IR::IROp_VUnZip2>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetDst(Node);
  const auto VectorLower = GetSrc(Op->VectorLower.ID());
  const auto VectorUpper = GetSrc(Op->VectorUpper.ID());

  if (OpSize == 8) {
    LOGMAN_MSG_A_FMT("Unsupported register size on VUnZip2");
  }
  else {
    switch (ElementSize) {
      case 1: {
        // Shuffle low bits
        mov(rax, 0x0F'0D'0B'09'07'05'03'01); // Lower
        mov(rcx, 0x80'80'80'80'80'80'80'80); // Upper
        vmovq(xmm15, rax);
        pinsrq(xmm15, rcx, 1);
        if (Is256Bit) {
          vinserti128(ymm15, ymm15, xmm15, 1);
          vpshufb(ymm14, ToYMM(VectorLower), ymm15);
          vpshufb(ymm13, ToYMM(VectorUpper), ymm15);
          vpunpcklqdq(ToYMM(Dst), ymm14, ymm13);
        } else {
          vpshufb(xmm14, VectorLower, xmm15);
          vpshufb(xmm13, VectorUpper, xmm15);
          // movlhps back to combine
          vmovlhps(Dst, xmm14, xmm13);
        }
        break;
      }
      case 2: {
        // Shuffle low bits
        mov(rax, 0x0F'0E'0B'0A'07'06'03'02); // Lower
        mov(rcx, 0x80'80'80'80'80'80'80'80); // Upper
        vmovq(xmm15, rax);
        pinsrq(xmm15, rcx, 1);
        if (Is256Bit) {
          vinserti128(ymm15, ymm15, xmm15, 1);
          vpshufb(ymm14, ToYMM(VectorLower), ymm15);
          vpshufb(ymm13, ToYMM(VectorUpper), ymm15);
          vpunpcklqdq(ToYMM(Dst), ymm14, ymm13);
        } else {
          vpshufb(xmm14, VectorLower, xmm15);
          vpshufb(xmm13, VectorUpper, xmm15);
          // movlhps back to combine
          vmovlhps(Dst, xmm14, xmm13);
        }
        break;
      }
      case 4: {
        if (Is256Bit) {
          vshufps(ToYMM(Dst), ToYMM(VectorLower), ToYMM(VectorUpper), 0b11'01'11'01);
        } else {
          vshufps(Dst, VectorLower, VectorUpper, 0b11'01'11'01);
        }
        break;
      }
      case 8: {
        if (Is256Bit) {
          vshufpd(ToYMM(Dst), ToYMM(VectorLower), ToYMM(VectorUpper), 0b1'1);
        } else {
          vshufpd(Dst, VectorLower, VectorUpper, 0b1'1);
        }
        break;
      }
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        break;
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
  const auto Op = IROp->C<IR::IROp_VUShlS>();

  const auto ElementSize = Op->Header.ElementSize;

  const auto Dst = ToYMM(GetDst(Node));
  const auto ShiftScalar = GetSrc(Op->ShiftScalar.ID());
  const auto Vector = ToYMM(GetSrc(Op->Vector.ID()));

  switch (ElementSize) {
    case 2: {
      vpsllw(Dst, Vector, ShiftScalar);
      break;
    }
    case 4: {
      vpslld(Dst, Vector, ShiftScalar);
      break;
    }
    case 8: {
      vpsllq(Dst, Vector, ShiftScalar);
      break;
    }
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
}

DEF_OP(VUShrS) {
  const auto Op = IROp->C<IR::IROp_VUShrS>();

  const auto ElementSize = Op->Header.ElementSize;

  const auto Dst = ToYMM(GetDst(Node));
  const auto ShiftScalar = GetSrc(Op->ShiftScalar.ID());
  const auto Vector = ToYMM(GetSrc(Op->Vector.ID()));

  switch (ElementSize) {
    case 2: {
      vpsrlw(Dst, Vector, ShiftScalar);
      break;
    }
    case 4: {
      vpsrld(Dst, Vector, ShiftScalar);
      break;
    }
    case 8: {
      vpsrlq(Dst, Vector, ShiftScalar);
      break;
    }
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
}

DEF_OP(VSShrS) {
  const auto Op = IROp->C<IR::IROp_VSShrS>();

  const auto ElementSize = Op->Header.ElementSize;

  const auto Dst = ToYMM(GetDst(Node));
  const auto ShiftScalar = GetSrc(Op->ShiftScalar.ID());
  const auto Vector = ToYMM(GetSrc(Op->Vector.ID()));

  switch (ElementSize) {
    case 2: {
      vpsraw(Dst, Vector, ShiftScalar);
      break;
    }
    case 4: {
      vpsrad(Dst, Vector, ShiftScalar);
      break;
    }
    case 8: // VPSRAQ is only introduced in AVX-512
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
}

DEF_OP(VInsElement) {
  const auto Op = IROp->C<IR::IROp_VInsElement>();
  const auto OpSize = IROp->Size;

  const auto DestIdx = Op->DestIdx;
  const auto SrcIdx = Op->SrcIdx;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetDst(Node);
  const auto DestVector = GetSrc(Op->DestVector.ID());
  const auto SrcVector = GetSrc(Op->SrcVector.ID());

  // Actually performs the insertion from the source vector into the destination vector.
  const auto PerformInsertion = [this, ElementSize](const Xbyak::Xmm& Src, uint32_t SrcIndex,
                                                    const Xbyak::Xmm& Dest, uint32_t DestIndex) {
    switch (ElementSize) {
    case 1: {
      pextrb(eax, Src, SrcIndex);
      pinsrb(Dest, eax, DestIndex);
      break;
    }
    case 2: {
      pextrw(eax, Src, SrcIndex);
      pinsrw(Dest, eax, DestIndex);
      break;
    }
    case 4: {
      pextrd(eax, Src, SrcIndex);
      pinsrd(Dest, eax, DestIndex);
      break;
    }
    case 8: {
      pextrq(rax, Src, SrcIndex);
      pinsrq(Dest, rax, DestIndex);
      break;
    }
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
    }
  };

  if (Is256Bit) {
    // Whether or not the index is in the upper lane.
    const auto IsUpperIdx = [ElementSize](uint32_t Index) {
      switch (ElementSize) {
        case 1:
          return Index >= 16;
        case 2:
          return Index >= 8;
        case 4:
          return Index >= 4;
        case 8:
          return Index >= 2;
        default:
          return false;
      }
    };

    const auto SrcIsUpper = IsUpperIdx(SrcIdx);
    const auto DstIsUpper = IsUpperIdx(DestIdx);

    // Sanitizes indices based on whether or not its accessing the upper lane.
    const auto SanitizeIndex = [ElementSize](uint32_t Index, bool IsUpper) -> uint32_t {
      switch (ElementSize) {
        case 1:
          if (IsUpper) {
            return Index - 16;
          }
          return Index;
        case 2:
          if (IsUpper) {
            return Index - 8;
          }
          return Index;
        case 4:
          if (IsUpper) {
            return Index - 4;
          }
          return Index;
        case 8:
          if (IsUpper) {
            return Index - 2;
          }
          return Index;
        default:
          return 0;
      }
    };

    // Helpers to get the source and destination vectors depending on
    // whether or not the top or bottom lane is accessed. Takes a temporary
    // to move the extracted lane into.
    const auto GetSrcVector = [this, SrcIsUpper, &SrcVector](const Xbyak::Xmm& Tmp) {
      if (SrcIsUpper) {
        vextracti128(Tmp, ToYMM(SrcVector), 1);
        return Tmp;
      } else {
        return SrcVector;
      }
    };
    // Always move the dst into a temporary, since regardless of the outcome
    // we need to insert it back into the original vector.
    const auto GetDstVector = [this, DstIsUpper, &DestVector](const Xbyak::Xmm& Tmp) {
      if (DstIsUpper) {
        vextracti128(Tmp, ToYMM(DestVector), 1);
        return Tmp;
      } else {
        vmovaps(Tmp, DestVector);
        return Tmp;
      }
    };

    const auto SrcReg = GetSrcVector(xmm14);
    const auto DstReg = GetDstVector(xmm15);

    const auto SanitizedDstIdx = SanitizeIndex(DestIdx, DstIsUpper);
    const auto SanitizedSrcIdx = SanitizeIndex(SrcIdx, SrcIsUpper);

    PerformInsertion(SrcReg, SanitizedSrcIdx, DstReg, SanitizedDstIdx);

    vmovapd(ToYMM(Dst), ToYMM(DestVector));
    if (DstIsUpper) {
      vinserti128(ToYMM(Dst), ToYMM(Dst), DstReg, 1);
    } else {
      vinserti128(ToYMM(Dst), ToYMM(Dst), DstReg, 0);
    }
  } else {
    vmovapd(xmm15, DestVector);
    PerformInsertion(SrcVector, SrcIdx, xmm15, DestIdx);
    vmovapd(Dst, xmm15);
  }
}

DEF_OP(VDupElement) {
  const auto Op = IROp->C<IR::IROp_VDupElement>();
  const auto OpSize = IROp->Size;

  const auto Index = Op->Index;
  const auto ElementSize = Op->Header.ElementSize;
  const auto ElementSizeBits = ElementSize * 8;
  const auto BitOffset = ElementSizeBits * Index;

  constexpr auto AVXRegSize = Core::CPUState::XMM_AVX_REG_SIZE;
  constexpr auto AVXBitSize = AVXRegSize * 8;
  constexpr auto SSERegSize = Core::CPUState::XMM_SSE_REG_SIZE;
  constexpr auto SSEBitSize = SSERegSize * 8;

  const auto Is256Bit = OpSize == AVXRegSize;
  const auto IsInUpperLane = BitOffset >= SSEBitSize;

  // Currently we only handle offsets within 256-bit registers at most.
  if (BitOffset >= AVXBitSize) {
    LOGMAN_MSG_A_FMT("Bit offset for element outside maximum range: Offset={}",
                     BitOffset);
    return;
  }

  // If we try to access the upper 128-bit lane on a 128-bit operation
  // then we have some invalid values being passed in.
  if (!Is256Bit && IsInUpperLane) {
    LOGMAN_MSG_A_FMT("Accessing upper 128-bit lane in 128-bit operation: "
                     "ElementSize={}, Index={}",
                     ElementSize, Index);
    return;
  }

  const auto Dst = GetDst(Node);
  const auto Vector = GetSrc(Op->Vector.ID());

  switch (ElementSize) {
    case 1: {
      if (Is256Bit && IsInUpperLane) {
        vextracti128(xmm15, ToYMM(Vector), 1);
      } else {
        vmovapd(xmm15, Vector);
      }

      // First extract the indexed byte and insert it
      // into the first element of the destination.
      pextrb(eax, xmm15, Index);
      pinsrb(Dst, eax, 0);

      // Now replicate.
      if (Is256Bit) {
        vpbroadcastb(ToYMM(Dst), Dst);
      } else {
        vpbroadcastb(Dst, Dst);
      }
      break;
    }
    case 2: {
      if (Is256Bit && IsInUpperLane) {
        vextracti128(xmm15, ToYMM(Vector), 1);
      } else {
        vmovapd(xmm15, Vector);
      }

      // First extract the indexed word and insert it
      // into the first element of the destination.
      pextrw(eax, xmm15, Index);
      pinsrw(Dst, eax, 0);

      if (Is256Bit) {
        vpbroadcastw(ToYMM(Dst), Dst);
      } else {
        vpbroadcastw(Dst, Dst);
      }
      break;
    }
    case 4: {
      if (IsInUpperLane) {
        vextracti128(xmm15, ToYMM(Vector), 1);
        vpshufd(Dst, xmm15,
                (Index << 0) |
                (Index << 2) |
                (Index << 4) |
                (Index << 6));
      } else {
        vpshufd(Dst, Vector,
                (Index << 0) |
                (Index << 2) |
                (Index << 4) |
                (Index << 6));
      }
      if (Is256Bit) {
        vinserti128(ToYMM(Dst), ToYMM(Dst), Dst, 1);
      }
      break;
    }
    case 8: {
      if (IsInUpperLane) {
        vextracti128(xmm15, ToYMM(Vector), 1);
        vshufpd(Dst, xmm15, xmm15,
                (Index << 0) |
                (Index << 1));
      } else {
        vshufpd(Dst, Vector, Vector,
                (Index << 0) |
                (Index << 1));
      }
      if (Is256Bit) {
        vinserti128(ToYMM(Dst), ToYMM(Dst), Dst, 1);
      }
      break;
    }
    case 16: {
      LOGMAN_THROW_AA_FMT(Is256Bit,
                        "Can't perform 128-bit broadcast with 128-bit operation: {}",
                        OpSize);

      vmovapd(ToYMM(Dst), ToYMM(Vector));
      if (IsInUpperLane) {
        vextracti128(xmm15, ToYMM(Dst), 1);
        vinserti128(ToYMM(Dst), ToYMM(Dst), xmm15, 0);
      } else {
        vinserti128(ToYMM(Dst), ToYMM(Dst), Dst, 1);
      }
      break;
    }
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
}



DEF_OP(VExtr) {
  const auto Op = IROp->C<IR::IROp_VExtr>();
  const auto OpSize = IROp->Size;

  constexpr auto AVXRegSize = Core::CPUState::XMM_AVX_REG_SIZE;
  const auto Is256Bit = OpSize == AVXRegSize;

  const auto Index = Op->Index;
  const auto ElementSize = Op->Header.ElementSize;

  const auto Dst = GetDst(Node);
  const auto VectorLower = GetSrc(Op->VectorLower.ID());
  const auto VectorUpper = GetSrc(Op->VectorUpper.ID());

  if (Is256Bit) {
    const auto ByteIndex = Index * ElementSize;
    const auto IsUpperVectorZero = ByteIndex >= OpSize;
    const auto SanitizedByteIndex = IsUpperVectorZero ? ByteIndex - OpSize
                                                      : ByteIndex;
    
    // Ensure we don't load junk outside of the space we're about to make.
    LOGMAN_THROW_AA_FMT(SanitizedByteIndex <= 31, "Invalid VExtr byte index: {}",
                        SanitizedByteIndex);

    const auto RegsSavedSize = AVXRegSize * 2;
    sub(rsp, RegsSavedSize);

    if (IsUpperVectorZero) {
      vpxor(xmm15, xmm15, xmm15);
      vmovups(ptr[rsp + 0 * AVXRegSize], ToYMM(VectorLower));
      vmovups(ptr[rsp + 1 * AVXRegSize], ymm15);
    } else {
      vmovups(ptr[rsp + 0 * AVXRegSize], ToYMM(VectorUpper));
      vmovups(ptr[rsp + 1 * AVXRegSize], ToYMM(VectorLower));
    }

    vmovups(ToYMM(Dst), ptr[rsp + SanitizedByteIndex]);

    add(rsp, RegsSavedSize);
  } else if (OpSize == 8) {
    // No way to do this with 64bit source without dropping to MMX
    // So emulate it
    vpxor(xmm14, xmm14, xmm14);
    movq(xmm15, VectorUpper);
    vshufpd(xmm15, xmm15, VectorLower, 0b00);
    vpalignr(Dst, xmm14, xmm15, Index);
  } else {
    vpalignr(Dst, VectorLower, VectorUpper, Index);
  }
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
  const auto Op = IROp->C<IR::IROp_VSShrI>();

  const auto BitShift = Op->BitShift;
  const auto ElementSize = Op->Header.ElementSize;

  const auto Dest = GetDst(Node);
  const auto Vector = GetSrc(Op->Vector.ID());

  switch (ElementSize) {
    case 1: {
      // This isn't a native instruction on x86
      const auto PerformShifts = [&](const Xbyak::Xmm& reg) {
        for (int i = 0; i < int(Core::CPUState::XMM_SSE_REG_SIZE); ++i) {
          pextrb(eax, reg, i);
          movsx(eax, al);
          sar(al, BitShift);
          pinsrb(reg, eax, i);
        }
      };

      vmovapd(ToYMM(Dest), ToYMM(Vector));
      PerformShifts(Dest);
      vextractf128(xmm15, ToYMM(Dest), 1);
      PerformShifts(xmm15);
      vinsertf128(ToYMM(Dest), ToYMM(Dest), xmm15, 1);
      break;
    }
    case 2: {
      vpsraw(ToYMM(Dest), ToYMM(Vector), BitShift);
      break;
    }
    case 4: {
      vpsrad(ToYMM(Dest), ToYMM(Vector), BitShift);
      break;
    }
    case 8: {
      // VPSRAQ is only introduced in AVX-512, so we can't use it.
      const auto PerformShifts = [&](const Xbyak::Xmm& reg) {
        for (int i = 0; i < 2; ++i) {
          pextrq(rax, reg, i);
          sar(rax, BitShift);
          pinsrq(reg, rax, i);
        }
      };

      vmovapd(ToYMM(Dest), ToYMM(Vector));
      PerformShifts(Dest);
      vextractf128(xmm15, ToYMM(Dest), 1);
      PerformShifts(xmm15);
      vinsertf128(ToYMM(Dest), ToYMM(Dest), xmm15, 1);
      break;
    }
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
}

DEF_OP(VShlI) {
  const auto Op = IROp->C<IR::IROp_VShlI>();

  const auto BitShift = Op->BitShift;
  const auto ElementSize = Op->Header.ElementSize;

  const auto Dst = ToYMM(GetDst(Node));
  const auto Vector = ToYMM(GetSrc(Op->Vector.ID()));

  switch (ElementSize) {
    case 2: {
      vpsllw(Dst, Vector, BitShift);
      break;
    }
    case 4: {
      vpslld(Dst, Vector, BitShift);
      break;
    }
    case 8: {
      vpsllq(Dst, Vector, BitShift);
      break;
    }
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
}

DEF_OP(VUShrNI) {
  const auto Op = IROp->C<IR::IROp_VUShrNI>();
  const auto OpSize = IROp->Size;

  const auto BitShift = Op->BitShift;
  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetDst(Node);
  const auto DstYMM = ToYMM(Dst);
  const auto Vector = GetSrc(Op->Vector.ID());

  vmovapd(DstYMM, ToYMM(Vector));
  vpxor(xmm15, xmm15, xmm15);

  switch (ElementSize) {
    case 1: {
      if (Is256Bit) {
        vpsrlw(DstYMM, DstYMM, BitShift);
      } else {
        vpsrlw(Dst, Dst, BitShift);
      }
      // <8 x i16> -> <8 x i8>
      mov(rax, 0x0E'0C'0A'08'06'04'02'00); // Lower
      mov(rcx, 0x80'80'80'80'80'80'80'80); // Upper
      break;
    }
    case 2: {
      if (Is256Bit) {
        vpsrld(DstYMM, DstYMM, BitShift);
      } else {
        vpsrld(Dst, Dst, BitShift);
      }
      // <4 x i32> -> <4 x i16>
      mov(rax, 0x0D'0C'09'08'05'04'01'00); // Lower
      mov(rcx, 0x80'80'80'80'80'80'80'80); // Upper
      break;
    }
    case 4: {
      if (Is256Bit) {
        vpsrlq(DstYMM, DstYMM, BitShift);
      } else {
        vpsrlq(Dst, Dst, BitShift);
      }
      // <2 x i64> -> <2 x i32>
      mov(rax, 0x0B'0A'09'08'03'02'01'00); // Lower
      mov(rcx, 0x80'80'80'80'80'80'80'80); // Upper
      break;
    }
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      return;
  }

  vmovq(xmm15, rax);
  vmovq(xmm14, rcx);
  punpcklqdq(xmm15, xmm14);
  if (Is256Bit) {
    vinserti128(ymm15, ymm15, xmm15, 1);
    vpshufb(DstYMM, DstYMM, ymm15);
    vextracti128(xmm14, DstYMM, 1);
    punpcklqdq(Dst, xmm14);
  } else {
    vpshufb(Dst, Dst, xmm15);
  }
}

DEF_OP(VUShrNI2) {
  // Src1 = Lower results
  // Src2 = Upper Results
  const auto Op = IROp->C<IR::IROp_VUShrNI2>();
  const auto OpSize = IROp->Size;

  const auto BitShift = Op->BitShift;
  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetDst(Node);
  const auto VectorLower = GetSrc(Op->VectorLower.ID());
  const auto VectorUpper = GetSrc(Op->VectorUpper.ID());

  vmovapd(ymm13, ToYMM(VectorUpper));
  switch (ElementSize) {
    case 1: {
      if (Is256Bit) {
        vpsrlw(ymm13, ymm13, BitShift);
      } else {
        vpsrlw(xmm13, xmm13, BitShift);
      }
      // <8 x i16> -> <8 x i8>
      mov(rax, 0x80'80'80'80'80'80'80'80); // Lower
      mov(rcx, 0x0E'0C'0A'08'06'04'02'00); // Upper
      break;
    }
    case 2: {
      if (Is256Bit) {
        vpsrld(ymm13, ymm13, BitShift);
      } else {
        vpsrld(xmm13, xmm13, BitShift);
      }
      // <4 x i32> -> <4 x i16>
      mov(rax, 0x80'80'80'80'80'80'80'80); // Lower
      mov(rcx, 0x0D'0C'09'08'05'04'01'00); // Upper
      break;
    }
    case 4: {
      if (Is256Bit) {
        vpsrlq(ymm13, ymm13, BitShift);
      } else {
        vpsrlq(xmm13, xmm13, BitShift);
      }
      // <2 x i64> -> <2 x i32>
      mov(rax, 0x80'80'80'80'80'80'80'80); // Lower
      mov(rcx, 0x0B'0A'09'08'03'02'01'00); // Upper
      break;
    }
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      return;
  }

  vmovq(xmm15, rax);
  vmovq(xmm14, rcx);
  punpcklqdq(xmm15, xmm14);
  if (Is256Bit) {
    vinserti128(ymm15, ymm15, xmm15, 1);
    vpshufb(ymm14, ymm13, ymm15);
    vextracti128(xmm12, ymm14, 1);
    punpckhqdq(xmm14, xmm12);

    vmovapd(Dst, VectorLower);
    vinserti128(ToYMM(Dst), ToYMM(Dst), xmm14, 1);
  } else {
    vpshufb(xmm14, xmm13, xmm15);
    vpor(Dst, xmm14, VectorLower);
  }
}

DEF_OP(VSXTL) {
  const auto Op = IROp->C<IR::IROp_VSXTL>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetDst(Node);
  const auto Vector = GetSrc(Op->Vector.ID());

  switch (ElementSize) {
    case 2:
      if (Is256Bit) {
        vpmovsxbw(ToYMM(Dst), Vector);
      } else {
        vpmovsxbw(Dst, Vector);
      }
      break;
    case 4:
      if (Is256Bit) {
        vpmovsxwd(ToYMM(Dst), Vector);
      } else {
        vpmovsxwd(Dst, Vector);
      }
      break;
    case 8:
      if (Is256Bit) {
        vpmovsxdq(ToYMM(Dst), Vector);
      } else {
        vpmovsxdq(Dst, Vector);
      }
      break;
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
}

DEF_OP(VSXTL2) {
  const auto Op = IROp->C<IR::IROp_VSXTL2>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetDst(Node);
  const auto Vector = GetSrc(Op->Vector.ID());

  if (Is256Bit) {
    const auto DstYMM = ToYMM(Dst);

    vextracti128(Dst, ToYMM(Vector), 1);
    switch (ElementSize) {
      case 2:
        vpmovsxbw(DstYMM, Dst);
        break;
      case 4:
        vpmovsxwd(DstYMM, Dst);
        break;
      case 8:
        vpmovsxdq(DstYMM, Dst);
        break;
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        break;
    }
  } else {
    vpsrldq(Dst, Vector, OpSize / 2);
    switch (ElementSize) {
      case 2:
        vpmovsxbw(Dst, Dst);
        break;
      case 4:
        vpmovsxwd(Dst, Dst);
        break;
      case 8:
        vpmovsxdq(Dst, Dst);
        break;
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        break;
    }
  }
}

DEF_OP(VUXTL) {
  const auto Op = IROp->C<IR::IROp_VUXTL>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetDst(Node);
  const auto Vector = GetSrc(Op->Vector.ID());

  switch (ElementSize) {
    case 2:
      if (Is256Bit) {
        vpmovzxbw(ToYMM(Dst), Vector);
      } else {
        vpmovzxbw(Dst, Vector);
      }
      break;
    case 4:
      if (Is256Bit) {
        vpmovzxwd(ToYMM(Dst), Vector);
      } else {
        vpmovzxwd(Dst, Vector);
      }
      break;
    case 8:
      if (Is256Bit) {
        vpmovzxdq(ToYMM(Dst), Vector);
      } else {
        vpmovzxdq(Dst, Vector);
      }
      break;
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
}

DEF_OP(VUXTL2) {
  const auto Op = IROp->C<IR::IROp_VUXTL2>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetDst(Node);
  const auto Vector = GetSrc(Op->Vector.ID());

  if (Is256Bit) {
    const auto DstYMM = ToYMM(Dst);

    vextracti128(Dst, ToYMM(Vector), 1);
    switch (ElementSize) {
      case 2:
        vpmovzxbw(DstYMM, Dst);
        break;
      case 4:
        vpmovzxwd(DstYMM, Dst);
        break;
      case 8:
        vpmovzxdq(DstYMM, Dst);
        break;
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        break;
    }
  } else {
    vpsrldq(Dst, Vector, OpSize / 2);
    switch (ElementSize) {
      case 2:
        vpmovzxbw(Dst, Dst);
        break;
      case 4:
        vpmovzxwd(Dst, Dst);
        break;
      case 8:
        vpmovzxdq(Dst, Dst);
        break;
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        break;
    }
  }
}

DEF_OP(VSQXTN) {
  const auto Op = IROp->C<IR::IROp_VSQXTN>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetDst(Node);
  const auto Vector = GetSrc(Op->Vector.ID());

  if (Is256Bit) {
    switch (ElementSize) {
      case 1:
        vpacksswb(ToYMM(Dst), ToYMM(Vector), ToYMM(Vector));
        break;
      case 2:
        vpackssdw(ToYMM(Dst), ToYMM(Vector), ToYMM(Vector));
        break;
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        return;
    }

    vextracti128(xmm15, ToYMM(Dst), 1);
    pslldq(xmm15, 8);
    vmovq(Dst, Dst);
    vpor(Dst, Dst, xmm15);
  } else {
    switch (ElementSize) {
      case 1:
        vpacksswb(Dst, Vector, Vector);
        break;
      case 2:
        vpackssdw(Dst, Vector, Vector);
        break;
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        return;
    }

    // Clear upper 64-bits.
    movq(Dst, Dst);
  }
}

DEF_OP(VSQXTN2) {
  const auto Op = IROp->C<IR::IROp_VSQXTN2>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetDst(Node);
  const auto VectorLower = GetSrc(Op->VectorLower.ID());
  const auto VectorUpper = GetSrc(Op->VectorUpper.ID());

  // Zero the lower bits
  vpxor(xmm15, xmm15, xmm15);

  if (Is256Bit) {
    switch (ElementSize) {
      case 1:
        vpacksswb(ymm15, ToYMM(VectorUpper), ToYMM(VectorUpper));
        break;
      case 2:
        vpackssdw(ymm15, ToYMM(VectorUpper), ToYMM(VectorUpper));
        break;
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        return;
    }

    vextracti128(xmm14, ymm15, 1);
    pslldq(xmm14, 8);
    vmovq(xmm15, xmm15);
    vpor(xmm15, xmm15, xmm14);

    vmovaps(Dst, VectorLower);
    vinserti128(ToYMM(Dst), ToYMM(Dst), xmm15, 1);
  } else {
    switch (ElementSize) {
      case 1:
        vpacksswb(xmm15, xmm15, VectorUpper);
        break;
      case 2:
        vpackssdw(xmm15, xmm15, VectorUpper);
        break;
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        return;
    }

    if (OpSize == 8) {
      psrldq(xmm15, OpSize / 2);
    }
    vpor(Dst, VectorLower, xmm15);
  }
}

DEF_OP(VSQXTUN) {
  const auto Op = IROp->C<IR::IROp_VSQXTUN>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetDst(Node);
  const auto Vector = GetSrc(Op->Vector.ID());

  if (Is256Bit) {
    switch (ElementSize) {
      case 1:
        vpackuswb(ToYMM(Dst), ToYMM(Vector), ToYMM(Vector));
        break;
      case 2:
        vpackusdw(ToYMM(Dst), ToYMM(Vector), ToYMM(Vector));
        break;
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        return;
    }

    vextracti128(xmm15, ToYMM(Dst), 1);
    pslldq(xmm15, 8);
    vmovq(Dst, Dst);
    vpor(Dst, Dst, xmm15);
  } else {
    switch (ElementSize) {
      case 1:
        vpackuswb(Dst, Vector, Vector);
        break;
      case 2:
        vpackusdw(Dst, Vector, Vector);
        break;
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        return;
    }
    // Clear upper 64 bits.
    movq(Dst, Dst);
  }
}

DEF_OP(VSQXTUN2) {
  const auto Op = IROp->C<IR::IROp_VSQXTUN2>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetDst(Node);
  const auto VectorLower = GetSrc(Op->VectorLower.ID());
  const auto VectorUpper = GetSrc(Op->VectorUpper.ID());

  // Zero the lower bits
  vpxor(xmm15, xmm15, xmm15);

  if (Is256Bit) {
    switch (ElementSize) {
      case 1:
        vpackuswb(ymm15, ToYMM(VectorUpper), ToYMM(VectorUpper));
        break;
      case 2:
        vpackusdw(ymm15, ToYMM(VectorUpper), ToYMM(VectorUpper));
        break;
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        return;
    }

    vextracti128(xmm14, ymm15, 1);
    pslldq(xmm14, 8);
    vmovq(xmm15, xmm15);
    vpor(xmm15, xmm15, xmm14);

    vmovaps(Dst, VectorLower);
    vinserti128(ToYMM(Dst), ToYMM(Dst), xmm15, 1);
  } else {
    switch (ElementSize) {
      case 1:
        vpackuswb(xmm15, xmm15, VectorUpper);
        break;
      case 2:
        vpackusdw(xmm15, xmm15, VectorUpper);
        break;
      default:
        LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
        return;
    }
    if (OpSize == 8) {
      psrldq(xmm15, OpSize / 2);
    }

    vpor(Dst, VectorLower, xmm15);
  }
}

DEF_OP(VMul) {
  const auto Op = IROp->C<IR::IROp_VUMul>();

  const auto ElementSize = Op->Header.ElementSize;

  const auto Dst = ToYMM(GetDst(Node));
  const auto Vector1 = ToYMM(GetSrc(Op->Vector1.ID()));
  const auto Vector2 = ToYMM(GetSrc(Op->Vector2.ID()));

  switch (ElementSize) {
    case 2: {
      vpmullw(Dst, Vector1, Vector2);
      break;
    }
    case 4: {
      vpmulld(Dst, Vector1, Vector2);
      break;
    }
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
}

DEF_OP(VUMull) {
  const auto Op = IROp->C<IR::IROp_VUMull>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetDst(Node);
  const auto Vector1 = GetSrc(Op->Vector1.ID());
  const auto Vector2 = GetSrc(Op->Vector2.ID());

  switch (ElementSize) {
    case 4: {
      // IR operation (128-bit):
      // [31:00 ] = src1[15:00] * src2[15:00]
      // [63:32 ] = src1[31:16] * src2[31:16]
      // [95:64 ] = src1[47:32] * src2[47:32]
      // [127:96] = src1[63:48] * src2[63:48]
      //
      vpxor(xmm15, xmm15, xmm15);
      vpxor(xmm14, xmm14, xmm14);

      if (Is256Bit) {
        vpxor(xmm13, xmm13, xmm13);
        vpxor(xmm12, xmm12, xmm12);

        vpunpckhwd(xmm13, Vector1, xmm13);
        vpunpckhwd(xmm12, Vector2, xmm12);
      }

      vpunpcklwd(xmm15, Vector1, xmm15);
      vpunpcklwd(xmm14, Vector2, xmm14);

      if (Is256Bit) {
        vinserti128(ymm15, ymm15, xmm13, 1);
        vinserti128(ymm14, ymm14, xmm12, 1);
        vpmulld(ToYMM(Dst), ymm14, ymm15);
      } else {
        vpmulld(Dst, xmm14, xmm15);
      }
      break;
    }
    case 8: {
      // We need to shuffle the data for this one
      // x86 PMULUDQ wants the 32bit values in [31:0] and [95:64]
      // Which then extends out to [63:0] and [127:64]
      vpshufd(xmm14, Vector1, 0b01'01'00'00);
      vpshufd(xmm15, Vector2, 0b01'01'00'00);

      if (Is256Bit) {
        vpshufd(xmm13, Vector1, 0b11'11'10'10);
        vpshufd(xmm12, Vector2, 0b11'11'10'10);

        vinserti128(ymm14, ymm14, xmm13, 1);
        vinserti128(ymm15, ymm15, xmm12, 1);

        vpmuludq(ToYMM(Dst), ymm14, ymm15);
      } else {
        vpmuludq(Dst, xmm14, xmm15);
      }
      break;
    }
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
}

DEF_OP(VSMull) {
  const auto Op = IROp->C<IR::IROp_VSMull>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetDst(Node);
  const auto Vector1 = GetSrc(Op->Vector1.ID());
  const auto Vector2 = GetSrc(Op->Vector2.ID());

  switch (ElementSize) {
    case 4: {
      // IR operation (128-bit):
      // [31:00 ] = src1[15:00] * src2[15:00]
      // [63:32 ] = src1[31:16] * src2[31:16]
      // [95:64 ] = src1[47:32] * src2[47:32]
      // [127:96] = src1[63:48] * src2[63:48]
      //
      vpxor(xmm15, xmm15, xmm15);
      vpxor(xmm14, xmm14, xmm14);

      if (Is256Bit) {
        vpxor(xmm13, xmm13, xmm13);
        vpxor(xmm12, xmm12, xmm12);

        vpunpckhwd(xmm13, Vector1, xmm13);
        vpunpckhwd(xmm12, Vector2, xmm12);
      }

      vpunpcklwd(xmm15, Vector1, xmm15);
      vpunpcklwd(xmm14, Vector2, xmm14);

      if (Is256Bit) {
        const auto SignExtend = [this](const Xbyak::Ymm& reg) {
          vpslld(reg, reg, 16);
          vpsrad(reg, reg, 16);
        };

        vinserti128(ymm15, ymm15, xmm13, 1);
        vinserti128(ymm14, ymm14, xmm12, 1);

        SignExtend(ymm15);
        SignExtend(ymm14);

        vpmulld(ToYMM(Dst), ymm14, ymm15);
      } else {
        const auto SignExtend = [this](const Xbyak::Xmm& reg) {
          pslld(reg, 16);
          psrad(reg, 16);
        };

        SignExtend(xmm15);
        SignExtend(xmm14);

        vpmulld(Dst, xmm14, xmm15);
      }
      break;
    }
    case 8: {
      // We need to shuffle the data for this one
      // x86 PMULDQ wants the 32bit values in [31:0] and [95:64]
      // Which then extends out to [63:0] and [127:64]
      vpshufd(xmm14, Vector1, 0b01'01'00'00);
      vpshufd(xmm15, Vector2, 0b01'01'00'00);

      if (Is256Bit) {
        vpshufd(xmm13, Vector1, 0b11'11'10'10);
        vpshufd(xmm12, Vector2, 0b11'11'10'10);

        vinserti128(ymm14, ymm14, xmm13, 1);
        vinserti128(ymm15, ymm15, xmm12, 1);

        vpmuldq(ToYMM(Dst), ymm14, ymm15);
      } else {
        vpmuldq(Dst, xmm14, xmm15);
      }
      break;
    }
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
}

DEF_OP(VUMull2) {
  const auto Op = IROp->C<IR::IROp_VUMull2>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetDst(Node);
  const auto Vector1 = GetSrc(Op->Vector1.ID());
  const auto Vector2 = GetSrc(Op->Vector2.ID());

  switch (ElementSize) {
    case 4: {
      // IR operation (128-bit):
      // [31:00 ] = src1[79:64  ] * src2[79:64  ]
      // [63:32 ] = src1[95:80  ] * src2[95:80  ]
      // [95:64 ] = src1[111:96 ] * src2[111:96 ]
      // [127:96] = src1[127:112] * src2[127:112]
      //
      vpxor(xmm15, xmm15, xmm15);
      vpxor(xmm14, xmm14, xmm14);

      if (Is256Bit) {
        vextracti128(xmm15, ToYMM(Vector1), 1);
        vextracti128(xmm14, ToYMM(Vector2), 1);

        vpxor(xmm12, xmm12, xmm12);
        vpxor(xmm13, xmm13, xmm13);
        vpxor(Dst, Dst, Dst);

        // Bottom half
        vpunpcklwd(xmm13, xmm15, xmm13);
        vpunpcklwd(xmm12, xmm14, xmm12);

        // Top half
        vpunpckhwd(xmm15, xmm15, Dst);
        vpunpckhwd(xmm14, xmm14, Dst);

        // Reinsert
        vinserti128(ymm13, ymm13, xmm15, 1);
        vinserti128(ymm12, ymm12, xmm14, 1);

        vpmulld(ToYMM(Dst), ymm12, ymm13);
      } else {
        vpunpckhwd(xmm15, Vector1, xmm15);
        vpunpckhwd(xmm14, Vector2, xmm14);
        vpmulld(Dst, xmm14, xmm15);
      }
      break;
    }
    case 8: {
      // IR operation (128-bit):
      // [63:00 ] = src1[95:64 ] * src2[95:64 ]
      // [127:64] = src1[127:96] * src2[127:96]
      //
      // x86 vpmuludq
      // [63:00 ] = src1[31:0 ] * src2[31:0 ]
      // [127:64] = src1[95:64] * src2[95:64]

      if (Is256Bit) {
        vextracti128(xmm14, ToYMM(Vector1), 1);
        vextracti128(xmm15, ToYMM(Vector2), 1);

        vpshufd(xmm12, xmm14, 0b01'01'00'00);
        vpshufd(xmm13, xmm15, 0b01'01'00'00);

        vpshufd(xmm14, xmm14, 0b11'11'10'10);
        vpshufd(xmm15, xmm15, 0b11'11'10'10);

        vinserti128(ymm12, ymm12, xmm14, 1);
        vinserti128(ymm13, ymm13, xmm15, 1);

        vpmuludq(ToYMM(Dst), ymm12, ymm13);
      } else {
        vpshufd(xmm14, Vector1, 0b11'11'10'10);
        vpshufd(xmm15, Vector2, 0b11'11'10'10);

        vpmuludq(Dst, xmm14, xmm15);
      }
      break;
    }
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
}

DEF_OP(VSMull2) {
  const auto Op = IROp->C<IR::IROp_VSMull2>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;
  const auto Is256Bit = OpSize == Core::CPUState::XMM_AVX_REG_SIZE;

  const auto Dst = GetDst(Node);
  const auto Vector1 = GetSrc(Op->Vector1.ID());
  const auto Vector2 = GetSrc(Op->Vector2.ID());

  switch (ElementSize) {
    case 4: {
      // IR operation (128-bit):
      // [31:00 ] = src1[79:64  ] * src2[79:64  ]
      // [63:32 ] = src1[95:80  ] * src2[95:80  ]
      // [95:64 ] = src1[111:96 ] * src2[111:96 ]
      // [127:96] = src1[127:112] * src2[127:112]
      //
      vpxor(xmm15, xmm15, xmm15);
      vpxor(xmm14, xmm14, xmm14);

      if (Is256Bit) {
        const auto SignExtend = [this](const Xbyak::Ymm& reg) {
          vpslld(reg, reg, 16);
          vpsrad(reg, reg, 16);
        };

        vextracti128(xmm15, ToYMM(Vector1), 1);
        vextracti128(xmm14, ToYMM(Vector2), 1);

        vpxor(xmm12, xmm12, xmm12);
        vpxor(xmm13, xmm13, xmm13);
        vpxor(Dst, Dst, Dst);

        // Bottom half
        vpunpcklwd(xmm13, xmm15, xmm13);
        vpunpcklwd(xmm12, xmm14, xmm12);

        // Top half
        vpunpckhwd(xmm15, xmm15, Dst);
        vpunpckhwd(xmm14, xmm14, Dst);

        // Reinsert
        vinserti128(ymm13, ymm13, xmm15, 1);
        vinserti128(ymm12, ymm12, xmm14, 1);

        SignExtend(ymm13);
        SignExtend(ymm12);

        vpmulld(ToYMM(Dst), ymm12, ymm13);
      } else {
        const auto SignExtend = [this](const Xbyak::Xmm& reg) {
          pslld(reg, 16);
          psrad(reg, 16);
        };

        vpunpckhwd(xmm15, Vector1, xmm15);
        vpunpckhwd(xmm14, Vector2, xmm14);

        SignExtend(xmm15);
        SignExtend(xmm14);

        vpmulld(Dst, xmm14, xmm15);
      }
      break;
    }
    case 8: {
      // IR operation (128-bit):
      // [63:00 ] = src1[95:64 ] * src2[95:64 ]
      // [127:64] = src1[127:96] * src2[127:96]
      //
      // x86 vpmuludq
      // [63:00 ] = src1[31:0 ] * src2[31:0 ]
      // [127:64] = src1[95:64] * src2[95:64]

      if (Is256Bit) {
        vextracti128(xmm14, ToYMM(Vector1), 1);
        vextracti128(xmm15, ToYMM(Vector2), 1);

        vpshufd(xmm12, xmm14, 0b01'01'00'00);
        vpshufd(xmm13, xmm15, 0b01'01'00'00);

        vpshufd(xmm14, xmm14, 0b11'11'10'10);
        vpshufd(xmm15, xmm15, 0b11'11'10'10);

        vinserti128(ymm12, ymm12, xmm14, 1);
        vinserti128(ymm13, ymm13, xmm15, 1);

        vpmuldq(ToYMM(Dst), ymm12, ymm13);
      } else {
        vpshufd(xmm14, Vector1, 0b11'11'10'10);
        vpshufd(xmm15, Vector2, 0b11'11'10'10);

        vpmuldq(Dst, xmm14, xmm15);
      }
      break;
    }
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
}

DEF_OP(VUABDL) {
  const auto Op = IROp->C<IR::IROp_VUABDL>();

  const auto ElementSize = Op->Header.ElementSize;

  const auto Dst = ToYMM(GetDst(Node));
  const auto Vector1 = GetSrc(Op->Vector1.ID());
  const auto Vector2 = GetSrc(Op->Vector2.ID());

  switch (ElementSize) {
    case 2: {
      vpmovzxbw(ymm14, Vector1);
      vpmovzxbw(ymm15, Vector2);
      vpsubw(Dst, ymm14, ymm15);
      vpabsw(Dst, Dst);
      break;
    }
    case 4: {
      vpmovzxwd(ymm14, Vector1);
      vpmovzxwd(ymm15, Vector2);
      vpsubd(Dst, ymm14, ymm15);
      vpabsd(Dst, Dst);
      break;
    }
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
  }
}

DEF_OP(VTBL1) {
  const auto Op = IROp->C<IR::IROp_VTBL1>();
  const auto OpSize = IROp->Size;

  const auto Dst = GetDst(Node);
  const auto VectorIndices = GetSrc(Op->VectorIndices.ID());
  const auto VectorTable = GetSrc(Op->VectorTable.ID());

  switch (OpSize) {
    case 8: {
      vpshufb(Dst, VectorTable, VectorIndices);
      vmovq(Dst, Dst);
      break;
    }
    case 16: {
      vpshufb(Dst, VectorTable, VectorIndices);
      break;
    }
    case 32: {
      // AVX2 vpshufb is a coward and doesn't lane cross, so we
      // need to get a little creative.
      const auto DstYMM = ToYMM(Dst);
      const auto VectorIndicesYMM = ToYMM(VectorIndices);
      const auto VectorTableYMM = ToYMM(VectorTable);

      // Permute the bottom lane of the table into a register
      // and do the same for the upper lane.
      vperm2i128(ymm15, VectorTableYMM, VectorTableYMM, 0b0000'0000);
      vperm2i128(ymm14, VectorTableYMM, VectorTableYMM, 0b0001'0001);

      // Shuffle away
      vpshufb(ymm15, ymm15, VectorIndicesYMM);
      vpshufb(ymm14, ymm14, VectorIndicesYMM);

      // Set up our comparison register
      mov(eax, 16);
      vmovd(xmm13, eax);
      vpbroadcastb(ymm13, xmm13);

      // Now create our control mask
      vpcmpgtb(ymm12, ymm13, VectorIndicesYMM);

      // And finally blend everything together
      // from the high and low halves depending
      // on the control mask.
      vpblendvb(DstYMM, ymm14, ymm15, ymm12);
      break;
    }
    default:
      LOGMAN_MSG_A_FMT("Unknown OpSize: {}", OpSize);
      break;
  }
}

DEF_OP(VRev64) {
  const auto Op = IROp->C<IR::IROp_VRev64>();
  const auto OpSize = IROp->Size;

  const auto ElementSize = Op->Header.ElementSize;

  const auto Dst = GetDst(Node);
  const auto Vector = GetSrc(Op->Vector.ID());

  switch (ElementSize) {
    case 1: {
      mov(rax, 0x00'01'02'03'04'05'06'07); // Lower
      vmovq(xmm15, rax);
      if (OpSize >= Core::CPUState::XMM_SSE_REG_SIZE) {
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

      if (OpSize == Core::CPUState::XMM_AVX_REG_SIZE) {
        // Replicate to the upper lane
        vinsertf128(ymm15, ymm15, xmm15, 1);
        vpshufb(ToYMM(Dst), ToYMM(Vector), ymm15);
      } else {
        vpshufb(Dst, Vector, xmm15);
      }
      break;
    }
    case 2: {
      // Full 16-bit byteswap in each 64-bit element
      mov(rax, 0x01'00'03'02'05'04'07'06); // Lower
      vmovq(xmm15, rax);
      if (OpSize >= Core::CPUState::XMM_SSE_REG_SIZE) {
        mov(rcx, 0x09'08'0B'0A'0D'0C'0F'0E); // Upper
        pinsrq(xmm15, rcx, 1);
      }
      else {
        // 8byte, upper bits get zero
        // Full 8bit byteswap in each 64-bit element
        mov(rcx, 0x80'80'80'80'80'80'80'80); // Upper
        pinsrq(xmm15, rcx, 1);
      }

      if (OpSize == Core::CPUState::XMM_AVX_REG_SIZE) {
        // Replicate to the upper lane
        vinsertf128(ymm15, ymm15, xmm15, 1);
        vpshufb(ToYMM(Dst), ToYMM(Vector), ymm15);
      } else {
        vpshufb(Dst, Vector, xmm15);
      }
      break;
    }
    case 4: {
      if (OpSize == Core::CPUState::XMM_AVX_REG_SIZE) {
        // Lower
        mov(rax, 0x03'02'01'00'07'06'05'04);
        vmovq(xmm15, rax);

        // Upper
        mov(rax, 0x0B'0A'09'08'0F'0E'0D'0C);
        pinsrq(xmm15, rax, 1);

        // Replicate to upper lane
        vinsertf128(ymm15, ymm15, xmm15, 1);

        // And finish it off.
        vpshufb(ToYMM(Dst), ToYMM(Vector), ymm15);
      } else if (OpSize == Core::CPUState::XMM_SSE_REG_SIZE) {
        vpshufd(Dst,
          Vector,
          (0b11 << 0) |
          (0b10 << 2) |
          (0b01 << 4) |
          (0b00 << 6));
      } else {
        vpshufd(Dst,
          Vector,
          (0b01 << 0) |
          (0b00 << 2) |
          (0b11 << 4) | // Last two don't matter, will be overwritten with zero
          (0b11 << 6));

        // Zero upper 64-bits
        mov(rcx, 0);
        pinsrq(Dst, rcx, 1);
      }
      break;
    }
    default:
      LOGMAN_MSG_A_FMT("Unknown Element Size: {}", ElementSize);
      break;
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

