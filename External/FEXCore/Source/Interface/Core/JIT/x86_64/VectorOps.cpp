#include "Interface/Core/JIT/x86_64/JITClass.h"
#include "Interface/IR/Passes/RegisterAllocationPass.h"


namespace FEXCore::CPU {

#define DEF_OP(x) void JITCore::Op_##x(FEXCore::IR::IROp_Header *IROp, uint32_t Node)
DEF_OP(VectorZero) {
  auto Dst = GetDst(Node);
  vpxor(Dst, Dst, Dst);
}

DEF_OP(VectorImm) {
  auto Op = IROp->C<IR::IROp_VectorImm>();
  uint8_t OpSize = IROp->Size;

  auto Dst = GetDst(Node);
  uint64_t Imm = Op->Immediate;

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
  if (OpSize == 16) {
    // Duplicate to the upper 64bits if we are 128bits
    movddup(Dst, Dst);
  }
}

DEF_OP(CreateVector2) {
  LogMan::Msg::A("Unimplemented");
}

DEF_OP(CreateVector4) {
  LogMan::Msg::A("Unimplemented");
}

DEF_OP(SplatVector) {
  auto Op = IROp->C<IR::IROp_SplatVector2>();
  uint8_t OpSize = IROp->Size;

  LogMan::Throw::A(OpSize <= 16, "Can't handle a vector of size: %d", OpSize);
  uint8_t Elements = 0;

  switch (Op->Header.Op) {
    case IR::OP_SPLATVECTOR4: Elements = 4; break;
    case IR::OP_SPLATVECTOR2: Elements = 2; break;
    default: LogMan::Msg::A("Uknown Splat size"); break;
  }

  uint8_t ElementSize = OpSize / Elements;

  switch (ElementSize) {
    case 4:
      movapd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));
      shufps(GetDst(Node), GetDst(Node), 0);
    break;
    case 8:
      movddup(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));
    break;
    default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.Size); break;
  }
}

DEF_OP(VMov) {
  auto Op = IROp->C<IR::IROp_VMov>();
  uint8_t OpSize = IROp->Size;

  switch (OpSize) {
    case 1: {
      vpxor(xmm15, xmm15, xmm15);
      pextrb(eax, GetSrc(Op->Header.Args[0].ID()), 0);
      pinsrb(xmm15, eax, 0);
      movapd(GetDst(Node), xmm15);
      break;
    }
    case 2: {
      vpxor(xmm15, xmm15, xmm15);
      pextrw(eax, GetSrc(Op->Header.Args[0].ID()), 0);
      pinsrw(xmm15, eax, 0);
      movapd(GetDst(Node), xmm15);
      break;
    }
    case 4: {
      vpxor(xmm15, xmm15, xmm15);
      pextrd(eax, GetSrc(Op->Header.Args[0].ID()), 0);
      pinsrd(xmm15, eax, 0);
      movapd(GetDst(Node), xmm15);
      break;
    }
    case 8: {
      movq(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));
      break;
    }
    case 16: {
      movaps(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));
      break;
    }
    default: LogMan::Msg::A("Unknown Element Size: %d", OpSize); break;
  }
}

