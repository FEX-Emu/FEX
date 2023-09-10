#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include <FEXCore/Core/CoreState.h>
#include <FEXCore/IR/IR.h>
#include <FEXCore/IR/IntrusiveIRList.h>

namespace FEXCore::Core {
  struct InternalThreadState;
}

namespace FEXCore::IR {
    class IRListView;
    struct IROp_Header;
}

namespace FEXCore::Core{
  struct DebugData;
}

namespace FEXCore::CPU {
  enum FallbackABI {
    FABI_UNKNOWN,
    FABI_VOID_U16,
    FABI_F80_F32,
    FABI_F80_F64,
    FABI_F80_I16,
    FABI_F80_I32,
    FABI_F32_F80,
    FABI_F64_F80,
    FABI_F64_F64,
    FABI_F64_F64_F64,
    FABI_I16_F80,
    FABI_I32_F80,
    FABI_I64_F80,
    FABI_I64_F80_F80,
    FABI_F80_F80,
    FABI_F80_F80_F80,
    FABI_I32_I64_I64_I128_I128_I16,
    FABI_I32_I128_I128_I16,
  };

  struct FallbackInfo {
    FallbackABI ABI;
    void *fn;
    FEXCore::Core::FallbackHandlerIndex HandlerIndex;
  };

  class InterpreterOps {
    public:
      static void InterpretIR(FEXCore::Core::CpuStateFrame *Frame, FEXCore::IR::IRListView const *IR);
      static void FillFallbackIndexPointers(uint64_t *Info);
      static bool GetFallbackHandler(IR::IROp_Header const *IROp, FallbackInfo *Info);

      struct IROpData {
        FEXCore::Core::InternalThreadState *State{};
        uint64_t CurrentEntry{};
        FEXCore::IR::IRListView const *CurrentIR{};
        volatile void *StackEntry{};
        void *SSAData{};
        struct {
          bool Quit;
          bool Redo;
        } BlockResults{};

        IR::NodeIterator BlockIterator{0, 0};
      };

#define DEF_OP(x) static void Op_##x(IR::IROp_Header *IROp, IROpData *Data, IR::NodeID Node)

  ///< Unhandled handler
  DEF_OP(Unhandled);

  ///< No-op Handler
  DEF_OP(NoOp);

  ///< ALU Ops
  DEF_OP(TruncElementPair);
  DEF_OP(Constant);
  DEF_OP(EntrypointOffset);
  DEF_OP(InlineConstant);
  DEF_OP(InlineEntrypointOffset);
  DEF_OP(CycleCounter);
  DEF_OP(Add);
  DEF_OP(Sub);
  DEF_OP(Neg);
  DEF_OP(Abs);
  DEF_OP(Mul);
  DEF_OP(UMul);
  DEF_OP(Div);
  DEF_OP(UDiv);
  DEF_OP(Rem);
  DEF_OP(URem);
  DEF_OP(MulH);
  DEF_OP(UMulH);
  DEF_OP(Or);
  DEF_OP(And);
  DEF_OP(Andn);
  DEF_OP(Xor);
  DEF_OP(Lshl);
  DEF_OP(Lshr);
  DEF_OP(Ashr);
  DEF_OP(Rol);
  DEF_OP(Ror);
  DEF_OP(Extr);
  DEF_OP(PDep);
  DEF_OP(PExt);
  DEF_OP(LDiv);
  DEF_OP(LUDiv);
  DEF_OP(LRem);
  DEF_OP(LURem);
  DEF_OP(Zext);
  DEF_OP(Not);
  DEF_OP(Popcount);
  DEF_OP(FindLSB);
  DEF_OP(FindMSB);
  DEF_OP(FindTrailingZeroes);
  DEF_OP(CountLeadingZeroes);
  DEF_OP(Rev);
  DEF_OP(Bfi);
  DEF_OP(Bfe);
  DEF_OP(Sbfe);
  DEF_OP(Select);
  DEF_OP(VExtractToGPR);
  DEF_OP(Float_ToGPR_ZU);
  DEF_OP(Float_ToGPR_ZS);
  DEF_OP(Float_ToGPR_S);
  DEF_OP(FCmp);

