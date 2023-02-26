#pragma once
#include "FDCountWatch.h"
#include "Common/FEXServerClient.h"
#include "Unwind/FileMapping.h"

#include <cstring>
#include <poll.h>
#include <map>
#include <list>
#include <sys/socket.h>
#include <thread>
#include <vector>

namespace Unwind {
class Unwinder;
}

namespace CoreDumpService {
  class CoreDumpClass {
    public:
      struct Description {
        uint32_t pid;
        uint32_t tid;
        uint32_t uid;
        uint32_t gid;
        uint32_t Signal;
        uint64_t Timestamp;
        uint8_t HostArch;
        uint8_t GuestArch;
      };


      int Init() {
        int SVs[2];
        int Result = socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, SVs);
        if (Result == -1) {
          return -1;
        }

        FDCountWatch::IncrementFDCountAndCheckLimits(2);

        PollFDs.emplace_back(pollfd {
          .fd = SVs[0],
          .events = POLLIN | POLLHUP | POLLERR | POLLNVAL | POLLREMOVE | POLLRDHUP,
          .revents = 0,
        });

        ExecutionThread = std::thread(&CoreDumpClass::ExecutionFunc, this);

        ServerSocket = SVs[0];

        // Return the second socket FD
        return SVs[1];
      }
      ~CoreDumpClass() {
        ExecutionThread.join();
        close(ServerSocket);
        for (auto FD : TrackedFDs) {
          close(FD);
        }

        FDCountWatch::IncrementFDCountAndCheckLimits(-TrackedFDs.size() - 1);

        TrackedFDs.clear();
      }

      void ExecutionFunc();

      bool IsDone() {
        return Running == false;
      }

      std::string const &GetCommandLineString() const {
        return CommandLineString;
      }

      Description const &GetDescription() const {
        return Desc;
      }

      FileMapping::FileMapping* FindFileMapping(const std::string &Path) {
        auto it = PathToFileMap.find(Path);
        if (it == PathToFileMap.end()) {
          return nullptr;
        }

        return it->second;
      }

      FileMapping::FileMapping* GetFileMapping(uint64_t Addr) {
        for (auto& Mapping : FileMappings) {
          if (Mapping.Begin <= Addr && Mapping.End > Addr) {
            return &Mapping;
          }
        }
        return nullptr;
      }

      FileMapping::MemMapping const* GetMapping(uint64_t Addr) {
        for (const auto& Mapping : MemMappings) {
          if (Mapping.Begin <= Addr && Mapping.End > Addr) {
            return &Mapping;
          }
        }
        return nullptr;
      }

      bool AddressHasAccess(uint64_t Addr, bool Write) {
        for (const auto& Mapping : MemMappings) {
          if (Mapping.Begin <= Addr && Mapping.End > Addr) {
            if (Write) {
              return Mapping.permissions & (1U << 2);
            }
            else {
              return Mapping.permissions & (1U << 3);
            }
          }
        }

        return false;
      }

      std::list<FileMapping::FileMapping> &GetFileMappingList() {
        return FileMappings;
      }

    private:
      void ParseCommandLineFD(int FD) {
        char Tmp[512];
        ssize_t Size = pread(FD, Tmp, 512, 0);
        if (Size != -1) {
          size_t Offset = 0;
          while (Offset < Size) {
            char *Arg = &Tmp[Offset];
            size_t Len = strlen(Arg);
            if (Len == 0) {
              break;
            }
            bool HasSpaces = strchr(Arg, ' ') != nullptr;

            if (HasSpaces) {
              CommandLineString += "\"";
            }

            CommandLineString += Arg;

            if (HasSpaces) {
              CommandLineString += "\" ";
            }
            else {
              CommandLineString += " ";
            }
            Offset += Len + 1;
          }

        }
      }

      void SetDesc(uint32_t pid, uint32_t tid, uint32_t uid, uint32_t gid, uint32_t Signal, uint64_t Timestamp, uint8_t HostArch, uint8_t GuestArch) {
        Desc = {
          .pid = pid,
          .tid = tid,
          .uid = uid,
          .gid = gid,
          .Signal = Signal,
          .Timestamp = Timestamp,
          .HostArch = HostArch,
          .GuestArch = GuestArch,
        };
      }

      void ConsumeFileMapsFD(int FD);
      void ConsumeMapsFD(int FD);

      void BacktraceHeader();

      int ServerSocket;
      std::thread ExecutionThread;
      std::vector<struct pollfd> PollFDs{};
      time_t RequestTimeout {10};
      std::atomic<bool> ShouldShutdown {false};
      std::atomic<bool> Running {true};

      Unwind::Unwinder *UnwindHost{};
      Unwind::Unwinder *UnwindGuest{};

      std::string CommandLineString{};
      Description Desc;

      FEXServerClient::CoreDump::PacketGuestContext GuestContextData;
      FEXServerClient::CoreDump::PacketHostContext HostContextData;
      void HandleSocketData();

      std::list<FileMapping::FileMapping> FileMappings;
      std::vector<FileMapping::MemMapping> MemMappings;
      std::unordered_map<std::string, FileMapping::FileMapping*> PathToFileMap;

      FEXCore::Context::Context::JITRegionPairs ThreadDispatcher{};
      std::vector<FEXCore::Context::Context::JITRegionPairs> ThreadJITRegions{};
      std::vector<int> TrackedFDs;
  };


  void InitCoreDumpThread();
  void Shutdown();

  int CreateCoreDumpService();
  void ShutdownFD(int FD);
}
