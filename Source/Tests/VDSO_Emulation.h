#pragma once
#include <FEXCore/IR/IR.h>

namespace FEX::VDSO {
  using MapperFn = std::function<void *(void *addr, size_t length, int prot, int flags, int fd, off_t offset)>;
  void* LoadVDSOThunks(bool Is64Bit, MapperFn Mapper);

  uint64_t GetVSyscallEntry(const void* VDSOBase);

  std::vector<FEXCore::IR::ThunkDefinition> const& GetVDSOThunkDefinitions();
}
