#include "Interface/Core/JIT/Arm64/JITClass.h"

namespace FEXCore::CPU {

using namespace vixl;
using namespace vixl::aarch64;
#define DEF_OP(x) void JITCore::Op_##x(FEXCore::IR::IROp_Header *IROp, uint32_t Node)
DEF_OP(VectorZero) {
  uint8_t OpSize = IROp->Size;
  switch (OpSize) {
    case 8: {
      eor(GetDst(Node).V8B(), GetDst(Node).V8B(), GetDst(Node).V8B());
      break;
    }
    case 16: {
       eor(GetDst(Node).V16B(), GetDst(Node).V16B(), GetDst(Node).V16B());
       break;
     }
    default: LogMan::Msg::A("Unknown Element Size: %d", OpSize); break;
  }
}

DEF_OP(VectorImm) {
  auto Op = IROp->C<IR::IROp_VectorImm>();

  uint8_t OpSize = IROp->Size;
  uint8_t Elements = OpSize / Op->Header.ElementSize;

  movi(GetDst(Node).VCast(OpSize * 8, Elements), Op->Immediate);
}

DEF_OP(CreateVector2) {
  LogMan::Msg::A("Unimplemented");
}

DEF_OP(CreateVector4) {
  LogMan::Msg::A("Unimplemented");
}

DEF_OP(SplatVector2) {
	auto Op = IROp->C<IR::IROp_SplatVector2>();
  uint8_t OpSize = IROp->Size;
	LogMan::Throw::A(OpSize <= 16, "Can't handle a vector of size: %d", OpSize);

	uint8_t ElementSize = OpSize / 2;

	switch (ElementSize) {
		case 4:
			dup(GetDst(Node).V4S(), GetSrc(Op->Header.Args[0].ID()).V4S(), 0);
		break;
		case 8:
			dup(GetDst(Node).V2D(), GetSrc(Op->Header.Args[0].ID()).V2D(), 0);
		break;
		default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.Size); break;
	}
}

DEF_OP(SplatVector4) {
	auto Op = IROp->C<IR::IROp_SplatVector4>();
  uint8_t OpSize = IROp->Size;
	LogMan::Throw::A(OpSize <= 16, "Can't handle a vector of size: %d", OpSize);

	uint8_t ElementSize = OpSize / 4;

	switch (ElementSize) {
		case 4:
			dup(GetDst(Node).V4S(), GetSrc(Op->Header.Args[0].ID()).V4S(), 0);
		break;
		case 8:
			dup(GetDst(Node).V2D(), GetSrc(Op->Header.Args[0].ID()).V2D(), 0);
		break;
		default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.Size); break;
	}
}

DEF_OP(VMov) {
	auto Op = IROp->C<IR::IROp_VMov>();
  uint8_t OpSize = IROp->Size;

	switch (OpSize) {
		case 1: {
			eor(VTMP1.V16B(), VTMP1.V16B(), VTMP1.V16B());
			mov(VTMP1.V16B(), 0, GetSrc(Op->Header.Args[0].ID()).V16B(), 0);
			mov(GetDst(Node), VTMP1);
			break;
		}
		case 2: {
			eor(VTMP1.V16B(), VTMP1.V16B(), VTMP1.V16B());
			mov(VTMP1.V8H(), 0, GetSrc(Op->Header.Args[0].ID()).V8H(), 0);
			mov(GetDst(Node), VTMP1);
			break;
		}
		case 4: {
			eor(VTMP1.V16B(), VTMP1.V16B(), VTMP1.V16B());
			mov(VTMP1.V4S(), 0, GetSrc(Op->Header.Args[0].ID()).V4S(), 0);
			mov(GetDst(Node), VTMP1);
			break;
		}
		case 8: {
			mov(GetDst(Node).V8B(), GetSrc(Op->Header.Args[0].ID()).V8B());
			break;
		}
		case 16: {
      if (GetDst(Node).GetCode() != GetSrc(Op->Header.Args[0].ID()).GetCode())
			  mov(GetDst(Node).V16B(), GetSrc(Op->Header.Args[0].ID()).V16B());
			break;
		}
		default: LogMan::Msg::A("Unknown Element Size: %d", OpSize); break;
	}
}

DEF_OP(VAnd) {
  auto Op = IROp->C<IR::IROp_VAnd>();
  and_(GetDst(Node).V16B(), GetSrc(Op->Header.Args[0].ID()).V16B(), GetSrc(Op->Header.Args[1].ID()).V16B());
}

DEF_OP(VOr) {
  auto Op = IROp->C<IR::IROp_VOr>();
  orr(GetDst(Node).V16B(), GetSrc(Op->Header.Args[0].ID()).V16B(), GetSrc(Op->Header.Args[1].ID()).V16B());
}

DEF_OP(VXor) {
  auto Op = IROp->C<IR::IROp_VXor>();
  eor(GetDst(Node).V16B(), GetSrc(Op->Header.Args[0].ID()).V16B(), GetSrc(Op->Header.Args[1].ID()).V16B());
}

DEF_OP(VAdd) {
  auto Op = IROp->C<IR::IROp_VAdd>();
  switch (Op->Header.ElementSize) {
    case 1: {
      add(GetDst(Node).V16B(), GetSrc(Op->Header.Args[0].ID()).V16B(), GetSrc(Op->Header.Args[1].ID()).V16B());
    break;
    }
    case 2: {
      add(GetDst(Node).V8H(), GetSrc(Op->Header.Args[0].ID()).V8H(), GetSrc(Op->Header.Args[1].ID()).V8H());
    break;
    }
    case 4: {
      add(GetDst(Node).V4S(), GetSrc(Op->Header.Args[0].ID()).V4S(), GetSrc(Op->Header.Args[1].ID()).V4S());
    break;
    }
    case 8: {
      add(GetDst(Node).V2D(), GetSrc(Op->Header.Args[0].ID()).V2D(), GetSrc(Op->Header.Args[1].ID()).V2D());
    break;
    }
    default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
  }
}

DEF_OP(VSub) {
  auto Op = IROp->C<IR::IROp_VSub>();
  switch (Op->Header.ElementSize) {
    case 1: {
      sub(GetDst(Node).V16B(), GetSrc(Op->Header.Args[0].ID()).V16B(), GetSrc(Op->Header.Args[1].ID()).V16B());
    break;
    }
    case 2: {
      sub(GetDst(Node).V8H(), GetSrc(Op->Header.Args[0].ID()).V8H(), GetSrc(Op->Header.Args[1].ID()).V8H());
    break;
    }
    case 4: {
      sub(GetDst(Node).V4S(), GetSrc(Op->Header.Args[0].ID()).V4S(), GetSrc(Op->Header.Args[1].ID()).V4S());
    break;
    }
    case 8: {
      sub(GetDst(Node).V2D(), GetSrc(Op->Header.Args[0].ID()).V2D(), GetSrc(Op->Header.Args[1].ID()).V2D());
    break;
    }
    default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
  }
}

DEF_OP(VUQAdd) {
  auto Op = IROp->C<IR::IROp_VUQAdd>();
  switch (Op->Header.ElementSize) {
    case 1: {
      uqadd(GetDst(Node).V16B(), GetSrc(Op->Header.Args[0].ID()).V16B(), GetSrc(Op->Header.Args[1].ID()).V16B());
    break;
    }
    case 2: {
      uqadd(GetDst(Node).V8H(), GetSrc(Op->Header.Args[0].ID()).V8H(), GetSrc(Op->Header.Args[1].ID()).V8H());
    break;
    }
    case 4: {
      uqadd(GetDst(Node).V4S(), GetSrc(Op->Header.Args[0].ID()).V4S(), GetSrc(Op->Header.Args[1].ID()).V4S());
    break;
    }
    case 8: {
      uqadd(GetDst(Node).V2D(), GetSrc(Op->Header.Args[0].ID()).V2D(), GetSrc(Op->Header.Args[1].ID()).V2D());
    break;
    }
    default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
  }
}

DEF_OP(VUQSub) {
  auto Op = IROp->C<IR::IROp_VUQSub>();
  switch (Op->Header.ElementSize) {
    case 1: {
      uqsub(GetDst(Node).V16B(), GetSrc(Op->Header.Args[0].ID()).V16B(), GetSrc(Op->Header.Args[1].ID()).V16B());
    break;
    }
    case 2: {
      uqsub(GetDst(Node).V8H(), GetSrc(Op->Header.Args[0].ID()).V8H(), GetSrc(Op->Header.Args[1].ID()).V8H());
    break;
    }
    case 4: {
      uqsub(GetDst(Node).V4S(), GetSrc(Op->Header.Args[0].ID()).V4S(), GetSrc(Op->Header.Args[1].ID()).V4S());
    break;
    }
    case 8: {
      uqsub(GetDst(Node).V2D(), GetSrc(Op->Header.Args[0].ID()).V2D(), GetSrc(Op->Header.Args[1].ID()).V2D());
    break;
    }
    default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
  }
}

DEF_OP(VSQAdd) {
  auto Op = IROp->C<IR::IROp_VSQAdd>();
  switch (Op->Header.ElementSize) {
    case 1: {
      sqadd(GetDst(Node).V16B(), GetSrc(Op->Header.Args[0].ID()).V16B(), GetSrc(Op->Header.Args[1].ID()).V16B());
    break;
    }
    case 2: {
      sqadd(GetDst(Node).V8H(), GetSrc(Op->Header.Args[0].ID()).V8H(), GetSrc(Op->Header.Args[1].ID()).V8H());
    break;
    }
    case 4: {
      sqadd(GetDst(Node).V4S(), GetSrc(Op->Header.Args[0].ID()).V4S(), GetSrc(Op->Header.Args[1].ID()).V4S());
    break;
    }
    case 8: {
      sqadd(GetDst(Node).V2D(), GetSrc(Op->Header.Args[0].ID()).V2D(), GetSrc(Op->Header.Args[1].ID()).V2D());
    break;
    }
    default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
  }
}

