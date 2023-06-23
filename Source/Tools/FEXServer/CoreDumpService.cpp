#include "CoreDumpService.h"
#include "ProcessPipe.h"
#include "Common/FEXServerClient.h"

#include "Common/SocketUtil.h"

#include <FEXCore/Utils/LogManager.h>

#include <dirent.h>
#include <mutex>
#include <poll.h>
#include <pwd.h>
#include <span>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <vector>

#include <fmt/chrono.h>

namespace CoreDumpService {
  void CoreDumpClass::ConsumeFileMapsFD(int FD) {
    lseek(FD, 0, SEEK_SET);
    DIR *dir = fdopendir(dup(FD));
    struct dirent *entry;
    char Tmp[PATH_MAX];
    FileMapping::FileMapping *CurrentMap{};
    while ((entry = readdir(dir)) != nullptr) {
      struct stat buf{};
      if (fstatat(FD, entry->d_name, &buf, 0)) {
        int LinkSize = readlinkat(FD, entry->d_name, Tmp, sizeof(Tmp));
        if (LinkSize > 0) {
          auto ReadlinkView = std::string_view(Tmp, LinkSize);
          uint64_t Begin, End;
          if (sscanf(entry->d_name, "%lx-%lx", &Begin, &End) == 2) {
            if (CurrentMap && CurrentMap->Path == ReadlinkView) {
              // Extending file mapping
              CurrentMap->End = End;
            }
            else {
              // Start a new file mapping
              CurrentMap = &FileMappings.emplace_back(FileMapping::FileMapping{Begin, End, fextl::string(ReadlinkView)});
              PathToFileMap.emplace(CurrentMap->Path, CurrentMap);
            }
          }
        }
      }
    }
    closedir(dir);
  }

  void CoreDumpClass::ConsumeMapsFD(int FD) {
    lseek(FD, 0, SEEK_SET);
    FILE *fp = fdopen(dup(FD), "rb");
    char Path[1024];
    while (fgets(Path, sizeof(Path), fp) != nullptr) {
      uint64_t Begin, End;
      char R, W, X, P;
      if (size_t Read = sscanf(Path, "%lx-%lx %c%c%c%c %*lx %*x:%*x %*x %s", &Begin, &End, &R, &W, &X, &P, Path)) {
        auto FileMapping = FindFileMapping(Path);

        auto Mapping = &MemMappings.emplace_back(FileMapping::MemMapping {
          .Begin = Begin,
          .End = End,
          .permissions =
            (R == 'r' ? (1U << 3) : 0) |
            (W == 'w' ? (1U << 2) : 0) |
            (X == 'x' ? (1U << 1) : 0) |
            (P == 'p' ? (1U << 0) : 0),
          .FileMapping = FileMapping,
          .Path = Read == 7 ? Path : "",
        });

        if (FileMapping) {
          FileMapping->MemMappings.emplace_back(Mapping);
        }
      }
    }
    fclose(fp);
  }

