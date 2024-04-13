// SPDX-License-Identifier: MIT
#pragma once

namespace Logger {
void AppendLogFD(int FD);
void StartLogThread();
bool LogThreadRunning();
void Shutdown();
} // namespace Logger