DEF_OP(VSQSub) {
  auto Op = IROp->C<IR::IROp_VSQSub>();
  switch (Op->Header.ElementSize) {
    case 1: {
      sqsub(GetDst(Node).V16B(), GetSrc(Op->Header.Args[0].ID()).V16B(), GetSrc(Op->Header.Args[1].ID()).V16B());
    break;
    }
    case 2: {
      sqsub(GetDst(Node).V8H(), GetSrc(Op->Header.Args[0].ID()).V8H(), GetSrc(Op->Header.Args[1].ID()).V8H());
    break;
    }
    case 4: {
      sqsub(GetDst(Node).V4S(), GetSrc(Op->Header.Args[0].ID()).V4S(), GetSrc(Op->Header.Args[1].ID()).V4S());
    break;
    }
    case 8: {
      sqsub(GetDst(Node).V2D(), GetSrc(Op->Header.Args[0].ID()).V2D(), GetSrc(Op->Header.Args[1].ID()).V2D());
    break;
    }
    default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
  }
}

DEF_OP(VAddP) {
  auto Op = IROp->C<IR::IROp_VAddP>();
  uint8_t OpSize = IROp->Size;

  if (OpSize == 8) {
    switch (Op->Header.ElementSize) {
      case 1: {
        addp(GetDst(Node).V8B(), GetSrc(Op->Header.Args[0].ID()).V8B(), GetSrc(Op->Header.Args[1].ID()).V8B());
        break;
      }
      case 2: {
        addp(GetDst(Node).V4H(), GetSrc(Op->Header.Args[0].ID()).V4H(), GetSrc(Op->Header.Args[1].ID()).V4H());
        break;
      }
      case 4: {
        addp(GetDst(Node).V2S(), GetSrc(Op->Header.Args[0].ID()).V2S(), GetSrc(Op->Header.Args[1].ID()).V2S());
        break;
      }
      default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
    }
  }
  else {
    switch (Op->Header.ElementSize) {
      case 1: {
        addp(GetDst(Node).V16B(), GetSrc(Op->Header.Args[0].ID()).V16B(), GetSrc(Op->Header.Args[1].ID()).V16B());
        break;
      }
      case 2: {
        addp(GetDst(Node).V8H(), GetSrc(Op->Header.Args[0].ID()).V8H(), GetSrc(Op->Header.Args[1].ID()).V8H());
        break;
      }
      case 4: {
        addp(GetDst(Node).V4S(), GetSrc(Op->Header.Args[0].ID()).V4S(), GetSrc(Op->Header.Args[1].ID()).V4S());
        break;
      }
      case 8: {
        addp(GetDst(Node).V2D(), GetSrc(Op->Header.Args[0].ID()).V2D(), GetSrc(Op->Header.Args[1].ID()).V2D());
        break;
      }
      default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
    }
  }
}

DEF_OP(VAddV) {
  auto Op = IROp->C<IR::IROp_VAddV>();
  uint8_t OpSize = IROp->Size;
  uint8_t Elements = OpSize / Op->Header.ElementSize;
  // Vector
  switch (Op->Header.ElementSize) {
    case 1:
    case 2:
    case 4:
      addv(GetDst(Node).VCast(Op->Header.ElementSize * 8, 1), GetSrc(Op->Header.Args[0].ID()).VCast(OpSize * 8, Elements));
      break;
    case 8:
      addp(GetDst(Node).VCast(OpSize * 8, 1), GetSrc(Op->Header.Args[0].ID()).VCast(OpSize * 8, Elements));
      break;
    default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
  }
}

DEF_OP(VURAvg) {
  auto Op = IROp->C<IR::IROp_VURAvg>();
  switch (Op->Header.ElementSize) {
    case 1: {
      urhadd(GetDst(Node).V16B(), GetSrc(Op->Header.Args[0].ID()).V16B(), GetSrc(Op->Header.Args[1].ID()).V16B());
    break;
    }
    case 2: {
      urhadd(GetDst(Node).V8H(), GetSrc(Op->Header.Args[0].ID()).V8H(), GetSrc(Op->Header.Args[1].ID()).V8H());
    break;
    }
    default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
  }
}

DEF_OP(VAbs) {
  auto Op = IROp->C<IR::IROp_VAbs>();
  uint8_t OpSize = IROp->Size;
  uint8_t Elements = OpSize / Op->Header.ElementSize;
  if (Op->Header.ElementSize == OpSize) {
    // Scalar
    switch (Op->Header.ElementSize) {
      case 8: {
        abs(GetDst(Node).D(), GetSrc(Op->Header.Args[0].ID()).D());
      break;
      }
      default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
    }
  }
  else {
    // Vector
    switch (Op->Header.ElementSize) {
      case 1:
      case 2:
      case 4:
      case 8:
        abs(GetDst(Node).VCast(OpSize * 8, Elements), GetSrc(Op->Header.Args[0].ID()).VCast(OpSize * 8, Elements));
        break;
      default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
    }
  }
}

DEF_OP(VFAdd) {
  auto Op = IROp->C<IR::IROp_VFAdd>();
  uint8_t OpSize = IROp->Size;
  if (Op->Header.ElementSize == OpSize) {
    // Scalar
    switch (Op->Header.ElementSize) {
      case 4: {
        fadd(GetDst(Node).S(), GetSrc(Op->Header.Args[0].ID()).S(), GetSrc(Op->Header.Args[1].ID()).S());
      break;
      }
      case 8: {
        fadd(GetDst(Node).D(), GetSrc(Op->Header.Args[0].ID()).D(), GetSrc(Op->Header.Args[1].ID()).D());
      break;
      }
      default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
    }
  }
  else {
    // Vector
    switch (Op->Header.ElementSize) {
      case 4: {
        fadd(GetDst(Node).V4S(), GetSrc(Op->Header.Args[0].ID()).V4S(), GetSrc(Op->Header.Args[1].ID()).V4S());
      break;
      }
      case 8: {
        fadd(GetDst(Node).V2D(), GetSrc(Op->Header.Args[0].ID()).V2D(), GetSrc(Op->Header.Args[1].ID()).V2D());
      break;
      }
      default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
    }
  }
}

DEF_OP(VFAddP) {
  auto Op = IROp->C<IR::IROp_VFAddP>();
  switch (Op->Header.ElementSize) {
    case 4: {
      faddp(GetDst(Node).V4S(), GetSrc(Op->Header.Args[0].ID()).V4S(), GetSrc(Op->Header.Args[1].ID()).V4S());
    break;
    }
    case 8: {
      faddp(GetDst(Node).V2D(), GetSrc(Op->Header.Args[0].ID()).V2D(), GetSrc(Op->Header.Args[1].ID()).V2D());
    break;
    }
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
        fsub(GetDst(Node).S(), GetSrc(Op->Header.Args[0].ID()).S(), GetSrc(Op->Header.Args[1].ID()).S());
      break;
      }
      case 8: {
        fsub(GetDst(Node).D(), GetSrc(Op->Header.Args[0].ID()).D(), GetSrc(Op->Header.Args[1].ID()).D());
      break;
      }
      default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
    }
  }
  else {
    // Vector
    switch (Op->Header.ElementSize) {
      case 4: {
        fsub(GetDst(Node).V4S(), GetSrc(Op->Header.Args[0].ID()).V4S(), GetSrc(Op->Header.Args[1].ID()).V4S());
      break;
      }
      case 8: {
        fsub(GetDst(Node).V2D(), GetSrc(Op->Header.Args[0].ID()).V2D(), GetSrc(Op->Header.Args[1].ID()).V2D());
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
        fmul(GetDst(Node).S(), GetSrc(Op->Header.Args[0].ID()).S(), GetSrc(Op->Header.Args[1].ID()).S());
      break;
      }
      case 8: {
        fmul(GetDst(Node).D(), GetSrc(Op->Header.Args[0].ID()).D(), GetSrc(Op->Header.Args[1].ID()).D());
      break;
      }
      default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
    }
  }
  else {
    // Vector
    switch (Op->Header.ElementSize) {
      case 4: {
        fmul(GetDst(Node).V4S(), GetSrc(Op->Header.Args[0].ID()).V4S(), GetSrc(Op->Header.Args[1].ID()).V4S());
      break;
      }
      case 8: {
        fmul(GetDst(Node).V2D(), GetSrc(Op->Header.Args[0].ID()).V2D(), GetSrc(Op->Header.Args[1].ID()).V2D());
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
        fdiv(GetDst(Node).S(), GetSrc(Op->Header.Args[0].ID()).S(), GetSrc(Op->Header.Args[1].ID()).S());
      break;
      }
      case 8: {
        fdiv(GetDst(Node).D(), GetSrc(Op->Header.Args[0].ID()).D(), GetSrc(Op->Header.Args[1].ID()).D());
      break;
      }
      default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
    }
  }
  else {
    // Vector
    switch (Op->Header.ElementSize) {
      case 4: {
        fdiv(GetDst(Node).V4S(), GetSrc(Op->Header.Args[0].ID()).V4S(), GetSrc(Op->Header.Args[1].ID()).V4S());
      break;
      }
      case 8: {
        fdiv(GetDst(Node).V2D(), GetSrc(Op->Header.Args[0].ID()).V2D(), GetSrc(Op->Header.Args[1].ID()).V2D());
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
        fmin(GetDst(Node).S(), GetSrc(Op->Header.Args[0].ID()).S(), GetSrc(Op->Header.Args[1].ID()).S());
      break;
      }
      case 8: {
        fmin(GetDst(Node).D(), GetSrc(Op->Header.Args[0].ID()).D(), GetSrc(Op->Header.Args[1].ID()).D());
      break;
      }
      default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
    }
  }
  else {
    // Vector
    switch (Op->Header.ElementSize) {
      case 4: {
        fmin(GetDst(Node).V4S(), GetSrc(Op->Header.Args[0].ID()).V4S(), GetSrc(Op->Header.Args[1].ID()).V4S());
      break;
      }
      case 8: {
        fmin(GetDst(Node).V2D(), GetSrc(Op->Header.Args[0].ID()).V2D(), GetSrc(Op->Header.Args[1].ID()).V2D());
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
        fmax(GetDst(Node).S(), GetSrc(Op->Header.Args[0].ID()).S(), GetSrc(Op->Header.Args[1].ID()).S());
      break;
      }
      case 8: {
        fmax(GetDst(Node).D(), GetSrc(Op->Header.Args[0].ID()).D(), GetSrc(Op->Header.Args[1].ID()).D());
      break;
      }
      default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
    }
  }
  else {
    // Vector
    switch (Op->Header.ElementSize) {
      case 4: {
        fmax(GetDst(Node).V4S(), GetSrc(Op->Header.Args[0].ID()).V4S(), GetSrc(Op->Header.Args[1].ID()).V4S());
      break;
      }
      case 8: {
        fmax(GetDst(Node).V2D(), GetSrc(Op->Header.Args[0].ID()).V2D(), GetSrc(Op->Header.Args[1].ID()).V2D());
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
        fmov(VTMP1.S(), 1.0f);
        fdiv(GetDst(Node).S(), VTMP1.S(), GetSrc(Op->Header.Args[0].ID()).S());
      break;
      }
      case 8: {
        fmov(VTMP1.D(), 1.0);
        fdiv(GetDst(Node).D(), VTMP1.D(), GetSrc(Op->Header.Args[0].ID()).D());
      break;
      }
      default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
    }
  }
  else {
    // Vector
    switch (Op->Header.ElementSize) {
      case 4: {
        fmov(VTMP1.V4S(), 1.0f);
        fdiv(GetDst(Node).V4S(), VTMP1.V4S(), GetSrc(Op->Header.Args[0].ID()).V4S());
      break;
      }
      case 8: {
        fmov(VTMP1.V2D(), 1.0);
        fdiv(GetDst(Node).V2D(), VTMP1.V2D(), GetSrc(Op->Header.Args[0].ID()).V2D());
      break;
      }
      default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
    }
  }
}

