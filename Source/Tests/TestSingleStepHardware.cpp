#include "Common/ArgumentLoader.h"
#include "Common/EnvironmentLoader.h"
#include "Common/Config.h"
#include "ELFLoader.h"
#include "HarnessHelpers.h"
#include "LogManager.h"

#include <FEXCore/Core/CodeLoader.h>
#include <cstdint>
#include <string>
#include <vector>
#include <sys/mman.h>


namespace FEX {
uint8_t data[] = {
  0xb8, 0x00, 0x00, 0x80, 0x3f, // mov eax, 0x3f800000
  0x0f, 0xc7, 0xf8, // rdseed eax
  0xba, 0x00, 0x00, 0x00, 0xe0, // mov edx, 0xe0000000
  0x89, 0x02, // mov dword [edx], eax
	0xC3, // RET
};

  class TestCodeLoader final : public FEXCore::CodeLoader {
    static constexpr uint32_t PAGE_SIZE = 4096;
  public:

    TestCodeLoader() = default;

    uint64_t StackSize() const override {
      return STACK_SIZE;
    }

    uint64_t SetupStack([[maybe_unused]] void *HostPtr, uint64_t GuestPtr) const override {
      return GuestPtr + STACK_SIZE - 8;
    }

    uint64_t DefaultRIP() const override {
      return RIP;
    }

    void MapMemoryRegion(std::function<void*(uint64_t, uint64_t, bool, bool)> Mapper) override {
      // XXX: Pull this from the config
      Mapper(0xe000'0000, 0x1000'0000, true, true);
      Mapper(0x2'0000'0000, 0x1'0000'1000, true, true);
    }

    void LoadMemory(MemoryWriter Writer) override {
      Writer(data, DefaultRIP(), sizeof(data));
    }

  private:
    constexpr static uint64_t STACK_SIZE = PAGE_SIZE;
    // Zero is special case to know when we are done
    constexpr static uint64_t RIP = 0x1;
  };
}

void MsgHandler(LogMan::DebugLevels Level, char const *Message) {
  const char *CharLevel{nullptr};

  switch (Level) {
  case LogMan::NONE:
    CharLevel = "NONE";
    break;
  case LogMan::ASSERT:
    CharLevel = "ASSERT";
    break;
  case LogMan::ERROR:
    CharLevel = "ERROR";
    break;
  case LogMan::DEBUG:
    CharLevel = "DEBUG";
    break;
  case LogMan::INFO:
    CharLevel = "Info";
    break;
  default:
    CharLevel = "???";
    break;
  }
  printf("[%s] %s\n", CharLevel, Message);
}

void AssertHandler(char const *Message) {
  printf("[ASSERT] %s\n", Message);
}

int main(int argc, char **argv, char **const envp) {
  LogMan::Throw::InstallHandler(AssertHandler);
  LogMan::Msg::InstallHandler(MsgHandler);
  FEX::Config::Init();
  FEX::EnvLoader::Load(envp);
  FEX::ArgLoader::Load(argc, argv);

  FEX::Config::Value<uint8_t> CoreConfig{"Core", 0};
  auto Args = FEX::ArgLoader::Get();

//  FEX::Core localCore {static_cast<FEX::CPUCore::CPUCoreType>(CoreConfig())};
//  FEX::TestCodeLoader Loader{};
//  bool Result = localCore.Load(&Loader);
//
//  auto Base = localCore.GetCPU()->MemoryMapper->GetPointer<void*>(0);
//  localCore.RunLoop(true);
//  printf("Managed to load? %s\n", Result ? "Yes" : "No");

  FEX::Config::Shutdown();
  return 0;
}
