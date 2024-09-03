// SPDX-License-Identifier: MIT
/*
$info$
tags: LinuxSyscalls|syscalls-shared
$end_info$
*/
#pragma once

#include <CodeEmitter/Emitter.h>

#include <FEXCore/fextl/unordered_map.h>

#include <linux/filter.h>
#include <linux/seccomp.h>

struct sock_fprog;
struct sock_filter;

namespace FEX::HLE {
class BPFEmitter final : public ARMEmitter::Emitter {
public:
  struct WorkingBuffer {
    struct seccomp_data Data;
    uint32_t ScratchMemory[BPF_MEMWORDS]; // Defined as 16 words.
  };

  BPFEmitter() = default;

  uint64_t JITFilter(uint32_t flags, const sock_fprog* prog);
  void* GetFunc() const {
    return Func;
  }

  size_t AllocationSize() const {
    return FuncSize;
  }

private:
  template<bool CalculateSize>
  uint64_t HandleLoad(uint32_t BPFIP, const sock_filter* Inst);
  template<bool CalculateSize>
  uint64_t HandleStore(uint32_t BPFIP, const sock_filter* Inst);
  template<bool CalculateSize>
  uint64_t HandleALU(uint32_t BPFIP, const sock_filter* Inst);
  template<bool CalculateSize>
  uint64_t HandleJmp(uint32_t BPFIP, uint32_t NumInst, const sock_filter* Inst);
  template<bool CalculateSize>
  uint64_t HandleRet(uint32_t BPFIP, const sock_filter* Inst);
  template<bool CalculateSize>
  uint64_t HandleMisc(uint32_t BPFIP, const sock_filter* Inst);

#define EMIT_INST(x)               \
  do {                             \
    if constexpr (CalculateSize) { \
      OpSize += 4;                 \
    } else {                       \
      x;                           \
    }                              \
  } while (0)

#define RETURN_ERROR(x)                                                                \
  if constexpr (CalculateSize) {                                                       \
    return ~0ULL;                                                                      \
  } else {                                                                             \
    static_assert(x == -EINVAL, "Early return error evaluation only supports EINVAL"); \
    return x;                                                                          \
  }

#define RETURN_SUCCESS()           \
  do {                             \
    if constexpr (CalculateSize) { \
      return OpSize;               \
    } else {                       \
      return 0;                    \
    }                              \
  } while (0)

  using SizeErrorCheck = decltype([](uint64_t Result) -> bool { return Result == ~0ULL; });
  using EmissionErrorCheck = decltype([](uint64_t Result) { return Result != 0; });

  template<bool CalculateSize, class Pred>
  uint64_t HandleEmission(uint32_t flags, const sock_fprog* prog);

  // Register selection comes from function signature.
  constexpr static auto REG_A = ARMEmitter::WReg::w0;
  constexpr static auto REG_X = ARMEmitter::WReg::w1;
  constexpr static auto REG_TMP = ARMEmitter::WReg::w2;
  constexpr static auto REG_TMP2 = ARMEmitter::WReg::w3;
  constexpr static auto REG_SECCOMP_DATA = ARMEmitter::XReg::x4;
  fextl::unordered_map<uint32_t, ARMEmitter::ForwardLabel> JumpLabels;
  fextl::unordered_map<uint32_t, ARMEmitter::ForwardLabel> ConstPool;

  void* Func;
  size_t FuncSize;
};


} // namespace FEX::HLE
