// SPDX-License-Identifier: MIT
#pragma once
#include <cstdint>

namespace ProcessPipe {
bool InitializeServerPipe();
bool InitializeServerSocket(bool abstract);
void WaitForRequests();
void SetConfiguration(bool Foreground, uint32_t PersistentTimeout);
void Shutdown();
void SetWatchFD(int FD);
} // namespace ProcessPipe