  void CoreDumpClass::BacktraceHeader() {
    auto Desc = GetDescription();
    fmt::print(stderr, "        PID: {}\n", Desc.pid);
    fmt::print(stderr, "        TID: {}\n", Desc.tid);
    fmt::print(stderr, "        UID: {} ({})\n", Desc.uid, getpwuid(Desc.uid)->pw_name);
    fmt::print(stderr, "        GID: {} ({})\n", Desc.gid, getpwuid(Desc.gid)->pw_name);
    fmt::print(stderr, "     Signal: {} ({})\n", Desc.Signal, strsignal(Desc.Signal));
    fmt::print(stderr, "  Host Arch: {}\n",
      Desc.HostArch == 1 ? "x86_64" :
      Desc.HostArch == 2 ? "AArch64" :
      "<Unknown>");
    fmt::print(stderr, " Guest Arch: {}\n",
      Desc.GuestArch == 1 ? "x86_64" :
      Desc.GuestArch == 0 ? "x86" :
      "<Unknown>");

    fmt::print(stderr, "  Timestamp: {:%a %Y-%m-%d %H:%M:%S %Z}\n", fmt::localtime(Desc.Timestamp));
    fmt::print(stderr, "CommandLine: {}\n", GetCommandLineString());

#ifdef _M_ARM_64
    uint64_t HostPC = HostContextData.context.pc;
#else
    uint64_t HostPC = HostContextData.context.gregs[REG_RIP];
#endif
    fmt::print(stderr, "Host Program counter: 0x{:x}\n", HostPC);

    bool CrashInDispatcher{};
    bool CrashInJITRegion{};
    if (ThreadDispatcher.Size) {
      if (HostPC >= ThreadDispatcher.Base &&
          HostPC < (ThreadDispatcher.Base + ThreadDispatcher.Size)) {
        CrashInDispatcher = true;
      }

      fmt::print(stderr, "Thread's Dispatcher: [0x{:x}, 0x{:x}) -> 0x{:x} bytes{}\n", ThreadDispatcher.Base, ThreadDispatcher.Base + ThreadDispatcher.Size, ThreadDispatcher.Size,
        CrashInDispatcher ? " *" : "");
    }

    if (ThreadJITRegions.size()) {
      fmt::print(stderr, "Thread's JIT regions: {}\n", ThreadJITRegions.size());
      for (size_t i = 0; i < ThreadJITRegions.size(); ++i) {
        auto const &JITRegion = ThreadJITRegions[i];
        bool CrashInThisRegion{};
        if (HostPC >= JITRegion.Base &&
            HostPC <= (JITRegion.Base + JITRegion.Size)) {
          CrashInJITRegion = true;
          CrashInThisRegion = true;
        }

        fmt::print(stderr, "  JIT regions {}: [0x{:x}, 0x{:x}) -> 0x{:x} bytes{}\n", i, JITRegion.Base, JITRegion.Base + JITRegion.Size, JITRegion.Size,
          CrashInThisRegion ? " <- Crash in this region" : "");
      }
    }

    if (CrashInDispatcher) {
      fmt::print(stderr, "@@ Crash was inside FEX's Dispatcher. This shouldn't occur.\n");
    }
    else if (CrashInJITRegion) {
      fmt::print(stderr, "@@ Crash was inside FEX's JIT. Highly likely a guest side crash.\n");
    } else {
      fmt::print(stderr, "@@ Crash was outside of FEX's JIT. Highly likely a host side crash. Please upload a coredump.\n");
    }

    fmt::print(stderr, "\n");
    for (auto const &FileMapping : GetFileMappingList()) {
      fmt::print(stderr, "                Found module {}\n", FileMapping.Path);
    }
  }

  void CoreDumpClass::HandleSocketData() {
    fextl::vector<uint8_t> Data(1500);
    size_t CurrentRead = SocketUtil::ReadDataFromSocket(ServerSocket, Data);
    size_t CurrentOffset{};
    while (CurrentOffset < CurrentRead) {
      FEXServerClient::CoreDump::PacketHeader *Req = reinterpret_cast<FEXServerClient::CoreDump::PacketHeader *>(&Data[CurrentOffset]);
      switch (Req->PacketType) {
        case FEXServerClient::CoreDump::PacketTypes::FD_COMMANDLINE: {
          // Client is waiting for acknowledgement.
          FEXServerClient::CoreDump::SendAckPacket(ServerSocket);
          int FD = FEXServerClient::CoreDump::HandleFDPacket(ServerSocket);

          ParseCommandLineFromFD(FD);
          CurrentOffset += sizeof(FEXServerClient::CoreDump::PacketHeader);
          break;
        }
        case FEXServerClient::CoreDump::PacketTypes::FD_MAPS: {
          // Client is waiting for acknowledgement.
          FEXServerClient::CoreDump::SendAckPacket(ServerSocket);
          int FD = FEXServerClient::CoreDump::HandleFDPacket(ServerSocket);

          ConsumeMapsFD(FD);
          CurrentOffset += sizeof(FEXServerClient::CoreDump::PacketHeader);
          break;
        }
        case FEXServerClient::CoreDump::PacketTypes::FD_MAP_FILES: {
          // Client is waiting for acknowledgement.
          FEXServerClient::CoreDump::SendAckPacket(ServerSocket);
          int FD = FEXServerClient::CoreDump::HandleFDPacket(ServerSocket);

          ConsumeFileMapsFD(FD);
          CurrentOffset += sizeof(FEXServerClient::CoreDump::PacketHeader);
          break;
        }
        case FEXServerClient::CoreDump::PacketTypes::DESC: {
          FEXServerClient::CoreDump::PacketDescription *Req = reinterpret_cast<FEXServerClient::CoreDump::PacketDescription *>(&Data[CurrentOffset]);
          SetDesc(Req->pid,
            Req->tid,
            Req->uid,
            Req->gid,
            Req->Signal,
            Req->Timestamp,
            Req->HostArch,
            Req->GuestArch);

          CurrentOffset += sizeof(*Req);
          break;
        }
        case FEXServerClient::CoreDump::PacketTypes::HOST_CONTEXT: {
          FEXServerClient::CoreDump::PacketHostContext *Req = reinterpret_cast<FEXServerClient::CoreDump::PacketHostContext *>(&Data[CurrentOffset]);
          HostContextData = *Req;
          // TODO: Create host context unwinder.
          CurrentOffset += sizeof(*Req);
          break;
        }
        case FEXServerClient::CoreDump::PacketTypes::GUEST_CONTEXT: {
          FEXServerClient::CoreDump::PacketGuestContext *Req = reinterpret_cast<FEXServerClient::CoreDump::PacketGuestContext *>(&Data[CurrentOffset]);
          Desc.GuestArch = Req->GuestArch;
          // TODO: Create guest context unwinder.
          CurrentOffset += sizeof(*Req);
          break;
        }
        case FEXServerClient::CoreDump::PacketTypes::CONTEXT_UNWIND: {
          // Print the header to the backtrace.
          BacktraceHeader();
          // TODO: Unwind the host and guest contexts that were received.
          // Shutdown the client once FEXServer has unwound.
          FEXServerClient::CoreDump::SendShutdownPacket(ServerSocket);

          ShouldShutdown = true;
          CurrentOffset += sizeof(FEXServerClient::CoreDump::PacketHeader);
          break;
        }
        case FEXServerClient::CoreDump::PacketTypes::GET_JIT_REGIONS: {
          FEXServerClient::CoreDump::PacketGetJITRegions *Req = reinterpret_cast<FEXServerClient::CoreDump::PacketGetJITRegions *>(&Data[CurrentOffset]);
          ThreadDispatcher = Req->Dispatcher;
          for (size_t i = 0; i < Req->NumJITRegions; ++i) {
            ThreadJITRegions.emplace_back(Req->JITRegions[i]);
          }
          CurrentOffset += sizeof(*Req) + Req->NumJITRegions * sizeof(FEXCore::Context::Context::JITRegionPairs);
          break;
        }
        default:
          // Something sent us an invalid packet. To ensure we don't spin infinitely, consume all the data.
          LogMan::Msg::EFmt("[CoreDumpClass] InvalidPacket size received 0x{:x} bytes", CurrentRead - CurrentOffset);
          CurrentOffset = CurrentRead;
          break;
      }
    }
  }

