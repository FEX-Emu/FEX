#pragma once
#include <llvm/ExecutionEngine/RTDyldMemoryManager.h>

namespace FEXCore::CPU {

class LLVMMemoryManager final : public llvm::RTDyldMemoryManager {
public:
  LLVMMemoryManager();
  ~LLVMMemoryManager();
//  uint64_t getSymbolAddress(const std::string &Name) override;

  llvm::JITSymbol findSymbol(const std::string &Name) override;

  uint8_t *allocateCodeSection(uintptr_t Size, unsigned Alignment,
                               unsigned SectionID,
                               llvm::StringRef SectionName) override;

  uint8_t *allocateDataSection(uintptr_t Size, unsigned Alignment,
                               unsigned SectionID,
                               llvm::StringRef SectionName,
                               bool IsReadOnly) override;

  bool finalizeMemory(std::string *ErrMsg) override;

  size_t GetLastCodeAllocation() { return LastCodeSize; }

private:
  constexpr static size_t CODE_SIZE = 128 * 1024 * 1024;
  uintptr_t CodeMemory {};
  size_t AllocateOffset {};

  size_t LastCodeSize {};
};
}
