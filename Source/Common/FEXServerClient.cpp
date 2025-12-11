// SPDX-License-Identifier: MIT
#include "Common/AsyncNet.h"
#include "Common/Config.h"
#include "FDUtils.h"
#include "Common/FEXServerClient.h"

#include <FEXCore/Utils/CompilerDefs.h>
#include <FEXCore/Utils/FileLoading.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/fextl/fmt.h>
#include <FEXCore/fextl/string.h>
#include <FEXCore/fextl/vector.h>
#include <FEXHeaderUtils/Filesystem.h>
#include <FEXHeaderUtils/Syscalls.h>

#include <cstdlib>
#include <fcntl.h>
#include <linux/limits.h>
#include <unistd.h>
#include <sys/poll.h>
#include <sys/prctl.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/uio.h>
#include <thread>
#include <cstring>

namespace FEXServerClient {
Logging::PacketHeader Logging::FillHeader(PacketTypes Type) {
  Logging::PacketHeader Msg {
    .PacketType = Type,
    .PID = ::getpid(),
    .TID = FHU::Syscalls::gettid(),
  };
  clock_gettime(CLOCK_MONOTONIC, &Msg.Timestamp);

  return Msg;
}

int RequestPIDFDPacket(int ServerSocket, PacketType Type) {
  fasio::tcp_socket Socket {ServerSocket};
  FEXServerRequestPacket Req {
    .Header {
      .Type = Type,
    },
  };

  // Send request
  fasio::error ec;
  write(Socket, fasio::mutable_buffer {std::as_writable_bytes(std::span {&Req, 1})}, ec);
  if (ec != fasio::error::success) {
    return -1;
  }

  // Wait for success response and log FD
  FEXServerResultPacket Res {};
  fasio::mutable_buffer ResBuffer {std::as_writable_bytes(std::span {&Res, 1})};
  int NewFD = -1;
  ResBuffer.FD = &NewFD;
  auto BytesRead = Socket.read_some(ResBuffer, ec);
  if (ec != fasio::error::success || BytesRead != sizeof(Res) || Res.Header.Type != PacketType::TYPE_SUCCESS) {
    return -1;
  }

  return NewFD;
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

fextl::string GetTempFolder() {
  const std::array<const char*, 5> Vars = {
    "XDG_RUNTIME_DIR", "TMPDIR", "TMP", "TEMP", "TEMPDIR",
  };

  for (auto& Var : Vars) {
    auto Path = getenv(Var);
    if (Path) {
      // If one of the env variable-driven paths works then use that.
      return Path;
    }
  }

  // Fallback to `/tmp/` if no env vars are set.
  // Might not be ideal but we don't have much of a choice.
  return fextl::string {"/tmp"};
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
  auto Folder = GetTempFolder();

  if (FEXCore::Config::FindContainer() == "pressure-vessel") {
    // In pressure-vessel the mount point changes location.
    // This is due to pressure-vesssel being a chroot environment.
    // It by default maps the host-filesystem to `/run/host/` so we need to redirect.
    // After pressure-vessel is fully set up it will set the `FEX_ROOTFS` environment variable,
    // which FEX will pick up.
    Folder = "/run/host/" + Folder;
  }

  return Folder;
}

fextl::string GetServerSocketName() {
  FEX_CONFIG_OPT(ServerSocketPath, SERVERSOCKETPATH);
  if (ServerSocketPath().empty()) {
    return fextl::fmt::format("{}.FEXServer.Socket", ::getuid());
  }
  return ServerSocketPath;
}

fextl::string GetServerSocketPath() {
  fextl::string name {};
#ifndef FEX_STEAM_SUPPORT
  FEX_CONFIG_OPT(ServerSocketPath, SERVERSOCKETPATH);

  name = ServerSocketPath();

  if (name.starts_with("/")) {
    return name;
  }

  auto Folder = GetTempFolder();
#else
  // Under Steam the FEXServer's socket is a game-specific directory.
  auto Folder = GetServerLockFolder();
#endif

  if (name.empty()) {
    return fextl::fmt::format("{}/{}.FEXServer.Socket", Folder, ::getuid());
  } else {
    return fextl::fmt::format("{}/{}", Folder, name);
  }
}

int GetServerFD() {
  return ServerFD;
}

int ConnectToServer(ConnectionOption ConnectionOption) {
  int SocketFD {-1};
  size_t SizeOfAddr {};
  struct sockaddr_un addr {};
  size_t SizeOfSocketString {};

  // Create the initial unix socket
  SocketFD = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
  if (SocketFD == -1) {
    LogMan::Msg::EFmt("Couldn't open AF_UNIX socket {}", errno);
    return -1;
  }

  // Steam doesn't get to connect to global sockets.
#ifndef FEX_STEAM_SUPPORT
  auto ServerSocketName = GetServerSocketName();

  // AF_UNIX has a special feature for named socket paths.
  // If the name of the socket begins with `\0` then it is an "abstract" socket address.
  // The entirety of the name is used as a path to a socket that doesn't have any filesystem backing.
  addr.sun_family = AF_UNIX;
  SizeOfSocketString = std::min(ServerSocketName.size() + 1, sizeof(addr.sun_path) - 1);
  addr.sun_path[0] = 0; // Abstract AF_UNIX sockets start with \0
  strncpy(addr.sun_path + 1, ServerSocketName.data(), SizeOfSocketString);
  // Include final null character.
  SizeOfAddr = sizeof(addr.sun_family) + SizeOfSocketString;

  if (connect(SocketFD, reinterpret_cast<struct sockaddr*>(&addr), SizeOfAddr) == -1) {
    if (ConnectionOption == ConnectionOption::Default || errno != ECONNREFUSED) {
      LogMan::Msg::EFmt("Couldn't connect to FEXServer socket {} {}", ServerSocketName, errno);
    }
  } else {
    return SocketFD;
  }
#endif

  // Try again with a path-based socket, since abstract sockets will fail if we have been
  // placed in a new netns as part of a sandbox.
  auto ServerSocketPath = GetServerSocketPath();

  addr.sun_family = AF_UNIX;
  SizeOfSocketString = std::min(ServerSocketPath.size(), sizeof(addr.sun_path) - 1);
  strncpy(addr.sun_path, ServerSocketPath.data(), SizeOfSocketString);
  SizeOfAddr = sizeof(addr.sun_family) + SizeOfSocketString;
  if (connect(SocketFD, reinterpret_cast<struct sockaddr*>(&addr), SizeOfAddr) == -1) {
    if (ConnectionOption == ConnectionOption::Default || (errno != ECONNREFUSED && errno != ENOENT)) {
      LogMan::Msg::EFmt("Couldn't connect to FEXServer socket {} {}", ServerSocketPath, errno);
    }
  } else {
    return SocketFD;
  }

  close(SocketFD);
  return -1;
}

bool SetupClient(std::string_view InterpreterPath) {
  ServerFD = FEXServerClient::ConnectToAndStartServer(InterpreterPath);
  if (ServerFD == -1) {
    return false;
  }

  // If we were started in a container then we want to use the rootfs that they provided.
  // In the pressure-vessel case this is a combination of our rootfs and the steam soldier runtime.
  if (FEXCore::Config::FindContainer() != "pressure-vessel") {
    fextl::string RootFSPath = FEXServerClient::RequestRootFSPath(ServerFD);

    //// If everything has passed then we can now update the rootfs path
    FEXCore::Config::Set(FEXCore::Config::CONFIG_ROOTFS, RootFSPath);
  }

  return true;
}

int StartServer(std::string_view InterpreterPath, int watch_fd) {
  int LocalServerFD {-1};
  // Couldn't connect to the server. Start one

  // Open some pipes for letting us know when the server is ready
  int fds[2] {};
  if (pipe2(fds, 0) != 0) {
    LogMan::Msg::EFmt("Couldn't open pipe");
    return -1;
  }

  // Extract directory from InterpreterPath
  fextl::string InterpreterDir {InterpreterPath};
  size_t LastSlash = InterpreterDir.rfind('/');
  if (LastSlash != fextl::string::npos) {
    InterpreterDir = InterpreterDir.substr(0, LastSlash);
  }

  fextl::string FEXServerPath = fextl::fmt::format("{}/FEXServer", InterpreterDir);
  // Check if a local FEXServer next to FEX exists
  // If it does then it takes priority over the installed one
  if (!FHU::Filesystem::Exists(FEXServerPath)) {
    FEXServerPath = "FEXServer";
  }

  // Set-up our SIGCHLD handler to ignore the signal.
  // This is early in the initialization stage so no handlers have been installed.
  //
  // We want to ignore the signal so that if FEXServer starts in daemon mode, it
  // doesn't leave a zombie process around waiting for something to get the result.
  struct sigaction action {};
  action.sa_handler = SIG_IGN;
  sigaction(SIGCHLD, &action, &action);

  pid_t pid = fork();
  if (pid == 0) {
    // Child
    close(fds[0]); // Close read end of pipe

    const char* argv[6];

    auto pipe_string = fextl::fmt::format("{}", fds[1]);
    auto watch_fd_string = fextl::fmt::format("{}", watch_fd);
    size_t arg_count {};
    argv[arg_count++] = FEXServerPath.c_str();
    argv[arg_count++] = "--wait_pipe";
    argv[arg_count++] = pipe_string.c_str();

    if (watch_fd != -1) {
      argv[arg_count++] = "--watch_fd";
      argv[arg_count++] = watch_fd_string.c_str();
    }

    argv[arg_count++] = nullptr;

    if (execvp(argv[0], (char* const*)argv) == -1) {
      // Let the parent know that we couldn't execute for some reason
      uint64_t error {1};
      write(fds[1], &error, sizeof(error));

      // Give a hopefully helpful error message for users
      LogMan::Msg::EFmt("Couldn't execute: {}", argv[0]);
      LogMan::Msg::EFmt("This means the squashFS rootfs won't be mounted.");
      LogMan::Msg::EFmt("Expect errors!");
      // Destroy this fork
      exit(1);
    }

    FEX_UNREACHABLE;
  } else {
    // Parent
    // Wait for the child to exit so we can check if it is mounted or not
    close(fds[1]); // Close write end of the pipe

    // Wait for a message from FEXServer
    pollfd PollFD;
    PollFD.fd = fds[0];
    PollFD.events = POLLIN | POLLOUT | POLLRDHUP | POLLERR | POLLHUP | POLLNVAL;

    // Wait for a result on the pipe that isn't EINTR
    while (poll(&PollFD, 1, -1) == -1 && errno == EINTR)
      ;

    // Check if child signaled an error
    uint64_t error = 0;
    ssize_t bytes_read = read(fds[0], &error, sizeof(error));
    close(fds[0]);
    if (bytes_read > 0 && error != 0) {
      return -1;
    }

    for (size_t i = 0; i < 5; ++i) {
      LocalServerFD = ConnectToServer(ConnectionOption::Default);

      if (LocalServerFD != -1) {
        break;
      }

      std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    if (LocalServerFD == -1) {
      // Still couldn't connect to the socket.
      LogMan::Msg::EFmt("Couldn't connect to FEXServer socket after launching the process");
    }
  }

  // Restore the original SIGCHLD handler if it existed.
  sigaction(SIGCHLD, &action, nullptr);

  return LocalServerFD;
}

int ConnectToAndStartServer(std::string_view InterpreterPath) {
  int LocalServerFD = ConnectToServer(ConnectionOption::NoPrintConnectionError);
  if (LocalServerFD == -1) {
    LocalServerFD = StartServer(InterpreterPath);
  }
  return LocalServerFD;
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
  return RequestPIDFDPacket(ServerSocket, PacketType::TYPE_GET_LOG_FD);
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
      FEXServerResultPacket* ResultPacket = reinterpret_cast<FEXServerResultPacket*>(Data.data());
      if (ResultPacket->Header.Type == PacketType::TYPE_GET_ROOTFS_PATH && ResultPacket->MountPath.Length > 0) {
        return fextl::string(ResultPacket->MountPath.Mount);
      }
    }
  }

  return {};
}

int RequestPIDFD(int ServerSocket) {
  return RequestPIDFDPacket(ServerSocket, PacketType::TYPE_GET_PID_FD);
}

void PopulateCodeCache(int ServerSocket, int ProgramFD, bool HasMultiblock) {
  fasio::error ec;
  fasio::tcp_socket Socket {ServerSocket};

  // Send request
  FEXServerRequestPacket Req {
    .Header {.Type = HasMultiblock ? PacketType::TYPE_POPULATE_CODE_CACHE : PacketType::TYPE_POPULATE_CODE_CACHE_NO_MULTIBLOCK}};

  fasio::mutable_buffer WriteBuffer {std::as_writable_bytes(std::span {&Req, 1})};
  WriteBuffer.FD = &ProgramFD;
  write(Socket, WriteBuffer, ec);
  if (ec != fasio::error::success) {
    return;
  }

  // Wait for success response to ensure FEXServer completed any pending cache generation.
  // The cache loading code handles missing caches gracefully, so we don't
  // actually care about the result here.
  FEXServerResultPacket Res {};
  fasio::mutable_buffer ResBuffer {std::as_writable_bytes(std::span {&Res, 1})};
  read(Socket, ResBuffer, ec);
}

int RequestCodeMapFD(int ServerSocket, int ProgramFD, bool HasMultiblock) {
  fasio::tcp_socket Socket {ServerSocket};
  FEXServerRequestPacket Req {
    .Header {
      .Type = HasMultiblock ? PacketType::TYPE_QUERY_CODE_MAP : PacketType::TYPE_QUERY_CODE_MAP_NO_MULTIBLOCK,
    },
  };

  // Send request
  fasio::error ec;
  {
    fasio::mutable_buffer WriteBuffer {std::as_writable_bytes(std::span {&Req, 1})};
    WriteBuffer.FD = &ProgramFD;
    write(Socket, WriteBuffer, ec);
    if (ec != fasio::error::success) {
      return -1;
    }
  }

  // Wait for success response and log FD
  FEXServerResultPacket Res {};
  fasio::mutable_buffer ResBuffer {std::as_writable_bytes(std::span {&Res, 1})};
  int NewFD = -1;
  ResBuffer.FD = &NewFD;
  read(Socket, ResBuffer, ec);
  if (ec != fasio::error::success || Res.Header.Type != PacketType::TYPE_SUCCESS) {
    return -1;
  }

  return NewFD;
}

/**  @} */

/**
 * @name FEX logging through FEXServer
 * @{ */

void MsgHandler(int FD, LogMan::DebugLevels Level, const char* Message) {
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

void AssertHandler(int FD, const char* Message) {
  MsgHandler(FD, LogMan::DebugLevels::ASSERT, Message);
}
/**  @} */
} // namespace FEXServerClient