DEF_OP(VFSqrt) {
  auto Op = IROp->C<IR::IROp_VFRSqrt>();
  uint8_t OpSize = IROp->Size;
  if (Op->Header.ElementSize == OpSize) {
    // Scalar
    switch (Op->Header.ElementSize) {
      case 4: {
        fsqrt(GetDst(Node).S(), GetSrc(Op->Header.Args[0].ID()).S());
      break;
      }
      case 8: {
        fsqrt(GetDst(Node).D(), GetSrc(Op->Header.Args[0].ID()).D());
      break;
      }
      default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
    }
  }
  else {
    // Vector
    switch (Op->Header.ElementSize) {
      case 4: {
        fsqrt(GetDst(Node).V4S(), GetSrc(Op->Header.Args[0].ID()).V4S());
      break;
      }
      case 8: {
        fsqrt(GetDst(Node).V2D(), GetSrc(Op->Header.Args[0].ID()).V2D());
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
        fmov(VTMP1.S(), 1.0f);
        fsqrt(VTMP2.S(), GetSrc(Op->Header.Args[0].ID()).S());
        fdiv(GetDst(Node).S(), VTMP1.S(), VTMP2.S());
      break;
      }
      case 8: {
        fmov(VTMP1.D(), 1.0);
        fsqrt(VTMP2.D(), GetSrc(Op->Header.Args[0].ID()).D());
        fdiv(GetDst(Node).D(), VTMP1.D(), VTMP2.D());
      break;
      }
      default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
    }
  }
  else {
    // Vector
    switch (Op->Header.ElementSize) {
      case 4: {
        fmov(VTMP1.V4S(), 1.0f);
        fsqrt(VTMP2.V4S(), GetSrc(Op->Header.Args[0].ID()).V4S());
        fdiv(GetDst(Node).V4S(), VTMP1.V4S(), VTMP2.V4S());
      break;
      }
      case 8: {
        fmov(VTMP1.V2D(), 1.0);
        fsqrt(VTMP2.V2D(), GetSrc(Op->Header.Args[0].ID()).V2D());
        fdiv(GetDst(Node).V2D(), VTMP1.V2D(), VTMP2.V2D());
      break;
      }
      default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
    }
  }
}

DEF_OP(VNeg) {
  auto Op = IROp->C<IR::IROp_VNeg>();
  uint8_t OpSize = IROp->Size;
  switch (Op->Header.ElementSize) {
  case 1:
    neg(GetDst(Node).V16B(), GetSrc(Op->Header.Args[0].ID()).V16B());
    break;
  case 2:
    neg(GetDst(Node).V8H(), GetSrc(Op->Header.Args[0].ID()).V8H());
    break;
  case 4:
    neg(GetDst(Node).V4S(), GetSrc(Op->Header.Args[0].ID()).V4S());
    break;
  case 8:
    neg(GetDst(Node).V2D(), GetSrc(Op->Header.Args[0].ID()).V2D());
    break;
  default: LogMan::Msg::A("Unsupported Not size: %d", OpSize);
  }
}

DEF_OP(VFNeg) {
  auto Op = IROp->C<IR::IROp_VFNeg>();
  uint8_t OpSize = IROp->Size;
  switch (Op->Header.ElementSize) {
  case 4:
    fneg(GetDst(Node).V4S(), GetSrc(Op->Header.Args[0].ID()).V4S());
    break;
  case 8:
    fneg(GetDst(Node).V2D(), GetSrc(Op->Header.Args[0].ID()).V2D());
    break;
  default: LogMan::Msg::A("Unsupported Not size: %d", OpSize);
  }
}

DEF_OP(VNot) {
  auto Op = IROp->C<IR::IROp_VNot>();
  mvn(GetDst(Node).V16B(), GetSrc(Op->Header.Args[0].ID()).V16B());
}

DEF_OP(VUMin) {
  auto Op = IROp->C<IR::IROp_VUMin>();
  switch (Op->Header.ElementSize) {
    case 1: {
      umin(GetDst(Node).V16B(), GetSrc(Op->Header.Args[0].ID()).V16B(), GetSrc(Op->Header.Args[1].ID()).V16B());
    break;
    }
    case 2: {
      umin(GetDst(Node).V8H(), GetSrc(Op->Header.Args[0].ID()).V8H(), GetSrc(Op->Header.Args[1].ID()).V8H());
    break;
    }
    case 4: {
      umin(GetDst(Node).V4S(), GetSrc(Op->Header.Args[0].ID()).V4S(), GetSrc(Op->Header.Args[1].ID()).V4S());
    break;
    }
    case 8: {
      umin(GetDst(Node).V2D(), GetSrc(Op->Header.Args[0].ID()).V2D(), GetSrc(Op->Header.Args[1].ID()).V2D());
    break;
    }
    default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
  }
}

DEF_OP(VSMin) {
  auto Op = IROp->C<IR::IROp_VSMin>();
  switch (Op->Header.ElementSize) {
    case 1: {
      smin(GetDst(Node).V16B(), GetSrc(Op->Header.Args[0].ID()).V16B(), GetSrc(Op->Header.Args[1].ID()).V16B());
    break;
    }
    case 2: {
      smin(GetDst(Node).V8H(), GetSrc(Op->Header.Args[0].ID()).V8H(), GetSrc(Op->Header.Args[1].ID()).V8H());
    break;
    }
    case 4: {
      smin(GetDst(Node).V4S(), GetSrc(Op->Header.Args[0].ID()).V4S(), GetSrc(Op->Header.Args[1].ID()).V4S());
    break;
    }
    case 8: {
      smin(GetDst(Node).V2D(), GetSrc(Op->Header.Args[0].ID()).V2D(), GetSrc(Op->Header.Args[1].ID()).V2D());
    break;
    }
    default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
  }
}

DEF_OP(VUMax) {
  auto Op = IROp->C<IR::IROp_VUMax>();
  switch (Op->Header.ElementSize) {
    case 1: {
      umax(GetDst(Node).V16B(), GetSrc(Op->Header.Args[0].ID()).V16B(), GetSrc(Op->Header.Args[1].ID()).V16B());
    break;
    }
    case 2: {
      umax(GetDst(Node).V8H(), GetSrc(Op->Header.Args[0].ID()).V8H(), GetSrc(Op->Header.Args[1].ID()).V8H());
    break;
    }
    case 4: {
      umax(GetDst(Node).V4S(), GetSrc(Op->Header.Args[0].ID()).V4S(), GetSrc(Op->Header.Args[1].ID()).V4S());
    break;
    }
    case 8: {
      umax(GetDst(Node).V2D(), GetSrc(Op->Header.Args[0].ID()).V2D(), GetSrc(Op->Header.Args[1].ID()).V2D());
    break;
    }
    default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
  }
}

DEF_OP(VSMax) {
  auto Op = IROp->C<IR::IROp_VSMax>();
  switch (Op->Header.ElementSize) {
    case 1: {
      smax(GetDst(Node).V16B(), GetSrc(Op->Header.Args[0].ID()).V16B(), GetSrc(Op->Header.Args[1].ID()).V16B());
    break;
    }
    case 2: {
      smax(GetDst(Node).V8H(), GetSrc(Op->Header.Args[0].ID()).V8H(), GetSrc(Op->Header.Args[1].ID()).V8H());
    break;
    }
    case 4: {
      smax(GetDst(Node).V4S(), GetSrc(Op->Header.Args[0].ID()).V4S(), GetSrc(Op->Header.Args[1].ID()).V4S());
    break;
    }
    case 8: {
      smax(GetDst(Node).V2D(), GetSrc(Op->Header.Args[0].ID()).V2D(), GetSrc(Op->Header.Args[1].ID()).V2D());
    break;
    }
    default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
  }
}

DEF_OP(VZip) {
  auto Op = IROp->C<IR::IROp_VZip>();
  uint8_t OpSize = IROp->Size;
  if (OpSize == 8) {
    switch (Op->Header.ElementSize) {
      case 1: {
        zip1(GetDst(Node).V8B(), GetSrc(Op->Header.Args[0].ID()).V8B(), GetSrc(Op->Header.Args[1].ID()).V8B());
      break;
      }
      case 2: {
        zip1(GetDst(Node).V4H(), GetSrc(Op->Header.Args[0].ID()).V4H(), GetSrc(Op->Header.Args[1].ID()).V4H());
      break;
      }
      case 4: {
        zip1(GetDst(Node).V2S(), GetSrc(Op->Header.Args[0].ID()).V2S(), GetSrc(Op->Header.Args[1].ID()).V2S());
      break;
      }
      default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
    }
  }
  else {
    switch (Op->Header.ElementSize) {
      case 1: {
        zip1(GetDst(Node).V16B(), GetSrc(Op->Header.Args[0].ID()).V16B(), GetSrc(Op->Header.Args[1].ID()).V16B());
      break;
      }
      case 2: {
        zip1(GetDst(Node).V8H(), GetSrc(Op->Header.Args[0].ID()).V8H(), GetSrc(Op->Header.Args[1].ID()).V8H());
      break;
      }
      case 4: {
        zip1(GetDst(Node).V4S(), GetSrc(Op->Header.Args[0].ID()).V4S(), GetSrc(Op->Header.Args[1].ID()).V4S());
      break;
      }
      case 8: {
        zip1(GetDst(Node).V2D(), GetSrc(Op->Header.Args[0].ID()).V2D(), GetSrc(Op->Header.Args[1].ID()).V2D());
      break;
      }
      default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
    }
  }
}

