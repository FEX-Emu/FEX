#pragma once
#include "Common/FEXServerClient.h"
#include "CoreFileWriter/CoreFileWriter.h"
#include "Unwind/FileMapping.h"

#include <FEXCore/fextl/map.h>
#include <FEXCore/fextl/list.h>
#include <FEXCore/fextl/string.h>
#include <FEXCore/fextl/unordered_map.h>
#include <FEXCore/fextl/vector.h>

#include <cstring>
#include <linux/limits.h>
#include <poll.h>
#include <sys/socket.h>
#include <thread>

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


      // Initializes this CoreDumpClass's execution thread and associated sockets.
      // Returns one socket of the socketpair for the paired connection.
      int InitExecutionThread() {
        int SVs[2];
        int Result = socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, SVs);
        if (Result == -1) {
          return -1;
        }

        PollFD = {
          .fd = SVs[0],
          .events = POLLIN | POLLHUP | POLLERR | POLLNVAL | POLLREMOVE | POLLRDHUP,
          .revents = 0,
        };

        ExecutionThread = std::thread(&CoreDumpClass::ExecutionFunc, this);

        ServerSocket = SVs[0];

        // Return the second socket FD
        return SVs[1];
      }
      ~CoreDumpClass() {
        ExecutionThread.join();
        close(ServerSocket);
      }

      void ExecutionFunc();

      bool IsDone() {
        return Running == false;
      }

      fextl::string const &GetCommandLineString() const {
        return CommandLineString;
      }

      Description const &GetDescription() const {
        return Desc;
      }

      FileMapping::FileMapping* FindFileMapping(const fextl::string &Path) {
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

      fextl::list<FileMapping::FileMapping> &GetFileMappingList() {
        return FileMappings;
      }

    private:
      // Parses the command line in `/proc/cmdline` format.
      // Arguments separated by '\0', wrapping each argument with spaces in double-quotes for improved readability.
      void ParseCommandLineFromFD(int FD) {
        char Tmp[PATH_MAX];
        ssize_t Size = pread(FD, Tmp, PATH_MAX, 0);
        if (Size != -1) {
          size_t Offset = 0;
          while (Offset < Size) {
            std::string_view Arg = &Tmp[Offset];
            if (Arg.empty()) {
              break;
            }
            bool HasSpaces = strchr(Arg.data(), ' ') != nullptr;

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
            Offset += Arg.size() + 1;
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
      pollfd PollFD;
      time_t RequestTimeout {10};
      std::atomic<bool> ShouldShutdown {false};
      std::atomic<bool> Running {true};

      Unwind::Unwinder *UnwindHost{};
      Unwind::Unwinder *UnwindGuest{};

      fextl::string CommandLineString{};
      Description Desc;

      FEXServerClient::CoreDump::PacketGuestContext GuestContextData;
      FEXServerClient::CoreDump::PacketHostContext HostContextData;
      fextl::unique_ptr<CoreFileWriter::CoreFileWriter> CoreWriter;
      void HandleSocketData();

      fextl::list<FileMapping::FileMapping> FileMappings;
      fextl::list<FileMapping::MemMapping> MemMappings;
      fextl::unordered_map<fextl::string, FileMapping::FileMapping*> PathToFileMap;

      FEXCore::Context::Context::JITRegionPairs ThreadDispatcher{};
      fextl::vector<FEXCore::Context::Context::JITRegionPairs> ThreadJITRegions{};
  };


  void InitCoreDumpThread();
  void Shutdown();

  int CreateCoreDumpService();
  void ShutdownFD(int FD);
}
