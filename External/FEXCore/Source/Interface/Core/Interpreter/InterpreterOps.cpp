#include "Interface/Context/Context.h"
#include "Interface/Core/CPUID.h"
#include "InterpreterDefines.h"
#include "InterpreterOps.h"
#include "F80Ops.h"

#ifdef _M_ARM_64
#include "Interface/Core/ArchHelpers/Arm64.h"
#endif

#include <FEXCore/Core/CPUBackend.h>
#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Debug/InternalThreadState.h>
#include <FEXCore/HLE/SyscallHandler.h>
#include <FEXCore/IR/IR.h>
#include <FEXCore/IR/IntrusiveIRList.h>
#include <FEXCore/Utils/BitUtils.h>
#include <FEXCore/Utils/CompilerDefs.h>
#include <FEXCore/Utils/LogManager.h>

#include "Interface/HLE/Thunks/Thunks.h"

#include <alloca.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <limits>
#include <memory>

namespace FEXCore::CPU {

using OpHandler = void (*)(IR::IROp_Header *IROp, InterpreterOps::IROpData *Data, IR::NodeID Node);
using OpHandlerArray = std::array<OpHandler, IR::IROps::OP_LAST + 1>;

constexpr OpHandlerArray InterpreterOpHandlers = [] {
  OpHandlerArray Handlers{};
  for (auto& Entry : Handlers) {
    Entry = &InterpreterOps::Op_Unhandled;
  }

  #define REGISTER_OP(op, x) Handlers[IR::IROps::OP_##op] = &InterpreterOps::Op_##x

  // ALU ops
  REGISTER_OP(TRUNCELEMENTPAIR,       TruncElementPair);
  REGISTER_OP(CONSTANT,               Constant);
  REGISTER_OP(ENTRYPOINTOFFSET,       EntrypointOffset);
  REGISTER_OP(INLINECONSTANT,         InlineConstant);
  REGISTER_OP(INLINEENTRYPOINTOFFSET, InlineEntrypointOffset);
  REGISTER_OP(CYCLECOUNTER,           CycleCounter);
  REGISTER_OP(ADD,                    Add);
  REGISTER_OP(SUB,                    Sub);
  REGISTER_OP(NEG,                    Neg);
  REGISTER_OP(MUL,                    Mul);
  REGISTER_OP(UMUL,                   UMul);
  REGISTER_OP(DIV,                    Div);
  REGISTER_OP(UDIV,                   UDiv);
  REGISTER_OP(REM,                    Rem);
  REGISTER_OP(UREM,                   URem);
  REGISTER_OP(MULH,                   MulH);
  REGISTER_OP(UMULH,                  UMulH);
  REGISTER_OP(OR,                     Or);
  REGISTER_OP(AND,                    And);
  REGISTER_OP(ANDN,                   Andn);
  REGISTER_OP(XOR,                    Xor);
  REGISTER_OP(LSHL,                   Lshl);
  REGISTER_OP(LSHR,                   Lshr);
  REGISTER_OP(ASHR,                   Ashr);
  REGISTER_OP(ROR,                    Ror);
  REGISTER_OP(EXTR,                   Extr);
  REGISTER_OP(PDEP,                   PDep);
  REGISTER_OP(PEXT,                   PExt);
  REGISTER_OP(LDIV,                   LDiv);
  REGISTER_OP(LUDIV,                  LUDiv);
  REGISTER_OP(LREM,                   LRem);
  REGISTER_OP(LUREM,                  LURem);
  REGISTER_OP(NOT,                    Not);
  REGISTER_OP(POPCOUNT,               Popcount);
  REGISTER_OP(FINDLSB,                FindLSB);
  REGISTER_OP(FINDMSB,                FindMSB);
  REGISTER_OP(FINDTRAILINGZEROS,      FindTrailingZeros);
  REGISTER_OP(COUNTLEADINGZEROES,     CountLeadingZeroes);
  REGISTER_OP(REV,                    Rev);
  REGISTER_OP(BFI,                    Bfi);
  REGISTER_OP(BFE,                    Bfe);
  REGISTER_OP(SBFE,                   Sbfe);
  REGISTER_OP(SELECT,                 Select);
  REGISTER_OP(VEXTRACTTOGPR,          VExtractToGPR);
  REGISTER_OP(FLOAT_TOGPR_ZS,         Float_ToGPR_ZS);
  REGISTER_OP(FLOAT_TOGPR_S,          Float_ToGPR_S);
  REGISTER_OP(FCMP,                   FCmp);

  // Atomic ops
  REGISTER_OP(CASPAIR,                CASPair);
  REGISTER_OP(CAS,                    CAS);
  REGISTER_OP(ATOMICADD,              AtomicAdd);
  REGISTER_OP(ATOMICSUB,              AtomicSub);
  REGISTER_OP(ATOMICAND,              AtomicAnd);
  REGISTER_OP(ATOMICOR,               AtomicOr);
  REGISTER_OP(ATOMICXOR,              AtomicXor);
  REGISTER_OP(ATOMICSWAP,             AtomicSwap);
  REGISTER_OP(ATOMICFETCHADD,         AtomicFetchAdd);
  REGISTER_OP(ATOMICFETCHSUB,         AtomicFetchSub);
  REGISTER_OP(ATOMICFETCHAND,         AtomicFetchAnd);
  REGISTER_OP(ATOMICFETCHOR,          AtomicFetchOr);
  REGISTER_OP(ATOMICFETCHXOR,         AtomicFetchXor);
  REGISTER_OP(ATOMICFETCHNEG,         AtomicFetchNeg);

  // Branch ops
  REGISTER_OP(CALLBACKRETURN,         CallbackReturn);
  REGISTER_OP(EXITFUNCTION,           ExitFunction);
  REGISTER_OP(JUMP,                   Jump);
  REGISTER_OP(CONDJUMP,               CondJump);
  REGISTER_OP(SYSCALL,                Syscall);
  REGISTER_OP(INLINESYSCALL,          InlineSyscall);
  REGISTER_OP(THUNK,                  Thunk);
  REGISTER_OP(VALIDATECODE,           ValidateCode);
  REGISTER_OP(THREADREMOVECODEENTRY,        ThreadRemoveCodeEntry);
  REGISTER_OP(CPUID,                  CPUID);

  // Conversion ops
  REGISTER_OP(VINSGPR,                VInsGPR);
  REGISTER_OP(VCASTFROMGPR,           VCastFromGPR);
  REGISTER_OP(FLOAT_FROMGPR_S,        Float_FromGPR_S);
  REGISTER_OP(FLOAT_FTOF,             Float_FToF);
  REGISTER_OP(VECTOR_STOF,            Vector_SToF);
  REGISTER_OP(VECTOR_FTOZS,           Vector_FToZS);
  REGISTER_OP(VECTOR_FTOS,            Vector_FToS);
  REGISTER_OP(VECTOR_FTOF,            Vector_FToF);
  REGISTER_OP(VECTOR_FTOI,            Vector_FToI);

  // Flag ops
  REGISTER_OP(GETHOSTFLAG,            GetHostFlag);

  // Memory ops
  REGISTER_OP(LOADCONTEXT,            LoadContext);
  REGISTER_OP(STORECONTEXT,           StoreContext);
  REGISTER_OP(LOADREGISTER,           LoadRegister);
  REGISTER_OP(STOREREGISTER,          StoreRegister);
  REGISTER_OP(LOADCONTEXTINDEXED,     LoadContextIndexed);
  REGISTER_OP(STORECONTEXTINDEXED,    StoreContextIndexed);
  REGISTER_OP(SPILLREGISTER,          SpillRegister);
  REGISTER_OP(FILLREGISTER,           FillRegister);
  REGISTER_OP(LOADFLAG,               LoadFlag);
  REGISTER_OP(STOREFLAG,              StoreFlag);
  REGISTER_OP(LOADMEM,                LoadMem);
  REGISTER_OP(STOREMEM,               StoreMem);
  REGISTER_OP(LOADMEMTSO,             LoadMem);
  REGISTER_OP(STOREMEMTSO,            StoreMem);
  REGISTER_OP(CACHELINECLEAR,         CacheLineClear);
  REGISTER_OP(CACHELINECLEAN,         CacheLineClean);
  REGISTER_OP(CACHELINEZERO,          CacheLineZero);

  // Misc ops
  REGISTER_OP(DUMMY,                  NoOp);
  REGISTER_OP(IRHEADER,               NoOp);
  REGISTER_OP(CODEBLOCK,              NoOp);
  REGISTER_OP(BEGINBLOCK,             NoOp);
  REGISTER_OP(ENDBLOCK,               NoOp);
  REGISTER_OP(GUESTOPCODE,            NoOp);
  REGISTER_OP(FENCE,                  Fence);
  REGISTER_OP(BREAK,                  Break);
  REGISTER_OP(PHI,                    NoOp);
  REGISTER_OP(PHIVALUE,               NoOp);
  REGISTER_OP(PRINT,                  Print);
  REGISTER_OP(GETROUNDINGMODE,        GetRoundingMode);
  REGISTER_OP(SETROUNDINGMODE,        SetRoundingMode);
  REGISTER_OP(INVALIDATEFLAGS,        NoOp);
  REGISTER_OP(PROCESSORID,            ProcessorID);
  REGISTER_OP(RDRAND,                 RDRAND);
  REGISTER_OP(YIELD,                  Yield);

  // Move ops
  REGISTER_OP(EXTRACTELEMENTPAIR,     ExtractElementPair);
  REGISTER_OP(CREATEELEMENTPAIR,      CreateElementPair);

  // Vector ops
  REGISTER_OP(VECTORZERO,             VectorZero);
  REGISTER_OP(VECTORIMM,              VectorImm);
  REGISTER_OP(VMOV,                   VMov);
  REGISTER_OP(VAND,                   VAnd);
  REGISTER_OP(VBIC,                   VBic);
  REGISTER_OP(VOR,                    VOr);
  REGISTER_OP(VXOR,                   VXor);
  REGISTER_OP(VADD,                   VAdd);
  REGISTER_OP(VSUB,                   VSub);
  REGISTER_OP(VUQADD,                 VUQAdd);
  REGISTER_OP(VUQSUB,                 VUQSub);
  REGISTER_OP(VSQADD,                 VSQAdd);
  REGISTER_OP(VSQSUB,                 VSQSub);
  REGISTER_OP(VADDP,                  VAddP);
  REGISTER_OP(VADDV,                  VAddV);
  REGISTER_OP(VUMINV,                 VUMinV);
  REGISTER_OP(VURAVG,                 VURAvg);
  REGISTER_OP(VABS,                   VAbs);
  REGISTER_OP(VPOPCOUNT,              VPopcount);
  REGISTER_OP(VFADD,                  VFAdd);
  REGISTER_OP(VFADDP,                 VFAddP);
  REGISTER_OP(VFSUB,                  VFSub);
  REGISTER_OP(VFMUL,                  VFMul);
  REGISTER_OP(VFDIV,                  VFDiv);
  REGISTER_OP(VFMIN,                  VFMin);
  REGISTER_OP(VFMAX,                  VFMax);
  REGISTER_OP(VFRECP,                 VFRecp);
  REGISTER_OP(VFSQRT,                 VFSqrt);
  REGISTER_OP(VFRSQRT,                VFRSqrt);
  REGISTER_OP(VNEG,                   VNeg);
  REGISTER_OP(VFNEG,                  VFNeg);
  REGISTER_OP(VNOT,                   VNot);
  REGISTER_OP(VUMIN,                  VUMin);
  REGISTER_OP(VSMIN,                  VSMin);
  REGISTER_OP(VUMAX,                  VUMax);
  REGISTER_OP(VSMAX,                  VSMax);
  REGISTER_OP(VZIP,                   VZip);
  REGISTER_OP(VZIP2,                  VZip);
  REGISTER_OP(VUNZIP,                 VUnZip);
  REGISTER_OP(VUNZIP2,                VUnZip);
  REGISTER_OP(VBSL,                   VBSL);
  REGISTER_OP(VCMPEQ,                 VCMPEQ);
  REGISTER_OP(VCMPEQZ,                VCMPEQZ);
  REGISTER_OP(VCMPGT,                 VCMPGT);
  REGISTER_OP(VCMPGTZ,                VCMPGTZ);
  REGISTER_OP(VCMPLTZ,                VCMPLTZ);
  REGISTER_OP(VFCMPEQ,                VFCMPEQ);
  REGISTER_OP(VFCMPNEQ,               VFCMPNEQ);
  REGISTER_OP(VFCMPLT,                VFCMPLT);
  REGISTER_OP(VFCMPGT,                VFCMPGT);
  REGISTER_OP(VFCMPLE,                VFCMPLE);
  REGISTER_OP(VFCMPORD,               VFCMPORD);
  REGISTER_OP(VFCMPUNO,               VFCMPUNO);
  REGISTER_OP(VUSHL,                  VUShl);
  REGISTER_OP(VUSHR,                  VUShr);
  REGISTER_OP(VSSHR,                  VSShr);
  REGISTER_OP(VUSHLS,                 VUShlS);
  REGISTER_OP(VUSHRS,                 VUShrS);
  REGISTER_OP(VSSHRS,                 VSShrS);
  REGISTER_OP(VINSELEMENT,            VInsElement);
  REGISTER_OP(VDUPELEMENT,            VDupElement);
  REGISTER_OP(VEXTR,                  VExtr);
  REGISTER_OP(VUSHRI,                 VUShrI);
  REGISTER_OP(VSSHRI,                 VSShrI);
  REGISTER_OP(VSHLI,                  VShlI);
  REGISTER_OP(VUSHRNI,                VUShrNI);
  REGISTER_OP(VUSHRNI2,               VUShrNI2);
  REGISTER_OP(VSXTL,                  VSXTL);
  REGISTER_OP(VSXTL2,                 VSXTL2);
  REGISTER_OP(VUXTL,                  VUXTL);
  REGISTER_OP(VUXTL2,                 VUXTL2);
  REGISTER_OP(VSQXTN,                 VSQXTN);
  REGISTER_OP(VSQXTN2,                VSQXTN2);
  REGISTER_OP(VSQXTUN,                VSQXTUN);
  REGISTER_OP(VSQXTUN2,               VSQXTUN2);
  REGISTER_OP(VUMUL,                  VUMul);
  REGISTER_OP(VSMUL,                  VSMul);
  REGISTER_OP(VUMULL,                 VUMull);
  REGISTER_OP(VSMULL,                 VSMull);
  REGISTER_OP(VUMULL2,                VUMull2);
  REGISTER_OP(VSMULL2,                VSMull2);
  REGISTER_OP(VUABDL,                 VUABDL);
  REGISTER_OP(VTBL1,                  VTBL1);
  REGISTER_OP(VREV64,                 VRev64);

  // Encryption ops
  REGISTER_OP(VAESIMC,                AESImc);
  REGISTER_OP(VAESENC,                AESEnc);
  REGISTER_OP(VAESENCLAST,            AESEncLast);
  REGISTER_OP(VAESDEC,                AESDec);
  REGISTER_OP(VAESDECLAST,            AESDecLast);
  REGISTER_OP(VAESKEYGENASSIST,       AESKeyGenAssist);
  REGISTER_OP(CRC32,                  CRC32);
  REGISTER_OP(PCLMUL,                 PCLMUL);

  // F80 ops
  REGISTER_OP(F80LOADFCW,             F80LOADFCW);
  REGISTER_OP(F80ADD,                 F80ADD);
  REGISTER_OP(F80SUB,                 F80SUB);
  REGISTER_OP(F80MUL,                 F80MUL);
  REGISTER_OP(F80DIV,                 F80DIV);
  REGISTER_OP(F80FYL2X,               F80FYL2X);
  REGISTER_OP(F80ATAN,                F80ATAN);
  REGISTER_OP(F80FPREM1,              F80FPREM1);
  REGISTER_OP(F80FPREM,               F80FPREM);
  REGISTER_OP(F80SCALE,               F80SCALE);
  REGISTER_OP(F80CVT,                 F80CVT);
  REGISTER_OP(F80CVTINT,              F80CVTINT);
  REGISTER_OP(F80CVTTO,               F80CVTTO);
  REGISTER_OP(F80CVTTOINT,            F80CVTTOINT);
  REGISTER_OP(F80ROUND,               F80ROUND);
  REGISTER_OP(F80F2XM1,               F80F2XM1);
  REGISTER_OP(F80TAN,                 F80TAN);
  REGISTER_OP(F80SQRT,                F80SQRT);
  REGISTER_OP(F80SIN,                 F80SIN);
  REGISTER_OP(F80COS,                 F80COS);
  REGISTER_OP(F80XTRACT_EXP,          F80XTRACT_EXP);
  REGISTER_OP(F80XTRACT_SIG,          F80XTRACT_SIG);
  REGISTER_OP(F80CMP,                 F80CMP);
  REGISTER_OP(F80BCDLOAD,             F80BCDLOAD);
  REGISTER_OP(F80BCDSTORE,            F80BCDSTORE);

  // F64 ops
  REGISTER_OP(F64SIN,                 F64SIN);
  REGISTER_OP(F64COS,                 F64COS);
  REGISTER_OP(F64TAN,                 F64TAN);
  REGISTER_OP(F64F2XM1,               F64F2XM1);
  REGISTER_OP(F64ATAN,                F64ATAN);
  REGISTER_OP(F64FPREM,               F64FPREM);
  REGISTER_OP(F64FPREM1,              F64FPREM1);
  REGISTER_OP(F64FYL2X,               F64FYL2X);
  REGISTER_OP(F64SCALE,               F64SCALE);

  return Handlers;
}();

void InterpreterOps::Op_Unhandled(FEXCore::IR::IROp_Header *IROp, IROpData *Data, IR::NodeID Node) {
  LOGMAN_MSG_A_FMT("Unhandled IR Op: {}", FEXCore::IR::GetName(IROp->Op));
}

void InterpreterOps::Op_NoOp(FEXCore::IR::IROp_Header *IROp, IROpData *Data, IR::NodeID Node) {
}

void InterpreterOps::InterpretIR(FEXCore::Core::CpuStateFrame *Frame, FEXCore::IR::IRListView const *CurrentIR) {
  volatile void *StackEntry = alloca(0);

  const uintptr_t ListSize = CurrentIR->GetSSACount();

  static_assert(sizeof(FEXCore::IR::OrderedNode) == 16);

  auto BlockEnd = CurrentIR->GetBlocks().end();

  constexpr size_t ListEntrySizeInBytes = sizeof(InterpVector256);
  const size_t SSADataSize = ListSize * ListEntrySizeInBytes;

  InterpreterOps::IROpData OpData{
    .State = Frame->Thread,
    .CurrentEntry = Frame->State.rip,
    .CurrentIR = CurrentIR,
    .StackEntry = StackEntry,
    .SSAData = alloca(SSADataSize),
    .BlockResults = {},
    .BlockIterator = CurrentIR->GetBlocks().begin(),
  };

  // Clear all SSAData entries to zero. Required for Zero-extend semantics
  memset(OpData.SSAData, 0, SSADataSize);

  while (1) {
    using namespace FEXCore::IR;
    auto [BlockNode, BlockHeader] = OpData.BlockIterator();
    auto BlockIROp = BlockHeader->CW<IROp_CodeBlock>();
    LOGMAN_THROW_AA_FMT(BlockIROp->Header.Op == IR::OP_CODEBLOCK, "IR type failed to be a code block");

    // Reset the block results per block
    memset(&OpData.BlockResults, 0, sizeof(OpData.BlockResults));

    auto CodeBegin = CurrentIR->at(BlockIROp->Begin);
    auto CodeLast = CurrentIR->at(BlockIROp->Last);

    for (auto [CodeNode, IROp] : CurrentIR->GetCode(BlockNode)) {
      const auto ID = CurrentIR->GetID(CodeNode);
      const uint32_t Op = IROp->Op;

      // Execute handler
      OpHandler Handler = InterpreterOpHandlers[Op];

      Handler(IROp, &OpData, ID);

      if (OpData.BlockResults.Quit ||
          OpData.BlockResults.Redo ||
          CodeBegin == CodeLast) {
        break;
      }

      ++CodeBegin;
    }

    // Iterator will have been set, go again
    if (OpData.BlockResults.Redo) {
      continue;
    }

    // If we have set to early exit or at the end block then leave
    if (OpData.BlockResults.Quit || ++OpData.BlockIterator == BlockEnd) {
      break;
    }
  }
}

}
