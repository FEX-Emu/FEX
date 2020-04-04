#include "Interface/Context/Context.h"
#include "Interface/Core/DebugData.h"
#include "Interface/Core/LLVMJIT/LLVMMemoryManager.h"
#include "Interface/HLE/Syscalls.h"

#include <FEXCore/Core/CPUBackend.h>

#include <llvm-c/Core.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/InitializePasses.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/IRPrintingPasses.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Vectorize.h>
#include <vector>

#define DESTMAP_AS_MAP 0
#if DESTMAP_AS_MAP
using DestMapType = std::unordered_map<uint64_t, llvm::Value*>;
#else
using DestMapType = std::vector<llvm::Value*>;
#endif

#if defined(_M_ARM_64) && defined(_M_X86_64)
#define AARCH64_ON_X86
#endif

namespace FEXCore::CPU {

static void CPUIDRun_Thunk(CPUIDEmu::FunctionResults *Results, FEXCore::CPUIDEmu *Class, uint32_t Function) {
  *Results = Class->RunFunction(Function);
}

static void SetExitState_Thunk(FEXCore::Core::InternalThreadState *Thread) {
  Thread->State.RunningEvents.ShouldStop = true;
}

#if defined(_M_ARM_64) && !defined(AARCH64_ON_X86)
static uint64_t AArch64ReadCycleCounter() {
  uint64_t res{};
  asm ("mrs %0, CNTVCT_EL0"
      : "=r" (res));
  return res;
}
#endif

class LLVMJITCore final : public CPUBackend {
public:
  explicit LLVMJITCore(FEXCore::Core::InternalThreadState *Thread);
  ~LLVMJITCore() override;
  std::string GetName() override { return "JIT"; }
  void* CompileCode(FEXCore::IR::IRListView<true> const *IR, FEXCore::Core::DebugData *DebugData) override ;

  void *MapRegion(void *HostPtr, uint64_t GuestPtr, uint64_t Size) override {
    return HostPtr;
  }

  bool NeedsOpDispatch() override { return true; }

private:
  void HandleIR(FEXCore::IR::IRListView<true> const *IR, IR::NodeWrapperIterator *Node);
  llvm::Value *CreateContextGEP(uint64_t Offset, uint8_t Size);
  llvm::Value *CreateContextPtr(uint64_t Offset, uint8_t Size);
  llvm::Value *CreateIndexedContextPtr(llvm::Value *Index, uint64_t Offset, uint8_t Size, uint8_t Stride);
  llvm::Value *CreateMemoryLoad(llvm::Value *Ptr, uint8_t Align);
  void CreateMemoryStore(llvm::Value *Ptr, llvm::Value *Val, uint8_t Align);

  void ValidateMemoryInVM(uint64_t Ptr, uint8_t Size, bool Load);
  template<typename Type>
  Type MemoryLoad_Validate(uint64_t Ptr);
  template<typename Type>
  void MemoryStore_Validate(uint64_t Ptr, Type Val);

  void DebugPrint(uint64_t Val);
  void DebugPrint128(__uint128_t Val);

  FEXCore::Core::InternalThreadState *ThreadState;
  FEXCore::Context::Context *CTX;

  struct LLVMState {
    LLVMContextRef ContextRef;
    llvm::Module *MainModule;
    llvm::EngineBuilder *MainEngineBuilder;
    llvm::IRBuilder<> *IRBuilder;
    LLVMMemoryManager *MemManager;
    std::vector<llvm::ExecutionEngine*> Functions;
  };

  struct LLVMCurrentState {
    llvm::Function *SyscallFunction;
    llvm::Function *CPUIDFunction;
    llvm::Function *ExitVMFunction;
    llvm::Function *ValuePrinter;
#if defined(_M_ARM_64) && !defined(AARCH64_ON_X86)
    llvm::Function *AArch64ReadCycleCounterFunction;
#endif

    llvm::Function *ValidateLoad8;
    llvm::Function *ValidateLoad16;
    llvm::Function *ValidateLoad32;
    llvm::Function *ValidateLoad64;
    llvm::Function *ValidateLoad128;

    llvm::Function *ValidateStore8;
    llvm::Function *ValidateStore16;
    llvm::Function *ValidateStore32;
    llvm::Function *ValidateStore64;
    llvm::Function *ValidateStore128;

    llvm::Function *DebugPrint;
    llvm::Function *DebugPrint128;

    llvm::Type *CPUStateType;
    llvm::GlobalVariable *CPUStateVar;
    llvm::LoadInst *CPUState;

    llvm::BasicBlock *CurrentBlock;
    std::vector<llvm::BasicBlock*> Blocks;
    llvm::BasicBlock *ExitBlock;
  };

  LLVMState JITState;
  LLVMCurrentState JITCurrentState;
  llvm::LLVMContext *Con;
  llvm::Function *Func;

  // Intrinsics
  llvm::CallInst *Popcount(llvm::Value *Arg) { return JITState.IRBuilder->CreateUnaryIntrinsic(llvm::Intrinsic::ctpop, Arg); }
  llvm::CallInst *BSwap(llvm::Value *Arg) { return JITState.IRBuilder->CreateUnaryIntrinsic(llvm::Intrinsic::bswap, Arg); }
  llvm::CallInst *CTTZ(llvm::Value *Arg) {
    std::vector<llvm::Type*> ArgTypes = {
      Arg->getType(),
    };
    std::vector<llvm::Value*> Args = {
      Arg,
      JITState.IRBuilder->getInt1(true),
    };

    return JITState.IRBuilder->CreateIntrinsic(llvm::Intrinsic::cttz, ArgTypes, Args);
  }

  llvm::CallInst *CTLZ(llvm::Value *Arg) {
    std::vector<llvm::Type*> ArgTypes = {
      Arg->getType(),
    };
    std::vector<llvm::Value*> Args = {
      Arg,
      JITState.IRBuilder->getInt1(true),
    };

    return JITState.IRBuilder->CreateIntrinsic(llvm::Intrinsic::ctlz, ArgTypes, Args);
  }


  llvm::Value *VSQXTN_64(llvm::Value *Arg, uint8_t RegisterSize, uint8_t ElementSize) {
    llvm::Value *Result{};
#ifdef _M_X86_64
    Arg = CastVectorToType(Arg, true, 16, ElementSize);
    RegisterSize <<= 1;

    uint8_t DestNumElements = RegisterSize / (ElementSize >> 1);
    uint8_t DestElementSize = ElementSize >> 1;

    std::vector<uint32_t> VectorMaskConstant;
    switch (ElementSize) {
      case 2:
        VectorMaskConstant = {
          0, 1, 2, 3, // First 8 bytes from high result of source
        };
        break;
      case 4:
        VectorMaskConstant = {
          0, 1, // First 4 16bits from high result of source
        };
        break;
      default: LogMan::Msg::A("Unhandled VSQXTN size: %d", ElementSize); break;
    }

    std::vector<llvm::Type*> ArgTypes = {
    };

    std::vector<llvm::Value*> Args = {
      Arg,
      llvm::UndefValue::get(Arg->getType()),
    };

    switch (ElementSize) {
      case 2:
        Result = JITState.IRBuilder->CreateIntrinsic(llvm::Intrinsic::x86_sse2_packsswb_128, ArgTypes, Args);
        break;
      case 4:
        Result = JITState.IRBuilder->CreateIntrinsic(llvm::Intrinsic::x86_sse2_packssdw_128, ArgTypes, Args);
        break;
      default: LogMan::Msg::A("Unhandled VSQXTN size: %d", ElementSize); break;
    }
    // We now need to shuffle the vectors
    // We want the top 64bits to be zero
    // The lower 64bits being the results we desire
    auto ZeroVector = JITState.IRBuilder->CreateVectorSplat(DestNumElements, JITState.IRBuilder->getIntN(DestElementSize * 8, 0));

    Result = JITState.IRBuilder->CreateShuffleVector(Result, ZeroVector, VectorMaskConstant);
#elif _M_ARM_64
    Arg = CastVectorToType(Arg, true, 16, ElementSize);
    RegisterSize <<= 1;

    std::vector<llvm::Type*> ArgTypes = {
      llvm::VectorType::get(llvm::Type::getIntNTy(*Con, (ElementSize >> 1) * 8), RegisterSize / ElementSize),
    };

    std::vector<llvm::Value*> Args = {
      Arg,
    };

    Result = JITState.IRBuilder->CreateIntrinsic(llvm::Intrinsic::aarch64_neon_sqxtn, ArgTypes, Args);

#else
    static_assert(false, "Unhandle intrinsic");
#endif

    return Result;
  }

  llvm::Value *VSQXTN2_64(llvm::Value *ArgLower, llvm::Value *ArgUpper, uint8_t RegisterSize, uint8_t ElementSize) {
    std::vector<uint32_t> VectorMaskConstant;

#ifdef _M_X86_64
    // x86 handles the incoming lower register as a full 128bit, cast it back to ensure LLVM doesn't throw up
    // We don't care about the upper bits anyway
    ArgLower = CastVectorToType(ArgLower, true, RegisterSize >> 1, ElementSize >> 1);

    switch (ElementSize) {
      case 2:
        VectorMaskConstant = {
          0, 1, 2, 3, 4, 5, 6, 7,
        };
        break;
      case 4:
        VectorMaskConstant = {
          0, 1, 2, 3,
        };
        break;
      default: LogMan::Msg::A("Unhandled VSQXTN2 size: %d", ElementSize); break;
    }
#else
    ArgLower = CastVectorToType(ArgLower, true, RegisterSize, ElementSize >> 1);
    switch (ElementSize) {
      case 2:
        VectorMaskConstant = {
          0, 1, 2, 3, 8, 9, 10, 11,
        };
        break;
      case 4:
        VectorMaskConstant = {
          0, 1, 4, 5,
        };
        break;
      default: LogMan::Msg::A("Unhandled VSQXTN2 size: %d", ElementSize); break;
    }
#endif

    // Convert our upper args
    ArgUpper = VSQXTN_64(ArgUpper, RegisterSize, ElementSize);

    // This will convert to VSQXTN2 in AArch64
    return JITState.IRBuilder->CreateShuffleVector(ArgLower, ArgUpper, VectorMaskConstant);
  }

  llvm::Value *VSQXTUN_64(llvm::Value *Arg, uint8_t RegisterSize, uint8_t ElementSize) {
    llvm::Value *Result{};
#ifdef _M_X86_64
    // Incoming source is a 64bit register
    // We want to still pass it through SSE and NEON's 128bit implementation and ignore the upper 64bits
    // Cast to the type we want
    llvm::Value *UndefVector = llvm::UndefValue::get(Arg->getType());

    std::vector<uint32_t> LargeVectorMask;
    for (uint32_t i = 0; i < (RegisterSize / ElementSize * 2); ++i) {
      LargeVectorMask.emplace_back(i);
    }
    Arg = JITState.IRBuilder->CreateShuffleVector(Arg, UndefVector, LargeVectorMask);

    RegisterSize <<= 1;

    uint8_t DestNumElements = RegisterSize / (ElementSize >> 1);
    uint8_t DestElementSize = ElementSize >> 1;

    std::vector<uint32_t> VectorMaskConstant;
    switch (ElementSize) {
      case 2:
        VectorMaskConstant = {
          0, 1, 2, 3, // First 8 bytes from high result of source
        };
        break;
      case 4:
        VectorMaskConstant = {
          0, 1, // First 4 16bits from high result of source
        };
        break;
      default: LogMan::Msg::A("Unhandled VSQXTUN size: %d", ElementSize); break;
    }

    std::vector<llvm::Type*> ArgTypes = {
    };

    std::vector<llvm::Value*> Args = {
      Arg,
      llvm::UndefValue::get(Arg->getType()),
    };

    switch (ElementSize) {
      case 2:
        Result = JITState.IRBuilder->CreateIntrinsic(llvm::Intrinsic::x86_sse2_packuswb_128, ArgTypes, Args);
        break;
      case 4:
        Result = JITState.IRBuilder->CreateIntrinsic(llvm::Intrinsic::x86_sse41_packusdw, ArgTypes, Args);
        break;
      default: LogMan::Msg::A("Unhandled VSQXTN size: %d", ElementSize); break;
    }
    // We now need to shuffle the vectors
    // We want the top 64bits to be zero
    // The lower 64bits being the results we desire
    auto ZeroVector = JITState.IRBuilder->CreateVectorSplat(DestNumElements, JITState.IRBuilder->getIntN(DestElementSize * 8, 0));

    Result = JITState.IRBuilder->CreateShuffleVector(Result, ZeroVector, VectorMaskConstant);
#elif _M_ARM_64
    Arg = CastVectorToType(Arg, true, 16, ElementSize);
    RegisterSize <<= 1;

    std::vector<llvm::Type*> ArgTypes = {
      llvm::VectorType::get(llvm::Type::getIntNTy(*Con, (ElementSize >> 1) * 8), RegisterSize / ElementSize),
    };

    std::vector<llvm::Value*> Args = {
      Arg,
    };

    Result = JITState.IRBuilder->CreateIntrinsic(llvm::Intrinsic::aarch64_neon_sqxtun, ArgTypes, Args);

#else
    static_assert(false, "Unhandle intrinsic");
#endif

    return Result;
  }

  llvm::Value *VSQXTUN2_64(llvm::Value *ArgLower, llvm::Value *ArgUpper, uint8_t RegisterSize, uint8_t ElementSize) {
    std::vector<uint32_t> VectorMaskConstant;

#ifdef _M_X86_64
    // x86 handles the incoming lower register as a full 128bit, cast it back to ensure LLVM doesn't throw up
    // We don't care about the upper bits anyway
    ArgLower = CastVectorToType(ArgLower, true, RegisterSize >> 1, ElementSize >> 1);

    switch (ElementSize) {
      case 2:
        VectorMaskConstant = {
          0, 1, 2, 3, 4, 5, 6, 7,
        };
        break;
      case 4:
        VectorMaskConstant = {
          0, 1, 2, 3,
        };
        break;
      default: LogMan::Msg::A("Unhandled VSQXTN2 size: %d", ElementSize); break;
    }
#else
    ArgLower = CastVectorToType(ArgLower, true, RegisterSize, ElementSize >> 1);
    switch (ElementSize) {
      case 2:
        VectorMaskConstant = {
          0, 1, 2, 3, 8, 9, 10, 11,
        };
        break;
      case 4:
        VectorMaskConstant = {
          0, 1, 4, 5,
        };
        break;
      default: LogMan::Msg::A("Unhandled VSQXTN2 size: %d", ElementSize); break;
    }
#endif
    // Convert our upper args
    ArgUpper = VSQXTUN_64(ArgUpper, RegisterSize, ElementSize);

    // This will convert to VSQXTN2 in AArch64
    return JITState.IRBuilder->CreateShuffleVector(ArgLower, ArgUpper, VectorMaskConstant);
  }

  llvm::Value *VSQXTN(llvm::Value *Arg, uint8_t RegisterSize, uint8_t ElementSize) {
#ifdef _M_X86_64
    std::vector<llvm::Type*> ArgTypes = {
    };

    std::vector<llvm::Value*> Args = {
      llvm::UndefValue::get(Arg->getType()),
      Arg,
    };

    uint8_t DestNumElements = RegisterSize / (ElementSize >> 1);
    uint8_t DestElementSize = ElementSize >> 1;

    llvm::Value *Result{};
    std::vector<uint32_t> VectorMaskConstant;

    switch (ElementSize) {
      case 2:
        VectorMaskConstant = {
          8, 9, 10, 11, 12, 13, 14, 15, // First 8 bytes from high result of source
          16, 17, 18, 19, 20, 21, 22, 23, // Second 8 bytes from the zero vector
        };
        Result = JITState.IRBuilder->CreateIntrinsic(llvm::Intrinsic::x86_sse2_packsswb_128, ArgTypes, Args);
        break;
      case 4:
        VectorMaskConstant = {
          4, 5, 6, 7, // First 4 16bits from high result of source
          8, 9, 10, 11, // Second 8 16bits from the zero vector
        };

        Result = JITState.IRBuilder->CreateIntrinsic(llvm::Intrinsic::x86_sse2_packssdw_128, ArgTypes, Args);
        break;
      default: LogMan::Msg::A("Unhandled VSQXTN size: %d", ElementSize); break;
    }

    // We now need to shuffle the vectors
    // We want the top 64bits to be zero
    // The lower 64bits being the results we desire
    auto ZeroVector = JITState.IRBuilder->CreateVectorSplat(DestNumElements, JITState.IRBuilder->getIntN(DestElementSize * 8, 0));

    Result = JITState.IRBuilder->CreateShuffleVector(Result, ZeroVector, VectorMaskConstant);

#elif _M_ARM_64
    std::vector<llvm::Type*> ArgTypes = {
      llvm::VectorType::get(llvm::Type::getIntNTy(*Con, (ElementSize >> 1) * 8), RegisterSize / ElementSize),
    };

    std::vector<llvm::Value*> Args = {
      Arg,
    };

    auto Result = JITState.IRBuilder->CreateIntrinsic(llvm::Intrinsic::aarch64_neon_sqxtn, ArgTypes, Args);
#else
    static_assert(false, "Unhandle intrinsic");
#endif

    return Result;
  }

  llvm::Value *VSQXTN2(llvm::Value *ArgLower, llvm::Value *ArgUpper, uint8_t RegisterSize, uint8_t ElementSize) {
#ifdef _M_X86_64
    // x86 handles the incoming lower register as a full 128bit, cast it back to ensure LLVM doesn't throw up
    // We don't care about the upper bits anyway
    ArgLower = CastVectorToType(ArgLower, true, RegisterSize, ElementSize >> 1);
#endif
    // Convert our upper args
    ArgUpper = VSQXTN(ArgUpper, RegisterSize, ElementSize);

    std::vector<uint32_t> VectorMaskConstant;

    switch (ElementSize) {
      case 2:
        VectorMaskConstant = {
          0, 1, 2, 3, 4, 5, 6, 7,
          16, 17, 18, 19, 20, 21, 22, 23
        };
        break;
      case 4:
        VectorMaskConstant = {
          0, 1, 2, 3,
          8, 9, 10, 11
        };
        break;
      default: LogMan::Msg::A("Unhandled VSQXTN2 size: %d", ElementSize); break;
    }

    // This will convert to VSQXTN2 in AArch64
    return JITState.IRBuilder->CreateShuffleVector(ArgLower, ArgUpper, VectorMaskConstant);
  }

  llvm::Value *VSQXTUN(llvm::Value *Arg, uint8_t RegisterSize, uint8_t ElementSize) {
#ifdef _M_X86_64
    std::vector<llvm::Type*> ArgTypes = {
    };

    std::vector<llvm::Value*> Args = {
      llvm::UndefValue::get(Arg->getType()),
      Arg,
    };

    uint8_t DestNumElements = RegisterSize / (ElementSize >> 1);
    uint8_t DestElementSize = ElementSize >> 1;

    llvm::Value *Result{};
    std::vector<uint32_t> VectorMaskConstant;

    switch (ElementSize) {
      case 2:
        VectorMaskConstant = {
          8, 9, 10, 11, 12, 13, 14, 15, // First 8 bytes from high result of source
          16, 17, 18, 19, 20, 21, 22, 23, // Second 8 bytes from the zero vector
        };
        Result = JITState.IRBuilder->CreateIntrinsic(llvm::Intrinsic::x86_sse2_packuswb_128, ArgTypes, Args);
        break;
      case 4:
        VectorMaskConstant = {
          4, 5, 6, 7, // First 4 16bits from high result of source
          8, 9, 10, 11, // Second 8 16bits from the zero vector
        };

        Result = JITState.IRBuilder->CreateIntrinsic(llvm::Intrinsic::x86_avx2_packusdw, ArgTypes, Args);
        break;
      default: LogMan::Msg::A("Unhandled VSQXTUN size: %d", ElementSize); break;
    }

    // We now need to shuffle the vectors
    // We want the top 64bits to be zero
    // The lower 64bits being the results we desire
    auto ZeroVector = JITState.IRBuilder->CreateVectorSplat(DestNumElements, JITState.IRBuilder->getIntN(DestElementSize * 8, 0));

    Result = JITState.IRBuilder->CreateShuffleVector(Result, ZeroVector, VectorMaskConstant);

#elif _M_ARM_64
    std::vector<llvm::Type*> ArgTypes = {
      llvm::VectorType::get(llvm::Type::getIntNTy(*Con, (ElementSize >> 1) * 8), RegisterSize / ElementSize),
    };

    std::vector<llvm::Value*> Args = {
      Arg,
    };

    auto Result = JITState.IRBuilder->CreateIntrinsic(llvm::Intrinsic::aarch64_neon_sqxtun, ArgTypes, Args);
#else
    static_assert(false, "Unhandle intrinsic");
#endif

    return Result;
  }

  llvm::Value *VSQXTUN2(llvm::Value *ArgLower, llvm::Value *ArgUpper, uint8_t RegisterSize, uint8_t ElementSize) {
#ifdef _M_X86_64
    // x86 handles the incoming lower register as a full 128bit, cast it back to ensure LLVM doesn't throw up
    // We don't care about the upper bits anyway
    ArgLower = CastVectorToType(ArgLower, true, RegisterSize, ElementSize >> 1);
#endif
    // Convert our upper args
    ArgUpper = VSQXTUN(ArgUpper, RegisterSize, ElementSize);

    std::vector<uint32_t> VectorMaskConstant;

    switch (ElementSize) {
      case 2:
        VectorMaskConstant = {
          0, 1, 2, 3, 4, 5, 6, 7,
          16, 17, 18, 19, 20, 21, 22, 23
        };
        break;
      case 4:
        VectorMaskConstant = {
          0, 1, 2, 3,
          8, 9, 10, 11
        };
        break;
      default: LogMan::Msg::A("Unhandled VSQXTUN2 size: %d", ElementSize); break;
    }

    // This will convert to VSQXTUN2 in AArch64
    return JITState.IRBuilder->CreateShuffleVector(ArgLower, ArgUpper, VectorMaskConstant);
  }

