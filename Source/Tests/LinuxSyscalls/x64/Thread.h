/*
$info$
tags: LinuxSyscalls|syscalls-x86-64
$end_info$
*/

#pragma once
#include <stdint.h>

namespace FEXCore::Core {
struct CpuStateFrame;
}

namespace FEX::HLE::x64 {
  uint64_t SetThreadArea(FEXCore::Core::CpuStateFrame *Frame, void *tls);
  void AdjustRipForNewThread(FEXCore::Core::CpuStateFrame *Frame);
}