  ///< Atomic ops
  DEF_OP(CASPair);
  DEF_OP(CAS);
  DEF_OP(AtomicAdd);
  DEF_OP(AtomicSub);
  DEF_OP(AtomicAnd);
  DEF_OP(AtomicOr);
  DEF_OP(AtomicXor);
  DEF_OP(AtomicSwap);
  DEF_OP(AtomicFetchAdd);
  DEF_OP(AtomicFetchSub);
  DEF_OP(AtomicFetchAnd);
  DEF_OP(AtomicFetchOr);
  DEF_OP(AtomicFetchXor);
  DEF_OP(AtomicFetchNeg);
  DEF_OP(TelemetrySetValue);

  ///< Branch ops
  DEF_OP(CallbackReturn);
  DEF_OP(ExitFunction);
  DEF_OP(Jump);
  DEF_OP(CondJump);
  DEF_OP(Syscall);
  DEF_OP(InlineSyscall);
  DEF_OP(Thunk);
  DEF_OP(ValidateCode);
  DEF_OP(ThreadRemoveCodeEntry);
  DEF_OP(CPUID);
  DEF_OP(XGETBV);

  ///< Conversion ops
  DEF_OP(VInsGPR);
  DEF_OP(VCastFromGPR);
  DEF_OP(VDupFromGPR);
  DEF_OP(Float_FromGPR_S);
  DEF_OP(Float_FToF);
  DEF_OP(Vector_SToF);
  DEF_OP(Vector_FToZS);
  DEF_OP(Vector_FToS);
  DEF_OP(Vector_FToF);
  DEF_OP(Vector_FToI);

  ///< Flag ops
  DEF_OP(GetHostFlag);

  ///< Memory ops
  DEF_OP(LoadContext);
  DEF_OP(StoreContext);
  DEF_OP(LoadRegister);
  DEF_OP(StoreRegister);
  DEF_OP(LoadContextIndexed);
  DEF_OP(StoreContextIndexed);
  DEF_OP(SpillRegister);
  DEF_OP(FillRegister);
  DEF_OP(LoadFlag);
  DEF_OP(StoreFlag);
  DEF_OP(LoadMem);
  DEF_OP(StoreMem);
  DEF_OP(VLoadVectorMasked);
  DEF_OP(VStoreVectorMasked);
  DEF_OP(VLoadVectorElement);
  DEF_OP(VStoreVectorElement);
  DEF_OP(VBroadcastFromMem);
  DEF_OP(Push);
  DEF_OP(MemSet);
  DEF_OP(MemCpy);
  DEF_OP(CacheLineClear);
  DEF_OP(CacheLineClean);
  DEF_OP(CacheLineZero);

  ///< Misc ops
  DEF_OP(EndBlock);
  DEF_OP(Fence);
  DEF_OP(Break);
  DEF_OP(Print);
  DEF_OP(GetRoundingMode);
  DEF_OP(SetRoundingMode);
  DEF_OP(ProcessorID);
  DEF_OP(RDRAND);
  DEF_OP(Yield);

  ///< Move ops
  DEF_OP(ExtractElementPair);
  DEF_OP(CreateElementPair);
  DEF_OP(Mov);

