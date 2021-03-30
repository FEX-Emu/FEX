/*
$info$
tags: backend|x86-64
desc: Main glue logic of the x86-64 splatter backend
$end_info$
*/

#include "Interface/Context/Context.h"

#include "Interface/Core/Dispatcher/X86Dispatcher.h"
#include "Interface/Core/JIT/x86_64/JITClass.h"
#include "Interface/Core/InternalThreadState.h"

#include "Interface/IR/Passes/RegisterAllocationPass.h"

#include <FEXCore/Core/X86Enums.h>
#include <FEXCore/Core/UContext.h>

#include <cmath>
#include <signal.h>

#include "Interface/Core/Interpreter/InterpreterOps.h"

// #define DEBUG_RA 1
// #define DEBUG_CYCLES

namespace FEXCore::CPU {

CodeBuffer AllocateNewCodeBuffer(size_t Size) {
  CodeBuffer Buffer;
  Buffer.Size = Size;
  Buffer.Ptr = static_cast<uint8_t*>(
               mmap(nullptr,
                    Buffer.Size,
                    PROT_READ | PROT_WRITE | PROT_EXEC,
                    MAP_PRIVATE | MAP_ANONYMOUS,
                    -1, 0));
  LogMan::Throw::A(Buffer.Ptr != reinterpret_cast<uint8_t*>(~0ULL), "Couldn't allocate code buffer");
  return Buffer;
}

void FreeCodeBuffer(CodeBuffer Buffer) {
  munmap(Buffer.Ptr, Buffer.Size);
}

}

