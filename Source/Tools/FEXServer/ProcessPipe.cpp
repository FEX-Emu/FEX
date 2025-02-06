// SPDX-License-Identifier: MIT
#include "FEXHeaderUtils/Syscalls.h"
#include "Logger.h"
#include "SquashFS.h"

#include <Common/AsyncNet.h>
#include <Common/FEXServerClient.h>

#include <fmt/ranges.h>

#include <atomic>
#include <cassert>
#include <fcntl.h>
#include <filesystem>
#include <poll.h>
#include <string>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <vector>

namespace ProcessPipe {
constexpr int USER_PERMS = S_IRWXU | S_IRWXG | S_IRWXO;
int ServerLockFD {-1};
std::optional<fasio::tcp_acceptor> ServerAcceptor;
std::optional<fasio::tcp_acceptor> ServerFSAcceptor;
time_t RequestTimeout {10};
bool Foreground {false};
std::vector<struct pollfd> PollFDs {};

// FD count watching
constexpr size_t static MAX_FD_DISTANCE = 32;
rlimit MaxFDs {};
std::atomic<size_t> NumFilesOpened {};

size_t GetNumFilesOpen() {
  // Walk /proc/self/fd/ to see how many open files we currently have
  const std::filesystem::path self {"/proc/self/fd/"};

  return std::distance(std::filesystem::directory_iterator {self}, std::filesystem::directory_iterator {});
}

void GetMaxFDs() {
  // Get our kernel limit for the number of open files
  if (getrlimit(RLIMIT_NOFILE, &MaxFDs) != 0) {
    fprintf(stderr, "[FEXMountDaemon] getrlimit(RLIMIT_NOFILE) returned error %d %s\n", errno, strerror(errno));
  }

  // Walk /proc/self/fd/ to see how many open files we currently have
  NumFilesOpened = GetNumFilesOpen();
}

void CheckRaiseFDLimit() {
  if (NumFilesOpened < (MaxFDs.rlim_cur - MAX_FD_DISTANCE)) {
    // No need to raise the limit.
    return;
  }

  if (MaxFDs.rlim_cur == MaxFDs.rlim_max) {
    fprintf(stderr, "[FEXMountDaemon] Our open FD limit is already set to max and we are wanting to increase it\n");
    fprintf(stderr, "[FEXMountDaemon] FEXMountDaemon will now no longer be able to track new instances of FEX\n");
    fprintf(stderr, "[FEXMountDaemon] Current limit is %zd(hard %zd) FDs and we are at %zd\n", MaxFDs.rlim_cur, MaxFDs.rlim_max,
            GetNumFilesOpen());
    fprintf(stderr, "[FEXMountDaemon] Ask your administrator to raise your kernel's hard limit on open FDs\n");
    return;
  }

  rlimit NewLimit = MaxFDs;

  // Just multiply by two
  NewLimit.rlim_cur <<= 1;

  // Now limit to the hard max
  NewLimit.rlim_cur = std::min(NewLimit.rlim_cur, NewLimit.rlim_max);

  if (setrlimit(RLIMIT_NOFILE, &NewLimit) != 0) {
    fprintf(stderr, "[FEXMountDaemon] Couldn't raise FD limit to %zd even though our hard limit is %zd\n", NewLimit.rlim_cur, NewLimit.rlim_max);
  } else {
    // Set the new limit
    MaxFDs = NewLimit;
  }
}

bool InitializeServerPipe() {
  auto ServerFolder = FEXServerClient::GetServerLockFolder();

  std::error_code ec {};
  if (!std::filesystem::exists(ServerFolder, ec)) {
    // Doesn't exist, create the the folder as a user convenience
    if (!std::filesystem::create_directories(ServerFolder, ec)) {
      LogMan::Msg::EFmt("Couldn't create server pipe folder at: {}", ServerFolder);
      return false;
    }
  }

  auto ServerLockPath = FEXServerClient::GetServerLockFile();

  // Now this is some tricky locking logic to ensure that we only ever have one server running
  // The logic is as follows:
  // - Try to make the lock file
  // - If Exists then check to see if it is a stale handle
  //   - Stale checking means opening the file that we know exists
  //   - Then we try getting a write lock
  //   - If we fail to get the write lock, then leave
  //   - Otherwise continue down the codepath and degrade to read lock
  // - Else try to acquire a write lock to ensure only one FEXServer exists
  //
  // - Once a write lock is acquired, downgrade it to a read lock
  //   - This ensures that future FEXServers won't race to create multiple read locks
  int Ret = open(ServerLockPath.c_str(), O_RDWR | O_CREAT | O_CLOEXEC | O_EXCL, USER_PERMS);
  ServerLockFD = Ret;

  if (Ret == -1 && errno == EEXIST) {
    // If the lock exists then it might be a stale connection.
    // Check the lock status to see if another process is still alive.
    ServerLockFD = open(ServerLockPath.c_str(), O_RDWR | O_CLOEXEC, USER_PERMS);
    if (ServerLockFD != -1) {
      // Now that we have opened the file, try to get a write lock.
      flock lk {
        .l_type = F_WRLCK,
        .l_whence = SEEK_SET,
        .l_start = 0,
        .l_len = 0,
      };
      Ret = fcntl(ServerLockFD, F_SETLK, &lk);

      if (Ret != -1) {
        // Write lock was gained, we can now continue onward.
      } else {
        // We couldn't get a write lock, this means that another process already owns a lock on the lock
        close(ServerLockFD);
        ServerLockFD = -1;
        return false;
      }
    } else {
      // File couldn't get opened even though it existed?
      // Must have raced something here.
      return false;
    }
  } else if (Ret == -1) {
    // Unhandled error.
    LogMan::Msg::EFmt("Unable to create FEXServer named lock file at: {} {} {}", ServerLockPath, errno, strerror(errno));
    return false;
  } else {
    // FIFO file was created. Try to get a write lock
    flock lk {
      .l_type = F_WRLCK,
      .l_whence = SEEK_SET,
      .l_start = 0,
      .l_len = 0,
    };
    Ret = fcntl(ServerLockFD, F_SETLK, &lk);

    if (Ret == -1) {
      // Couldn't get a write lock, something else must have got it
      close(ServerLockFD);
      ServerLockFD = -1;
      return false;
    }
  }

  // Now that a write lock is held, downgrade it to a read lock
  flock lk {
    .l_type = F_RDLCK,
    .l_whence = SEEK_SET,
    .l_start = 0,
    .l_len = 0,
  };
  Ret = fcntl(ServerLockFD, F_SETLK, &lk);

  if (Ret == -1) {
    // This shouldn't occur
    LogMan::Msg::EFmt("Unable to downgrade a write lock to a read lock {} {} {}", ServerLockPath, errno, strerror(errno));
    close(ServerLockFD);
    ServerLockFD = -1;
    return false;
  }

  return true;
}

static fasio::poll_reactor Reactor;

void HandleSocketData(fasio::tcp_socket&);

bool InitializeServerSocket(bool abstract) {
  fextl::string ServerSocketName;
  if (abstract) {
    ServerSocketName = FEXServerClient::GetServerSocketName();
  } else {
    ServerSocketName = FEXServerClient::GetServerSocketPath();
    // Unlink the socket file if it exists
    // We are being asked to create a daemon, not error check
    // We don't care if this failed or not
    unlink(ServerSocketName.c_str());
  }
  auto Acceptor = fasio::tcp_acceptor::create(Reactor, abstract, ServerSocketName);
  if (!Acceptor) {
    LogMan::Msg::EFmt("Failed to create FEXServer socket: error {} (){})", errno, strerror(errno));
    return false;
  }

  Acceptor->async_accept([](fasio::error ec, std::optional<fasio::tcp_socket> Socket) {
    if (ec != fasio::error::success) {
      if (ec == fasio::error::generic_errno) {
        LogMan::Msg::EFmt("FEXServer failed to establish client connection: error {} ({})", errno, strerror(errno));
      }
      // Ignore error and wait for next connection
      return fasio::post_callback::repeat;
    }

    int FD = Socket->FD;
    Reactor.read_callbacks[FD] = [Socket = std::move(Socket).value()](fasio::error ec) mutable {
      if (ec != fasio::error::success) {
        return fasio::post_callback::drop;
      }
      HandleSocketData(Socket);
      // Wait for next data
      return fasio::post_callback::repeat;
    };

    // Wait for next connection
    return fasio::post_callback::repeat;
  });

  (abstract ? ServerAcceptor : ServerFSAcceptor) = std::move(Acceptor).value();
  return true;
}

void SendEmptyErrorPacket(fasio::tcp_socket& Socket) {
  FEXServerClient::FEXServerResultPacket Res {
    .Header {
      .Type = FEXServerClient::PacketType::TYPE_ERROR,
    },
  };

  fasio::mutable_buffer Data = {.Data = std::as_writable_bytes(std::span(&Res, 1))};
  fasio::error ec;
  write(Socket, Data, ec);
}

void SendFDSuccessPacket(fasio::tcp_socket& Socket, int FD) {
  FEXServerClient::FEXServerResultPacket Res {
    .Header {
      .Type = FEXServerClient::PacketType::TYPE_SUCCESS,
    },
  };

  fasio::mutable_buffer Data = {.Data = std::as_writable_bytes(std::span(&Res, 1)), .FD = &FD};
  fasio::error ec;
  write(Socket, Data, ec);
}

void HandleSocketData(fasio::tcp_socket& Socket) {
  std::vector<uint8_t> Data(1500);

  // Get the current number of FDs of the process before we start handling sockets.
  GetMaxFDs();

  fasio::mutable_buffer buffer = {std::as_writable_bytes(std::span(Data))};

  {
    fasio::error ec;

    auto Read = Socket.read_some(buffer, ec);
    if (ec == fasio::error::success) {
      assert(Read >= sizeof(FEXServerClient::FEXServerRequestPacket));
      buffer = {buffer.Data.subspan(0, Read)};
    } else if (ec == fasio::error::eof) {
      return;
    } else {
      perror("read");
      return;
    }
  }

  while (buffer.size() > 0) {
    FEXServerClient::FEXServerRequestPacket* Req = reinterpret_cast<FEXServerClient::FEXServerRequestPacket*>(Data.data());
    switch (Req->Header.Type) {
    case FEXServerClient::PacketType::TYPE_KILL:
      Reactor.stop_async();
      buffer += sizeof(FEXServerClient::FEXServerRequestPacket::BasicRequest);
      break;
    case FEXServerClient::PacketType::TYPE_GET_LOG_FD: {
      if (Logger::LogThreadRunning()) {
        int fds[2] {};
        pipe2(fds, 0);
        // 0 = Read
        // 1 = Write
        Logger::AppendLogFD(fds[0]);

        SendFDSuccessPacket(Socket, fds[1]);

        // Close the write side now, doesn't matter to us
        close(fds[1]);

        // Check if we need to increase the FD limit.
        ++NumFilesOpened;
        CheckRaiseFDLimit();
      } else {
        // Log thread isn't running. Let FEXInterpreter know it can't have one.
        SendEmptyErrorPacket(Socket);
      }

      buffer += sizeof(FEXServerClient::FEXServerRequestPacket::Header);
      break;
    }
    case FEXServerClient::PacketType::TYPE_GET_ROOTFS_PATH: {
      const fextl::string& MountFolder = SquashFS::GetMountFolder();

      FEXServerClient::FEXServerResultPacket Res {
        .MountPath {
          .Header {
            .Type = FEXServerClient::PacketType::TYPE_GET_ROOTFS_PATH,
          },
          .Length = MountFolder.size() + 1,
        },
      };

      char Null {};

      fasio::mutable_buffer Data[] = {
        {.Data = std::as_writable_bytes(std::span(&Res, 1))},
        {.Data = std::as_writable_bytes(std::span(const_cast<fextl::string&>(MountFolder)))},
        {.Data = std::as_writable_bytes(std::span(&Null, 1))},
      };
      fasio::error ec;
      write(Socket, Chained(Data), ec);

      buffer += sizeof(FEXServerClient::FEXServerRequestPacket::BasicRequest);
      break;
    }
    case FEXServerClient::PacketType::TYPE_GET_PID_FD: {
      int FD = FHU::Syscalls::pidfd_open(::getpid(), 0);

      if (FD < 0) {
        // Couldn't get PIDFD due to too old of kernel.
        // Return a pipe to track the same information.
        //
        int fds[2];
        pipe2(fds, O_CLOEXEC);
        SendFDSuccessPacket(Socket, fds[0]);

        // Close the read side now, doesn't matter to us
        close(fds[0]);

        // Check if we need to increase the FD limit.
        ++NumFilesOpened;
        CheckRaiseFDLimit();

        // Write side will naturally close on process exit, letting the other process know we have exited.
      } else {
        SendFDSuccessPacket(Socket, FD);

        // Close the FD now since we've sent it
        close(FD);
      }

      buffer += sizeof(FEXServerClient::FEXServerRequestPacket::Header);
      break;
    }
    // Invalid
    case FEXServerClient::PacketType::TYPE_ERROR:
    default:
      // Something sent us an invalid packet. Drop this client and continue
      LogMan::Msg::EFmt("Invalid FEXServer packet received: {:02x}", fmt::join(buffer.Data, ""));
      close(Socket.FD);
      return;
    }
  }
}

void CloseConnections() {
  // Close the server pipe so new processes will know to spin up a new FEXServer.
  // This one is closing
  close(ServerLockFD);

  // Close the server socket so no more connections can be started
  ServerAcceptor.reset();
  ServerFSAcceptor.reset();
}

void WaitForRequests() {
  Reactor.enable_async_stop();
  Reactor.run(Foreground ? std::nullopt : std::optional {std::chrono::seconds {RequestTimeout}});

  LogMan::Msg::DFmt("[FEXServer] Shutting Down");

  CloseConnections();
}

void SetConfiguration(bool Foreground, uint32_t PersistentTimeout) {
  ProcessPipe::Foreground = Foreground;
  ProcessPipe::RequestTimeout = PersistentTimeout;
}

void Shutdown() {
  Reactor.stop_async();
}
} // namespace ProcessPipe
