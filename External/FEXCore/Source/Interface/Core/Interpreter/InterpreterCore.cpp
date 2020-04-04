#include "LogManager.h"
#include "Common/MathUtils.h"
#include "Interface/Context/Context.h"
#include "Interface/Core/DebugData.h"
#include "Interface/Core/InternalThreadState.h"
#include "Interface/HLE/Syscalls.h"
#include "LogManager.h"

#include <FEXCore/Core/CPUBackend.h>
#include <FEXCore/IR/IR.h>
#include <FEXCore/IR/IntrusiveIRList.h>

#include <atomic>
#include <cmath>
#include <limits>
#include <vector>

namespace FEXCore::CPU {

#define DESTMAP_AS_MAP 0
#if DESTMAP_AS_MAP
using DestMapType = std::unordered_map<uint32_t, uint32_t>;
#else
using DestMapType = std::vector<uint32_t>;
#endif

class InterpreterCore final : public CPUBackend {
public:
  explicit InterpreterCore(FEXCore::Context::Context *ctx);
  ~InterpreterCore() override = default;
  std::string GetName() override { return "Interpreter"; }
  void *CompileCode(FEXCore::IR::IRListView<true> const *IR, FEXCore::Core::DebugData *DebugData) override;

  void *MapRegion(void* HostPtr, uint64_t, uint64_t) override { return HostPtr; }

  bool NeedsOpDispatch() override { return true; }

  void ExecuteCode(FEXCore::Core::InternalThreadState *Thread);
private:
  FEXCore::Context::Context *CTX;
  uint32_t AllocateTmpSpace(size_t Size);

  template<typename Res>
  Res GetDest(IR::OrderedNodeWrapper Op);

  template<typename Res>
  Res GetSrc(IR::OrderedNodeWrapper Src);

  std::vector<uint8_t> TmpSpace;
  DestMapType DestMap;
  size_t TmpOffset{};