  ///< Vector ops
  DEF_OP(VectorZero);
  DEF_OP(VectorImm);
  DEF_OP(LoadNamedVectorConstant);
  DEF_OP(LoadNamedVectorIndexedConstant);
  DEF_OP(VMov);
  DEF_OP(VAnd);
  DEF_OP(VBic);
  DEF_OP(VOr);
  DEF_OP(VXor);
  DEF_OP(VAdd);
  DEF_OP(VSub);
  DEF_OP(VUQAdd);
  DEF_OP(VUQSub);
  DEF_OP(VSQAdd);
  DEF_OP(VSQSub);
  DEF_OP(VAddP);
  DEF_OP(VAddV);
  DEF_OP(VUMinV);
  DEF_OP(VURAvg);
  DEF_OP(VAbs);
  DEF_OP(VPopcount);
  DEF_OP(VFAdd);
  DEF_OP(VFAddP);
  DEF_OP(VFSub);
  DEF_OP(VFMul);
  DEF_OP(VFDiv);
  DEF_OP(VFMin);
  DEF_OP(VFMax);
  DEF_OP(VFRecp);
  DEF_OP(VFSqrt);
  DEF_OP(VFRSqrt);
  DEF_OP(VNeg);
  DEF_OP(VFNeg);
  DEF_OP(VNot);
  DEF_OP(VUMin);
  DEF_OP(VSMin);
  DEF_OP(VUMax);
  DEF_OP(VSMax);
  DEF_OP(VZip);
  DEF_OP(VUnZip);
  DEF_OP(VTrn);
  DEF_OP(VBSL);
  DEF_OP(VCMPEQ);
  DEF_OP(VCMPEQZ);
  DEF_OP(VCMPGT);
  DEF_OP(VCMPGTZ);
  DEF_OP(VCMPLTZ);
  DEF_OP(VFCMPEQ);
  DEF_OP(VFCMPNEQ);
  DEF_OP(VFCMPLT);
  DEF_OP(VFCMPGT);
  DEF_OP(VFCMPLE);
  DEF_OP(VFCMPORD);
  DEF_OP(VFCMPUNO);
  DEF_OP(VUShl);
  DEF_OP(VUShr);
  DEF_OP(VSShr);
  DEF_OP(VUShlS);
  DEF_OP(VUShrS);
  DEF_OP(VSShrS);
  DEF_OP(VUShrSWide);
  DEF_OP(VSShrSWide);
  DEF_OP(VUShlSWide);
  DEF_OP(VInsElement);
  DEF_OP(VDupElement);
  DEF_OP(VExtr);
  DEF_OP(VUShrI);
  DEF_OP(VSShrI);
  DEF_OP(VShlI);
  DEF_OP(VUShrNI);
  DEF_OP(VUShrNI2);
  DEF_OP(VSXTL);
  DEF_OP(VSXTL2);
  DEF_OP(VUXTL);
  DEF_OP(VUXTL2);
  DEF_OP(VSQXTN);
  DEF_OP(VSQXTN2);
  DEF_OP(VSQXTNPair);
  DEF_OP(VSQXTUN);
  DEF_OP(VSQXTUN2);
  DEF_OP(VSQXTUNPair);
  DEF_OP(VUMul);
  DEF_OP(VUMull);
  DEF_OP(VSMul);
  DEF_OP(VSMull);
  DEF_OP(VUMull2);
  DEF_OP(VSMull2);
  DEF_OP(VUMulH);
  DEF_OP(VSMulH);
  DEF_OP(VUABDL);
  DEF_OP(VUABDL2);
  DEF_OP(VTBL1);
  DEF_OP(VRev32);
  DEF_OP(VRev64);
  DEF_OP(VPCMPESTRX);
  DEF_OP(VPCMPISTRX);
  DEF_OP(VFCADD);

  ///< Encryption ops
  DEF_OP(AESImc);
  DEF_OP(AESEnc);
  DEF_OP(AESEncLast);
  DEF_OP(AESDec);
  DEF_OP(AESDecLast);
  DEF_OP(AESKeyGenAssist);
  DEF_OP(CRC32);
  DEF_OP(PCLMUL);

  ///< F80 ops
  DEF_OP(F80LOADFCW);
  DEF_OP(F80ADD);
  DEF_OP(F80SUB);
  DEF_OP(F80MUL);
  DEF_OP(F80DIV);
  DEF_OP(F80FYL2X);
  DEF_OP(F80ATAN);
  DEF_OP(F80FPREM1);
  DEF_OP(F80FPREM);
  DEF_OP(F80SCALE);
  DEF_OP(F80CVT);
  DEF_OP(F80CVTINT);
  DEF_OP(F80CVTTO);
  DEF_OP(F80CVTTOINT);
  DEF_OP(F80ROUND);
  DEF_OP(F80F2XM1);
  DEF_OP(F80TAN);
  DEF_OP(F80SQRT);
  DEF_OP(F80SIN);
  DEF_OP(F80COS);
  DEF_OP(F80XTRACT_EXP);
  DEF_OP(F80XTRACT_SIG);
  DEF_OP(F80CMP);
  DEF_OP(F80BCDLOAD);
  DEF_OP(F80BCDSTORE);