DEF_OP(VAnd) {
  auto Op = IROp->C<IR::IROp_VAnd>();
  vpand(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
}

DEF_OP(VOr) {
  auto Op = IROp->C<IR::IROp_VOr>();
  vpor(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
}

DEF_OP(VXor) {
  auto Op = IROp->C<IR::IROp_VXor>();
  vpxor(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
}

DEF_OP(VAdd) {
  auto Op = IROp->C<IR::IROp_VAdd>();
  switch (Op->Header.ElementSize) {
    case 1: {
      vpaddb(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
    }
    case 2: {
      vpaddw(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
    }
    case 4: {
      vpaddd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
    }
    case 8: {
      vpaddq(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
    }
    default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
  }
}

DEF_OP(VSub) {
  auto Op = IROp->C<IR::IROp_VSub>();
  switch (Op->Header.ElementSize) {
    case 1: {
      vpsubb(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
    }
    case 2: {
      vpsubw(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
    }
    case 4: {
      vpsubd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
    }
    case 8: {
      vpsubq(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
    }
    default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
  }
}

DEF_OP(VUQAdd) {
  auto Op = IROp->C<IR::IROp_VUQAdd>();
  switch (Op->Header.ElementSize) {
    case 1: {
      vpaddusb(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
    }
    case 2: {
      vpaddusw(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
    }
    default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
  }
}

DEF_OP(VUQSub) {
  auto Op = IROp->C<IR::IROp_VUQSub>();
  switch (Op->Header.ElementSize) {
    case 1: {
      vpsubusb(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
    }
    case 2: {
      vpsubusw(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
    }
    default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
  }
}

DEF_OP(VSQAdd) {
  auto Op = IROp->C<IR::IROp_VSQAdd>();
  switch (Op->Header.ElementSize) {
    case 1: {
      vpaddsb(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
    }
    case 2: {
      vpaddsw(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
    }
    default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
  }
}

DEF_OP(VSQSub) {
  auto Op = IROp->C<IR::IROp_VSQSub>();
  switch (Op->Header.ElementSize) {
    case 1: {
      vpsubsb(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
    }
    case 2: {
      vpsubsw(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
    }
    default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
  }
}

DEF_OP(VAddP) {
  auto Op = IROp->C<IR::IROp_VAddP>();
  uint8_t OpSize = IROp->Size;

  if (OpSize == 8) {
    // Can't handle this natively without dropping to MMX
    // Emulate
    vpxor(xmm14, xmm14, xmm14);
    movq(xmm15, GetSrc(Op->Header.Args[0].ID()));
    vshufpd(xmm15, xmm15, GetSrc(Op->Header.Args[1].ID()), 0b00);
    vpaddw(GetDst(Node), xmm15, xmm14);
    switch (Op->Header.ElementSize) {
      case 2:
        vphaddw(GetDst(Node), xmm15, xmm14);
        break;
      case 4:
        vphaddd(GetDst(Node), xmm15, xmm14);
        break;
      default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
    }
  }
  else {
    switch (Op->Header.ElementSize) {
      case 2:
        vphaddw(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
        break;
      case 4:
        vphaddd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
        break;
      default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
    }
  }
}

DEF_OP(VAddV) {
  auto Op = IROp->C<IR::IROp_VAddV>();
  uint8_t OpSize = IROp->Size;

  auto Src = GetSrc(Op->Header.Args[0].ID());
  auto Dest = GetDst(Node);
  vpxor(xmm15, xmm15, xmm15);
  uint8_t Elements = OpSize / Op->Header.ElementSize;
  switch (Op->Header.ElementSize) {
    case 2: {
      for (int i = Elements; i > 1; i >>= 1) {
        phaddw(Dest, Src);
        Src = Dest;
      }
      pextrw(eax, Dest, 0);
      pinsrw(xmm15, eax, 0);
    break;
    }
    case 4: {
      for (int i = Elements; i > 1; i >>= 1) {
        phaddd(Dest, Src);
        Src = Dest;
      }
      pextrd(eax, Dest, 0);
      pinsrd(xmm15, eax, 0);
    break;
    }
    default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
  }

  movaps(Dest, xmm15);
}

DEF_OP(VURAvg) {
  auto Op = IROp->C<IR::IROp_VURAvg>();
  switch (Op->Header.ElementSize) {
    case 1: {
      vpavgb(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
    }
    case 2: {
      vpavgw(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
    }
    default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
  }
}

DEF_OP(VAbs) {
  auto Op = IROp->C<IR::IROp_VAbs>();
  switch (Op->Header.ElementSize) {
    case 1: {
      vpabsb(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));
      break;
    }
    case 2: {
      vpabsw(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));
      break;
    }
    case 4: {
      vpabsd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));
      break;
    }
    case 8: {
      vpabsq(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));
      break;
    }
    default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
  }
}

DEF_OP(VFAdd) {
  auto Op = IROp->C<IR::IROp_VFAdd>();
  uint8_t OpSize = IROp->Size;

  if (Op->Header.ElementSize == OpSize) {
    // Scalar
    switch (Op->Header.ElementSize) {
      case 4: {
        vaddss(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
      }
      case 8: {
        vaddsd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
      }
      default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
    }
  }
  else {
    // Vector
    switch (Op->Header.ElementSize) {
      case 4: {
        vaddps(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
      }
      case 8: {
        vaddpd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
      }
      default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
    }
  }
}

DEF_OP(VFAddP) {
  auto Op = IROp->C<IR::IROp_VFAddP>();
  switch (Op->Header.ElementSize) {
    case 4:
      vhaddps(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
    case 8:
      vhaddpd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
    default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
  }
}

DEF_OP(VFSub) {
  auto Op = IROp->C<IR::IROp_VFSub>();
  uint8_t OpSize = IROp->Size;

  if (Op->Header.ElementSize == OpSize) {
    // Scalar
    switch (Op->Header.ElementSize) {
      case 4: {
        vsubss(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
      }
      case 8: {
        vsubsd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
      }
      default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
    }
  }
  else {
    // Vector
    switch (Op->Header.ElementSize) {
      case 4: {
        vsubps(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
      }
      case 8: {
        vsubpd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
      }
      default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
    }
  }
}

DEF_OP(VFMul) {
  auto Op = IROp->C<IR::IROp_VFMul>();
  uint8_t OpSize = IROp->Size;

  if (Op->Header.ElementSize == OpSize) {
    // Scalar
    switch (Op->Header.ElementSize) {
      case 4: {
        vmulss(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
      }
      case 8: {
        vmulsd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
      }
      default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
    }
  }
  else {
    // Vector
    switch (Op->Header.ElementSize) {
      case 4: {
        vmulps(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
      }
      case 8: {
        vmulpd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
      }
      default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
    }
  }
}

DEF_OP(VFDiv) {
  auto Op = IROp->C<IR::IROp_VFDiv>();
  uint8_t OpSize = IROp->Size;

  if (Op->Header.ElementSize == OpSize) {
    // Scalar
    switch (Op->Header.ElementSize) {
      case 4: {
        vdivss(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
      }
      case 8: {
        vdivsd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
      }
      default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
    }
  }
  else {
    // Vector
    switch (Op->Header.ElementSize) {
      case 4: {
        vdivps(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
      }
      case 8: {
        vdivpd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
      }
      default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
    }
  }
}

DEF_OP(VFMin) {
  auto Op = IROp->C<IR::IROp_VFMin>();
  uint8_t OpSize = IROp->Size;

  if (Op->Header.ElementSize == OpSize) {
    // Scalar
    switch (Op->Header.ElementSize) {
      case 4: {
        vminss(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
      }
      case 8: {
        vminsd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
      }
      default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
    }
  }
  else {
    // Vector
    switch (Op->Header.ElementSize) {
      case 4: {
        vminps(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
      }
      case 8: {
        vminpd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
      }
      default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
    }
  }
}

DEF_OP(VFMax) {
  auto Op = IROp->C<IR::IROp_VFMax>();
  uint8_t OpSize = IROp->Size;

  if (Op->Header.ElementSize == OpSize) {
    // Scalar
    switch (Op->Header.ElementSize) {
      case 4: {
        vmaxss(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
      }
      case 8: {
        vmaxsd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
      }
      default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
    }
  }
  else {
    // Vector
    switch (Op->Header.ElementSize) {
      case 4: {
        vmaxps(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
      }
      case 8: {
        vmaxpd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
      }
      default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
    }
  }
}

DEF_OP(VFRecp) {
  auto Op = IROp->C<IR::IROp_VFRecp>();
  uint8_t OpSize = IROp->Size;

  if (Op->Header.ElementSize == OpSize) {
    // Scalar
    switch (Op->Header.ElementSize) {
      case 4: {
        mov(eax, 0x3f800000); // 1.0f
        vmovd(xmm15, eax);
        vdivss(GetDst(Node), xmm15, GetSrc(Op->Header.Args[0].ID()));
      break;
      }
      default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
    }
  }
  else {
    // Vector
    switch (Op->Header.ElementSize) {
      case 4: {
        mov(eax, 0x3f800000); // 1.0f
        vmovd(xmm15, eax);
        pshufd(xmm15, xmm15, 0);
        vdivps(GetDst(Node), xmm15, GetSrc(Op->Header.Args[0].ID()));
      break;
      }
      default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
    }
  }
}

DEF_OP(VFSqrt) {
  auto Op = IROp->C<IR::IROp_VFSqrt>();
  uint8_t OpSize = IROp->Size;

  if (Op->Header.ElementSize == OpSize) {
    // Scalar
    switch (Op->Header.ElementSize) {
      case 4: {
        vsqrtss(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[0].ID()));
      break;
      }
      case 8: {
        vsqrtsd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[0].ID()));
      break;
      }
      default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
    }
  }
  else {
    // Vector
    switch (Op->Header.ElementSize) {
      case 4: {
        vsqrtps(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));
      break;
      }
      case 8: {
        vsqrtpd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));
      break;
      }
      default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
    }
  }
}

DEF_OP(VFRSqrt) {
  auto Op = IROp->C<IR::IROp_VFRSqrt>();
  uint8_t OpSize = IROp->Size;

  if (Op->Header.ElementSize == OpSize) {
    // Scalar
    switch (Op->Header.ElementSize) {
      case 4: {
        mov(eax, 0x3f800000); // 1.0f
        sqrtss(xmm15, GetSrc(Op->Header.Args[0].ID()));
        vmovd(GetDst(Node), eax);
        divss(GetDst(Node), xmm15);
      break;
      }
      case 8: {
        mov(eax, 0x3f800000); // 1.0f
        sqrtsd(xmm15, GetSrc(Op->Header.Args[0].ID()));
        vmovd(GetDst(Node), eax);
        divsd(GetDst(Node), xmm15);
      break;
      }
      default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
    }
  }
  else {
    // Vector
    switch (Op->Header.ElementSize) {
      case 4: {
        mov(rax, 0x3f800000); // 1.0f
        sqrtps(xmm15, GetSrc(Op->Header.Args[0].ID()));
        vmovd(GetDst(Node), eax);
        pshufd(GetDst(Node), GetDst(Node), 0);
        divps(GetDst(Node), xmm15);
      break;
      }
      default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
    }
  }
}

DEF_OP(VNeg) {
  auto Op = IROp->C<IR::IROp_VNeg>();
  vpxor(xmm15, xmm15, xmm15);
  switch (Op->Header.ElementSize) {
    case 1: {
      vpsubb(GetDst(Node), xmm15, GetSrc(Op->Header.Args[0].ID()));
    break;
    }
    case 2: {
      vpsubw(GetDst(Node), xmm15, GetSrc(Op->Header.Args[0].ID()));
    break;
    }
    case 4: {
      vpsubd(GetDst(Node), xmm15, GetSrc(Op->Header.Args[0].ID()));
    break;
    }
    case 8: {
      vpsubq(GetDst(Node), xmm15, GetSrc(Op->Header.Args[0].ID()));
    break;
    }
    default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
  }
}

DEF_OP(VFNeg) {
  auto Op = IROp->C<IR::IROp_VNeg>();
  switch (Op->Header.ElementSize) {
    case 4: {
      mov(rax, 0x80000000);
      vmovd(xmm15, eax);
      pshufd(xmm15, xmm15, 0);
      vxorps(GetDst(Node), xmm15, GetSrc(Op->Header.Args[0].ID()));
    break;
    }
    case 8: {
      mov(rax, 0x8000000000000000ULL);
      vmovq(xmm15, rax);
      pshufd(xmm15, xmm15, 0b01'00'01'00);
      vxorpd(GetDst(Node), xmm15, GetSrc(Op->Header.Args[0].ID()));
    break;
    }
    default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
  }
}

DEF_OP(VNot) {
  auto Op = IROp->C<IR::IROp_VNot>();
  pcmpeqd(xmm15, xmm15);
  vpxor(GetDst(Node), xmm15, GetSrc(Op->Header.Args[0].ID()));
}

DEF_OP(VUMin) {
  auto Op = IROp->C<IR::IROp_VUMin>();
  switch (Op->Header.ElementSize) {
    case 1: {
      vpminub(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
    }
    case 2: {
      vpminuw(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
    }
    case 4: {
      vpminud(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
    }
    default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
  }
}

DEF_OP(VSMin) {
  auto Op = IROp->C<IR::IROp_VSMin>();
  switch (Op->Header.ElementSize) {
    case 1: {
      vpminsb(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
    }
    case 2: {
      vpminsw(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
    }
    case 4: {
      vpminsd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
    }
    default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
  }
}

DEF_OP(VUMax) {
  auto Op = IROp->C<IR::IROp_VUMax>();
  switch (Op->Header.ElementSize) {
    case 1: {
      vpmaxub(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
    }
    case 2: {
      vpmaxuw(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
    }
    case 4: {
      vpmaxud(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
    }
    default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
  }
}

DEF_OP(VSMax) {
  auto Op = IROp->C<IR::IROp_VSMax>();
  switch (Op->Header.ElementSize) {
    case 1: {
      vpmaxsb(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
    }
    case 2: {
      vpmaxsw(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
    }
    case 4: {
      vpmaxsd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
    }
    default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
  }
}

DEF_OP(VZip) {
  auto Op = IROp->C<IR::IROp_VZip>();
  movapd(xmm15, GetSrc(Op->Header.Args[0].ID()));

  switch (Op->Header.ElementSize) {
    case 1: {
      punpcklbw(xmm15, GetSrc(Op->Header.Args[1].ID()));
      break;
    }
    case 2: {
      punpcklwd(xmm15, GetSrc(Op->Header.Args[1].ID()));
      break;
    }
    case 4: {
      punpckldq(xmm15, GetSrc(Op->Header.Args[1].ID()));
      break;
    }
    case 8: {
      punpcklqdq(xmm15, GetSrc(Op->Header.Args[1].ID()));
      break;
    }
    default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
  }
  movapd(GetDst(Node), xmm15);
}

DEF_OP(VZip2) {
  auto Op = IROp->C<IR::IROp_VZip2>();
  uint8_t OpSize = IROp->Size;

  movapd(xmm15, GetSrc(Op->Header.Args[0].ID()));

  if (OpSize == 8) {
    vpslldq(xmm15, GetSrc(Op->Header.Args[0].ID()), 4);
    vpslldq(xmm14, GetSrc(Op->Header.Args[1].ID()), 4);
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
    default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
    }
  }
  else {
    switch (Op->Header.ElementSize) {
    case 1: {
      punpckhbw(xmm15, GetSrc(Op->Header.Args[1].ID()));
    break;
    }
    case 2: {
      punpckhwd(xmm15, GetSrc(Op->Header.Args[1].ID()));
    break;
    }
    case 4: {
      punpckhdq(xmm15, GetSrc(Op->Header.Args[1].ID()));
    break;
    }
    case 8: {
      punpckhqdq(xmm15, GetSrc(Op->Header.Args[1].ID()));
    break;
    }
    default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
    }
    movapd(GetDst(Node), xmm15);
  }
}

DEF_OP(VBSL) {
  LogMan::Msg::A("Unimplemented");
}

DEF_OP(VCMPEQ) {
  auto Op = IROp->C<IR::IROp_VCMPEQ>();

  switch (Op->Header.ElementSize) {
    case 1:
      vpcmpeqb(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
    case 2:
      vpcmpeqw(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
    case 4:
      vpcmpeqd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
    case 8:
      vpcmpeqq(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
    default: LogMan::Msg::A("Unsupported elementSize: %d", Op->Header.ElementSize);
  }
}

DEF_OP(VCMPEQZ) {
  auto Op = IROp->C<IR::IROp_VCMPEQZ>();
  vpxor(xmm15, xmm15, xmm15);

  switch (Op->Header.ElementSize) {
    case 1:
      vpcmpeqb(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), xmm15);
      break;
    case 2:
      vpcmpeqw(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), xmm15);
      break;
    case 4:
      vpcmpeqd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), xmm15);
      break;
    case 8:
      vpcmpeqq(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), xmm15);
      break;
    default: LogMan::Msg::A("Unsupported elementSize: %d", Op->Header.ElementSize);
  }
}

DEF_OP(VCMPGT) {
  auto Op = IROp->C<IR::IROp_VCMPGT>();

  switch (Op->Header.ElementSize) {
    case 1:
      vpcmpgtb(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
    case 2:
      vpcmpgtw(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
    case 4:
      vpcmpgtd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
    case 8:
      vpcmpgtq(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
    default: LogMan::Msg::A("Unsupported elementSize: %d", Op->Header.ElementSize);
  }
}

DEF_OP(VCMPGTZ) {
  auto Op = IROp->C<IR::IROp_VCMPGTZ>();
  vpxor(xmm15, xmm15, xmm15);

  switch (Op->Header.ElementSize) {
    case 1:
      vpcmpgtb(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), xmm15);
      break;
    case 2:
      vpcmpgtw(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), xmm15);
      break;
    case 4:
      vpcmpgtd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), xmm15);
      break;
    case 8:
      vpcmpgtq(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), xmm15);
      break;
    default: LogMan::Msg::A("Unsupported elementSize: %d", Op->Header.ElementSize);
  }
}

DEF_OP(VCMPLTZ) {
  auto Op = IROp->C<IR::IROp_VCMPLTZ>();
  vpxor(xmm15, xmm15, xmm15);

  switch (Op->Header.ElementSize) {
    case 1:
      vpcmpgtb(GetDst(Node), xmm15, GetSrc(Op->Header.Args[0].ID()));
      break;
    case 2:
      vpcmpgtw(GetDst(Node), xmm15, GetSrc(Op->Header.Args[0].ID()));
      break;
    case 4:
      vpcmpgtd(GetDst(Node), xmm15, GetSrc(Op->Header.Args[0].ID()));
      break;
    case 8:
      vpcmpgtq(GetDst(Node), xmm15, GetSrc(Op->Header.Args[0].ID()));
      break;
    default: LogMan::Msg::A("Unsupported elementSize: %d", Op->Header.ElementSize);
  }
}

DEF_OP(VFCMPEQ) {
  auto Op = IROp->C<IR::IROp_VFCMPEQ>();
  uint8_t OpSize = IROp->Size;

  if (Op->Header.ElementSize == OpSize) {
    switch (Op->Header.ElementSize) {
    case 4:
      vcmpss(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()), 0);
    break;
    case 8:
      vcmpsd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()), 0);
    break;
    default: LogMan::Msg::A("Unsupported elementSize: %d", Op->Header.ElementSize);
    }
  }
  else {
    switch (Op->Header.ElementSize) {
    case 4:
      vcmpps(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()), 0);
    break;
    case 8:
      vcmppd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()), 0);
    break;
    default: LogMan::Msg::A("Unsupported elementSize: %d", Op->Header.ElementSize);
    }
  }
}

DEF_OP(VFCMPNEQ) {
  auto Op = IROp->C<IR::IROp_VFCMPNEQ>();
  uint8_t OpSize = IROp->Size;

  if (Op->Header.ElementSize == OpSize) {
    switch (Op->Header.ElementSize) {
    case 4:
      vcmpss(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()), 4);
    break;
    case 8:
      vcmpsd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()), 4);
    break;
    default: LogMan::Msg::A("Unsupported elementSize: %d", Op->Header.ElementSize);
    }

  }
  else {
    switch (Op->Header.ElementSize) {
    case 4:
      vcmpps(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()), 4);
    break;
    case 8:
      vcmppd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()), 4);
    break;
    default: LogMan::Msg::A("Unsupported elementSize: %d", Op->Header.ElementSize);
    }
  }
}

DEF_OP(VFCMPLT) {
  auto Op = IROp->C<IR::IROp_VFCMPLT>();
  uint8_t OpSize = IROp->Size;

  if (Op->Header.ElementSize == OpSize) {
    switch (Op->Header.ElementSize) {
    case 4:
      vcmpss(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()), 1);
    break;
    case 8:
      vcmpsd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()), 1);
    break;
    default: LogMan::Msg::A("Unsupported elementSize: %d", Op->Header.ElementSize);
    }
  }
  else {
    switch (Op->Header.ElementSize) {
    case 4:
      vcmpps(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()), 1);
    break;
    case 8:
      vcmppd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()), 1);
    break;
    default: LogMan::Msg::A("Unsupported elementSize: %d", Op->Header.ElementSize);
    }
  }
}

DEF_OP(VFCMPGT) {
  auto Op = IROp->C<IR::IROp_VFCMPGT>();
  uint8_t OpSize = IROp->Size;

  if (Op->Header.ElementSize == OpSize) {
    switch (Op->Header.ElementSize) {
    case 4:
      vcmpss(GetDst(Node), GetSrc(Op->Header.Args[1].ID()), GetSrc(Op->Header.Args[0].ID()), 1);
    break;
    case 8:
      vcmpsd(GetDst(Node), GetSrc(Op->Header.Args[1].ID()), GetSrc(Op->Header.Args[0].ID()), 1);
    break;
    default: LogMan::Msg::A("Unsupported elementSize: %d", Op->Header.ElementSize);
    }
  }
  else {
    switch (Op->Header.ElementSize) {
    case 4:
      vcmpps(GetDst(Node), GetSrc(Op->Header.Args[1].ID()), GetSrc(Op->Header.Args[0].ID()), 1);
    break;
    case 8:
      vcmppd(GetDst(Node), GetSrc(Op->Header.Args[1].ID()), GetSrc(Op->Header.Args[0].ID()), 1);
    break;
    default: LogMan::Msg::A("Unsupported elementSize: %d", Op->Header.ElementSize);
    }
  }
}

DEF_OP(VFCMPLE) {
  auto Op = IROp->C<IR::IROp_VFCMPLE>();
  uint8_t OpSize = IROp->Size;

  if (Op->Header.ElementSize == OpSize) {
    switch (Op->Header.ElementSize) {
    case 4:
      vcmpss(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()), 2);
    break;
    case 8:
      vcmpsd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()), 2);
    break;
    default: LogMan::Msg::A("Unsupported elementSize: %d", Op->Header.ElementSize);
    }
  }
  else {
    switch (Op->Header.ElementSize) {
    case 4:
      vcmpps(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()), 2);
    break;
    case 8:
      vcmppd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()), 2);
    break;
    default: LogMan::Msg::A("Unsupported elementSize: %d", Op->Header.ElementSize);
    }
  }
}

DEF_OP(VFCMPORD) {
  auto Op = IROp->C<IR::IROp_VFCMPORD>();
  uint8_t OpSize = IROp->Size;

  if (Op->Header.ElementSize == OpSize) {
    switch (Op->Header.ElementSize) {
    case 4:
      vcmpss(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()), 7);
    break;
    case 8:
      vcmpsd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()), 7);
    break;
    default: LogMan::Msg::A("Unsupported elementSize: %d", Op->Header.ElementSize);
    }
  }
  else {
    switch (Op->Header.ElementSize) {
    case 4:
      vcmpps(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()), 7);
    break;
    case 8:
      vcmppd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()), 7);
    break;
    default: LogMan::Msg::A("Unsupported elementSize: %d", Op->Header.ElementSize);
    }
  }
}

