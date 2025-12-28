// SPDX-License-Identifier: MIT
#pragma once
#ifdef _WIN32
#include <FEXCore/Utils/LogManager.h>
#include <winnt.h>

namespace FEX::ArchHelpers::Context {
#ifdef ARCHITECTURE_arm64
static inline uint64_t GetSp(PCONTEXT Context) {
  return Context->Sp;
}

static inline uint64_t GetPc(PCONTEXT Context) {
  return Context->Pc;
}

static inline void SetSp(PCONTEXT Context, uint64_t val) {
  Context->Sp = val;
}

static inline void SetPc(PCONTEXT Context, uint64_t val) {
  Context->Pc = val;
}

static inline uint64_t GetState(PCONTEXT Context) {
  return Context->X28;
}

static inline void SetState(PCONTEXT Context, uint64_t val) {
  Context->X28 = val;
}

static inline uint64_t* GetArmGPRs(PCONTEXT Context) {
  return Context->X;
}
#endif

#ifdef ARCHITECTURE_x86_64
static inline uint64_t GetSp(PCONTEXT Context) {
  return Context->Rsp;
}

static inline uint64_t GetPc(PCONTEXT Context) {
  return Context->Rip;
}

static inline void SetSp(PCONTEXT Context, uint64_t val) {
  Context->Rsp = val;
}

static inline void SetPc(PCONTEXT Context, uint64_t val) {
  Context->Rip = val;
}

static inline uint64_t GetState(PCONTEXT Context) {
  return Context->R14;
}

static inline void SetState(PCONTEXT Context, uint64_t val) {
  Context->R14 = val;
}

static inline uint64_t* GetArmGPRs(PCONTEXT Context) {
  ERROR_AND_DIE_FMT("Not implemented for x86 host");
}

#endif

} // namespace FEX::ArchHelpers::Context

#endif
