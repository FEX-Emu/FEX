#pragma once
#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Debug/InternalThreadState.h>

#include <stdint.h>
#include <vector>

namespace FEXCore::Core {
  struct RuntimeStats;
}

namespace FEXCore::Context {
  class Context;

namespace Debug {

  void CompileRIP(FEXCore::Context::Context *CTX, uint64_t RIP);

  uint64_t GetThreadCount(FEXCore::Context::Context *CTX);
  FEXCore::Core::RuntimeStats *GetRuntimeStatsForThread(FEXCore::Context::Context *CTX, uint64_t Thread);

  bool GetDebugDataForRIP(FEXCore::Context::Context *CTX, uint64_t RIP, FEXCore::Core::DebugData *Data);
  bool FindHostCodeForRIP(FEXCore::Context::Context *CTX, uint64_t RIP, uint8_t **Code);
	// XXX:
  // bool FindIRForRIP(FEXCore::Context::Context *CTX, uint64_t RIP, FEXCore::IR::IntrusiveIRList **ir);
  // void SetIRForRIP(FEXCore::Context::Context *CTX, uint64_t RIP, FEXCore::IR::IntrusiveIRList *const ir);
}
}