DEF_OP(VFCMPUNO) {
  auto Op = IROp->C<IR::IROp_VFCMPUNO>();
  uint8_t OpSize = IROp->Size;

  if (Op->Header.ElementSize == OpSize) {
    switch (Op->Header.ElementSize) {
    case 4:
      vcmpss(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()), 3);
    break;
    case 8:
      vcmpsd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()), 3);
    break;
    default: LogMan::Msg::A("Unsupported elementSize: %d", Op->Header.ElementSize);
    }
  }
  else {
    switch (Op->Header.ElementSize) {
    case 4:
      vcmpps(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()), 3);
    break;
    case 8:
      vcmppd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()), 3);
    break;
    default: LogMan::Msg::A("Unsupported elementSize: %d", Op->Header.ElementSize);
    }
  }
}

DEF_OP(VUShl) {
  LogMan::Msg::A("Unimplemented");
}

DEF_OP(VUShr) {
  LogMan::Msg::A("Unimplemented");
}

DEF_OP(VSShr) {
  LogMan::Msg::A("Unimplemented");
}

DEF_OP(VUShlS) {
  auto Op = IROp->C<IR::IROp_VUShlS>();

  switch (Op->Header.ElementSize) {
    case 2: {
      vpsllw(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
    }
    case 4: {
      vpslld(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
    }
    case 8: {
      vpsllq(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
    }
    default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
  }
}

DEF_OP(VUShrS) {
  auto Op = IROp->C<IR::IROp_VUShrS>();

  switch (Op->Header.ElementSize) {
    case 2: {
      vpsrlw(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
    }
    case 4: {
      vpsrld(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
    }
    case 8: {
      vpsrlq(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
    }
    default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
  }
}

DEF_OP(VSShrS) {
  auto Op = IROp->C<IR::IROp_VSShrS>();

  switch (Op->Header.ElementSize) {
    case 2: {
      vpsraw(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
    }
    case 4: {
      vpsrad(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
    }
    case 8: // Doesn't exist on x86
    default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
  }
}

DEF_OP(VInsElement) {
  auto Op = IROp->C<IR::IROp_VInsElement>();
  movapd(xmm15, GetSrc(Op->Header.Args[0].ID()));

  // Dst_d[Op->DestIdx] = Src2_d[Op->SrcIdx];

  // pextrq reg64/mem64, xmm, imm
  // pinsrq xmm, reg64/mem64, imm8
  switch (Op->Header.ElementSize) {
  case 1: {
    pextrb(eax, GetSrc(Op->Header.Args[1].ID()), Op->SrcIdx);
    pinsrb(xmm15, eax, Op->DestIdx);
  break;
  }
  case 2: {
    pextrw(eax, GetSrc(Op->Header.Args[1].ID()), Op->SrcIdx);
    pinsrw(xmm15, eax, Op->DestIdx);
  break;
  }
  case 4: {
    pextrd(eax, GetSrc(Op->Header.Args[1].ID()), Op->SrcIdx);
    pinsrd(xmm15, eax, Op->DestIdx);
  break;
  }
  case 8: {
    pextrq(rax, GetSrc(Op->Header.Args[1].ID()), Op->SrcIdx);
    pinsrq(xmm15, rax, Op->DestIdx);
  break;
  }
  default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
  }

  movapd(GetDst(Node), xmm15);
}

DEF_OP(VInsScalarElement) {
  auto Op = IROp->C<IR::IROp_VInsScalarElement>();
  movapd(xmm15, GetSrc(Op->Header.Args[0].ID()));

  // Dst_d[Op->DestIdx] = Src2_d[Op->SrcIdx];

  // pextrq reg64/mem64, xmm, imm
  // pinsrq xmm, reg64/mem64, imm8
  switch (Op->Header.ElementSize) {
  case 1: {
    pextrb(eax, GetSrc(Op->Header.Args[1].ID()), 0);
    pinsrb(xmm15, eax, Op->DestIdx);
  break;
  }
  case 2: {
    pextrw(eax, GetSrc(Op->Header.Args[1].ID()), 0);
    pinsrw(xmm15, eax, Op->DestIdx);
  break;
  }
  case 4: {
    pextrd(eax, GetSrc(Op->Header.Args[1].ID()), 0);
    pinsrd(xmm15, eax, Op->DestIdx);
  break;
  }
  case 8: {
    pextrq(rax, GetSrc(Op->Header.Args[1].ID()), 0);
    pinsrq(xmm15, rax, Op->DestIdx);
  break;
  }
  default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
  }

  movapd(GetDst(Node), xmm15);
}

DEF_OP(VExtractElement) {
  auto Op = IROp->C<IR::IROp_VExtractElement>();

  switch (Op->Header.ElementSize) {
    case 1: {
      pextrb(eax, GetSrc(Op->Header.Args[0].ID()), Op->Index);
      pinsrb(GetDst(Node), eax, 0);
      break;
    }
    case 2: {
      pextrw(eax, GetSrc(Op->Header.Args[0].ID()), Op->Index);
      pinsrw(GetDst(Node), eax, 0);
      break;
    }
    case 4: {
      pextrd(eax, GetSrc(Op->Header.Args[0].ID()), Op->Index);
      pinsrd(GetDst(Node), eax, 0);
      break;
    }
    case 8: {
      pextrq(rax, GetSrc(Op->Header.Args[0].ID()), Op->Index);
      pinsrq(GetDst(Node), rax, 0);
      break;
    }
    default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
  }
}

DEF_OP(VExtr) {
  auto Op = IROp->C<IR::IROp_VExtr>();
  uint8_t OpSize = IROp->Size;

  if (OpSize == 8) {
    // No way to do this with 64bit source without dropping to MMX
    // So emulate it
    vpxor(xmm14, xmm14, xmm14);
    movq(xmm15, GetSrc(Op->Header.Args[1].ID()));
    vshufpd(xmm15, xmm15, GetSrc(Op->Header.Args[0].ID()), 0b00);
    vpalignr(GetDst(Node), xmm14, xmm15, Op->Index);
  }
  else {
    vpalignr(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()), Op->Index);
  }
}

DEF_OP(VSLI) {
  auto Op = IROp->C<IR::IROp_VSLI>();
  movapd(xmm15, GetSrc(Op->Header.Args[0].ID()));
  pslldq(xmm15, Op->ByteShift);
  movapd(GetDst(Node), xmm15);
}

DEF_OP(VSRI) {
  auto Op = IROp->C<IR::IROp_VSRI>();
  movapd(xmm15, GetSrc(Op->Header.Args[0].ID()));
  psrldq(xmm15, Op->ByteShift);
  movapd(GetDst(Node), xmm15);
}

DEF_OP(VUShrI) {
  auto Op = IROp->C<IR::IROp_VUShrI>();
  movapd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));
  switch (Op->Header.ElementSize) {
    case 2: {
      psrlw(GetDst(Node), Op->BitShift);
      break;
    }
    case 4: {
      psrld(GetDst(Node), Op->BitShift);
      break;
    }
    case 8: {
      psrlq(GetDst(Node), Op->BitShift);
      break;
    }
    default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
  }
}

DEF_OP(VSShrI) {
  auto Op = IROp->C<IR::IROp_VSShrI>();
  movapd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));
  switch (Op->Header.ElementSize) {
    case 2: {
      psraw(GetDst(Node), Op->BitShift);
      break;
    }
    case 4: {
      psrad(GetDst(Node), Op->BitShift);
      break;
    }
    default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
  }
}

DEF_OP(VShlI) {
  auto Op = IROp->C<IR::IROp_VShlI>();
  movapd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));
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
    default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
  }
}