DEF_OP(VZip2) {
  auto Op = IROp->C<IR::IROp_VZip2>();
  uint8_t OpSize = IROp->Size;
  if (OpSize == 8) {
    switch (Op->Header.ElementSize) {
    case 1: {
      zip2(GetDst(Node).V8B(), GetSrc(Op->Header.Args[0].ID()).V8B(), GetSrc(Op->Header.Args[1].ID()).V8B());
    break;
    }
    case 2: {
      zip2(GetDst(Node).V4H(), GetSrc(Op->Header.Args[0].ID()).V4H(), GetSrc(Op->Header.Args[1].ID()).V4H());
    break;
    }
    case 4: {
      zip2(GetDst(Node).V2S(), GetSrc(Op->Header.Args[0].ID()).V2S(), GetSrc(Op->Header.Args[1].ID()).V2S());
    break;
    }
    default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
    }
  }
  else {
    switch (Op->Header.ElementSize) {
    case 1: {
      zip2(GetDst(Node).V16B(), GetSrc(Op->Header.Args[0].ID()).V16B(), GetSrc(Op->Header.Args[1].ID()).V16B());
    break;
    }
    case 2: {
      zip2(GetDst(Node).V8H(), GetSrc(Op->Header.Args[0].ID()).V8H(), GetSrc(Op->Header.Args[1].ID()).V8H());
    break;
    }
    case 4: {
      zip2(GetDst(Node).V4S(), GetSrc(Op->Header.Args[0].ID()).V4S(), GetSrc(Op->Header.Args[1].ID()).V4S());
    break;
    }
    case 8: {
      zip2(GetDst(Node).V2D(), GetSrc(Op->Header.Args[0].ID()).V2D(), GetSrc(Op->Header.Args[1].ID()).V2D());
    break;
    }
    default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
    }
  }
}

DEF_OP(VBSL) {
  LogMan::Msg::A("Unimplemented");
}

DEF_OP(VCMPEQ) {
  auto Op = IROp->C<IR::IROp_VCMPEQ>();
  uint8_t OpSize = IROp->Size;
  if (Op->Header.ElementSize == OpSize) {
    // Scalar
    switch (Op->Header.ElementSize) {
      case 4: {
        cmeq(GetDst(Node).S(), GetSrc(Op->Header.Args[0].ID()).S(), GetSrc(Op->Header.Args[1].ID()).S());
      break;
      }
      case 8: {
        cmeq(GetDst(Node).D(), GetSrc(Op->Header.Args[0].ID()).D(), GetSrc(Op->Header.Args[1].ID()).D());
      break;
    }
    default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
    }
  }
  else {
    // Vector
    switch (Op->Header.ElementSize) {
      case 1: {
        cmeq(GetDst(Node).V16B(), GetSrc(Op->Header.Args[0].ID()).V16B(), GetSrc(Op->Header.Args[1].ID()).V16B());
      break;
      }
      case 2: {
        cmeq(GetDst(Node).V8H(), GetSrc(Op->Header.Args[0].ID()).V8H(), GetSrc(Op->Header.Args[1].ID()).V8H());
      break;
      }
      case 4: {
        cmeq(GetDst(Node).V4S(), GetSrc(Op->Header.Args[0].ID()).V4S(), GetSrc(Op->Header.Args[1].ID()).V4S());
      break;
      }
      case 8: {
        cmeq(GetDst(Node).V2D(), GetSrc(Op->Header.Args[0].ID()).V2D(), GetSrc(Op->Header.Args[1].ID()).V2D());
      break;
    }
    default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
    }
  }
}

DEF_OP(VCMPEQZ) {
  auto Op = IROp->C<IR::IROp_VCMPEQZ>();
  uint8_t OpSize = IROp->Size;
  if (Op->Header.ElementSize == OpSize) {
    // Scalar
    switch (Op->Header.ElementSize) {
      case 4: {
        cmeq(GetDst(Node).S(), GetSrc(Op->Header.Args[0].ID()).S(), 0);
      break;
      }
      case 8: {
        cmeq(GetDst(Node).D(), GetSrc(Op->Header.Args[0].ID()).D(), 0);
      break;
    }
    default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
    }
  }
  else {
    // Vector
    switch (Op->Header.ElementSize) {
      case 1: {
        cmeq(GetDst(Node).V16B(), GetSrc(Op->Header.Args[0].ID()).V16B(), 0);
      break;
      }
      case 2: {
        cmeq(GetDst(Node).V8H(), GetSrc(Op->Header.Args[0].ID()).V8H(), 0);
      break;
      }
      case 4: {
        cmeq(GetDst(Node).V4S(), GetSrc(Op->Header.Args[0].ID()).V4S(), 0);
      break;
      }
      case 8: {
        cmeq(GetDst(Node).V2D(), GetSrc(Op->Header.Args[0].ID()).V2D(), 0);
      break;
    }
    default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
    }
  }
}

DEF_OP(VCMPGT) {
  auto Op = IROp->C<IR::IROp_VCMPGT>();
  uint8_t OpSize = IROp->Size;
  if (Op->Header.ElementSize == OpSize) {
    // Scalar
    switch (Op->Header.ElementSize) {
      case 4: {
        cmgt(GetDst(Node).S(), GetSrc(Op->Header.Args[0].ID()).S(), GetSrc(Op->Header.Args[1].ID()).S());
      break;
      }
      case 8: {
        cmgt(GetDst(Node).D(), GetSrc(Op->Header.Args[0].ID()).D(), GetSrc(Op->Header.Args[1].ID()).D());
      break;
      }
      default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
    }
  }
  else {
    // Vector
    switch (Op->Header.ElementSize) {
      case 1: {
        cmgt(GetDst(Node).V16B(), GetSrc(Op->Header.Args[0].ID()).V16B(), GetSrc(Op->Header.Args[1].ID()).V16B());
      break;
      }
      case 2: {
        cmgt(GetDst(Node).V8H(), GetSrc(Op->Header.Args[0].ID()).V8H(), GetSrc(Op->Header.Args[1].ID()).V8H());
      break;
      }
      case 4: {
        cmgt(GetDst(Node).V4S(), GetSrc(Op->Header.Args[0].ID()).V4S(), GetSrc(Op->Header.Args[1].ID()).V4S());
      break;
      }
      case 8: {
        cmgt(GetDst(Node).V2D(), GetSrc(Op->Header.Args[0].ID()).V2D(), GetSrc(Op->Header.Args[1].ID()).V2D());
      break;
      }
      default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
    }
  }
}

DEF_OP(VCMPGTZ) {
  auto Op = IROp->C<IR::IROp_VCMPGTZ>();
  uint8_t OpSize = IROp->Size;
  if (Op->Header.ElementSize == OpSize) {
    // Scalar
    switch (Op->Header.ElementSize) {
      case 4: {
        cmgt(GetDst(Node).S(), GetSrc(Op->Header.Args[0].ID()).S(), 0);
      break;
      }
      case 8: {
        cmgt(GetDst(Node).D(), GetSrc(Op->Header.Args[0].ID()).D(), 0);
      break;
      }
      default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
    }
  }
  else {
    // Vector
    switch (Op->Header.ElementSize) {
      case 1: {
        cmgt(GetDst(Node).V16B(), GetSrc(Op->Header.Args[0].ID()).V16B(), 0);
      break;
      }
      case 2: {
        cmgt(GetDst(Node).V8H(), GetSrc(Op->Header.Args[0].ID()).V8H(), 0);
      break;
      }
      case 4: {
        cmgt(GetDst(Node).V4S(), GetSrc(Op->Header.Args[0].ID()).V4S(), 0);
      break;
      }
      case 8: {
        cmgt(GetDst(Node).V2D(), GetSrc(Op->Header.Args[0].ID()).V2D(), 0);
      break;
      }
      default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
    }
  }
}

DEF_OP(VCMPLTZ) {
  auto Op = IROp->C<IR::IROp_VCMPLTZ>();
  uint8_t OpSize = IROp->Size;
  if (Op->Header.ElementSize == OpSize) {
    // Scalar
    switch (Op->Header.ElementSize) {
      case 4: {
        cmlt(GetDst(Node).S(), GetSrc(Op->Header.Args[0].ID()).S(), 0);
      break;
      }
      case 8: {
        cmlt(GetDst(Node).D(), GetSrc(Op->Header.Args[0].ID()).D(), 0);
      break;
      }
      default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
    }
  }
  else {
    // Vector
    switch (Op->Header.ElementSize) {
      case 1: {
        cmlt(GetDst(Node).V16B(), GetSrc(Op->Header.Args[0].ID()).V16B(), 0);
      break;
      }
      case 2: {
        cmlt(GetDst(Node).V8H(), GetSrc(Op->Header.Args[0].ID()).V8H(), 0);
      break;
      }
      case 4: {
        cmlt(GetDst(Node).V4S(), GetSrc(Op->Header.Args[0].ID()).V4S(), 0);
      break;
      }
      case 8: {
        cmlt(GetDst(Node).V2D(), GetSrc(Op->Header.Args[0].ID()).V2D(), 0);
      break;
      }
      default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
    }
  }
}

