// SPDX-License-Identifier: MIT
#include "Common/Config.h"
#include "Common/FEXServerClient.h"
#include "Common/SocketUtil.h"

#include <FEXCore/Utils/CompilerDefs.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/NetStream.h>
#include <FEXCore/fextl/fmt.h>
#include <FEXCore/fextl/string.h>
#include <FEXCore/fextl/vector.h>
#include <FEXHeaderUtils/Filesystem.h>

#include <fcntl.h>
#include <linux/limits.h>
#include <optional>
#include <unistd.h>
#include <span>
#include <sys/poll.h>
#include <sys/prctl.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/uio.h>
#include <thread>

namespace FEXServerClient {
  ssize_t SendPacket(int Socket, void *Msg, size_t Size) {
    return send(Socket, Msg, Size, 0);
  }

  bool ReceivePacket(int ServerSocket, void *Msg, size_t Size) {
    ssize_t Read = recv(ServerSocket, Msg, Size, 0);
    return Read == Size;
  }

  ssize_t SendFDPacket(int ServerSocket, void *Msg, size_t Size, int FD) {
    struct iovec iov {
      .iov_base = Msg,
      .iov_len = Size,
    };

    struct msghdr msg {
      .msg_name = nullptr,
      .msg_namelen = 0,
      .msg_iov = &iov,
      .msg_iovlen = 1,
    };

    // Setup the ancillary buffer. This is where we will be getting pipe FDs
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

    // Now we need to setup the ancillary buffer data. We are only sending an FD
    struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_len = CMSG_LEN(sizeof(int));
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;

    // We are giving the daemon the write side of the pipe
    memcpy(CMSG_DATA(cmsg), &FD, sizeof(int));

    return sendmsg(ServerSocket, &msg, 0);
  }

  union AncillaryBuffer {
    constexpr static size_t CMSG_SIZE = CMSG_SPACE(sizeof(int));
    struct cmsghdr Header;
    uint8_t Buffer[CMSG_SIZE];
  };

  int ParseFDFromAuxBuffer(struct msghdr *msg) {
    // Now that we have the data, we can extract the FD from the ancillary buffer
    struct cmsghdr *cmsg = CMSG_FIRSTHDR(msg);

    // Do some error checking
    if (cmsg == nullptr ||
        cmsg->cmsg_len != CMSG_LEN(sizeof(int)) ||
        cmsg->cmsg_level != SOL_SOCKET ||
        cmsg->cmsg_type != SCM_RIGHTS) {
      // Couldn't get a socket
      return -1;
    }

    // Check for Success.
    // If type error was returned then the FEXServer doesn't have a log to pipe in to
    // Now that we know the cmsg is sane, read the FD
    int NewFD{};
    memcpy(&NewFD, CMSG_DATA(cmsg), sizeof(NewFD));
    return NewFD;
  }

  template<typename T>
  int ReceiveFDWithAncillary(int ServerSocket) {
    T Res{};
    struct iovec iov {
      .iov_base = &Res,
      .iov_len = sizeof(T),
    };

    struct msghdr msg {
      .msg_name = nullptr,
      .msg_namelen = 0,
      .msg_iov = &iov,
      .msg_iovlen = 1,
    };

    // Setup the ancillary buffer. This is where we will be getting pipe FDs
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

    // Wait for success response with SCM_RIGHTS
    ssize_t DataResult = recvmsg(ServerSocket, &msg, 0);
    if (DataResult > 0) {
      return ParseFDFromAuxBuffer(&msg);
    }

    return -1;
  }

  int RequestPIDFDPacket(int ServerSocket, void *Msg, size_t size) {
    int Result = write(ServerSocket, Msg, size);
    if (Result != -1) {
      return ReceiveFDWithAncillary<FEXServerResultPacket>(ServerSocket);
    }

    return -1;
  }

  static int ServerFD {-1};

  fextl::string GetServerLockFolder() {
    return FEXCore::Config::GetDataDirectory() + "Server/";
  }

  fextl::string GetServerLockFile() {
    return GetServerLockFolder() + "Server.lock";
  }