  void CoreDumpClass::ExecutionFunc() {
    auto LastDataTime = std::chrono::system_clock::now();

    while (!ShouldShutdown) {
      struct timespec ts{};
      ts.tv_sec = RequestTimeout;

      int Result = ppoll(&PollFD, 1, &ts, nullptr);
      if (Result > 0) {
        // Walk the FDs and see if we got any results
        if (PollFD.revents != 0) {
          //LogMan::Msg::DFmt("Event: 0x{:x}", Event.revents);
          if (PollFD.revents & POLLIN) {
            HandleSocketData();
          }

          // Handle POLLIN and other events at the same time.
          if (PollFD.revents & (POLLHUP | POLLERR | POLLNVAL | POLLREMOVE | POLLRDHUP)) {
            // Listen socket error or shutting down
            ShouldShutdown = true;
          }

          // Reset the revents for the next query.
          PollFD.revents = 0;
        }
      }
      else {
        auto Now = std::chrono::system_clock::now();
        auto Diff = Now - LastDataTime;
        if (Diff >= std::chrono::seconds(RequestTimeout)) {
          // If we have no data after a timeout
          // Then we can just go ahead and leave
          ShouldShutdown = true;
        }
      }
    }

    // Close the socket on this side
    shutdown(ServerSocket, SHUT_RDWR);
    close(ServerSocket);

    Running = false;
  }

  std::mutex InsertMutex{};
  fextl::vector<fextl::unique_ptr<CoreDumpClass>> CoreDumpObjects;
  std::thread CoreDumpTracker{};
  std::atomic<bool> ShouldShutdown;
  void CoreDumpTrackerThread() {
    while (!ShouldShutdown) {
      std::this_thread::sleep_for(std::chrono::seconds(5));

      {
        std::unique_lock lk{InsertMutex};
        for (auto it = CoreDumpObjects.begin(); it != CoreDumpObjects.end(); ) {
          if (it->get()->IsDone()) {
            it = CoreDumpObjects.erase(it);
          }
          else {
            ++it;
          }
        }
      }
    }
  }

  void InitCoreDumpThread() {
    if (!CoreDumpTracker.joinable()) {
      CoreDumpTracker = std::thread(CoreDumpTrackerThread);
    }
  }

  void Shutdown() {
    ShouldShutdown = true;
    if (CoreDumpTracker.joinable()) {
      CoreDumpTracker.join();
    }
  }

  int CreateCoreDumpService() {
    CoreDumpService::InitCoreDumpThread();

    CoreDumpClass *CoreDump{};
    {
      std::unique_lock lk{InsertMutex};
      auto it = &CoreDumpObjects.emplace_back(fextl::make_unique<CoreDumpClass>());
      CoreDump = it->get();
    }
    return CoreDump->InitExecutionThread();
  }

  void ShutdownFD(int FD) {
    // Only close the socket FD.
    close(FD);
  }
}
