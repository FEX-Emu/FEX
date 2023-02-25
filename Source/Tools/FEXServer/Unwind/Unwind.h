#pragma once
#include "ELFMapping.h"
#include "FileMapping.h"

#include <cstring>
#include <functional>
#include <list>
#include <map>
#include <string>
#include <unistd.h>

namespace CoreDumpService {
class CoreDumpClass;
}

namespace Unwind {
  class Unwinder {
    public:

      Unwinder(CoreDumpService::CoreDumpClass *CoreDump)
        : CoreDump {CoreDump} {}

      using PeekMemoryFunc = std::function<uint64_t(uint64_t Addr, uint32_t Size)>;
      using GetFileFD = std::function<int (std::string const *Filename)>;
      virtual void Backtrace() = 0;

      void SetPeekMemory(PeekMemoryFunc PeekFunc) {
        PeekMem = PeekFunc;
      }

      void SetGetFileFD(GetFileFD GetFunc) {
        GetFD = GetFunc;
      }

      GetFileFD GetFD{};
      PeekMemoryFunc PeekMem{};
      CoreDumpService::CoreDumpClass *CoreDump;

    protected:
    bool CanBacktrace = true;
  };

}
