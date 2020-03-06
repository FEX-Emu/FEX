#pragma once

#include <FEXCore/Core/CodeLoader.h>

class TestCode final : public FEXCore::CodeLoader {
public:

  TestCode(uint8_t const *Code, size_t Size)
    : CodePtr {Code}
    , CodeSize {Size} {
  }

  uint64_t StackSize() const override {
    return STACK_SIZE;
  }

  uint64_t SetupStack([[maybe_unused]] void *HostPtr, uint64_t GuestPtr) const override {
    return GuestPtr + STACK_SIZE - 16;
  }

  uint64_t DefaultRIP() const override {
    return RIP;
  }

  FEXCore::CodeLoader::MemoryLayout GetLayout() const override {
    // Needs to be page aligned
    constexpr uint64_t ConstCodeSize = 0x1000;
    return std::make_tuple(CODE_START_RANGE, CODE_START_RANGE + ConstCodeSize, ConstCodeSize);
  }

  void MapMemoryRegion(std::function<void* (uint64_t, uint64_t)> Mapper) override {
  }

  void LoadMemory(MemoryWriter Writer) override {
    Writer(reinterpret_cast<void const*>(CodePtr), 0, CodeSize);
  }

  uint64_t GetFinalRIP() override { return CODE_START_RANGE + CodeSize; }

private:
  static constexpr uint64_t STACK_SIZE = 0x1000;
  static constexpr uint64_t CODE_START_RANGE = 0x0;
  static constexpr uint64_t RIP = 0;

  uint8_t const *CodePtr;
  size_t CodeSize;

};