  FEXCore::IR::IRListView<true> *CurrentIR;
};

static void InterpreterExecution(FEXCore::Core::InternalThreadState *Thread) {
  InterpreterCore *Core = reinterpret_cast<InterpreterCore*>(Thread->CPUBackend.get());
  Core->ExecuteCode(Thread);
}

InterpreterCore::InterpreterCore(FEXCore::Context::Context *ctx)
  : CTX {ctx} {
  // Grab our space for temporary data
  TmpSpace.resize(4096 * 32);
#if !DESTMAP_AS_MAP
  DestMap.resize(4096);
#endif
}

uint32_t InterpreterCore::AllocateTmpSpace(size_t Size) {
  // XXX: IR generation has a bug where the size can periodically end up being zero
  // LogMan::Throw::A(Size !=0, "Dest Op had zero destination size");
  Size = Size < 16 ? 16 : Size;

  // Force alignment by size
  size_t NewBase = AlignUp(TmpOffset, Size);
  size_t NewEnd = NewBase + Size;

  if (NewEnd >= TmpSpace.size()) {
    // If we are going to overrun the end of our temporary space then double the size of it
    TmpSpace.resize(TmpSpace.size() * 2);
  }

  // Make sure to set the new offset
  TmpOffset = NewEnd;

  return NewBase;
}

template<typename Res>
Res InterpreterCore::GetDest(IR::OrderedNodeWrapper Op) {
  auto DstPtr = &TmpSpace.at(DestMap[Op.ID()]);
  return reinterpret_cast<Res>(DstPtr);
}

template<typename Res>
Res InterpreterCore::GetSrc(IR::OrderedNodeWrapper Src) {
#if DESTMAP_AS_MAP
  LogMan::Throw::A(DestMap.find(Src.ID()) != DestMap.end(), "Op had source but it wasn't in the destination map");
#endif

  auto DstPtr = &TmpSpace.at(DestMap[Src.ID()]);
  LogMan::Throw::A(DstPtr != nullptr, "Destmap had slot but didn't get allocated memory");
  return reinterpret_cast<Res>(DstPtr);
}

void *InterpreterCore::CompileCode([[maybe_unused]] FEXCore::IR::IRListView<true> const *IR, [[maybe_unused]] FEXCore::Core::DebugData *DebugData) {
  return reinterpret_cast<void*>(InterpreterExecution);
}

void InterpreterCore::ExecuteCode(FEXCore::Core::InternalThreadState *Thread) {
  auto IR = Thread->IRLists.find(Thread->State.State.rip);
  auto DebugData = Thread->DebugData.find(Thread->State.State.rip);
  CurrentIR = IR->second.get();

  TmpOffset = 0; // Reset where we are in the temp data range

  uintptr_t ListBegin = CurrentIR->GetListData();
  uintptr_t DataBegin = CurrentIR->GetData();

#if DESTMAP_AS_MAP
  DestMap.clear();
#else
  uintptr_t ListSize = CurrentIR->GetSSACount();
  if (ListSize > DestMap.size()) {
    DestMap.resize(std::max(DestMap.size() * 2, ListSize));
  }
#endif

  static_assert(sizeof(FEXCore::IR::IROp_Header) == 4);
  static_assert(sizeof(FEXCore::IR::OrderedNode) == 16);

  auto HeaderIterator = CurrentIR->begin();
  IR::OrderedNodeWrapper *HeaderNodeWrapper = HeaderIterator();
  IR::OrderedNode *HeaderNode = HeaderNodeWrapper->GetNode(ListBegin);
  auto HeaderOp = HeaderNode->Op(DataBegin)->CW<FEXCore::IR::IROp_IRHeader>();
  LogMan::Throw::A(HeaderOp->Header.Op == IR::OP_IRHEADER, "First op wasn't IRHeader");

  IR::OrderedNode const *BlockNode = HeaderOp->Blocks.GetNode(ListBegin);

#define GD *GetDest<uint64_t*>(*WrapperOp)
#define GDP GetDest<void*>(*WrapperOp)

  while (1) {
    using namespace FEXCore::IR;
    auto BlockIROp = BlockNode->Op(DataBegin)->C<FEXCore::IR::IROp_CodeBlock>();
    LogMan::Throw::A(BlockIROp->Header.Op == IR::OP_CODEBLOCK, "IR type failed to be a code block");

    // We grab these nodes this way so we can iterate easily
    auto CodeBegin = CurrentIR->at(BlockIROp->Begin);
    auto CodeLast = CurrentIR->at(BlockIROp->Last);
    struct {
      bool Quit;
      bool Redo;
    } BlockResults{};

    auto HandleBlock = [&]() {
      while (1) {
        OrderedNodeWrapper *WrapperOp = CodeBegin();
        OrderedNode *RealNode = WrapperOp->GetNode(ListBegin);
        FEXCore::IR::IROp_Header *IROp = RealNode->Op(DataBegin);
        uint8_t OpSize = IROp->Size;
        uint32_t Node = WrapperOp->ID();

        if (IROp->HasDest) {
          uint64_t AllocSize = OpSize * std::max(static_cast<uint8_t>(1), IROp->Elements);
          DestMap[Node] = AllocateTmpSpace(AllocSize);
          // Clear any previous results
          memset(GDP, 0, 16);
        }

        switch (IROp->Op) {
          case IR::OP_DUMMY:
          case IR::OP_BEGINBLOCK:
            break;
          case IR::OP_ENDBLOCK: {
            auto Op = IROp->C<IR::IROp_EndBlock>();
            Thread->State.State.rip += Op->RIPIncrement;
            break;
          }
          case IR::OP_EXITFUNCTION:
            BlockResults.Quit = true;
            return;
            break;
          case IR::OP_CONDJUMP: {
            auto Op = IROp->C<IR::IROp_CondJump>();
            uint64_t Arg = *GetSrc<uint64_t*>(Op->Header.Args[0]);
            if (!!Arg) {
              BlockNode = Op->Header.Args[1].GetNode(ListBegin);
            }
            else  {
              BlockNode = Op->Header.Args[2].GetNode(ListBegin);
            }
            BlockResults.Redo = true;
            return;
            break;
          }
          case IR::OP_JUMP: {
            auto Op = IROp->C<IR::IROp_Jump>();
            BlockNode = Op->Header.Args[0].GetNode(ListBegin);
            BlockResults.Redo = true;
            return;
            break;
          }
          case IR::OP_BREAK: {
            auto Op = IROp->C<IR::IROp_Break>();
            switch (Op->Reason) {
              case 4: // HLT
                Thread->State.RunningEvents.ShouldStop = true;
                BlockResults.Quit = true;
                return;
              break;
            default: LogMan::Msg::A("Unknown Break Reason: %d", Op->Reason); break;
            }
            break;
          }
          case IR::OP_SYSCALL: {
            auto Op = IROp->C<IR::IROp_Syscall>();

            FEXCore::HLE::SyscallArguments Args;
            for (size_t j = 0; j < FEXCore::HLE::SyscallArguments::MAX_ARGS; ++j) {
              if (Op->Header.Args[j].IsInvalid()) break;
              Args.Argument[j] = *GetSrc<uint64_t*>(Op->Header.Args[j]);
            }

            uint64_t Res = FEXCore::HandleSyscall(CTX->SyscallHandler, Thread, &Args);
            GD = Res;
            break;
          }
          case IR::OP_CPUID: {
            auto Op = IROp->C<IR::IROp_CPUID>();
            uint64_t *DstPtr = GetDest<uint64_t*>(*WrapperOp);
            uint64_t Arg = *GetSrc<uint64_t*>(Op->Header.Args[0]);

            auto Results = CTX->CPUID.RunFunction(Arg);
            memcpy(DstPtr, &Results.Res, sizeof(uint32_t) * 4);
            break;
          }
          case IR::OP_PRINT: {
            auto Op = IROp->C<IR::IROp_Print>();

            if (OpSize <= 8) {
              uint64_t Src = *GetSrc<uint64_t*>(Op->Header.Args[0]);
              LogMan::Msg::I(">>>> Value in Arg: 0x%lx, %ld", Src, Src);
            }
            else if (OpSize == 16) {
              __uint128_t Src = *GetSrc<__uint128_t*>(Op->Header.Args[0]);
              uint64_t Src0 = Src;
              uint64_t Src1 = Src >> 64;
              LogMan::Msg::I(">>>> Value[0] in Arg: 0x%lx, %ld", Src0, Src0);
              LogMan::Msg::I("     Value[1] in Arg: 0x%lx, %ld", Src1, Src1);
            }
            else
              LogMan::Msg::A("Unknown value size: %d", OpSize);
            break;
          }
          case IR::OP_CYCLECOUNTER: {
            #ifdef DEBUG_CYCLES
              GD = 0;
            #else
              timespec time;
              clock_gettime(CLOCK_REALTIME, &time);
              GD = time.tv_nsec + time.tv_sec * 1000000000;
            #endif
            break;
          }
          case IR::OP_MOV: {
            auto Op = IROp->C<IR::IROp_Mov>();
            memcpy(GDP, GetSrc<void*>(Op->Header.Args[0]), OpSize);
            break;
          }
          case IR::OP_VBITCAST: {
            auto Op = IROp->C<IR::IROp_VBitcast>();
            memcpy(GDP, GetSrc<void*>(Op->Header.Args[0]), 16);
            break;
          }
          case IR::OP_VCASTFROMGPR: {
            auto Op = IROp->C<IR::IROp_VCastFromGPR>();
            memcpy(GDP, GetSrc<void*>(Op->Header.Args[0]), Op->ElementSize);
            break;
          }
          case IR::OP_VEXTRACTTOGPR: {
            auto Op = IROp->C<IR::IROp_VExtractToGPR>();
            LogMan::Throw::A(Op->RegisterSize <= 16, "OpSize is too large for VExtractToGPR: %d", OpSize);
            if (Op->RegisterSize == 16) {
              __uint128_t SourceMask = (1ULL << (Op->ElementSize * 8)) - 1;
              uint64_t Shift = Op->ElementSize * Op->Idx * 8;
              if (Op->ElementSize == 8)
                SourceMask = ~0ULL;

              __uint128_t Src = *GetSrc<__uint128_t*>(Op->Header.Args[0]);
              Src >>= Shift;
              Src &= SourceMask;
              memcpy(GDP, &Src, Op->ElementSize);
            }
            else {
              uint64_t SourceMask = (1ULL << (Op->ElementSize * 8)) - 1;
              uint64_t Shift = Op->ElementSize * Op->Idx * 8;
              if (Op->ElementSize == 8)
                SourceMask = ~0ULL;

              uint64_t Src = *GetSrc<uint64_t*>(Op->Header.Args[0]);
              Src >>= Shift;
              Src &= SourceMask;
              GD = Src;
            }
            break;
          }
          case IR::OP_VEXTRACTELEMENT: {
            auto Op = IROp->C<IR::IROp_VExtractElement>();
            LogMan::Throw::A(Op->RegisterSize <= 16, "OpSize is too large for VExtractToGPR: %d", OpSize);
            if (Op->RegisterSize == 16) {
              __uint128_t SourceMask = (1ULL << (Op->ElementSize * 8)) - 1;
              uint64_t Shift = Op->ElementSize * Op->Index * 8;
              if (Op->ElementSize == 8)
                SourceMask = ~0ULL;

              __uint128_t Src = *GetSrc<__uint128_t*>(Op->Header.Args[0]);
              Src >>= Shift;
              Src &= SourceMask;
              memcpy(GDP, &Src, Op->ElementSize);
            }
            else {
              uint64_t SourceMask = (1ULL << (Op->ElementSize * 8)) - 1;
              uint64_t Shift = Op->ElementSize * Op->Index * 8;
              if (Op->ElementSize == 8)
                SourceMask = ~0ULL;

              uint64_t Src = *GetSrc<uint64_t*>(Op->Header.Args[0]);
              Src >>= Shift;
              Src &= SourceMask;
              GD = Src;
            }
            break;
          }
          case IR::OP_CONSTANT: {
            auto Op = IROp->C<IR::IROp_Constant>();
            GD = Op->Constant;
            break;
          }
          case IR::OP_LOADCONTEXT: {
            auto Op = IROp->C<IR::IROp_LoadContext>();

            uintptr_t ContextPtr = reinterpret_cast<uintptr_t>(&Thread->State.State);
            ContextPtr += Op->Offset;
            #define LOAD_CTX(x, y) \
              case x: { \
                y const *Data = reinterpret_cast<y const*>(ContextPtr); \
                GD = *Data; \
                break; \
              }
            switch (Op->Size) {
              LOAD_CTX(1, uint8_t)
              LOAD_CTX(2, uint16_t)
              LOAD_CTX(4, uint32_t)
              LOAD_CTX(8, uint64_t)
              case 16: {
                void const *Data = reinterpret_cast<void const*>(ContextPtr);
                memcpy(GDP, Data, Op->Size);
                break;
              }
              default:  LogMan::Msg::A("Unhandled LoadContext size: %d", Op->Size);
            }
            #undef LOAD_CTX
            break;
          }
          case IR::OP_LOADCONTEXTINDEXED: {
            auto Op = IROp->C<IR::IROp_LoadContextIndexed>();
            uint64_t Index = *GetSrc<uint64_t*>(Op->Header.Args[0]);

            uintptr_t ContextPtr = reinterpret_cast<uintptr_t>(&Thread->State.State);
            ContextPtr += Op->BaseOffset;
            ContextPtr += Index * Op->Stride;

            #define LOAD_CTX(x, y) \
              case x: { \
                y const *Data = reinterpret_cast<y const*>(ContextPtr); \
                GD = *Data; \
                break; \
              }
            switch (Op->Size) {
              LOAD_CTX(1, uint8_t)
              LOAD_CTX(2, uint16_t)
              LOAD_CTX(4, uint32_t)
              LOAD_CTX(8, uint64_t)
              case 16: {
                void const *Data = reinterpret_cast<void const*>(ContextPtr);
                memcpy(GDP, Data, Op->Size);
                break;
              }
              default:  LogMan::Msg::A("Unhandled LoadContextIndexed size: %d", Op->Size);
            }
            #undef LOAD_CTX
            break;
          }
          case IR::OP_STORECONTEXT: {
            auto Op = IROp->C<IR::IROp_StoreContext>();

            uintptr_t ContextPtr = reinterpret_cast<uintptr_t>(&Thread->State.State);
            ContextPtr += Op->Offset;

            void *Data = reinterpret_cast<void*>(ContextPtr);
            void *Src = GetSrc<void*>(Op->Header.Args[0]);
            memcpy(Data, Src, Op->Size);
            break;
          }
          case IR::OP_STORECONTEXTINDEXED: {
            auto Op = IROp->C<IR::IROp_StoreContextIndexed>();
            uint64_t Index = *GetSrc<uint64_t*>(Op->Header.Args[1]);

            uintptr_t ContextPtr = reinterpret_cast<uintptr_t>(&Thread->State.State);
            ContextPtr += Op->BaseOffset;
            ContextPtr += Index * Op->Stride;

            void *Data = reinterpret_cast<void*>(ContextPtr);
            void *Src = GetSrc<void*>(Op->Header.Args[0]);
            memcpy(Data, Src, Op->Size);
            break;
          }
          case IR::OP_LOADFLAG: {
            auto Op = IROp->C<IR::IROp_LoadFlag>();

            uintptr_t ContextPtr = reinterpret_cast<uintptr_t>(&Thread->State.State);
            ContextPtr += offsetof(FEXCore::Core::CPUState, flags[0]);
            ContextPtr += Op->Flag;
            uint8_t const *Data = reinterpret_cast<uint8_t const*>(ContextPtr);
            GD = *Data;
            break;
          }
          case IR::OP_STOREFLAG: {
            auto Op = IROp->C<IR::IROp_StoreFlag>();
            uint8_t Arg = *GetSrc<uint8_t*>(Op->Header.Args[0]) & 1;

            uintptr_t ContextPtr = reinterpret_cast<uintptr_t>(&Thread->State.State);
            ContextPtr += offsetof(FEXCore::Core::CPUState, flags[0]);
            ContextPtr += Op->Flag;
            uint8_t *Data = reinterpret_cast<uint8_t*>(ContextPtr);
            *Data = Arg;
            break;
          }
          case IR::OP_LOADMEM: {
            auto Op = IROp->C<IR::IROp_LoadMem>();
            void const *Data{};
            if (Thread->CTX->Config.UnifiedMemory) {
              Data = *GetSrc<void const**>(Op->Header.Args[0]);
            }
            else {
              Data = Thread->CTX->MemoryMapper.GetPointer<void const*>(*GetSrc<uint64_t*>(Op->Header.Args[0]));
              LogMan::Throw::A(Data != nullptr, "Couldn't Map pointer to 0x%lx\n", *GetSrc<uint64_t*>(Op->Header.Args[0]));
            }
            memcpy(GDP, Data, OpSize);
            break;
          }
          case IR::OP_STOREMEM: {
            #define STORE_DATA(x, y) \
              case x: { \
                y *Data{}; \
                if (Thread->CTX->Config.UnifiedMemory) { \
                  Data = *GetSrc<y**>(Op->Header.Args[0]); \
                } \
                else { \
                  Data = Thread->CTX->MemoryMapper.GetPointer<y*>(*GetSrc<uint64_t*>(Op->Header.Args[0])); \
                  LogMan::Throw::A(Data != nullptr, "Couldn't Map pointer to 0x%lx\n", *GetSrc<uint64_t*>(Op->Header.Args[0])); \
                } \
                memcpy(Data, GetSrc<y*>(Op->Header.Args[1]), sizeof(y)); \
                break; \
              }

            auto Op = IROp->C<IR::IROp_StoreMem>();

            switch (Op->Size) {
              STORE_DATA(1, uint8_t)
              STORE_DATA(2, uint16_t)
              STORE_DATA(4, uint32_t)
              STORE_DATA(8, uint64_t)
              case 16: {
                void *Data{};
                if (Thread->CTX->Config.UnifiedMemory) {
                  Data = *GetSrc<void**>(Op->Header.Args[0]);
                }
                else {
                  Data = Thread->CTX->MemoryMapper.GetPointer<void*>(*GetSrc<uint64_t*>(Op->Header.Args[0]));
                  LogMan::Throw::A(Data != nullptr, "Couldn't Map pointer to 0x%lx\n", *GetSrc<uint64_t*>(Op->Header.Args[0]));
                }

                void *Src = GetSrc<void*>(Op->Header.Args[1]);
                memcpy(Data, Src, 16);
                break;
              }
              default: LogMan::Msg::A("Unhandled StoreMem size"); break;
            }
            #undef STORE_DATA
            break;
          }
          #define DO_OP(size, type, func)              \
            case size: {                                      \
            auto *Dst_d  = reinterpret_cast<type*>(GDP);  \
            auto *Src1_d = reinterpret_cast<type*>(Src1); \
            auto *Src2_d = reinterpret_cast<type*>(Src2); \
            *Dst_d = func(*Src1_d, *Src2_d);          \
            break;                                            \
            }

          case IR::OP_ADD: {
            auto Op = IROp->C<IR::IROp_Add>();
            void *Src1 = GetSrc<void*>(Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(Op->Header.Args[1]);
            auto Func = [](auto a, auto b) { return a + b; };

            switch (OpSize) {
              DO_OP(1, uint8_t,  Func)
              DO_OP(2, uint16_t, Func)
              DO_OP(4, uint32_t, Func)
              DO_OP(8, uint64_t, Func)
              default: LogMan::Msg::A("Unknown Size: %d", OpSize); break;
            }
            break;
          }
          case IR::OP_SUB: {
            auto Op = IROp->C<IR::IROp_Sub>();
            void *Src1 = GetSrc<void*>(Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(Op->Header.Args[1]);
            auto Func = [](auto a, auto b) { return a - b; };

            switch (OpSize) {
              DO_OP(1, uint8_t,  Func)
              DO_OP(2, uint16_t, Func)
              DO_OP(4, uint32_t, Func)
              DO_OP(8, uint64_t, Func)
              default: LogMan::Msg::A("Unknown Size: %d", OpSize); break;
            }
            break;
          }
          case IR::OP_NEG: {
            auto Op = IROp->C<IR::IROp_Neg>();
            uint64_t Src = *GetSrc<int64_t*>(Op->Header.Args[0]);
            switch (OpSize) {
              case 1:
                GD = -static_cast<int8_t>(Src);
                break;
              case 2:
                GD = -static_cast<int16_t>(Src);
                break;
              case 4:
                GD = -static_cast<int32_t>(Src);
                break;
              case 8:
                GD = -static_cast<int64_t>(Src);
                break;
              default: LogMan::Msg::A("Unknown NEG Size: %d\n", OpSize); break;
            };
            break;
          }
          case IR::OP_OR: {
            auto Op = IROp->C<IR::IROp_Or>();
            void *Src1 = GetSrc<void*>(Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(Op->Header.Args[1]);
            auto Func = [](auto a, auto b) { return a | b; };

            switch (OpSize) {
              DO_OP(1, uint8_t,  Func)
              DO_OP(2, uint16_t, Func)
              DO_OP(4, uint32_t, Func)
              DO_OP(8, uint64_t, Func)
              DO_OP(16, __uint128_t, Func)
              default: LogMan::Msg::A("Unknown Size: %d", OpSize); break;
            }
            break;
          }
          case IR::OP_AND: {
            auto Op = IROp->C<IR::IROp_And>();
            void *Src1 = GetSrc<void*>(Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(Op->Header.Args[1]);
            auto Func = [](auto a, auto b) { return a & b; };

            switch (OpSize) {
              DO_OP(1, uint8_t,  Func)
              DO_OP(2, uint16_t, Func)
              DO_OP(4, uint32_t, Func)
              DO_OP(8, uint64_t, Func)
              default: LogMan::Msg::A("Unknown Size: %d", OpSize); break;
            }
            break;
          }
          case IR::OP_XOR: {
            auto Op = IROp->C<IR::IROp_Xor>();
            void *Src1 = GetSrc<void*>(Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(Op->Header.Args[1]);
            auto Func = [](auto a, auto b) { return a ^ b; };

            switch (OpSize) {
              DO_OP(1, uint8_t,  Func)
              DO_OP(2, uint16_t, Func)
              DO_OP(4, uint32_t, Func)
              DO_OP(8, uint64_t, Func)
              default: LogMan::Msg::A("Unknown Size: %d", OpSize); break;
            }
            break;
          }
          case IR::OP_LSHL: {
            auto Op = IROp->C<IR::IROp_Lshl>();
            uint64_t Src1 = *GetSrc<uint64_t*>(Op->Header.Args[0]);
            uint64_t Src2 = *GetSrc<uint64_t*>(Op->Header.Args[1]);
            uint8_t Mask = OpSize * 8 - 1;
            GD = Src1 << (Src2 & Mask);
            break;
          }
          case IR::OP_LSHR: {
            auto Op = IROp->C<IR::IROp_Lshr>();
            uint64_t Src1 = *GetSrc<uint64_t*>(Op->Header.Args[0]);
            uint64_t Src2 = *GetSrc<uint64_t*>(Op->Header.Args[1]);
            uint8_t Mask = OpSize * 8 - 1;
            GD = Src1 >> (Src2 & Mask);
            break;
          }
          case IR::OP_ASHR: {
            auto Op = IROp->C<IR::IROp_Ashr>();
            uint64_t Src1 = *GetSrc<uint64_t*>(Op->Header.Args[0]);
            uint64_t Src2 = *GetSrc<uint64_t*>(Op->Header.Args[1]);
            uint8_t Mask = OpSize * 8 - 1;
            switch (OpSize) {
              case 1:
                GD = static_cast<int8_t>(Src1) >> (Src2 & Mask);
                break;
              case 2:
                GD = static_cast<int16_t>(Src1) >> (Src2 & Mask);
                break;
              case 4:
                GD = static_cast<int32_t>(Src1) >> (Src2 & Mask);
                break;
              case 8:
                GD = static_cast<int64_t>(Src1) >> (Src2 & Mask);
                break;
              default: LogMan::Msg::A("Unknown ASHR Size: %d\n", OpSize); break;
            };
            break;
          }
          case IR::OP_ROR: {
            auto Op = IROp->C<IR::IROp_Ror>();
            uint64_t Src1 = *GetSrc<uint64_t*>(Op->Header.Args[0]);
            uint64_t Src2 = *GetSrc<uint64_t*>(Op->Header.Args[1]);
            auto Ror = [] (auto In, auto R) {
            auto RotateMask = sizeof(In) * 8 - 1;
              R &= RotateMask;
              return (In >> R) | (In << (sizeof(In) * 8 - R));
            };

            switch (OpSize) {
              case 1:
                GD = Ror(static_cast<uint8_t>(Src1), static_cast<uint8_t>(Src2));
                break;
              case 2:
                GD = Ror(static_cast<uint16_t>(Src1), static_cast<uint16_t>(Src2));
                break;
              case 4:
                GD = Ror(static_cast<uint32_t>(Src1), static_cast<uint32_t>(Src2));
                break;
              case 8: {
                GD = Ror(static_cast<uint64_t>(Src1), static_cast<uint64_t>(Src2));
                break;
              }
              default: LogMan::Msg::A("Unknown ROR Size: %d\n", OpSize); break;
            }
            break;
          }
          case IR::OP_ROL: {
            auto Op = IROp->C<IR::IROp_Rol>();
            uint64_t Src1 = *GetSrc<uint64_t*>(Op->Header.Args[0]);
            uint64_t Src2 = *GetSrc<uint64_t*>(Op->Header.Args[1]);
            auto Rol = [] (auto In, auto R) {
            auto RotateMask = sizeof(In) * 8 - 1;
              R &= RotateMask;
              return (In << R) | (In >> (sizeof(In) * 8 - R));
            };

            switch (OpSize) {
            case 1:
              GD = Rol(static_cast<uint8_t>(Src1), static_cast<uint8_t>(Src2));
              break;
            case 2:
              GD = Rol(static_cast<uint16_t>(Src1), static_cast<uint16_t>(Src2));
              break;
            case 4:
              GD = Rol(static_cast<uint32_t>(Src1), static_cast<uint32_t>(Src2));
              break;
            case 8: {
              GD = Rol(static_cast<uint64_t>(Src1), static_cast<uint64_t>(Src2));
              break;
            }
            default: LogMan::Msg::A("Unknown ROL Size: %d\n", OpSize); break;
            }
            break;
          }
          case IR::OP_NOT: {
            auto Op = IROp->C<IR::IROp_Not>();
            uint64_t Src = *GetSrc<uint64_t*>(Op->Header.Args[0]);
            uint8_t Mask = OpSize * 8 - 1;
            GD = (~Src) & Mask;
            break;
          }
          case IR::OP_ZEXT: {
            auto Op = IROp->C<IR::IROp_Zext>();
            LogMan::Throw::A(Op->SrcSize <= 64, "Can't support Zext of size: %ld", Op->SrcSize);
            uint64_t Src = *GetSrc<uint64_t*>(Op->Header.Args[0]);
            if (Op->SrcSize == 64) {
              // Zext 64bit to 128bit
              __uint128_t SrcLarge = Src;
              memcpy(GDP, &SrcLarge, 16);
            }
            else {
              GD = Src & ((1ULL << Op->SrcSize) - 1);
            }
            break;
          }
          case IR::OP_SEXT: {
            auto Op = IROp->C<IR::IROp_Sext>();
            LogMan::Throw::A(Op->SrcSize <= 64, "Can't support Zext of size: %ld", Op->SrcSize);
            switch (Op->SrcSize / 8) {
              case 1:
                GD = *GetSrc<int8_t*>(Op->Header.Args[0]);
                break;
              case 2:
                GD = *GetSrc<int16_t*>(Op->Header.Args[0]);
                break;
              case 4:
                GD = *GetSrc<int32_t*>(Op->Header.Args[0]);
                break;
              case 8:
                GD = *GetSrc<uint64_t*>(Op->Header.Args[0]);
                break;
              default: LogMan::Msg::A("Unknown Sext size: %d", Op->SrcSize / 8);
            }
            break;
          }
          case IR::OP_MUL: {
            auto Op = IROp->C<IR::IROp_Mul>();
            uint64_t Src1 = *GetSrc<uint64_t*>(Op->Header.Args[0]);
            uint64_t Src2 = *GetSrc<uint64_t*>(Op->Header.Args[1]);

            switch (OpSize) {
              case 1:
                GD = static_cast<int64_t>(static_cast<int8_t>(Src1)) * static_cast<int64_t>(static_cast<int8_t>(Src2));
                break;
              case 2:
                GD = static_cast<int64_t>(static_cast<int16_t>(Src1)) * static_cast<int64_t>(static_cast<int16_t>(Src2));
                break;
              case 4:
                GD = static_cast<int64_t>(static_cast<int32_t>(Src1)) * static_cast<int64_t>(static_cast<int32_t>(Src2));
                break;
              case 8:
                GD = static_cast<int64_t>(Src1) * static_cast<int64_t>(Src2);
                break;
              case 16: {
                __int128_t Tmp = static_cast<__int128_t>(static_cast<int64_t>(Src1)) * static_cast<__int128_t>(static_cast<int64_t>(Src2));
                memcpy(GDP, &Tmp, 16);
                break;
              }
              default: LogMan::Msg::A("Unknown Mul Size: %d\n", OpSize); break;
            }
            break;
          }
          case IR::OP_MULH: {
            auto Op = IROp->C<IR::IROp_MulH>();
            uint64_t Src1 = *GetSrc<uint64_t*>(Op->Header.Args[0]);
            uint64_t Src2 = *GetSrc<uint64_t*>(Op->Header.Args[1]);

            switch (OpSize) {
              case 1: {
                int64_t Tmp = static_cast<int64_t>(static_cast<int8_t>(Src1)) * static_cast<int64_t>(static_cast<int8_t>(Src2));
                GD = Tmp >> 8;
                break;
              }
              case 2: {
                int64_t Tmp = static_cast<int64_t>(static_cast<int16_t>(Src1)) * static_cast<int64_t>(static_cast<int16_t>(Src2));
                GD = Tmp >> 16;
                break;
              }
              case 4: {
                int64_t Tmp = static_cast<int64_t>(static_cast<int32_t>(Src1)) * static_cast<int64_t>(static_cast<int32_t>(Src2));
                GD = Tmp >> 32;
                break;
              }
              case 8: {
                __int128_t Tmp = static_cast<__int128_t>(static_cast<int64_t>(Src1)) * static_cast<__int128_t>(static_cast<int64_t>(Src2));
                GD = Tmp >> 64;
              }
              break;
              default: LogMan::Msg::A("Unknown MulH Size: %d\n", OpSize); break;
            }
            break;
          }
          case IR::OP_UMUL: {
            auto Op = IROp->C<IR::IROp_UMul>();
            uint64_t Src1 = *GetSrc<uint64_t*>(Op->Header.Args[0]);
            uint64_t Src2 = *GetSrc<uint64_t*>(Op->Header.Args[1]);

            switch (OpSize) {
              case 1:
                GD = static_cast<uint8_t>(Src1) * static_cast<uint8_t>(Src2);
                break;
              case 2:
                GD = static_cast<uint16_t>(Src1) * static_cast<uint16_t>(Src2);
                break;
              case 4:
                GD = static_cast<uint32_t>(Src1) * static_cast<uint32_t>(Src2);
                break;
              case 8:
                GD = static_cast<uint64_t>(Src1) * static_cast<uint64_t>(Src2);
                break;
              case 16: {
                __uint128_t Tmp = static_cast<__uint128_t>(static_cast<uint64_t>(Src1)) * static_cast<__uint128_t>(static_cast<uint64_t>(Src2));
                memcpy(GDP, &Tmp, 16);
                break;
              }
              default: LogMan::Msg::A("Unknown UMul Size: %d\n", OpSize); break;
            }
            break;
          }
          case IR::OP_UMULH: {
            auto Op = IROp->C<IR::IROp_UMulH>();
            uint64_t Src1 = *GetSrc<uint64_t*>(Op->Header.Args[0]);
            uint64_t Src2 = *GetSrc<uint64_t*>(Op->Header.Args[1]);
            switch (OpSize) {
              case 1:
                GD = static_cast<uint16_t>(Src1) * static_cast<uint16_t>(Src2);
                GD >>= 8;
                break;
              case 2:
                GD = static_cast<uint32_t>(Src1) * static_cast<uint32_t>(Src2);
                GD >>= 16;
                break;
              case 4:
                GD = static_cast<uint64_t>(Src1) * static_cast<uint64_t>(Src2);
                GD >>= 32;
                break;
              case 8: {
                __uint128_t Tmp = static_cast<__uint128_t>(Src1) * static_cast<__uint128_t>(Src2);
                GD = Tmp >> 64;
                break;
              }
              case 16: {
                // XXX: This is incorrect
                __uint128_t Tmp = static_cast<__uint128_t>(Src1) * static_cast<__uint128_t>(Src2);
                GD = Tmp >> 64;
                break;
              }
              default: LogMan::Msg::A("Unknown UMulH Size: %d\n", OpSize); break;
            }
            break;
          }
          case IR::OP_DIV: {
            auto Op = IROp->C<IR::IROp_Div>();
            uint64_t Src1 = *GetSrc<uint64_t*>(Op->Header.Args[0]);
            uint64_t Src2 = *GetSrc<uint64_t*>(Op->Header.Args[1]);

            switch (OpSize) {
              case 1:
                GD = static_cast<int64_t>(static_cast<int8_t>(Src1)) / static_cast<int64_t>(static_cast<int8_t>(Src2));
                break;
              case 2:
                GD = static_cast<int64_t>(static_cast<int16_t>(Src1)) / static_cast<int64_t>(static_cast<int16_t>(Src2));
                break;
              case 4:
                GD = static_cast<int64_t>(static_cast<int32_t>(Src1)) / static_cast<int64_t>(static_cast<int32_t>(Src2));
                break;
              case 8:
                GD = static_cast<int64_t>(Src1) / static_cast<int64_t>(Src2);
                break;
              case 16: {
                __int128_t Tmp = *GetSrc<__int128_t*>(Op->Header.Args[0]) / *GetSrc<__int128_t*>(Op->Header.Args[1]);
                memcpy(GDP, &Tmp, 16);
                break;
              }
              default: LogMan::Msg::A("Unknown Mul Size: %d\n", OpSize); break;
            }
            break;
          }
          case IR::OP_UDIV: {
            auto Op = IROp->C<IR::IROp_UDiv>();
            uint64_t Src1 = *GetSrc<uint64_t*>(Op->Header.Args[0]);
            uint64_t Src2 = *GetSrc<uint64_t*>(Op->Header.Args[1]);

            switch (OpSize) {
              case 1:
                GD = static_cast<uint64_t>(static_cast<uint8_t>(Src1)) / static_cast<uint64_t>(static_cast<uint8_t>(Src2));
                break;
              case 2:
                GD = static_cast<uint64_t>(static_cast<uint16_t>(Src1)) / static_cast<uint64_t>(static_cast<uint16_t>(Src2));
                break;
              case 4:
                GD = static_cast<uint64_t>(static_cast<uint32_t>(Src1)) / static_cast<uint64_t>(static_cast<uint32_t>(Src2));
                break;
              case 8:
                GD = static_cast<uint64_t>(Src1) / static_cast<uint64_t>(Src2);
                break;
              case 16: {
                __uint128_t Tmp = *GetSrc<__uint128_t*>(Op->Header.Args[0]) / *GetSrc<__uint128_t*>(Op->Header.Args[1]);
                memcpy(GDP, &Tmp, 16);
                break;
              }
              default: LogMan::Msg::A("Unknown Mul Size: %d\n", OpSize); break;
            }
            break;
          }
          case IR::OP_REM: {
            auto Op = IROp->C<IR::IROp_Rem>();
            uint64_t Src1 = *GetSrc<uint64_t*>(Op->Header.Args[0]);
            uint64_t Src2 = *GetSrc<uint64_t*>(Op->Header.Args[1]);

            switch (OpSize) {
              case 1:
                GD = static_cast<int64_t>(static_cast<int8_t>(Src1)) % static_cast<int64_t>(static_cast<int8_t>(Src2));
                break;
              case 2:
                GD = static_cast<int64_t>(static_cast<int16_t>(Src1)) % static_cast<int64_t>(static_cast<int16_t>(Src2));
                break;
              case 4:
                GD = static_cast<int64_t>(static_cast<int32_t>(Src1)) % static_cast<int64_t>(static_cast<int32_t>(Src2));
                break;
              case 8:
                GD = static_cast<int64_t>(Src1) % static_cast<int64_t>(Src2);
                break;
              case 16: {
                __int128_t Tmp = *GetSrc<__int128_t*>(Op->Header.Args[0]) % *GetSrc<__int128_t*>(Op->Header.Args[1]);
                memcpy(GDP, &Tmp, 16);
                break;
              }
              default: LogMan::Msg::A("Unknown Mul Size: %d\n", OpSize); break;
            }
            break;
          }
          case IR::OP_UREM: {
            auto Op = IROp->C<IR::IROp_URem>();
            uint64_t Src1 = *GetSrc<uint64_t*>(Op->Header.Args[0]);
            uint64_t Src2 = *GetSrc<uint64_t*>(Op->Header.Args[1]);

            switch (OpSize) {
              case 1:
                GD = static_cast<uint64_t>(static_cast<uint8_t>(Src1)) % static_cast<uint64_t>(static_cast<uint8_t>(Src2));
                break;
              case 2:
                GD = static_cast<uint64_t>(static_cast<uint16_t>(Src1)) % static_cast<uint64_t>(static_cast<uint16_t>(Src2));
                break;
              case 4:
                GD = static_cast<uint64_t>(static_cast<uint32_t>(Src1)) % static_cast<uint64_t>(static_cast<uint32_t>(Src2));
                break;
              case 8:
                GD = static_cast<uint64_t>(Src1) % static_cast<uint64_t>(Src2);
                break;
              case 16: {
                __uint128_t Tmp = *GetSrc<__uint128_t*>(Op->Header.Args[0]) % *GetSrc<__uint128_t*>(Op->Header.Args[1]);
                memcpy(GDP, &Tmp, 16);
                break;
              }
              default: LogMan::Msg::A("Unknown Mul Size: %d\n", OpSize); break;
            }
            break;
          }
          case IR::OP_POPCOUNT: {
            auto Op = IROp->C<IR::IROp_Popcount>();
            uint64_t Src = *GetSrc<uint64_t*>(Op->Header.Args[0]);
            GD = __builtin_popcountl(Src);
            break;
          }
          case IR::OP_FINDLSB: {
            auto Op = IROp->C<IR::IROp_FindLSB>();
            uint64_t Src = *GetSrc<uint64_t*>(Op->Header.Args[0]);
            uint64_t Result = __builtin_ffsll(Src);
            GD = Result - 1;
            break;
          }
          case IR::OP_FINDMSB: {
            auto Op = IROp->C<IR::IROp_FindMSB>();
            switch (OpSize) {
              case 1: GD = ((24 + OpSize * 8) - __builtin_clz(*GetSrc<uint8_t*>(Op->Header.Args[0]))) - 1; break;
              case 2: GD = ((16 + OpSize * 8) - __builtin_clz(*GetSrc<uint16_t*>(Op->Header.Args[0]))) - 1; break;
              case 4: GD = (OpSize * 8 - __builtin_clz(*GetSrc<uint32_t*>(Op->Header.Args[0]))) - 1; break;
              case 8: GD = (OpSize * 8 - __builtin_clzll(*GetSrc<uint64_t*>(Op->Header.Args[0]))) - 1; break;
              default: LogMan::Msg::A("Unknown REV size: %d", OpSize); break;
            }
            break;
          }
          case IR::OP_REV: {
            auto Op = IROp->C<IR::IROp_Rev>();
            switch (OpSize) {
              case 2: GD = __builtin_bswap16(*GetSrc<uint16_t*>(Op->Header.Args[0])); break;
              case 4: GD = __builtin_bswap32(*GetSrc<uint32_t*>(Op->Header.Args[0])); break;
              case 8: GD = __builtin_bswap64(*GetSrc<uint64_t*>(Op->Header.Args[0])); break;
              default: LogMan::Msg::A("Unknown REV size: %d", OpSize); break;
            }
            break;
          }
          case IR::OP_FINDTRAILINGZEROS: {
            auto Op = IROp->C<IR::IROp_FindTrailingZeros>();
            switch (OpSize) {
              case 1: {
                auto Src = *GetSrc<uint8_t*>(Op->Header.Args[0]);
                if (Src)
                  GD = __builtin_ctz(Src);
                else
                  GD = sizeof(Src) * 8;
                break;
              }
              case 2: {
                auto Src = *GetSrc<uint16_t*>(Op->Header.Args[0]);
                if (Src)
                  GD = __builtin_ctz(Src);
                else
                  GD = sizeof(Src) * 8;
                break;
              }
              case 4: {
                auto Src = *GetSrc<uint32_t*>(Op->Header.Args[0]);
                if (Src)
                  GD = __builtin_ctz(Src);
                else
                  GD = sizeof(Src) * 8;
                break;
              }
              case 8: {
                auto Src = *GetSrc<uint64_t*>(Op->Header.Args[0]);
                if (Src)
                  GD = __builtin_ctzll(Src);
                else
                  GD = sizeof(Src) * 8;
                break;
              }
              default: LogMan::Msg::A("Unknown size: %d", OpSize); break;
            }
            break;
          }
          case IR::OP_BFI: {
            auto Op = IROp->C<IR::IROp_Bfi>();
            uint64_t SourceMask = (1ULL << Op->Width) - 1;
            if (Op->Width == 64)
              SourceMask = ~0ULL;
            uint64_t DestMask = ~(SourceMask << Op->lsb);
            uint64_t Src1 = *GetSrc<uint64_t*>(Op->Header.Args[0]);
            uint64_t Src2 = *GetSrc<uint64_t*>(Op->Header.Args[1]);
            uint64_t Res = (Src1 & DestMask) | ((Src2 & SourceMask) << Op->lsb);
            GD = Res;
            break;
          }
          case IR::OP_BFE: {
            auto Op = IROp->C<IR::IROp_Bfe>();
            LogMan::Throw::A(OpSize <= 16, "OpSize is too large for BFE: %d", OpSize);
            if (OpSize == 16) {
              LogMan::Throw::A(Op->Width <= 64, "Can't extract width of %d", Op->Width);
              __uint128_t SourceMask = (1ULL << Op->Width) - 1;
              if (Op->Width == 64)
                SourceMask = ~0ULL;
              SourceMask <<= Op->lsb;
              __uint128_t Src = (*GetSrc<__uint128_t*>(Op->Header.Args[0]) & SourceMask) >> Op->lsb;
              memcpy(GDP, &Src, OpSize);
            }
            else {
              uint64_t SourceMask = (1ULL << Op->Width) - 1;
              if (Op->Width == 64)
                SourceMask = ~0ULL;
              SourceMask <<= Op->lsb;
              uint64_t Src = *GetSrc<uint64_t*>(Op->Header.Args[0]);
              GD = (Src & SourceMask) >> Op->lsb;
            }
            break;
          }
          case IR::OP_SELECT: {
            auto Op = IROp->C<IR::IROp_Select>();
            bool CompResult = false;
            uint64_t Src1 = *GetSrc<uint64_t*>(Op->Header.Args[0]);
            uint64_t Src2 = *GetSrc<uint64_t*>(Op->Header.Args[1]);

            uint64_t ArgTrue = *GetSrc<uint64_t*>(Op->Header.Args[2]);
            uint64_t ArgFalse = *GetSrc<uint64_t*>(Op->Header.Args[3]);

            switch (Op->Cond.Val) {
              case FEXCore::IR::COND_EQ:
                CompResult = Src1 == Src2;
                break;
              case FEXCore::IR::COND_NEQ:
                CompResult = Src1 != Src2;
                break;
              case FEXCore::IR::COND_SGE:
                CompResult = static_cast<int64_t>(Src1) >= static_cast<int64_t>(Src2);
                break;
              case FEXCore::IR::COND_SLT:
                CompResult = static_cast<int64_t>(Src1) < static_cast<int64_t>(Src2);
                break;
              case FEXCore::IR::COND_SGT:
                CompResult = static_cast<int64_t>(Src1) > static_cast<int64_t>(Src2);
                break;
              case FEXCore::IR::COND_SLE:
                CompResult = static_cast<int64_t>(Src1) <= static_cast<int64_t>(Src2);
                break;
              case FEXCore::IR::COND_UGE:
                CompResult = Src1 >= Src2;
                break;
              case FEXCore::IR::COND_ULT:
                CompResult = Src1 < Src2;
                break;
              case FEXCore::IR::COND_UGT:
                CompResult = Src1 > Src2;
                break;
              case FEXCore::IR::COND_ULE:
                CompResult = Src1 <= Src2;
                break;
              case FEXCore::IR::COND_MI:
              case FEXCore::IR::COND_PL:
              case FEXCore::IR::COND_VS:
              case FEXCore::IR::COND_VC:
              default:
                LogMan::Msg::A("Unsupported compare type");
                break;
            }
            GD = CompResult ? ArgTrue : ArgFalse;
            break;
          }
          case IR::OP_CAS: {
            auto Op = IROp->C<IR::IROp_CAS>();
            auto Size = OpSize;
            switch (Size) {
              case 1: {
                std::atomic<uint8_t> *Data = {};
                if (Thread->CTX->Config.UnifiedMemory) {
                  Data = *GetSrc<std::atomic<uint8_t> **>(Op->Header.Args[2]);
                }
                else {
                  Data = Thread->CTX->MemoryMapper.GetPointer<std::atomic<uint8_t> *>(*GetSrc<uint64_t*>(Op->Header.Args[2]));
                  LogMan::Throw::A(Data != nullptr, "Couldn't Map pointer to 0x%lx\n", *GetSrc<uint64_t*>(Op->Header.Args[2]));
                }

                uint8_t Src1 = *GetSrc<uint8_t*>(Op->Header.Args[0]);
                uint8_t Src2 = *GetSrc<uint8_t*>(Op->Header.Args[1]);

                uint8_t Expected = Src1;
                bool Result = Data->compare_exchange_strong(Expected, Src2);
                GD = Result ? Src1 : Expected;
                break;
              }
              case 2: {
                std::atomic<uint16_t> *Data = {};
                if (Thread->CTX->Config.UnifiedMemory) {
                  Data = *GetSrc<std::atomic<uint16_t> **>(Op->Header.Args[2]);
                }
                else {
                  Data = Thread->CTX->MemoryMapper.GetPointer<std::atomic<uint16_t> *>(*GetSrc<uint64_t*>(Op->Header.Args[2]));
                  LogMan::Throw::A(Data != nullptr, "Couldn't Map pointer to 0x%lx\n", *GetSrc<uint64_t*>(Op->Header.Args[2]));
                }

                uint16_t Src1 = *GetSrc<uint16_t*>(Op->Header.Args[0]);
                uint16_t Src2 = *GetSrc<uint16_t*>(Op->Header.Args[1]);

                uint16_t Expected = Src1;
                bool Result = Data->compare_exchange_strong(Expected, Src2);
                GD = Result ? Src1 : Expected;
                break;
              }
              case 4: {
                std::atomic<uint32_t> *Data = {};
                if (Thread->CTX->Config.UnifiedMemory) {
                  Data = *GetSrc<std::atomic<uint32_t> **>(Op->Header.Args[2]);
                }
                else {
                  Data = Thread->CTX->MemoryMapper.GetPointer<std::atomic<uint32_t> *>(*GetSrc<uint64_t*>(Op->Header.Args[2]));
                  LogMan::Throw::A(Data != nullptr, "Couldn't Map pointer to 0x%lx\n", *GetSrc<uint64_t*>(Op->Header.Args[2]));
                }

                uint32_t Src1 = *GetSrc<uint32_t*>(Op->Header.Args[0]);
                uint32_t Src2 = *GetSrc<uint32_t*>(Op->Header.Args[1]);

                uint32_t Expected = Src1;
                bool Result = Data->compare_exchange_strong(Expected, Src2);
                GD = Result ? Src1 : Expected;
                break;
              }
              case 8: {
                std::atomic<uint64_t> *Data = {};
                if (Thread->CTX->Config.UnifiedMemory) {
                  Data = *GetSrc<std::atomic<uint64_t> **>(Op->Header.Args[2]);
                }
                else {
                  Data = Thread->CTX->MemoryMapper.GetPointer<std::atomic<uint64_t> *>(*GetSrc<uint64_t*>(Op->Header.Args[2]));
                  LogMan::Throw::A(Data != nullptr, "Couldn't Map pointer to 0x%lx\n", *GetSrc<uint64_t*>(Op->Header.Args[2]));
                }

                uint64_t Src1 = *GetSrc<uint64_t*>(Op->Header.Args[0]);
                uint64_t Src2 = *GetSrc<uint64_t*>(Op->Header.Args[1]);

                uint64_t Expected = Src1;
                bool Result = Data->compare_exchange_strong(Expected, Src2);
                GD = Result ? Src1 : Expected;
                break;
              }
              default: LogMan::Msg::A("Unknown CAS size: %d", Size); break;
            }
            break;
          }
          case IR::OP_ATOMICADD: {
            auto Op = IROp->C<IR::IROp_AtomicAdd>();
            switch (Op->Size) {
              case 1: {
                std::atomic<uint8_t> *Data = {};
                if (Thread->CTX->Config.UnifiedMemory) {
                  Data = *GetSrc<std::atomic<uint8_t> **>(Op->Header.Args[0]);
                }
                else {
                  Data = Thread->CTX->MemoryMapper.GetPointer<std::atomic<uint8_t> *>(*GetSrc<uint64_t*>(Op->Header.Args[0]));
                  LogMan::Throw::A(Data != nullptr, "Couldn't Map pointer to 0x%lx\n", *GetSrc<uint64_t*>(Op->Header.Args[0]));
                }

                uint8_t Src = *GetSrc<uint8_t*>(Op->Header.Args[1]);
                *Data += Src;
                break;
              }
              case 2: {
                std::atomic<uint16_t> *Data = {};
                if (Thread->CTX->Config.UnifiedMemory) {
                  Data = *GetSrc<std::atomic<uint16_t> **>(Op->Header.Args[0]);
                }
                else {
                  Data = Thread->CTX->MemoryMapper.GetPointer<std::atomic<uint16_t> *>(*GetSrc<uint64_t*>(Op->Header.Args[0]));
                  LogMan::Throw::A(Data != nullptr, "Couldn't Map pointer to 0x%lx\n", *GetSrc<uint64_t*>(Op->Header.Args[0]));
                }
                LogMan::Throw::A(Data != nullptr, "Couldn't Map pointer to 0x%lx\n", *GetSrc<uint16_t*>(Op->Header.Args[0]));
                uint16_t Src = *GetSrc<uint16_t*>(Op->Header.Args[1]);
                *Data += Src;
                break;
              }
              case 4: {
                std::atomic<uint32_t> *Data = {};
                if (Thread->CTX->Config.UnifiedMemory) {
                  Data = *GetSrc<std::atomic<uint32_t> **>(Op->Header.Args[0]);
                }
                else {
                  Data = Thread->CTX->MemoryMapper.GetPointer<std::atomic<uint32_t> *>(*GetSrc<uint64_t*>(Op->Header.Args[0]));
                  LogMan::Throw::A(Data != nullptr, "Couldn't Map pointer to 0x%lx\n", *GetSrc<uint64_t*>(Op->Header.Args[0]));
                }
                uint32_t Src = *GetSrc<uint32_t*>(Op->Header.Args[1]);
                *Data += Src;
                break;
              }
              case 8: {
                std::atomic<uint64_t> *Data = {};
                if (Thread->CTX->Config.UnifiedMemory) {
                  Data = *GetSrc<std::atomic<uint64_t> **>(Op->Header.Args[0]);
                }
                else {
                  Data = Thread->CTX->MemoryMapper.GetPointer<std::atomic<uint64_t> *>(*GetSrc<uint64_t*>(Op->Header.Args[0]));
                  LogMan::Throw::A(Data != nullptr, "Couldn't Map pointer to 0x%lx\n", *GetSrc<uint64_t*>(Op->Header.Args[0]));
                }
                uint64_t Src = *GetSrc<uint64_t*>(Op->Header.Args[1]);
                *Data += Src;
                break;
              }
              default:  LogMan::Msg::A("Unhandled Atomic size: %d", Op->Size);
            }
            break;
          }
          case IR::OP_ATOMICSUB: {
            auto Op = IROp->C<IR::IROp_AtomicSub>();
            switch (Op->Size) {
              case 1: {
                std::atomic<uint8_t> *Data = {};
                if (Thread->CTX->Config.UnifiedMemory) {
                  Data = *GetSrc<std::atomic<uint8_t> **>(Op->Header.Args[0]);
                }
                else {
                  Data = Thread->CTX->MemoryMapper.GetPointer<std::atomic<uint8_t> *>(*GetSrc<uint64_t*>(Op->Header.Args[0]));
                  LogMan::Throw::A(Data != nullptr, "Couldn't Map pointer to 0x%lx\n", *GetSrc<uint64_t*>(Op->Header.Args[0]));
                }
                uint8_t Src = *GetSrc<uint8_t*>(Op->Header.Args[1]);
                *Data -= Src;
                break;
              }
              case 2: {
                std::atomic<uint16_t> *Data = {};
                if (Thread->CTX->Config.UnifiedMemory) {
                  Data = *GetSrc<std::atomic<uint16_t> **>(Op->Header.Args[0]);
                }
                else {
                  Data = Thread->CTX->MemoryMapper.GetPointer<std::atomic<uint16_t> *>(*GetSrc<uint64_t*>(Op->Header.Args[0]));
                  LogMan::Throw::A(Data != nullptr, "Couldn't Map pointer to 0x%lx\n", *GetSrc<uint64_t*>(Op->Header.Args[0]));
                }
                uint16_t Src = *GetSrc<uint16_t*>(Op->Header.Args[1]);
                *Data -= Src;
                break;
              }
              case 4: {
                std::atomic<uint32_t> *Data = {};
                if (Thread->CTX->Config.UnifiedMemory) {
                  Data = *GetSrc<std::atomic<uint32_t> **>(Op->Header.Args[0]);
                }
                else {
                  Data = Thread->CTX->MemoryMapper.GetPointer<std::atomic<uint32_t> *>(*GetSrc<uint64_t*>(Op->Header.Args[0]));
                  LogMan::Throw::A(Data != nullptr, "Couldn't Map pointer to 0x%lx\n", *GetSrc<uint64_t*>(Op->Header.Args[0]));
                }
                uint32_t Src = *GetSrc<uint32_t*>(Op->Header.Args[1]);
                *Data -= Src;
                break;
              }
              case 8: {
                std::atomic<uint64_t> *Data = {};
                if (Thread->CTX->Config.UnifiedMemory) {
                  Data = *GetSrc<std::atomic<uint64_t> **>(Op->Header.Args[0]);
                }
                else {
                  Data = Thread->CTX->MemoryMapper.GetPointer<std::atomic<uint64_t> *>(*GetSrc<uint64_t*>(Op->Header.Args[0]));
                  LogMan::Throw::A(Data != nullptr, "Couldn't Map pointer to 0x%lx\n", *GetSrc<uint64_t*>(Op->Header.Args[0]));
                }
                uint64_t Src = *GetSrc<uint64_t*>(Op->Header.Args[1]);
                *Data -= Src;
                break;
              }
              default:  LogMan::Msg::A("Unhandled Atomic size: %d", Op->Size);
            }
            break;
          }
          case IR::OP_ATOMICAND: {
            auto Op = IROp->C<IR::IROp_AtomicAnd>();
            switch (Op->Size) {
              case 1: {
                std::atomic<uint8_t> *Data = {};
                if (Thread->CTX->Config.UnifiedMemory) {
                  Data = *GetSrc<std::atomic<uint8_t> **>(Op->Header.Args[0]);
                }
                else {
                  Data = Thread->CTX->MemoryMapper.GetPointer<std::atomic<uint8_t> *>(*GetSrc<uint64_t*>(Op->Header.Args[0]));
                  LogMan::Throw::A(Data != nullptr, "Couldn't Map pointer to 0x%lx\n", *GetSrc<uint64_t*>(Op->Header.Args[0]));
                }
                uint8_t Src = *GetSrc<uint8_t*>(Op->Header.Args[1]);
                *Data &= Src;
                break;
              }
              case 2: {
                std::atomic<uint16_t> *Data = {};
                if (Thread->CTX->Config.UnifiedMemory) {
                  Data = *GetSrc<std::atomic<uint16_t> **>(Op->Header.Args[0]);
                }
                else {
                  Data = Thread->CTX->MemoryMapper.GetPointer<std::atomic<uint16_t> *>(*GetSrc<uint64_t*>(Op->Header.Args[0]));
                  LogMan::Throw::A(Data != nullptr, "Couldn't Map pointer to 0x%lx\n", *GetSrc<uint64_t*>(Op->Header.Args[0]));
                }
                uint16_t Src = *GetSrc<uint16_t*>(Op->Header.Args[1]);
                *Data &= Src;
                break;
              }
              case 4: {
                std::atomic<uint32_t> *Data = {};
                if (Thread->CTX->Config.UnifiedMemory) {
                  Data = *GetSrc<std::atomic<uint32_t> **>(Op->Header.Args[0]);
                }
                else {
                  Data = Thread->CTX->MemoryMapper.GetPointer<std::atomic<uint32_t> *>(*GetSrc<uint64_t*>(Op->Header.Args[0]));
                  LogMan::Throw::A(Data != nullptr, "Couldn't Map pointer to 0x%lx\n", *GetSrc<uint64_t*>(Op->Header.Args[0]));
                }
                uint32_t Src = *GetSrc<uint32_t*>(Op->Header.Args[1]);
                *Data &= Src;
                break;
              }
              case 8: {
                std::atomic<uint64_t> *Data = {};
                if (Thread->CTX->Config.UnifiedMemory) {
                  Data = *GetSrc<std::atomic<uint64_t> **>(Op->Header.Args[0]);
                }
                else {
                  Data = Thread->CTX->MemoryMapper.GetPointer<std::atomic<uint64_t> *>(*GetSrc<uint64_t*>(Op->Header.Args[0]));
                  LogMan::Throw::A(Data != nullptr, "Couldn't Map pointer to 0x%lx\n", *GetSrc<uint64_t*>(Op->Header.Args[0]));
                }
                uint64_t Src = *GetSrc<uint64_t*>(Op->Header.Args[1]);
                *Data &= Src;
                break;
              }
              default:  LogMan::Msg::A("Unhandled Atomic size: %d", Op->Size);
            }
            break;
          }
          case IR::OP_ATOMICOR: {
            auto Op = IROp->C<IR::IROp_AtomicOr>();
            switch (Op->Size) {
              case 1: {
                std::atomic<uint8_t> *Data = {};
                if (Thread->CTX->Config.UnifiedMemory) {
                  Data = *GetSrc<std::atomic<uint8_t> **>(Op->Header.Args[0]);
                }
                else {
                  Data = Thread->CTX->MemoryMapper.GetPointer<std::atomic<uint8_t> *>(*GetSrc<uint64_t*>(Op->Header.Args[0]));
                  LogMan::Throw::A(Data != nullptr, "Couldn't Map pointer to 0x%lx\n", *GetSrc<uint64_t*>(Op->Header.Args[0]));
                }
                LogMan::Throw::A(Data != nullptr, "Couldn't Map pointer to 0x%lx\n", *GetSrc<uint8_t*>(Op->Header.Args[0]));
                uint8_t Src = *GetSrc<uint8_t*>(Op->Header.Args[1]);
                *Data |= Src;
                break;
              }
              case 2: {
                std::atomic<uint16_t> *Data = {};
                if (Thread->CTX->Config.UnifiedMemory) {
                  Data = *GetSrc<std::atomic<uint16_t> **>(Op->Header.Args[0]);
                }
                else {
                  Data = Thread->CTX->MemoryMapper.GetPointer<std::atomic<uint16_t> *>(*GetSrc<uint64_t*>(Op->Header.Args[0]));
                  LogMan::Throw::A(Data != nullptr, "Couldn't Map pointer to 0x%lx\n", *GetSrc<uint64_t*>(Op->Header.Args[0]));
                }
                LogMan::Throw::A(Data != nullptr, "Couldn't Map pointer to 0x%lx\n", *GetSrc<uint16_t*>(Op->Header.Args[0]));
                uint16_t Src = *GetSrc<uint16_t*>(Op->Header.Args[1]);
                *Data |= Src;
                break;
              }
              case 4: {
                std::atomic<uint32_t> *Data = {};
                if (Thread->CTX->Config.UnifiedMemory) {
                  Data = *GetSrc<std::atomic<uint32_t> **>(Op->Header.Args[0]);
                }
                else {
                  Data = Thread->CTX->MemoryMapper.GetPointer<std::atomic<uint32_t> *>(*GetSrc<uint64_t*>(Op->Header.Args[0]));
                  LogMan::Throw::A(Data != nullptr, "Couldn't Map pointer to 0x%lx\n", *GetSrc<uint64_t*>(Op->Header.Args[0]));
                }
                uint32_t Src = *GetSrc<uint32_t*>(Op->Header.Args[1]);
                *Data |= Src;
                break;
              }
              case 8: {
                std::atomic<uint64_t> *Data = {};
                if (Thread->CTX->Config.UnifiedMemory) {
                  Data = *GetSrc<std::atomic<uint64_t> **>(Op->Header.Args[0]);
                }
                else {
                  Data = Thread->CTX->MemoryMapper.GetPointer<std::atomic<uint64_t> *>(*GetSrc<uint64_t*>(Op->Header.Args[0]));
                  LogMan::Throw::A(Data != nullptr, "Couldn't Map pointer to 0x%lx\n", *GetSrc<uint64_t*>(Op->Header.Args[0]));
                }
                uint64_t Src = *GetSrc<uint64_t*>(Op->Header.Args[1]);
                *Data |= Src;
                break;
              }
              default:  LogMan::Msg::A("Unhandled Atomic size: %d", Op->Size);
            }
            break;
          }
          case IR::OP_ATOMICXOR: {
            auto Op = IROp->C<IR::IROp_AtomicXor>();
            switch (Op->Size) {
              case 1: {
                std::atomic<uint8_t> *Data = {};
                if (Thread->CTX->Config.UnifiedMemory) {
                  Data = *GetSrc<std::atomic<uint8_t> **>(Op->Header.Args[0]);
                }
                else {
                  Data = Thread->CTX->MemoryMapper.GetPointer<std::atomic<uint8_t> *>(*GetSrc<uint64_t*>(Op->Header.Args[0]));
                  LogMan::Throw::A(Data != nullptr, "Couldn't Map pointer to 0x%lx\n", *GetSrc<uint64_t*>(Op->Header.Args[0]));
                }
                uint8_t Src = *GetSrc<uint8_t*>(Op->Header.Args[1]);
                *Data ^= Src;
                break;
              }
              case 2: {
                std::atomic<uint16_t> *Data = {};
                if (Thread->CTX->Config.UnifiedMemory) {
                  Data = *GetSrc<std::atomic<uint16_t> **>(Op->Header.Args[0]);
                }
                else {
                  Data = Thread->CTX->MemoryMapper.GetPointer<std::atomic<uint16_t> *>(*GetSrc<uint64_t*>(Op->Header.Args[0]));
                  LogMan::Throw::A(Data != nullptr, "Couldn't Map pointer to 0x%lx\n", *GetSrc<uint64_t*>(Op->Header.Args[0]));
                }
                LogMan::Throw::A(Data != nullptr, "Couldn't Map pointer to 0x%lx\n", *GetSrc<uint16_t*>(Op->Header.Args[0]));
                uint16_t Src = *GetSrc<uint16_t*>(Op->Header.Args[1]);
                *Data ^= Src;
                break;
              }
              case 4: {
                std::atomic<uint32_t> *Data = {};
                if (Thread->CTX->Config.UnifiedMemory) {
                  Data = *GetSrc<std::atomic<uint32_t> **>(Op->Header.Args[0]);
                }
                else {
                  Data = Thread->CTX->MemoryMapper.GetPointer<std::atomic<uint32_t> *>(*GetSrc<uint64_t*>(Op->Header.Args[0]));
                  LogMan::Throw::A(Data != nullptr, "Couldn't Map pointer to 0x%lx\n", *GetSrc<uint64_t*>(Op->Header.Args[0]));
                }
                uint32_t Src = *GetSrc<uint32_t*>(Op->Header.Args[1]);
                *Data ^= Src;
                break;
              }
              case 8: {
                std::atomic<uint64_t> *Data = {};
                if (Thread->CTX->Config.UnifiedMemory) {
                  Data = *GetSrc<std::atomic<uint64_t> **>(Op->Header.Args[0]);
                }
                else {
                  Data = Thread->CTX->MemoryMapper.GetPointer<std::atomic<uint64_t> *>(*GetSrc<uint64_t*>(Op->Header.Args[0]));
                  LogMan::Throw::A(Data != nullptr, "Couldn't Map pointer to 0x%lx\n", *GetSrc<uint64_t*>(Op->Header.Args[0]));
                }
                uint64_t Src = *GetSrc<uint64_t*>(Op->Header.Args[1]);
                *Data ^= Src;
                break;
              }
              default:  LogMan::Msg::A("Unhandled Atomic size: %d", Op->Size);
            }
            break;
          }
          case IR::OP_ATOMICSWAP: {
            auto Op = IROp->C<IR::IROp_AtomicSwap>();
            switch (Op->Size) {
              case 1: {
                std::atomic<uint8_t> *Data = {};
                if (Thread->CTX->Config.UnifiedMemory) {
                  Data = *GetSrc<std::atomic<uint8_t> **>(Op->Header.Args[0]);
                }
                else {
                  Data = Thread->CTX->MemoryMapper.GetPointer<std::atomic<uint8_t> *>(*GetSrc<uint64_t*>(Op->Header.Args[0]));
                  LogMan::Throw::A(Data != nullptr, "Couldn't Map pointer to 0x%lx\n", *GetSrc<uint64_t*>(Op->Header.Args[0]));
                }
                uint8_t Src = *GetSrc<uint8_t*>(Op->Header.Args[1]);
                uint8_t Previous = Data->exchange(Src);
                GD = Previous;
                break;
              }
              case 2: {
                std::atomic<uint16_t> *Data = {};
                if (Thread->CTX->Config.UnifiedMemory) {
                  Data = *GetSrc<std::atomic<uint16_t> **>(Op->Header.Args[0]);
                }
                else {
                  Data = Thread->CTX->MemoryMapper.GetPointer<std::atomic<uint16_t> *>(*GetSrc<uint64_t*>(Op->Header.Args[0]));
                  LogMan::Throw::A(Data != nullptr, "Couldn't Map pointer to 0x%lx\n", *GetSrc<uint64_t*>(Op->Header.Args[0]));
                }
                uint16_t Src = *GetSrc<uint16_t*>(Op->Header.Args[1]);
                uint16_t Previous = Data->exchange(Src);
                GD = Previous;
                break;
              }
              case 4: {
                std::atomic<uint32_t> *Data = {};
                if (Thread->CTX->Config.UnifiedMemory) {
                  Data = *GetSrc<std::atomic<uint32_t> **>(Op->Header.Args[0]);
                }
                else {
                  Data = Thread->CTX->MemoryMapper.GetPointer<std::atomic<uint32_t> *>(*GetSrc<uint64_t*>(Op->Header.Args[0]));
                  LogMan::Throw::A(Data != nullptr, "Couldn't Map pointer to 0x%lx\n", *GetSrc<uint64_t*>(Op->Header.Args[0]));
                }
                uint32_t Src = *GetSrc<uint32_t*>(Op->Header.Args[1]);
                uint32_t Previous = Data->exchange(Src);
                GD = Previous;
                break;
              }
              case 8: {
                std::atomic<uint64_t> *Data = {};
                if (Thread->CTX->Config.UnifiedMemory) {
                  Data = *GetSrc<std::atomic<uint64_t> **>(Op->Header.Args[0]);
                }
                else {
                  Data = Thread->CTX->MemoryMapper.GetPointer<std::atomic<uint64_t> *>(*GetSrc<uint64_t*>(Op->Header.Args[0]));
                  LogMan::Throw::A(Data != nullptr, "Couldn't Map pointer to 0x%lx\n", *GetSrc<uint64_t*>(Op->Header.Args[0]));
                }
                uint64_t Src = *GetSrc<uint64_t*>(Op->Header.Args[1]);
                uint64_t Previous = Data->exchange(Src);
                GD = Previous;
                break;
              }
              default:  LogMan::Msg::A("Unhandled Atomic size: %d", Op->Size);
            }
            break;
          }
          case IR::OP_ATOMICFETCHADD: {
            auto Op = IROp->C<IR::IROp_AtomicFetchAdd>();
            switch (Op->Size) {
              case 1: {
                std::atomic<uint8_t> *Data = {};
                if (Thread->CTX->Config.UnifiedMemory) {
                  Data = *GetSrc<std::atomic<uint8_t> **>(Op->Header.Args[0]);
                }
                else {
                  Data = Thread->CTX->MemoryMapper.GetPointer<std::atomic<uint8_t> *>(*GetSrc<uint64_t*>(Op->Header.Args[0]));
                  LogMan::Throw::A(Data != nullptr, "Couldn't Map pointer to 0x%lx\n", *GetSrc<uint64_t*>(Op->Header.Args[0]));
                }
                uint8_t Src = *GetSrc<uint8_t*>(Op->Header.Args[1]);
                uint8_t Previous = Data->fetch_add(Src);
                GD = Previous;
                break;
              }
              case 2: {
                std::atomic<uint16_t> *Data = {};
                if (Thread->CTX->Config.UnifiedMemory) {
                  Data = *GetSrc<std::atomic<uint16_t> **>(Op->Header.Args[0]);
                }
                else {
                  Data = Thread->CTX->MemoryMapper.GetPointer<std::atomic<uint16_t> *>(*GetSrc<uint64_t*>(Op->Header.Args[0]));
                  LogMan::Throw::A(Data != nullptr, "Couldn't Map pointer to 0x%lx\n", *GetSrc<uint64_t*>(Op->Header.Args[0]));
                }
                uint16_t Src = *GetSrc<uint16_t*>(Op->Header.Args[1]);
                uint16_t Previous = Data->fetch_add(Src);
                GD = Previous;
                break;
              }
              case 4: {
                std::atomic<uint32_t> *Data = {};
                if (Thread->CTX->Config.UnifiedMemory) {
                  Data = *GetSrc<std::atomic<uint32_t> **>(Op->Header.Args[0]);
                }
                else {
                  Data = Thread->CTX->MemoryMapper.GetPointer<std::atomic<uint32_t> *>(*GetSrc<uint64_t*>(Op->Header.Args[0]));
                  LogMan::Throw::A(Data != nullptr, "Couldn't Map pointer to 0x%lx\n", *GetSrc<uint64_t*>(Op->Header.Args[0]));
                }
                uint32_t Src = *GetSrc<uint32_t*>(Op->Header.Args[1]);
                uint32_t Previous = Data->fetch_add(Src);
                GD = Previous;
                break;
              }
              case 8: {
                std::atomic<uint64_t> *Data = {};
                if (Thread->CTX->Config.UnifiedMemory) {
                  Data = *GetSrc<std::atomic<uint64_t> **>(Op->Header.Args[0]);
                }
                else {
                  Data = Thread->CTX->MemoryMapper.GetPointer<std::atomic<uint64_t> *>(*GetSrc<uint64_t*>(Op->Header.Args[0]));
                  LogMan::Throw::A(Data != nullptr, "Couldn't Map pointer to 0x%lx\n", *GetSrc<uint64_t*>(Op->Header.Args[0]));
                }
                uint64_t Src = *GetSrc<uint64_t*>(Op->Header.Args[1]);
                uint64_t Previous = Data->fetch_add(Src);
                GD = Previous;
                break;
              }
              default:  LogMan::Msg::A("Unhandled Atomic size: %d", Op->Size);
            }
            break;
          }
          case IR::OP_ATOMICFETCHSUB: {
            auto Op = IROp->C<IR::IROp_AtomicFetchSub>();
            switch (Op->Size) {
              case 1: {
                std::atomic<uint8_t> *Data = {};
                if (Thread->CTX->Config.UnifiedMemory) {
                  Data = *GetSrc<std::atomic<uint8_t> **>(Op->Header.Args[0]);
                }
                else {
                  Data = Thread->CTX->MemoryMapper.GetPointer<std::atomic<uint8_t> *>(*GetSrc<uint64_t*>(Op->Header.Args[0]));
                  LogMan::Throw::A(Data != nullptr, "Couldn't Map pointer to 0x%lx\n", *GetSrc<uint64_t*>(Op->Header.Args[0]));
                }
                uint8_t Src = *GetSrc<uint8_t*>(Op->Header.Args[1]);
                uint8_t Previous = Data->fetch_sub(Src);
                GD = Previous;
                break;
              }
              case 2: {
                std::atomic<uint16_t> *Data = {};
                if (Thread->CTX->Config.UnifiedMemory) {
                  Data = *GetSrc<std::atomic<uint16_t> **>(Op->Header.Args[0]);
                }
                else {
                  Data = Thread->CTX->MemoryMapper.GetPointer<std::atomic<uint16_t> *>(*GetSrc<uint64_t*>(Op->Header.Args[0]));
                  LogMan::Throw::A(Data != nullptr, "Couldn't Map pointer to 0x%lx\n", *GetSrc<uint64_t*>(Op->Header.Args[0]));
                }
                uint16_t Src = *GetSrc<uint16_t*>(Op->Header.Args[1]);
                uint16_t Previous = Data->fetch_sub(Src);
                GD = Previous;
                break;
              }
              case 4: {
                std::atomic<uint32_t> *Data = {};
                if (Thread->CTX->Config.UnifiedMemory) {
                  Data = *GetSrc<std::atomic<uint32_t> **>(Op->Header.Args[0]);
                }
                else {
                  Data = Thread->CTX->MemoryMapper.GetPointer<std::atomic<uint32_t> *>(*GetSrc<uint64_t*>(Op->Header.Args[0]));
                  LogMan::Throw::A(Data != nullptr, "Couldn't Map pointer to 0x%lx\n", *GetSrc<uint64_t*>(Op->Header.Args[0]));
                }
                uint32_t Src = *GetSrc<uint32_t*>(Op->Header.Args[1]);
                uint32_t Previous = Data->fetch_sub(Src);
                GD = Previous;
                break;
              }
              case 8: {
                std::atomic<uint64_t> *Data = {};
                if (Thread->CTX->Config.UnifiedMemory) {
                  Data = *GetSrc<std::atomic<uint64_t> **>(Op->Header.Args[0]);
                }
                else {
                  Data = Thread->CTX->MemoryMapper.GetPointer<std::atomic<uint64_t> *>(*GetSrc<uint64_t*>(Op->Header.Args[0]));
                  LogMan::Throw::A(Data != nullptr, "Couldn't Map pointer to 0x%lx\n", *GetSrc<uint64_t*>(Op->Header.Args[0]));
                }
                uint64_t Src = *GetSrc<uint64_t*>(Op->Header.Args[1]);
                uint64_t Previous = Data->fetch_sub(Src);
                GD = Previous;
                break;
              }
              default:  LogMan::Msg::A("Unhandled Atomic size: %d", Op->Size);
            }
            break;
          }
          case IR::OP_ATOMICFETCHAND: {
            auto Op = IROp->C<IR::IROp_AtomicFetchAnd>();
            switch (Op->Size) {
              case 1: {
                std::atomic<uint8_t> *Data = {};
                if (Thread->CTX->Config.UnifiedMemory) {
                  Data = *GetSrc<std::atomic<uint8_t> **>(Op->Header.Args[0]);
                }
                else {
                  Data = Thread->CTX->MemoryMapper.GetPointer<std::atomic<uint8_t> *>(*GetSrc<uint64_t*>(Op->Header.Args[0]));
                  LogMan::Throw::A(Data != nullptr, "Couldn't Map pointer to 0x%lx\n", *GetSrc<uint64_t*>(Op->Header.Args[0]));
                }
                uint8_t Src = *GetSrc<uint8_t*>(Op->Header.Args[1]);
                uint8_t Previous = Data->fetch_and(Src);
                GD = Previous;
                break;
              }
              case 2: {
                std::atomic<uint16_t> *Data = {};
                if (Thread->CTX->Config.UnifiedMemory) {
                  Data = *GetSrc<std::atomic<uint16_t> **>(Op->Header.Args[0]);
                }
                else {
                  Data = Thread->CTX->MemoryMapper.GetPointer<std::atomic<uint16_t> *>(*GetSrc<uint64_t*>(Op->Header.Args[0]));
                  LogMan::Throw::A(Data != nullptr, "Couldn't Map pointer to 0x%lx\n", *GetSrc<uint64_t*>(Op->Header.Args[0]));
                }
                uint16_t Src = *GetSrc<uint16_t*>(Op->Header.Args[1]);
                uint16_t Previous = Data->fetch_and(Src);
                GD = Previous;
                break;
              }
              case 4: {
                std::atomic<uint32_t> *Data = {};
                if (Thread->CTX->Config.UnifiedMemory) {
                  Data = *GetSrc<std::atomic<uint32_t> **>(Op->Header.Args[0]);
                }
                else {
                  Data = Thread->CTX->MemoryMapper.GetPointer<std::atomic<uint32_t> *>(*GetSrc<uint64_t*>(Op->Header.Args[0]));
                  LogMan::Throw::A(Data != nullptr, "Couldn't Map pointer to 0x%lx\n", *GetSrc<uint64_t*>(Op->Header.Args[0]));
                }
                uint32_t Src = *GetSrc<uint32_t*>(Op->Header.Args[1]);
                uint32_t Previous = Data->fetch_and(Src);
                GD = Previous;
                break;
              }
              case 8: {
                std::atomic<uint64_t> *Data = {};
                if (Thread->CTX->Config.UnifiedMemory) {
                  Data = *GetSrc<std::atomic<uint64_t> **>(Op->Header.Args[0]);
                }
                else {
                  Data = Thread->CTX->MemoryMapper.GetPointer<std::atomic<uint64_t> *>(*GetSrc<uint64_t*>(Op->Header.Args[0]));
                  LogMan::Throw::A(Data != nullptr, "Couldn't Map pointer to 0x%lx\n", *GetSrc<uint64_t*>(Op->Header.Args[0]));
                }
                uint64_t Src = *GetSrc<uint64_t*>(Op->Header.Args[1]);
                uint64_t Previous = Data->fetch_and(Src);
                GD = Previous;
                break;
              }
              default:  LogMan::Msg::A("Unhandled Atomic size: %d", Op->Size);
            }
            break;
          }
          case IR::OP_ATOMICFETCHOR: {
            auto Op = IROp->C<IR::IROp_AtomicFetchOr>();
            switch (Op->Size) {
              case 1: {
                std::atomic<uint8_t> *Data = {};
                if (Thread->CTX->Config.UnifiedMemory) {
                  Data = *GetSrc<std::atomic<uint8_t> **>(Op->Header.Args[0]);
                }
                else {
                  Data = Thread->CTX->MemoryMapper.GetPointer<std::atomic<uint8_t> *>(*GetSrc<uint64_t*>(Op->Header.Args[0]));
                  LogMan::Throw::A(Data != nullptr, "Couldn't Map pointer to 0x%lx\n", *GetSrc<uint64_t*>(Op->Header.Args[0]));
                }
                uint8_t Src = *GetSrc<uint8_t*>(Op->Header.Args[1]);
                uint8_t Previous = Data->fetch_or(Src);
                GD = Previous;
                break;
              }
              case 2: {
                std::atomic<uint16_t> *Data = {};
                if (Thread->CTX->Config.UnifiedMemory) {
                  Data = *GetSrc<std::atomic<uint16_t> **>(Op->Header.Args[0]);
                }
                else {
                  Data = Thread->CTX->MemoryMapper.GetPointer<std::atomic<uint16_t> *>(*GetSrc<uint64_t*>(Op->Header.Args[0]));
                  LogMan::Throw::A(Data != nullptr, "Couldn't Map pointer to 0x%lx\n", *GetSrc<uint64_t*>(Op->Header.Args[0]));
                }
                uint16_t Src = *GetSrc<uint16_t*>(Op->Header.Args[1]);
                uint16_t Previous = Data->fetch_or(Src);
                GD = Previous;
                break;
              }
              case 4: {
                std::atomic<uint32_t> *Data = {};
                if (Thread->CTX->Config.UnifiedMemory) {
                  Data = *GetSrc<std::atomic<uint32_t> **>(Op->Header.Args[0]);
                }
                else {
                  Data = Thread->CTX->MemoryMapper.GetPointer<std::atomic<uint32_t> *>(*GetSrc<uint64_t*>(Op->Header.Args[0]));
                  LogMan::Throw::A(Data != nullptr, "Couldn't Map pointer to 0x%lx\n", *GetSrc<uint64_t*>(Op->Header.Args[0]));
                }
                uint32_t Src = *GetSrc<uint32_t*>(Op->Header.Args[1]);
                uint32_t Previous = Data->fetch_or(Src);
                GD = Previous;
                break;
              }
              case 8: {
                std::atomic<uint64_t> *Data = {};
                if (Thread->CTX->Config.UnifiedMemory) {
                  Data = *GetSrc<std::atomic<uint64_t> **>(Op->Header.Args[0]);
                }
                else {
                  Data = Thread->CTX->MemoryMapper.GetPointer<std::atomic<uint64_t> *>(*GetSrc<uint64_t*>(Op->Header.Args[0]));
                  LogMan::Throw::A(Data != nullptr, "Couldn't Map pointer to 0x%lx\n", *GetSrc<uint64_t*>(Op->Header.Args[0]));
                }
                uint64_t Src = *GetSrc<uint64_t*>(Op->Header.Args[1]);
                uint64_t Previous = Data->fetch_or(Src);
                GD = Previous;
                break;
              }
              default:  LogMan::Msg::A("Unhandled Atomic size: %d", Op->Size);
            }
            break;
          }
          case IR::OP_ATOMICFETCHXOR: {
            auto Op = IROp->C<IR::IROp_AtomicFetchXor>();
            switch (Op->Size) {
              case 1: {
                std::atomic<uint8_t> *Data = {};
                if (Thread->CTX->Config.UnifiedMemory) {
                  Data = *GetSrc<std::atomic<uint8_t> **>(Op->Header.Args[0]);
                }
                else {
                  Data = Thread->CTX->MemoryMapper.GetPointer<std::atomic<uint8_t> *>(*GetSrc<uint64_t*>(Op->Header.Args[0]));
                  LogMan::Throw::A(Data != nullptr, "Couldn't Map pointer to 0x%lx\n", *GetSrc<uint64_t*>(Op->Header.Args[0]));
                }
                uint8_t Src = *GetSrc<uint8_t*>(Op->Header.Args[1]);
                uint8_t Previous = Data->fetch_xor(Src);
                GD = Previous;
                break;
              }
              case 2: {
                std::atomic<uint16_t> *Data = {};
                if (Thread->CTX->Config.UnifiedMemory) {
                  Data = *GetSrc<std::atomic<uint16_t> **>(Op->Header.Args[0]);
                }
                else {
                  Data = Thread->CTX->MemoryMapper.GetPointer<std::atomic<uint16_t> *>(*GetSrc<uint64_t*>(Op->Header.Args[0]));
                  LogMan::Throw::A(Data != nullptr, "Couldn't Map pointer to 0x%lx\n", *GetSrc<uint64_t*>(Op->Header.Args[0]));
                }
                uint16_t Src = *GetSrc<uint16_t*>(Op->Header.Args[1]);
                uint16_t Previous = Data->fetch_xor(Src);
                GD = Previous;
                break;
              }
              case 4: {
                std::atomic<uint32_t> *Data = {};
                if (Thread->CTX->Config.UnifiedMemory) {
                  Data = *GetSrc<std::atomic<uint32_t> **>(Op->Header.Args[0]);
                }
                else {
                  Data = Thread->CTX->MemoryMapper.GetPointer<std::atomic<uint32_t> *>(*GetSrc<uint64_t*>(Op->Header.Args[0]));
                  LogMan::Throw::A(Data != nullptr, "Couldn't Map pointer to 0x%lx\n", *GetSrc<uint64_t*>(Op->Header.Args[0]));
                }
                uint32_t Src = *GetSrc<uint32_t*>(Op->Header.Args[1]);
                uint32_t Previous = Data->fetch_xor(Src);
                GD = Previous;
                break;
              }
              case 8: {
                std::atomic<uint64_t> *Data = {};
                if (Thread->CTX->Config.UnifiedMemory) {
                  Data = *GetSrc<std::atomic<uint64_t> **>(Op->Header.Args[0]);
                }
                else {
                  Data = Thread->CTX->MemoryMapper.GetPointer<std::atomic<uint64_t> *>(*GetSrc<uint64_t*>(Op->Header.Args[0]));
                  LogMan::Throw::A(Data != nullptr, "Couldn't Map pointer to 0x%lx\n", *GetSrc<uint64_t*>(Op->Header.Args[0]));
                }
                uint64_t Src = *GetSrc<uint64_t*>(Op->Header.Args[1]);
                uint64_t Previous = Data->fetch_xor(Src);
                GD = Previous;
                break;
              }
              default:  LogMan::Msg::A("Unhandled Atomic size: %d", Op->Size);
            }
            break;
          }
          // Vector ops
          case IR::OP_CREATEVECTOR2: {
            auto Op = IROp->C<IR::IROp_CreateVector2>();
            LogMan::Throw::A(OpSize <= 16, "Can't handle a vector of size: %d", OpSize);
            void *Src1 = GetSrc<void*>(Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(Op->Header.Args[1]);
            uint8_t Tmp[16];
            uint8_t ElementSize = OpSize / 2;
            #define CREATE_VECTOR(elementsize, type) \
              case elementsize: { \
                auto *Dst_d = reinterpret_cast<type*>(Tmp); \
                auto *Src1_d = reinterpret_cast<type*>(Src1); \
                auto *Src2_d = reinterpret_cast<type*>(Src2); \
                Dst_d[0] = *Src1_d; \
                Dst_d[1] = *Src2_d; \
                break; \
              }
            switch (ElementSize) {
              CREATE_VECTOR(1, uint8_t)
              CREATE_VECTOR(2, uint16_t)
              CREATE_VECTOR(4, uint32_t)
              CREATE_VECTOR(8, uint64_t)
              default: LogMan::Msg::A("Unknown Element Size: %d", ElementSize); break;
            }
            #undef CREATE_VECTOR
            memcpy(GDP, Tmp, OpSize);

            break;
          }
          case IR::OP_SPLATVECTOR4:
          case IR::OP_SPLATVECTOR3:
          case IR::OP_SPLATVECTOR2: {
            auto Op = IROp->C<IR::IROp_SplatVector2>();
            LogMan::Throw::A(OpSize <= 16, "Can't handle a vector of size: %d", OpSize);
            void *Src = GetSrc<void*>(Op->Header.Args[0]);
            uint8_t Tmp[16];
            uint8_t Elements = 0;

            switch (Op->Header.Op) {
              case IR::OP_SPLATVECTOR4: Elements = 4; break;
              case IR::OP_SPLATVECTOR3: Elements = 3; break;
              case IR::OP_SPLATVECTOR2: Elements = 2; break;
              default: LogMan::Msg::A("Uknown Splat size"); break;
            }

            #define CREATE_VECTOR(elementsize, type) \
              case elementsize: { \
                auto *Dst_d = reinterpret_cast<type*>(Tmp); \
                auto *Src_d = reinterpret_cast<type*>(Src); \
                for (uint8_t i = 0; i < Elements; ++i) \
                  Dst_d[i] = *Src_d;\
                break; \
              }
            uint8_t ElementSize = OpSize / Elements;
            switch (ElementSize) {
              CREATE_VECTOR(1, uint8_t)
              CREATE_VECTOR(2, uint16_t)
              CREATE_VECTOR(4, uint32_t)
              CREATE_VECTOR(8, uint64_t)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.Size); break;
            }
            #undef CREATE_VECTOR
            memcpy(GDP, Tmp, OpSize);

            break;
          }
          case IR::OP_VOR: {
            auto Op = IROp->C<IR::IROp_VOr>();
            __uint128_t Src1 = *GetSrc<__uint128_t*>(Op->Header.Args[0]);
            __uint128_t Src2 = *GetSrc<__uint128_t*>(Op->Header.Args[1]);

            __uint128_t Dst = Src1 | Src2;
            memcpy(GDP, &Dst, 16);
            break;
          }
          case IR::OP_VAND: {
            auto Op = IROp->C<IR::IROp_VAnd>();
            __uint128_t Src1 = *GetSrc<__uint128_t*>(Op->Header.Args[0]);
            __uint128_t Src2 = *GetSrc<__uint128_t*>(Op->Header.Args[1]);

            __uint128_t Dst = Src1 & Src2;
            memcpy(GDP, &Dst, 16);
            break;
          }
          case IR::OP_VXOR: {
            auto Op = IROp->C<IR::IROp_VXor>();
            __uint128_t Src1 = *GetSrc<__uint128_t*>(Op->Header.Args[0]);
            __uint128_t Src2 = *GetSrc<__uint128_t*>(Op->Header.Args[1]);

            __uint128_t Dst = Src1 ^ Src2;
            memcpy(GDP, &Dst, 16);
            break;
          }
          case IR::OP_VSLI: {
            auto Op = IROp->C<IR::IROp_VSLI>();
            __uint128_t Src1 = *GetSrc<__uint128_t*>(Op->Header.Args[0]);
            __uint128_t Src2 = Op->ByteShift * 8;

            __uint128_t Dst = Op->ByteShift >= sizeof(__uint128_t) ? 0 : Src1 << Src2;
            memcpy(GDP, &Dst, 16);
            break;
          }
          case IR::OP_VSRI: {
            auto Op = IROp->C<IR::IROp_VSRI>();
            __uint128_t Src1 = *GetSrc<__uint128_t*>(Op->Header.Args[0]);
            __uint128_t Src2 = Op->ByteShift * 8;

            __uint128_t Dst = Op->ByteShift >= sizeof(__uint128_t) ? 0 : Src1 >> Src2;
            memcpy(GDP, &Dst, 16);
            break;
          }
          case IR::OP_VNOT: {
            auto Op = IROp->C<IR::IROp_VNot>();
            __uint128_t Src1 = *GetSrc<__uint128_t*>(Op->Header.Args[0]);

            __uint128_t Dst = ~Src1;
            memcpy(GDP, &Dst, 16);
            break;
          }
          #define DO_VECTOR_OP(size, type, func)              \
            case size: {                                      \
            auto *Dst_d  = reinterpret_cast<type*>(Tmp);  \
            auto *Src1_d = reinterpret_cast<type*>(Src1); \
            auto *Src2_d = reinterpret_cast<type*>(Src2); \
            for (uint8_t i = 0; i < Elements; ++i) {          \
              Dst_d[i] = func(Src1_d[i], Src2_d[i]);          \
            }                                                 \
            break;                                            \
            }
          #define DO_VECTOR_PAIR_OP(size, type, func)              \
            case size: {                                      \
            auto *Dst_d  = reinterpret_cast<type*>(Tmp);  \
            auto *Src1_d = reinterpret_cast<type*>(Src1); \
            auto *Src2_d = reinterpret_cast<type*>(Src2); \
            for (uint8_t i = 0; i < Elements; ++i) {          \
              Dst_d[i] = func(Src1_d[i*2], Src1_d[i*2 + 1]);          \
              Dst_d[i+Elements] = func(Src2_d[i*2], Src2_d[i*2 + 1]);          \
            }                                                 \
            break;                                            \
            }
          #define DO_VECTOR_SCALAR_OP(size, type, func)\
            case size: {                                      \
            auto *Dst_d  = reinterpret_cast<type*>(Tmp);  \
            auto *Src1_d = reinterpret_cast<type*>(Src1); \
            auto *Src2_d = reinterpret_cast<type*>(Src2); \
            for (uint8_t i = 0; i < Elements; ++i) {          \
              Dst_d[i] = func(Src1_d[i], *Src2_d);          \
            }                                                 \
            break;                                            \
            }
          #define DO_VECTOR_1SRC_OP(size, type, func)              \
            case size: {                                      \
            auto *Dst_d  = reinterpret_cast<type*>(Tmp);  \
            auto *Src_d = reinterpret_cast<type*>(Src); \
            for (uint8_t i = 0; i < Elements; ++i) {          \
              Dst_d[i] = func(Src_d[i]);          \
            }                                                 \
            break;                                            \
            }
          #define DO_VECTOR_SAT_OP(size, type, func, min, max)              \
            case size: {                                      \
            auto *Dst_d  = reinterpret_cast<type*>(Tmp);  \
            auto *Src1_d = reinterpret_cast<type*>(Src1); \
            auto *Src2_d = reinterpret_cast<type*>(Src2); \
            for (uint8_t i = 0; i < Elements; ++i) {          \
              Dst_d[i] = func(Src1_d[i], Src2_d[i], min, max);          \
            }                                                 \
            break;                                            \
            }

          case IR::OP_VUSHRI: {
            auto Op = IROp->C<IR::IROp_VUShrI>();
            void *Src = GetSrc<void*>(Op->Header.Args[0]);
            uint8_t BitShift = Op->BitShift;
            uint8_t Tmp[16];

            uint8_t Elements = Op->RegisterSize / Op->ElementSize;

            auto Func = [BitShift](auto a) { return BitShift >= (sizeof(a) * 8) ? 0 : a >> BitShift; };
            switch (Op->ElementSize) {
              DO_VECTOR_1SRC_OP(1, uint8_t, Func)
              DO_VECTOR_1SRC_OP(2, uint16_t, Func)
              DO_VECTOR_1SRC_OP(4, uint32_t, Func)
              DO_VECTOR_1SRC_OP(8, uint64_t, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
            }
            memcpy(GDP, Tmp, Op->RegisterSize);
            break;
          }
          case IR::OP_VSSHRI: {
            auto Op = IROp->C<IR::IROp_VSShrI>();
            void *Src = GetSrc<void*>(Op->Header.Args[0]);
            uint8_t BitShift = Op->BitShift;
            uint8_t Tmp[16];

            uint8_t Elements = Op->RegisterSize / Op->ElementSize;

            auto Func = [BitShift](auto a) { return BitShift >= (sizeof(a) * 8) ? (a >> (sizeof(a) * 8 - 1)) : a >> BitShift; };
            switch (Op->ElementSize) {
              DO_VECTOR_1SRC_OP(1, int8_t, Func)
              DO_VECTOR_1SRC_OP(2, int16_t, Func)
              DO_VECTOR_1SRC_OP(4, int32_t, Func)
              DO_VECTOR_1SRC_OP(8, int64_t, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
            }
            memcpy(GDP, Tmp, Op->RegisterSize);
            break;
          }
          case IR::OP_VSHLI: {
            auto Op = IROp->C<IR::IROp_VShlI>();
            void *Src = GetSrc<void*>(Op->Header.Args[0]);
            uint8_t BitShift = Op->BitShift;
            uint8_t Tmp[16];

            uint8_t Elements = Op->RegisterSize / Op->ElementSize;

            auto Func = [BitShift](auto a) { return BitShift >= (sizeof(a) * 8) ? 0 : (a << BitShift); };
            switch (Op->ElementSize) {
              DO_VECTOR_1SRC_OP(1, uint8_t, Func)
              DO_VECTOR_1SRC_OP(2, uint16_t, Func)
              DO_VECTOR_1SRC_OP(4, uint32_t, Func)
              DO_VECTOR_1SRC_OP(8, uint64_t, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
            }
            memcpy(GDP, Tmp, Op->RegisterSize);
            break;
          }

          case IR::OP_VADD: {
            auto Op = IROp->C<IR::IROp_VAdd>();
            void *Src1 = GetSrc<void*>(Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(Op->Header.Args[1]);
            uint8_t Tmp[16];

            uint8_t Elements = Op->RegisterSize / Op->ElementSize;

            auto Func = [](auto a, auto b) { return a + b; };
            switch (Op->ElementSize) {
              DO_VECTOR_OP(1, uint8_t,  Func)
              DO_VECTOR_OP(2, uint16_t, Func)
              DO_VECTOR_OP(4, uint32_t, Func)
              DO_VECTOR_OP(8, uint64_t, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
            }
            memcpy(GDP, Tmp, Op->RegisterSize);
            break;
          }
          case IR::OP_VSUB: {
            auto Op = IROp->C<IR::IROp_VSub>();
            void *Src1 = GetSrc<void*>(Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(Op->Header.Args[1]);
            uint8_t Tmp[16];

            uint8_t Elements = Op->RegisterSize / Op->ElementSize;

            auto Func = [](auto a, auto b) { return a - b; };
            switch (Op->ElementSize) {
              DO_VECTOR_OP(1, uint8_t,  Func)
              DO_VECTOR_OP(2, uint16_t, Func)
              DO_VECTOR_OP(4, uint32_t, Func)
              DO_VECTOR_OP(8, uint64_t, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
            }
            memcpy(GDP, Tmp, Op->RegisterSize);
            break;
          }
          case IR::OP_VUQADD: {
            auto Op = IROp->C<IR::IROp_VUQAdd>();
            void *Src1 = GetSrc<void*>(Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(Op->Header.Args[1]);
            uint8_t Tmp[16];

            uint8_t Elements = Op->RegisterSize / Op->ElementSize;

            auto Func = [](auto a, auto b) {
              decltype(a) res = a + b;
              return res < a ? ~0U : res;
            };
            switch (Op->ElementSize) {
              DO_VECTOR_OP(1, uint8_t,  Func)
              DO_VECTOR_OP(2, uint16_t, Func)
              DO_VECTOR_OP(4, uint32_t, Func)
              DO_VECTOR_OP(8, uint64_t, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
            }
            memcpy(GDP, Tmp, Op->RegisterSize);
            break;
          }
          case IR::OP_VUQSUB: {
            auto Op = IROp->C<IR::IROp_VUQSub>();
            void *Src1 = GetSrc<void*>(Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(Op->Header.Args[1]);
            uint8_t Tmp[16];

            uint8_t Elements = Op->RegisterSize / Op->ElementSize;

            auto Func = [](auto a, auto b) {
              decltype(a) res = a - b;
              return res > a ? 0U : res;
            };
            switch (Op->ElementSize) {
              DO_VECTOR_OP(1, uint8_t,  Func)
              DO_VECTOR_OP(2, uint16_t, Func)
              DO_VECTOR_OP(4, uint32_t, Func)
              DO_VECTOR_OP(8, uint64_t, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
            }
            memcpy(GDP, Tmp, Op->RegisterSize);
            break;
          }
          case IR::OP_VSQADD: {
            auto Op = IROp->C<IR::IROp_VSQAdd>();
            void *Src1 = GetSrc<void*>(Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(Op->Header.Args[1]);
            uint8_t Tmp[16];

            uint8_t Elements = Op->RegisterSize / Op->ElementSize;

            auto Func = [](auto a, auto b) {
              decltype(a) res = a + b;

              if (a > 0) {
                if (b > (std::numeric_limits<decltype(a)>::max() - a)) {
                  return std::numeric_limits<decltype(a)>::max();
                }
              }
              else if (b < (std::numeric_limits<decltype(a)>::min() - a)) {
                return std::numeric_limits<decltype(a)>::min();
              }

              return res;
            };
            switch (Op->ElementSize) {
              DO_VECTOR_OP(1, int8_t,  Func)
              DO_VECTOR_OP(2, int16_t, Func)
              DO_VECTOR_OP(4, int32_t, Func)
              DO_VECTOR_OP(8, int64_t, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
            }
            memcpy(GDP, Tmp, Op->RegisterSize);
            break;
          }
          case IR::OP_VSQSUB: {
            auto Op = IROp->C<IR::IROp_VSQSub>();
            void *Src1 = GetSrc<void*>(Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(Op->Header.Args[1]);
            uint8_t Tmp[16];

            uint8_t Elements = Op->RegisterSize / Op->ElementSize;

            auto Func = [](auto a, auto b) {
              __int128_t res = a - b;
              if (res < std::numeric_limits<decltype(a)>::min())
                return std::numeric_limits<decltype(a)>::min();

              if (res > std::numeric_limits<decltype(a)>::max())
                return std::numeric_limits<decltype(a)>::max();
              return (decltype(a))res;
            };

            switch (Op->ElementSize) {
              DO_VECTOR_OP(1, int8_t,  Func)
              DO_VECTOR_OP(2, int16_t, Func)
              DO_VECTOR_OP(4, int32_t, Func)
              DO_VECTOR_OP(8, int64_t, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
            }
            memcpy(GDP, Tmp, Op->RegisterSize);
            break;
          }

          case IR::OP_VFADD: {
            auto Op = IROp->C<IR::IROp_VFAdd>();
            void *Src1 = GetSrc<void*>(Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(Op->Header.Args[1]);
            uint8_t Tmp[16];

            uint8_t Elements = Op->RegisterSize / Op->ElementSize;

            auto Func = [](auto a, auto b) { return a + b; };
            switch (Op->ElementSize) {
              DO_VECTOR_OP(4, float, Func)
              DO_VECTOR_OP(8, double, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
            }
            memcpy(GDP, Tmp, Op->RegisterSize);
            break;
          }
          case IR::OP_VFSUB: {
            auto Op = IROp->C<IR::IROp_VFSub>();
            void *Src1 = GetSrc<void*>(Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(Op->Header.Args[1]);
            uint8_t Tmp[16];

            uint8_t Elements = Op->RegisterSize / Op->ElementSize;

            auto Func = [](auto a, auto b) { return a - b; };
            switch (Op->ElementSize) {
              DO_VECTOR_OP(4, float, Func)
              DO_VECTOR_OP(8, double, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
            }
            memcpy(GDP, Tmp, Op->RegisterSize);
            break;
          }
          case IR::OP_VADDP: {
            auto Op = IROp->C<IR::IROp_VAddP>();
            void *Src1 = GetSrc<void*>(Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(Op->Header.Args[1]);
            uint8_t Tmp[16];

            uint8_t Elements = Op->RegisterSize / Op->ElementSize;

            auto Func = [](auto a, auto b) { return a + b; };
            switch (Op->ElementSize) {
              DO_VECTOR_PAIR_OP(1, uint8_t,  Func)
              DO_VECTOR_PAIR_OP(2, uint16_t, Func)
              DO_VECTOR_PAIR_OP(4, uint32_t, Func)
              DO_VECTOR_PAIR_OP(8, uint64_t, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
            }
            memcpy(GDP, Tmp, Op->RegisterSize);
            break;
          }
          case IR::OP_VFMUL: {
            auto Op = IROp->C<IR::IROp_VFMul>();
            void *Src1 = GetSrc<void*>(Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(Op->Header.Args[1]);
            uint8_t Tmp[16];

            uint8_t Elements = Op->RegisterSize / Op->ElementSize;

            auto Func = [](auto a, auto b) { return a * b; };
            switch (Op->ElementSize) {
              DO_VECTOR_OP(4, float, Func)
              DO_VECTOR_OP(8, double, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
            }
            memcpy(GDP, Tmp, Op->RegisterSize);
            break;
          }
          case IR::OP_VFDIV: {
            auto Op = IROp->C<IR::IROp_VFDiv>();
            void *Src1 = GetSrc<void*>(Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(Op->Header.Args[1]);
            uint8_t Tmp[16];

            uint8_t Elements = Op->RegisterSize / Op->ElementSize;

            auto Func = [](auto a, auto b) { return a / b; };
            switch (Op->ElementSize) {
              DO_VECTOR_OP(4, float, Func)
              DO_VECTOR_OP(8, double, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
            }
            memcpy(GDP, Tmp, Op->RegisterSize);
            break;
          }
          case IR::OP_VFMIN: {
            auto Op = IROp->C<IR::IROp_VFMin>();
            void *Src1 = GetSrc<void*>(Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(Op->Header.Args[1]);
            uint8_t Tmp[16];

            uint8_t Elements = Op->RegisterSize / Op->ElementSize;

            auto Func = [](auto a, auto b) { return std::min(a, b); };
            switch (Op->ElementSize) {
              DO_VECTOR_OP(4, float, Func)
              DO_VECTOR_OP(8, double, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
            }
            memcpy(GDP, Tmp, Op->RegisterSize);
            break;
          }
          case IR::OP_VFMAX: {
            auto Op = IROp->C<IR::IROp_VFMax>();
            void *Src1 = GetSrc<void*>(Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(Op->Header.Args[1]);
            uint8_t Tmp[16];

            uint8_t Elements = Op->RegisterSize / Op->ElementSize;

            auto Func = [](auto a, auto b) { return std::max(a, b); };
            switch (Op->ElementSize) {
              DO_VECTOR_OP(4, float, Func)
              DO_VECTOR_OP(8, double, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
            }
            memcpy(GDP, Tmp, Op->RegisterSize);
            break;
          }
          case IR::OP_VFRECP: {
            auto Op = IROp->C<IR::IROp_VFRecp>();
            void *Src = GetSrc<void*>(Op->Header.Args[0]);
            uint8_t Tmp[16];

            uint8_t Elements = Op->RegisterSize / Op->ElementSize;

            auto Func = [](auto a) { return 1.0 / a; };
            switch (Op->ElementSize) {
              DO_VECTOR_1SRC_OP(4, float, Func)
              DO_VECTOR_1SRC_OP(8, double, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
            }
            memcpy(GDP, Tmp, Op->RegisterSize);
            break;
          }
          case IR::OP_VFSQRT: {
            auto Op = IROp->C<IR::IROp_VFSqrt>();
            void *Src = GetSrc<void*>(Op->Header.Args[0]);
            uint8_t Tmp[16];

            uint8_t Elements = Op->RegisterSize / Op->ElementSize;

            auto Func = [](auto a) { return std::sqrt(a); };
            switch (Op->ElementSize) {
              DO_VECTOR_1SRC_OP(4, float, Func)
              DO_VECTOR_1SRC_OP(8, double, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
            }
            memcpy(GDP, Tmp, Op->RegisterSize);
            break;
          }
          case IR::OP_VFRSQRT: {
            auto Op = IROp->C<IR::IROp_VFRSqrt>();
            void *Src = GetSrc<void*>(Op->Header.Args[0]);
            uint8_t Tmp[16];

            uint8_t Elements = Op->RegisterSize / Op->ElementSize;

            auto Func = [](auto a) { return 1.0 / std::sqrt(a); };
            switch (Op->ElementSize) {
              DO_VECTOR_1SRC_OP(4, float, Func)
              DO_VECTOR_1SRC_OP(8, double, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
            }
            memcpy(GDP, Tmp, Op->RegisterSize);
            break;
          }
          #define DO_VECTOR_1SRC_2TYPE_OP(size, type, type2, func, min, max)              \
            case size: {                                      \
            auto *Dst_d  = reinterpret_cast<type*>(Tmp);  \
            auto *Src_d = reinterpret_cast<type2*>(Src); \
            for (uint8_t i = 0; i < Elements; ++i) {          \
              Dst_d[i] = (type)func(Src_d[i], min, max);          \
            }                                                 \
            break;                                            \
            }
          #define DO_VECTOR_1SRC_2TYPE_OP_TOP(size, type, type2, func, min, max)              \
            case size: {                                      \
            auto *Dst_d  = reinterpret_cast<type*>(Tmp);  \
            auto *Src_d = reinterpret_cast<type2*>(Src2); \
            memcpy(Dst_d, Src1, Elements * sizeof(type2));\
            for (uint8_t i = 0; i < Elements; ++i) {          \
              Dst_d[i+Elements] = (type)func(Src_d[i], min, max);          \
            }                                                 \
            break;                                            \
            }

          #define DO_VECTOR_1SRC_2TYPE_OP_TOP_SRC(size, type, type2, func, min, max)              \
            case size: {                                      \
            auto *Dst_d  = reinterpret_cast<type*>(Tmp);  \
            auto *Src_d = reinterpret_cast<type2*>(Src); \
            for (uint8_t i = 0; i < Elements; ++i) {          \
              Dst_d[i] = (type)func(Src_d[i+Elements], min, max);          \
            }                                                 \
            break;                                            \
            }
          #define DO_VECTOR_2SRC_2TYPE_OP(size, type, type2, func)              \
            case size: {                                      \
            auto *Dst_d  = reinterpret_cast<type*>(Tmp);  \
            auto *Src1_d = reinterpret_cast<type2*>(Src1); \
            auto *Src2_d = reinterpret_cast<type2*>(Src2); \
            for (uint8_t i = 0; i < Elements; ++i) {          \
              Dst_d[i] = (type)func((type)Src1_d[i], (type)Src2_d[i]);          \
            }                                                 \
            break;                                            \
            }
          #define DO_VECTOR_2SRC_2TYPE_OP_TOP_SRC(size, type, type2, func)              \
            case size: {                                      \
            auto *Dst_d  = reinterpret_cast<type*>(Tmp);  \
            auto *Src1_d = reinterpret_cast<type2*>(Src1); \
            auto *Src2_d = reinterpret_cast<type2*>(Src2); \
            for (uint8_t i = 0; i < Elements; ++i) {          \
              Dst_d[i] = (type)func((type)Src1_d[i+Elements], (type)Src2_d[i+Elements]);          \
            }                                                 \
            break;                                            \
            }

          case IR::OP_VUSHRNI: {
            auto Op = IROp->C<IR::IROp_VUShrNI>();
            void *Src = GetSrc<void*>(Op->Header.Args[0]);
            uint8_t BitShift = Op->BitShift;
            uint8_t Tmp[16]{};

            uint8_t Elements = Op->RegisterSize / Op->ElementSize;

            auto Func = [BitShift](auto a, auto min, auto max) { return BitShift >= (sizeof(a) * 8) ? 0 : a >> BitShift; };
            switch (Op->ElementSize) {
              DO_VECTOR_1SRC_2TYPE_OP(2, uint8_t, uint16_t, Func, 0, 0)
              DO_VECTOR_1SRC_2TYPE_OP(4, uint16_t, uint32_t, Func, 0, 0)
              DO_VECTOR_1SRC_2TYPE_OP(8, uint32_t, uint64_t, Func, 0, 0)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
            }
            memcpy(GDP, Tmp, Op->RegisterSize);
            break;
          }
          case IR::OP_VUSHRNI2: {
            auto Op = IROp->C<IR::IROp_VUShrNI2>();
            void *Src1 = GetSrc<void*>(Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(Op->Header.Args[1]);
            uint8_t BitShift = Op->BitShift;
            uint8_t Tmp[16];

            uint8_t Elements = Op->RegisterSize / Op->ElementSize;

            auto Func = [BitShift](auto a, auto min, auto max) { return BitShift >= (sizeof(a) * 8) ? 0 : a >> BitShift; };
            switch (Op->ElementSize) {
              DO_VECTOR_1SRC_2TYPE_OP_TOP(2, uint8_t, uint16_t, Func, 0, 0)
              DO_VECTOR_1SRC_2TYPE_OP_TOP(4, uint16_t, uint32_t, Func, 0, 0)
              DO_VECTOR_1SRC_2TYPE_OP_TOP(8, uint32_t, uint64_t, Func, 0, 0)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
            }
            memcpy(GDP, Tmp, Op->RegisterSize);
            break;
          }
          case IR::OP_VSQXTN: {
            auto Op = IROp->C<IR::IROp_VSQXTN>();
            void *Src = GetSrc<void*>(Op->Header.Args[0]);
            uint8_t Tmp[16]{};

            uint8_t Elements = Op->RegisterSize / Op->ElementSize;

            auto Func = [](auto a, auto min, auto max) { return std::max(std::min(a, (decltype(a))max), (decltype(a))min); };
            switch (Op->ElementSize) {
              DO_VECTOR_1SRC_2TYPE_OP(2, int8_t, int16_t, Func, std::numeric_limits<int8_t>::min(), std::numeric_limits<int8_t>::max())
              DO_VECTOR_1SRC_2TYPE_OP(4, int16_t, int32_t, Func, std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::max())
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
            }
            memcpy(GDP, Tmp, Op->RegisterSize);
            break;
          }
          case IR::OP_VSQXTN2: {
            auto Op = IROp->C<IR::IROp_VSQXTN2>();
            void *Src1 = GetSrc<void*>(Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(Op->Header.Args[1]);
            uint8_t Tmp[16]{};

            uint8_t Elements = Op->RegisterSize / Op->ElementSize;

            auto Func = [](auto a, auto min, auto max) { return std::max(std::min(a, (decltype(a))max), (decltype(a))min); };
            switch (Op->ElementSize) {
              DO_VECTOR_1SRC_2TYPE_OP_TOP(2, int8_t, int16_t, Func, std::numeric_limits<int8_t>::min(), std::numeric_limits<int8_t>::max())
              DO_VECTOR_1SRC_2TYPE_OP_TOP(4, int16_t, int32_t, Func, std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::max())
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
            }
            memcpy(GDP, Tmp, Op->RegisterSize);
            break;
          }
          case IR::OP_VSQXTUN: {
            auto Op = IROp->C<IR::IROp_VSQXTUN>();
            void *Src = GetSrc<void*>(Op->Header.Args[0]);
            uint8_t Tmp[16]{};

            uint8_t Elements = Op->RegisterSize / Op->ElementSize;

            auto Func = [](auto a, auto min, auto max) { return std::max(std::min(a, (decltype(a))max), (decltype(a))min); };
            switch (Op->ElementSize) {
              DO_VECTOR_1SRC_2TYPE_OP(2, uint8_t, int16_t, Func, 0, (1 << 8) - 1)
              DO_VECTOR_1SRC_2TYPE_OP(4, uint16_t, int32_t, Func, 0, (1 << 16) - 1)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
            }
            memcpy(GDP, Tmp, Op->RegisterSize);
            break;
          }
          case IR::OP_VSQXTUN2: {
            auto Op = IROp->C<IR::IROp_VSQXTUN2>();
            void *Src1 = GetSrc<void*>(Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(Op->Header.Args[1]);
            uint8_t Tmp[16]{};

            uint8_t Elements = Op->RegisterSize / Op->ElementSize;

            auto Func = [](auto a, auto min, auto max) { return std::max(std::min(a, (decltype(a))max), (decltype(a))min); };
            switch (Op->ElementSize) {
              DO_VECTOR_1SRC_2TYPE_OP_TOP(2, uint8_t, int16_t, Func, 0, (1 << 8) - 1)
              DO_VECTOR_1SRC_2TYPE_OP_TOP(4, uint16_t, int32_t, Func, 0, (1 << 16) - 1)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
            }
            memcpy(GDP, Tmp, Op->RegisterSize);
            break;
          }
          case IR::OP_VECTOR_UTOF: {
            auto Op = IROp->C<IR::IROp_Vector_UToF>();
            void *Src = GetSrc<void*>(Op->Header.Args[0]);
            uint8_t Tmp[16];

            uint8_t Elements = Op->RegisterSize / Op->ElementSize;

            auto Func = [](auto a, auto min, auto max) { return a; };
            switch (Op->ElementSize) {
              DO_VECTOR_1SRC_2TYPE_OP(4, float, uint32_t, Func, 0, 0)
              DO_VECTOR_1SRC_2TYPE_OP(8, double, uint64_t, Func, 0, 0)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
            }
            memcpy(GDP, Tmp, Op->RegisterSize);
            break;
          }
          case IR::OP_VECTOR_STOF: {
            auto Op = IROp->C<IR::IROp_Vector_SToF>();
            void *Src = GetSrc<void*>(Op->Header.Args[0]);
            uint8_t Tmp[16];

            uint8_t Elements = Op->RegisterSize / Op->ElementSize;

            auto Func = [](auto a, auto min, auto max) { return a; };
            switch (Op->ElementSize) {
              DO_VECTOR_1SRC_2TYPE_OP(4, float, int32_t, Func, 0, 0)
              DO_VECTOR_1SRC_2TYPE_OP(8, double, int64_t, Func, 0, 0)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
            }
            memcpy(GDP, Tmp, Op->RegisterSize);
            break;
          }
          case IR::OP_VECTOR_FTOZU: {
            auto Op = IROp->C<IR::IROp_Vector_FToZU>();
            void *Src = GetSrc<void*>(Op->Header.Args[0]);
            uint8_t Tmp[16];

            uint8_t Elements = Op->RegisterSize / Op->ElementSize;

            auto Func = [](auto a, auto min, auto max) { return a; };
            switch (Op->ElementSize) {
              DO_VECTOR_1SRC_2TYPE_OP(4, uint32_t, float, Func, 0, 0)
              DO_VECTOR_1SRC_2TYPE_OP(8, uint64_t, double, Func, 0, 0)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
            }
            memcpy(GDP, Tmp, Op->RegisterSize);
            break;
          }
          case IR::OP_VECTOR_FTOZS: {
            auto Op = IROp->C<IR::IROp_Vector_FToZS>();
            void *Src = GetSrc<void*>(Op->Header.Args[0]);
            uint8_t Tmp[16];

            uint8_t Elements = Op->RegisterSize / Op->ElementSize;

            auto Func = [](auto a, auto min, auto max) { return a; };
            switch (Op->ElementSize) {
              DO_VECTOR_1SRC_2TYPE_OP(4, int32_t, float, Func, 0, 0)
              DO_VECTOR_1SRC_2TYPE_OP(8, int64_t, double, Func, 0, 0)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
            }
            memcpy(GDP, Tmp, Op->RegisterSize);
            break;
          }
          case IR::OP_VUMUL: {
            auto Op = IROp->C<IR::IROp_VUMul>();
            void *Src1 = GetSrc<void*>(Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(Op->Header.Args[1]);
            uint8_t Tmp[16];

            uint8_t Elements = Op->RegisterSize / Op->ElementSize;

            auto Func = [](auto a, auto b) { return a * b; };
            switch (Op->ElementSize) {
              DO_VECTOR_OP(1, uint8_t,  Func)
              DO_VECTOR_OP(2, uint16_t, Func)
              DO_VECTOR_OP(4, uint32_t, Func)
              DO_VECTOR_OP(8, uint64_t, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
            }
            memcpy(GDP, Tmp, Op->RegisterSize);
            break;
          }
          case IR::OP_VSMUL: {
            auto Op = IROp->C<IR::IROp_VSMul>();
            void *Src1 = GetSrc<void*>(Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(Op->Header.Args[1]);
            uint8_t Tmp[16];

            uint8_t Elements = Op->RegisterSize / Op->ElementSize;

            auto Func = [](auto a, auto b) { return a * b; };
            switch (Op->ElementSize) {
              DO_VECTOR_OP(1, int8_t,  Func)
              DO_VECTOR_OP(2, int16_t, Func)
              DO_VECTOR_OP(4, int32_t, Func)
              DO_VECTOR_OP(8, int64_t, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
            }
            memcpy(GDP, Tmp, Op->RegisterSize);
            break;
          }
          case IR::OP_VUMULL: {
            auto Op = IROp->C<IR::IROp_VUMull>();
            void *Src1 = GetSrc<void*>(Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(Op->Header.Args[1]);

            uint8_t Tmp[16];

            uint8_t Elements = Op->RegisterSize / (Op->ElementSize << 1);

            auto Func = [](auto a, auto b) { return a * b; };
            switch (Op->ElementSize) {
              DO_VECTOR_2SRC_2TYPE_OP(1, uint16_t, uint8_t, Func)
              DO_VECTOR_2SRC_2TYPE_OP(2, uint32_t, uint16_t, Func)
              DO_VECTOR_2SRC_2TYPE_OP(4, uint64_t, uint32_t, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
            }
            memcpy(GDP, Tmp, Op->RegisterSize);
            break;
          }
          case IR::OP_VSMULL: {
            auto Op = IROp->C<IR::IROp_VSMull>();
            void *Src1 = GetSrc<void*>(Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(Op->Header.Args[1]);

            uint8_t Tmp[16];

            uint8_t Elements = Op->RegisterSize / (Op->ElementSize << 1);

            auto Func = [](auto a, auto b) { return a * b; };
            switch (Op->ElementSize) {
              DO_VECTOR_2SRC_2TYPE_OP(1, int16_t, int8_t, Func)
              DO_VECTOR_2SRC_2TYPE_OP(2, int32_t, int16_t, Func)
              DO_VECTOR_2SRC_2TYPE_OP(4, int64_t, int32_t, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
            }
            memcpy(GDP, Tmp, Op->RegisterSize);
            break;
          }
          case IR::OP_VUMULL2: {
            auto Op = IROp->C<IR::IROp_VUMull2>();
            void *Src1 = GetSrc<void*>(Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(Op->Header.Args[1]);

            uint8_t Tmp[16];

            uint8_t Elements = Op->RegisterSize / (Op->ElementSize << 1);

            auto Func = [](auto a, auto b) { return a * b; };
            switch (Op->ElementSize) {
              DO_VECTOR_2SRC_2TYPE_OP_TOP_SRC(1, uint16_t, uint8_t, Func)
              DO_VECTOR_2SRC_2TYPE_OP_TOP_SRC(2, uint32_t, uint16_t, Func)
              DO_VECTOR_2SRC_2TYPE_OP_TOP_SRC(4, uint64_t, uint32_t, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
            }
            memcpy(GDP, Tmp, Op->RegisterSize);
            break;
          }
          case IR::OP_VSMULL2: {
            auto Op = IROp->C<IR::IROp_VSMull2>();
            void *Src1 = GetSrc<void*>(Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(Op->Header.Args[1]);

            uint8_t Tmp[16];

            uint8_t Elements = Op->RegisterSize / (Op->ElementSize << 1);

            auto Func = [](auto a, auto b) { return a * b; };
            switch (Op->ElementSize) {
              DO_VECTOR_2SRC_2TYPE_OP_TOP_SRC(1, int16_t, int8_t, Func)
              DO_VECTOR_2SRC_2TYPE_OP_TOP_SRC(2, int32_t, int16_t, Func)
              DO_VECTOR_2SRC_2TYPE_OP_TOP_SRC(4, int64_t, int32_t, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
            }
            memcpy(GDP, Tmp, Op->RegisterSize);
            break;
          }
          case IR::OP_VSXTL: {
            auto Op = IROp->C<IR::IROp_VSXTL>();
            void *Src = GetSrc<void*>(Op->Header.Args[0]);
            uint8_t Tmp[16]{};

            uint8_t Elements = Op->RegisterSize / Op->ElementSize;

            auto Func = [](auto a, auto min, auto max) { return a; };
            switch (Op->ElementSize) {
              DO_VECTOR_1SRC_2TYPE_OP(1, int16_t, int8_t, Func,  0, 0)
              DO_VECTOR_1SRC_2TYPE_OP(2, int32_t, int16_t, Func, 0, 0)
              DO_VECTOR_1SRC_2TYPE_OP(4, int64_t, int32_t, Func, 0, 0)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
            }
            memcpy(GDP, Tmp, Op->RegisterSize);
            break;
          }
          case IR::OP_VSXTL2: {
            auto Op = IROp->C<IR::IROp_VSXTL2>();
            void *Src = GetSrc<void*>(Op->Header.Args[0]);
            uint8_t Tmp[16];

            uint8_t Elements = Op->RegisterSize / (Op->ElementSize << 1);

            auto Func = [](auto a, auto min, auto max) { return a; };
            switch (Op->ElementSize) {
              DO_VECTOR_1SRC_2TYPE_OP_TOP_SRC(1, int16_t, int8_t, Func,  0, 0)
              DO_VECTOR_1SRC_2TYPE_OP_TOP_SRC(2, int32_t, int16_t, Func, 0, 0)
              DO_VECTOR_1SRC_2TYPE_OP_TOP_SRC(4, int64_t, int32_t, Func, 0, 0)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
            }
            memcpy(GDP, Tmp, Op->RegisterSize);
            break;
          }
          case IR::OP_VUXTL: {
            auto Op = IROp->C<IR::IROp_VUXTL>();
            void *Src = GetSrc<void*>(Op->Header.Args[0]);
            uint8_t Tmp[16]{};

            uint8_t Elements = Op->RegisterSize / Op->ElementSize;

            auto Func = [](auto a, auto min, auto max) { return a; };
            switch (Op->ElementSize) {
              DO_VECTOR_1SRC_2TYPE_OP(1, uint16_t, uint8_t, Func,  0, 0)
              DO_VECTOR_1SRC_2TYPE_OP(2, uint32_t, uint16_t, Func, 0, 0)
              DO_VECTOR_1SRC_2TYPE_OP(4, uint64_t, uint32_t, Func, 0, 0)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
            }
            memcpy(GDP, Tmp, Op->RegisterSize);
            break;
          }
          case IR::OP_VUXTL2: {
            auto Op = IROp->C<IR::IROp_VUXTL2>();
            void *Src = GetSrc<void*>(Op->Header.Args[0]);

            uint8_t Tmp[16];

            uint8_t Elements = Op->RegisterSize / (Op->ElementSize << 1);

            auto Func = [](auto a, auto min, auto max) { return a; };
            switch (Op->ElementSize) {
              DO_VECTOR_1SRC_2TYPE_OP_TOP_SRC(1, uint16_t, uint8_t, Func,  0, 0)
              DO_VECTOR_1SRC_2TYPE_OP_TOP_SRC(2, uint32_t, uint16_t, Func, 0, 0)
              DO_VECTOR_1SRC_2TYPE_OP_TOP_SRC(4, uint64_t, uint32_t, Func, 0, 0)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
            }
            memcpy(GDP, Tmp, Op->RegisterSize);
            break;
          }
          case IR::OP_VUMIN: {
            auto Op = IROp->C<IR::IROp_VUMin>();
            void *Src1 = GetSrc<void*>(Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(Op->Header.Args[1]);
            uint8_t Tmp[16];

            uint8_t Elements = Op->RegisterSize / Op->ElementSize;
            auto Func = [](auto a, auto b) { return std::min(a, b); };

            switch (Op->ElementSize) {
              DO_VECTOR_OP(1, uint8_t,  Func)
              DO_VECTOR_OP(2, uint16_t, Func)
              DO_VECTOR_OP(4, uint32_t, Func)
              DO_VECTOR_OP(8, uint64_t, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
            }
            memcpy(GDP, Tmp, Op->RegisterSize);
            break;
          }
          case IR::OP_VSMIN: {
            auto Op = IROp->C<IR::IROp_VSMin>();
            void *Src1 = GetSrc<void*>(Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(Op->Header.Args[1]);
            uint8_t Tmp[16];

            uint8_t Elements = Op->RegisterSize / Op->ElementSize;
            auto Func = [](auto a, auto b) { return std::min(a, b); };

            switch (Op->ElementSize) {
              DO_VECTOR_OP(1, int8_t,  Func)
              DO_VECTOR_OP(2, int16_t, Func)
              DO_VECTOR_OP(4, int32_t, Func)
              DO_VECTOR_OP(8, int64_t, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
            }
            memcpy(GDP, Tmp, Op->RegisterSize);
            break;
          }
          case IR::OP_VUMAX: {
            auto Op = IROp->C<IR::IROp_VUMax>();
            void *Src1 = GetSrc<void*>(Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(Op->Header.Args[1]);
            uint8_t Tmp[16];

            uint8_t Elements = Op->RegisterSize / Op->ElementSize;
            auto Func = [](auto a, auto b) { return std::max(a, b); };

            switch (Op->ElementSize) {
              DO_VECTOR_OP(1, uint8_t,  Func)
              DO_VECTOR_OP(2, uint16_t, Func)
              DO_VECTOR_OP(4, uint32_t, Func)
              DO_VECTOR_OP(8, uint64_t, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
            }
            memcpy(GDP, Tmp, Op->RegisterSize);
            break;
          }
          case IR::OP_VSMAX: {
            auto Op = IROp->C<IR::IROp_VSMax>();
            void *Src1 = GetSrc<void*>(Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(Op->Header.Args[1]);
            uint8_t Tmp[16];

            uint8_t Elements = Op->RegisterSize / Op->ElementSize;
            auto Func = [](auto a, auto b) { return std::max(a, b); };

            switch (Op->ElementSize) {
              DO_VECTOR_OP(1, int8_t,  Func)
              DO_VECTOR_OP(2, int16_t, Func)
              DO_VECTOR_OP(4, int32_t, Func)
              DO_VECTOR_OP(8, int64_t, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
            }
            memcpy(GDP, Tmp, Op->RegisterSize);
            break;
          }
          case IR::OP_VUSHL: {
            auto Op = IROp->C<IR::IROp_VUShl>();
            void *Src1 = GetSrc<void*>(Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(Op->Header.Args[1]);
            uint8_t Tmp[16];

            uint8_t Elements = Op->RegisterSize / Op->ElementSize;
            auto Func = [](auto a, auto b) { return b >= (sizeof(a) * 8) ? 0 : a << b; };

            switch (Op->ElementSize) {
              DO_VECTOR_OP(1, uint8_t,  Func)
              DO_VECTOR_OP(2, uint16_t, Func)
              DO_VECTOR_OP(4, uint32_t, Func)
              DO_VECTOR_OP(8, uint64_t, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
            }
            memcpy(GDP, Tmp, Op->RegisterSize);
            break;
          }
          case IR::OP_VSSHR: {
            auto Op = IROp->C<IR::IROp_VSShr>();
            void *Src1 = GetSrc<void*>(Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(Op->Header.Args[1]);
            uint8_t Tmp[16];

            uint8_t Elements = Op->RegisterSize / Op->ElementSize;
            auto Func = [](auto a, auto b) { return b >= (sizeof(a) * 8) ? (a >> (sizeof(a) * 8 - 1)) : a >> b; };

            switch (Op->ElementSize) {
              DO_VECTOR_OP(1, int8_t,  Func)
              DO_VECTOR_OP(2, int16_t, Func)
              DO_VECTOR_OP(4, int32_t, Func)
              DO_VECTOR_OP(8, int64_t, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
            }
            memcpy(GDP, Tmp, Op->RegisterSize);
            break;
          }

          case IR::OP_VUSHLS: {
            auto Op = IROp->C<IR::IROp_VUShlS>();
            void *Src1 = GetSrc<void*>(Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(Op->Header.Args[1]);
            uint8_t Tmp[16];

            uint8_t Elements = Op->RegisterSize / Op->ElementSize;
            auto Func = [](auto a, auto b) { return b >= (sizeof(a) * 8) ? 0 : a << b; };

            switch (Op->ElementSize) {
              DO_VECTOR_SCALAR_OP(1, uint8_t, Func)
              DO_VECTOR_SCALAR_OP(2, uint16_t, Func)
              DO_VECTOR_SCALAR_OP(4, uint32_t, Func)
              DO_VECTOR_SCALAR_OP(8, uint64_t, Func)
              DO_VECTOR_SCALAR_OP(16, __uint128_t, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
            }
            memcpy(GDP, Tmp, Op->RegisterSize);
            break;
          }
          case IR::OP_VUSHRS: {
            auto Op = IROp->C<IR::IROp_VUShrS>();
            void *Src1 = GetSrc<void*>(Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(Op->Header.Args[1]);
            uint8_t Tmp[16];

            uint8_t Elements = Op->RegisterSize / Op->ElementSize;
            auto Func = [](auto a, auto b) { return b >= (sizeof(a) * 8) ? 0 : a >> b; };

            switch (Op->ElementSize) {
              DO_VECTOR_SCALAR_OP(1, uint8_t, Func)
              DO_VECTOR_SCALAR_OP(2, uint16_t, Func)
              DO_VECTOR_SCALAR_OP(4, uint32_t, Func)
              DO_VECTOR_SCALAR_OP(8, uint64_t, Func)
              DO_VECTOR_SCALAR_OP(16, __uint128_t, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
            }
            memcpy(GDP, Tmp, Op->RegisterSize);
            break;
          }
          case IR::OP_VSSHRS: {
            auto Op = IROp->C<IR::IROp_VSShrS>();
            void *Src1 = GetSrc<void*>(Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(Op->Header.Args[1]);
            uint8_t Tmp[16];

            uint8_t Elements = Op->RegisterSize / Op->ElementSize;
            auto Func = [](auto a, auto b) { return b >= (sizeof(a) * 8) ? (a >> (sizeof(a) * 8 - 1)) : a >> b; };

            switch (Op->ElementSize) {
              DO_VECTOR_SCALAR_OP(1, int8_t, Func)
              DO_VECTOR_SCALAR_OP(2, int16_t, Func)
              DO_VECTOR_SCALAR_OP(4, int32_t, Func)
              DO_VECTOR_SCALAR_OP(8, int64_t, Func)
              DO_VECTOR_SCALAR_OP(16, __int128_t, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
            }
            memcpy(GDP, Tmp, Op->RegisterSize);
            break;
          }
          case IR::OP_VUSHR: {
            auto Op = IROp->C<IR::IROp_VUShr>();
            void *Src1 = GetSrc<void*>(Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(Op->Header.Args[1]);
            uint8_t Tmp[16];

            uint8_t Elements = Op->RegisterSize / Op->ElementSize;
            auto Func = [](auto a, auto b) { return b >= (sizeof(a) * 8) ? 0 : a >> b; };

            switch (Op->ElementSize) {
              DO_VECTOR_OP(1, uint8_t,  Func)
              DO_VECTOR_OP(2, uint16_t, Func)
              DO_VECTOR_OP(4, uint32_t, Func)
              DO_VECTOR_OP(8, uint64_t, Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
            }
            memcpy(GDP, Tmp, Op->RegisterSize);
            break;
          }
          case IR::OP_VZIP2:
          case IR::OP_VZIP: {
            auto Op = IROp->C<IR::IROp_VZip>();
            void *Src1 = GetSrc<void*>(Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(Op->Header.Args[1]);
            uint8_t Tmp[16];
            uint8_t Elements = Op->RegisterSize / Op->ElementSize;
            uint8_t BaseOffset = IROp->Op == IR::OP_VZIP2 ? (Elements / 2) : 0;
            Elements >>= 1;

            switch (Op->ElementSize) {
              case 1: {
                auto *Dst_d  = reinterpret_cast<uint8_t*>(Tmp);
                auto *Src1_d = reinterpret_cast<uint8_t*>(Src1);
                auto *Src2_d = reinterpret_cast<uint8_t*>(Src2);
                for (unsigned i = 0; i < Elements; ++i) {
                  Dst_d[i*2] = Src1_d[BaseOffset + i];
                  Dst_d[i*2+1] = Src2_d[BaseOffset + i];
                }
                break;
              }
              case 2: {
                auto *Dst_d  = reinterpret_cast<uint16_t*>(Tmp);
                auto *Src1_d = reinterpret_cast<uint16_t*>(Src1);
                auto *Src2_d = reinterpret_cast<uint16_t*>(Src2);
                for (unsigned i = 0; i < Elements; ++i) {
                  Dst_d[i*2] = Src1_d[BaseOffset + i];
                  Dst_d[i*2+1] = Src2_d[BaseOffset + i];
                }
                break;
              }
              case 4: {
                auto *Dst_d  = reinterpret_cast<uint32_t*>(Tmp);
                auto *Src1_d = reinterpret_cast<uint32_t*>(Src1);
                auto *Src2_d = reinterpret_cast<uint32_t*>(Src2);
                for (unsigned i = 0; i < Elements; ++i) {
                  Dst_d[i*2] = Src1_d[BaseOffset + i];
                  Dst_d[i*2+1] = Src2_d[BaseOffset + i];
                }
                break;
              }
              case 8: {
                auto *Dst_d  = reinterpret_cast<uint64_t*>(Tmp);
                auto *Src1_d = reinterpret_cast<uint64_t*>(Src1);
                auto *Src2_d = reinterpret_cast<uint64_t*>(Src2);
                for (unsigned i = 0; i < Elements; ++i) {
                  Dst_d[i*2] = Src1_d[BaseOffset + i];
                  Dst_d[i*2+1] = Src2_d[BaseOffset + i];
                }
                break;
              }
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
            }

            memcpy(GDP, Tmp, Op->RegisterSize);
            break;
          }
          case IR::OP_VINSELEMENT: {
            auto Op = IROp->C<IR::IROp_VInsElement>();
            void *Src1 = GetSrc<void*>(Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(Op->Header.Args[1]);
            uint8_t Tmp[16];

            // Copy src1 in to dest
            memcpy(Tmp, Src1, Op->RegisterSize);
            switch (Op->ElementSize) {
              case 1: {
                auto *Dst_d  = reinterpret_cast<uint8_t*>(Tmp);
                auto *Src2_d = reinterpret_cast<uint8_t*>(Src2);
                Dst_d[Op->DestIdx] = Src2_d[Op->SrcIdx];
                break;
              }
              case 2: {
                auto *Dst_d  = reinterpret_cast<uint16_t*>(Tmp);
                auto *Src2_d = reinterpret_cast<uint16_t*>(Src2);
                Dst_d[Op->DestIdx] = Src2_d[Op->SrcIdx];
                break;
              }
              case 4: {
                auto *Dst_d  = reinterpret_cast<uint32_t*>(Tmp);
                auto *Src2_d = reinterpret_cast<uint32_t*>(Src2);
                Dst_d[Op->DestIdx] = Src2_d[Op->SrcIdx];
                break;
              }
              case 8: {
                auto *Dst_d  = reinterpret_cast<uint64_t*>(Tmp);
                auto *Src2_d = reinterpret_cast<uint64_t*>(Src2);
                Dst_d[Op->DestIdx] = Src2_d[Op->SrcIdx];
                break;
              }
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
            };
            memcpy(GDP, Tmp, Op->RegisterSize);
            break;
          }
          case IR::OP_VINSSCALARELEMENT: {
            auto Op = IROp->C<IR::IROp_VInsScalarElement>();
            void *Src1 = GetSrc<void*>(Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(Op->Header.Args[1]);
            uint8_t Tmp[16];

            // Copy src1 in to dest
            memcpy(Tmp, Src1, Op->RegisterSize);
            switch (Op->ElementSize) {
              case 1: {
                auto *Dst_d  = reinterpret_cast<uint8_t*>(Tmp);
                auto Src2_d = *reinterpret_cast<uint8_t*>(Src2);
                Dst_d[Op->DestIdx] = Src2_d;
                break;
              }
              case 2: {
                auto *Dst_d  = reinterpret_cast<uint16_t*>(Tmp);
                auto Src2_d = *reinterpret_cast<uint16_t*>(Src2);
                Dst_d[Op->DestIdx] = Src2_d;
                break;
              }
              case 4: {
                auto *Dst_d  = reinterpret_cast<uint32_t*>(Tmp);
                auto Src2_d = *reinterpret_cast<uint32_t*>(Src2);
                Dst_d[Op->DestIdx] = Src2_d;
                break;
              }
              case 8: {
                auto *Dst_d  = reinterpret_cast<uint64_t*>(Tmp);
                auto Src2_d = *reinterpret_cast<uint64_t*>(Src2);
                Dst_d[Op->DestIdx] = Src2_d;
                break;
              }
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
            };
            memcpy(GDP, Tmp, Op->RegisterSize);
            break;
          }
          case IR::OP_VCMPEQ: {
            auto Op = IROp->C<IR::IROp_VCMPEQ>();
            void *Src1 = GetSrc<void*>(Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(Op->Header.Args[1]);
            uint8_t Tmp[16];

            uint8_t Elements = Op->RegisterSize / Op->ElementSize;
            auto Func = [](auto a, auto b) { return a == b ? ~0ULL : 0; };

            switch (Op->ElementSize) {
              DO_VECTOR_OP(1, uint8_t,   Func)
              DO_VECTOR_OP(2, uint16_t,  Func)
              DO_VECTOR_OP(4, uint32_t,  Func)
              DO_VECTOR_OP(8, uint64_t,  Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
            }

            memcpy(GDP, Tmp, Op->RegisterSize);
            break;
          }
          case IR::OP_VCMPGT: {
            auto Op = IROp->C<IR::IROp_VCMPGT>();
            void *Src1 = GetSrc<void*>(Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(Op->Header.Args[1]);
            uint8_t Tmp[16];

            uint8_t Elements = Op->RegisterSize / Op->ElementSize;
            auto Func = [](auto a, auto b) { return a > b ? ~0ULL : 0; };

            switch (Op->ElementSize) {
              DO_VECTOR_OP(1, int8_t,   Func)
              DO_VECTOR_OP(2, int16_t,  Func)
              DO_VECTOR_OP(4, int32_t,  Func)
              DO_VECTOR_OP(8, int64_t,  Func)
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->ElementSize); break;
            }

            memcpy(GDP, Tmp, Op->RegisterSize);
            break;
          }
          case IR::OP_LUDIV: {
            auto Op = IROp->C<IR::IROp_LUDiv>();
            // Each source is OpSize in size
            // So you can have up to a 128bit divide from x86-64
            switch (OpSize) {
              case 2: {
                uint16_t SrcLow = *GetSrc<uint16_t*>(Op->Header.Args[0]);
                uint16_t SrcHigh = *GetSrc<uint16_t*>(Op->Header.Args[1]);
                uint16_t Divisor = *GetSrc<uint16_t*>(Op->Header.Args[2]);
                uint32_t Source = (static_cast<uint32_t>(SrcHigh) << 16) | SrcLow;
                uint32_t Res = Source / Divisor;

                // We only store the lower bits of the result
                GD = static_cast<uint16_t>(Res);
                break;
              }
              case 4: {
                uint32_t SrcLow = *GetSrc<uint32_t*>(Op->Header.Args[0]);
                uint32_t SrcHigh = *GetSrc<uint32_t*>(Op->Header.Args[1]);
                uint32_t Divisor = *GetSrc<uint32_t*>(Op->Header.Args[2]);
                uint64_t Source = (static_cast<uint64_t>(SrcHigh) << 32) | SrcLow;
                uint64_t Res = Source / Divisor;

                // We only store the lower bits of the result
                GD = static_cast<uint32_t>(Res);
                break;
              }
              case 8: {
                uint64_t SrcLow = *GetSrc<uint64_t*>(Op->Header.Args[0]);
                uint64_t SrcHigh = *GetSrc<uint64_t*>(Op->Header.Args[1]);
                uint64_t Divisor = *GetSrc<uint64_t*>(Op->Header.Args[2]);
                __uint128_t Source = (static_cast<__uint128_t>(SrcHigh) << 64) | SrcLow;
                __uint128_t Res = Source / Divisor;

                // We only store the lower bits of the result
                memcpy(GDP, &Res, OpSize);
                break;
              }
              default: LogMan::Msg::A("Unknown LUDIV Size: %d", OpSize); break;
            }
            break;
          }
          case IR::OP_LDIV: {
            auto Op = IROp->C<IR::IROp_LDiv>();
            // Each source is OpSize in size
            // So you can have up to a 128bit divide from x86-64
            switch (OpSize) {
              case 2: {
                uint16_t SrcLow = *GetSrc<uint16_t*>(Op->Header.Args[0]);
                uint16_t SrcHigh = *GetSrc<uint16_t*>(Op->Header.Args[1]);
                int16_t Divisor = *GetSrc<uint16_t*>(Op->Header.Args[2]);
                int32_t Source = (static_cast<uint32_t>(SrcHigh) << 16) | SrcLow;
                int32_t Res = Source / Divisor;

                // We only store the lower bits of the result
                GD = static_cast<int16_t>(Res);
                break;
              }
              case 4: {
                uint32_t SrcLow = *GetSrc<uint32_t*>(Op->Header.Args[0]);
                uint32_t SrcHigh = *GetSrc<uint32_t*>(Op->Header.Args[1]);
                int32_t Divisor = *GetSrc<uint32_t*>(Op->Header.Args[2]);
                int64_t Source = (static_cast<uint64_t>(SrcHigh) << 32) | SrcLow;
                int64_t Res = Source / Divisor;

                // We only store the lower bits of the result
                GD = static_cast<int32_t>(Res);
                break;
              }
              case 8: {
                uint64_t SrcLow = *GetSrc<uint64_t*>(Op->Header.Args[0]);
                uint64_t SrcHigh = *GetSrc<uint64_t*>(Op->Header.Args[1]);
                int64_t Divisor = *GetSrc<int64_t*>(Op->Header.Args[2]);
                __int128_t Source = (static_cast<__int128_t>(SrcHigh) << 64) | SrcLow;
                __int128_t Res = Source / Divisor;

                // We only store the lower bits of the result
                memcpy(GDP, &Res, OpSize);
                break;
              }
              default: LogMan::Msg::A("Unknown LDIV Size: %d", OpSize); break;
            }
            break;
          }
          case IR::OP_LUREM: {
            auto Op = IROp->C<IR::IROp_LURem>();
            // Each source is OpSize in size
            // So you can have up to a 128bit Remainder from x86-64
            switch (OpSize) {
              case 2: {
                uint16_t SrcLow = *GetSrc<uint16_t*>(Op->Header.Args[0]);
                uint16_t SrcHigh = *GetSrc<uint16_t*>(Op->Header.Args[1]);
                uint16_t Divisor = *GetSrc<uint16_t*>(Op->Header.Args[2]);
                uint32_t Source = (static_cast<uint32_t>(SrcHigh) << 16) | SrcLow;
                uint32_t Res = Source % Divisor;

                // We only store the lower bits of the result
                GD = static_cast<uint16_t>(Res);
                break;
              }

              case 4: {
                uint32_t SrcLow = *GetSrc<uint32_t*>(Op->Header.Args[0]);
                uint32_t SrcHigh = *GetSrc<uint32_t*>(Op->Header.Args[1]);
                uint32_t Divisor = *GetSrc<uint32_t*>(Op->Header.Args[2]);
                uint64_t Source = (static_cast<uint64_t>(SrcHigh) << 32) | SrcLow;
                uint64_t Res = Source % Divisor;

                // We only store the lower bits of the result
                GD = static_cast<uint32_t>(Res);
                break;
              }
              case 8: {
                uint64_t SrcLow = *GetSrc<uint64_t*>(Op->Header.Args[0]);
                uint64_t SrcHigh = *GetSrc<uint64_t*>(Op->Header.Args[1]);
                uint64_t Divisor = *GetSrc<uint64_t*>(Op->Header.Args[2]);
                __uint128_t Source = (static_cast<__uint128_t>(SrcHigh) << 64) | SrcLow;
                __uint128_t Res = Source % Divisor;
                // We only store the lower bits of the result
                memcpy(GDP, &Res, OpSize);
                break;
              }
              default: LogMan::Msg::A("Unknown LUREM Size: %d", OpSize); break;
            }
            break;
          }
          case IR::OP_LREM: {
            auto Op = IROp->C<IR::IROp_LRem>();
            // Each source is OpSize in size
            // So you can have up to a 128bit Remainder from x86-64
            switch (OpSize) {
              case 2: {
                uint16_t SrcLow = *GetSrc<uint16_t*>(Op->Header.Args[0]);
                uint16_t SrcHigh = *GetSrc<uint16_t*>(Op->Header.Args[1]);
                int16_t Divisor = *GetSrc<uint16_t*>(Op->Header.Args[2]);
                int32_t Source = (static_cast<uint32_t>(SrcHigh) << 16) | SrcLow;
                int32_t Res = Source % Divisor;

                // We only store the lower bits of the result
                GD = static_cast<int16_t>(Res);
                break;
              }
              case 4: {
                uint32_t SrcLow = *GetSrc<uint32_t*>(Op->Header.Args[0]);
                uint32_t SrcHigh = *GetSrc<uint32_t*>(Op->Header.Args[1]);
                int32_t Divisor = *GetSrc<uint32_t*>(Op->Header.Args[2]);
                int64_t Source = (static_cast<uint64_t>(SrcHigh) << 32) | SrcLow;
                int64_t Res = Source % Divisor;

                // We only store the lower bits of the result
                GD = static_cast<int32_t>(Res);
                break;
              }
              case 8: {
                uint64_t SrcLow = *GetSrc<uint64_t*>(Op->Header.Args[0]);
                uint64_t SrcHigh = *GetSrc<uint64_t*>(Op->Header.Args[1]);
                int64_t Divisor = *GetSrc<int64_t*>(Op->Header.Args[2]);
                __int128_t Source = (static_cast<__int128_t>(SrcHigh) << 64) | SrcLow;
                __int128_t Res = Source % Divisor;
                // We only store the lower bits of the result
                memcpy(GDP, &Res, OpSize);
                break;
              }
              default: LogMan::Msg::A("Unknown LREM Size: %d", OpSize); break;
            }
            break;
          }
          case IR::OP_VEXTR: {
            auto Op = IROp->C<IR::IROp_VExtr>();
            __uint128_t Src1 = *GetSrc<__uint128_t*>(Op->Header.Args[0]);
            __uint128_t Src2 = *GetSrc<__uint128_t*>(Op->Header.Args[1]);

            uint64_t Offset = Op->Index * Op->ElementSize * 8;
            __uint128_t Dst = (Src1 << (sizeof(__uint128_t) * 8 - Offset)) | (Src2 >> Offset);

            memcpy(GDP, &Dst, Op->RegisterSize);
            break;
          }
          case IR::OP_VINSGPR: {
            auto Op = IROp->C<IR::IROp_VInsGPR>();
            __uint128_t Src1 = *GetSrc<__uint128_t*>(Op->Header.Args[0]);
            __uint128_t Src2 = *GetSrc<__uint128_t*>(Op->Header.Args[1]);

            uint64_t Offset = Op->Index * Op->ElementSize * 8;
            __uint128_t Mask = (1ULL << (Op->ElementSize * 8)) - 1;
            Mask <<= Offset;
            Mask = ~Mask;
            __uint128_t Dst = Src1 & Mask;
            Dst |= Src2 << Offset;

            memcpy(GDP, &Dst, Op->RegisterSize);
            break;
          }
          case IR::OP_FLOAT_FROMGPR_S: {
            auto Op = IROp->C<IR::IROp_Float_FromGPR_S>();
            if (Op->ElementSize == 8) {
              double Dst = (double)*GetSrc<int64_t*>(Op->Header.Args[0]);
              memcpy(GDP, &Dst, Op->ElementSize);
            }
            else {
              float Dst = (float)*GetSrc<int32_t*>(Op->Header.Args[0]);
              memcpy(GDP, &Dst, Op->ElementSize);
            }
            break;
          }
          case IR::OP_FLOAT_FROMGPR_U: {
            auto Op = IROp->C<IR::IROp_Float_FromGPR_U>();
            if (Op->ElementSize == 8) {
              double Dst = (double)*GetSrc<uint64_t*>(Op->Header.Args[0]);
              memcpy(GDP, &Dst, Op->ElementSize);
            }
            else {
              float Dst = (float)*GetSrc<uint32_t*>(Op->Header.Args[0]);
              memcpy(GDP, &Dst, Op->ElementSize);
            }
            break;
          }
          case IR::OP_FLOAT_TOGPR_ZS: {
            auto Op = IROp->C<IR::IROp_Float_ToGPR_ZS>();
            if (Op->ElementSize == 8) {
              int64_t Dst = (int64_t)*GetSrc<double*>(Op->Header.Args[0]);
              memcpy(GDP, &Dst, Op->ElementSize);
            }
            else {
              int32_t Dst = (int32_t)*GetSrc<float*>(Op->Header.Args[0]);
              memcpy(GDP, &Dst, Op->ElementSize);
            }
            break;
          }
          case IR::OP_FLOAT_TOGPR_ZU: {
            auto Op = IROp->C<IR::IROp_Float_ToGPR_ZU>();
            if (Op->ElementSize == 8) {
              uint64_t Dst = (uint64_t)*GetSrc<double*>(Op->Header.Args[0]);
              memcpy(GDP, &Dst, Op->ElementSize);
            }
            else {
              uint32_t Dst = (uint32_t)*GetSrc<float*>(Op->Header.Args[0]);
              memcpy(GDP, &Dst, Op->ElementSize);
            }
            break;
          }
          case IR::OP_FLOAT_FTOF: {
            auto Op = IROp->C<IR::IROp_Float_FToF>();
            uint16_t Conv = (Op->DstElementSize << 8) | Op->SrcElementSize;
            switch (Conv) {
              case 0x0804: { // Double <- Float
                double Dst = (double)*GetSrc<float*>(Op->Header.Args[0]);
                memcpy(GDP, &Dst, 8);
                break;
              }
              case 0x0408: { // Float <- Double
                float Dst = (float)*GetSrc<double*>(Op->Header.Args[0]);
                memcpy(GDP, &Dst, 4);
                break;
              }
              default: LogMan::Msg::A("Unknown FCVT sizes: 0x%x", Conv);
            }
            break;
          }
          case IR::OP_VECTOR_FTOF: {
            auto Op = IROp->C<IR::IROp_Vector_FToF>();
            void *Src = GetSrc<void*>(Op->Header.Args[0]);
            uint8_t Tmp[16]{};

            uint16_t Conv = (Op->DstElementSize << 8) | Op->SrcElementSize;

            auto Func = [](auto a, auto min, auto max) { return a; };
            switch (Conv) {
              case 0x0804: { // Double <- float
                uint8_t Elements = Op->RegisterSize / Op->SrcElementSize;
                switch (Op->SrcElementSize) {
                DO_VECTOR_1SRC_2TYPE_OP(4, double, float, Func, 0, 0)
                }
                break;
              }
              case 0x0408: { // Float <- Double
                uint8_t Elements = Op->RegisterSize / Op->SrcElementSize;
                switch (Op->SrcElementSize) {
                DO_VECTOR_1SRC_2TYPE_OP(8, float, double, Func, 0, 0)
                }
                break;

                break;
              }
              default: LogMan::Msg::A("Unknown Conversion Type : 0%04x", Conv); break;
            }
            memcpy(GDP, Tmp, Op->RegisterSize);
            break;
          }
          case IR::OP_FCMP: {
            auto Op = IROp->C<IR::IROp_FCmp>();
            uint32_t ResultFlags{};
            if (Op->ElementSize == 4) {
              float Src1 = *GetSrc<float*>(Op->Header.Args[0]);
              float Src2 = *GetSrc<float*>(Op->Header.Args[1]);
              if (Op->Flags & (1 << FCMP_FLAG_LT)) {
                if (Src1 < Src2) {
                  ResultFlags |= (1 << FCMP_FLAG_LT);
                }
              }
              if (Op->Flags & (1 << FCMP_FLAG_UNORDERED)) {
                if (std::isnan(Src1) || std::isnan(Src2)) {
                  ResultFlags |= (1 << FCMP_FLAG_UNORDERED);
                }
              }
              if (Op->Flags & (1 << FCMP_FLAG_EQ)) {
                if (Src1 == Src2) {
                  ResultFlags |= (1 << FCMP_FLAG_EQ);
                }
              }
            }
            else {
              double Src1 = *GetSrc<double*>(Op->Header.Args[0]);
              double Src2 = *GetSrc<double*>(Op->Header.Args[1]);
              if (Op->Flags & (1 << FCMP_FLAG_LT)) {
                if (Src1 < Src2) {
                  ResultFlags |= (1 << FCMP_FLAG_LT);
                }
              }
              if (Op->Flags & (1 << FCMP_FLAG_UNORDERED)) {
                if (std::isnan(Src1) || std::isnan(Src2)) {
                  ResultFlags |= (1 << FCMP_FLAG_UNORDERED);
                }
              }
              if (Op->Flags & (1 << FCMP_FLAG_EQ)) {
                if (Src1 == Src2) {
                  ResultFlags |= (1 << FCMP_FLAG_EQ);
                }
              }
            }

            GD = ResultFlags;
            break;
          }
          case IR::OP_GETHOSTFLAG: {
            auto Op = IROp->C<IR::IROp_GetHostFlag>();
            GD = (*GetSrc<uint64_t*>(Op->Header.Args[0]) >> Op->Flag) & 1;
            break;
          }
          #define DO_SCALAR_COMPARE_OP(size, type, type2, func)              \
            case size: {                                      \
            auto *Dst_d  = reinterpret_cast<type2*>(Tmp);  \
            auto *Src1_d = reinterpret_cast<type*>(Src1); \
            auto *Src2_d = reinterpret_cast<type*>(Src2); \
            Dst_d[0] = func(Src1_d[0], Src2_d[0]);          \
            break;                                            \
            }

          #define DO_VECTOR_COMPARE_OP(size, type, type2, func)              \
            case size: {                                      \
            auto *Dst_d  = reinterpret_cast<type2*>(Tmp);  \
            auto *Src1_d = reinterpret_cast<type*>(Src1); \
            auto *Src2_d = reinterpret_cast<type*>(Src2); \
            for (uint8_t i = 0; i < Elements; ++i) {          \
              Dst_d[i] = func(Src1_d[i], Src2_d[i]);          \
            }                                                 \
            break;                                            \
            }

          case IR::OP_VFCMPEQ: {
            auto Op = IROp->C<IR::IROp_VFCMPEQ>();
            void *Src1 = GetSrc<void*>(Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(Op->Header.Args[1]);

            auto Func = [](auto a, auto b) { return a == b ? ~0ULL : 0; };

            uint8_t Tmp[16];
            uint8_t Elements = Op->RegisterSize / Op->ElementSize;

            if (Op->ElementSize == Op->RegisterSize) {
              switch (Op->ElementSize) {
              DO_SCALAR_COMPARE_OP(4, float, uint32_t, Func);
              DO_SCALAR_COMPARE_OP(8, double, uint64_t, Func);
              default: LogMan::Msg::A("Unsupported elementSize: %d", Op->ElementSize);
              }
            }
            else {
              switch (Op->ElementSize) {
              DO_VECTOR_COMPARE_OP(4, float, uint32_t, Func);
              DO_VECTOR_COMPARE_OP(8, double, uint64_t, Func);
              default: LogMan::Msg::A("Unsupported elementSize: %d", Op->ElementSize);
              }
            }

            memcpy(GDP, Tmp, Op->RegisterSize);
            break;
          }
          case IR::OP_VFCMPNEQ: {
            auto Op = IROp->C<IR::IROp_VFCMPNEQ>();
            void *Src1 = GetSrc<void*>(Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(Op->Header.Args[1]);

            auto Func = [](auto a, auto b) { return a != b ? ~0ULL : 0; };

            uint8_t Tmp[16];
            uint8_t Elements = Op->RegisterSize / Op->ElementSize;

            if (Op->ElementSize == Op->RegisterSize) {
              switch (Op->ElementSize) {
              DO_SCALAR_COMPARE_OP(4, float, uint32_t, Func);
              DO_SCALAR_COMPARE_OP(8, double, uint64_t, Func);
              default: LogMan::Msg::A("Unsupported elementSize: %d", Op->ElementSize);
              }
            }
            else {
              switch (Op->ElementSize) {
              DO_VECTOR_COMPARE_OP(4, float, uint32_t, Func);
              DO_VECTOR_COMPARE_OP(8, double, uint64_t, Func);
              default: LogMan::Msg::A("Unsupported elementSize: %d", Op->ElementSize);
              }
            }

            memcpy(GDP, Tmp, Op->RegisterSize);
            break;
          }
          case IR::OP_VFCMPLT: {
            auto Op = IROp->C<IR::IROp_VFCMPLT>();
            void *Src1 = GetSrc<void*>(Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(Op->Header.Args[1]);

            auto Func = [](auto a, auto b) { return a < b ? ~0ULL : 0; };

            uint8_t Tmp[16];
            uint8_t Elements = Op->RegisterSize / Op->ElementSize;

            if (Op->ElementSize == Op->RegisterSize) {
              switch (Op->ElementSize) {
              DO_SCALAR_COMPARE_OP(4, float, uint32_t, Func);
              DO_SCALAR_COMPARE_OP(8, double, uint64_t, Func);
              default: LogMan::Msg::A("Unsupported elementSize: %d", Op->ElementSize);
              }
            }
            else {
              switch (Op->ElementSize) {
              DO_VECTOR_COMPARE_OP(4, float, uint32_t, Func);
              DO_VECTOR_COMPARE_OP(8, double, uint64_t, Func);
              default: LogMan::Msg::A("Unsupported elementSize: %d", Op->ElementSize);
              }
            }

            memcpy(GDP, Tmp, Op->RegisterSize);
            break;
          }

          case IR::OP_VFCMPLE: {
            auto Op = IROp->C<IR::IROp_VFCMPLE>();
            void *Src1 = GetSrc<void*>(Op->Header.Args[0]);
            void *Src2 = GetSrc<void*>(Op->Header.Args[1]);

            auto Func = [](auto a, auto b) { return a <= b ? ~0ULL : 0; };

            uint8_t Tmp[16];
            uint8_t Elements = Op->RegisterSize / Op->ElementSize;

            if (Op->ElementSize == Op->RegisterSize) {
              switch (Op->ElementSize) {
              DO_SCALAR_COMPARE_OP(4, float, uint32_t, Func);
              DO_SCALAR_COMPARE_OP(8, double, uint64_t, Func);
              default: LogMan::Msg::A("Unsupported elementSize: %d", Op->ElementSize);
              }
            }
            else {
              switch (Op->ElementSize) {
              DO_VECTOR_COMPARE_OP(4, float, uint32_t, Func);
              DO_VECTOR_COMPARE_OP(8, double, uint64_t, Func);
              default: LogMan::Msg::A("Unsupported elementSize: %d", Op->ElementSize);
              }
            }

            memcpy(GDP, Tmp, Op->RegisterSize);
            break;
          }

          default:
            LogMan::Msg::A("Unknown IR Op: %d(%s)", IROp->Op, FEXCore::IR::GetName(IROp->Op).data());
            break;
        }

        // CodeLast is inclusive. So we still need to dump the CodeLast op as well
        if (CodeBegin == CodeLast) {
          break;
        }
        ++CodeBegin;
      }
    };

    HandleBlock();

    if (BlockResults.Redo) {
      continue;
    }

    if (BlockIROp->Next.ID() == 0 || BlockResults.Quit) {
      break;
    } else {
      BlockNode = BlockIROp->Next.GetNode(ListBegin);
    }
  }

  Thread->Stats.InstructionsExecuted.fetch_add(DebugData->second.GuestInstructionCount);
}

FEXCore::CPU::CPUBackend *CreateInterpreterCore(FEXCore::Context::Context *ctx) {
  return new InterpreterCore(ctx);
}

}