  llvm::Value *VADDP(llvm::Value *ArgLower, llvm::Value *ArgUpper, uint8_t RegisterSize, uint8_t ElementSize) {
#ifdef _M_X86_64
    // Will generate VPHADD
    uint8_t NumElements = RegisterSize / ElementSize;
    std::vector<llvm::Value*> Values;

    for (size_t i = 0; i < NumElements; ++i) {
      Values.emplace_back(JITState.IRBuilder->CreateExtractElement(ArgLower, i));;
    }

    for (size_t i = 0; i < NumElements; ++i) {
      Values.emplace_back(JITState.IRBuilder->CreateExtractElement(ArgUpper, i));;
    }

    for (size_t i = 0; i < NumElements; ++i) {
      Values[i] = JITState.IRBuilder->CreateAdd(Values[i*2], Values[i*2+1]);
    }
    // Cast to the type we want
    llvm::Value *Result = llvm::UndefValue::get(ArgLower->getType());

    for (size_t i = 0; i < NumElements; ++i) {
      Result = JITState.IRBuilder->CreateInsertElement(Result, Values[i], i);
    }
#elif _M_ARM_64
    // AArch64 doesn't seem to generate addp through the normal method
    std::vector<llvm::Type*> ArgTypes = {
      ArgLower->getType(),
    };

    std::vector<llvm::Value*> Args = {
      ArgLower, ArgUpper,
    };

    auto Result = JITState.IRBuilder->CreateIntrinsic(llvm::Intrinsic::aarch64_neon_addp, ArgTypes, Args);
#else
    static_assert(false, "Unhandle intrinsic");
#endif

    return Result;
  }

  llvm::CallInst *FSHL(llvm::Value *Val, llvm::Value *Val2, llvm::Value *Amt) {
    std::vector<llvm::Type*> ArgTypes = {
      Val->getType(),
    };
    std::vector<llvm::Value*> Args = {
      Val,
      Val2,
      Amt,
    };

    return JITState.IRBuilder->CreateIntrinsic(llvm::Intrinsic::fshl, ArgTypes, Args);
  }

  llvm::CallInst *FSHR(llvm::Value *Val, llvm::Value *Val2, llvm::Value *Amt) {
    std::vector<llvm::Type*> ArgTypes = {
      Val->getType(),
    };
    std::vector<llvm::Value*> Args = {
      Val,
      Val2,
      Amt,
    };

    return JITState.IRBuilder->CreateIntrinsic(llvm::Intrinsic::fshr, ArgTypes, Args);
  }
  llvm::CallInst *CycleCounter() {
#ifdef _M_X86_64
    return JITState.IRBuilder->CreateIntrinsic(llvm::Intrinsic::readcyclecounter, {}, {});
#elif _M_ARM_64
    // LLVM has readcyclecounter be a zero constant because it's trying to use PMCCNTR_EL0 on AArch64
    // We actually want to use the CNTVCT_EL0 register but there is no good way to encode this
    // Call out to a helper function that just uses it...
    // Could potentially add an intrinsic to LLVM for AArch64 to read system registers
    return JITState.IRBuilder->CreateCall(JITCurrentState.AArch64ReadCycleCounterFunction, {});
#else
    static_assert(false, "No way to read cycle counter");
#endif
  }

  llvm::CallInst *SQRT(llvm::Value *Arg) {
    std::vector<llvm::Type*> ArgTypes = {
      Arg->getType(),
    };
    std::vector<llvm::Value*> Args = {
      Arg,
    };

    return JITState.IRBuilder->CreateIntrinsic(llvm::Intrinsic::sqrt, ArgTypes, Args);
  }

  llvm::CallInst *SQADD(llvm::Value *Arg1, llvm::Value *Arg2) {
    std::vector<llvm::Type*> ArgTypes = {
      Arg1->getType(),
    };
    std::vector<llvm::Value*> Args = {
      Arg1,
      Arg2,
    };

    return JITState.IRBuilder->CreateIntrinsic(llvm::Intrinsic::sadd_sat, ArgTypes, Args);
  }
  llvm::CallInst *SQSUB(llvm::Value *Arg1, llvm::Value *Arg2) {
    std::vector<llvm::Type*> ArgTypes = {
      Arg1->getType(),
    };
    std::vector<llvm::Value*> Args = {
      Arg1,
      Arg2,
    };

    return JITState.IRBuilder->CreateIntrinsic(llvm::Intrinsic::ssub_sat, ArgTypes, Args);
  }

  llvm::CallInst *UQADD(llvm::Value *Arg1, llvm::Value *Arg2) {
    std::vector<llvm::Type*> ArgTypes = {
      Arg1->getType(),
    };
    std::vector<llvm::Value*> Args = {
      Arg1,
      Arg2,
    };

    return JITState.IRBuilder->CreateIntrinsic(llvm::Intrinsic::uadd_sat, ArgTypes, Args);
  }
  llvm::CallInst *UQSUB(llvm::Value *Arg1, llvm::Value *Arg2) {
    std::vector<llvm::Type*> ArgTypes = {
      Arg1->getType(),
    };
    std::vector<llvm::Value*> Args = {
      Arg1,
      Arg2,
    };

    return JITState.IRBuilder->CreateIntrinsic(llvm::Intrinsic::usub_sat, ArgTypes, Args);
  }

  void CreateDebugPrint(llvm::Value *Val) {
    std::vector<llvm::Value*> Args;
    Args.emplace_back(JITState.IRBuilder->getInt64(reinterpret_cast<uint64_t>(this)));
    Args.emplace_back(Val);
    if (Val->getType()->getIntegerBitWidth() > 64)
      JITState.IRBuilder->CreateCall(JITCurrentState.DebugPrint128, Args);
    else
      JITState.IRBuilder->CreateCall(JITCurrentState.DebugPrint, Args);
  }

  void CreateGlobalVariables(llvm::ExecutionEngine *Engine, llvm::Module *FunctionModule);

  llvm::Value *CastVectorToType(llvm::Value *Arg, bool Integer, uint8_t RegisterSize, uint8_t ElementSize);
  llvm::Value *CastScalarToType(llvm::Value *Arg, bool Integer, uint8_t RegisterSize, uint8_t ElementSize);
  llvm::Value *CastToOpaqueStructure(llvm::Value *Arg, llvm::Type *DstType);
  void SetDest(IR::OrderedNodeWrapper Op, llvm::Value *Val);
  llvm::Value *GetSrc(IR::OrderedNodeWrapper Src);

  DestMapType DestMap;
  FEXCore::IR::IRListView<true> const *CurrentIR;

  std::unordered_map<IR::OrderedNodeWrapper::NodeOffsetType, llvm::BasicBlock*> JumpTargets;