namespace FEXCore::CPU {

void X86JITCore::CopyNecessaryDataForCompileThread(CPUBackend *Original) {
  X86JITCore *Core = reinterpret_cast<X86JITCore*>(Original);
  ThreadSharedData = Core->ThreadSharedData;
}

void X86JITCore::PushRegs() {
  for (auto &Xmm : RAXMM_x) {
    sub(rsp, 16);
    movaps(ptr[rsp], Xmm);
  }

  for (auto &Reg : RA64)
    push(Reg);

  auto NumPush = RA64.size();
  if (NumPush & 1)
    sub(rsp, 8); // Align
}

void X86JITCore::PopRegs() {
  auto NumPush = RA64.size();

  if (NumPush & 1)
    add(rsp, 8); // Align
  for (uint32_t i = RA64.size(); i > 0; --i)
    pop(RA64[i - 1]);

  for (uint32_t i = RAXMM_x.size(); i > 0; --i) {
    movaps(RAXMM_x[i - 1], ptr[rsp]);
    add(rsp, 16);
  }
}

void X86JITCore::Op_Unhandled(FEXCore::IR::IROp_Header *IROp, uint32_t Node) {
  FallbackInfo Info;
  if (!InterpreterOps::GetFallbackHandler(IROp, &Info)) {
    auto Name = FEXCore::IR::GetName(IROp->Op);
    LogMan::Msg::A("Unhandled IR Op: %s", std::string(Name).c_str());
  } else {
    switch(Info.ABI) {
      case FABI_VOID_U16: {
        PushRegs();
        mov(edi, GetSrc<RA_32>(IROp->Args[0].ID()));
        mov(rax, (uintptr_t)Info.fn);

        call(rax);

        PopRegs();
        break;
      }
      case FABI_F80_F32:{
        PushRegs();

        movss(xmm0, GetSrc(IROp->Args[0].ID()));
        mov(rax, (uintptr_t)Info.fn);

        call(rax);

        PopRegs();

        pxor(GetDst(Node), GetDst(Node));
        movq(GetDst(Node), rax);
        pinsrw(GetDst(Node), edx, 4);
      }
      break;

      case FABI_F80_F64:{
        PushRegs();

        movsd(xmm0, GetSrc(IROp->Args[0].ID()));
        mov(rax, (uintptr_t)Info.fn);

        call(rax);

        PopRegs();

        pxor(GetDst(Node), GetDst(Node));
        movq(GetDst(Node), rax);
        pinsrw(GetDst(Node), edx, 4);
      }
      break;

      case FABI_F80_I16:
      case FABI_F80_I32: {
        PushRegs();

        mov(edi, GetSrc<RA_32>(IROp->Args[0].ID()));
        mov(rax, (uintptr_t)Info.fn);

        call(rax);

        PopRegs();

        pxor(GetDst(Node), GetDst(Node));
        movq(GetDst(Node), rax);
        pinsrw(GetDst(Node), edx, 4);
      }
      break;

      case FABI_F32_F80:{
        PushRegs();

        movq(rdi, GetSrc(IROp->Args[0].ID()));
        pextrq(rsi, GetSrc(IROp->Args[0].ID()), 1);

        mov(rax, (uintptr_t)Info.fn);

        call(rax);

        PopRegs();

        movss(GetDst(Node), xmm0);
      }
      break;

      case FABI_F64_F80:{
        PushRegs();

        movq(rdi, GetSrc(IROp->Args[0].ID()));
        pextrq(rsi, GetSrc(IROp->Args[0].ID()), 1);

        mov(rax, (uintptr_t)Info.fn);

        call(rax);

        PopRegs();

        movsd(GetDst(Node), xmm0);
      }
      break;

      case FABI_I16_F80:{
        PushRegs();

        movq(rdi, GetSrc(IROp->Args[0].ID()));
        pextrq(rsi, GetSrc(IROp->Args[0].ID()), 1);

        mov(rax, (uintptr_t)Info.fn);

        call(rax);

        PopRegs();

        movzx(GetDst<RA_64>(Node), ax);
      }
      break;
      case FABI_I32_F80:{
        PushRegs();

        movq(rdi, GetSrc(IROp->Args[0].ID()));
        pextrq(rsi, GetSrc(IROp->Args[0].ID()), 1);

        mov(rax, (uintptr_t)Info.fn);

        call(rax);

        PopRegs();

        mov(GetDst<RA_32>(Node), eax);
      }
      break;
      case FABI_I64_F80:{
        PushRegs();

        movq(rdi, GetSrc(IROp->Args[0].ID()));
        pextrq(rsi, GetSrc(IROp->Args[0].ID()), 1);

        mov(rax, (uintptr_t)Info.fn);

        call(rax);

        PopRegs();

        mov(GetDst<RA_64>(Node), rax);
      }
      break;
      case FABI_I64_F80_F80:{
        PushRegs();

        movq(rdi, GetSrc(IROp->Args[0].ID()));
        pextrq(rsi, GetSrc(IROp->Args[0].ID()), 1);

        movq(rdx, GetSrc(IROp->Args[1].ID()));
        pextrq(rcx, GetSrc(IROp->Args[1].ID()), 1);

        mov(rax, (uintptr_t)Info.fn);

        call(rax);

        PopRegs();

        mov(GetDst<RA_64>(Node), rax);
      }
      break;
      case FABI_F80_F80:{
        PushRegs();

        movq(rdi, GetSrc(IROp->Args[0].ID()));
        pextrq(rsi, GetSrc(IROp->Args[0].ID()), 1);

        mov(rax, (uintptr_t)Info.fn);

        call(rax);

        PopRegs();

        pxor(GetDst(Node), GetDst(Node));
        movq(GetDst(Node), rax);
        pinsrw(GetDst(Node), edx, 4);
      }
      break;
      case FABI_F80_F80_F80:{
        PushRegs();

        movq(rdi, GetSrc(IROp->Args[0].ID()));
        pextrq(rsi, GetSrc(IROp->Args[0].ID()), 1);

        movq(rdx, GetSrc(IROp->Args[1].ID()));
        pextrq(rcx, GetSrc(IROp->Args[1].ID()), 1);

        mov(rax, (uintptr_t)Info.fn);

        call(rax);

        PopRegs();

        pxor(GetDst(Node), GetDst(Node));
        movq(GetDst(Node), rax);
        pinsrw(GetDst(Node), edx, 4);
      }
      break;

      case FABI_UNKNOWN:
      default:
      auto Name = FEXCore::IR::GetName(IROp->Op);
        LogMan::Msg::A("Unhandled IR Fallback abi: %s %d", std::string(Name).c_str(), Info.ABI);
    }
  }
}

void X86JITCore::Op_NoOp(FEXCore::IR::IROp_Header *IROp, uint32_t Node) {
}

X86JITCore::X86JITCore(FEXCore::Context::Context *ctx, FEXCore::Core::InternalThreadState *Thread, CodeBuffer Buffer, bool CompileThread)
  : CodeGenerator(Buffer.Size, Buffer.Ptr, nullptr)
  , CTX {ctx}
  , ThreadState {Thread}
  , InitialCodeBuffer {Buffer}
{
  CurrentCodeBuffer = &InitialCodeBuffer;

  RAPass = Thread->PassManager->GetRAPass();

  RAPass->AllocateRegisterSet(RegisterCount, RegisterClasses);
  RAPass->AddRegisters(FEXCore::IR::GPRClass, NumGPRs);
  RAPass->AddRegisters(FEXCore::IR::FPRClass, NumXMMs);
  RAPass->AddRegisters(FEXCore::IR::GPRPairClass, NumGPRPairs);

  for (uint32_t i = 0; i < NumGPRPairs; ++i) {
    RAPass->AddRegisterConflict(FEXCore::IR::GPRClass, i * 2,     FEXCore::IR::GPRPairClass, i);
    RAPass->AddRegisterConflict(FEXCore::IR::GPRClass, i * 2 + 1, FEXCore::IR::GPRPairClass, i);
  }

  for (uint32_t i = 0; i < FEXCore::IR::IROps::OP_LAST + 1; ++i) {
    OpHandlers[i] = &X86JITCore::Op_Unhandled;
  }

  RegisterALUHandlers();
  RegisterAtomicHandlers();
  RegisterBranchHandlers();
  RegisterConversionHandlers();
  RegisterFlagHandlers();
  RegisterMemoryHandlers();
  RegisterMiscHandlers();
  RegisterMoveHandlers();
  RegisterVectorHandlers();
  RegisterEncryptionHandlers();

  if (!CompileThread) {
    DispatcherConfig config;
    config.ExitFunctionLink = reinterpret_cast<uintptr_t>(&ExitFunctionLink);
    config.ExitFunctionLinkThis = reinterpret_cast<uintptr_t>(this);

    Dispatcher = new X86Dispatcher(CTX, ThreadState, config);
    DispatchPtr = Dispatcher->DispatchPtr;
    CallbackPtr = Dispatcher->CallbackPtr;

    ThreadSharedData.SignalHandlerRefCounterPtr = &Dispatcher->SignalHandlerRefCounter;
    ThreadSharedData.SignalHandlerReturnAddress = Dispatcher->SignalHandlerReturnAddress;


    // This will register the host signal handler per thread, which is fine
    CTX->SignalDelegation->RegisterHostSignalHandler(SIGILL, [](FEXCore::Core::InternalThreadState *Thread, int Signal, void *info, void *ucontext) -> bool {
      X86JITCore *Core = reinterpret_cast<X86JITCore*>(Thread->CPUBackend.get());
      return Core->Dispatcher->HandleSIGILL(Signal, info, ucontext);
    });

    CTX->SignalDelegation->RegisterHostSignalHandler(SignalDelegator::SIGNAL_FOR_PAUSE, [](FEXCore::Core::InternalThreadState *Thread, int Signal, void *info, void *ucontext) -> bool {
      X86JITCore *Core = reinterpret_cast<X86JITCore*>(Thread->CPUBackend.get());
      return Core->Dispatcher->HandleSignalPause(Signal, info, ucontext);
    });

    auto GuestSignalHandler = [](FEXCore::Core::InternalThreadState *Thread, int Signal, void *info, void *ucontext, GuestSigAction *GuestAction, stack_t *GuestStack) -> bool {
      X86JITCore *Core = reinterpret_cast<X86JITCore*>(Thread->CPUBackend.get());
      return Core->Dispatcher->HandleGuestSignal(Signal, info, ucontext, GuestAction, GuestStack);
    };

    for (uint32_t Signal = 0; Signal < SignalDelegator::MAX_SIGNALS; ++Signal) {
      CTX->SignalDelegation->RegisterHostSignalHandlerForGuest(Signal, GuestSignalHandler);
    }
  }
}

X86JITCore::~X86JITCore() {
  for (auto CodeBuffer : CodeBuffers) {
    FreeCodeBuffer(CodeBuffer);
  }
  CodeBuffers.clear();


  FreeCodeBuffer(InitialCodeBuffer);
}

void X86JITCore::ClearCache() {
  if (*ThreadSharedData.SignalHandlerRefCounterPtr == 0) {
    if (!CodeBuffers.empty()) {
      // If we have more than one code buffer we are tracking then walk them and delete
      // This is a cleanup step
      for (auto CodeBuffer : CodeBuffers) {
        FreeCodeBuffer(CodeBuffer);
      }
      CodeBuffers.clear();

      // Set the current code buffer to the initial
      setNewBuffer(InitialCodeBuffer.Ptr, InitialCodeBuffer.Size);
      CurrentCodeBuffer = &InitialCodeBuffer;
    }

    if (CurrentCodeBuffer->Size == MAX_CODE_SIZE) {
      // Rewind to the start of the code cache start
      reset();
    }
    else {
      FreeCodeBuffer(InitialCodeBuffer);

      // Resize the code buffer and reallocate our code size
      CurrentCodeBuffer->Size *= 1.5;
      CurrentCodeBuffer->Size = std::min(CurrentCodeBuffer->Size, MAX_CODE_SIZE);

      InitialCodeBuffer = AllocateNewCodeBuffer(CurrentCodeBuffer->Size);
      setNewBuffer(InitialCodeBuffer.Ptr, InitialCodeBuffer.Size);
    }
  }
  else {
    // We have signal handlers that have generated code
    // This means that we can not safely clear the code at this point in time
    // Allocate some new code buffers that we can switch over to instead
    auto NewCodeBuffer = AllocateNewCodeBuffer(X86JITCore::INITIAL_CODE_SIZE);
    EmplaceNewCodeBuffer(NewCodeBuffer);
    setNewBuffer(NewCodeBuffer.Ptr, NewCodeBuffer.Size);
  }
}

IR::PhysicalRegister X86JITCore::GetPhys(uint32_t Node) {
  auto PhyReg = RAData->GetNodeRegister(Node);

  LogMan::Throw::A(PhyReg.Raw != 255, "Couldn't Allocate register for node: ssa%d. Class: %d", Node, PhyReg.Class);

  return PhyReg;
}

bool X86JITCore::IsFPR(uint32_t Node) {
  return RAData->GetNodeRegister(Node).Class == IR::FPRClass.Val;
}

bool X86JITCore::IsGPR(uint32_t Node) {
  return RAData->GetNodeRegister(Node).Class == IR::GPRClass.Val;
}

template<uint8_t RAType>
Xbyak::Reg X86JITCore::GetSrc(uint32_t Node) {
  // rax, rcx, rdx, rsi, r8, r9,
  // r10
  // Callee Saved
  // rbx, rbp, r12, r13, r14, r15
  auto PhyReg = GetPhys(Node);
  if (RAType == RA_64)
    return RA64[PhyReg.Reg].cvt64();
  else if (RAType == RA_XMM)
    return RAXMM[PhyReg.Reg];
  else if (RAType == RA_32)
    return RA64[PhyReg.Reg].cvt32();
  else if (RAType == RA_16)
    return RA64[PhyReg.Reg].cvt16();
  else if (RAType == RA_8)
    return RA64[PhyReg.Reg].cvt8();
}

template
Xbyak::Reg X86JITCore::GetSrc<X86JITCore::RA_64>(uint32_t Node);

template
Xbyak::Reg X86JITCore::GetSrc<X86JITCore::RA_32>(uint32_t Node);

template
Xbyak::Reg X86JITCore::GetSrc<X86JITCore::RA_16>(uint32_t Node);

template
Xbyak::Reg X86JITCore::GetSrc<X86JITCore::RA_8>(uint32_t Node);

Xbyak::Xmm X86JITCore::GetSrc(uint32_t Node) {
  auto PhyReg = GetPhys(Node);
  return RAXMM_x[PhyReg.Reg];
}

template<uint8_t RAType>
Xbyak::Reg X86JITCore::GetDst(uint32_t Node) {
  auto PhyReg = GetPhys(Node);
  if (RAType == RA_64)
    return RA64[PhyReg.Reg].cvt64();
  else if (RAType == RA_XMM)
    return RAXMM[PhyReg.Reg];
  else if (RAType == RA_32)
    return RA64[PhyReg.Reg].cvt32();
  else if (RAType == RA_16)
    return RA64[PhyReg.Reg].cvt16();
  else if (RAType == RA_8)
    return RA64[PhyReg.Reg].cvt8();
}

template
Xbyak::Reg X86JITCore::GetDst<X86JITCore::RA_64>(uint32_t Node);

template
Xbyak::Reg X86JITCore::GetDst<X86JITCore::RA_32>(uint32_t Node);

template
Xbyak::Reg X86JITCore::GetDst<X86JITCore::RA_16>(uint32_t Node);

template
Xbyak::Reg X86JITCore::GetDst<X86JITCore::RA_8>(uint32_t Node);

template<uint8_t RAType>
std::pair<Xbyak::Reg, Xbyak::Reg> X86JITCore::GetSrcPair(uint32_t Node) {
  auto PhyReg = GetPhys(Node);
  if (RAType == RA_64)
    return RA64Pair[PhyReg.Reg];
  else if (RAType == RA_32)
    return {RA64Pair[PhyReg.Reg].first.cvt32(), RA64Pair[PhyReg.Reg].second.cvt32()};
}

template
std::pair<Xbyak::Reg, Xbyak::Reg> X86JITCore::GetSrcPair<X86JITCore::RA_64>(uint32_t Node);

template
std::pair<Xbyak::Reg, Xbyak::Reg> X86JITCore::GetSrcPair<X86JITCore::RA_32>(uint32_t Node);

Xbyak::Xmm X86JITCore::GetDst(uint32_t Node) {
  auto PhyReg = GetPhys(Node);
  return RAXMM_x[PhyReg.Reg];
}

bool X86JITCore::IsInlineConstant(const IR::OrderedNodeWrapper& WNode, uint64_t* Value) {
  auto OpHeader = IR->GetOp<IR::IROp_Header>(WNode);

  if (OpHeader->Op == IR::IROps::OP_INLINECONSTANT) {
    auto Op = OpHeader->C<IR::IROp_InlineConstant>();
    if (Value) {
      *Value = Op->Constant;
    }
    return true;
  } else {
    return false;
  }
}

bool X86JITCore::IsInlineEntrypointOffset(const IR::OrderedNodeWrapper& WNode, uint64_t* Value) {
  auto OpHeader = IR->GetOp<IR::IROp_Header>(WNode);

  if (OpHeader->Op == IR::IROps::OP_INLINEENTRYPOINTOFFSET) {
    auto Op = OpHeader->C<IR::IROp_InlineEntrypointOffset>();
    if (Value) {
      *Value = IR->GetHeader()->Entry + Op->Offset;
    }
    return true;
  } else {
    return false;
  }
}

std::tuple<X86JITCore::SetCC, X86JITCore::CMovCC, X86JITCore::JCC> X86JITCore::GetCC(IR::CondClassType cond) {
    switch (cond.Val) {
    case FEXCore::IR::COND_EQ:  return { &CodeGenerator::sete , &CodeGenerator::cmove , &CodeGenerator::je  };
    case FEXCore::IR::COND_NEQ: return { &CodeGenerator::setne, &CodeGenerator::cmovne, &CodeGenerator::jne };
    case FEXCore::IR::COND_SGE: return { &CodeGenerator::setge, &CodeGenerator::cmovge, &CodeGenerator::jge };
    case FEXCore::IR::COND_SLT: return { &CodeGenerator::setl , &CodeGenerator::cmovl , &CodeGenerator::jl  };
    case FEXCore::IR::COND_SGT: return { &CodeGenerator::setg , &CodeGenerator::cmovg , &CodeGenerator::jg  };
    case FEXCore::IR::COND_SLE: return { &CodeGenerator::setle, &CodeGenerator::cmovle, &CodeGenerator::jle };
    case FEXCore::IR::COND_UGE: return { &CodeGenerator::setae, &CodeGenerator::cmovae, &CodeGenerator::jae };
    case FEXCore::IR::COND_ULT: return { &CodeGenerator::setb , &CodeGenerator::cmovb , &CodeGenerator::jb  };
    case FEXCore::IR::COND_UGT: return { &CodeGenerator::seta , &CodeGenerator::cmova , &CodeGenerator::ja  };
    case FEXCore::IR::COND_ULE: return { &CodeGenerator::setna, &CodeGenerator::cmovna, &CodeGenerator::jna };

	  case FEXCore::IR::COND_FLU:  return { &CodeGenerator::setb , &CodeGenerator::cmovb , &CodeGenerator::jb  };
	  case FEXCore::IR::COND_FGE:  return { &CodeGenerator::setae, &CodeGenerator::cmovae, &CodeGenerator::jae };
	  case FEXCore::IR::COND_FLEU: return { &CodeGenerator::setna, &CodeGenerator::cmovna, &CodeGenerator::jna };
	  case FEXCore::IR::COND_FGT:  return { &CodeGenerator::seta , &CodeGenerator::cmova , &CodeGenerator::ja  };
	  case FEXCore::IR::COND_FU:   return { &CodeGenerator::setp , &CodeGenerator::cmovp , &CodeGenerator::jp  };
	  case FEXCore::IR::COND_FNU:  return { &CodeGenerator::setnp, &CodeGenerator::cmovnp, &CodeGenerator::jnp };

    case FEXCore::IR::COND_MI:
    case FEXCore::IR::COND_PL:
    case FEXCore::IR::COND_VS:
    case FEXCore::IR::COND_VC:
    default:
      LogMan::Msg::A("Unsupported compare type");
      break;
  }

  // Hope for the best
  return { &CodeGenerator::sete , &CodeGenerator::cmove , &CodeGenerator::je  };
}

void *X86JITCore::CompileCode([[maybe_unused]] FEXCore::IR::IRListView const *IR, [[maybe_unused]] FEXCore::Core::DebugData *DebugData, FEXCore::IR::RegisterAllocationData *RAData) {
  JumpTargets.clear();
  uint32_t SSACount = IR->GetSSACount();

  this->RAData = RAData;

  // Fairly excessive buffer range to make sure we don't overflow
  uint32_t BufferRange = SSACount * 16;
  if ((getSize() + BufferRange) > CurrentCodeBuffer->Size) {
    ThreadState->CTX->ClearCodeCache(ThreadState, false);
  }

	void *Entry = getCurr<void*>();
  this->IR = IR;

  if (CTX->GetGdbServerStatus()) {
    Label RunBlock;

    // If we have a gdb server running then run in a less efficient mode that checks if we need to exit
    // This happens when single stepping
    static_assert(sizeof(CTX->Config.RunningMode) == 4, "This is expected to be size of 4");
    mov(rax, reinterpret_cast<uint64_t>(CTX));

    // If the value == 0 then branch to the top
    cmp(dword [rax + (offsetof(FEXCore::Context::Context, Config.RunningMode))], 0);
    je(RunBlock);
    // Else we need to pause now
    mov(rax, Dispatcher->ThreadPauseHandlerAddress);
    jmp(rax);
    ud2();

    L(RunBlock);
  }

  LogMan::Throw::A(RAData != nullptr, "Needs RA");

  SpillSlots = RAData->SpillSlots();

  if (SpillSlots) {
    sub(rsp, SpillSlots * 16);
  }

#ifdef BLOCKSTATS
  BlockSamplingData::BlockData *SamplingData = CTX->BlockData->GetBlockData(HeaderOp->Entry);
  if (GetSamplingData) {
    mov(rcx, reinterpret_cast<uintptr_t>(SamplingData));
    rdtsc();
    shl(rdx, 32);
    or_(rax, rdx);
    mov(qword [rcx + offsetof(BlockSamplingData::BlockData, Start)], rax);
  }

  auto ExitBlock = [&]() {
    if (GetSamplingData) {
      mov(rcx, reinterpret_cast<uintptr_t>(SamplingData));
      // Get time
      rdtsc();
      shl(rdx, 32);
      or_(rax, rdx);

      // Calculate time spent in block
      mov(rdx, qword [rcx + offsetof(BlockSamplingData::BlockData, Start)]);
      sub(rax, rdx);

      // Add time to total time
      add(qword [rcx + offsetof(BlockSamplingData::BlockData, TotalTime)], rax);

      // Increment call count
      inc(qword [rcx + offsetof(BlockSamplingData::BlockData, TotalCalls)]);

      // Calculate min
      mov(rdx, qword [rcx + offsetof(BlockSamplingData::BlockData, Min)]);
      cmp(rdx, rax);
      cmova(rdx, rax);
      mov(qword [rcx + offsetof(BlockSamplingData::BlockData, Min)], rdx);

      // Calculate max
      mov(rdx, qword [rcx + offsetof(BlockSamplingData::BlockData, Max)]);
      cmp(rdx, rax);
      cmovb(rdx, rax);
      mov(qword [rcx + offsetof(BlockSamplingData::BlockData, Max)], rdx);
    }
  };
#endif

  PendingTargetLabel = nullptr;

  for (auto [BlockNode, BlockHeader] : IR->GetBlocks()) {
    using namespace FEXCore::IR;
    {
      auto BlockIROp = BlockHeader->CW<IROp_CodeBlock>();
      LogMan::Throw::A(BlockIROp->Header.Op == IR::OP_CODEBLOCK, "IR type failed to be a code block");

      uint32_t Node = IR->GetID(BlockNode);
      auto IsTarget = JumpTargets.find(Node);
      if (IsTarget == JumpTargets.end()) {
        IsTarget = JumpTargets.try_emplace(Node).first;
      }

      // if there is a pending branch, and it is not fall-through
      if (PendingTargetLabel && PendingTargetLabel != &IsTarget->second)
      {
        jmp(*PendingTargetLabel, T_NEAR);
      }
      PendingTargetLabel = nullptr;

      L(IsTarget->second);
    }

    for (auto [CodeNode, IROp] : IR->GetCode(BlockNode)) {
      #ifdef DEBUG_RA
      if (IROp->Op != IR::OP_BEGINBLOCK &&
          IROp->Op != IR::OP_CONDJUMP &&
          IROp->Op != IR::OP_JUMP) {
        std::stringstream Inst;
        auto Name = FEXCore::IR::GetName(IROp->Op);

        if (IROp->HasDest) {
          uint64_t PhysReg = RAPass->GetNodeRegister(Node);
          if (PhysReg >= GPRPairBase)
            Inst << "\tPair" << GetPhys(Node) << " = " << Name << " ";
          else if (PhysReg >= XMMBase)
            Inst << "\tXMM" << GetPhys(Node) << " = " << Name << " ";
          else
            Inst << "\tReg" << GetPhys(Node) << " = " << Name << " ";
        }
        else {
          Inst << "\t" << Name << " ";
        }

        uint8_t NumArgs = IR::GetArgs(IROp->Op);
        for (uint8_t i = 0; i < NumArgs; ++i) {
          uint32_t ArgNode = IROp->Args[i].ID();
          uint64_t PhysReg = RAPass->GetNodeRegister(ArgNode);
          if (PhysReg >= GPRPairBase)
            Inst << "Pair" << GetPhys(ArgNode) << (i + 1 == NumArgs ? "" : ", ");
          else if (PhysReg >= XMMBase)
            Inst << "XMM" << GetPhys(ArgNode) << (i + 1 == NumArgs ? "" : ", ");
          else
            Inst << "Reg" << GetPhys(ArgNode) << (i + 1 == NumArgs ? "" : ", ");
        }

        LogMan::Msg::D("%s", Inst.str().c_str());
      }
      #endif
      uint32_t ID = IR->GetID(CodeNode);

      // Execute handler
      OpHandler Handler = OpHandlers[IROp->Op];
      (this->*Handler)(IROp, ID);
    }
  }

  // Make sure last branch is generated. It certainly can't be eliminated here.
  if (PendingTargetLabel)
  {
    jmp(*PendingTargetLabel, T_NEAR);
  }
  PendingTargetLabel = nullptr;

  void *Exit = getCurr<void*>();
  this->IR = nullptr;

  ready();

  if (DebugData) {
    DebugData->HostCodeSize = reinterpret_cast<uintptr_t>(Exit) - reinterpret_cast<uintptr_t>(Entry);
  }
  return Entry;
}

uint64_t X86JITCore::ExitFunctionLink(X86JITCore *core, FEXCore::Core::CpuStateFrame *Frame, uint64_t *record) {
  auto Thread = Frame->Thread;
  auto GuestRip = record[1];

  auto HostCode = Thread->LookupCache->FindBlock(GuestRip);

  if (!HostCode) {
    Thread->CurrentFrame->State.rip = GuestRip;
    return core->Dispatcher->AbsoluteLoopTopAddress;
  }

  auto LinkerAddress = core->Dispatcher->ExitFunctionLinkerAddress;
  Thread->LookupCache->AddBlockLink(GuestRip, (uintptr_t)record, [record, LinkerAddress]{
    // undo the link
    record[0] = LinkerAddress;
  });

  record[0] = HostCode;
  return HostCode;
}

FEXCore::CPU::CPUBackend *CreateX86JITCore(FEXCore::Context::Context *ctx, FEXCore::Core::InternalThreadState *Thread, bool CompileThread) {
  return new X86JITCore(ctx, Thread, AllocateNewCodeBuffer(CompileThread ? X86JITCore::MAX_CODE_SIZE : X86JITCore::INITIAL_CODE_SIZE), CompileThread);
}
}
