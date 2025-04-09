// SPDX-License-Identifier: MIT
#include "Common/CPUInfo.h"

#include "ELFCodeLoader.h"
#include "Linux/Utils/ELFContainer.h"

#include <FEXCore/Core/Context.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/fextl/queue.h>
#include <FEXCore/fextl/set.h>
#include <FEXCore/fextl/vector.h>
#include <FEXHeaderUtils/Syscalls.h>

#include <cstddef>
#include <sys/resource.h>
#include <sys/sysinfo.h>
#include <thread>

namespace FEX::AOT {
void AOTGenSection(FEXCore::Context::Context* CTX, ELFCodeLoader::LoadedSection& Section) {
  // Make sure this section is executable and big enough
  if (!Section.Executable || Section.Size < 16) {
    return;
  }

  fextl::set<uintptr_t> InitialBranchTargets;

  // Load the ELF again with symbol parsing this time
  ELFLoader::ELFContainer container {Section.Filename, "", true};

  // Add symbols to the branch targets list
  container.AddSymbols([&](ELFLoader::ELFSymbol* sym) {
    auto Destination = sym->Address + Section.ElfBase;

    if (!(Destination >= Section.Base && Destination <= (Section.Base + Section.Size))) {
      return; // outside of current section, unlikely to be real code
    }

    InitialBranchTargets.insert(Destination);
  });

  LogMan::Msg::IFmt("Symbol seed: {}", InitialBranchTargets.size());

  // Add unwind entries to the branch target list
  container.AddUnwindEntries([&](uintptr_t Entry) {
    auto Destination = Entry + Section.ElfBase;

    if (!(Destination >= Section.Base && Destination <= (Section.Base + Section.Size))) {
      return; // outside of current section, unlikely to be real code
    }

    InitialBranchTargets.insert(Destination);
  });

  LogMan::Msg::IFmt("Symbol + Unwind seed: {}", InitialBranchTargets.size());

  // Scan the executable section and try to find function entries
  for (size_t Offset = 0; Offset < (Section.Size - 16); Offset++) {
    uint8_t* pCode = (uint8_t*)(Section.Base + Offset);

    // Possible CALL <disp32>
    if (*pCode == 0xE8) {
      uintptr_t Destination = (int)(pCode[1] | (pCode[2] << 8) | (pCode[3] << 16) | (pCode[4] << 24));
      Destination += (uintptr_t)pCode + 5;

      auto DestinationPtr = (uint8_t*)Destination;

      if (!(Destination >= Section.Base && Destination <= (Section.Base + Section.Size))) {
        continue; // outside of current section, unlikely to be real code
      }

      if (DestinationPtr[0] == 0 && DestinationPtr[1] == 0) {
        continue; // add al, [rax], unlikely to be real code
      }

      InitialBranchTargets.insert(Destination);
    }

    // endbr64 marker marks an indirect branch destination
    if (pCode[0] == 0xf3 && pCode[1] == 0x0f && pCode[2] == 0x1e && pCode[3] == 0xfa) {
      InitialBranchTargets.insert((uintptr_t)pCode);
    }
  }

  uint64_t SectionMaxAddress = Section.Base + Section.Size;

  fextl::set<uint64_t> Compiled;
  std::atomic<int> counter = 0;

  fextl::queue<uint64_t> BranchTargets;

  // Setup BranchTargets, Compiled sets from InitiaBranchTargets

  Compiled.insert(InitialBranchTargets.begin(), InitialBranchTargets.end());
  for (auto BranchTarget : InitialBranchTargets) {
    BranchTargets.push(BranchTarget);
  }

  InitialBranchTargets.clear();


  std::mutex QueueMutex;
  fextl::vector<std::thread> ThreadPool;

  // This code is tricky to refactor so it doesn't allocate memory through glibc.
  FEXCore::Allocator::YesIKnowImNotSupposedToUseTheGlibcAllocator glibc;
  for (int i = 0; i < FEX::CPUInfo::CalculateNumberOfCPUs(); i++) {
    std::thread thd([&BranchTargets, CTX, &counter, &Compiled, &Section, &QueueMutex, SectionMaxAddress]() {
      // Set the priority of the thread so it doesn't overwhelm the system when running in the background
      setpriority(PRIO_PROCESS, FHU::Syscalls::gettid(), 19);

      // Setup thread - Each compilation thread uses its own backing FEX thread
      auto Thread = CTX->CreateThread(0, 0);
      fextl::set<uint64_t> ExternalBranchesLocal;
      CTX->ConfigureAOTGen(Thread, &ExternalBranchesLocal, SectionMaxAddress);

      for (;;) {
        uint64_t BranchTarget;

        // Get a entrypoint to process from the queue
        QueueMutex.lock();
        if (BranchTargets.empty()) {
          QueueMutex.unlock();
          break; // no entrypoint to process - exit
        }

        BranchTarget = BranchTargets.front();
        BranchTargets.pop();
        QueueMutex.unlock();

        // Compile entrypoint
        counter++;
        CTX->CompileRIP(Thread, BranchTarget);

        // Are there more branches?
        if (ExternalBranchesLocal.size() > 0) {
          // Add them to the "to process" list
          QueueMutex.lock();
          for (auto Destination : ExternalBranchesLocal) {
            if (!(Destination >= Section.Base && Destination <= (Section.Base + Section.Size))) {
              continue;
            }
            if (Compiled.contains(Destination)) {
              continue;
            }
            Compiled.insert(Destination);
            BranchTargets.push(Destination);
          }
          QueueMutex.unlock();
          ExternalBranchesLocal.clear();
        }
      }

      // All entryproints processed, cleanup this thread
      CTX->DestroyThread(Thread);
      // This thread is now getting abandoned. Disable glibc allocator checking so glibc can safely cleanup its internal allocations.
      FEXCore::Allocator::YesIKnowImNotSupposedToUseTheGlibcAllocator::HardDisable();
    });

    // Add to the thread pool
    ThreadPool.push_back(std::move(thd));
  }

  // Make sure all threads are finished
  for (auto& Thread : ThreadPool) {
    Thread.join();
  }

  ThreadPool.clear();

  LogMan::Msg::IFmt("\nAll Done: {}", counter.load());
}
} // namespace FEX::AOT