  // Target Machines
#ifdef _M_X86_64
  const std::string arch = "x86-64";
  const std::string cpu = "skylake";
  const llvm::Triple TargetTriple{"x86_64", "unknown", "linux", "gnu"};
#else
  const std::string arch = "aarch64";
  const std::string cpu = "cortex-a76";
  const llvm::Triple TargetTriple{"aarch64", "unknown", "linux", "gnu"};
#endif
  const llvm::SmallVector<std::string, 0> Attrs;
  llvm::TargetMachine *LLVMTarget;
};

LLVMJITCore::LLVMJITCore(FEXCore::Core::InternalThreadState *Thread)
  : ThreadState {Thread}
  , CTX {Thread->CTX} {

  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();

  JITState.ContextRef = LLVMContextCreate();
  Con = *llvm::unwrap(&JITState.ContextRef);
  JITState.MainModule = new llvm::Module("Main Module", *Con);
  JITState.IRBuilder = new llvm::IRBuilder<>(*Con);
  JITState.MainEngineBuilder = new llvm::EngineBuilder(std::unique_ptr<llvm::Module>(JITState.MainModule));
  JITState.MainEngineBuilder->setEngineKind(llvm::EngineKind::JIT);
  LLVMTarget = JITState.MainEngineBuilder->selectTarget(
      TargetTriple,
      arch, cpu, Attrs);

  JITState.MemManager = new LLVMMemoryManager();
  CTX->Config.LLVM_MemoryValidation = false;
#if !DESTMAP_AS_MAP
  DestMap.resize(0x1000);
#endif
}

LLVMJITCore::~LLVMJITCore() {
  // MainEngineBuilder takes overship of MainModule
  delete JITState.MainEngineBuilder;
  delete JITState.IRBuilder;
  // Causes fault when destroying MCJIT
  //for (auto Module : JITState.Functions) {
  //  delete Module;
  //}
  LLVMContextDispose(JITState.ContextRef);
}

void LLVMJITCore::ValidateMemoryInVM(uint64_t Ptr, uint8_t Size, bool Load) {
  uint64_t VirtualBase = CTX->MemoryMapper.GetBaseOffset<uint64_t>(0);
  uint64_t VirtualEnd = VirtualBase + (1ULL << 36ULL);
  if (Ptr < VirtualBase || (Ptr + Size) >= VirtualEnd) {
    LogMan::Msg::A("Invalid memory load at 0x%016lx. Wasn't within virtual range [0x%016lx, 0x%015lx)", Ptr, VirtualBase, VirtualEnd);
  }
 LogMan::Msg::D("%s guestmem: 0x%lx", Load ? "Loading from" : "Storing", Ptr - VirtualBase);
}

void LLVMJITCore::DebugPrint(uint64_t Val) {
  LogMan::Msg::I(">>>> Value in Arg: 0x%lx, %ld", Val, Val);
}

void LLVMJITCore::DebugPrint128(__uint128_t Val) {
  LogMan::Msg::I(">>>Val: %016lx, %016lx", static_cast<uint64_t>(Val >> 64), static_cast<uint64_t>(Val));
}

template<typename Type>
Type LLVMJITCore::MemoryLoad_Validate(uint64_t Ptr) {
  ValidateMemoryInVM(Ptr, sizeof(Type), true);
  Type *TypedAddr = reinterpret_cast<Type*>(Ptr);
  Type Ret = TypedAddr[0];
  uint64_t Data;
  memcpy(&Data, &Ret, sizeof(Data));
  LogMan::Msg::D("\tLoading: 0x%016lx", Data);
  return Ret;
}

template<typename Type>
void LLVMJITCore::MemoryStore_Validate(uint64_t Ptr, Type Val) {
  ValidateMemoryInVM(Ptr, sizeof(Type), false);
  Type *TypedAddr = reinterpret_cast<Type*>(Ptr);
  TypedAddr[0] = Val;
  uint64_t Data;
  memcpy(&Data, &Val, sizeof(Data));
  LogMan::Msg::D("\tStoring: 0x%016lx", Data);
}

llvm::Value *LLVMJITCore::CreateMemoryLoad(llvm::Value *Ptr, uint8_t Align) {
  if (CTX->Config.LLVM_MemoryValidation) {
    std::vector<llvm::Value*> Args;
    Args.emplace_back(JITState.IRBuilder->getInt64(reinterpret_cast<uint64_t>(this)));
    Args.emplace_back(Ptr);

    unsigned PtrSize = Ptr->getType()->getPointerElementType()->getIntegerBitWidth();
    switch (PtrSize) {
    case 8: return JITState.IRBuilder->CreateCall(JITCurrentState.ValidateLoad8, Args);
    case 16: return JITState.IRBuilder->CreateCall(JITCurrentState.ValidateLoad16, Args);
    case 32: return JITState.IRBuilder->CreateCall(JITCurrentState.ValidateLoad32, Args);
    case 64: return JITState.IRBuilder->CreateCall(JITCurrentState.ValidateLoad64, Args);
    case 128: return JITState.IRBuilder->CreateCall(JITCurrentState.ValidateLoad128, Args);
    default: LogMan::Msg::A("Unknown Load Size: %d", PtrSize); break;
    }
  }

  return JITState.IRBuilder->CreateAlignedLoad(Ptr, Align);
}

void LLVMJITCore::CreateMemoryStore(llvm::Value *Ptr, llvm::Value *Val, uint8_t Align) {
  if (CTX->Config.LLVM_MemoryValidation) {
    std::vector<llvm::Value*> Args;
    Args.emplace_back(JITState.IRBuilder->getInt64(reinterpret_cast<uint64_t>(this)));
    Args.emplace_back(Ptr);
    Args.emplace_back(Val);

    unsigned PtrSize = Ptr->getType()->getPointerElementType()->getIntegerBitWidth();
    switch (PtrSize) {
    case 8: JITState.IRBuilder->CreateCall(JITCurrentState.ValidateStore8, Args); break;
    case 16: JITState.IRBuilder->CreateCall(JITCurrentState.ValidateStore16, Args); break;
    case 32: JITState.IRBuilder->CreateCall(JITCurrentState.ValidateStore32, Args); break;
    case 64: JITState.IRBuilder->CreateCall(JITCurrentState.ValidateStore64, Args); break;
    case 128: JITState.IRBuilder->CreateCall(JITCurrentState.ValidateStore128, Args); break;
    default: LogMan::Msg::A("Unknown Store Size: %d", PtrSize); break;
    }
    return;
  }

  JITState.IRBuilder->CreateAlignedStore(Val, Ptr, Align);
}


void LLVMJITCore::CreateGlobalVariables(llvm::ExecutionEngine *Engine, llvm::Module *FunctionModule) {
  using namespace llvm;
  Type *voidTy = Type::getVoidTy(*Con);
  Type *i8 = Type::getInt8Ty(*Con);
  Type *i16 = Type::getInt16Ty(*Con);
  Type *i32 = Type::getInt32Ty(*Con);
  Type *i64 = Type::getInt64Ty(*Con);
  Type *i128 = Type::getInt128Ty(*Con);

  // Syscall Function
  {
    auto FuncType = FunctionType::get(i64,
      {
        i64, // Technically a this pointer
        i64,
        ArrayType::get(i64, 7)->getPointerTo(),
      },
      false);
    JITCurrentState.SyscallFunction = Function::Create(FuncType,
      Function::ExternalLinkage,
      "Syscall",
      FunctionModule);
    using ClassPtrType = uint64_t (*)(FEXCore::SyscallHandler*, FEXCore::Core::InternalThreadState *, FEXCore::HLE::SyscallArguments *);
    union PtrCast {
      ClassPtrType ClassPtr;
      void* Data;
    };
    PtrCast Ptr;
    Ptr.ClassPtr = &FEXCore::HandleSyscall;
    Engine->addGlobalMapping(JITCurrentState.SyscallFunction, Ptr.Data);
  }

  // CPUID Function
  {
    auto FuncType = FunctionType::get(voidTy,
      {
        ArrayType::get(i32, 4)->getPointerTo(),
        i64, // Technically this is a pointer
        i32, // CPUID Function
      },
      false);
    JITCurrentState.CPUIDFunction = Function::Create(FuncType,
      Function::ExternalLinkage,
      "CPUID",
      FunctionModule);
    using ClassPtrType = void (*)(FEXCore::CPUIDEmu::FunctionResults*, FEXCore::CPUIDEmu*, uint32_t);
    union PtrCast {
      ClassPtrType ClassPtr;
      void* Data;
    };
    PtrCast Ptr;
    Ptr.ClassPtr = &CPUIDRun_Thunk;
    Engine->addGlobalMapping(JITCurrentState.CPUIDFunction, Ptr.Data);
  }

#if defined(_M_ARM_64) && !defined(AARCH64_ON_X86)
  // AArch64ReadCycleCounter Function
  {
    auto FuncType = FunctionType::get(i64,
      { },
      false);
    JITCurrentState.AArch64ReadCycleCounterFunction = Function::Create(FuncType,
      Function::ExternalLinkage,
      "AArch64ReadCycleCounter",
      FunctionModule);
    using ClassPtrType = uint64_t (*)();
    union PtrCast {
      ClassPtrType ClassPtr;
      void* Data;
    };
    PtrCast Ptr;
    Ptr.ClassPtr = &AArch64ReadCycleCounter;
    Engine->addGlobalMapping(JITCurrentState.AArch64ReadCycleCounterFunction, Ptr.Data);
  }
#endif

  // Exit VM function
  {
    auto FuncType = FunctionType::get(voidTy,
    {
      i64, // Technically this is a pointer
    },
    false);
    JITCurrentState.ExitVMFunction = Function::Create(FuncType,
      Function::ExternalLinkage,
      "ExitVM",
      FunctionModule);
    using ClassPtrType = void (*)(FEXCore::Core::InternalThreadState *Thread);
    union PtrCast {
      ClassPtrType ClassPtr;
      void* Data;
    };
    PtrCast Ptr;
    Ptr.ClassPtr = &SetExitState_Thunk;
    Engine->addGlobalMapping(JITCurrentState.ExitVMFunction, Ptr.Data);
  }

  if (CTX->Config.LLVM_MemoryValidation) {
    // Memory validate load 8
    {
      auto FuncType = FunctionType::get(i8,
        {i64, // this pointer
        i8->getPointerTo()}, false);
      JITCurrentState.ValidateLoad8 = Function::Create(FuncType,
        Function::ExternalLinkage,
        "LoadValidate8",
        FunctionModule);
      using ClassPtrType = uint8_t (LLVMJITCore::*)(uint64_t);
      union PtrCast {
        ClassPtrType ClassPtr;
        void* Data;
      };
      PtrCast Ptr;
      Ptr.ClassPtr = &LLVMJITCore::MemoryLoad_Validate<uint8_t>;
      Engine->addGlobalMapping(JITCurrentState.ValidateLoad8, Ptr.Data);
    }
    // Memory validate load 16
    {
      auto FuncType = FunctionType::get(i16,
        {i64, // this pointer
        i16->getPointerTo()}, false);
      JITCurrentState.ValidateLoad16 = Function::Create(FuncType,
        Function::ExternalLinkage,
        "LoadValidate16",
        FunctionModule);
      using ClassPtrType = uint16_t (LLVMJITCore::*)(uint64_t);
      union PtrCast {
        ClassPtrType ClassPtr;
        void* Data;
      };
      PtrCast Ptr;
      Ptr.ClassPtr = &LLVMJITCore::MemoryLoad_Validate<uint16_t>;
      Engine->addGlobalMapping(JITCurrentState.ValidateLoad16, Ptr.Data);
    }
    // Memory validate load 32
    {
      auto FuncType = FunctionType::get(i32,
        {i64, // this pointer
        i32->getPointerTo()}, false);

      JITCurrentState.ValidateLoad32 = Function::Create(FuncType,
        Function::ExternalLinkage,
        "LoadValidate32",
        FunctionModule);
      using ClassPtrType = uint32_t (LLVMJITCore::*)(uint64_t);
      union PtrCast {
        ClassPtrType ClassPtr;
        void* Data;
      };
      PtrCast Ptr;
      Ptr.ClassPtr = &LLVMJITCore::MemoryLoad_Validate<uint32_t>;
      Engine->addGlobalMapping(JITCurrentState.ValidateLoad32, Ptr.Data);
    }
    // Memory validate load 64
    {
      auto FuncType = FunctionType::get(i64,
        {i64, // this pointer
        i64->getPointerTo()}, false);
      JITCurrentState.ValidateLoad64 = Function::Create(FuncType,
        Function::ExternalLinkage,
        "LoadValidate64",
        FunctionModule);
      using ClassPtrType = uint64_t (LLVMJITCore::*)(uint64_t);
      union PtrCast {
        ClassPtrType ClassPtr;
        void* Data;
      };
      PtrCast Ptr;
      Ptr.ClassPtr = &LLVMJITCore::MemoryLoad_Validate<uint64_t>;
      Engine->addGlobalMapping(JITCurrentState.ValidateLoad64, Ptr.Data);
    }
    // Memory validate load 128
    {
      auto FuncType = FunctionType::get(i128,
        {i64, // this pointer
        i128->getPointerTo()}, false);
      JITCurrentState.ValidateLoad128 = Function::Create(FuncType,
        Function::ExternalLinkage,
        "LoadValidate128",
        FunctionModule);
      using ClassPtrType = __uint128_t (LLVMJITCore::*)(uint64_t);
      union PtrCast {
        ClassPtrType ClassPtr;
        void* Data;
      };
      PtrCast Ptr;
      Ptr.ClassPtr = &LLVMJITCore::MemoryLoad_Validate<__uint128_t>;
      Engine->addGlobalMapping(JITCurrentState.ValidateLoad128, Ptr.Data);
    }

    // Memory validate Store 8
    {
      auto FuncType = FunctionType::get(voidTy,
        {i64, // this pointer
        i8->getPointerTo(),
        i8}, false);
      JITCurrentState.ValidateStore8 = Function::Create(FuncType,
        Function::ExternalLinkage,
        "StoreValidate8",
        FunctionModule);
      using ClassPtrType = void (LLVMJITCore::*)(uint64_t, uint8_t);
      union PtrCast {
        ClassPtrType ClassPtr;
        void* Data;
      };
      PtrCast Ptr;
      Ptr.ClassPtr = &LLVMJITCore::MemoryStore_Validate<uint8_t>;
      Engine->addGlobalMapping(JITCurrentState.ValidateStore8, Ptr.Data);
    }

    // Memory validate Store 16
    {
      auto FuncType = FunctionType::get(voidTy,
        {i64, // this pointer
        i16->getPointerTo(),
        i16}, false);
      JITCurrentState.ValidateStore16 = Function::Create(FuncType,
        Function::ExternalLinkage,
        "StoreValidate16",
        FunctionModule);
      using ClassPtrType = void (LLVMJITCore::*)(uint64_t, uint16_t);
      union PtrCast {
        ClassPtrType ClassPtr;
        void* Data;
      };
      PtrCast Ptr;
      Ptr.ClassPtr = &LLVMJITCore::MemoryStore_Validate<uint16_t>;
      Engine->addGlobalMapping(JITCurrentState.ValidateStore16, Ptr.Data);
    }

    // Memory validate Store 32
    {
      auto FuncType = FunctionType::get(voidTy,
        {i64, // this pointer
        i32->getPointerTo(),
        i32}, false);
      JITCurrentState.ValidateStore32 = Function::Create(FuncType,
        Function::ExternalLinkage,
        "StoreValidate32",
        FunctionModule);
      using ClassPtrType = void (LLVMJITCore::*)(uint64_t, uint32_t);
      union PtrCast {
        ClassPtrType ClassPtr;
        void* Data;
      };
      PtrCast Ptr;
      Ptr.ClassPtr = &LLVMJITCore::MemoryStore_Validate<uint32_t>;
      Engine->addGlobalMapping(JITCurrentState.ValidateStore32, Ptr.Data);
    }

    // Memory validate Store 64
    {
      auto FuncType = FunctionType::get(voidTy,
        {i64, // this pointer
        i64->getPointerTo(),
        i64}, false);
      JITCurrentState.ValidateStore64 = Function::Create(FuncType,
        Function::ExternalLinkage,
        "StoreValidate64",
        FunctionModule);
      using ClassPtrType = void (LLVMJITCore::*)(uint64_t, uint64_t);
      union PtrCast {
        ClassPtrType ClassPtr;
        void* Data;
      };
      PtrCast Ptr;
      Ptr.ClassPtr = &LLVMJITCore::MemoryStore_Validate<uint64_t>;
      Engine->addGlobalMapping(JITCurrentState.ValidateStore64, Ptr.Data);
    }

    // Memory validate Store 128
    {
      auto FuncType = FunctionType::get(voidTy,
        {i64, // this pointer
        i128->getPointerTo(),
        i128}, false);
      JITCurrentState.ValidateStore128 = Function::Create(FuncType,
        Function::ExternalLinkage,
        "StoreValidate128",
        FunctionModule);
      using ClassPtrType = void (LLVMJITCore::*)(uint64_t, __uint128_t);
      union PtrCast {
        ClassPtrType ClassPtr;
        void* Data;
      };
      PtrCast Ptr;
      Ptr.ClassPtr = &LLVMJITCore::MemoryStore_Validate<__uint128_t>;
      Engine->addGlobalMapping(JITCurrentState.ValidateStore128, Ptr.Data);
    }
  }

  // Value Print
  {
    auto FuncType = FunctionType::get(voidTy,
      {i64, // this pointer
      i64}, false);
    JITCurrentState.DebugPrint = Function::Create(FuncType,
      Function::ExternalLinkage,
      "PrintVal",
      FunctionModule);
    using ClassPtrType = void (LLVMJITCore::*)(uint64_t);
    union PtrCast {
      ClassPtrType ClassPtr;
      void* Data;
    };
    PtrCast Ptr;
    Ptr.ClassPtr = &LLVMJITCore::DebugPrint;
    Engine->addGlobalMapping(JITCurrentState.DebugPrint, Ptr.Data);
  }

  // Value Print 128
  {
    auto FuncType = FunctionType::get(voidTy,
      {i64, // this pointer
      i128}, false);
    JITCurrentState.DebugPrint128 = Function::Create(FuncType,
      Function::ExternalLinkage,
      "PrintVal128",
      FunctionModule);
    using ClassPtrType = void (LLVMJITCore::*)(__uint128_t);
    union PtrCast {
      ClassPtrType ClassPtr;
      void* Data;
    };
    PtrCast Ptr;
    Ptr.ClassPtr = &LLVMJITCore::DebugPrint128;
    Engine->addGlobalMapping(JITCurrentState.DebugPrint128, Ptr.Data);
  }

  // JIT State
  {
    JITCurrentState.CPUStateType = StructType::create(*Con,
      {
        i64, // RIP
        ArrayType::get(i64, 16), // Gregs
        i64, // Pad to ensure alignment
        ArrayType::get(i128, 16), // XMMs
        i64, i64, // GS, FS
        ArrayType::get(i8, 48), //rflags
        ArrayType::get(i128, 8), // MMs
      },
      "CPUStateType");

    FunctionModule->getOrInsertGlobal("X86State::State", JITCurrentState.CPUStateType->getPointerTo());
    JITCurrentState.CPUStateVar = FunctionModule->getNamedGlobal("X86State::State");
    JITCurrentState.CPUStateVar->setConstant(true);
    JITCurrentState.CPUStateVar->setInitializer(
      ConstantInt::getIntegerValue(
        JITCurrentState.CPUStateType->getPointerTo(),
        APInt(64, reinterpret_cast<uint64_t>(&ThreadState->State))));
    JITCurrentState.CPUState = JITState.IRBuilder->CreateLoad(JITCurrentState.CPUStateVar, false, "X86State::State::Local");
  }
}

llvm::Value *LLVMJITCore::CreateContextGEP(uint64_t Offset, uint8_t Size) {
  std::vector<llvm::Value*> GEPValues = {
    JITState.IRBuilder->getInt32(0), // First value in the pointer to CPUState
  };

  if (Offset == 0) { // RIP
    if (Size != 8) return nullptr;
    GEPValues.emplace_back(JITState.IRBuilder->getInt32(0));
  }
  else if (Offset >= offsetof(FEXCore::Core::CPUState, gregs) && Offset < offsetof(FEXCore::Core::CPUState, xmm)) {
    if (Size != 8 || Offset % 8 != 0) return nullptr;
    GEPValues.emplace_back(JITState.IRBuilder->getInt32(1));
    GEPValues.emplace_back(JITState.IRBuilder->getInt32((Offset - offsetof(FEXCore::Core::CPUState, gregs)) / 8));
  }
  else if (Offset >= offsetof(FEXCore::Core::CPUState, xmm) && Offset < offsetof(FEXCore::Core::CPUState, gs)) {
    if (Size != 16 || Offset % 16 != 0) return nullptr;
    GEPValues.emplace_back(JITState.IRBuilder->getInt32(3));
    GEPValues.emplace_back(JITState.IRBuilder->getInt32((Offset - offsetof(FEXCore::Core::CPUState, xmm)) / 16));
  }
  else if (Offset == offsetof(FEXCore::Core::CPUState, gs)) {
    if (Size != 8) return nullptr;
    GEPValues.emplace_back(JITState.IRBuilder->getInt32(4));
  }
  else if (Offset == offsetof(FEXCore::Core::CPUState, fs)) {
    if (Size != 8) return nullptr;
    GEPValues.emplace_back(JITState.IRBuilder->getInt32(5));
  }
  else if (Offset >= offsetof(FEXCore::Core::CPUState, flags)) {
    if (Size != 1) return nullptr;
    GEPValues.emplace_back(JITState.IRBuilder->getInt32(6));
    GEPValues.emplace_back(JITState.IRBuilder->getInt32(Offset - offsetof(FEXCore::Core::CPUState, flags[0])));
  }
  else
    LogMan::Msg::A("Unknown X86State GEP: 0x%lx", Offset);

  return JITState.IRBuilder->CreateGEP(JITCurrentState.CPUState, GEPValues, "Context::Value");
}

llvm::Value *LLVMJITCore::CreateContextPtr(uint64_t Offset, uint8_t Size) {
  llvm::Type *i8 = llvm::Type::getInt8Ty(*Con);
  llvm::Type *i16 = llvm::Type::getInt16Ty(*Con);
  llvm::Type *i32 = llvm::Type::getInt32Ty(*Con);
  llvm::Type *i64 = llvm::Type::getInt64Ty(*Con);
  llvm::Type *i128 = llvm::Type::getInt128Ty(*Con);

  // Let's try to create our pointer with GEP
  // This can only happen if we are a full value from the context and is aligned correctly
  llvm::Value *GEPResult = CreateContextGEP(Offset, Size);
  if (GEPResult) return GEPResult;

  llvm::Value *StateBasePtr = JITState.IRBuilder->CreatePtrToInt(JITCurrentState.CPUState, i64);
  StateBasePtr = JITState.IRBuilder->CreateAdd(StateBasePtr, JITState.IRBuilder->getInt64(Offset));

  // Convert back to pointer of correct size
  switch (Size) {
  case 1:  return JITState.IRBuilder->CreateIntToPtr(StateBasePtr, i8->getPointerTo());
  case 2:  return JITState.IRBuilder->CreateIntToPtr(StateBasePtr, i16->getPointerTo());
  case 4:  return JITState.IRBuilder->CreateIntToPtr(StateBasePtr, i32->getPointerTo());
  case 8:  return JITState.IRBuilder->CreateIntToPtr(StateBasePtr, i64->getPointerTo());
  case 16: return JITState.IRBuilder->CreateIntToPtr(StateBasePtr, i128->getPointerTo());
  default: LogMan::Msg::A("Unknown context pointer size: %d", Size); break;
  }
  return nullptr;
}

llvm::Value *LLVMJITCore::CreateIndexedContextPtr(llvm::Value *Index, uint64_t Offset, uint8_t Size, uint8_t Stride) {
  llvm::Type *i8 = llvm::Type::getInt8Ty(*Con);
  llvm::Type *i16 = llvm::Type::getInt16Ty(*Con);
  llvm::Type *i32 = llvm::Type::getInt32Ty(*Con);
  llvm::Type *i64 = llvm::Type::getInt64Ty(*Con);
  llvm::Type *i128 = llvm::Type::getInt128Ty(*Con);

  llvm::Value *StateBasePtr = JITState.IRBuilder->CreatePtrToInt(JITCurrentState.CPUState, i64);
  Index = JITState.IRBuilder->CreateMul(JITState.IRBuilder->CreateZExt(Index, i64), JITState.IRBuilder->getInt64(Stride));
  StateBasePtr = JITState.IRBuilder->CreateAdd(Index, StateBasePtr);
  StateBasePtr = JITState.IRBuilder->CreateAdd(StateBasePtr, JITState.IRBuilder->getInt64(Offset));

  // Convert back to pointer of correct size
  switch (Size) {
  case 1:  return JITState.IRBuilder->CreateIntToPtr(StateBasePtr, i8->getPointerTo());
  case 2:  return JITState.IRBuilder->CreateIntToPtr(StateBasePtr, i16->getPointerTo());
  case 4:  return JITState.IRBuilder->CreateIntToPtr(StateBasePtr, i32->getPointerTo());
  case 8:  return JITState.IRBuilder->CreateIntToPtr(StateBasePtr, i64->getPointerTo());
  case 16: return JITState.IRBuilder->CreateIntToPtr(StateBasePtr, i128->getPointerTo());
  default: LogMan::Msg::A("Unknown context pointer size: %d", Size); break;
  }
  return nullptr;
}

llvm::Value *LLVMJITCore::CastVectorToType(llvm::Value *Arg, bool Integer, uint8_t RegisterSize, uint8_t ElementSize) {
  uint8_t DestSizeInBits = RegisterSize * ElementSize * 8;
  uint8_t NumElements = RegisterSize / ElementSize;
  llvm::Type *ElementType;
  if (Integer) {
    ElementType = llvm::Type::getIntNTy(*Con, ElementSize * 8);
  }
  else {
    if (ElementSize == 4) {
      ElementType = llvm::Type::getFloatTy(*Con);
    }
    else {
      ElementType = llvm::Type::getDoubleTy(*Con);
    }
  }

  llvm::Type *VectorType = llvm::VectorType::get(ElementType, NumElements);
  auto SrcType = Arg->getType();

  // If the source type is not vector(frequent with opaque types) and it matches the destination size then we can just bitcast it
	if (!SrcType->isVectorTy() && SrcType->getPrimitiveSizeInBits() == DestSizeInBits) {
    return JITState.IRBuilder->CreateBitCast(Arg, VectorType);
  }

  uint8_t NumSrcElements = SrcType->getPrimitiveSizeInBits() / (ElementSize * 8);
  llvm::Type *SrcVectorType = llvm::VectorType::get(ElementType, NumSrcElements);

  // First thing we need to cast the argument type to a vector size that is supported by the source type
  Arg = JITState.IRBuilder->CreateBitCast(Arg, SrcVectorType);

  llvm::Value *Undef = llvm::UndefValue::get(SrcVectorType);
  std::vector<uint32_t> Mask;
  // We then need to shuffle the elements to match the size we want
  // If the destination size is smaller then we can just shuffle the low elements
  // If our destination is more than the number of source elements then we need to pull in some undef
  for (uint32_t i = 0; i < NumElements; ++i)
    Mask.emplace_back(i);

  return JITState.IRBuilder->CreateShuffleVector(Arg, Undef, Mask);
}

llvm::Value *LLVMJITCore::CastScalarToType(llvm::Value *Arg, bool Integer, uint8_t RegisterSize, uint8_t ElementSize) {
  llvm::Type *ElementType;
  if (Integer) {
    ElementType = llvm::Type::getIntNTy(*Con, ElementSize * 8);
  }
  else {
    if (ElementSize == 4) {
      ElementType = llvm::Type::getFloatTy(*Con);
    }
    else {
      ElementType = llvm::Type::getDoubleTy(*Con);
    }
  }

  return JITState.IRBuilder->CreateBitCast(Arg, ElementType);
}

llvm::Value *LLVMJITCore::CastToOpaqueStructure(llvm::Value *Arg, llvm::Type *DstType) {
  if (Arg->getType()->isVectorTy()) {
    // First do a bitcast from the vector type to the same size integer
    unsigned ElementSize = Arg->getType()->getVectorElementType()->getPrimitiveSizeInBits();
    unsigned NumElements = Arg->getType()->getVectorNumElements();
    auto NewIntegerType = llvm::Type::getIntNTy(*Con, ElementSize * NumElements);
    Arg = JITState.IRBuilder->CreateBitCast(Arg, NewIntegerType);
  }

  return JITState.IRBuilder->CreateZExtOrTrunc(Arg, DstType);
}

void LLVMJITCore::SetDest(IR::OrderedNodeWrapper Op, llvm::Value *Val) {
  DestMap[Op.ID()] = Val;
}

llvm::Value *LLVMJITCore::GetSrc(IR::OrderedNodeWrapper Src) {
#if DESTMAP_AS_MAP
  LogMan::Throw::A(DestMap.find(Src.ID()) != DestMap.end(), "Op had Src but wasn't added to the dest map");
#endif

  auto DstPtr = DestMap[Src.ID()];
  LogMan::Throw::A(DstPtr != nullptr, "Destmap had slot but wasn't allocated memory");
  return DstPtr;
}

void LLVMJITCore::HandleIR(FEXCore::IR::IRListView<true> const *IR, IR::NodeWrapperIterator *Node) {
  using namespace llvm;

  uintptr_t ListBegin = CurrentIR->GetListData();
  uintptr_t DataBegin = CurrentIR->GetData();

  IR::OrderedNodeWrapper *WrapperOp = (*Node)();
  IR::OrderedNode *RealNode = WrapperOp->GetNode(ListBegin);
  FEXCore::IR::IROp_Header *IROp = RealNode->Op(DataBegin);
  uint8_t OpSize = IROp->Size;

  switch (IROp->Op) {
    case IR::OP_ENDBLOCK: {
      auto Op = IROp->C<IR::IROp_EndBlock>();

      if (Op->RIPIncrement) {
        auto DownCountValue = JITState.IRBuilder->CreateGEP(JITCurrentState.CPUState,
          {
            JITState.IRBuilder->getInt32(0),
            JITState.IRBuilder->getInt32(0),
          },
          "RIPIncrement");
        auto LoadRIP = JITState.IRBuilder->CreateLoad(DownCountValue);
        auto NewValue = JITState.IRBuilder->CreateAdd(LoadRIP, JITState.IRBuilder->getInt64(Op->RIPIncrement));
        JITState.IRBuilder->CreateStore(NewValue, DownCountValue);
      }
    break;
    }
    case IR::OP_BREAK: {
      std::vector<llvm::Value*> Args;
      // We need to pull this argument from the ExecuteCodeFunction
      Args.emplace_back(Func->args().begin());

      JITState.IRBuilder->CreateCall(JITCurrentState.ExitVMFunction, Args);
      JITState.IRBuilder->CreateBr(JITCurrentState.ExitBlock);
    break;
    }
    case IR::OP_EXITFUNCTION: {
      JITState.IRBuilder->CreateBr(JITCurrentState.ExitBlock);
    break;
    }
    case IR::OP_JUMP: {
      auto Op = IROp->C<IR::IROp_Jump>();
      JITState.IRBuilder->CreateBr(JumpTargets[Op->Header.Args[0].ID()]);
    break;
    }
    case IR::OP_CONDJUMP: {
      auto Op = IROp->C<IR::IROp_CondJump>();
      auto Cond = GetSrc(Op->Header.Args[0]);

      llvm::Value *Zero = JITState.IRBuilder->getInt64(0);
      Cond = JITState.IRBuilder->CreateZExtOrTrunc(Cond, Zero->getType());

      auto Comp = JITState.IRBuilder->CreateICmpNE(Cond, Zero);
      JITState.IRBuilder->CreateCondBr(Comp, JumpTargets[Op->Header.Args[1].ID()], JumpTargets[Op->Header.Args[2].ID()]);
    break;
    }
    case IR::OP_MOV: {
      auto Op = IROp->C<IR::IROp_Mov>();
      auto Src = GetSrc(Op->Header.Args[0]);
      SetDest(*WrapperOp, Src);
    break;
    }
    case IR::OP_SELECT: {
      auto Op = IROp->C<IR::IROp_Select>();
      auto Src1 = GetSrc(Op->Header.Args[0]);
      auto Src2 = GetSrc(Op->Header.Args[1]);

      auto ArgTrue = GetSrc(Op->Header.Args[2]);
      auto ArgFalse = GetSrc(Op->Header.Args[3]);

      Src2 = JITState.IRBuilder->CreateZExtOrTrunc(Src2, Src1->getType());
      ArgFalse = JITState.IRBuilder->CreateZExtOrTrunc(ArgFalse, ArgTrue->getType());

      Value *Cmp{};
      switch (Op->Cond.Val) {
      case FEXCore::IR::COND_EQ:
        Cmp = JITState.IRBuilder->CreateICmpEQ(Src1, Src2);
      break;
      case FEXCore::IR::COND_NEQ:
        Cmp = JITState.IRBuilder->CreateICmpNE(Src1, Src2);
      break;
      case FEXCore::IR::COND_SGE:
        Cmp = JITState.IRBuilder->CreateICmpSGE(Src1, Src2);
      break;
      case FEXCore::IR::COND_SLT:
        Cmp = JITState.IRBuilder->CreateICmpSLT(Src1, Src2);
      break;
      case FEXCore::IR::COND_SGT:
        Cmp = JITState.IRBuilder->CreateICmpSGT(Src1, Src2);
      break;
      case FEXCore::IR::COND_SLE:
        Cmp = JITState.IRBuilder->CreateICmpSLE(Src1, Src2);
      break;
      case FEXCore::IR::COND_UGE:
        Cmp = JITState.IRBuilder->CreateICmpUGE(Src1, Src2);
      break;
      case FEXCore::IR::COND_UGT:
        Cmp = JITState.IRBuilder->CreateICmpUGT(Src1, Src2);
      break;
      case FEXCore::IR::COND_ULT:
        Cmp = JITState.IRBuilder->CreateICmpULT(Src1, Src2);
      break;
      case FEXCore::IR::COND_ULE:
        Cmp = JITState.IRBuilder->CreateICmpULE(Src1, Src2);
      break;
      default: LogMan::Msg::A("Unknown Select Op Type: %d", Op->Cond); break;
      }

      auto Result = JITState.IRBuilder->CreateSelect(Cmp, ArgTrue, ArgFalse);
      SetDest(*WrapperOp, Result);
    break;
    }
    case FEXCore::IR::IROps::OP_CONSTANT: {
      auto Op = IROp->C<IR::IROp_Constant>();
      auto Result = JITState.IRBuilder->getInt64(Op->Constant);
      SetDest(*WrapperOp, Result);
    break;
    }
    case FEXCore::IR::IROps::OP_SYSCALL: {
      auto Op = IROp->C<IR::IROp_Syscall>();

      std::vector<llvm::Value*> Args;
      Args.emplace_back(JITState.IRBuilder->getInt64(reinterpret_cast<uint64_t>(CTX->SyscallHandler)));
      // We need to pull this argument from the ExecuteCodeFunction
      Args.emplace_back(Func->args().begin());

      uint8_t NumArgs{};
      for (; NumArgs < FEXCore::HLE::SyscallArguments::MAX_ARGS; ++NumArgs) {
        if (Op->Header.Args[NumArgs].IsInvalid()) break;
      }

      auto LLVMArgs = JITState.IRBuilder->CreateAlloca(ArrayType::get(Type::getInt64Ty(*Con), FEXCore::HLE::SyscallArguments::MAX_ARGS));
      for (unsigned i = 0; i < NumArgs; ++i) {
        auto Location = JITState.IRBuilder->CreateGEP(LLVMArgs,
            {
              JITState.IRBuilder->getInt32(0),
              JITState.IRBuilder->getInt32(i),
            },
            "Arg");
        auto Src = GetSrc(Op->Header.Args[i]);
        JITState.IRBuilder->CreateStore(Src, Location);
      }
      Args.emplace_back(LLVMArgs);

      auto Result = JITState.IRBuilder->CreateCall(JITCurrentState.SyscallFunction, Args);
      SetDest(*WrapperOp, Result);
    break;
    }
    case IR::OP_CPUID: {
      auto Op = IROp->C<IR::IROp_CPUID>();
      auto Src = GetSrc(Op->Header.Args[0]);
      std::vector<llvm::Value*> Args{};

      auto ReturnType = ArrayType::get(Type::getInt32Ty(*Con), 4);
      auto LLVMArgs = JITState.IRBuilder->CreateAlloca(ReturnType);
      Args.emplace_back(LLVMArgs);
      Args.emplace_back(JITState.IRBuilder->getInt64(reinterpret_cast<uint64_t>(&CTX->CPUID)));
      Args.emplace_back(Src);
      JITState.IRBuilder->CreateCall(JITCurrentState.CPUIDFunction, Args);
      auto Result = JITState.IRBuilder->CreateLoad(ReturnType, LLVMArgs);
      SetDest(*WrapperOp, Result);
    break;
    }
    case IR::OP_LOADCONTEXT: {
      auto Op = IROp->C<IR::IROp_LoadContext>();
      auto Value = CreateContextPtr(Op->Offset, Op->Size);
      llvm::Value *Load;
      if ((Op->Offset % Op->Size) == 0)
        Load = JITState.IRBuilder->CreateAlignedLoad(Value, Op->Size);
      else
        Load = JITState.IRBuilder->CreateLoad(Value);
      SetDest(*WrapperOp, Load);
      break;
    }
    case IR::OP_LOADCONTEXTINDEXED: {
      auto Op = IROp->C<IR::IROp_LoadContextIndexed>();
      auto Index = GetSrc(Op->Header.Args[0]);

      auto Value = CreateIndexedContextPtr(Index, Op->BaseOffset, Op->Size, Op->Stride);
      llvm::Value *Load;
      if ((Op->BaseOffset % Op->Size) == 0)
        Load = JITState.IRBuilder->CreateAlignedLoad(Value, Op->Size);
      else
        Load = JITState.IRBuilder->CreateLoad(Value);
      SetDest(*WrapperOp, Load);
    break;
    }
    case IR::OP_STORECONTEXT: {
      auto Op = IROp->C<IR::IROp_StoreContext>();
      auto Src = GetSrc(Op->Header.Args[0]);
      auto Value = CreateContextPtr(Op->Offset, Op->Size);

      Src = CastToOpaqueStructure(Src, Type::getIntNTy(*Con, Op->Size * 8));

      if ((Op->Offset % Op->Size) == 0)
        JITState.IRBuilder->CreateAlignedStore(Src, Value, Op->Size);
      else
        JITState.IRBuilder->CreateStore(Src, Value);
      break;
    }
    case IR::OP_STORECONTEXTINDEXED: {
      auto Op = IROp->C<IR::IROp_StoreContextIndexed>();
      auto Src = GetSrc(Op->Header.Args[0]);
      auto Index = GetSrc(Op->Header.Args[1]);
      auto Value = CreateIndexedContextPtr(Index, Op->BaseOffset, Op->Size, Op->Stride);

      Src = CastToOpaqueStructure(Src, Type::getIntNTy(*Con, Op->Size * 8));

      if ((Op->BaseOffset % Op->Size) == 0)
        JITState.IRBuilder->CreateAlignedStore(Src, Value, Op->Size);
      else
        JITState.IRBuilder->CreateStore(Src, Value);
    break;
    }

    case IR::OP_LOADFLAG: {
      auto Op = IROp->C<IR::IROp_LoadFlag>();
      auto Value = CreateContextPtr(offsetof(FEXCore::Core::CPUState, flags) + Op->Flag, 1);
      auto Load = JITState.IRBuilder->CreateLoad(Value);
      SetDest(*WrapperOp, Load);
    break;
    }
    case IR::OP_STOREFLAG: {
      auto Op = IROp->C<IR::IROp_StoreFlag>();
      auto Src = GetSrc(Op->Header.Args[0]);
      auto Value = CreateContextPtr(offsetof(FEXCore::Core::CPUState, flags) + Op->Flag, 1);
      Src = JITState.IRBuilder->CreateZExtOrTrunc(Src, Type::getInt8Ty(*Con));
      Src = JITState.IRBuilder->CreateAnd(Src, JITState.IRBuilder->getInt8(1));

      JITState.IRBuilder->CreateStore(Src, Value);
    break;
    }
    case IR::OP_ADD: {
      auto Op = IROp->C<IR::IROp_Add>();
      auto Src1 = GetSrc(Op->Header.Args[0]);
      auto Src2 = GetSrc(Op->Header.Args[1]);

      Src2 = JITState.IRBuilder->CreateZExtOrTrunc(Src2, Src1->getType());

      auto Result = JITState.IRBuilder->CreateAdd(Src1, Src2);
      SetDest(*WrapperOp, Result);
    break;
    }
    case IR::OP_SUB: {
      auto Op = IROp->C<IR::IROp_Add>();
      auto Src1 = GetSrc(Op->Header.Args[0]);
      auto Src2 = GetSrc(Op->Header.Args[1]);

      Src1 = JITState.IRBuilder->CreateZExtOrTrunc(Src1, Type::getIntNTy(*Con, OpSize * 8));
      Src2 = JITState.IRBuilder->CreateZExtOrTrunc(Src2, Type::getIntNTy(*Con, OpSize * 8));

      auto Result = JITState.IRBuilder->CreateSub(Src1, Src2);
      SetDest(*WrapperOp, Result);
    break;
    }
    case IR::OP_XOR: {
      auto Op = IROp->C<IR::IROp_Xor>();
      auto Src1 = GetSrc(Op->Header.Args[0]);
      auto Src2 = GetSrc(Op->Header.Args[1]);

      Src2 = JITState.IRBuilder->CreateZExtOrTrunc(Src2, Src1->getType());

      auto Result = JITState.IRBuilder->CreateXor(Src1, Src2);
      SetDest(*WrapperOp, Result);
    break;
    }
    case IR::OP_NOT: {
      auto Op = IROp->C<IR::IROp_Not>();
      auto Src = GetSrc(Op->Header.Args[0]);

      auto Result = JITState.IRBuilder->CreateNot(Src);
      SetDest(*WrapperOp, Result);
    break;
    }
    case IR::OP_NEG: {
      auto Op = IROp->C<IR::IROp_Neg>();
      auto Src = GetSrc(Op->Header.Args[0]);

      auto Result = JITState.IRBuilder->CreateNeg(Src);
      SetDest(*WrapperOp, Result);
    break;
    }
    case IR::OP_BFE: {
      auto Op = IROp->C<IR::IROp_Bfe>();
      auto Src = GetSrc(Op->Header.Args[0]);
      LogMan::Throw::A(OpSize <= 16, "OpSize is too large for BFE: %d", OpSize);

      auto BitWidth = Src->getType()->getIntegerBitWidth();
      if (OpSize == 16) {
        LogMan::Throw::A(Op->Width <= 64, "Can't extract width of %d", Op->Width);

        // Generate our 128bit mask
        auto SourceMask = JITState.IRBuilder->CreateShl(JITState.IRBuilder->getIntN(BitWidth, 1), JITState.IRBuilder->getIntN(BitWidth, Op->Width));
        SourceMask = JITState.IRBuilder->CreateSub(SourceMask, JITState.IRBuilder->getIntN(BitWidth, 1));

        // Shift the source in to the correct location
        auto Result = JITState.IRBuilder->CreateLShr(Src, JITState.IRBuilder->getIntN(BitWidth, Op->lsb));
        // Mask what we want
        Result = JITState.IRBuilder->CreateAnd(Result, SourceMask);
        SetDest(*WrapperOp, Result);
      }
      else {
        uint64_t SourceMask = (1ULL << Op->Width) - 1;
        if (Op->Width == 64)
          SourceMask = ~0ULL;

        auto Result = JITState.IRBuilder->CreateLShr(Src, JITState.IRBuilder->getIntN(BitWidth, Op->lsb));
        Result = JITState.IRBuilder->CreateAnd(Result,
            JITState.IRBuilder->getIntN(BitWidth, SourceMask));
        SetDest(*WrapperOp, Result);
      }
    break;
    }
    case IR::OP_BFI: {
      auto Op = IROp->C<IR::IROp_Bfi>();
      auto Src1 = GetSrc(Op->Header.Args[0]);
      auto Src2 = GetSrc(Op->Header.Args[1]);

      uint64_t SourceMask = (1ULL << Op->Width) - 1;
      if (Op->Width == 64)
        SourceMask = ~0ULL;
      uint64_t DestMask = ~(SourceMask << Op->lsb);

      auto BitWidth = Src1->getType()->getIntegerBitWidth();
      Src2 = JITState.IRBuilder->CreateZExtOrTrunc(Src2, Src1->getType());
      auto MaskedDest = JITState.IRBuilder->CreateAnd(Src1, JITState.IRBuilder->getIntN(BitWidth, DestMask));
      auto MaskedSrc = JITState.IRBuilder->CreateAnd(Src2, JITState.IRBuilder->getIntN(BitWidth, SourceMask));
      MaskedSrc = JITState.IRBuilder->CreateShl(MaskedSrc, JITState.IRBuilder->getIntN(BitWidth, Op->lsb));

      auto Result = JITState.IRBuilder->CreateOr(MaskedDest, MaskedSrc);

      SetDest(*WrapperOp, Result);
    break;
    }
    case IR::OP_LSHR: {
      auto Op = IROp->C<IR::IROp_Lshr>();
      auto Src1 = GetSrc(Op->Header.Args[0]);
      auto Src2 = GetSrc(Op->Header.Args[1]);

      // Our IR assumes defined behaviour for shifting all the bits out of the value
      // So we need to ZEXT to the next size up and then trunc
      auto OriginalType = Src1->getType();
      auto BiggerType = Type::getIntNTy(*Con, OriginalType->getPrimitiveSizeInBits() * 2);
      Src1 = JITState.IRBuilder->CreateZExt(Src1, BiggerType);
      Src2 = JITState.IRBuilder->CreateZExtOrTrunc(Src2, BiggerType);

      auto Result = JITState.IRBuilder->CreateLShr(Src1, Src2);
      Result = JITState.IRBuilder->CreateTrunc(Result, OriginalType);
      SetDest(*WrapperOp, Result);
    break;
    }
    case IR::OP_ASHR: {
      auto Op = IROp->C<IR::IROp_Ashr>();
      auto Src1 = GetSrc(Op->Header.Args[0]);
      auto Src2 = GetSrc(Op->Header.Args[1]);

      // Our IR assumes defined behaviour for shifting all the bits out of the value
      // So we need to ZEXT to the next size up and then trunc
      auto OriginalType = Src1->getType();
      auto BiggerType = Type::getIntNTy(*Con, OriginalType->getPrimitiveSizeInBits() * 2);
      Src1 = JITState.IRBuilder->CreateSExt(Src1, BiggerType);
      Src2 = JITState.IRBuilder->CreateZExtOrTrunc(Src2, BiggerType);

      auto Result = JITState.IRBuilder->CreateAShr(Src1, Src2);
      Result = JITState.IRBuilder->CreateTrunc(Result, OriginalType);
      SetDest(*WrapperOp, Result);
    break;
    }
    case IR::OP_LSHL: {
      auto Op = IROp->C<IR::IROp_Lshl>();
      auto Src1 = GetSrc(Op->Header.Args[0]);
      auto Src2 = GetSrc(Op->Header.Args[1]);

      // Our IR assumes defined behaviour for shifting all the bits out of the value
      // So we need to ZEXT to the next size up and then trunc
      auto OriginalType = Src1->getType();
      auto BiggerType = Type::getIntNTy(*Con, OriginalType->getPrimitiveSizeInBits() * 2);
      Src1 = JITState.IRBuilder->CreateZExt(Src1, BiggerType);
      Src2 = JITState.IRBuilder->CreateZExtOrTrunc(Src2, BiggerType);

      auto Result = JITState.IRBuilder->CreateShl(Src1, Src2);
      Result = JITState.IRBuilder->CreateTrunc(Result, OriginalType);
      SetDest(*WrapperOp, Result);
    break;
    }
    case IR::OP_AND: {
      auto Op = IROp->C<IR::IROp_And>();
      auto Src1 = GetSrc(Op->Header.Args[0]);
      auto Src2 = GetSrc(Op->Header.Args[1]);

      Src2 = JITState.IRBuilder->CreateZExtOrTrunc(Src2, Src1->getType());

      auto Result = JITState.IRBuilder->CreateAnd(Src1, Src2);
      SetDest(*WrapperOp, Result);
    break;
    }
    case IR::OP_UMUL:
    case IR::OP_MUL: {
      auto Op = IROp->C<IR::IROp_Mul>();
      auto Src1 = GetSrc(Op->Header.Args[0]);
      auto Src2 = GetSrc(Op->Header.Args[1]);

      Src2 = JITState.IRBuilder->CreateZExtOrTrunc(Src2, Src1->getType());

      auto Result = JITState.IRBuilder->CreateMul(Src1, Src2);
      SetDest(*WrapperOp, Result);
    break;
    }
    case IR::OP_ROL: {
      auto Op = IROp->C<IR::IROp_Rol>();
      auto Src1 = GetSrc(Op->Header.Args[0]);
      auto Src2 = GetSrc(Op->Header.Args[1]);

      Src2 = JITState.IRBuilder->CreateZExtOrTrunc(Src2, Src1->getType());

      auto Result = FSHL(Src1, Src1, Src2);
      SetDest(*WrapperOp, Result);
    break;
    }
    case IR::OP_ROR: {
      auto Op = IROp->C<IR::IROp_Ror>();
      auto Src1 = GetSrc(Op->Header.Args[0]);
      auto Src2 = GetSrc(Op->Header.Args[1]);

      Src2 = JITState.IRBuilder->CreateZExtOrTrunc(Src2, Src1->getType());
      auto Result = FSHR(Src1, Src1, Src2);
      SetDest(*WrapperOp, Result);
    break;
    }
    case IR::OP_PRINT: {
      auto Op = IROp->C<IR::IROp_Print>();
      auto Src = GetSrc(Op->Header.Args[0]);
      if (Src->getType()->getIntegerBitWidth() < 64) {
        Src = JITState.IRBuilder->CreateZExtOrTrunc(Src, Type::getInt64Ty(*Con));
      }
      CreateDebugPrint(Src);
    break;
    }

    case IR::OP_CYCLECOUNTER: {
#ifdef DEBUG_CYCLES
      SetDest(*WrapperOp, JITState.IRBuilder->getInt64(0));
#else
      SetDest(*WrapperOp, CycleCounter());
#endif
    break;
    }

    case IR::OP_POPCOUNT: {
      auto Op = IROp->C<IR::IROp_Popcount>();
      auto Src = GetSrc(Op->Header.Args[0]);

      SetDest(*WrapperOp, Popcount(Src));
    break;
    }
    case IR::OP_FINDLSB: {
      auto Op = IROp->C<IR::IROp_FindLSB>();
      auto Src = GetSrc(Op->Header.Args[0]);

      unsigned SrcBitWidth = Src->getType()->getIntegerBitWidth();
      llvm::Value *Result = CTTZ(Src);

      // Need to compare source to zero, since we are expecting -1 on zero, llvm CTTZ returns undef on zero
      auto Comp = JITState.IRBuilder->CreateICmpEQ(Src, JITState.IRBuilder->getIntN(SrcBitWidth, 0));
      Result = JITState.IRBuilder->CreateSelect(Comp, JITState.IRBuilder->getIntN(SrcBitWidth, ~0ULL), Result);
      SetDest(*WrapperOp, Result);
      break;
    }
    case IR::OP_FINDMSB: {
      auto Op = IROp->C<IR::IROp_FindMSB>();
      auto Src = GetSrc(Op->Header.Args[0]);

      unsigned SrcBitWidth = Src->getType()->getIntegerBitWidth();
      llvm::Value *Result = CTLZ(Src);

      Result = JITState.IRBuilder->CreateSub(JITState.IRBuilder->getIntN(SrcBitWidth, SrcBitWidth - 1), Result);
      SetDest(*WrapperOp, Result);
      break;
    }
    case IR::OP_FINDTRAILINGZEROS: {
      auto Op = IROp->C<IR::IROp_FindTrailingZeros>();
      auto Src = GetSrc(Op->Header.Args[0]);

      unsigned SrcBitWidth = Src->getType()->getIntegerBitWidth();
      llvm::Value *Result = CTTZ(Src);

      // Need to compare source to zero, since we are expecting -1 on zero, llvm CTTZ returns undef on zero
      auto Comp = JITState.IRBuilder->CreateICmpEQ(Src, JITState.IRBuilder->getIntN(SrcBitWidth, 0));
      Result = JITState.IRBuilder->CreateSelect(Comp, JITState.IRBuilder->getIntN(SrcBitWidth, SrcBitWidth), Result);
      SetDest(*WrapperOp, Result);
      break;
    }
    case IR::OP_SEXT: {
      auto Op = IROp->C<IR::IROp_Sext>();
      auto Src = GetSrc(Op->Header.Args[0]);
      llvm::Type *SourceType = Type::getIntNTy(*Con, Op->SrcSize);
      llvm::Type *TargetType = Type::getIntNTy(*Con, 64);

      auto Result = JITState.IRBuilder->CreateSExtOrTrunc(Src, SourceType);
      Result = JITState.IRBuilder->CreateSExt(Result, TargetType);
      SetDest(*WrapperOp, Result);
      break;
    }
    case IR::OP_ZEXT: {
      auto Op = IROp->C<IR::IROp_Zext>();
      auto Src = GetSrc(Op->Header.Args[0]);
      llvm::Type *SourceType = Type::getIntNTy(*Con, Op->SrcSize);
      llvm::Type *TargetType = Type::getIntNTy(*Con, 64);

      auto Result = JITState.IRBuilder->CreateZExtOrTrunc(Src, SourceType);
      Result = JITState.IRBuilder->CreateZExt(Result, TargetType);
      SetDest(*WrapperOp, Result);
    break;
    }

    case IR::OP_OR: {
      auto Op = IROp->C<IR::IROp_Or>();
      auto Src1 = GetSrc(Op->Header.Args[0]);
      auto Src2 = GetSrc(Op->Header.Args[1]);

      Src2 = JITState.IRBuilder->CreateZExtOrTrunc(Src2, Src1->getType());

      auto Result = JITState.IRBuilder->CreateOr(Src1, Src2);
      SetDest(*WrapperOp, Result);
    break;
    }
    case IR::OP_UDIV: {
      auto Op = IROp->C<IR::IROp_UDiv>();
      auto Src  = GetSrc(Op->Header.Args[0]);
      auto Divisor = GetSrc(Op->Header.Args[1]);

      Divisor = JITState.IRBuilder->CreateZExtOrTrunc(Divisor, Src->getType());

      auto Result = JITState.IRBuilder->CreateUDiv(Src, Divisor);
      SetDest(*WrapperOp, Result);

    break;
    }
    case IR::OP_DIV: {
      auto Op = IROp->C<IR::IROp_UDiv>();
      auto Src  = GetSrc(Op->Header.Args[0]);
      auto Divisor = GetSrc(Op->Header.Args[1]);

      Divisor = JITState.IRBuilder->CreateZExtOrTrunc(Divisor, Src->getType());

      auto Result = JITState.IRBuilder->CreateSDiv(Src, Divisor);
      SetDest(*WrapperOp, Result);
    break;
    }
    case IR::OP_UREM: {
      auto Op = IROp->C<IR::IROp_URem>();
      auto Src  = GetSrc(Op->Header.Args[0]);
      auto Divisor = GetSrc(Op->Header.Args[1]);

      Divisor = JITState.IRBuilder->CreateZExtOrTrunc(Divisor, Src->getType());

      auto Result = JITState.IRBuilder->CreateURem(Src, Divisor);
      SetDest(*WrapperOp, Result);

    break;
    }
    case IR::OP_REM: {
      auto Op = IROp->C<IR::IROp_Rem>();
      auto Src  = GetSrc(Op->Header.Args[0]);
      auto Divisor = GetSrc(Op->Header.Args[1]);

      Divisor = JITState.IRBuilder->CreateZExtOrTrunc(Divisor, Src->getType());

      auto Result = JITState.IRBuilder->CreateSRem(Src, Divisor);
      SetDest(*WrapperOp, Result);
    break;
    }
    case IR::OP_LUDIV: {
      auto Op = IROp->C<IR::IROp_LUDiv>();
      // Each source is OpSize in size
      // So you can have up to a 128bit divide from x86-64
      auto SrcLow  = GetSrc(Op->Header.Args[0]);
      auto SrcHigh = GetSrc(Op->Header.Args[1]);
      auto Divisor = GetSrc(Op->Header.Args[2]);

      Type *iNormal = Type::getIntNTy(*Con, OpSize * 8);
      Type *iLarge = Type::getIntNTy(*Con, OpSize * 8 * 2);

      // Zero extend all values to large size
      SrcLow = JITState.IRBuilder->CreateZExt(SrcLow, iLarge);
      SrcHigh = JITState.IRBuilder->CreateZExt(SrcHigh, iLarge);
      Divisor = JITState.IRBuilder->CreateZExt(Divisor, iLarge);
      // Combine the split values
      SrcHigh = JITState.IRBuilder->CreateShl(SrcHigh, JITState.IRBuilder->getIntN(OpSize * 8 * 2, OpSize * 8));
      auto Dividend = JITState.IRBuilder->CreateOr(SrcHigh, SrcLow);

      // Now do the divide
      auto Result = JITState.IRBuilder->CreateUDiv(Dividend, Divisor);

      // Now truncate back down origina size and store
      Result = JITState.IRBuilder->CreateTrunc(Result, iNormal);
      SetDest(*WrapperOp, Result);
    break;
    }
    case IR::OP_LDIV: {
      auto Op = IROp->C<IR::IROp_LDiv>();
      // Each source is OpSize in size
      // So you can have up to a 128bit divide from x86-64
      auto SrcLow  = GetSrc(Op->Header.Args[0]);
      auto SrcHigh = GetSrc(Op->Header.Args[1]);
      auto Divisor = GetSrc(Op->Header.Args[2]);

      Type *iNormal = Type::getIntNTy(*Con, OpSize * 8);
      Type *iLarge = Type::getIntNTy(*Con, OpSize * 8 * 2);

      // Zero extend all values to large size
      SrcLow = JITState.IRBuilder->CreateZExt(SrcLow, iLarge);
      SrcHigh = JITState.IRBuilder->CreateZExt(SrcHigh, iLarge);
      Divisor = JITState.IRBuilder->CreateSExt(Divisor, iLarge);
      // Combine the split values
      SrcHigh = JITState.IRBuilder->CreateShl(SrcHigh, JITState.IRBuilder->getIntN(OpSize * 8 * 2, OpSize * 8));
      auto Dividend = JITState.IRBuilder->CreateOr(SrcHigh, SrcLow);

      // Now do the divide
      auto Result = JITState.IRBuilder->CreateSDiv(Dividend, Divisor);

      // Now truncate back down origina size and store
      Result = JITState.IRBuilder->CreateTrunc(Result, iNormal);
      SetDest(*WrapperOp, Result);
    break;
    }

    case IR::OP_LUREM: {
      auto Op = IROp->C<IR::IROp_LURem>();
      // Each source is OpOpSize in size
      // So you can have up to a 128bit divide from x86-64
      auto SrcLow  = GetSrc(Op->Header.Args[0]);
      auto SrcHigh = GetSrc(Op->Header.Args[1]);
      auto Divisor = GetSrc(Op->Header.Args[2]);

      Type *iNormal = Type::getIntNTy(*Con, OpSize * 8);
      Type *iLarge = Type::getIntNTy(*Con, OpSize * 8 * 2);

      // Zero extend all values to large size
      SrcLow = JITState.IRBuilder->CreateZExt(SrcLow, iLarge);
      SrcHigh = JITState.IRBuilder->CreateZExt(SrcHigh, iLarge);
      Divisor = JITState.IRBuilder->CreateZExt(Divisor, iLarge);
      // Combine the split values
      SrcHigh = JITState.IRBuilder->CreateShl(SrcHigh, JITState.IRBuilder->getIntN(OpSize * 8 * 2, OpSize * 8));
      auto Dividend = JITState.IRBuilder->CreateOr(SrcHigh, SrcLow);

      // Now do the remainder
      auto Result = JITState.IRBuilder->CreateURem(Dividend, Divisor);

      // Now truncate back down origina size and store
      Result = JITState.IRBuilder->CreateTrunc(Result, iNormal);
      SetDest(*WrapperOp, Result);
    break;
    }
    case IR::OP_LREM: {
      auto Op = IROp->C<IR::IROp_LRem>();
      // Each source is OpOpSize in size
      // So you can have up to a 128bit divide from x86-64
      auto SrcLow  = GetSrc(Op->Header.Args[0]);
      auto SrcHigh = GetSrc(Op->Header.Args[1]);
      auto Divisor = GetSrc(Op->Header.Args[2]);

      Type *iNormal = Type::getIntNTy(*Con, OpSize * 8);
      Type *iLarge = Type::getIntNTy(*Con, OpSize * 8 * 2);

      // Zero extend all values to large size
      SrcLow = JITState.IRBuilder->CreateZExt(SrcLow, iLarge);
      SrcHigh = JITState.IRBuilder->CreateZExt(SrcHigh, iLarge);
      Divisor = JITState.IRBuilder->CreateSExt(Divisor, iLarge);
      // Combine the split values
      SrcHigh = JITState.IRBuilder->CreateShl(SrcHigh, JITState.IRBuilder->getIntN(OpSize * 8 * 2, OpSize * 8));
      auto Dividend = JITState.IRBuilder->CreateOr(SrcHigh, SrcLow);

      // Now do the remainder
      auto Result = JITState.IRBuilder->CreateSRem(Dividend, Divisor);

      // Now truncate back down origina size and store
      Result = JITState.IRBuilder->CreateTrunc(Result, iNormal);
      SetDest(*WrapperOp, Result);
    break;
    }

    case IR::OP_UMULH: {
      auto Op = IROp->C<IR::IROp_UMulH>();
      auto Src1 = GetSrc(Op->Header.Args[0]);
      auto Src2 = GetSrc(Op->Header.Args[1]);

      Type *iNormal = Type::getIntNTy(*Con, OpSize * 8);
      Type *iLarge = Type::getIntNTy(*Con, OpSize * 8 * 2);

      // Zero extend all values to larger value
      Src1 = JITState.IRBuilder->CreateZExt(Src1, iLarge);
      Src2 = JITState.IRBuilder->CreateZExt(Src2, iLarge);

      // Do the large multiply
      auto Result = JITState.IRBuilder->CreateMul(Src1, Src2);
      Result = JITState.IRBuilder->CreateLShr(Result, JITState.IRBuilder->getIntN(OpSize * 8 * 2, OpSize * 8));
      // Now truncate back down to origianl size and store
      Result = JITState.IRBuilder->CreateTrunc(Result, iNormal);
      SetDest(*WrapperOp, Result);
    break;
    }
    case IR::OP_MULH: {
      auto Op = IROp->C<IR::IROp_MulH>();
      auto Src1 = GetSrc(Op->Header.Args[0]);
      auto Src2 = GetSrc(Op->Header.Args[1]);

      Type *iNormal = Type::getIntNTy(*Con, OpSize * 8);
      Type *iLarge = Type::getIntNTy(*Con, OpSize * 8 * 2);

      // Sign extend all values to larger value
      Src1 = JITState.IRBuilder->CreateSExt(Src1, iLarge);
      Src2 = JITState.IRBuilder->CreateSExt(Src2, iLarge);

      // Do the large multiply
      auto Result = JITState.IRBuilder->CreateMul(Src1, Src2);
      Result = JITState.IRBuilder->CreateLShr(Result, JITState.IRBuilder->getIntN(OpSize * 8 * 2, OpSize * 8));

      // Now truncate back down to origianl size and store
      Result = JITState.IRBuilder->CreateTrunc(Result, iNormal);
      SetDest(*WrapperOp, Result);
    break;
    }
    case IR::OP_REV: {
      auto Op = IROp->C<IR::IROp_Rev>();
      auto Src = GetSrc(Op->Header.Args[0]);
      SetDest(*WrapperOp, BSwap(Src));
    break;
    }
    case IR::OP_VCASTFROMGPR: {
      auto Op = IROp->C<IR::IROp_VCastFromGPR>();
      LogMan::Throw::A(OpSize <= 16, "Can't handle a vector of size: %d", OpSize);
      auto Src = GetSrc(Op->Header.Args[0]);

      auto Result = JITState.IRBuilder->CreateVectorSplat(Op->RegisterSize / Op->ElementSize, Src);
      SetDest(*WrapperOp, Result);
    break;
    }
    case IR::OP_VFCMPEQ: {
      auto Op = IROp->C<IR::IROp_VFCMPEQ>();
      auto Src1 = GetSrc(Op->Header.Args[0]);
      auto Src2 = GetSrc(Op->Header.Args[1]);

      // Cast to the type we want
      Src1 = CastVectorToType(Src1, false, Op->RegisterSize, Op->ElementSize);
      Src2 = CastVectorToType(Src2, false, Op->RegisterSize, Op->ElementSize);

      // Do an icmpeq, this will return a vector of <NumElements x i1>
      auto Result = JITState.IRBuilder->CreateFCmpOEQ(Src1, Src2);

      // Now we will do a sext <NumElements x i1> -> <NumElements x ElementSize>
      Result = JITState.IRBuilder->CreateSExt(Result, llvm::VectorType::get(Type::getIntNTy(*Con, Src1->getType()->getVectorElementType()->getPrimitiveSizeInBits()), Src1->getType()->getVectorNumElements()));

      SetDest(*WrapperOp, Result);
      break;
    }
    case IR::OP_VFCMPNEQ: {
      auto Op = IROp->C<IR::IROp_VFCMPNEQ>();
      auto Src1 = GetSrc(Op->Header.Args[0]);
      auto Src2 = GetSrc(Op->Header.Args[1]);

      // Cast to the type we want
      Src1 = CastVectorToType(Src1, false, Op->RegisterSize, Op->ElementSize);
      Src2 = CastVectorToType(Src2, false, Op->RegisterSize, Op->ElementSize);

      // Do an icmpeq, this will return a vector of <NumElements x i1>
      auto Result = JITState.IRBuilder->CreateFCmpUNE(Src1, Src2);

      // Now we will do a sext <NumElements x i1> -> <NumElements x ElementSize>
      Result = JITState.IRBuilder->CreateSExt(Result, llvm::VectorType::get(Type::getIntNTy(*Con, Src1->getType()->getVectorElementType()->getPrimitiveSizeInBits()), Src1->getType()->getVectorNumElements()));

      SetDest(*WrapperOp, Result);
      break;
    }
    case IR::OP_VFCMPLT: {
      auto Op = IROp->C<IR::IROp_VFCMPLT>();
      auto Src1 = GetSrc(Op->Header.Args[0]);
      auto Src2 = GetSrc(Op->Header.Args[1]);

      // Cast to the type we want
      Src1 = CastVectorToType(Src1, false, Op->RegisterSize, Op->ElementSize);
      Src2 = CastVectorToType(Src2, false, Op->RegisterSize, Op->ElementSize);

      // Do an icmpeq, this will return a vector of <NumElements x i1>
      auto Result = JITState.IRBuilder->CreateFCmpOLT(Src1, Src2);

      // Now we will do a sext <NumElements x i1> -> <NumElements x ElementSize>
      Result = JITState.IRBuilder->CreateSExt(Result, llvm::VectorType::get(Type::getIntNTy(*Con, Src1->getType()->getVectorElementType()->getPrimitiveSizeInBits()), Src1->getType()->getVectorNumElements()));

      SetDest(*WrapperOp, Result);
      break;
    }
    case IR::OP_VFCMPGT: {
      auto Op = IROp->C<IR::IROp_VFCMPGT>();
      auto Src1 = GetSrc(Op->Header.Args[0]);
      auto Src2 = GetSrc(Op->Header.Args[1]);

      // Cast to the type we want
      Src1 = CastVectorToType(Src1, false, Op->RegisterSize, Op->ElementSize);
      Src2 = CastVectorToType(Src2, false, Op->RegisterSize, Op->ElementSize);

      // Do an icmpeq, this will return a vector of <NumElements x i1>
      auto Result = JITState.IRBuilder->CreateFCmpOGT(Src1, Src2);

      // Now we will do a sext <NumElements x i1> -> <NumElements x ElementSize>
      Result = JITState.IRBuilder->CreateSExt(Result, llvm::VectorType::get(Type::getIntNTy(*Con, Src1->getType()->getVectorElementType()->getPrimitiveSizeInBits()), Src1->getType()->getVectorNumElements()));

      SetDest(*WrapperOp, Result);
      break;
    }
    case IR::OP_VFCMPLE: {
      auto Op = IROp->C<IR::IROp_VFCMPLE>();
      auto Src1 = GetSrc(Op->Header.Args[0]);
      auto Src2 = GetSrc(Op->Header.Args[1]);

      // Cast to the type we want
      Src1 = CastVectorToType(Src1, false, Op->RegisterSize, Op->ElementSize);
      Src2 = CastVectorToType(Src2, false, Op->RegisterSize, Op->ElementSize);

      // Do an icmpeq, this will return a vector of <NumElements x i1>
      auto Result = JITState.IRBuilder->CreateFCmpOLE(Src1, Src2);

      // Now we will do a sext <NumElements x i1> -> <NumElements x ElementSize>
      Result = JITState.IRBuilder->CreateSExt(Result, llvm::VectorType::get(Type::getIntNTy(*Con, Src1->getType()->getVectorElementType()->getPrimitiveSizeInBits()), Src1->getType()->getVectorNumElements()));

      SetDest(*WrapperOp, Result);
      break;
    }
    case IR::OP_FCMP: {
      auto Op = IROp->C<IR::IROp_FCmp>();
      auto Src1 = GetSrc(Op->Header.Args[0]);
      auto Src2 = GetSrc(Op->Header.Args[1]);

      // Cast to the type we want
      Src1 = CastScalarToType(Src1, false, OpSize * 8, Op->ElementSize);
      Src2 = CastScalarToType(Src2, false, OpSize * 8, Op->ElementSize);

      llvm::Value *Result = JITState.IRBuilder->getInt64(0);
      if (Op->Flags & (1 << IR::FCMP_FLAG_LT)) {
        auto FCmp = JITState.IRBuilder->CreateFCmpOLT(Src1, Src2);
        FCmp = JITState.IRBuilder->CreateZExt(FCmp, Result->getType());
        FCmp = JITState.IRBuilder->CreateShl(FCmp, JITState.IRBuilder->getInt64(IR::FCMP_FLAG_LT));
        Result = JITState.IRBuilder->CreateOr(Result, FCmp);
      }
      if (Op->Flags & (1 << IR::FCMP_FLAG_UNORDERED)) {
        auto FCmp = JITState.IRBuilder->CreateFCmpUNO(Src1, Src2);
        FCmp = JITState.IRBuilder->CreateZExt(FCmp, Result->getType());
        FCmp = JITState.IRBuilder->CreateShl(FCmp, JITState.IRBuilder->getInt64(IR::FCMP_FLAG_UNORDERED));
        Result = JITState.IRBuilder->CreateOr(Result, FCmp);
      }
      if (Op->Flags & (1 << IR::FCMP_FLAG_EQ)) {
        auto FCmp = JITState.IRBuilder->CreateFCmpUEQ(Src1, Src2);
        FCmp = JITState.IRBuilder->CreateZExt(FCmp, Result->getType());
        FCmp = JITState.IRBuilder->CreateShl(FCmp, JITState.IRBuilder->getInt64(IR::FCMP_FLAG_EQ));
        Result = JITState.IRBuilder->CreateOr(Result, FCmp);
      }

      SetDest(*WrapperOp, Result);
      break;
    }
    case IR::OP_GETHOSTFLAG: {
      auto Op = IROp->C<IR::IROp_GetHostFlag>();
      auto Src = GetSrc(Op->Header.Args[0]);
      auto Result = JITState.IRBuilder->CreateLShr(Src, JITState.IRBuilder->getInt64(Op->Flag));
      Result = JITState.IRBuilder->CreateAnd(Result, JITState.IRBuilder->getInt64(1));

      SetDest(*WrapperOp, Result);
      break;
    }
    case IR::OP_CREATEVECTOR2: {
      auto Op = IROp->C<IR::IROp_CreateVector2>();
      LogMan::Throw::A(OpSize <= 16, "Can't handle a vector of size: %d", OpSize);
      auto Src1 = GetSrc(Op->Header.Args[0]);
      auto Src2 = GetSrc(Op->Header.Args[1]);

      // Cast to the type we want
      Value *Undef = UndefValue::get(VectorType::get(Src1->getType(), 2));

      // Src1 = CastToOpaqueStructure(Src1, ElementType);
      // Src2 = CastToOpaqueStructure(Src2, ElementType);

      Undef = JITState.IRBuilder->CreateInsertElement(Undef, Src1, JITState.IRBuilder->getInt32(0));
      Undef = JITState.IRBuilder->CreateInsertElement(Undef, Src2, JITState.IRBuilder->getInt32(1));
      SetDest(*WrapperOp, Undef);
    break;
    }
    case IR::OP_SPLATVECTOR2: {
      auto Op = IROp->C<IR::IROp_SplatVector2>();
      LogMan::Throw::A(OpSize <= 16, "Can't handle a vector of size: %d", OpSize);
      auto Src = GetSrc(Op->Header.Args[0]);

      auto Result = JITState.IRBuilder->CreateVectorSplat(2, Src);
      SetDest(*WrapperOp, Result);
    break;
    }
    case IR::OP_SPLATVECTOR3: {
      auto Op = IROp->C<IR::IROp_SplatVector3>();
      LogMan::Throw::A(OpSize <= 16, "Can't handle a vector of size: %d", OpSize);
      auto Src = GetSrc(Op->Header.Args[0]);

      auto Result = JITState.IRBuilder->CreateVectorSplat(3, Src);
      SetDest(*WrapperOp, Result);
    break;
    }
    case IR::OP_SPLATVECTOR4: {
      auto Op = IROp->C<IR::IROp_SplatVector4>();
      LogMan::Throw::A(OpSize <= 16, "Can't handle a vector of size: %d", OpSize);
      auto Src = GetSrc(Op->Header.Args[0]);

      auto Result = JITState.IRBuilder->CreateVectorSplat(4, Src);
      SetDest(*WrapperOp, Result);
    break;
    }
    case IR::OP_VBITCAST: {
      auto Op = IROp->C<IR::IROp_VBitcast>();
      auto Src = GetSrc(Op->Header.Args[0]);

      auto Result = CastToOpaqueStructure(Src, Type::getIntNTy(*Con, Src->getType()->getPrimitiveSizeInBits()));

      SetDest(*WrapperOp, Result);
    break;
    }
    case IR::OP_VEXTRACTTOGPR: {
      auto Op = IROp->C<IR::IROp_VExtractToGPR>();
      auto Src = GetSrc(Op->Header.Args[0]);

      // Cast to the type we want
      Src = CastVectorToType(Src, true, Op->RegisterSize, Op->ElementSize);

      auto Result = JITState.IRBuilder->CreateExtractElement(Src, JITState.IRBuilder->getInt32(Op->Idx));
      Result = JITState.IRBuilder->CreateZExt(Result, Type::getInt64Ty(*Con));
      SetDest(*WrapperOp, Result);
    break;
    }
    case IR::OP_VEXTRACTELEMENT: {
      auto Op = IROp->C<IR::IROp_VExtractElement>();
      auto Src = GetSrc(Op->Header.Args[0]);

      // Cast to the type we want
      Src = CastVectorToType(Src, true, Op->RegisterSize, Op->ElementSize);

      auto Result = JITState.IRBuilder->CreateExtractElement(Src, JITState.IRBuilder->getInt32(Op->Index));
      SetDest(*WrapperOp, Result);
    break;
    }
    case IR::OP_VINSGPR: {
      auto Op = IROp->C<IR::IROp_VInsGPR>();
      auto Src1 = GetSrc(Op->Header.Args[0]);
      auto Src2 = GetSrc(Op->Header.Args[1]);

      // Cast to the type we want
      Src1 = CastVectorToType(Src1, true, Op->RegisterSize, Op->ElementSize);
      Src2 = CastToOpaqueStructure(Src2, Type::getIntNTy(*Con, Op->ElementSize * 8));

      auto Result = JITState.IRBuilder->CreateInsertElement(Src1, Src2, JITState.IRBuilder->getInt32(Op->Index));
      SetDest(*WrapperOp, Result);
    break;
    }
    case IR::OP_VOR: {
      auto Op = IROp->C<IR::IROp_VOr>();
      auto Src1 = GetSrc(Op->Header.Args[0]);
      auto Src2 = GetSrc(Op->Header.Args[1]);

      // Cast to the type we want
      Src1 = CastVectorToType(Src1, true, Op->RegisterSize, Op->ElementSize);
      Src2 = CastVectorToType(Src2, true, Op->RegisterSize, Op->ElementSize);

      auto Result = JITState.IRBuilder->CreateOr(Src1, Src2);
      SetDest(*WrapperOp, Result);
    break;
    }
    case IR::OP_VXOR: {
      auto Op = IROp->C<IR::IROp_VXor>();
      auto Src1 = GetSrc(Op->Header.Args[0]);
      auto Src2 = GetSrc(Op->Header.Args[1]);

      // Cast to the type we want
      Src1 = CastVectorToType(Src1, true, Op->RegisterSize, Op->ElementSize);
      Src2 = CastVectorToType(Src2, true, Op->RegisterSize, Op->ElementSize);

      auto Result = JITState.IRBuilder->CreateXor(Src1, Src2);
      SetDest(*WrapperOp, Result);
    break;
    }
    case IR::OP_VAND: {
      auto Op = IROp->C<IR::IROp_VAnd>();
      auto Src1 = GetSrc(Op->Header.Args[0]);
      auto Src2 = GetSrc(Op->Header.Args[1]);

      // Cast to the type we want
      Src1 = CastVectorToType(Src1, true, Op->RegisterSize, Op->ElementSize);
      Src2 = CastVectorToType(Src2, true, Op->RegisterSize, Op->ElementSize);

      auto Result = JITState.IRBuilder->CreateAnd(Src1, Src2);
      SetDest(*WrapperOp, Result);
    break;
    }
    case IR::OP_VADD: {
      auto Op = IROp->C<IR::IROp_VAdd>();
      auto Src1 = GetSrc(Op->Header.Args[0]);
      auto Src2 = GetSrc(Op->Header.Args[1]);

      // Cast to the type we want
      Src1 = CastVectorToType(Src1, true, Op->RegisterSize, Op->ElementSize);
      Src2 = CastVectorToType(Src2, true, Op->RegisterSize, Op->ElementSize);

      auto Result = JITState.IRBuilder->CreateAdd(Src1, Src2);

      SetDest(*WrapperOp, Result);
    break;
    }
    case IR::OP_VSUB: {
      auto Op = IROp->C<IR::IROp_VSub>();
      auto Src1 = GetSrc(Op->Header.Args[0]);
      auto Src2 = GetSrc(Op->Header.Args[1]);

      // Cast to the type we want
      Src1 = CastVectorToType(Src1, true, Op->RegisterSize, Op->ElementSize);
      Src2 = CastVectorToType(Src2, true, Op->RegisterSize, Op->ElementSize);

      auto Result = JITState.IRBuilder->CreateSub(Src1, Src2);

      SetDest(*WrapperOp, Result);
    break;
    }
    case IR::OP_VADDP: {
      auto Op = IROp->C<IR::IROp_VAddP>();
      auto Src1 = GetSrc(Op->Header.Args[0]);
      auto Src2 = GetSrc(Op->Header.Args[1]);

      // Cast to the type we want
      Src1 = CastVectorToType(Src1, true, Op->RegisterSize, Op->ElementSize);
      Src2 = CastVectorToType(Src2, true, Op->RegisterSize, Op->ElementSize);

      auto Result = VADDP(Src1, Src2, Op->RegisterSize, Op->ElementSize);

      SetDest(*WrapperOp, Result);
    break;
    }
    case IR::OP_VUMUL:
    case IR::OP_VSMUL: {
      auto Op = IROp->C<IR::IROp_VUMul>();
      auto Src1 = GetSrc(Op->Header.Args[0]);
      auto Src2 = GetSrc(Op->Header.Args[1]);

      // Cast to the type we want
      Src1 = CastVectorToType(Src1, true, Op->RegisterSize, Op->ElementSize);
      Src2 = CastVectorToType(Src2, true, Op->RegisterSize, Op->ElementSize);

      auto Result = JITState.IRBuilder->CreateMul(Src1, Src2);

      SetDest(*WrapperOp, Result);
      break;
    }
    case IR::OP_VSQADD: {
      auto Op = IROp->C<IR::IROp_VSQAdd>();
      auto Src1 = GetSrc(Op->Header.Args[0]);
      auto Src2 = GetSrc(Op->Header.Args[1]);

      // Cast to the type we want
      Src1 = CastVectorToType(Src1, true, Op->RegisterSize, Op->ElementSize);
      Src2 = CastVectorToType(Src2, true, Op->RegisterSize, Op->ElementSize);

      auto Result = SQADD(Src1, Src2);

      SetDest(*WrapperOp, Result);
      break;
    }
    case IR::OP_VSQSUB: {
      auto Op = IROp->C<IR::IROp_VSQSub>();
      auto Src1 = GetSrc(Op->Header.Args[0]);
      auto Src2 = GetSrc(Op->Header.Args[1]);

      // Cast to the type we want
      Src1 = CastVectorToType(Src1, true, Op->RegisterSize, Op->ElementSize);
      Src2 = CastVectorToType(Src2, true, Op->RegisterSize, Op->ElementSize);

      auto Result = SQSUB(Src1, Src2);

      SetDest(*WrapperOp, Result);
      break;
    }
    case IR::OP_VUQADD: {
      auto Op = IROp->C<IR::IROp_VUQAdd>();
      auto Src1 = GetSrc(Op->Header.Args[0]);
      auto Src2 = GetSrc(Op->Header.Args[1]);

      // Cast to the type we want
      Src1 = CastVectorToType(Src1, true, Op->RegisterSize, Op->ElementSize);
      Src2 = CastVectorToType(Src2, true, Op->RegisterSize, Op->ElementSize);

      auto Result = UQADD(Src1, Src2);

      SetDest(*WrapperOp, Result);
      break;
    }
    case IR::OP_VUQSUB: {
      auto Op = IROp->C<IR::IROp_VUQSub>();
      auto Src1 = GetSrc(Op->Header.Args[0]);
      auto Src2 = GetSrc(Op->Header.Args[1]);

      // Cast to the type we want
      Src1 = CastVectorToType(Src1, true, Op->RegisterSize, Op->ElementSize);
      Src2 = CastVectorToType(Src2, true, Op->RegisterSize, Op->ElementSize);

      auto Result = UQSUB(Src1, Src2);

      SetDest(*WrapperOp, Result);
      break;
    }
    case IR::OP_VUMULL: {
      auto Op = IROp->C<IR::IROp_VUMull>();
      auto Src1 = GetSrc(Op->Header.Args[0]);
      auto Src2 = GetSrc(Op->Header.Args[1]);

      // Cast to the type we want
      Src1 = CastVectorToType(Src1, true, Op->RegisterSize, Op->ElementSize);
      Src2 = CastVectorToType(Src2, true, Op->RegisterSize, Op->ElementSize);

      switch (Op->ElementSize) {
        case 1: {
          std::vector<uint32_t> Mask {0, 1, 2, 3, 4, 5, 6, 7, 8};
          Value *Undef = UndefValue::get(VectorType::get(Src1->getType()->getVectorElementType(), 16));
          Src1 = JITState.IRBuilder->CreateShuffleVector(Src1, Undef, Mask);
          Src2 = JITState.IRBuilder->CreateShuffleVector(Src2, Undef, Mask);
          break;
        }
        case 2: {
          std::vector<uint32_t> Mask {0, 1, 2, 3};
          Value *Undef = UndefValue::get(VectorType::get(Src1->getType()->getVectorElementType(), 8));
          Src1 = JITState.IRBuilder->CreateShuffleVector(Src1, Undef, Mask);
          Src2 = JITState.IRBuilder->CreateShuffleVector(Src2, Undef, Mask);
          break;
        }
        case 4: {
          std::vector<uint32_t> Mask {0, 2}; // XXX: Why is this 0 and 2? It should be 0 and 1 but llvm "optimizes" the code to 0 and 4 in that case. How does this work?
          Value *Undef = UndefValue::get(VectorType::get(Src1->getType()->getVectorElementType(), 4));
          Src1 = JITState.IRBuilder->CreateShuffleVector(Src1, Undef, Mask, "Shuffle1");
          Src2 = JITState.IRBuilder->CreateShuffleVector(Src2, Undef, Mask, "Shuffle2");
          break;
        }
        default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
      }

      auto ElementType = llvm::Type::getIntNTy(*Con, Op->ElementSize * 8 * 2);
      llvm::Type *VectorType = llvm::VectorType::get(ElementType, Op->RegisterSize / Op->ElementSize / 2);

      Src1 = JITState.IRBuilder->CreateZExt(Src1, VectorType);
      Src2 = JITState.IRBuilder->CreateZExt(Src2, VectorType);

      auto Result = JITState.IRBuilder->CreateMul(Src1, Src2);

      SetDest(*WrapperOp, Result);
    break;
    }
    case IR::OP_VSMULL: {
      auto Op = IROp->C<IR::IROp_VSMull>();
      auto Src1 = GetSrc(Op->Header.Args[0]);
      auto Src2 = GetSrc(Op->Header.Args[1]);

      // Cast to the type we want
      Src1 = CastVectorToType(Src1, true, Op->RegisterSize, Op->ElementSize);
      Src2 = CastVectorToType(Src2, true, Op->RegisterSize, Op->ElementSize);

      switch (Op->ElementSize) {
        case 1: {
          std::vector<uint32_t> Mask {0, 1, 2, 3, 4, 5, 6, 7, 8};
          Value *Undef = UndefValue::get(VectorType::get(Src1->getType()->getVectorElementType(), 16));
          Src1 = JITState.IRBuilder->CreateShuffleVector(Src1, Undef, Mask);
          Src2 = JITState.IRBuilder->CreateShuffleVector(Src2, Undef, Mask);
          break;
        }
        case 2: {
          std::vector<uint32_t> Mask {0, 1, 2, 3};
          Value *Undef = UndefValue::get(VectorType::get(Src1->getType()->getVectorElementType(), 8));
          Src1 = JITState.IRBuilder->CreateShuffleVector(Src1, Undef, Mask);
          Src2 = JITState.IRBuilder->CreateShuffleVector(Src2, Undef, Mask);
          break;
        }
        case 4: {
          std::vector<uint32_t> Mask {0, 2}; // XXX: Why is this 0 and 2? It should be 0 and 1 but llvm "optimizes" the code to 0 and 4 in that case. How does this work?
          Value *Undef = UndefValue::get(VectorType::get(Src1->getType()->getVectorElementType(), 4));
          Src1 = JITState.IRBuilder->CreateShuffleVector(Src1, Undef, Mask, "Shuffle1");
          Src2 = JITState.IRBuilder->CreateShuffleVector(Src2, Undef, Mask, "Shuffle2");
          break;
        }
        default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
      }

      auto ElementType = llvm::Type::getIntNTy(*Con, Op->ElementSize * 8 * 2);
      llvm::Type *VectorType = llvm::VectorType::get(ElementType, Op->RegisterSize / Op->ElementSize / 2);

      Src1 = JITState.IRBuilder->CreateSExt(Src1, VectorType);
      Src2 = JITState.IRBuilder->CreateSExt(Src2, VectorType);

      auto Result = JITState.IRBuilder->CreateMul(Src1, Src2);

      SetDest(*WrapperOp, Result);
      break;
    }
    case IR::OP_VUMULL2: {
      auto Op = IROp->C<IR::IROp_VUMull2>();
      auto Src1 = GetSrc(Op->Header.Args[0]);
      auto Src2 = GetSrc(Op->Header.Args[1]);

      // Cast to the type we want
      Src1 = CastVectorToType(Src1, true, Op->RegisterSize, Op->ElementSize);
      Src2 = CastVectorToType(Src2, true, Op->RegisterSize, Op->ElementSize);

      switch (Op->ElementSize) {
        case 1: {
          std::vector<uint32_t> Mask {9, 10, 11, 12, 13, 14, 15};
          Value *Undef = UndefValue::get(VectorType::get(Src1->getType()->getVectorElementType(), 16));
          Src1 = JITState.IRBuilder->CreateShuffleVector(Src1, Undef, Mask);
          Src2 = JITState.IRBuilder->CreateShuffleVector(Src2, Undef, Mask);
          break;
        }
        case 2: {
          std::vector<uint32_t> Mask {4, 5, 6, 7};
          Value *Undef = UndefValue::get(VectorType::get(Src1->getType()->getVectorElementType(), 8));
          Src1 = JITState.IRBuilder->CreateShuffleVector(Src1, Undef, Mask);
          Src2 = JITState.IRBuilder->CreateShuffleVector(Src2, Undef, Mask);
          break;
        }
        case 4: {
          std::vector<uint32_t> Mask {2, 3};
          Value *Undef = UndefValue::get(VectorType::get(Src1->getType()->getVectorElementType(), 4));
          Src1 = JITState.IRBuilder->CreateShuffleVector(Src1, Undef, Mask, "Shuffle1");
          Src2 = JITState.IRBuilder->CreateShuffleVector(Src2, Undef, Mask, "Shuffle2");
          break;
        }
        default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
      }

      auto ElementType = llvm::Type::getIntNTy(*Con, Op->ElementSize * 8 * 2);
      llvm::Type *VectorType = llvm::VectorType::get(ElementType, Op->RegisterSize / Op->ElementSize / 2);

      Src1 = JITState.IRBuilder->CreateZExt(Src1, VectorType);
      Src2 = JITState.IRBuilder->CreateZExt(Src2, VectorType);

      auto Result = JITState.IRBuilder->CreateMul(Src1, Src2);

      SetDest(*WrapperOp, Result);
      break;
    }
    case IR::OP_VSMULL2: {
      auto Op = IROp->C<IR::IROp_VSMull2>();
      auto Src1 = GetSrc(Op->Header.Args[0]);
      auto Src2 = GetSrc(Op->Header.Args[1]);

      // Cast to the type we want
      Src1 = CastVectorToType(Src1, true, Op->RegisterSize, Op->ElementSize);
      Src2 = CastVectorToType(Src2, true, Op->RegisterSize, Op->ElementSize);

      switch (Op->ElementSize) {
        case 1: {
          std::vector<uint32_t> Mask {9, 10, 11, 12, 13, 14, 15};
          Value *Undef = UndefValue::get(VectorType::get(Src1->getType()->getVectorElementType(), 16));
          Src1 = JITState.IRBuilder->CreateShuffleVector(Src1, Undef, Mask);
          Src2 = JITState.IRBuilder->CreateShuffleVector(Src2, Undef, Mask);
          break;
        }
        case 2: {
          std::vector<uint32_t> Mask {4, 5, 6, 7};
          Value *Undef = UndefValue::get(VectorType::get(Src1->getType()->getVectorElementType(), 8));
          Src1 = JITState.IRBuilder->CreateShuffleVector(Src1, Undef, Mask);
          Src2 = JITState.IRBuilder->CreateShuffleVector(Src2, Undef, Mask);
          break;
        }
        case 4: {
          std::vector<uint32_t> Mask {2, 3};
          Value *Undef = UndefValue::get(VectorType::get(Src1->getType()->getVectorElementType(), 4));
          Src1 = JITState.IRBuilder->CreateShuffleVector(Src1, Undef, Mask, "Shuffle1");
          Src2 = JITState.IRBuilder->CreateShuffleVector(Src2, Undef, Mask, "Shuffle2");
          break;
        }
        default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
      }

      auto ElementType = llvm::Type::getIntNTy(*Con, Op->ElementSize * 8 * 2);
      llvm::Type *VectorType = llvm::VectorType::get(ElementType, Op->RegisterSize / Op->ElementSize / 2);

      Src1 = JITState.IRBuilder->CreateSExt(Src1, VectorType);
      Src2 = JITState.IRBuilder->CreateSExt(Src2, VectorType);

      auto Result = JITState.IRBuilder->CreateMul(Src1, Src2);

      SetDest(*WrapperOp, Result);
      break;
    }

    case IR::OP_VNOT: {
      auto Op = IROp->C<IR::IROp_VNot>();
      auto Src1 = GetSrc(Op->Header.Args[0]);

      // Cast to the type we want
      Src1 = CastVectorToType(Src1, true, Op->RegisterSize, Op->ElementSize);

      auto Result = JITState.IRBuilder->CreateNot(Src1);

      SetDest(*WrapperOp, Result);
    break;
    }
    case IR::OP_VFADD: {
      auto Op = IROp->C<IR::IROp_VFAdd>();
      auto Src1 = GetSrc(Op->Header.Args[0]);
      auto Src2 = GetSrc(Op->Header.Args[1]);

      // Cast to the type we want
      Src1 = CastVectorToType(Src1, false, Op->RegisterSize, Op->ElementSize);
      Src2 = CastVectorToType(Src2, false, Op->RegisterSize, Op->ElementSize);

      auto Result = JITState.IRBuilder->CreateFAdd(Src1, Src2);

      SetDest(*WrapperOp, Result);
    break;
    }
    case IR::OP_VFSUB: {
      auto Op = IROp->C<IR::IROp_VFSub>();
      auto Src1 = GetSrc(Op->Header.Args[0]);
      auto Src2 = GetSrc(Op->Header.Args[1]);

      // Cast to the type we want
      Src1 = CastVectorToType(Src1, false, Op->RegisterSize, Op->ElementSize);
      Src2 = CastVectorToType(Src2, false, Op->RegisterSize, Op->ElementSize);

      auto Result = JITState.IRBuilder->CreateFSub(Src1, Src2);

      SetDest(*WrapperOp, Result);
    break;
    }
    case IR::OP_VFMUL: {
      auto Op = IROp->C<IR::IROp_VFMul>();
      auto Src1 = GetSrc(Op->Header.Args[0]);
      auto Src2 = GetSrc(Op->Header.Args[1]);

      // Cast to the type we want
      Src1 = CastVectorToType(Src1, false, Op->RegisterSize, Op->ElementSize);
      Src2 = CastVectorToType(Src2, false, Op->RegisterSize, Op->ElementSize);

      auto Result = JITState.IRBuilder->CreateFMul(Src1, Src2);

      SetDest(*WrapperOp, Result);
    break;
    }
    case IR::OP_VFDIV: {
      auto Op = IROp->C<IR::IROp_VFDiv>();
      auto Src1 = GetSrc(Op->Header.Args[0]);
      auto Src2 = GetSrc(Op->Header.Args[1]);

      // Cast to the type we want
      Src1 = CastVectorToType(Src1, false, Op->RegisterSize, Op->ElementSize);
      Src2 = CastVectorToType(Src2, false, Op->RegisterSize, Op->ElementSize);

      auto Result = JITState.IRBuilder->CreateFDiv(Src1, Src2);

      SetDest(*WrapperOp, Result);
    break;
    }
    case IR::OP_VFMIN: {
      auto Op = IROp->C<IR::IROp_VFMin>();
      auto Src1 = GetSrc(Op->Header.Args[0]);
      auto Src2 = GetSrc(Op->Header.Args[1]);

      // Cast to the type we want
      Src1 = CastVectorToType(Src1, false, Op->RegisterSize, Op->ElementSize);
      Src2 = CastVectorToType(Src2, false, Op->RegisterSize, Op->ElementSize);

      auto Result = JITState.IRBuilder->CreateFCmpOLT(Src1, Src2);
      Result = JITState.IRBuilder->CreateSelect(Result, Src1, Src2);

      SetDest(*WrapperOp, Result);
    break;
    }
    case IR::OP_VFMAX: {
      auto Op = IROp->C<IR::IROp_VFMax>();
      auto Src1 = GetSrc(Op->Header.Args[0]);
      auto Src2 = GetSrc(Op->Header.Args[1]);

      // Cast to the type we want
      Src1 = CastVectorToType(Src1, false, Op->RegisterSize, Op->ElementSize);
      Src2 = CastVectorToType(Src2, false, Op->RegisterSize, Op->ElementSize);

      auto Result = JITState.IRBuilder->CreateFCmpOLT(Src1, Src2);
      Result = JITState.IRBuilder->CreateSelect(Result, Src2, Src1);

      SetDest(*WrapperOp, Result);
    break;
    }
    case IR::OP_VFRECP: {
      auto Op = IROp->C<IR::IROp_VFRecp>();
      auto Src = GetSrc(Op->Header.Args[0]);

      // Cast to the type we want
      Src = CastVectorToType(Src, false, Op->RegisterSize, Op->ElementSize);
      Value *Dividend = llvm::ConstantFP::get(Src->getType(), 1.0);

      auto Result = JITState.IRBuilder->CreateFDiv(Dividend, Src);

      SetDest(*WrapperOp, Result);
    break;
    }
    case IR::OP_VFSQRT: {
      auto Op = IROp->C<IR::IROp_VFSqrt>();
      auto Src = GetSrc(Op->Header.Args[0]);

      // Cast to the type we want
      Src = CastVectorToType(Src, false, Op->RegisterSize, Op->ElementSize);

      auto Result = SQRT(Src);

      SetDest(*WrapperOp, Result);
    break;
    }
    case IR::OP_VFRSQRT: {
      auto Op = IROp->C<IR::IROp_VFRSqrt>();
      auto Src = GetSrc(Op->Header.Args[0]);

      // Cast to the type we want
      Src = CastVectorToType(Src, false, Op->RegisterSize, Op->ElementSize);
      Value *Dividend = llvm::ConstantFP::get(Src->getType(), 1.0);

      auto Result = JITState.IRBuilder->CreateFDiv(Dividend, SQRT(Src));

      SetDest(*WrapperOp, Result);
    break;
    }
    case IR::OP_VSQXTN: {
      auto Op = IROp->C<IR::IROp_VSQXTN>();
      auto Src = GetSrc(Op->Header.Args[0]);

      // Cast to the type we want
      Src = CastVectorToType(Src, true, Op->RegisterSize, Op->ElementSize);

      llvm::Value *Result{};
      if (Op->RegisterSize == 8) {
        Result = VSQXTN_64(Src, Op->RegisterSize, Op->ElementSize);
      }
      else {
        Result = VSQXTN(Src, Op->RegisterSize, Op->ElementSize);
      }

      SetDest(*WrapperOp, Result);
      break;
    }
    case IR::OP_VSQXTN2: {
      auto Op = IROp->C<IR::IROp_VSQXTN2>();
      auto Src1 = GetSrc(Op->Header.Args[0]);
      auto Src2 = GetSrc(Op->Header.Args[1]);

      // Cast to the type we want
      Src1 = CastVectorToType(Src1, true, Op->RegisterSize, Op->ElementSize >> 1);
      Src2 = CastVectorToType(Src2, true, Op->RegisterSize, Op->ElementSize);

      llvm::Value *Result{};
      if (Op->RegisterSize == 8) {
        Result = VSQXTN2_64(Src1, Src2, Op->RegisterSize, Op->ElementSize);
      }
      else {
        Result = VSQXTN2(Src1, Src2, Op->RegisterSize, Op->ElementSize);
      }

      SetDest(*WrapperOp, Result);
      break;
    }
    case IR::OP_VSQXTUN: {
      auto Op = IROp->C<IR::IROp_VSQXTUN>();
      auto Src = GetSrc(Op->Header.Args[0]);

      // Cast to the type we want
      Src = CastVectorToType(Src, true, Op->RegisterSize, Op->ElementSize);

      llvm::Value *Result{};
      if (Op->RegisterSize == 8) {
        Result = VSQXTUN_64(Src, Op->RegisterSize, Op->ElementSize);
      }
      else {
        Result = VSQXTUN(Src, Op->RegisterSize, Op->ElementSize);
      }

      SetDest(*WrapperOp, Result);
      break;
    }
    case IR::OP_VSQXTUN2: {
      auto Op = IROp->C<IR::IROp_VSQXTUN2>();
      auto Src1 = GetSrc(Op->Header.Args[0]);
      auto Src2 = GetSrc(Op->Header.Args[1]);

      // Cast to the type we want
      Src1 = CastVectorToType(Src1, true, Op->RegisterSize, Op->ElementSize >> 1);
      Src2 = CastVectorToType(Src2, true, Op->RegisterSize, Op->ElementSize);

      llvm::Value *Result{};
      if (Op->RegisterSize == 8) {
        Result = VSQXTUN2_64(Src1, Src2, Op->RegisterSize, Op->ElementSize);
      }
      else {
        Result = VSQXTUN2(Src1, Src2, Op->RegisterSize, Op->ElementSize);
      }

      SetDest(*WrapperOp, Result);
      break;
    }
    case IR::OP_VUSHL: {
      auto Op = IROp->C<IR::IROp_VUShl>();
      auto Src1 = GetSrc(Op->Header.Args[0]);
      auto Src2 = GetSrc(Op->Header.Args[1]);

      // Cast to the type we want
      Src1 = CastVectorToType(Src1, true, Op->RegisterSize, Op->ElementSize);
      Src2 = CastVectorToType(Src2, true, Op->RegisterSize, Op->ElementSize);

      // Now we will do a lshr <NumElements x i1> -> <NumElements x ElementSize>
      auto Result = JITState.IRBuilder->CreateShl(Src1, Src2);

      SetDest(*WrapperOp, Result);
    break;
    }
    case IR::OP_VUSHLS: {
      auto Op = IROp->C<IR::IROp_VUShlS>();
      auto Src1 = GetSrc(Op->Header.Args[0]);
      auto Src2 = GetSrc(Op->Header.Args[1]);

      // Cast to the type we want
      Src1 = CastVectorToType(Src1, true, Op->RegisterSize, Op->ElementSize);

      Src2 = JITState.IRBuilder->CreateZExtOrTrunc(Src2, Src1->getType()->getScalarType());
      Src2 = JITState.IRBuilder->CreateVectorSplat(Op->RegisterSize / Op->ElementSize, Src2);

      // Now we will do a lshr <NumElements x i1> -> <NumElements x ElementSize>
      auto Result = JITState.IRBuilder->CreateShl(Src1, Src2);

      // Ensure we shift out to zero if the shift is > element size
      auto Cmp = JITState.IRBuilder->CreateICmpUGT(Src2, JITState.IRBuilder->CreateVectorSplat(Op->RegisterSize / Op->ElementSize, JITState.IRBuilder->getIntN(Op->ElementSize * 8, Op->ElementSize * 8 - 1)));
      Result = JITState.IRBuilder->CreateSelect(Cmp, JITState.IRBuilder->CreateVectorSplat(Op->RegisterSize / Op->ElementSize, JITState.IRBuilder->getIntN(Op->ElementSize * 8, 0)), Result);

      SetDest(*WrapperOp, Result);
    break;
    }
    case IR::OP_VUSHR: {
      auto Op = IROp->C<IR::IROp_VUShr>();
      auto Src1 = GetSrc(Op->Header.Args[0]);
      auto Src2 = GetSrc(Op->Header.Args[1]);

      // Cast to the type we want
      Src1 = CastVectorToType(Src1, true, Op->RegisterSize, Op->ElementSize);
      Src2 = CastVectorToType(Src2, true, Op->RegisterSize, Op->ElementSize);

      // Now we will do a lshr <NumElements x i1> -> <NumElements x ElementSize>
      auto Result = JITState.IRBuilder->CreateLShr(Src1, Src2);

      SetDest(*WrapperOp, Result);
      break;
    }
    case IR::OP_VUSHRS: {
      auto Op = IROp->C<IR::IROp_VUShrS>();
      auto Src1 = GetSrc(Op->Header.Args[0]);
      auto Src2 = GetSrc(Op->Header.Args[1]);

      // Cast to the type we want
      Src1 = CastVectorToType(Src1, true, Op->RegisterSize, Op->ElementSize);

      Src2 = JITState.IRBuilder->CreateZExtOrTrunc(Src2, Src1->getType()->getScalarType());
      Src2 = JITState.IRBuilder->CreateVectorSplat(Op->RegisterSize / Op->ElementSize, Src2);

      // Now we will do a lshr <NumElements x i1> -> <NumElements x ElementSize>
      auto Result = JITState.IRBuilder->CreateLShr(Src1, Src2);

      // Ensure we shift out to zero if the shift is > element size
      auto Cmp = JITState.IRBuilder->CreateICmpUGE(Src2, JITState.IRBuilder->CreateVectorSplat(Op->RegisterSize / Op->ElementSize, JITState.IRBuilder->getIntN(Op->ElementSize * 8, Op->ElementSize * 8)));
      Result = JITState.IRBuilder->CreateSelect(Cmp, JITState.IRBuilder->CreateVectorSplat(Op->RegisterSize / Op->ElementSize, JITState.IRBuilder->getIntN(Op->ElementSize * 8, 0)), Result);

      SetDest(*WrapperOp, Result);
      break;
    }
    case IR::OP_VSSHR: {
      auto Op = IROp->C<IR::IROp_VSShr>();
      auto Src1 = GetSrc(Op->Header.Args[0]);
      auto Src2 = GetSrc(Op->Header.Args[1]);

      // Cast to the type we want
      Src1 = CastVectorToType(Src1, true, Op->RegisterSize, Op->ElementSize);
      Src2 = CastVectorToType(Src2, true, Op->RegisterSize, Op->ElementSize);

      // Now we will do a lshr <NumElements x i1> -> <NumElements x ElementSize>
      auto Result = JITState.IRBuilder->CreateAShr(Src1, Src2);

      SetDest(*WrapperOp, Result);
      break;
    }
    case IR::OP_VSSHRS: {
      auto Op = IROp->C<IR::IROp_VSShrS>();
      auto Src1 = GetSrc(Op->Header.Args[0]);
      auto Src2 = GetSrc(Op->Header.Args[1]);

      // Cast to the type we want
      Src1 = CastVectorToType(Src1, true, Op->RegisterSize, Op->ElementSize);

      Src2 = JITState.IRBuilder->CreateZExtOrTrunc(Src2, Src1->getType()->getScalarType());
      Src2 = JITState.IRBuilder->CreateVectorSplat(Op->RegisterSize / Op->ElementSize, Src2);

      // Now we will do a lshr <NumElements x i1> -> <NumElements x ElementSize>
      auto Result = JITState.IRBuilder->CreateAShr(Src1, Src2);

      SetDest(*WrapperOp, Result);
      break;
    }
    case IR::OP_VSHLI: {
      auto Op = IROp->C<IR::IROp_VShlI>();
      auto Src1 = GetSrc(Op->Header.Args[0]);

      // Cast to the type we want
      Src1 = CastVectorToType(Src1, true, Op->RegisterSize, Op->ElementSize);

      auto Src2 = JITState.IRBuilder->CreateVectorSplat(Op->RegisterSize / Op->ElementSize, JITState.IRBuilder->getIntN(Op->ElementSize * 8, std::min((uint8_t)(Op->ElementSize * 8 - 1), Op->BitShift)));

      // Now we will do a lshr <NumElements x i1> -> <NumElements x ElementSize>
      auto Result = JITState.IRBuilder->CreateShl(Src1, Src2);

      SetDest(*WrapperOp, Result);
      break;
    }
    case IR::OP_VUSHRI: {
      auto Op = IROp->C<IR::IROp_VUShrI>();
      auto Src1 = GetSrc(Op->Header.Args[0]);

      // Cast to the type we want
      Src1 = CastVectorToType(Src1, true, Op->RegisterSize, Op->ElementSize);

      auto Src2 = JITState.IRBuilder->CreateVectorSplat(Op->RegisterSize / Op->ElementSize, JITState.IRBuilder->getIntN(Op->ElementSize * 8, std::min((uint8_t)(Op->ElementSize * 8 - 1), Op->BitShift)));

      // Now we will do a lshr <NumElements x i1> -> <NumElements x ElementSize>
      auto Result = JITState.IRBuilder->CreateLShr(Src1, Src2);

      SetDest(*WrapperOp, Result);
      break;
    }
    case IR::OP_VSSHRI: {
      auto Op = IROp->C<IR::IROp_VUShrI>();
      auto Src1 = GetSrc(Op->Header.Args[0]);

      // Cast to the type we want
      Src1 = CastVectorToType(Src1, true, Op->RegisterSize, Op->ElementSize);

      auto Src2 = JITState.IRBuilder->CreateVectorSplat(Op->RegisterSize / Op->ElementSize, JITState.IRBuilder->getIntN(Op->ElementSize * 8, std::min((uint8_t)(Op->ElementSize * 8 - 1), Op->BitShift)));

      // Now we will do a lshr <NumElements x i1> -> <NumElements x ElementSize>
      auto Result = JITState.IRBuilder->CreateAShr(Src1, Src2);

      SetDest(*WrapperOp, Result);
      break;
    }
    case IR::OP_VUSHRNI: {
      auto Op = IROp->C<IR::IROp_VUShrNI>();
      auto Src1 = GetSrc(Op->Header.Args[0]);

      uint32_t NumElements = Op->RegisterSize / Op->ElementSize;
      uint8_t ElementSize = Op->ElementSize * 8;
      // Cast to the type we want
      Src1 = CastVectorToType(Src1, true, Op->RegisterSize, Op->ElementSize);

      auto Src2 = JITState.IRBuilder->CreateVectorSplat(NumElements, JITState.IRBuilder->getIntN(ElementSize, std::min((uint8_t)(ElementSize - 1), Op->BitShift)));

      // Now we will do a lshr <NumElements x i1> -> <NumElements x ElementSize>
      llvm::Value *Result = JITState.IRBuilder->CreateLShr(Src1, Src2);
      Result = JITState.IRBuilder->CreateTrunc(Result, llvm::VectorType::get(llvm::Type::getIntNTy(*Con, ElementSize >> 1), NumElements));

      SetDest(*WrapperOp, Result);
      break;
    }
    case IR::OP_VUSHRNI2: {
      auto Op = IROp->C<IR::IROp_VUShrNI2>();
      auto Src1 = GetSrc(Op->Header.Args[0]);
      auto Src2 = GetSrc(Op->Header.Args[1]);

      uint32_t NumElements = Op->RegisterSize / Op->ElementSize;
      uint8_t ElementSize = Op->ElementSize * 8;

      // Low source needs to already be in the size we want (Typically from VUSHRNI)
      Src1 = CastVectorToType(Src1, true, Op->RegisterSize >> 1, Op->ElementSize >> 1);
      // Cast to the type we want
      Src2 = CastVectorToType(Src2, true, Op->RegisterSize, Op->ElementSize);

      auto Shift = JITState.IRBuilder->CreateVectorSplat(NumElements, JITState.IRBuilder->getIntN(ElementSize, std::min((uint8_t)(ElementSize - 1), Op->BitShift)));

      // Now we will do a lshr <NumElements x i1> -> <NumElements x ElementSize>
      llvm::Value *ResultHigh = JITState.IRBuilder->CreateLShr(Src2, Shift);
      ResultHigh = JITState.IRBuilder->CreateTrunc(ResultHigh, llvm::VectorType::get(llvm::Type::getIntNTy(*Con, ElementSize >> 1), NumElements));

      std::vector<uint32_t> Mask;
      for (uint32_t i = 0; i < (NumElements * 2); ++i) {
        Mask.emplace_back(i);
      }
      auto Result = JITState.IRBuilder->CreateShuffleVector(Src1, ResultHigh, Mask, "Shuffle1");

      SetDest(*WrapperOp, Result);
      break;
    }
    case IR::OP_VSLI: {
      auto Op = IROp->C<IR::IROp_VSLI>();
      auto Src = GetSrc(Op->Header.Args[0]);

      // Cast to the type we want
      Src = CastToOpaqueStructure(Src, Type::getIntNTy(*Con, Src->getType()->getPrimitiveSizeInBits()));

      Value *Result{};
      if (Op->ByteShift >= Op->ElementSize) {
        Result = JITState.IRBuilder->CreateVectorSplat(Op->RegisterSize / Op->ElementSize, JITState.IRBuilder->getIntN(Op->ElementSize * 8, 0));
      }
      else {
        Result = JITState.IRBuilder->CreateShl(Src, JITState.IRBuilder->getIntN(Src->getType()->getPrimitiveSizeInBits(), Op->ByteShift * 8));
      }

      SetDest(*WrapperOp, Result);
      break;
    }
    case IR::OP_VSRI: {
      auto Op = IROp->C<IR::IROp_VSRI>();
      auto Src = GetSrc(Op->Header.Args[0]);

      // Cast to the type we want
      Src = CastToOpaqueStructure(Src, Type::getIntNTy(*Con, Src->getType()->getPrimitiveSizeInBits()));

      Value *Result{};
      if (Op->ByteShift >= Op->ElementSize) {
        Result = JITState.IRBuilder->CreateVectorSplat(Op->RegisterSize / Op->ElementSize, JITState.IRBuilder->getIntN(Op->ElementSize * 8, 0));
      }
      else {
        Result = JITState.IRBuilder->CreateLShr(Src, JITState.IRBuilder->getIntN(Src->getType()->getPrimitiveSizeInBits(), Op->ByteShift * 8));
      }

      SetDest(*WrapperOp, Result);
      break;
    }
    case IR::OP_VSXTL: {
      auto Op = IROp->C<IR::IROp_VSXTL>();
      auto Src = GetSrc(Op->Header.Args[0]);

      // Cast to the type we want
      Src = CastVectorToType(Src, true, Op->RegisterSize, Op->ElementSize);

      uint8_t SrcNumElements = Op->RegisterSize / Op->ElementSize;
      uint8_t DstNumElements = SrcNumElements / 2;
      llvm::Type *TargetType = VectorType::get(Type::getIntNTy(*Con, Op->ElementSize * 16), DstNumElements);

      std::vector<uint32_t> Mask;
      for (uint32_t i = 0; i < DstNumElements; ++i) {
        Mask.emplace_back(i);
      }
      // Shuffle out which elements we actually want
      Src = JITState.IRBuilder->CreateShuffleVector(Src, Src, Mask);
      auto Result = JITState.IRBuilder->CreateSExt(Src, TargetType);
      SetDest(*WrapperOp, Result);
      break;
    }
    case IR::OP_VSXTL2: {
      auto Op = IROp->C<IR::IROp_VSXTL2>();
      auto Src = GetSrc(Op->Header.Args[0]);

      // Cast to the type we want
      Src = CastVectorToType(Src, true, Op->RegisterSize, Op->ElementSize);

      uint8_t SrcNumElements = Op->RegisterSize / Op->ElementSize;
      uint8_t DstNumElements = SrcNumElements / 2;
      llvm::Type *TargetType = VectorType::get(Type::getIntNTy(*Con, Op->ElementSize * 16), DstNumElements);

      std::vector<uint32_t> Mask;
      for (uint32_t i = DstNumElements; i < (SrcNumElements); ++i) {
        Mask.emplace_back(i);
      }
      // Shuffle out which elements we actually want
      Src = JITState.IRBuilder->CreateShuffleVector(Src, Src, Mask);
      auto Result = JITState.IRBuilder->CreateSExt(Src, TargetType);
      SetDest(*WrapperOp, Result);
      break;
    }
    case IR::OP_VUXTL: {
      auto Op = IROp->C<IR::IROp_VUXTL>();
      auto Src = GetSrc(Op->Header.Args[0]);

      // Cast to the type we want
      Src = CastVectorToType(Src, true, Op->RegisterSize, Op->ElementSize);

      uint8_t SrcNumElements = Op->RegisterSize / Op->ElementSize;
      uint8_t DstNumElements = SrcNumElements / 2;
      llvm::Type *TargetType = VectorType::get(Type::getIntNTy(*Con, Op->ElementSize * 16), DstNumElements);

      std::vector<uint32_t> Mask;
      for (uint32_t i = 0; i < DstNumElements; ++i) {
        Mask.emplace_back(i);
      }
      // Shuffle out which elements we actually want
      Src = JITState.IRBuilder->CreateShuffleVector(Src, Src, Mask);
      auto Result = JITState.IRBuilder->CreateZExt(Src, TargetType);
      SetDest(*WrapperOp, Result);
      break;
    }
    case IR::OP_VCMPEQ: {
      auto Op = IROp->C<IR::IROp_VCMPEQ>();
      auto Src1 = GetSrc(Op->Header.Args[0]);
      auto Src2 = GetSrc(Op->Header.Args[1]);

      // Cast to the type we want
      Src1 = CastVectorToType(Src1, true, Op->RegisterSize, Op->ElementSize);
      Src2 = CastVectorToType(Src2, true, Op->RegisterSize, Op->ElementSize);

      // Do an icmpeq, this will return a vector of <NumElements x i1>
      auto Result = JITState.IRBuilder->CreateICmpEQ(Src1, Src2);

      // Now we will do a sext <NumElements x i1> -> <NumElements x ElementSize>
      Result = JITState.IRBuilder->CreateSExt(Result, Src1->getType());

      SetDest(*WrapperOp, Result);
    break;
    }
    case IR::OP_VCMPGT: {
      auto Op = IROp->C<IR::IROp_VCMPGT>();
      auto Src1 = GetSrc(Op->Header.Args[0]);
      auto Src2 = GetSrc(Op->Header.Args[1]);

      // Cast to the type we want
      Src1 = CastVectorToType(Src1, true, Op->RegisterSize, Op->ElementSize);
      Src2 = CastVectorToType(Src2, true, Op->RegisterSize, Op->ElementSize);

      // Do an icmpeq, this will return a vector of <NumElements x i1>
      auto Result = JITState.IRBuilder->CreateICmpSGT(Src1, Src2);

      // Now we will do a sext <NumElements x i1> -> <NumElements x ElementSize>
      Result = JITState.IRBuilder->CreateSExt(Result, Src1->getType());

      SetDest(*WrapperOp, Result);
    break;
    }
    case IR::OP_VUMIN: {
      auto Op = IROp->C<IR::IROp_VUMin>();
      auto Src1 = GetSrc(Op->Header.Args[0]);
      auto Src2 = GetSrc(Op->Header.Args[1]);

      // Cast to the type we want
      Src1 = CastVectorToType(Src1, true, Op->RegisterSize, Op->ElementSize);
      Src2 = CastVectorToType(Src2, true, Op->RegisterSize, Op->ElementSize);

      auto Result = JITState.IRBuilder->CreateICmpULT(Src1, Src2);
      Result = JITState.IRBuilder->CreateSelect(Result, Src1, Src2);

      SetDest(*WrapperOp, Result);
      break;
    }
    case IR::OP_VUMAX: {
      auto Op = IROp->C<IR::IROp_VUMax>();
      auto Src1 = GetSrc(Op->Header.Args[0]);
      auto Src2 = GetSrc(Op->Header.Args[1]);

      // Cast to the type we want
      Src1 = CastVectorToType(Src1, true, Op->RegisterSize, Op->ElementSize);
      Src2 = CastVectorToType(Src2, true, Op->RegisterSize, Op->ElementSize);

      auto Result = JITState.IRBuilder->CreateICmpULT(Src1, Src2);
      Result = JITState.IRBuilder->CreateSelect(Result, Src2, Src1);

      SetDest(*WrapperOp, Result);
      break;
    }
    case IR::OP_VSMIN: {
      auto Op = IROp->C<IR::IROp_VSMin>();
      auto Src1 = GetSrc(Op->Header.Args[0]);
      auto Src2 = GetSrc(Op->Header.Args[1]);

      // Cast to the type we want
      Src1 = CastVectorToType(Src1, true, Op->RegisterSize, Op->ElementSize);
      Src2 = CastVectorToType(Src2, true, Op->RegisterSize, Op->ElementSize);

      auto Result = JITState.IRBuilder->CreateICmpSLT(Src1, Src2);
      Result = JITState.IRBuilder->CreateSelect(Result, Src1, Src2);

      SetDest(*WrapperOp, Result);
      break;
    }
    case IR::OP_VSMAX: {
      auto Op = IROp->C<IR::IROp_VSMax>();
      auto Src1 = GetSrc(Op->Header.Args[0]);
      auto Src2 = GetSrc(Op->Header.Args[1]);

      // Cast to the type we want
      Src1 = CastVectorToType(Src1, true, Op->RegisterSize, Op->ElementSize);
      Src2 = CastVectorToType(Src2, true, Op->RegisterSize, Op->ElementSize);

      auto Result = JITState.IRBuilder->CreateICmpSLT(Src1, Src2);
      Result = JITState.IRBuilder->CreateSelect(Result, Src2, Src1);

      SetDest(*WrapperOp, Result);
      break;
    }
    case IR::OP_VEXTR: {
      auto Op = IROp->C<IR::IROp_VExtr>();
      auto Src1 = GetSrc(Op->Header.Args[0]);
      auto Src2 = GetSrc(Op->Header.Args[1]);

      // Cast to the type we want
      Src1 = CastVectorToType(Src1, true, Op->RegisterSize, 1);
      Src2 = CastVectorToType(Src2, true, Op->RegisterSize, 1);

      std::vector<uint32_t> VectorMask;
      for (unsigned i = 0; i < 16; ++i) {
        VectorMask.emplace_back(Op->Index + i);
      }

      auto VectorMaskConstant = ConstantDataVector::get(*Con, VectorMask);

      auto Result = JITState.IRBuilder->CreateShuffleVector(Src2, Src1, VectorMaskConstant);

      SetDest(*WrapperOp, Result);
      break;
    }
    case IR::OP_VZIP2:
    case IR::OP_VZIP: {
      auto Op = IROp->C<IR::IROp_VZip>();
      auto Src1 = GetSrc(Op->Header.Args[0]);
      auto Src2 = GetSrc(Op->Header.Args[1]);

      // Cast to the type we want
      Src1 = CastVectorToType(Src1, true, Op->RegisterSize, Op->ElementSize);
      Src2 = CastVectorToType(Src2, true, Op->RegisterSize, Op->ElementSize);

      unsigned NumElements = Op->RegisterSize / Op->ElementSize;
      unsigned BaseElement = IROp->Op == IR::OP_VZIP2 ? NumElements / 2 : 0;
      std::vector<uint32_t> VectorMask;
      for (unsigned i = 0; i < NumElements; ++i) {
        unsigned shfl = i % 2 ? (NumElements + (i >> 1)) : (i >> 1);
        shfl += BaseElement;
        VectorMask.emplace_back(shfl);
      }

      auto VectorMaskConstant = ConstantDataVector::get(*Con, VectorMask);

      auto Result = JITState.IRBuilder->CreateShuffleVector(Src1, Src2, VectorMaskConstant);

      SetDest(*WrapperOp, Result);
    break;
    }
    case IR::OP_VINSELEMENT: {
      auto Op = IROp->C<IR::IROp_VInsElement>();
      auto Src1 = GetSrc(Op->Header.Args[0]);
      auto Src2 = GetSrc(Op->Header.Args[1]);

      // Cast to the type we want
      Src1 = CastVectorToType(Src1, true, Op->RegisterSize, Op->ElementSize);
      Src2 = CastVectorToType(Src2, true, Op->RegisterSize, Op->ElementSize);

      // Extract our source index
      auto Source = JITState.IRBuilder->CreateExtractElement(Src2, JITState.IRBuilder->getInt32(Op->SrcIdx));
      auto Result = JITState.IRBuilder->CreateInsertElement(Src1, Source, JITState.IRBuilder->getInt32(Op->DestIdx));
      SetDest(*WrapperOp, Result);
    break;
    }
    case IR::OP_VINSSCALARELEMENT: {
      auto Op = IROp->C<IR::IROp_VInsScalarElement>();
      auto Src1 = GetSrc(Op->Header.Args[0]);
      auto Src2 = GetSrc(Op->Header.Args[1]);

      // Cast to the type we want
      Src1 = CastVectorToType(Src1, true, Op->RegisterSize, Op->ElementSize);
      Src2 = CastScalarToType(Src2, true, Op->RegisterSize, Op->ElementSize);

      // Extract our source index
      auto Result = JITState.IRBuilder->CreateInsertElement(Src1, Src2, JITState.IRBuilder->getInt32(Op->DestIdx));
      SetDest(*WrapperOp, Result);
      break;
    }
    case IR::OP_VECTOR_UTOF: {
      auto Op = IROp->C<IR::IROp_Vector_UToF>();
      auto Src = GetSrc(Op->Header.Args[0]);

      uint8_t Elements = Op->RegisterSize / Op->ElementSize;
      Src = CastVectorToType(Src, true, Op->RegisterSize, Op->ElementSize);

      switch (Op->ElementSize) {
        case 4: {
          auto Result = JITState.IRBuilder->CreateUIToFP(Src, llvm::VectorType::get(Type::getFloatTy(*Con), Elements));
          SetDest(*WrapperOp, Result);
          break;
        }
        case 8: {
          auto Result = JITState.IRBuilder->CreateUIToFP(Src, llvm::VectorType::get(Type::getDoubleTy(*Con), Elements));
          SetDest(*WrapperOp, Result);
          break;
        }
        default: LogMan::Msg::A("Unknown castGPR element size: %d", Op->ElementSize);
      }
      break;
    }
    case IR::OP_VECTOR_STOF: {
      auto Op = IROp->C<IR::IROp_Vector_SToF>();
      auto Src = GetSrc(Op->Header.Args[0]);

      uint8_t Elements = Op->RegisterSize / Op->ElementSize;
      Src = CastVectorToType(Src, true, Op->RegisterSize, Op->ElementSize);

      switch (Op->ElementSize) {
        case 4: {
          auto Result = JITState.IRBuilder->CreateSIToFP(Src, llvm::VectorType::get(Type::getFloatTy(*Con), Elements));
          SetDest(*WrapperOp, Result);
          break;
        }
        case 8: {
          auto Result = JITState.IRBuilder->CreateSIToFP(Src, llvm::VectorType::get(Type::getDoubleTy(*Con), Elements));
          SetDest(*WrapperOp, Result);
          break;
        }
        default: LogMan::Msg::A("Unknown castGPR element size: %d", Op->ElementSize);
      }
      break;
    }
    case IR::OP_VECTOR_FTOZU: {
      auto Op = IROp->C<IR::IROp_Vector_FToZU>();
      auto Src = GetSrc(Op->Header.Args[0]);

      uint8_t Elements = Op->RegisterSize / Op->ElementSize;
      Src = CastVectorToType(Src, false, Op->RegisterSize, Op->ElementSize);

      switch (Op->ElementSize) {
        case 4: {
          auto Result = JITState.IRBuilder->CreateFPToUI(Src, llvm::VectorType::get(Type::getInt32Ty(*Con), Elements));
          SetDest(*WrapperOp, Result);
          break;
        }
        case 8: {
          auto Result = JITState.IRBuilder->CreateFPToUI(Src, llvm::VectorType::get(Type::getInt64Ty(*Con), Elements));
          SetDest(*WrapperOp, Result);
          break;
        }
        default: LogMan::Msg::A("Unknown castGPR element size: %d", Op->ElementSize);
      }
      break;
    }
    case IR::OP_VECTOR_FTOZS: {
      auto Op = IROp->C<IR::IROp_Vector_FToZS>();
      auto Src = GetSrc(Op->Header.Args[0]);

      uint8_t Elements = Op->RegisterSize / Op->ElementSize;
      Src = CastVectorToType(Src, false, Op->RegisterSize, Op->ElementSize);

      switch (Op->ElementSize) {
        case 4: {
          auto Result = JITState.IRBuilder->CreateFPToSI(Src, llvm::VectorType::get(Type::getInt32Ty(*Con), Elements));
          SetDest(*WrapperOp, Result);
          break;
        }
        case 8: {
          auto Result = JITState.IRBuilder->CreateFPToSI(Src, llvm::VectorType::get(Type::getInt64Ty(*Con), Elements));
          SetDest(*WrapperOp, Result);
          break;
        }
        default: LogMan::Msg::A("Unknown castGPR element size: %d", Op->ElementSize);
      }
      break;
    }
    case IR::OP_VECTOR_FTOF: {
      auto Op = IROp->C<IR::IROp_Vector_FToF>();
      auto Src = GetSrc(Op->Header.Args[0]);
      uint16_t Conv = (Op->DstElementSize << 8) | Op->SrcElementSize;

      // Cast to the type we want
      Src = CastVectorToType(Src, false, Op->RegisterSize, Op->SrcElementSize);

      switch (Conv) {
        case 0x0804: { // Double <- float
          uint8_t Elements = Op->RegisterSize / Op->SrcElementSize;
          auto Result = JITState.IRBuilder->CreateFPExt(Src, llvm::VectorType::get(Type::getDoubleTy(*Con), Elements));
          SetDest(*WrapperOp, Result);
          break;
        }
        case 0x0408: { // Float <- Double
          uint8_t Elements = Op->RegisterSize / Op->SrcElementSize;
          auto Result = JITState.IRBuilder->CreateFPTrunc(Src, llvm::VectorType::get(Type::getFloatTy(*Con), Elements));
          // This will be a <2 x float>
          // We need to convert it to a <4 x float> and have the upper two elements be zero
          auto ZeroVector = JITState.IRBuilder->CreateVectorSplat(4, JITState.IRBuilder->CreateBitCast(JITState.IRBuilder->getInt32(0), Type::getFloatTy(*Con)));
          ZeroVector = JITState.IRBuilder->CreateInsertElement(ZeroVector, JITState.IRBuilder->CreateExtractElement(Result, JITState.IRBuilder->getInt32(0)), JITState.IRBuilder->getInt32(0));
          ZeroVector = JITState.IRBuilder->CreateInsertElement(ZeroVector, JITState.IRBuilder->CreateExtractElement(Result, JITState.IRBuilder->getInt32(1)), JITState.IRBuilder->getInt32(1));
          SetDest(*WrapperOp, ZeroVector);
          break;
        }
        default: LogMan::Msg::A("Unknown Conversion Type : 0%04x", Conv); break;
      }
      break;
    }
    case IR::OP_FLOAT_FTOF: {
      auto Op = IROp->C<IR::IROp_Float_FToF>();
      uint16_t Conv = (Op->DstElementSize << 8) | Op->SrcElementSize;
      auto Src = GetSrc(Op->Header.Args[0]);

      // Cast to the type we want
      Src = CastScalarToType(Src, false, OpSize * 8, Op->SrcElementSize);

      switch (Conv) {
        case 0x0804: { // Double <- float
          auto Result = JITState.IRBuilder->CreateFPExt(Src, Type::getDoubleTy(*Con));
          SetDest(*WrapperOp, Result);
          break;
        }
        case 0x0408: { // Float <- Double
          auto Result = JITState.IRBuilder->CreateFPTrunc(Src, Type::getFloatTy(*Con));
          SetDest(*WrapperOp, Result);
          break;
        }
        default: LogMan::Msg::A("Unknown Conversion Type : 0%04x", Conv); break;
      }

      break;
    }
    case IR::OP_FLOAT_TOGPR_ZU: {
      auto Op = IROp->C<IR::IROp_Float_ToGPR_ZU>();
      auto Src = GetSrc(Op->Header.Args[0]);

      Src = CastScalarToType(Src, false, Op->ElementSize, Op->ElementSize);

      switch (Op->ElementSize) {
        case 4: {
          auto Result = JITState.IRBuilder->CreateFPToUI(Src, Type::getInt32Ty(*Con));
          SetDest(*WrapperOp, Result);
          break;
        }
        case 8: {
          auto Result = JITState.IRBuilder->CreateFPToUI(Src, Type::getInt64Ty(*Con));
          SetDest(*WrapperOp, Result);
          break;
        }
        default: LogMan::Msg::A("Unknown castGPR element size: %d", Op->ElementSize);
      }
      break;
    }
    case IR::OP_FLOAT_TOGPR_ZS: {
      auto Op = IROp->C<IR::IROp_Float_ToGPR_ZS>();
      auto Src = GetSrc(Op->Header.Args[0]);

      Src = CastScalarToType(Src, false, Op->ElementSize, Op->ElementSize);

      switch (Op->ElementSize) {
        case 4: {
          auto Result = JITState.IRBuilder->CreateFPToSI(Src, Type::getInt32Ty(*Con));
          SetDest(*WrapperOp, Result);
          break;
        }
        case 8: {
          auto Result = JITState.IRBuilder->CreateFPToSI(Src, Type::getInt64Ty(*Con));
          SetDest(*WrapperOp, Result);
          break;
        }
        default: LogMan::Msg::A("Unknown castGPR element size: %d", Op->ElementSize);
      }
      break;
    }
    case IR::OP_FLOAT_FROMGPR_U: {
      auto Op = IROp->C<IR::IROp_Float_FromGPR_U>();
      auto Src = GetSrc(Op->Header.Args[0]);

      switch (Op->ElementSize) {
        case 4: {
          auto Result = JITState.IRBuilder->CreateUIToFP(Src, Type::getFloatTy(*Con));
          SetDest(*WrapperOp, Result);
          break;
        }
        case 8: {
          auto Result = JITState.IRBuilder->CreateUIToFP(Src, Type::getDoubleTy(*Con));
          SetDest(*WrapperOp, Result);
          break;
        }
        default: LogMan::Msg::A("Unknown castGPR element size: %d", Op->ElementSize);
      }
      break;
    }
    case IR::OP_FLOAT_FROMGPR_S: {
      auto Op = IROp->C<IR::IROp_Float_FromGPR_S>();
      auto Src = GetSrc(Op->Header.Args[0]);

      switch (Op->ElementSize) {
        case 4: {
          auto Result = JITState.IRBuilder->CreateSIToFP(Src, Type::getFloatTy(*Con));
          SetDest(*WrapperOp, Result);
          break;
        }
        case 8: {
          auto Result = JITState.IRBuilder->CreateSIToFP(Src, Type::getDoubleTy(*Con));
          SetDest(*WrapperOp, Result);
          break;
        }
        default: LogMan::Msg::A("Unknown castGPR element size: %d", Op->ElementSize);
      }
      break;
    }
    case IR::OP_CAS: {
      auto Op = IROp->C<IR::IROp_CAS>();
      auto Src1 = GetSrc(Op->Header.Args[0]);
      auto Src2 = GetSrc(Op->Header.Args[1]);
      auto MemSrc = GetSrc(Op->Header.Args[2]);

      if (!ThreadState->CTX->Config.UnifiedMemory) {
        MemSrc = JITState.IRBuilder->CreateAdd(MemSrc, JITState.IRBuilder->getInt64(CTX->MemoryMapper.GetBaseOffset<uint64_t>(0)));
      }
      // Cast the pointer type correctly
      MemSrc = JITState.IRBuilder->CreateIntToPtr(MemSrc, Type::getIntNTy(*Con, OpSize * 8)->getPointerTo());

      Src1 = JITState.IRBuilder->CreateZExtOrTrunc(Src1, MemSrc->getType()->getPointerElementType());
      Src2 = JITState.IRBuilder->CreateZExtOrTrunc(Src2, MemSrc->getType()->getPointerElementType());

      llvm::Value *Result = JITState.IRBuilder->CreateAtomicCmpXchg(MemSrc, Src1, Src2, llvm::AtomicOrdering::SequentiallyConsistent, llvm::AtomicOrdering::SequentiallyConsistent);

      // Result is a { <Type>, i1 } So we need to extract it first
      // Behaves exactly like std::atomic::compare_exchange_strong(Desired (Src1), Src2) ? Src1 : Desired
      Result = JITState.IRBuilder->CreateExtractValue(Result, {0});
      SetDest(*WrapperOp, Result);
    break;
    }
    case IR::OP_LOADMEM: {
      auto Op = IROp->C<IR::IROp_LoadMem>();
      auto Src = GetSrc(Op->Header.Args[0]);

      if (!ThreadState->CTX->Config.UnifiedMemory) {
        Src = JITState.IRBuilder->CreateAdd(Src, JITState.IRBuilder->getInt64(CTX->MemoryMapper.GetBaseOffset<uint64_t>(0)));
      }
      // Cast the pointer type correctly
      Src = JITState.IRBuilder->CreateIntToPtr(Src, Type::getIntNTy(*Con, Op->Size * 8)->getPointerTo());
      auto Result = CreateMemoryLoad(Src, Op->Align);
      SetDest(*WrapperOp, Result);
    break;
    }
    case IR::OP_STOREMEM: {
      auto Op = IROp->C<IR::IROp_StoreMem>();

      auto Dst = GetSrc(Op->Header.Args[0]);
      auto Src = GetSrc(Op->Header.Args[1]);

      if (!ThreadState->CTX->Config.UnifiedMemory) {
        Dst = JITState.IRBuilder->CreateAdd(Dst, JITState.IRBuilder->getInt64(CTX->MemoryMapper.GetBaseOffset<uint64_t>(0)));
      }

      auto Type = Type::getIntNTy(*Con, Op->Size * 8);
      Src = JITState.IRBuilder->CreateZExtOrTrunc(Src, Type);
      Dst = JITState.IRBuilder->CreateIntToPtr(Dst, Type->getPointerTo());
      CreateMemoryStore(Dst, Src, Op->Align);
    break;
    }
    case IR::OP_ATOMICFETCHADD:
    case IR::OP_ATOMICFETCHSUB:
    case IR::OP_ATOMICFETCHAND:
    case IR::OP_ATOMICFETCHOR:
    case IR::OP_ATOMICFETCHXOR: {
      auto Op = IROp->C<IR::IROp_AtomicFetchAdd>();
      auto Src = GetSrc(Op->Header.Args[0]);
      auto Value = GetSrc(Op->Header.Args[1]);

      if (!ThreadState->CTX->Config.UnifiedMemory) {
        Src = JITState.IRBuilder->CreateAdd(Src, JITState.IRBuilder->getInt64(CTX->MemoryMapper.GetBaseOffset<uint64_t>(0)));
      }
      AtomicRMWInst::BinOp AtomicOp{};
      switch (IROp->Op) {
        case IR::OP_ATOMICFETCHADD: AtomicOp = AtomicRMWInst::Add; break;
        case IR::OP_ATOMICFETCHSUB: AtomicOp = AtomicRMWInst::Sub; break;
        case IR::OP_ATOMICFETCHAND: AtomicOp = AtomicRMWInst::And; break;
        case IR::OP_ATOMICFETCHOR:  AtomicOp = AtomicRMWInst::Or; break;
        case IR::OP_ATOMICFETCHXOR: AtomicOp = AtomicRMWInst::Xor; break;
        default: LogMan::Msg::A("Unknown Atomic Op: %d", IROp->Op);
      }
      // Cast the pointer type correctly
      Src = JITState.IRBuilder->CreateIntToPtr(Src, Type::getIntNTy(*Con, Op->Size * 8)->getPointerTo());
      auto Result = JITState.IRBuilder->CreateAtomicRMW(AtomicOp, Src, Value, AtomicOrdering::AcquireRelease);
      SetDest(*WrapperOp, Result);
    break;
    }
    case IR::OP_DUMMY:
    break;
    default:
      LogMan::Msg::A("Unknown IR Op: %d(%s)", IROp->Op, FEXCore::IR::GetName(IROp->Op).data());
    break;
  }
}

void* FEXCore::CPU::LLVMJITCore::CompileCode(FEXCore::IR::IRListView<true> const *IR, FEXCore::Core::DebugData *DebugData) {
  using namespace llvm;
  JumpTargets.clear();
  JITCurrentState.Blocks.clear();

  CurrentIR = IR;

#if DESTMAP_AS_MAP
  DestMap.clear();
#else
  uintptr_t ListSize = CurrentIR->GetListSize();
  if (ListSize > DestMap.size()) {
    DestMap.resize(std::max(DestMap.size() * 2, ListSize));
  }
#endif

  uintptr_t ListBegin = CurrentIR->GetListData();
  uintptr_t DataBegin = CurrentIR->GetData();

  auto HeaderIterator = CurrentIR->begin();
  IR::OrderedNodeWrapper *HeaderNodeWrapper = HeaderIterator();
  IR::OrderedNode *HeaderNode = HeaderNodeWrapper->GetNode(ListBegin);
  auto HeaderOp = HeaderNode->Op(DataBegin)->CW<FEXCore::IR::IROp_IRHeader>();
  LogMan::Throw::A(HeaderOp->Header.Op == IR::OP_IRHEADER, "First op wasn't IRHeader");

  std::ostringstream FunctionName;
  FunctionName << "Function_0x";
  FunctionName << std::hex << HeaderOp->Entry;

  auto FunctionModule = new llvm::Module("Module", *Con);
  auto EngineBuilder = llvm::EngineBuilder(std::unique_ptr<llvm::Module>(FunctionModule));
  EngineBuilder.setEngineKind(llvm::EngineKind::JIT);
  EngineBuilder.setMCJITMemoryManager(std::unique_ptr<llvm::RTDyldMemoryManager>(JITState.MemManager));

  auto Engine = EngineBuilder.create(LLVMTarget);

  Type *i64 = Type::getInt64Ty(*Con);
  auto FunctionType = FunctionType::get(Type::getVoidTy(*Con),
    {
      i64,
    }, false);
  Func = Function::Create(FunctionType,
    Function::ExternalLinkage,
    FunctionName.str(),
    FunctionModule);

  Func->setCallingConv(CallingConv::C);

  auto Builder = JITState.IRBuilder;

  auto Entry = BasicBlock::Create(*Con, "Entry", Func);
  JITCurrentState.Blocks.emplace_back(Entry);
  JITState.IRBuilder->SetInsertPoint(Entry);
  JITCurrentState.CurrentBlock = Entry;

  CreateGlobalVariables(Engine, FunctionModule);

  {
    IR::OrderedNode *BlockNode = HeaderOp->Blocks.GetNode(ListBegin);

    while (1) {
      using namespace FEXCore::IR;
      auto BlockIROp = BlockNode->Op(DataBegin)->CW<FEXCore::IR::IROp_CodeBlock>();
      LogMan::Throw::A(BlockIROp->Header.Op == IR::OP_CODEBLOCK, "IR type failed to be a code block");

      auto Block = BasicBlock::Create(*Con, "Block", Func);
      JITCurrentState.Blocks.emplace_back(Block);
      JumpTargets[BlockNode->Wrapped(ListBegin).ID()] = Block;

      if (BlockIROp->Next.ID() == 0) {
        break;
      } else {
        BlockNode = BlockIROp->Next.GetNode(ListBegin);
      }
    }
  }

  // Let's create the exit block quick
  JITCurrentState.ExitBlock = BasicBlock::Create(*Con, "ExitBlock", Func);
  JITCurrentState.Blocks.emplace_back(JITCurrentState.ExitBlock);

  JITState.IRBuilder->SetInsertPoint(JITCurrentState.ExitBlock);
  Builder->CreateRetVoid();

  IR::OrderedNode *BlockNode = HeaderOp->Blocks.GetNode(ListBegin);

  bool First = true;
  while (1) {
    using namespace FEXCore::IR;
    auto BlockIROp = BlockNode->Op(DataBegin)->CW<FEXCore::IR::IROp_CodeBlock>();
    LogMan::Throw::A(BlockIROp->Header.Op == IR::OP_CODEBLOCK, "IR type failed to be a code block");

    // We grab these nodes this way so we can iterate easily
    auto CodeBegin = CurrentIR->at(BlockIROp->Begin);
    auto CodeLast = CurrentIR->at(BlockIROp->Last);

    auto Block = JumpTargets[BlockNode->Wrapped(ListBegin).ID()];

    if (First) {
      JITState.IRBuilder->SetInsertPoint(Entry);
      JITState.IRBuilder->CreateBr(Block);
      First = false;
    }

    JITState.IRBuilder->SetInsertPoint(Block);
    JITCurrentState.CurrentBlock = Block;

    while (1) {
      HandleIR(CurrentIR, &CodeBegin);

      // CodeLast is inclusive. So we still need to dump the CodeLast op as well
      if (CodeBegin == CodeLast) {
        break;
      }
      ++CodeBegin;
    }

    if (BlockIROp->Next.ID() == 0) {
      break;
    } else {
      BlockNode = BlockIROp->Next.GetNode(ListBegin);
    }
  }

  llvm::ModulePassManager MPM;

  llvm::LoopAnalysisManager LAM;
  llvm::FunctionAnalysisManager FAM;
  llvm::CGSCCAnalysisManager CGAM;
  llvm::ModuleAnalysisManager MAM;

  llvm::PassBuilder passBuilder(LLVMTarget);

  passBuilder.registerLoopAnalyses(LAM);
  passBuilder.registerFunctionAnalyses(FAM);
  passBuilder.registerCGSCCAnalyses(CGAM);
  passBuilder.registerModuleAnalyses(MAM);

  passBuilder.crossRegisterProxies(LAM, FAM, CGAM, MAM);

  MPM = passBuilder.buildModuleOptimizationPipeline(
    llvm::PassBuilder::OptimizationLevel::O3);

  raw_ostream &Out = outs();

  if (CTX->Config.LLVM_PrinterPass)
  {
    MPM.addPass(PrintModulePass(Out));
  }

  if (CTX->Config.LLVM_IRValidation)
  {
    verifyModule(*FunctionModule, &Out);
  }

  MPM.run(*FunctionModule, MAM);
  Engine->finalizeObject();

  JITState.Functions.emplace_back(Engine);

  DebugData->HostCodeSize = JITState.MemManager->GetLastCodeAllocation();
  void *FunctionPtr = reinterpret_cast<void*>(Engine->getFunctionAddress(FunctionName.str()));

  return FunctionPtr;
}

FEXCore::CPU::CPUBackend *CreateLLVMCore(FEXCore::Core::InternalThreadState *Thread) {
  return new LLVMJITCore(Thread);
}

}