DEF_OP(VUShrNI) {
  auto Op = IROp->C<IR::IROp_VUShrNI>();
  movapd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));
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
    default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
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
  movapd(xmm13, GetSrc(Op->Header.Args[1].ID()));
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
    default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
  }

  vmovq(xmm15, rax);
  vmovq(xmm14, rcx);
  punpcklqdq(xmm15, xmm14);
  vpshufb(xmm14, xmm13, xmm15);
  vpor(GetDst(Node), xmm14, GetSrc(Op->Header.Args[0].ID()));
}

DEF_OP(VBitcast) {
  auto Op = IROp->C<IR::IROp_VBitcast>();
  movaps(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));
}

DEF_OP(VSXTL) {
  auto Op = IROp->C<IR::IROp_VSXTL>();
  switch (Op->Header.ElementSize) {
    case 2:
      pmovsxbw(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));
    break;
    case 4:
      pmovsxwd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));
    break;
    case 8:
      pmovsxdq(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));
    break;
    default: LogMan::Msg::A("Unknown element size: %d", Op->Header.ElementSize);
  }
}

DEF_OP(VSXTL2) {
  auto Op = IROp->C<IR::IROp_VSXTL2>();
  uint8_t OpSize = IROp->Size;

  vpsrldq(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), OpSize / 2);
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
    default: LogMan::Msg::A("Unknown element size: %d", Op->Header.ElementSize);
  }
}

