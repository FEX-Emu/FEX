#pragma once
#include <FEXCore/IR/IR.h>
#include <FEXCore/IR/IntrusiveIRList.h>
#include <FEXCore/IR/IREmitter.h>

#include "HarnessHelpers.h"

#include <FEXCore/Utils/LogManager.h>

#include <string>
#include <vector>

using namespace FEXCore::IR;

namespace FEX::IRLoader {
  class Loader final {
    public:
      Loader(std::string const &Filename, std::string const &ConfigFilename);

			bool IsValid() const { return ParsedCode != nullptr; }
      IREmitter* GetIREmitter() { return ParsedCode.get(); }
      uint64_t GetEntryRIP() const { return EntryRIP; }

      bool CompareStates(FEXCore::Core::CPUState const* State) {
        return Config.CompareStates(State, nullptr);
      }

      void LoadMemory() {
        Config.LoadMemory();
      }

      void MapRegions() {
        for (auto& [region, size] : Config.GetMemoryRegions()) {
          mmap(reinterpret_cast<void*>(region), size, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        }
      }

    private:
      uint64_t EntryRIP{};
      std::unique_ptr<IREmitter> ParsedCode;

      FEX::HarnessHelper::ConfigLoader Config;
  };
}
