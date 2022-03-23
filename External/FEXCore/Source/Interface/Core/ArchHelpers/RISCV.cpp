#include "Interface/Core/ArchHelpers/RISCV.h"
#include "Interface/Core/ArchHelpers/MContext.h"

#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/Telemetry.h>

#include <atomic>
#include <stdint.h>

#include <signal.h>

namespace FEXCore::ArchHelpers::RISCV {
FEXCORE_TELEMETRY_STATIC_INIT(SplitLock, TYPE_HAS_SPLIT_LOCKS);
FEXCORE_TELEMETRY_STATIC_INIT(SplitLock16B, TYPE_16BYTE_SPLIT);
FEXCORE_TELEMETRY_STATIC_INIT(Cas16Tear,  TYPE_CAS_16BIT_TEAR);
FEXCORE_TELEMETRY_STATIC_INIT(Cas32Tear,  TYPE_CAS_32BIT_TEAR);
FEXCORE_TELEMETRY_STATIC_INIT(Cas64Tear,  TYPE_CAS_64BIT_TEAR);
FEXCORE_TELEMETRY_STATIC_INIT(Cas128Tear, TYPE_CAS_128BIT_TEAR);

uint32_t Helper_LoadFence32(uint64_t Addr) {
  uint32_t Result{};
  asm (R"(
  fence rw, rw;
  lw %[Result], 0(%[Addr]);
  fence r, rw;
  )"
  : [Result] "=r" (Result)
  : [Addr] "r" (Addr));
  return Result;
}

uint64_t Helper_LoadFence64(uint64_t Addr) {
  uint64_t Result{};
  asm (R"(
  fence rw, rw;
  ld %[Result], 0(%[Addr]);
  fence r, rw;
  )"
  : [Result] "=r" (Result)
  : [Addr] "r" (Addr));
  return Result;
}

void Helper_StoreFence32(uint32_t Data, uint64_t Addr) {
  asm (R"(
  fence rw, w;
  sw %[Data], 0(%[Addr]);
  )"
  :: [Data] "r" (Data)
   , [Addr] "r" (Addr)
  : "memory");
}

void Helper_StoreFence64(uint64_t Data, uint64_t Addr) {
  asm (R"(
  fence rw, w;
  sd %[Data], 0(%[Addr]);
  )"
  :: [Data] "r" (Data)
   , [Addr] "r" (Addr)
  : "memory");
}

uint32_t Helper_StoreRelease32(uint32_t Data, uint64_t Addr) {
  uint32_t Result{};
  asm (R"(
  sc.w.aqrl %[Result], %[Data], (%[Addr]);
  )"
  : [Result] "=r" (Result)
  : [Data] "r" (Data)
  , [Addr] "r" (Addr)
  : "memory");
  return Result;
}

uint64_t Helper_StoreRelease64(uint64_t Data, uint64_t Addr) {
  uint64_t Result{};
  asm (R"(
  sc.d.aqrl %[Result], %[Data], (%[Addr]);
  )"
  : [Result] "=r" (Result)
  : [Data] "r" (Data)
  , [Addr] "r" (Addr)
  : "memory");
  return Result;
}

static uint32_t DoLoad32(uint64_t Addr) {
  uint64_t AlignmentMask = 0b1111;
  if ((Addr & AlignmentMask) > 12) {
    // Address crosses over 16byte threshold
    // Needs non-atomic load with fences
    return Helper_LoadFence32(Addr);
  }
  else {
    AlignmentMask = 0b111;
    if ((Addr & AlignmentMask) >= 5) {
      // Crosses 8byte boundary
      // Needs 128bit load
      // Fits within a 16byte region
      // XXX: Until we have a 128-bit load, do a fenced load
      return Helper_LoadFence32(Addr);
    }
    else {
      // Fits within 8byte boundary
      // Only needs 64bit CAS
      // Alignments can be [1,5)
      uint64_t Alignment = Addr & AlignmentMask;
      Addr &= ~AlignmentMask;

      std::atomic<uint64_t> *Atomic = reinterpret_cast<std::atomic<uint64_t>*>(Addr);
      uint64_t TmpResult = Atomic->load();

      return TmpResult >> (Alignment * 8);
    }
  }
}

uint64_t DoLoad64(uint64_t Addr) {
  uint64_t AlignmentMask = 0b1111;
  if ((Addr & AlignmentMask) > 8) {
    // Address crosses over 16byte threshold
    // Needs non-atomic load with fences
    return Helper_LoadFence64(Addr);
  }
  else {
    // Fits within a 16byte region
    // Can use a 128-bit load
    // XXX: Until we have a 128-bit load, do a fenced load
    return Helper_LoadFence64(Addr);
  }
}

static uint32_t DoStore32(uint32_t Data, uint64_t Addr) {
  uint64_t AlignmentMask = 0b1111;
  if ((Addr & AlignmentMask) > 12) {
    // Address crosses over 16byte threshold
    // Needs non-atomic store with fences
    Helper_StoreFence32(Data, Addr);
    // Claim we stored successfully to guarantee forward progress
    return 0;
  }
  else {
    AlignmentMask = 0b111;
    if ((Addr & AlignmentMask) >= 5) {
      // Crosses 8byte boundary
      // Needs 128bit load
      // Fits within a 16byte region
      // XXX: Until we have a 128-bit load, do a fenced load
      Helper_StoreFence32(Data, Addr);
      // Claim we stored successfully to guarantee forward progress
      return 0;
    }
    else {
      // Fits within 8byte boundary
      // Only needs 64bit CAS
      // Alignments can be [1,5)
      // XXX: We can do store release with some insert here
      Helper_StoreFence64(Data, Addr);
      // Claim we stored successfully to guarantee forward progress
      return 0;
    }
  }
}