DEF_OP(VFCMPEQ) {
  auto Op = IROp->C<IR::IROp_VFCMPEQ>();
  uint8_t OpSize = IROp->Size;
  if (Op->Header.ElementSize == OpSize) {
    // Scalar
    switch (Op->Header.ElementSize) {
      case 4: {
        fcmeq(GetDst(Node).S(), GetSrc(Op->Header.Args[0].ID()).S(), GetSrc(Op->Header.Args[1].ID()).S());
      break;
      }
      case 8: {
        fcmeq(GetDst(Node).D(), GetSrc(Op->Header.Args[0].ID()).D(), GetSrc(Op->Header.Args[1].ID()).D());
      break;
      }
      default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
    }
  }
  else {
    // Vector
    switch (Op->Header.ElementSize) {
      case 2: {
        fcmeq(GetDst(Node).V8H(), GetSrc(Op->Header.Args[0].ID()).V8H(), GetSrc(Op->Header.Args[1].ID()).V8H());
      break;
      }
      case 4: {
        fcmeq(GetDst(Node).V4S(), GetSrc(Op->Header.Args[0].ID()).V4S(), GetSrc(Op->Header.Args[1].ID()).V4S());
      break;
      }
      case 8: {
        fcmeq(GetDst(Node).V2D(), GetSrc(Op->Header.Args[0].ID()).V2D(), GetSrc(Op->Header.Args[1].ID()).V2D());
      break;
      }
      default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
    }
  }
}

DEF_OP(VFCMPNEQ) {
  auto Op = IROp->C<IR::IROp_VFCMPNEQ>();
  uint8_t OpSize = IROp->Size;
  if (Op->Header.ElementSize == OpSize) {
    // Scalar
    switch (Op->Header.ElementSize) {
      case 4: {
        fcmeq(GetDst(Node).S(), GetSrc(Op->Header.Args[0].ID()).S(), GetSrc(Op->Header.Args[1].ID()).S());
      break;
      }
      case 8: {
        fcmeq(GetDst(Node).D(), GetSrc(Op->Header.Args[0].ID()).D(), GetSrc(Op->Header.Args[1].ID()).D());
      break;
      }
      default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
    }
    mvn(GetDst(Node).V8B(), GetDst(Node).V8B());
  }
  else {
    // Vector
    switch (Op->Header.ElementSize) {
      case 2: {
        fcmeq(GetDst(Node).V8H(), GetSrc(Op->Header.Args[0].ID()).V8H(), GetSrc(Op->Header.Args[1].ID()).V8H());
      break;
      }
      case 4: {
        fcmeq(GetDst(Node).V4S(), GetSrc(Op->Header.Args[0].ID()).V4S(), GetSrc(Op->Header.Args[1].ID()).V4S());
      break;
      }
      case 8: {
        fcmeq(GetDst(Node).V2D(), GetSrc(Op->Header.Args[0].ID()).V2D(), GetSrc(Op->Header.Args[1].ID()).V2D());
      break;
      }
      default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
    }
    mvn(GetDst(Node).V16B(), GetDst(Node).V16B());
  }
}

DEF_OP(VFCMPLT) {
  auto Op = IROp->C<IR::IROp_VFCMPLT>();
  uint8_t OpSize = IROp->Size;
  if (Op->Header.ElementSize == OpSize) {
    // Scalar
    switch (Op->Header.ElementSize) {
      case 4: {
        fcmgt(GetDst(Node).S(), GetSrc(Op->Header.Args[1].ID()).S(), GetSrc(Op->Header.Args[0].ID()).S());
      break;
      }
      case 8: {
        fcmgt(GetDst(Node).D(), GetSrc(Op->Header.Args[1].ID()).D(), GetSrc(Op->Header.Args[0].ID()).D());
      break;
      }
      default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
    }
  }
  else {
    // Vector
    switch (Op->Header.ElementSize) {
      case 2: {
        fcmgt(GetDst(Node).V8H(), GetSrc(Op->Header.Args[1].ID()).V8H(), GetSrc(Op->Header.Args[0].ID()).V8H());
      break;
      }
      case 4: {
        fcmgt(GetDst(Node).V4S(), GetSrc(Op->Header.Args[1].ID()).V4S(), GetSrc(Op->Header.Args[0].ID()).V4S());
      break;
      }
      case 8: {
        fcmgt(GetDst(Node).V2D(), GetSrc(Op->Header.Args[1].ID()).V2D(), GetSrc(Op->Header.Args[0].ID()).V2D());
      break;
      }
      default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
    }
  }
}

DEF_OP(VFCMPGT) {
  auto Op = IROp->C<IR::IROp_VFCMPGT>();
  uint8_t OpSize = IROp->Size;
  if (Op->Header.ElementSize == OpSize) {
    // Scalar
    switch (Op->Header.ElementSize) {
      case 4: {
        fcmgt(GetDst(Node).S(), GetSrc(Op->Header.Args[0].ID()).S(), GetSrc(Op->Header.Args[1].ID()).S());
      break;
      }
      case 8: {
        fcmgt(GetDst(Node).D(), GetSrc(Op->Header.Args[0].ID()).D(), GetSrc(Op->Header.Args[1].ID()).D());
      break;
      }
      default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
    }
  }
  else {
    // Vector
    switch (Op->Header.ElementSize) {
      case 2: {
        fcmgt(GetDst(Node).V8H(), GetSrc(Op->Header.Args[0].ID()).V8H(), GetSrc(Op->Header.Args[1].ID()).V8H());
      break;
      }
      case 4: {
        fcmgt(GetDst(Node).V4S(), GetSrc(Op->Header.Args[0].ID()).V4S(), GetSrc(Op->Header.Args[1].ID()).V4S());
      break;
      }
      case 8: {
        fcmgt(GetDst(Node).V2D(), GetSrc(Op->Header.Args[0].ID()).V2D(), GetSrc(Op->Header.Args[1].ID()).V2D());
      break;
      }
      default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
    }
  }
}

DEF_OP(VFCMPLE) {
  auto Op = IROp->C<IR::IROp_VFCMPLE>();
  uint8_t OpSize = IROp->Size;
  if (Op->Header.ElementSize == OpSize) {
    // Scalar
    switch (Op->Header.ElementSize) {
      case 4: {
        fcmge(GetDst(Node).S(), GetSrc(Op->Header.Args[1].ID()).S(), GetSrc(Op->Header.Args[0].ID()).S());
      break;
      }
      case 8: {
        fcmge(GetDst(Node).D(), GetSrc(Op->Header.Args[1].ID()).D(), GetSrc(Op->Header.Args[0].ID()).D());
      break;
      }
      default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
    }
  }
  else {
    // Vector
    switch (Op->Header.ElementSize) {
      case 2: {
        fcmge(GetDst(Node).V8H(), GetSrc(Op->Header.Args[1].ID()).V8H(), GetSrc(Op->Header.Args[0].ID()).V8H());
      break;
      }
      case 4: {
        fcmge(GetDst(Node).V4S(), GetSrc(Op->Header.Args[1].ID()).V4S(), GetSrc(Op->Header.Args[0].ID()).V4S());
      break;
      }
      case 8: {
        fcmge(GetDst(Node).V2D(), GetSrc(Op->Header.Args[1].ID()).V2D(), GetSrc(Op->Header.Args[0].ID()).V2D());
      break;
      }
      default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
    }
  }
}

DEF_OP(VFCMPORD) {
  auto Op = IROp->C<IR::IROp_VFCMPORD>();
  uint8_t OpSize = IROp->Size;
  if (Op->Header.ElementSize == OpSize) {
    // Scalar
    switch (Op->Header.ElementSize) {
      case 4: {
        fcmge(VTMP1.S(), GetSrc(Op->Header.Args[0].ID()).S(), GetSrc(Op->Header.Args[1].ID()).S());
        fcmgt(VTMP2.S(), GetSrc(Op->Header.Args[1].ID()).S(), GetSrc(Op->Header.Args[0].ID()).S());
        orr(GetDst(Node).V8B(), VTMP1.V8B(), VTMP2.V8B());
      break;
      }
      case 8: {
        fcmge(VTMP1.D(), GetSrc(Op->Header.Args[0].ID()).D(), GetSrc(Op->Header.Args[1].ID()).D());
        fcmgt(VTMP2.D(), GetSrc(Op->Header.Args[1].ID()).D(), GetSrc(Op->Header.Args[0].ID()).D());
        orr(GetDst(Node).V8B(), VTMP1.V8B(), VTMP2.V8B());
      break;
      }
      default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
    }
  }
  else {
    // Vector
    switch (Op->Header.ElementSize) {
      case 2: {
        fcmge(VTMP1.V8H(), GetSrc(Op->Header.Args[0].ID()).V8H(), GetSrc(Op->Header.Args[1].ID()).V8H());
        fcmgt(VTMP2.V8H(), GetSrc(Op->Header.Args[1].ID()).V8H(), GetSrc(Op->Header.Args[0].ID()).V8H());
        orr(GetDst(Node).V16B(), VTMP1.V16B(), VTMP2.V16B());
      break;
      }
      case 4: {
        fcmge(VTMP1.V4S(), GetSrc(Op->Header.Args[0].ID()).V4S(), GetSrc(Op->Header.Args[1].ID()).V4S());
        fcmgt(VTMP2.V4S(), GetSrc(Op->Header.Args[1].ID()).V4S(), GetSrc(Op->Header.Args[0].ID()).V4S());
        orr(GetDst(Node).V16B(), VTMP1.V16B(), VTMP2.V16B());
      break;
      }
      case 8: {
        fcmge(VTMP1.V2D(), GetSrc(Op->Header.Args[0].ID()).V2D(), GetSrc(Op->Header.Args[1].ID()).V2D());
        fcmgt(VTMP2.V2D(), GetSrc(Op->Header.Args[1].ID()).V2D(), GetSrc(Op->Header.Args[0].ID()).V2D());
        orr(GetDst(Node).V16B(), VTMP1.V16B(), VTMP2.V16B());
      break;
      }
      default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
    }
  }
}