DEF_OP(VUXTL) {
  auto Op = IROp->C<IR::IROp_VUXTL>();
  switch (Op->Header.ElementSize) {
    case 2:
      pmovzxbw(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));
    break;
    case 4:
      pmovzxwd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));
    break;
    case 8:
      pmovzxdq(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));
    break;
    default: LogMan::Msg::A("Unknown element size: %d", Op->Header.ElementSize);
  }
}

DEF_OP(VUXTL2) {
  auto Op = IROp->C<IR::IROp_VUXTL2>();
  uint8_t OpSize = IROp->Size;

  vpsrldq(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), OpSize / 2);
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
    default: LogMan::Msg::A("Unknown element size: %d", Op->Header.ElementSize);
  }
}

DEF_OP(VSQXTN) {
  auto Op = IROp->C<IR::IROp_VSQXTN>();
  switch (Op->Header.ElementSize) {
    case 1:
      packsswb(xmm15, GetSrc(Op->Header.Args[0].ID()));
    break;
    case 2:
      packssdw(xmm15, GetSrc(Op->Header.Args[0].ID()));
    break;
    default: LogMan::Msg::A("Unknown element size: %d", Op->Header.ElementSize);
  }
  psrldq(xmm15, 8);
  movaps(GetDst(Node), xmm15);
}

