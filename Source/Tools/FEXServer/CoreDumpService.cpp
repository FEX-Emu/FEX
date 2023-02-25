#include "CoreDumpService.h"
#include "ProcessPipe.h"
#include "Unwind/Unwind_x86.h"
#include "Unwind/Unwind_x86_64.h"
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

namespace CoreDumpService {
  int ReceiveFDPacket(int ServerSocket) {
    // Wait for success response with SCM_RIGHTS
    FEXServerClient::CoreDump::PacketHeader Res{};
    struct iovec iov {
      .iov_base = &Res,
      .iov_len = sizeof(Res),
    };

    struct msghdr msg {
      .msg_name = nullptr,
      .msg_namelen = 0,
      .msg_iov = &iov,
      .msg_iovlen = 1,
    };

    // Setup the ancillary buffer. This is where we will be getting FDs
    // We only need 4 bytes for the FD
    constexpr size_t CMSG_SIZE = CMSG_SPACE(sizeof(int));
    union AncillaryBuffer {
      struct cmsghdr Header;
      uint8_t Buffer[CMSG_SIZE];
    };
    AncillaryBuffer AncBuf{};

    // Now link to our ancilllary buffer
    msg.msg_control = AncBuf.Buffer;
    msg.msg_controllen = CMSG_SIZE;

    ssize_t DataResult = recvmsg(ServerSocket, &msg, 0);
    if (DataResult > 0) {
      // Now that we have the data, we can extract the FD from the ancillary buffer
      struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);

      // Do some error checking
      if (cmsg == nullptr ||
          cmsg->cmsg_len != CMSG_LEN(sizeof(int)) ||
          cmsg->cmsg_level != SOL_SOCKET ||
          cmsg->cmsg_type != SCM_RIGHTS) {
        // Couldn't get a socket
      }
      else {
        // Check for Success.
        // If type error was returned then the FEXServer doesn't have a log to pipe in to
        // Now that we know the cmsg is sane, read the FD
        int NewFD{};
        memcpy(&NewFD, CMSG_DATA(cmsg), sizeof(NewFD));
        return NewFD;
      }
    }
    return -1;
  }

  template<typename T>
  int ReceivePacket(int ServerSocket, T &Msg, size_t Size) {
    struct iovec iov {
      .iov_base = &Msg,
      .iov_len = Size,
    };

    struct msghdr msg {
      .msg_name = nullptr,
      .msg_namelen = 0,
      .msg_iov = &iov,
      .msg_iovlen = 1,
    };

    ssize_t Read = recvmsg(ServerSocket, &msg, 0);
    return Read == iov.iov_len;
  }

  template<typename T>
  ssize_t SendPacket(int Socket, T &Msg, size_t size) {
    struct iovec iov {
      .iov_base = &Msg,
      .iov_len = size,
    };

    struct msghdr msg {
      .msg_name = nullptr,
      .msg_namelen = 0,
      .msg_iov = &iov,
      .msg_iovlen = 1,
    };

    return sendmsg(Socket, &msg, 0);
  }

  template<typename T>
  ssize_t SendPacketWithData(int Socket, T &Msg, size_t size, std::span<const char> Data) {
    struct iovec iov[2] = {
      {
        .iov_base = &Msg,
        .iov_len = size,
      },
      {
        .iov_base = const_cast<char*>(Data.data()),
        .iov_len = Data.size(),
      }
    };

    struct msghdr msg {
      .msg_name = nullptr,
      .msg_namelen = 0,
      .msg_iov = iov,
      .msg_iovlen = 2,
    };

    return sendmsg(Socket, &msg, 0);
  }


  void SendAckPacket(int ServerSocket) {
    FEXServerClient::CoreDump::PacketHeader Msg = FEXServerClient::CoreDump::FillHeader(FEXServerClient::CoreDump::PacketTypes::ACK);

    struct iovec iov {
      .iov_base = &Msg,
      .iov_len = sizeof(Msg),
    };

    struct msghdr msg {
      .msg_name = nullptr,
      .msg_namelen = 0,
      .msg_iov = &iov,
      .msg_iovlen = 1,
    };

    sendmsg(ServerSocket, &msg, 0);
  }

  void SendShutdownPacket(int ServerSocket) {
    FEXServerClient::CoreDump::PacketHeader Msg = FEXServerClient::CoreDump::FillHeader(FEXServerClient::CoreDump::PacketTypes::CLIENT_SHUTDOWN);

    struct iovec iov {
      .iov_base = &Msg,
      .iov_len = sizeof(Msg),
    };

    struct msghdr msg {
      .msg_name = nullptr,
      .msg_namelen = 0,
      .msg_iov = &iov,
      .msg_iovlen = 1,
    };

    sendmsg(ServerSocket, &msg, 0);
  }

  uint64_t PeekMemory(int ServerSocket, uint64_t Addr, uint32_t Size) {
    FEXServerClient::CoreDump::PacketPeekMem Msg = FEXServerClient::CoreDump::PacketPeekMem::Fill(Addr, Size);
    SendPacket(ServerSocket, Msg, sizeof(Msg));
    FEXServerClient::CoreDump::PacketPeekMemResponse MsgResponse;
    if (ReceivePacket(ServerSocket, MsgResponse, sizeof(MsgResponse))) {
      return MsgResponse.Data;
    }
    return 0;
  }

  int GetFDFromClient(int ServerSocket, std::string const *Filename) {
    FEXServerClient::CoreDump::PacketGetFDFromFilename Msg = FEXServerClient::CoreDump::PacketGetFDFromFilename::Fill(Filename);
    SendPacketWithData(ServerSocket, Msg, sizeof(Msg), std::span(Filename->data(), Filename->size()));
    return ReceiveFDPacket(ServerSocket);
  }

  void CoreDumpClass::ConsumeFileMapsFD(int FD) {
    lseek(FD, 0, SEEK_SET);
    DIR *dir = fdopendir(dup(FD));
    struct dirent *entry;
    char Tmp[512];
    FileMapping::FileMapping *CurrentMap{};
    while ((entry = readdir(dir)) != nullptr) {
      struct stat buf{};
      if (fstatat(FD, entry->d_name, &buf, 0)) {
        int LinkSize = readlinkat(FD, entry->d_name, Tmp, 512);
        std::string_view ReadlinkView;
        if (LinkSize > 0) {
          ReadlinkView = std::string_view(Tmp, LinkSize);
          uint64_t Begin, End;
          if (sscanf(entry->d_name, "%lx-%lx", &Begin, &End) == 2) {
            if (CurrentMap && CurrentMap->Path == ReadlinkView) {
              // Extending file mapping
              CurrentMap->End = End;
            }
            else {
              // Start a new file mapping
              CurrentMap = &FileMappings.emplace_back(FileMapping::FileMapping{Begin, End, std::string(ReadlinkView)});
              PathToFileMap.emplace(CurrentMap->Path, CurrentMap);
            }
          }
        }
        else {
          ReadlinkView = "";
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

    char TimeBuffer[128];
    time_t TimeStamp = Desc.Timestamp;
    struct tm tm;
    localtime_r(&TimeStamp, &tm);
    strftime(TimeBuffer, sizeof(TimeBuffer), "%a %Y-%m-%d %H:%M:%S %Z", &tm);
    fmt::print(stderr, "  Timestamp: {}\n", TimeBuffer);
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
          CrashInThisRegion ? " *" : "");
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
    std::vector<uint8_t> Data(1500);
    size_t CurrentRead = SocketUtil::ReadDataFromSocket(ServerSocket, Data);
    size_t CurrentOffset{};
    while (CurrentOffset < CurrentRead) {
      FEXServerClient::CoreDump::PacketHeader *Req = reinterpret_cast<FEXServerClient::CoreDump::PacketHeader *>(&Data[CurrentOffset]);
      switch (Req->PacketType) {
        case FEXServerClient::CoreDump::PacketTypes::FD_COMMANDLINE: {
          // Client is waiting for acknowledgement.
          SendAckPacket(ServerSocket);
          int FD = ReceiveFDPacket(ServerSocket);

          ParseCommandLineFD(FD);
          CurrentOffset += sizeof(FEXServerClient::CoreDump::PacketHeader);
          break;
        }
        case FEXServerClient::CoreDump::PacketTypes::FD_MAPS: {
          // Client is waiting for acknowledgement.
          SendAckPacket(ServerSocket);
          int FD = ReceiveFDPacket(ServerSocket);

          ConsumeMapsFD(FD);
          CurrentOffset += sizeof(FEXServerClient::CoreDump::PacketHeader);
          break;
        }
        case FEXServerClient::CoreDump::PacketTypes::FD_MAP_FILES: {
          // Client is waiting for acknowledgement.
          SendAckPacket(ServerSocket);
          int FD = ReceiveFDPacket(ServerSocket);

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

          UnwindHost = Unwind::x86_64::Unwind(this, &HostContextData.siginfo, &HostContextData.context);

          auto PeekMem = [&](uint64_t Addr, uint8_t Size) -> uint64_t {
            return PeekMemory(ServerSocket, Addr, Size);
          };
          auto GetFD = [&](std::string const *Filename) -> int {
            return GetFDFromClient(ServerSocket, Filename);
          };

          UnwindHost->SetPeekMemory(PeekMem);
          UnwindHost->SetGetFileFD(GetFD);
          CurrentOffset += sizeof(*Req);
          break;
        }
        case FEXServerClient::CoreDump::PacketTypes::GUEST_CONTEXT: {
          FEXServerClient::CoreDump::PacketGuestContext *Req = reinterpret_cast<FEXServerClient::CoreDump::PacketGuestContext *>(&Data[CurrentOffset]);
          Desc.GuestArch = Req->GuestArch;
          if (Desc.GuestArch == 0) {
            UnwindGuest = Unwind::x86::Unwind(this, &Req->siginfo, &Req->context);
          }
          else {
            UnwindGuest = Unwind::x86_64::Unwind(this, &Req->siginfo, &Req->context);
          }

          auto PeekMem = [&](uint64_t Addr, uint32_t Size) -> uint64_t {
            return PeekMemory(ServerSocket, Addr, Size);
          };
          auto GetFD = [&](std::string const *Filename) -> int {
            return GetFDFromClient(ServerSocket, Filename);
          };
          UnwindGuest->SetPeekMemory(PeekMem);
          UnwindGuest->SetGetFileFD(GetFD);

          CurrentOffset += sizeof(*Req);
          break;
        }
        case FEXServerClient::CoreDump::PacketTypes::CONTEXT_UNWIND: {
          // Print the header to the backtrace.
          BacktraceHeader();
          if (UnwindGuest) {
            UnwindGuest->Backtrace();
          }
          // Shutdown the client once FEXServer has unwound.
          SendShutdownPacket(ServerSocket);

          // Wait a moment for the client to sanely clean-up.
          // Then just leave if it hung regardless.
          std::this_thread::sleep_for(std::chrono::seconds(1000));
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

      if (PollFDs.empty()) {
        ShouldShutdown = true;
      }
      else {
        int Result = ppoll(&PollFDs.at(0), PollFDs.size(), &ts, nullptr);
        if (Result > 0) {
          // Walk the FDs and see if we got any results
          for (auto it = PollFDs.begin(); it != PollFDs.end(); ) {
            auto &Event = *it;
            bool Erase{};

            if (Event.revents != 0) {
              if (Event.fd == ServerSocket) {
                //LogMan::Msg::DFmt("Event: 0x{:x}", Event.revents);
                if (Event.revents & POLLIN) {
                  HandleSocketData();
                }

                // Handle POLLIN and other events at the same time.
                if (Event.revents & (POLLHUP | POLLERR | POLLNVAL | POLLREMOVE | POLLRDHUP)) {
                  // Listen socket error or shutting down
                  Erase = true;
                }
              }

              Event.revents = 0;
              --Result;
            }

            if (Erase) {
              it = PollFDs.erase(it);
            }
            else {
              ++it;
            }

            if (Result == 0) {
              // Early break if we've consumed all the results
              break;
            }
          }
        }
        else {
          auto Now = std::chrono::system_clock::now();
          auto Diff = Now - LastDataTime;
          if (Diff >= std::chrono::seconds(RequestTimeout) &&
              PollFDs.size() < 1) {
            // If we have no connections after a timeout
            // Then we can just go ahead and leave
            ShouldShutdown = true;
          }
        }
      }
    }

    // Close the socket on this side
    shutdown(ServerSocket, SHUT_RDWR);
    close(ServerSocket);

    Running = false;
  }

  std::mutex InsertMutex{};
  std::vector<std::unique_ptr<CoreDumpClass>> CoreDumpObjects;
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
      auto it = &CoreDumpObjects.emplace_back(std::make_unique<CoreDumpClass>());
      CoreDump = it->get();
    }
    return CoreDump->Init();
  }

  void ShutdownFD(int FD) {
    // Only close the socket FD.
    close(FD);
  }
}