DEF_OP(VFCMPUNO) {
  auto Op = IROp->C<IR::IROp_VFCMPUNO>();
  uint8_t OpSize = IROp->Size;
  if (Op->Header.ElementSize == OpSize) {
    // Scalar
    switch (Op->Header.ElementSize) {
      case 4: {
        fcmge(VTMP1.S(), GetSrc(Op->Header.Args[0].ID()).S(), GetSrc(Op->Header.Args[1].ID()).S());
        fcmgt(VTMP2.S(), GetSrc(Op->Header.Args[1].ID()).S(), GetSrc(Op->Header.Args[0].ID()).S());
        orr(GetDst(Node).V8B(), VTMP1.V8B(), VTMP2.V8B());
        mvn(GetDst(Node).V8B(), GetDst(Node).V8B());
      break;
      }
      case 8: {
        fcmge(VTMP1.D(), GetSrc(Op->Header.Args[0].ID()).D(), GetSrc(Op->Header.Args[1].ID()).D());
        fcmgt(VTMP2.D(), GetSrc(Op->Header.Args[1].ID()).D(), GetSrc(Op->Header.Args[0].ID()).D());
        orr(GetDst(Node).V8B(), VTMP1.V8B(), VTMP2.V8B());
        mvn(GetDst(Node).V8B(), GetDst(Node).V8B());
      break;
      }
      default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
    }
  }
  else {
    // Vector
    switch (Op->Header.ElementSize) {
      case 2: {
        fcmge(VTMP1.V8H(), GetSrc(Op->Header.Args[0].ID()).V8H(), GetSrc(Op->Header.Args[1].ID()).V8H());
        fcmgt(VTMP2.V8H(), GetSrc(Op->Header.Args[1].ID()).V8H(), GetSrc(Op->Header.Args[0].ID()).V8H());
        orr(GetDst(Node).V16B(), VTMP1.V16B(), VTMP2.V16B());
        mvn(GetDst(Node).V16B(), GetDst(Node).V16B());
      break;
      }
      case 4: {
        fcmge(VTMP1.V4S(), GetSrc(Op->Header.Args[0].ID()).V4S(), GetSrc(Op->Header.Args[1].ID()).V4S());
        fcmgt(VTMP2.V4S(), GetSrc(Op->Header.Args[1].ID()).V4S(), GetSrc(Op->Header.Args[0].ID()).V4S());
        orr(GetDst(Node).V16B(), VTMP1.V16B(), VTMP2.V16B());
        mvn(GetDst(Node).V16B(), GetDst(Node).V16B());
      break;
      }
      case 8: {
        fcmge(VTMP1.V2D(), GetSrc(Op->Header.Args[0].ID()).V2D(), GetSrc(Op->Header.Args[1].ID()).V2D());
        fcmgt(VTMP2.V2D(), GetSrc(Op->Header.Args[1].ID()).V2D(), GetSrc(Op->Header.Args[0].ID()).V2D());
        orr(GetDst(Node).V16B(), VTMP1.V16B(), VTMP2.V16B());
        mvn(GetDst(Node).V16B(), GetDst(Node).V16B());
      break;
      }
      default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
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
    case 1: {
      dup(VTMP1.V16B(), GetSrc(Op->Header.Args[1].ID()).V16B(), 0);
      ushl(GetDst(Node).V16B(), GetSrc(Op->Header.Args[0].ID()).V16B(), VTMP1.V16B());
    break;
    }
    case 2: {
      dup(VTMP1.V8H(), GetSrc(Op->Header.Args[1].ID()).V8H(), 0);
      ushl(GetDst(Node).V8H(), GetSrc(Op->Header.Args[0].ID()).V8H(), VTMP1.V8H());
    break;
    }
    case 4: {
      dup(VTMP1.V4S(), GetSrc(Op->Header.Args[1].ID()).V4S(), 0);
      ushl(GetDst(Node).V4S(), GetSrc(Op->Header.Args[0].ID()).V4S(), VTMP1.V4S());
    break;
    }
    case 8: {
      dup(VTMP1.V2D(), GetSrc(Op->Header.Args[1].ID()).V2D(), 0);
      ushl(GetDst(Node).V2D(), GetSrc(Op->Header.Args[0].ID()).V2D(), VTMP1.V2D());
    break;
    }
    default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
  }
}

DEF_OP(VUShrS) {
  auto Op = IROp->C<IR::IROp_VUShrS>();

  switch (Op->Header.ElementSize) {
    case 1: {
      dup(VTMP1.V16B(), GetSrc(Op->Header.Args[1].ID()).V16B(), 0);
      neg(VTMP1.V16B(), VTMP1.V16B());
      ushl(GetDst(Node).V16B(), GetSrc(Op->Header.Args[0].ID()).V16B(), VTMP1.V16B());
    break;
    }
    case 2: {
      dup(VTMP1.V8H(), GetSrc(Op->Header.Args[1].ID()).V8H(), 0);
      neg(VTMP1.V8H(), VTMP1.V8H());
      ushl(GetDst(Node).V8H(), GetSrc(Op->Header.Args[0].ID()).V8H(), VTMP1.V8H());
    break;
    }
    case 4: {
      dup(VTMP1.V4S(), GetSrc(Op->Header.Args[1].ID()).V4S(), 0);
      neg(VTMP1.V4S(), VTMP1.V4S());
      ushl(GetDst(Node).V4S(), GetSrc(Op->Header.Args[0].ID()).V4S(), VTMP1.V4S());
    break;
    }
    case 8: {
      dup(VTMP1.V2D(), GetSrc(Op->Header.Args[1].ID()).V2D(), 0);
      neg(VTMP1.V2D(), VTMP1.V2D());
      ushl(GetDst(Node).V2D(), GetSrc(Op->Header.Args[0].ID()).V2D(), VTMP1.V2D());
    break;
    }
    default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
  }
}

DEF_OP(VSShrS) {
  auto Op = IROp->C<IR::IROp_VSShrS>();

  switch (Op->Header.ElementSize) {
    case 1: {
      dup(VTMP1.V16B(), GetSrc(Op->Header.Args[1].ID()).V16B(), 0);
      neg(VTMP1.V16B(), VTMP1.V16B());
      sshl(GetDst(Node).V16B(), GetSrc(Op->Header.Args[0].ID()).V16B(), VTMP1.V16B());
    break;
    }
    case 2: {
      dup(VTMP1.V8H(), GetSrc(Op->Header.Args[1].ID()).V8H(), 0);
      neg(VTMP1.V8H(), VTMP1.V8H());
      sshl(GetDst(Node).V8H(), GetSrc(Op->Header.Args[0].ID()).V8H(), VTMP1.V8H());
    break;
    }
    case 4: {
      dup(VTMP1.V4S(), GetSrc(Op->Header.Args[1].ID()).V4S(), 0);
      neg(VTMP1.V4S(), VTMP1.V4S());
      sshl(GetDst(Node).V4S(), GetSrc(Op->Header.Args[0].ID()).V4S(), VTMP1.V4S());
    break;
    }
    case 8: {
      dup(VTMP1.V2D(), GetSrc(Op->Header.Args[1].ID()).V2D(), 0);
      neg(VTMP1.V2D(), VTMP1.V2D());
      sshl(GetDst(Node).V2D(), GetSrc(Op->Header.Args[0].ID()).V2D(), VTMP1.V2D());
    break;
    }
    default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
  }
}

DEF_OP(VInsElement) {
  auto Op = IROp->C<IR::IROp_VInsElement>();
  
  auto reg = GetSrc(Op->Header.Args[0].ID());

  if (GetDst(Node).GetCode() != reg.GetCode()) {
    mov(VTMP1, reg);
    reg = VTMP1;
  }

  switch (Op->Header.ElementSize) {
    case 1: {
      mov(reg.V16B(), Op->DestIdx, GetSrc(Op->Header.Args[1].ID()).V16B(), Op->SrcIdx);
    break;
    }
    case 2: {
      mov(reg.V8H(), Op->DestIdx, GetSrc(Op->Header.Args[1].ID()).V8H(), Op->SrcIdx);
    break;
    }
    case 4: {
      mov(reg.V4S(), Op->DestIdx, GetSrc(Op->Header.Args[1].ID()).V4S(), Op->SrcIdx);
    break;
    }
    case 8: {
      mov(reg.V2D(), Op->DestIdx, GetSrc(Op->Header.Args[1].ID()).V2D(), Op->SrcIdx);
    break;
    }
    default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
  }

  if (GetDst(Node).GetCode() != reg.GetCode()) {
    mov(GetDst(Node), reg);
  }
}

DEF_OP(VInsScalarElement) {
  auto Op = IROp->C<IR::IROp_VInsScalarElement>();
  
  auto reg = GetSrc(Op->Header.Args[0].ID());

  if (GetDst(Node).GetCode() != reg.GetCode()) {
    mov(VTMP1, reg);
    reg = VTMP1;
  }

  switch (Op->Header.ElementSize) {
    case 1: {
      mov(reg.V16B(), Op->DestIdx, GetSrc(Op->Header.Args[1].ID()).V16B(), 0);
    break;
    }
    case 2: {
      mov(reg.V8H(), Op->DestIdx, GetSrc(Op->Header.Args[1].ID()).V8H(), 0);
    break;
    }
    case 4: {
      mov(reg.V4S(), Op->DestIdx, GetSrc(Op->Header.Args[1].ID()).V4S(), 0);
    break;
    }
    case 8: {
      mov(reg.V2D(), Op->DestIdx, GetSrc(Op->Header.Args[1].ID()).V2D(), 0);
    break;
    }
    default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
  }

  if (GetDst(Node).GetCode() != reg.GetCode()) {
    mov(GetDst(Node), reg);
  }
}

DEF_OP(VExtractElement) {
  auto Op = IROp->C<IR::IROp_VExtractElement>();
  uint8_t OpSize = IROp->Size;
  switch (OpSize) {
    case 1:
      mov(GetDst(Node).B(), GetSrc(Op->Header.Args[0].ID()).V16B(), Op->Index);
    break;
    case 2:
      mov(GetDst(Node).H(), GetSrc(Op->Header.Args[0].ID()).V8H(), Op->Index);
    break;
    case 4:
      mov(GetDst(Node).S(), GetSrc(Op->Header.Args[0].ID()).V4S(), Op->Index);
    break;
    case 8:
      mov(GetDst(Node).D(), GetSrc(Op->Header.Args[0].ID()).V2D(), Op->Index);
    break;
    default:  LogMan::Msg::A("Unhandled ExtractElementSize: %d", OpSize);
  }
}

DEF_OP(VExtr) {
  auto Op = IROp->C<IR::IROp_VExtr>();
  uint8_t OpSize = IROp->Size;

  // AArch64 ext op has bit arrangement as [Vm:Vn] so arguments need to be swapped
  auto UpperBits = GetSrc(Op->Header.Args[0].ID());
  auto LowerBits = GetSrc(Op->Header.Args[1].ID());
  auto Index = Op->Index;

  if (Index >= OpSize) {
    // Upper bits have moved in to the lower bits
    LowerBits = UpperBits;

    // Upper bits are all now zero
    UpperBits = VTMP1;
    eor(VTMP1.V16B(), VTMP1.V16B(), VTMP1.V16B());
    Index -= OpSize;
  }

  if (OpSize == 8) {
    ext(GetDst(Node).V8B(), LowerBits.V8B(), UpperBits.V8B(), Index * Op->Header.ElementSize);
  }
  else {
    ext(GetDst(Node).V16B(), LowerBits.V16B(), UpperBits.V16B(), Index * Op->Header.ElementSize);
  }
}

DEF_OP(VSLI) {
  auto Op = IROp->C<IR::IROp_VSLI>();
  uint8_t OpSize = IROp->Size;
  uint8_t BitShift = Op->ByteShift * 8;
  if (BitShift < 64) {
    // Move to Pair [TMP2:TMP1]
    mov(TMP1, GetSrc(Op->Header.Args[0].ID()).V2D(), 0);
    mov(TMP2, GetSrc(Op->Header.Args[0].ID()).V2D(), 1);
    // Left shift low 64bits
    lsl(TMP3, TMP1, BitShift);

    // Extract high 64bits from [TMP2:TMP1]
    extr(TMP1, TMP2, TMP1, 64 - BitShift);

    mov(GetDst(Node).V2D(), 0, TMP3);
    mov(GetDst(Node).V2D(), 1, TMP1);
  }
  else {
    if (Op->ByteShift >= OpSize) {
      eor(GetDst(Node).V16B(), GetDst(Node).V16B(), GetDst(Node).V16B());
    }
    else {
      mov(TMP1, GetSrc(Op->Header.Args[0].ID()).V2D(), 0);
      lsl(TMP1, TMP1, BitShift - 64);
      mov(GetDst(Node).V2D(), 0, xzr);
      mov(GetDst(Node).V2D(), 1, TMP1);
    }
  }
}

DEF_OP(VSRI) {
  auto Op = IROp->C<IR::IROp_VSRI>();
  uint8_t OpSize = IROp->Size;
  uint8_t BitShift = Op->ByteShift * 8;
  if (BitShift < 64) {
    // Move to Pair [TMP2:TMP1]
    mov(TMP1, GetSrc(Op->Header.Args[0].ID()).V2D(), 0);
    mov(TMP2, GetSrc(Op->Header.Args[0].ID()).V2D(), 1);

    // Extract Low 64bits [TMP2:TMP2] >> BitShift
    extr(TMP1, TMP2, TMP1, BitShift);
    // Right shift high bits
    lsr(TMP2, TMP2, BitShift);

    mov(GetDst(Node).V2D(), 0, TMP1);
    mov(GetDst(Node).V2D(), 1, TMP2);
  }
  else {
    if (Op->ByteShift >= OpSize) {
      eor(GetDst(Node).V16B(), GetDst(Node).V16B(), GetDst(Node).V16B());
    }
    else {
      mov(TMP1, GetSrc(Op->Header.Args[0].ID()).V2D(), 1);
      lsr(TMP1, TMP1, BitShift - 64);
      mov(GetDst(Node).V2D(), 0, TMP1);
      mov(GetDst(Node).V2D(), 1, xzr);
    }
  }
}

DEF_OP(VUShrI) {
  auto Op = IROp->C<IR::IROp_VUShrI>();

  if (Op->BitShift >= (Op->Header.ElementSize * 8)) {
    eor(GetDst(Node).V16B(), GetDst(Node).V16B(), GetDst(Node).V16B());
  }
  else {
    switch (Op->Header.ElementSize) {
      case 1: {
        ushr(GetDst(Node).V16B(), GetSrc(Op->Header.Args[0].ID()).V16B(), Op->BitShift);
      break;
      }
      case 2: {
        ushr(GetDst(Node).V8H(), GetSrc(Op->Header.Args[0].ID()).V8H(), Op->BitShift);
      break;
      }
      case 4: {
        ushr(GetDst(Node).V4S(), GetSrc(Op->Header.Args[0].ID()).V4S(), Op->BitShift);
      break;
      }
      case 8: {
        ushr(GetDst(Node).V2D(), GetSrc(Op->Header.Args[0].ID()).V2D(), Op->BitShift);
      break;
      }
      default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
    }
  }
}

DEF_OP(VSShrI) {
  auto Op = IROp->C<IR::IROp_VSShrI>();

  switch (Op->Header.ElementSize) {
    case 1: {
      sshr(GetDst(Node).V16B(), GetSrc(Op->Header.Args[0].ID()).V16B(), std::min((uint8_t)(Op->Header.ElementSize * 8 - 1), Op->BitShift));
    break;
    }
    case 2: {
      sshr(GetDst(Node).V8H(), GetSrc(Op->Header.Args[0].ID()).V8H(), std::min((uint8_t)(Op->Header.ElementSize * 8 - 1), Op->BitShift));
    break;
    }
    case 4: {
      sshr(GetDst(Node).V4S(), GetSrc(Op->Header.Args[0].ID()).V4S(), std::min((uint8_t)(Op->Header.ElementSize * 8 - 1), Op->BitShift));
    break;
    }
    case 8: {
      sshr(GetDst(Node).V2D(), GetSrc(Op->Header.Args[0].ID()).V2D(), std::min((uint8_t)(Op->Header.ElementSize * 8 - 1), Op->BitShift));
    break;
    }
    default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
  }
}

DEF_OP(VShlI) {
  auto Op = IROp->C<IR::IROp_VShlI>();

  if (Op->BitShift >= (Op->Header.ElementSize * 8)) {
    eor(GetDst(Node).V16B(), GetDst(Node).V16B(), GetDst(Node).V16B());
  }
  else {
    switch (Op->Header.ElementSize) {
      case 1: {
        shl(GetDst(Node).V16B(), GetSrc(Op->Header.Args[0].ID()).V16B(), Op->BitShift);
      break;
      }
      case 2: {
        shl(GetDst(Node).V8H(), GetSrc(Op->Header.Args[0].ID()).V8H(), Op->BitShift);
      break;
      }
      case 4: {
        shl(GetDst(Node).V4S(), GetSrc(Op->Header.Args[0].ID()).V4S(), Op->BitShift);
      break;
      }
      case 8: {
        shl(GetDst(Node).V2D(), GetSrc(Op->Header.Args[0].ID()).V2D(), Op->BitShift);
      break;
      }
      default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
    }
  }
}

DEF_OP(VUShrNI) {
  auto Op = IROp->C<IR::IROp_VUShrNI>();

  switch (Op->Header.ElementSize) {
    case 1: {
      shrn(GetDst(Node).V8B(), GetSrc(Op->Header.Args[0].ID()).V8H(), Op->BitShift);
    break;
    }
    case 2: {
      shrn(GetDst(Node).V4H(), GetSrc(Op->Header.Args[0].ID()).V4S(), Op->BitShift);
    break;
    }
    case 4: {
      shrn(GetDst(Node).V2S(), GetSrc(Op->Header.Args[0].ID()).V2D(), Op->BitShift);
    break;
    }
    default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
  }
}

DEF_OP(VUShrNI2) {
  auto Op = IROp->C<IR::IROp_VUShrNI2>();
  mov(VTMP1, GetSrc(Op->Header.Args[0].ID()));
  switch (Op->Header.ElementSize) {
    case 1: {
      shrn2(VTMP1.V16B(), GetSrc(Op->Header.Args[1].ID()).V8H(), Op->BitShift);
    break;
    }
    case 2: {
      shrn2(VTMP1.V8H(), GetSrc(Op->Header.Args[1].ID()).V4S(), Op->BitShift);
    break;
    }
    case 4: {
      shrn2(VTMP1.V4S(), GetSrc(Op->Header.Args[1].ID()).V2D(), Op->BitShift);
    break;
    }
    default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
  }

  mov(GetDst(Node), VTMP1);
}

DEF_OP(VBitcast) {
  auto Op = IROp->C<IR::IROp_VBitcast>();
  mov(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));
}

