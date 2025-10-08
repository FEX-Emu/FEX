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

  template<bool CalculateSize, class Pred>
  uint64_t HandleEmission(uint32_t flags, const sock_fprog* prog);

  fextl::unordered_map<uint32_t, ARMEmitter::ForwardLabel> JumpLabels;
  fextl::unordered_map<uint32_t, ARMEmitter::ForwardLabel> ConstPool;

  void* Func {};
  size_t FuncSize {};
};


} // namespace FEX::HLE
