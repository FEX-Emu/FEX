#include "DebuggerState.h"

#include <FEXCore/Core/Context.h>
#include <FEXCore/Debug/ContextDebug.h>

namespace FEX::DebuggerState {

FEXCore::Context::Context *s_CTX{};
FEXCore::Config::ConfigCore s_CoreType = FEXCore::Config::ConfigCore::CONFIG_INTERPRETER;
int s_IsStepping = 0;
bool NewState = false;

std::function<void()> StepCallback;
std::function<void()> PauseCallback;
std::function<void()> ContinueCallback;
std::function<void()> NewStateCallback;
std::function<void()> CloseCallback;
std::function<void(char const *, bool)> CreateCallback;
std::function<void(uint64_t)> CompileRIPCallback;
std::function<void(std::stringstream *out, uint64_t PC)> GetIRCallback;

bool ActiveCore() {
  return s_CTX != nullptr;
}

void SetContext(FEXCore::Context::Context *ctx) {
  s_CTX = ctx;
}

FEXCore::Context::Context *GetContext() {
  return s_CTX;
}

FEXCore::Config::ConfigCore GetCoreType() {
  return s_CoreType;
}

void SetCoreType(FEXCore::Config::ConfigCore CoreType) {
  s_CoreType = CoreType;
}

int GetRunningMode() {
  return s_IsStepping;
}

int GetCoreCurrentRunningMode() {
  if (ActiveCore()) {
    return FEXCore::Config::GetConfig(s_CTX, FEXCore::Config::CONFIG_SINGLESTEP);
  }
  return s_IsStepping;
}

void SetRunningMode(int RunningMode) {
  s_IsStepping = RunningMode;

  if (ActiveCore()) {
    FEXCore::Config::SetConfig(s_CTX, FEXCore::Config::CONFIG_SINGLESTEP, RunningMode);
  }
}

FEXCore::Core::CPUState GetCPUState() {
  if (!ActiveCore()) {
    return FEXCore::Core::CPUState{};
  }
  return FEXCore::Context::Debug::GetCPUState(s_CTX);
}

bool IsCoreRunning() {
  if (!ActiveCore()) {
    return false;
  }

  return !FEXCore::Context::IsDone(s_CTX);
}

// Client interface
void RegisterStepCallback(std::function<void()> Callback) {
  StepCallback = std::move(Callback);
}

void RegisterPauseCallback(std::function<void()> Callback) {
  PauseCallback = std::move(Callback);
}

void RegisterContinueCallback(std::function<void()> Callback) {
  ContinueCallback = std::move(Callback);
}

void Step() {
  StepCallback();
}

void Pause() {
  PauseCallback();
}

void Continue() {
  ContinueCallback();
}

void RegisterCreateCallback(std::function<void(char const *Filename, bool)> Callback)
{
  CreateCallback = std::move(Callback);
}

void Create(char const *Filename, bool ELF) {
  CreateCallback(Filename, ELF);
}

void RegisterCompileRIPCallback(std::function<void(uint64_t)> Callback) {
  CompileRIPCallback = std::move(Callback);
}
void CompileRIP(uint64_t RIP) {
  CompileRIPCallback(RIP);
}

void RegisterCloseCallback(std::function<void()> Callback) {
  CloseCallback = std::move(Callback);
}

void Close() {
  CloseCallback();
}

void RegisterNewStateCallback(std::function<void()> Callback) {
  NewStateCallback = std::move(Callback);
}

void RegisterGetIRCallback(std::function<void(std::stringstream *out, uint64_t PC)> Callback) {
  GetIRCallback = std::move(Callback);
}

void GetIR(std::stringstream *out, uint64_t PC) {
  GetIRCallback(out, PC);
}

void CallNewState() {
  NewStateCallback();
}

bool HasNewState() {
  return NewState;
}

void SetHasNewState(bool State) {
  NewState = State;
}

}
