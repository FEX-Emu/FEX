#include "LogManager.h"
#include "Common/MathUtils.h"
#include "Interface/Core/LLVMJIT/LLVMMemoryManager.h"

#include <sys/mman.h>

namespace FEXCore::CPU {

LLVMMemoryManager::LLVMMemoryManager() {
  CodeMemory = reinterpret_cast<uintptr_t>(mmap(nullptr, CODE_SIZE, PROT_EXEC | PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
  LogMan::Throw::A(CodeMemory != -1ULL, "Failed to allocate code memory");
}

LLVMMemoryManager::~LLVMMemoryManager() {
  munmap(reinterpret_cast<void*>(CodeMemory), CODE_SIZE);
  CodeMemory = 0;
}

llvm::JITSymbol LLVMMemoryManager::findSymbol(const std::string &Name) {
  return llvm::JITSymbol(getSymbolAddress(Name), llvm::JITSymbolFlags::Exported);
}

uint8_t *LLVMMemoryManager::allocateCodeSection(uintptr_t Size, unsigned Alignment,
                             [[maybe_unused]] unsigned SectionID,
                             [[maybe_unused]] llvm::StringRef SectionName) {
  size_t Base = AlignUp(AllocateOffset, Alignment);
  size_t NewEnd = Base + Size;

  if (NewEnd >= CODE_SIZE) {
    LogMan::Msg::A("Tried allocating code and code cache is full!");
    return nullptr;
  }

  AllocateOffset = NewEnd;
  LastCodeSize = Size;
  return reinterpret_cast<uint8_t*>(CodeMemory + Base);
}

uint8_t *LLVMMemoryManager::allocateDataSection(uintptr_t Size, unsigned Alignment,
                             [[maybe_unused]] unsigned SectionID,
                             [[maybe_unused]] llvm::StringRef SectionName,
                             [[maybe_unused]] bool IsReadOnly) {

  // Put data section right after code section
  size_t Base = AlignUp(AllocateOffset, Alignment);
  size_t NewEnd = Base + Size;

  if (NewEnd >= CODE_SIZE) {
    LogMan::Msg::A("Tried allocating code and code cache is full!");
    return nullptr;
  }

  AllocateOffset = NewEnd;
  return reinterpret_cast<uint8_t*>(CodeMemory + Base);
}

bool LLVMMemoryManager::finalizeMemory([[maybe_unused]] std::string *ErrMsg) {
  return true;
}

}