  //< F64 ops
  DEF_OP(F64SIN);
  DEF_OP(F64COS);
  DEF_OP(F64TAN);
  DEF_OP(F64F2XM1);
  DEF_OP(F64ATAN);
  DEF_OP(F64FPREM);
  DEF_OP(F64FPREM1);
  DEF_OP(F64FYL2X);
  DEF_OP(F64SCALE);
#undef DEF_OP
  template<typename unsigned_type, typename signed_type, typename float_type>
  [[nodiscard]] static bool IsConditionTrue(IR::CondClassType Cond, uint64_t Src1, uint64_t Src2) {
    bool CompResult = false;
    if constexpr (sizeof(unsigned_type) == 16) {
      LOGMAN_THROW_A_FMT(Cond != IR::CondClassType::FLU &&
        Cond != IR::CondClassType::FGE &&
        Cond != IR::CondClassType::FLEU &&
        Cond != IR::CondClassType::FGT &&
        Cond != IR::CondClassType::FU &&
        Cond != IR::CondClassType::FNU, "Unsupported comparison for 128-bit floats");
    }

    switch (Cond) {
      case IR::CondClassType::EQ:
        CompResult = static_cast<unsigned_type>(Src1) == static_cast<unsigned_type>(Src2);
        break;
      case IR::CondClassType::NEQ:
        CompResult = static_cast<unsigned_type>(Src1) != static_cast<unsigned_type>(Src2);
        break;
      case IR::CondClassType::SGE:
        CompResult = static_cast<signed_type>(Src1) >= static_cast<signed_type>(Src2);
        break;
      case IR::CondClassType::SLT:
        CompResult = static_cast<signed_type>(Src1) < static_cast<signed_type>(Src2);
        break;
      case IR::CondClassType::SGT:
        CompResult = static_cast<signed_type>(Src1) > static_cast<signed_type>(Src2);
        break;
      case IR::CondClassType::SLE:
        CompResult = static_cast<signed_type>(Src1) <= static_cast<signed_type>(Src2);
        break;
      case IR::CondClassType::UGE:
        CompResult = static_cast<unsigned_type>(Src1) >= static_cast<unsigned_type>(Src2);
        break;
      case IR::CondClassType::ULT:
        CompResult = static_cast<unsigned_type>(Src1) < static_cast<unsigned_type>(Src2);
        break;
      case IR::CondClassType::UGT:
        CompResult = static_cast<unsigned_type>(Src1) > static_cast<unsigned_type>(Src2);
        break;
      case IR::CondClassType::ULE:
        CompResult = static_cast<unsigned_type>(Src1) <= static_cast<unsigned_type>(Src2);
        break;

      case IR::CondClassType::FLU:
        CompResult = reinterpret_cast<float_type&>(Src1) < reinterpret_cast<float_type&>(Src2) || (std::isnan(reinterpret_cast<float_type&>(Src1)) || std::isnan(reinterpret_cast<float_type&>(Src2)));
        break;
      case IR::CondClassType::FGE:
        CompResult = reinterpret_cast<float_type&>(Src1) >= reinterpret_cast<float_type&>(Src2) && !(std::isnan(reinterpret_cast<float_type&>(Src1)) || std::isnan(reinterpret_cast<float_type&>(Src2)));
        break;
      case IR::CondClassType::FLEU:
        CompResult = reinterpret_cast<float_type&>(Src1) <= reinterpret_cast<float_type&>(Src2) || (std::isnan(reinterpret_cast<float_type&>(Src1)) || std::isnan(reinterpret_cast<float_type&>(Src2)));
        break;
      case IR::CondClassType::FGT:
        CompResult = reinterpret_cast<float_type&>(Src1) > reinterpret_cast<float_type&>(Src2) && !(std::isnan(reinterpret_cast<float_type&>(Src1)) || std::isnan(reinterpret_cast<float_type&>(Src2)));
        break;
      case IR::CondClassType::FU:
        CompResult = (std::isnan(reinterpret_cast<float_type&>(Src1)) || std::isnan(reinterpret_cast<float_type&>(Src2)));
        break;
      case IR::CondClassType::FNU:
        CompResult = !(std::isnan(reinterpret_cast<float_type&>(Src1)) || std::isnan(reinterpret_cast<float_type&>(Src2)));
        break;
      case IR::CondClassType::MI:
      case IR::CondClassType::PL:
      case IR::CondClassType::VS:
      case IR::CondClassType::VC:
      default:
        LOGMAN_MSG_A_FMT("Unsupported compare type");
        break;
    }

    return CompResult;
  }

  static uint8_t GetOpSize(FEXCore::IR::IRListView const *CurrentIR, IR::OrderedNodeWrapper Node) {
    auto IROp = CurrentIR->GetOp<FEXCore::IR::IROp_Header>(Node);
    return IROp->Size;
  }

  // The maximum size a vector can be within FEX's interpreter.
  // NOTE: If we ever support AVX-512, this should be changed
  //       to 64 bytes in size.
  static constexpr size_t MaxInterpeterVectorSize = Core::CPUState::XMM_AVX_REG_SIZE;

  // Alias for specifying temporary data that is operated on
  // before storing into a destination.
  using TempVectorDataArray = std::array<uint8_t, MaxInterpeterVectorSize>;

  };
} // namespace FEXCore::CPU
