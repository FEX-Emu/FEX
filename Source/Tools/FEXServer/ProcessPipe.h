#pragma once
#include <cstdint>
#include <sys/types.h>

namespace ProcessPipe {
  bool InitializeServerPipe();
  bool InitializeServerSocket();
  void WaitForRequests();
  void SetConfiguration(bool Foreground, uint32_t PersistentTimeout);
  void Shutdown();

  void CheckRaiseFDLimit(ssize_t IncrementAmount);
}