  fextl::string GetServerRootFSLockFile() {
    return GetServerLockFolder() + "RootFS.lock";
  }

  fextl::string GetServerMountFolder() {
    // We need a FEXServer mount directory that has some tricky requirements.
    // - We don't want to use `/tmp/` if possible.
    //   - systemd services use `PrivateTmp` feature to gives services their own tmp.
    //   - We will use this as a fallback path /only/.
    // - Can't be `[$XDG_DATA_HOME,$HOME]/.fex-emu/`
    //   - Might be mounted with a filesystem (sshfs) which can't handle mount points inside it.
    //
    // Directories it can be in:
    // - $XDG_RUNTIME_DIR if set
    //   - Is typically `/run/user/<UID>/`
    //   - systemd `PrivateTmp` feature doesn't touch this.
    //   - If this path doesn't exist then fallback to `/tmp/` as a last resort.
    //   - pressure-vessel explicitly creates an internal XDG_RUNTIME_DIR inside its chroot.
    //     - This is okay since pressure-vessel rbinds the FEX rootfs from the host to `/run/pressure-vessel/interpreter-root`.
    fextl::string Folder{};
    auto XDGRuntimeEnv = getenv("XDG_RUNTIME_DIR");
    if (XDGRuntimeEnv) {
      // If the XDG runtime directory works then use that.
      Folder = XDGRuntimeEnv;
    }
    else {
      // Fallback to `/tmp/` if XDG_RUNTIME_DIR doesn't exist.
      // Might not be ideal but we don't have much of a choice.
      Folder = std::filesystem::temp_directory_path().string();
    }

    if (FEXCore::Config::FindContainer() == "pressure-vessel") {
      // In pressure-vessel the mount point changes location.
      // This is due to pressure-vesssel being a chroot environment.
      // It by default maps the host-filesystem to `/run/host/` so we need to redirect.
      // After pressure-vessel is fully set up it will set the `FEX_ROOTFS` environment variable,
      // which the FEXInterpreter will pick up on.
      Folder = "/run/host/" + Folder;
    }

    return Folder;
  }

  fextl::string GetServerSocketName() {
    return fextl::fmt::format("{}.FEXServer.Socket", ::geteuid());
  }

  int GetServerFD() {
    return ServerFD;
  }

  int ConnectToServer(ConnectionOption ConnectionOption) {
    auto ServerSocketName = GetServerSocketName();

    // Create the initial unix socket
    int SocketFD = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (SocketFD == -1) {
      LogMan::Msg::EFmt("Couldn't open AF_UNIX socket {} {}", errno, strerror(errno));
      return -1;
    }

    // AF_UNIX has a special feature for named socket paths.
    // If the name of the socket begins with `\0` then it is an "abstract" socket address.
    // The entirety of the name is used as a path to a socket that doesn't have any filesystem backing.
    struct sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    size_t SizeOfSocketString = std::min(ServerSocketName.size() + 1, sizeof(addr.sun_path) - 1);
    addr.sun_path[0] = 0; // Abstract AF_UNIX sockets start with \0
    strncpy(addr.sun_path + 1, ServerSocketName.data(), SizeOfSocketString);
    // Include final null character.
    size_t SizeOfAddr = sizeof(addr.sun_family) + SizeOfSocketString;

    if (connect(SocketFD, reinterpret_cast<struct sockaddr*>(&addr), SizeOfAddr) == -1) {
      if (ConnectionOption == ConnectionOption::Default || errno != ECONNREFUSED) {
        LogMan::Msg::EFmt("Couldn't connect to FEXServer socket {} {} {}", ServerSocketName, errno, strerror(errno));
      }
      close(SocketFD);
      return -1;
    }

    return SocketFD;
  }

  bool SetupClient(char *InterpreterPath) {
    ServerFD = FEXServerClient::ConnectToAndStartServer(InterpreterPath);
    if (ServerFD == -1) {
      return false;
    }

    // If we were started in a container then we want to use the rootfs that they provided.
    // In the pressure-vessel case this is a combination of our rootfs and the steam soldier runtime.
    if (FEXCore::Config::FindContainer() != "pressure-vessel") {
      fextl::string RootFSPath = FEXServerClient::RequestRootFSPath(ServerFD);

      //// If everything has passed then we can now update the rootfs path
      FEXCore::Config::EraseSet(FEXCore::Config::CONFIG_ROOTFS, RootFSPath);
    }

    return true;
  }