DEF_OP(VSXTL) {
  auto Op = IROp->C<IR::IROp_VSXTL>();
  switch (Op->Header.ElementSize) {
    case 2:
      sxtl(GetDst(Node).V8H(), GetSrc(Op->Header.Args[0].ID()).V8B());
    break;
    case 4:
      sxtl(GetDst(Node).V4S(), GetSrc(Op->Header.Args[0].ID()).V4H());
    break;
    case 8:
      sxtl(GetDst(Node).V2D(), GetSrc(Op->Header.Args[0].ID()).V2S());
    break;
    default: LogMan::Msg::A("Unknown element size: %d", Op->Header.ElementSize);
  }
}

DEF_OP(VSXTL2) {
  auto Op = IROp->C<IR::IROp_VSXTL2>();
  switch (Op->Header.ElementSize) {
    case 2:
      sxtl2(GetDst(Node).V8H(), GetSrc(Op->Header.Args[0].ID()).V16B());
    break;
    case 4:
      sxtl2(GetDst(Node).V4S(), GetSrc(Op->Header.Args[0].ID()).V8H());
    break;
    case 8:
      sxtl2(GetDst(Node).V2D(), GetSrc(Op->Header.Args[0].ID()).V4S());
    break;
    default: LogMan::Msg::A("Unknown element size: %d", Op->Header.ElementSize);
  }
}

DEF_OP(VUXTL) {
  auto Op = IROp->C<IR::IROp_VUXTL>();
  switch (Op->Header.ElementSize) {
    case 2:
      uxtl(GetDst(Node).V8H(), GetSrc(Op->Header.Args[0].ID()).V8B());
    break;
    case 4:
      uxtl(GetDst(Node).V4S(), GetSrc(Op->Header.Args[0].ID()).V4H());
    break;
    case 8:
      uxtl(GetDst(Node).V2D(), GetSrc(Op->Header.Args[0].ID()).V2S());
    break;
    default: LogMan::Msg::A("Unknown element size: %d", Op->Header.ElementSize);
  }
}

