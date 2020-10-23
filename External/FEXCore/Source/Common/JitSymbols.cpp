#include "Common/JitSymbols.h"

#include <string>
#include <sstream>
#include <unistd.h>

namespace FEXCore {
  JITSymbols::JITSymbols() {
    std::stringstream PerfMap;
    PerfMap << "/tmp/perf-" << getpid() << ".map";

    fp = fopen(PerfMap.str().c_str(), "wb");
    if (fp) {
      // Disable buffering on this file
      setvbuf(fp, nullptr, _IONBF, 0);
    }
  }

  JITSymbols::~JITSymbols() {
    if (fp) {
      fclose(fp);
    }
  }

  void JITSymbols::Register(void *HostAddr, uint64_t GuestAddr, uint32_t CodeSize) {
    if (!fp) return;

    // Linux perf format is very straightforward
    // `<HostPtr> <Size> <Name>\n`
    std::stringstream String;
    String << std::hex << HostAddr << " " << CodeSize << " " << "JIT_0x" << GuestAddr << "_" << HostAddr << std::endl;
    fwrite(String.str().c_str(), 1, String.str().size(), fp);
  }
}