  int ConnectToAndStartServer(char *InterpreterPath) {
    int ServerFD = ConnectToServer(ConnectionOption::NoPrintConnectionError);
    if (ServerFD == -1) {
      // Couldn't connect to the server. Start one

      // Open some pipes for letting us know when the server is ready
      int fds[2]{};
      if (pipe2(fds, 0) != 0) {
        LogMan::Msg::EFmt("Couldn't open pipe");
        return -1;
      }

      fextl::string FEXServerPath = FHU::Filesystem::ParentPath(InterpreterPath) + "/FEXServer";
      // Check if a local FEXServer next to FEXInterpreter exists
      // If it does then it takes priority over the installed one
      if (!FHU::Filesystem::Exists(FEXServerPath)) {
        FEXServerPath = "FEXServer";
      }

      // Set-up our SIGCHLD handler to ignore the signal.
      // This is early in the initialization stage so no handlers have been installed.
      //
      // We want to ignore the signal so that if FEXServer starts in daemon mode, it
      // doesn't leave a zombie process around waiting for something to get the result.
      struct sigaction action{};
      action.sa_handler = SIG_IGN,
      sigaction(SIGCHLD, &action, &action);

      pid_t pid = fork();
      if (pid == 0) {
        // Child
        close(fds[0]); // Close read end of pipe

        const char *argv[2];

        argv[0] = FEXServerPath.c_str();
        argv[1] = nullptr;

        if (execvp(argv[0], (char * const*)argv) == -1) {
          // Let the parent know that we couldn't execute for some reason
          uint64_t error{1};
          write(fds[1], &error, sizeof(error));

          // Give a hopefully helpful error message for users
          LogMan::Msg::EFmt("Couldn't execute: {}", argv[0]);
          LogMan::Msg::EFmt("This means the squashFS rootfs won't be mounted.");
          LogMan::Msg::EFmt("Expect errors!");
          // Destroy this fork
          exit(1);
        }

        FEX_UNREACHABLE;
      }
      else {
        // Parent
        // Wait for the child to exit so we can check if it is mounted or not
        close(fds[1]); // Close write end of the pipe

        // Wait for a message from FEXServer
        pollfd PollFD;
        PollFD.fd = fds[0];
        PollFD.events = POLLIN | POLLOUT | POLLRDHUP | POLLERR | POLLHUP | POLLNVAL;

        // Wait for a result on the pipe that isn't EINTR
        while (poll(&PollFD, 1, -1) == -1 && errno == EINTR);

        for (size_t i = 0; i < 5; ++i) {
          ServerFD = ConnectToServer(ConnectionOption::Default);

          if (ServerFD != -1) {
            break;
          }

          std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        if (ServerFD == -1) {
          // Still couldn't connect to the socket.
          LogMan::Msg::EFmt("Couldn't connect to FEXServer socket {} after launching the process", GetServerSocketName());
        }
      }

      // Restore the original SIGCHLD handler if it existed.
      sigaction(SIGCHLD, &action, nullptr);
    }
    return ServerFD;
  }

  /**
   * @name Packet request functions
   * @{ */
  void RequestServerKill(int ServerSocket) {
    FEXServerRequestPacket Req {
      .Header {
        .Type = PacketType::TYPE_KILL,
      },
    };

    write(ServerSocket, &Req, sizeof(Req.BasicRequest));
  }

  int RequestLogFD(int ServerSocket) {
    FEXServerRequestPacket Msg {
      .Header {
        .Type = PacketType::TYPE_GET_LOG_FD,
      },
    };

    return RequestPIDFDPacket(ServerSocket, &Msg, sizeof(Msg.BasicRequest));
  }

  int RequestCoredumpFD(int ServerSocket) {
    FEXServerRequestPacket Msg {
      .Header {
        .Type = PacketType::TYPE_GET_COREDUMP_FD,
      },
    };

    return RequestPIDFDPacket(ServerSocket, &Msg, sizeof(Msg.BasicRequest));
  }

  void SendEmptyErrorPacket(int ServerSocket) {
    FEXServerClient::FEXServerResultPacket Res {
      .Header {
        .Type = FEXServerClient::PacketType::TYPE_ERROR,
      },
    };
    SendPacket(ServerSocket, &Res, sizeof(Res));
  }

  fextl::string RequestRootFSPath(int ServerSocket) {
    FEXServerRequestPacket Req {
      .Header {
        .Type = PacketType::TYPE_GET_ROOTFS_PATH,
      },
    };

    int Result = write(ServerSocket, &Req, sizeof(Req.BasicRequest));
    if (Result != -1) {
      // Wait for success response with data
      fextl::vector<char> Data(PATH_MAX + sizeof(FEXServerResultPacket));

      ssize_t DataResult = recv(ServerSocket, Data.data(), Data.size(), 0);
      if (DataResult >= sizeof(FEXServerResultPacket)) {
        FEXServerResultPacket *ResultPacket = reinterpret_cast<FEXServerResultPacket*>(Data.data());
        if (ResultPacket->Header.Type == PacketType::TYPE_GET_ROOTFS_PATH &&
            ResultPacket->MountPath.Length > 0) {
          return fextl::string(ResultPacket->MountPath.Mount);
        }
      }
    }

    return {};
  }

  int RequestPIDFD(int ServerSocket) {
    FEXServerRequestPacket Msg {
      .Header {
        .Type = PacketType::TYPE_GET_PID_FD,
      },
    };

    return RequestPIDFDPacket(ServerSocket, &Msg, sizeof(Msg.BasicRequest));
  }

  /**  @} */

  /**
   * @name FEX logging through FEXServer
   * @{ */

  void MsgHandler(int FD, LogMan::DebugLevels Level, char const *Message) {
    size_t MsgLen = strlen(Message) + 1;

    Logging::PacketMsg Msg;
    Msg.Header = Logging::FillHeader(Logging::PacketTypes::TYPE_MSG);
    Msg.MessageLength = MsgLen;
    Msg.Level = Level;

    const iovec vec[2] = {
      {
        .iov_base = &Msg,
        .iov_len = sizeof(Msg),
      },
      {
        .iov_base = const_cast<char*>(Message),
        .iov_len = Msg.MessageLength,
      },
    };

    writev(FD, vec, 2);
  }

  void AssertHandler(int FD, char const *Message) {
    MsgHandler(FD, LogMan::DebugLevels::ASSERT, Message);
  }
  /**  @} */

  /**
   * @name FEX core dump through FEXServer
   * @{ */
  namespace CoreDump {
    bool WaitForAck(int Socket) {
      CoreDump::PacketHeader Msg;
      return ReceivePacket(Socket, &Msg, sizeof(Msg)) && Msg.PacketType == CoreDump::PacketTypes::ACK;
    }

    ssize_t SendPacketWithData(int Socket, void *Msg, size_t size, std::span<const char> Data) {
      struct iovec iov[2] = {
        {
          .iov_base = Msg,
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

    bool SendFD(int CoreDumpSocket, CoreDump::PacketTypes Type, int FD) {
      CoreDump::PacketHeader Msg;
      Msg = CoreDump::FillHeader(Type);
      if (SendPacket(CoreDumpSocket, &Msg, sizeof(Msg)) != -1) {
        if (WaitForAck(CoreDumpSocket)) {
          Msg = CoreDump::FillHeader(CoreDump::PacketTypes::SUCCESS);
          return SendFDPacket(CoreDumpSocket, &Msg, sizeof(Msg), FD) != -1;
        }
      }
      return false;
    }

    void SendAckPacket(int CoreDumpSocket) {
      FEXServerClient::CoreDump::PacketHeader Msg = FEXServerClient::CoreDump::FillHeader(FEXServerClient::CoreDump::PacketTypes::ACK);
      SendPacket(CoreDumpSocket, &Msg, sizeof(Msg));
    }

    ///< Client side handling.
    void SendDescPacket(int CoreDumpSocket, uint32_t Signal, uint8_t GuestArch) {
      CoreDump::PacketDescription Msg;
      Msg = CoreDump::PacketDescription::Fill(Signal, GuestArch);
      SendPacket(CoreDumpSocket, &Msg, sizeof(Msg));
    }

    void SendCommandLineFD(int CoreDumpSocket) {
      int FD = open("/proc/self/cmdline", O_RDONLY | O_CLOEXEC);
      SendFD(CoreDumpSocket, CoreDump::PacketTypes::FD_COMMANDLINE, FD);
      close(FD);
    }

    void SendMapsFD(int CoreDumpSocket) {
      int FD = open("/proc/self/maps", O_RDONLY | O_CLOEXEC);

      SendFD(CoreDumpSocket, CoreDump::PacketTypes::FD_MAPS, FD);
      close(FD);
    }

    void SendMapFilesFD(int CoreDumpSocket) {
      int FD = open("/proc/self/map_files", O_RDONLY | O_CLOEXEC | O_DIRECTORY);
      SendFD(CoreDumpSocket, CoreDump::PacketTypes::FD_MAP_FILES, FD);
      close(FD);
    }

    void SendHostContext(int CoreDumpSocket, siginfo_t const *siginfo, mcontext_t const *context) {
      CoreDump::PacketHostContext Msg = CoreDump::PacketHostContext::Fill(siginfo, context);
      SendPacket(CoreDumpSocket, &Msg, sizeof(Msg));
    }

    void SendGuestContext(int CoreDumpSocket, siginfo_t const *siginfo, void const *context, size_t ContextSize, uint8_t GuestArch) {
      CoreDump::PacketGuestContext Msg = CoreDump::PacketGuestContext::Fill(siginfo, context, ContextSize, GuestArch);
      SendPacket(CoreDumpSocket, &Msg, sizeof(Msg));
    }

    void SendContextUnwind(int CoreDumpSocket) {
      CoreDump::PacketHeader Msg = CoreDump::FillHeader(CoreDump::PacketTypes::CONTEXT_UNWIND);
      SendPacket(CoreDumpSocket, &Msg, sizeof(Msg));
    }

    void SendJITRegions(int CoreDumpSocket, FEXCore::Context::Context::JITRegionPairs const *Dispatcher, fextl::vector<FEXCore::Context::Context::JITRegionPairs> const *RegionPairs) {
      FEXServerClient::CoreDump::PacketGetJITRegions Msg = FEXServerClient::CoreDump::PacketGetJITRegions::Fill(Dispatcher, RegionPairs->size());
      SendPacketWithData(CoreDumpSocket, &Msg, sizeof(Msg), std::span(reinterpret_cast<const char*>(RegionPairs->data()), RegionPairs->size() * sizeof(FEXCore::Context::Context::JITRegionPairs)));
    }

    void HandleSocketData(bool *ShouldShutdown, int CoreDumpSocket) {
      fextl::vector<uint8_t> Data(1500);
      size_t CurrentRead = SocketUtil::ReadDataFromSocket(CoreDumpSocket, Data);
      size_t CurrentOffset{};
      while (CurrentOffset < CurrentRead) {
        FEXServerClient::CoreDump::PacketHeader *Req = reinterpret_cast<FEXServerClient::CoreDump::PacketHeader *>(&Data[CurrentOffset]);
        switch (Req->PacketType) {
        case FEXServerClient::CoreDump::PacketTypes::PEEK_MEMORY: {
          FEXServerClient::CoreDump::PacketPeekMem *Req = reinterpret_cast<FEXServerClient::CoreDump::PacketPeekMem *>(&Data[CurrentOffset]);
          uint64_t Data{};
          if (Req->Size == 8) {
            uint64_t *Addr = (uint64_t*)Req->Addr;
            Data  = *Addr;
          }
          else {
            uint32_t *Addr = (uint32_t*)Req->Addr;
            Data  = *Addr;
          }

          FEXServerClient::CoreDump::PacketPeekMemResponse Msg = FEXServerClient::CoreDump::PacketPeekMemResponse::Fill(Data);
          SendPacket(CoreDumpSocket, &Msg, sizeof(Msg));
          CurrentOffset += sizeof(*Req);
          break;
        }

        case FEXServerClient::CoreDump::PacketTypes::GET_FD_FROM_CLIENT: {
          FEXServerClient::CoreDump::PacketGetFDFromFilename *Req = reinterpret_cast<FEXServerClient::CoreDump::PacketGetFDFromFilename *>(&Data[CurrentOffset]);
          int FD = open(Req->Filepath, O_RDONLY);
          auto Msg = CoreDump::FillHeader(CoreDump::PacketTypes::SUCCESS);
          SendFDPacket(CoreDumpSocket, &Msg, sizeof(Msg), FD);
          close(FD);
          CurrentOffset += sizeof(*Req) + Req->FilenameLength + 1;
          break;
        }
        case FEXServerClient::CoreDump::PacketTypes::CLIENT_SHUTDOWN: {
          *ShouldShutdown = true;
          CurrentOffset += sizeof(FEXServerClient::CoreDump::PacketHeader);
          break;
        }
        default:
          // Something sent us an invalid packet. To ensure we don't spin infinitely, consume all the data.
          LogMan::Msg::EFmt("[CoreDumpClass] InvalidPacket size received 0x{:x} bytes.", CurrentRead - CurrentOffset);
          CurrentOffset = CurrentRead;
          break;
        }
      }
    }
    void WaitForRequests(int CoreDumpSocket) {
      time_t RequestTimeout {10};
      struct pollfd PollFD = {
        .fd = CoreDumpSocket,
        .events = POLLIN | POLLHUP | POLLERR | POLLNVAL | POLLREMOVE | POLLRDHUP,
        .revents = 0,
      };

      bool ShouldShutdown = false;
      while (!ShouldShutdown) {
        struct timespec ts{};
        ts.tv_sec = RequestTimeout;

        int Result = ppoll(&PollFD, 1, &ts, nullptr);
        if (Result > 0) {
          if (PollFD.revents != 0) {
            // Handle POLLIN and other events at the same time.
            if (PollFD.revents & (POLLHUP | POLLERR | POLLNVAL | POLLREMOVE | POLLRDHUP)) {
              // Listen socket error or shutting down
              ShouldShutdown = true;
            }
            else if (PollFD.revents & POLLIN) {
              HandleSocketData(&ShouldShutdown, CoreDumpSocket);
            }

            PollFD.revents = 0;
            --Result;
          }
        }
      }
    }

    ///< Server side handling
    int GetFDFromClient(int ServerSocket, fextl::string const *Filename) {
      FEXServerClient::CoreDump::PacketGetFDFromFilename Msg = FEXServerClient::CoreDump::PacketGetFDFromFilename::Fill(Filename);
      FEXServerClient::CoreDump::SendPacketWithData(ServerSocket, &Msg, sizeof(Msg), std::span(Filename->data(), Filename->size()));
      return FEXServerClient::CoreDump::HandleFDPacket(ServerSocket);
    }

    void SendShutdownPacket(int CoreDumpSocket) {
      FEXServerClient::CoreDump::PacketHeader Msg = FEXServerClient::CoreDump::FillHeader(FEXServerClient::CoreDump::PacketTypes::CLIENT_SHUTDOWN);
      SendPacket(CoreDumpSocket, &Msg, sizeof(Msg));
    }

    std::optional<uint64_t> PeekMemory(int ServerSocket, uint64_t Addr, uint32_t Size) {
      FEXServerClient::CoreDump::PacketPeekMem Msg = FEXServerClient::CoreDump::PacketPeekMem::Fill(Addr, Size);
      SendPacket(ServerSocket, &Msg, sizeof(Msg));
      FEXServerClient::CoreDump::PacketPeekMemResponse MsgResponse;
      if (ReceivePacket(ServerSocket, &MsgResponse, sizeof(MsgResponse))) {
        return MsgResponse.Data;
      }
      return std::nullopt;
    }

    int HandleFDPacket(int CoreDumpSocket) {
      return ReceiveFDWithAncillary<FEXServerClient::CoreDump::PacketHeader>(CoreDumpSocket);
    }
  }
}
