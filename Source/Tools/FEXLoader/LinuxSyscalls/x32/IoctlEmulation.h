// SPDX-License-Identifier: MIT
#pragma once
#include <cstdint>

namespace FEXCore::Core {
  struct CpuStateFrame;
}

namespace FEX::HLE::x32 {
  void InitializeStaticIoctlHandlers();
  uint32_t ioctl32(FEXCore::Core::CpuStateFrame *Frame, int fd, uint32_t request, uint32_t args);
  void CheckAndAddFDDuplication(int fd, int NewFD);
}