DEF_OP(VUXTL2) {
  auto Op = IROp->C<IR::IROp_VUXTL2>();
  switch (Op->Header.ElementSize) {
    case 2:
      uxtl2(GetDst(Node).V8H(), GetSrc(Op->Header.Args[0].ID()).V16B());
    break;
    case 4:
      uxtl2(GetDst(Node).V4S(), GetSrc(Op->Header.Args[0].ID()).V8H());
    break;
    case 8:
      uxtl2(GetDst(Node).V2D(), GetSrc(Op->Header.Args[0].ID()).V4S());
    break;
    default: LogMan::Msg::A("Unknown element size: %d", Op->Header.ElementSize);
  }
}

DEF_OP(VSQXTN) {
  auto Op = IROp->C<IR::IROp_VSQXTN>();
  switch (Op->Header.ElementSize) {
    case 1:
      sqxtn(GetDst(Node).V8B(), GetSrc(Op->Header.Args[0].ID()).V8H());
    break;
    case 2:
      sqxtn(GetDst(Node).V4H(), GetSrc(Op->Header.Args[0].ID()).V4S());
    break;
    case 4:
      sqxtn(GetDst(Node).V2S(), GetSrc(Op->Header.Args[0].ID()).V2D());
    break;
    default: LogMan::Msg::A("Unknown element size: %d", Op->Header.ElementSize);
  }
}

DEF_OP(VSQXTN2) {
  auto Op = IROp->C<IR::IROp_VSQXTN2>();
  uint8_t OpSize = IROp->Size;
  mov(VTMP1, GetSrc(Op->Header.Args[0].ID()));
  if (OpSize == 8) {
    switch (Op->Header.ElementSize) {
      case 1:
        sqxtn(VTMP2.V8B(), GetSrc(Op->Header.Args[1].ID()).V8H());
        ins(VTMP1.V4S(), 1, VTMP2.V4S(), 0);
      break;
      case 2:
        sqxtn(VTMP2.V4H(), GetSrc(Op->Header.Args[1].ID()).V4S());
        ins(VTMP1.V4S(), 1, VTMP2.V4S(), 0);
      break;
      case 4:
        sqxtn(VTMP2.V2S(), GetSrc(Op->Header.Args[1].ID()).V2D());
        ins(VTMP1.V4S(), 1, VTMP2.V4S(), 0);
      break;
      default: LogMan::Msg::A("Unknown element size: %d", Op->Header.ElementSize);
    }
  }
  else {
    switch (Op->Header.ElementSize) {
      case 1:
        sqxtn2(VTMP1.V16B(), GetSrc(Op->Header.Args[1].ID()).V8H());
      break;
      case 2:
        sqxtn2(VTMP1.V8H(), GetSrc(Op->Header.Args[1].ID()).V4S());
      break;
      case 4:
        sqxtn2(VTMP1.V4S(), GetSrc(Op->Header.Args[1].ID()).V2D());
      break;
      default: LogMan::Msg::A("Unknown element size: %d", Op->Header.ElementSize);
    }
  }
  mov(GetDst(Node), VTMP1);
}

DEF_OP(VSQXTUN) {
  auto Op = IROp->C<IR::IROp_VSQXTUN>();
  switch (Op->Header.ElementSize) {
    case 1:
      sqxtun(GetDst(Node).V8B(), GetSrc(Op->Header.Args[0].ID()).V8H());
    break;
    case 2:
      sqxtun(GetDst(Node).V4H(), GetSrc(Op->Header.Args[0].ID()).V4S());
    break;
    case 4:
      sqxtun(GetDst(Node).V2S(), GetSrc(Op->Header.Args[0].ID()).V2D());
    break;
    default: LogMan::Msg::A("Unknown element size: %d", Op->Header.ElementSize);
  }
}

DEF_OP(VSQXTUN2) {
  auto Op = IROp->C<IR::IROp_VSQXTUN2>();
  uint8_t OpSize = IROp->Size;
  mov(VTMP1, GetSrc(Op->Header.Args[0].ID()));
  if (OpSize == 8) {
    switch (Op->Header.ElementSize) {
      case 1:
        sqxtun(VTMP2.V8B(), GetSrc(Op->Header.Args[1].ID()).V8H());
        ins(VTMP1.V4S(), 1, VTMP2.V4S(), 0);
      break;
      case 2:
        sqxtun(VTMP2.V4H(), GetSrc(Op->Header.Args[1].ID()).V4S());
        ins(VTMP1.V4S(), 1, VTMP2.V4S(), 0);
      break;
      case 4:
        sqxtun(VTMP2.V2S(), GetSrc(Op->Header.Args[1].ID()).V2D());
        ins(VTMP1.V4S(), 1, VTMP2.V4S(), 0);
      break;
      default: LogMan::Msg::A("Unknown element size: %d", Op->Header.ElementSize);
    }
  }
  else {
    switch (Op->Header.ElementSize) {
      case 1:
        sqxtun2(VTMP1.V16B(), GetSrc(Op->Header.Args[1].ID()).V8H());
      break;
      case 2:
        sqxtun2(VTMP1.V8H(), GetSrc(Op->Header.Args[1].ID()).V4S());
      break;
      case 4:
        sqxtun2(VTMP1.V4S(), GetSrc(Op->Header.Args[1].ID()).V2D());
      break;
      default: LogMan::Msg::A("Unknown element size: %d", Op->Header.ElementSize);
    }
  }
  mov(GetDst(Node), VTMP1);
}

DEF_OP(VMul) {
  auto Op = IROp->C<IR::IROp_VUMul>();
  switch (Op->Header.ElementSize) {
    case 1: {
      mul(GetDst(Node).V16B(), GetSrc(Op->Header.Args[0].ID()).V16B(), GetSrc(Op->Header.Args[1].ID()).V16B());
    break;
    }
    case 2: {
      mul(GetDst(Node).V8H(), GetSrc(Op->Header.Args[0].ID()).V8H(), GetSrc(Op->Header.Args[1].ID()).V8H());
    break;
    }
    case 4: {
      mul(GetDst(Node).V4S(), GetSrc(Op->Header.Args[0].ID()).V4S(), GetSrc(Op->Header.Args[1].ID()).V4S());
    break;
    }
    case 8: {
      mul(GetDst(Node).V2D(), GetSrc(Op->Header.Args[0].ID()).V2D(), GetSrc(Op->Header.Args[1].ID()).V2D());
    break;
    }
    default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
  }
}

DEF_OP(VUMull) {
  auto Op = IROp->C<IR::IROp_VUMull>();
  switch (Op->Header.ElementSize) {
    case 2: {
      umull(GetDst(Node).V8H(), GetSrc(Op->Header.Args[0].ID()).V8B(), GetSrc(Op->Header.Args[1].ID()).V8B());
    break;
    }
    case 4: {
      umull(GetDst(Node).V4S(), GetSrc(Op->Header.Args[0].ID()).V4H(), GetSrc(Op->Header.Args[1].ID()).V4H());
    break;
    }
    case 8: {
      umull(GetDst(Node).V2D(), GetSrc(Op->Header.Args[0].ID()).V2S(), GetSrc(Op->Header.Args[1].ID()).V2S());
    break;
    }
    default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize >> 1); break;
  }
}

DEF_OP(VSMull) {
  auto Op = IROp->C<IR::IROp_VSMull>();
  switch (Op->Header.ElementSize) {
    case 2: {
      smull(GetDst(Node).V8H(), GetSrc(Op->Header.Args[0].ID()).V8B(), GetSrc(Op->Header.Args[1].ID()).V8B());
    break;
    }
    case 4: {
      smull(GetDst(Node).V4S(), GetSrc(Op->Header.Args[0].ID()).V4H(), GetSrc(Op->Header.Args[1].ID()).V4H());
    break;
    }
    case 8: {
      smull(GetDst(Node).V2D(), GetSrc(Op->Header.Args[0].ID()).V2S(), GetSrc(Op->Header.Args[1].ID()).V2S());
    break;
    }
    default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize >> 1); break;
  }
}

DEF_OP(VUMull2) {
  auto Op = IROp->C<IR::IROp_VUMull2>();
  switch (Op->Header.ElementSize) {
    case 2: {
      umull2(GetDst(Node).V8H(), GetSrc(Op->Header.Args[0].ID()).V16B(), GetSrc(Op->Header.Args[1].ID()).V16B());
    break;
    }
    case 4: {
      umull2(GetDst(Node).V4S(), GetSrc(Op->Header.Args[0].ID()).V8H(), GetSrc(Op->Header.Args[1].ID()).V8H());
    break;
    }
    case 8: {
      umull2(GetDst(Node).V2D(), GetSrc(Op->Header.Args[0].ID()).V4S(), GetSrc(Op->Header.Args[1].ID()).V4S());
    break;
    }
    default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize >> 1); break;
  }
}

DEF_OP(VSMull2) {
  auto Op = IROp->C<IR::IROp_VSMull2>();
  switch (Op->Header.ElementSize) {
    case 2: {
      smull2(GetDst(Node).V8H(), GetSrc(Op->Header.Args[0].ID()).V16B(), GetSrc(Op->Header.Args[1].ID()).V16B());
    break;
    }
    case 4: {
      smull2(GetDst(Node).V4S(), GetSrc(Op->Header.Args[0].ID()).V8H(), GetSrc(Op->Header.Args[1].ID()).V8H());
    break;
    }
    case 8: {
      smull2(GetDst(Node).V2D(), GetSrc(Op->Header.Args[0].ID()).V4S(), GetSrc(Op->Header.Args[1].ID()).V4S());
    break;
    }
    default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize >> 1); break;
  }
}

DEF_OP(VTBL1) {
  auto Op = IROp->C<IR::IROp_VTBL1>();
  uint8_t OpSize = IROp->Size;

  switch (OpSize) {
    case 8: {
      tbl(GetDst(Node).V8B(), GetSrc(Op->Header.Args[0].ID()).V16B(), GetSrc(Op->Header.Args[1].ID()).V8B());
    break;
    }
    case 16: {
      tbl(GetDst(Node).V16B(), GetSrc(Op->Header.Args[0].ID()).V16B(), GetSrc(Op->Header.Args[1].ID()).V16B());
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
  REGISTER_OP(SPLATVECTOR2,      SplatVector2);
  REGISTER_OP(SPLATVECTOR4,      SplatVector4);
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

