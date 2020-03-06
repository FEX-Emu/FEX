#pragma once
#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Memory/MemMapper.h>
#include <FEXCore/Debug/InternalThreadState.h>

#include <stdint.h>
#include <vector>

namespace FEXCore::Core {
  struct RuntimeStats;
}

namespace FEXCore::Context {
  struct Context;

namespace Debug {

  void CompileRIP(FEXCore::Context::Context *CTX, uint64_t RIP);

  uint64_t GetThreadCount(FEXCore::Context::Context *CTX);
  FEXCore::Core::RuntimeStats *GetRuntimeStatsForThread(FEXCore::Context::Context *CTX, uint64_t Thread);
  FEXCore::Core::CPUState GetCPUState(FEXCore::Context::Context *CTX);

  void GetMemoryRegions(FEXCore::Context::Context *CTX, std::vector<FEXCore::Memory::MemRegion> *Regions);

  bool GetDebugDataForRIP(FEXCore::Context::Context *CTX, uint64_t RIP, FEXCore::Core::DebugData *Data);
  bool FindHostCodeForRIP(FEXCore::Context::Context *CTX, uint64_t RIP, uint8_t **Code);
	// XXX:
  // bool FindIRForRIP(FEXCore::Context::Context *CTX, uint64_t RIP, FEXCore::IR::IntrusiveIRList **ir);
  // void SetIRForRIP(FEXCore::Context::Context *CTX, uint64_t RIP, FEXCore::IR::IntrusiveIRList *const ir);
  FEXCore::Core::ThreadState *GetThreadState(FEXCore::Context::Context *CTX);
}
}


