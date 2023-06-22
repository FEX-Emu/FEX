#pragma once
#include <cstdint>

namespace ProcessPipe {
  bool InitializeServerPipe();
  bool InitializeServerSocket();
  void WaitForRequests();
  void SetConfiguration(bool Foreground, uint32_t PersistentTimeout);
  void Shutdown();

  void CheckRaiseFDLimit();
}