DEF_OP(VSQXTN2) {
  auto Op = IROp->C<IR::IROp_VSQXTN2>();
  uint8_t OpSize = IROp->Size;

  // Zero the lower bits
  vpxor(xmm15, xmm15, xmm15);
  switch (Op->Header.ElementSize) {
    case 1:
      packsswb(xmm15, GetSrc(Op->Header.Args[1].ID()));
    break;
    case 2:
      packssdw(xmm15, GetSrc(Op->Header.Args[1].ID()));
    break;
    default: LogMan::Msg::A("Unknown element size: %d", Op->Header.ElementSize);
  }

  if (OpSize == 8) {
    psrldq(xmm15, OpSize / 2);
  }
  vpor(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), xmm15);
}

DEF_OP(VSQXTUN) {
  auto Op = IROp->C<IR::IROp_VSQXTUN>();
  switch (Op->Header.ElementSize) {
    case 1:
      packuswb(xmm15, GetSrc(Op->Header.Args[0].ID()));
    break;
    case 2:
      packusdw(xmm15, GetSrc(Op->Header.Args[0].ID()));
    break;
    default: LogMan::Msg::A("Unknown element size: %d", Op->Header.ElementSize);
  }
  psrldq(xmm15, 8);
  movaps(GetDst(Node), xmm15);
}