bool HandleAtomicLoad(void *_ucontext, void *_info, uint32_t Instr) {
  mcontext_t* mcontext = &reinterpret_cast<ucontext_t*>(_ucontext)->uc_mcontext;
  siginfo_t* info = reinterpret_cast<siginfo_t*>(_info);

  if (info->si_code != BUS_ADRALN) {
    // This only handles alignment problems
    return false;
  }

  uint32_t Size = 1 << ((Instr & 0x0000'7000) >> 12);
  uint32_t AddrReg = (Instr >> 15) & 0x1F;
  uint32_t DestReg = (Instr >> 7) & 0x1F;

  uint64_t Addr = mcontext->__gregs[AddrReg];

  if (Size == 4) {
    auto Res = DoLoad32(Addr);
    // We set the result register if it isn't a zero register
    if (DestReg != 0) {
      mcontext->__gregs[DestReg] = Res;
    }
    return true;
  }
  else if (Size == 8) {
    auto Res = DoLoad64(Addr);
    // We set the result register if it isn't a zero register
    if (DestReg != 0) {
      mcontext->__gregs[DestReg] = Res;
    }
    return true;
  }

  return false;
}

bool HandleAtomicStore(void *_ucontext, void *_info, uint32_t Instr) {
  mcontext_t* mcontext = &reinterpret_cast<ucontext_t*>(_ucontext)->uc_mcontext;
  siginfo_t* info = reinterpret_cast<siginfo_t*>(_info);

  uint32_t Size = 1 << ((Instr & 0x0000'7000) >> 12);
  uint32_t AddrReg = (Instr >> 15) & 0x1F;
  uint32_t DataReg = (Instr >> 20) & 0x1F;
  uint32_t DestReg = (Instr >> 7) & 0x1F;

  uint64_t Addr = mcontext->__gregs[AddrReg];

  if (Size == 4) {
    auto Res = DoStore32(mcontext->__gregs[DataReg], Addr);
    // We set the result register if it isn't a zero register
    if (DestReg != 0) {
      mcontext->__gregs[DestReg] = Res;
    }
    return true;
  }
  else if (Size == 8) {
    auto Res = DoLoad64(Addr);
    // We set the result register if it isn't a zero register
    if (DestReg != 0) {
      mcontext->__gregs[DestReg] = Res;
    }
    return true;
  }

  return false;
}

AtomicOperation FindAtomicOperationType(uint32_t *PC) {
  // Scan forward a maximum of ten instructions
  bool Found = false;
  size_t NumberOfInstructions = 0;
  size_t OpSize = 0;
  for (; NumberOfInstructions < 10; ++NumberOfInstructions) {
    uint32_t Instr = PC[NumberOfInstructions];
    if ((Instr & AMO_OP_MASK) == AMO_OP_SC_AQRL) {
      // Found the ending SC.W.AQRL for this op
      if (OpSize == 0) {
        // Recover the size of the store if OpSize == 0 for 4byte or 8byte store
        OpSize = 1 << ((Instr & 0x0000'7000) >> 12);
      }
      Found = true;
      break;
    }

    // If the first instruction is a `SLLI64` or `ZEXTH_64` then we are either an 8-bit or 16-bit emulated operation
    if (NumberOfInstructions == 0) {
      if ((Instr 
    }
    // The first logical operation we find after the load that isn't a zero extend or and with immediate
  }

  if (!Found) {
    return AtomicOperation::OP_NONE;
  }
}

bool HandleSIGBUS(bool ParanoidTSO, int Signal, void *info, void *ucontext) {
  siginfo_t* _info = reinterpret_cast<siginfo_t*>(info);

  if (_info->si_code != BUS_ADRALN) {
    // This only handles alignment problems
    return false;
  }

  uint32_t *PC = (uint32_t*)ArchHelpers::Context::GetPc(ucontext);
  uint32_t Instr = PC[0];

  // 2 = 32bit
  // 3 = 64bit
  if ((Instr & AMO_OP_MASK) == AMO_OP_LR_AQRL) {

    auto AtomicOp = FindAtomicOperationType(&PC[1]);

    LogMan::Msg::DFmt("AMO Load");
    if (FEXCore::ArchHelpers::RISCV::HandleAtomicLoad(ucontext, info, Instr)) {
      LogMan::Msg::DFmt("AMO Load skipping: 0x{:x} -> 0x{:x}", ArchHelpers::Context::GetPc(ucontext), ArchHelpers::Context::GetPc(ucontext) + 4);
      // Skip this instruction now
      ArchHelpers::Context::SetPc(ucontext, ArchHelpers::Context::GetPc(ucontext) + 4);
      return true;
    }
    else {
      LogMan::Msg::EFmt("Unhandled JIT SIGBUS LR_AQ: PC: {} Instruction: 0x{:08x}\n", fmt::ptr(PC), PC[0]);
      return false;
    }
  }
  else if ((Instr & AMO_OP_MASK) == AMO_OP_SC_AQRL) {
    LogMan::Msg::DFmt("AMO Store");
    if (FEXCore::ArchHelpers::RISCV::HandleAtomicStore(ucontext, info, Instr)) {
      LogMan::Msg::DFmt("AMO Store skipping: 0x{:x} -> 0x{:x}", ArchHelpers::Context::GetPc(ucontext), ArchHelpers::Context::GetPc(ucontext) + 4);
      // Skip this instruction now
      ArchHelpers::Context::SetPc(ucontext, ArchHelpers::Context::GetPc(ucontext) + 4);
      return true;
    }
    else {
      LogMan::Msg::EFmt("Unhandled JIT SIGBUS SC_AQ: PC: {} Instruction: 0x{:08x}\n", fmt::ptr(PC), PC[0]);
      return false;
    }
  }

  return false;
}


}
