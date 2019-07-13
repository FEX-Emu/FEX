#pragma once
#include <FEXCore/Config/Config.h>
#include <FEXCore/Core/Context.h>
#include <FEXCore/Core/CoreState.h>
#include <sstream>

namespace FEX::DebuggerState {
bool ActiveCore();

void SetContext(FEXCore::Context::Context *ctx);
FEXCore::Context::Context *GetContext();

FEXCore::Config::ConfigCore GetCoreType();
void SetCoreType(FEXCore::Config::ConfigCore CoreType);

int GetRunningMode(); ///< This is the running mode we've set
int GetCoreCurrentRunningMode(); ///< This typically matches `GetRunningMode()` but can differ when we are in the middle of stepping
void SetRunningMode(int RunningMode);

FEXCore::Core::CPUState GetCPUState();

bool IsCoreRunning();

// Client interface
void RegisterStepCallback(std::function<void()> Callback);
void RegisterPauseCallback(std::function<void()> Callback);
void RegisterContinueCallback(std::function<void()> Callback);
void Step();
void Pause();
void Continue();

void RegisterCreateCallback(std::function<void(char const *Filename, bool ELF)> Callback);
void Create(char const *Filename, bool ELF);

void RegisterCompileRIPCallback(std::function<void(uint64_t)> Callback);
void CompileRIP(uint64_t RIP);

void RegisterCloseCallback(std::function<void()> Callback);
void Close();

void RegisterNewStateCallback(std::function<void()> Callback);
void CallNewState();

void RegisterGetIRCallback(std::function<void(std::stringstream *out, uint64_t PC)>);
void GetIR(std::stringstream *out, uint64_t PC);

bool HasNewState();
void SetHasNewState(bool State = true);
}