DEF_OP(VSQXTUN2) {
  auto Op = IROp->C<IR::IROp_VSQXTUN2>();
  uint8_t OpSize = IROp->Size;

  // Zero the lower bits
  vpxor(xmm15, xmm15, xmm15);
  switch (Op->Header.ElementSize) {
    case 1:
      packuswb(xmm15, GetSrc(Op->Header.Args[1].ID()));
    break;
    case 2:
      packusdw(xmm15, GetSrc(Op->Header.Args[1].ID()));
    break;
    default: LogMan::Msg::A("Unknown element size: %d", Op->Header.ElementSize);
  }
  if (OpSize == 8) {
    psrldq(xmm15, OpSize / 2);
  }

  vpor(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), xmm15);
}

DEF_OP(VMul) {
  auto Op = IROp->C<IR::IROp_VUMul>();
  switch (Op->Header.ElementSize) {
    case 2: {
      vpmullw(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
    }
    case 4: {
      vpmulld(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
    }
    default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
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
      vpunpcklwd(xmm15, GetSrc(Op->Header.Args[0].ID()), xmm15);
      vpunpcklwd(xmm14, GetSrc(Op->Header.Args[1].ID()), xmm14);
      vpmulld(GetDst(Node), xmm14, xmm15);
      break;
    }
    case 8: {
      // We need to shuffle the data for this one
      // x86 PMULUDQ wants the 32bit values in [31:0] and [95:64]
      // Which then extends out to [63:0] and [127:64]
      vpshufd(xmm14, GetSrc(Op->Header.Args[0].ID()), 0b10'10'00'00);
      vpshufd(xmm15, GetSrc(Op->Header.Args[1].ID()), 0b10'10'00'00);

      vpmuludq(GetDst(Node), xmm14, xmm15);
    break;
    }
    default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
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
      vpunpcklwd(xmm15, GetSrc(Op->Header.Args[0].ID()), xmm15);
      vpunpcklwd(xmm14, GetSrc(Op->Header.Args[1].ID()), xmm14);
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
      vpshufd(xmm14, GetSrc(Op->Header.Args[0].ID()), 0b10'10'00'00);
      vpshufd(xmm15, GetSrc(Op->Header.Args[1].ID()), 0b10'10'00'00);

      vpmuldq(GetDst(Node), xmm14, xmm15);
    break;
    }
    default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
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
      vpunpckhwd(xmm15, GetSrc(Op->Header.Args[0].ID()), xmm15);
      vpunpckhwd(xmm14, GetSrc(Op->Header.Args[1].ID()), xmm14);
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

      vpshufd(xmm14, GetSrc(Op->Header.Args[0].ID()), 0b11'11'10'10);
      vpshufd(xmm15, GetSrc(Op->Header.Args[1].ID()), 0b11'11'10'10);

      vpmuludq(GetDst(Node), xmm14, xmm15);
    break;
    }
    default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
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
      vpunpckhwd(xmm15, GetSrc(Op->Header.Args[0].ID()), xmm15);
      vpunpckhwd(xmm14, GetSrc(Op->Header.Args[1].ID()), xmm14);
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

      vpshufd(xmm14, GetSrc(Op->Header.Args[0].ID()), 0b11'11'10'10);
      vpshufd(xmm15, GetSrc(Op->Header.Args[1].ID()), 0b11'11'10'10);

      vpmuldq(GetDst(Node), xmm14, xmm15);
    break;
    }
    default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
  }
}

DEF_OP(VTBL1) {
  auto Op = IROp->C<IR::IROp_VTBL1>();
  uint8_t OpSize = IROp->Size;

  switch (OpSize) {
    case 8: {
      vpshufb(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      movq(GetDst(Node), GetDst(Node));
      break;
    }
    case 16: {
      vpshufb(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
      break;
    }
    default: LogMan::Msg::A("Unknown OpSize: %d", OpSize); break;
  }
}

#undef DEF_OP
void JITCore::RegisterVectorHandlers() {
#define REGISTER_OP(op, x) OpHandlers[FEXCore::IR::IROps::OP_##op] = &JITCore::Op_##x
  REGISTER_OP(VECTORZERO,        VectorZero);
  REGISTER_OP(VECTORIMM,         VectorImm);
  REGISTER_OP(CREATEVECTOR2,     CreateVector2);
  REGISTER_OP(CREATEVECTOR4,     CreateVector4);
  REGISTER_OP(SPLATVECTOR2,      SplatVector);
  REGISTER_OP(SPLATVECTOR4,      SplatVector);
  REGISTER_OP(VMOV,              VMov);
  REGISTER_OP(VAND,              VAnd);
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
  REGISTER_OP(VURAVG,            VURAvg);
  REGISTER_OP(VABS,              VAbs);
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
  REGISTER_OP(VINSSCALARELEMENT, VInsScalarElement);
  REGISTER_OP(VEXTRACTELEMENT,   VExtractElement);
  REGISTER_OP(VEXTR,             VExtr);
  REGISTER_OP(VSLI,              VSLI);
  REGISTER_OP(VSRI,              VSRI);
  REGISTER_OP(VUSHRI,            VUShrI);
  REGISTER_OP(VSSHRI,            VSShrI);
  REGISTER_OP(VSHLI,             VShlI);
  REGISTER_OP(VUSHRNI,           VUShrNI);
  REGISTER_OP(VUSHRNI2,          VUShrNI2);
  REGISTER_OP(VBITCAST,          VBitcast);
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
  REGISTER_OP(VTBL1,             VTBL1);
#undef REGISTER_OP
}
}

