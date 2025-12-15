// SPDX-License-Identifier: MIT
#include "FEXHeaderUtils/Syscalls.h"
#include "Logger.h"
#include "SquashFS.h"

#include <Common/AsyncNet.h>
#include <Common/Config.h>
#include <Common/FDUtils.h>
#include <Common/FEXServerClient.h>

#include <FEXCore/Core/CodeCache.h>
#include <FEXCore/HLE/SourcecodeResolver.h>

#include <fmt/ranges.h>

#include <atomic>
#include <cassert>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <poll.h>
#include <string>
#include <sys/file.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <vector>

#include <xxhash.h>

namespace FEXCore {
inline bool operator<(const FEXCore::ExecutableFileInfo& a, const FEXCore::ExecutableFileInfo& b) noexcept {
  return a.FileId < b.FileId;
}
} // namespace FEXCore

template<>
struct std::hash<FEXCore::ExecutableFileInfo> {
  std::size_t operator()(const FEXCore::ExecutableFileInfo& Val) const noexcept {
    return Val.FileId;
  }
};

namespace ProcessPipe {
constexpr int USER_PERMS = S_IRWXU | S_IRWXG | S_IRWXO;
int ServerLockFD {-1};
int WatchFD {-1};
std::optional<fasio::tcp_acceptor> ServerAcceptor;
std::optional<fasio::tcp_acceptor> ServerFSAcceptor;
int NumClients = 0;
time_t RequestTimeout {10};
bool Foreground {false};
std::vector<struct pollfd> PollFDs {};

// FD count watching
constexpr size_t static MAX_FD_DISTANCE = 32;
rlimit MaxFDs {};
std::atomic<size_t> NumFilesOpened {};

static std::string CodeMapDirectory;

void SetWatchFD(int FD) {
  WatchFD = FD;
}

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
      struct flock lk {
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
    struct flock lk {
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
  struct flock lk {
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
    LogMan::Msg::EFmt("Failed to create FEXServer socket: error {} ({})", errno, strerror(errno));
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
    ++NumClients;
    Reactor.bind_handler(
      pollfd {
        .fd = FD,
        .events = POLLIN | POLLPRI | POLLRDHUP,
        .revents = 0,
      },
      [Socket = std::move(Socket).value()](fasio::error ec) mutable {
        if (ec != fasio::error::success) {
          close(Socket.FD);
          --NumClients;
          return fasio::post_callback::drop;
        }
        HandleSocketData(Socket);
        // Wait for next data
        return fasio::post_callback::repeat;
      });

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

// Discovers any pending code maps, parses their contents into a runtime data structure, and deletes them
static std::map<FEXCore::ExecutableFileInfo, fextl::set<uintptr_t>>
ImportPendingCodeMaps(const FEXCore::ExecutableFileInfo& MainFileId, bool HasMultiblock) {
  // Detect code maps by checking file name suffixes by counting up an index.
  // Code maps that are ready for reading must be non-empty and flock(FLOCK_EX) must succeed:
  // - If empty, we tried generating the cache before the client could even lock it
  // - If exclusively lockable, we know the client either closed or crashed
  std::vector<std::string> CodeMaps;
  for (int Index = 0; true; ++Index) {
    auto CodeMap = fmt::format("{}/new/{}.{}.bin", CodeMapDirectory, FEXCore::CodeMap::GetBaseFilename(MainFileId, !HasMultiblock), Index);
    auto FD = open(CodeMap.c_str(), O_RDONLY);
    if (FD == -1) {
      break;
    }

    // Acquire exclusive lock to ensure the client process is done writing data.
    // Also ensure the file is non-empty, otherwise we're racing the client in acquiring the initial lock.
    struct stat FileStats;
    fstat(FD, &FileStats);
    if (FileStats.st_size == 0 || flock(FD, LOCK_EX | LOCK_NB) != 0) {
      fmt::print("Code map {} is still in use, skipping\n", CodeMap);
      // Still being written to by a client process, so skip this file
      // TODO: Rename from X.n.bin to X.0.bin (once the latter has been removed!) to ensure we'll catch it on next run
      close(FD);
      continue;
    } else {
      fmt::print("Found code map {}, queuing for merge\n", CodeMap);
    }
    close(FD);
    CodeMaps.push_back(CodeMap);
  }

  // Update merged code map
  std::map<FEXCore::ExecutableFileInfo, fextl::set<uintptr_t>> ImportedCodeMaps;
  if (!CodeMaps.empty()) {
    fmt::print("Found {} new code maps, updating reference code map\n", CodeMaps.size());

    for (auto& CodeMap : CodeMaps) {
      std::ifstream Incoming(CodeMap, std::ios_base::binary);
      auto NewBlocks = FEXCore::CodeMap::ParseCodeMap(Incoming);
      for (auto& [FileId, Contents] : NewBlocks) {
        ImportedCodeMaps.emplace(std::piecewise_construct, std::forward_as_tuple(nullptr, FileId, std::move(Contents.Filename)),
                                 std::forward_as_tuple(std::move(Contents.Blocks)));
      }
    }
  }

  // Delete all imported code maps
  for (auto& CodeMapFile : CodeMaps) {
    std::filesystem::remove(CodeMapFile);
    // TODO: Rename any pending (not finalized) code maps to PROGRAMNAME.0.bin so it will be found on the next run
  }

  return ImportedCodeMaps;
}

/**
 * Writes aggregated code map data into a single code map file that is ready to be used for cache generation
 */
static void WriteNewCodeMap(const FEXCore::ExecutableFileInfo& File, const std::string& OutputName, const fextl::set<uintptr_t>& Blocks,
                            bool IsMainFile, const auto& Dependencies) {
  fmt::print("Writing {} blocks to {}\n", Blocks.size(), OutputName);

  struct CodeMapOpener : FEXCore::CodeMapOpener {
    CodeMapOpener(const std::string& Filename) {
      FD = creat(Filename.c_str(), 0644);
    }

    int OpenCodeMapFile() override {
      return FD;
    }

    int FD;
  };

  CodeMapOpener CodeMapOpener(OutputName);
  FEXCore::CodeMapWriter OutputCodeMap(CodeMapOpener, true);
  if (IsMainFile) {
    // List the main executable and all used libraries
    OutputCodeMap.AppendSetMainExecutable(File);

    for (auto& [Dependency, _] : Dependencies) {
      OutputCodeMap.AppendLibraryLoad(Dependency);
    }
  } else {
    // List only the library itself
    OutputCodeMap.AppendLibraryLoad(File);
  }

  for (auto& Block : Blocks) {
    OutputCodeMap.AppendBlock(FEXCore::ExecutableFileSectionInfo {const_cast<FEXCore::ExecutableFileInfo&>(File), 0}, Block);
  }
}

enum class NeedsCacheRefresh {
  No,
  Yes,
};

/**
 * Checks and processes new code maps generated by FEX for the given application.
 *
 * Processed code maps are merged into the reference code map and deleted afterwards.
 *
 * The returned map is a list of all dependencies of the main executables discovered,
 * associated with a flag to indicate need for cache regeneration.
 */
static std::map<FEXCore::ExecutableFileInfo, NeedsCacheRefresh> AggregateCodeMaps(const FEXCore::ExecutableFileInfo& MainFileId, bool HasMultiblock) {
  std::map<FEXCore::ExecutableFileInfo, NeedsCacheRefresh> Result;

  // Read all dependencies discovered in previous runs
  {
    auto MainFileCodeMapPath = fmt::format("{}/ready/{}", CodeMapDirectory, FEXCore::CodeMap::GetBaseFilename(MainFileId, !HasMultiblock));
    std::ifstream MainFileCodeMap(MainFileCodeMapPath, std::ios_base::binary);
    for (auto& [FileId, Contents] : FEXCore::CodeMap::ParseCodeMap(MainFileCodeMap)) {
      Result.emplace(std::piecewise_construct, std::forward_as_tuple(nullptr, FileId, Contents.Filename),
                     std::forward_as_tuple(NeedsCacheRefresh::No));
    }
  }

  // Accumulate information from new code maps
  auto IncomingCodeMap = ImportPendingCodeMaps(MainFileId, HasMultiblock);
  for (auto& [File, _] : IncomingCodeMap) {
    Result.emplace(std::piecewise_construct, std::forward_as_tuple(nullptr, File.FileId, File.Filename),
                   std::forward_as_tuple(NeedsCacheRefresh::No));
  }

  // For each referenced library, add referenced offsets to that library's reference code map
  for (auto& [File, Blocks] : IncomingCodeMap) {
    const auto BinaryName = std::string {FEXCore::CodeMap::GetBaseFilename(File, !HasMultiblock)};
    auto OutputName = fmt::format("{}/ready/{}", CodeMapDirectory, BinaryName);

    // Check if the new code maps add any new information to the previous code map
    if (auto ReferenceCodeMap = std::ifstream(OutputName, std::ios_base::binary)) {
      auto PreviousBlocks = FEXCore::CodeMap::ParseCodeMap(ReferenceCodeMap).at(File.FileId).Blocks;
      auto NumPreviousBlocks = PreviousBlocks.size();
      Blocks.merge(std::move(PreviousBlocks));
      if (Blocks.size() == NumPreviousBlocks) {
        // No new blocks => no need to regenerate the corresponding cache
        continue;
      } else {
        fmt::println("  Found {} new blocks ({} total) in code map {} for {}", Blocks.size(), NumPreviousBlocks, BinaryName, File.Filename);
      }
    }

    // Update code map and queue for cache generation
    std::map<FEXCore::ExecutableFileInfo, NeedsCacheRefresh> Empty;
    WriteNewCodeMap(File, OutputName, Blocks, true, File.FileId == MainFileId.FileId ? Result : Empty);
    Result.at(File) = NeedsCacheRefresh::Yes;
  }

  return Result;
}

int32_t EmbedSubprocess(const char* path, char* const* args) {
  pid_t pid = fork();
  if (pid == 0) {
    execvp(path, args);
    _exit(-1);
  } else {
    int32_t Status {};
    while (waitpid(pid, &Status, 0) == -1 && errno == EINTR)
      ;
    if (WIFEXITED(Status)) {
      return (int8_t)WEXITSTATUS(Status);
    }
  }

  return -1;
}

/**
 * Spawn a FEXOfflineCompiler instance to generate a code cache from the given code map
 */
static int RunOfflineCompiler(const char* CodeMap) {
  const char* ExecveArgs[] = {"FEXOfflineCompiler", "generate", CodeMap, nullptr};
  return EmbedSubprocess("FEXOfflineCompiler", const_cast<char* const*>(&ExecveArgs[0]));
};

void HandleSocketData(fasio::tcp_socket& Socket) {
  std::vector<uint8_t> Data(1500);

  // Get the current number of FDs of the process before we start handling sockets.
  GetMaxFDs();

  int inFD = -1;
  fasio::mutable_buffer buffer = {std::as_writable_bytes(std::span(Data)), nullptr, &inFD};

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
        // Log thread isn't running. Let FEX know it can't have one.
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

    case FEXServerClient::PacketType::TYPE_POPULATE_CODE_CACHE:
    case FEXServerClient::PacketType::TYPE_POPULATE_CODE_CACHE_NO_MULTIBLOCK: {
      char Tmp[PATH_MAX];
      int TmpLen = FEX::get_fdpath(inFD, Tmp);
      assert(TmpLen != -1);

      std::filesystem::path Path {std::string_view(Tmp, TmpLen)};
      auto filename_hash = XXH3_64bits(Tmp, TmpLen);
      const bool HasMultiblock = (Req->Header.Type == FEXServerClient::PacketType::TYPE_POPULATE_CODE_CACHE);

      FEXCore::ExecutableFileInfo MainFileId = {nullptr, filename_hash, fextl::string(Tmp, TmpLen)};
      fmt::print("Requested {}cache generation for {}\n", HasMultiblock ? "" : "nomb-", MainFileId.Filename);

      auto GetCacheFilename = [](const FEXCore::ExecutableFileInfo& FileId) {
        return fmt::format("{}cache/{}-{:016x}", FEX::Config::GetCacheDirectory(), FEXCore::CodeMap::GetBaseFilename(FileId, false),
                           0 /* TODO: Use unique cache id */);
      };

      // Update code maps; any update necessitates an update of the corresponding cache
      auto Binaries = AggregateCodeMaps(MainFileId, HasMultiblock);

      // Check for other conditions that require a cache refresh even when the code map didn't change
      for (auto& [FileInfo, NeedsRefresh] : Binaries) {
        if (NeedsRefresh == NeedsCacheRefresh::Yes) {
          // Already queued for cache generation, no need for further checks
          continue;
        }

        // Trigger cache generation for this file if no cache exists or if the cache is older than the most recent update to its code map
        std::error_code ec;
        const auto BinaryName = FEXCore::CodeMap::GetBaseFilename(FileInfo, !HasMultiblock);
        const auto MergedCodeMapFilename = fmt::format("{}/ready/{}", CodeMapDirectory, BinaryName);
        const auto LastCodeMapUpdate = std::filesystem::last_write_time(MergedCodeMapFilename, ec);
        if (std::filesystem::last_write_time(GetCacheFilename(FileInfo), ec) < LastCodeMapUpdate || ec) {
          fmt::println("  Scheduling update for {} cache for {}", ec ? "missing" : "outdated", BinaryName);
          NeedsRefresh = NeedsCacheRefresh::Yes;
        }
      }

      // Trigger offline-compile for each binary that needs it
      for (const auto& [File, NeedsRefresh] : Binaries) {
        if (NeedsRefresh != NeedsCacheRefresh::Yes) {
          continue;
        }

        const auto BinaryName = (std::string)FEXCore::CodeMap::GetBaseFilename(File, !HasMultiblock);
        fmt::println("Generating cache for {}", BinaryName);
        int Status = RunOfflineCompiler(fmt::format("{}/ready/{}", CodeMapDirectory, BinaryName).c_str());
        if (Status != 0) {
          fmt::println("ERROR: Cache generation failed with status {}", Status);
        }
      }

      FEXServerClient::FEXServerResultPacket Res {
        .Header {
          .Type = FEXServerClient::PacketType::TYPE_SUCCESS,
        },
      };

      fasio::mutable_buffer Data = {.Data = std::as_writable_bytes(std::span(&Res, 1))};
      fasio::error ec;
      write(Socket, Data, ec);
      buffer += sizeof(FEXServerClient::FEXServerRequestPacket::Header);
      close(inFD);
      inFD = -1;
      break;
    }

    case FEXServerClient::PacketType::TYPE_QUERY_CODE_MAP:
    case FEXServerClient::PacketType::TYPE_QUERY_CODE_MAP_NO_MULTIBLOCK: {
      char Tmp[PATH_MAX];
      int TmpLen = FEX::get_fdpath(inFD, Tmp);
      assert(TmpLen != -1);
      std::filesystem::path BinaryPath = std::string_view(Tmp, TmpLen);
      // TODO: Move to common code
      const auto filename_hash = XXH3_64bits(Tmp, TmpLen);
      const bool HasMultiblock = (Req->Header.Type == FEXServerClient::PacketType::TYPE_QUERY_CODE_MAP);

      FEXServerClient::FEXServerResultPacket Res {
        .Header {
          .Type = FEXServerClient::PacketType::TYPE_SUCCESS,
        },
      };

      // Find first code map that doesn't exist yet
      int Index = 0;
      std::string Filename;
      do {
        Filename = fmt::format("{}/{}.{}.bin", CodeMapDirectory,
                               FEXCore::CodeMap::GetBaseFilename(
                                 FEXCore::ExecutableFileInfo {nullptr, filename_hash, (fextl::string)BinaryPath.string()}, !HasMultiblock),
                               Index++);
      } while (std::filesystem::exists(Filename));

      std::filesystem::create_directories(CodeMapDirectory);
      auto CodeMapFD = open(Filename.c_str(), O_CREAT | O_CLOEXEC | O_WRONLY, 0644);

      fasio::mutable_buffer Data = {.Data = std::as_writable_bytes(std::span(&Res, 1)),
                                    .FD = (CodeMapFD != -1 ? std::optional {&CodeMapFD} : std::nullopt)};
      fasio::error ec;
      write(Socket, Data, ec);
      buffer += sizeof(FEXServerClient::FEXServerRequestPacket::Header);
      close(inFD);
      inFD = -1;
      close(CodeMapFD);
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

  if (inFD != -1) {
    LogMan::Msg::EFmt("Received unused FD argument");
    close(inFD);
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
  if (WatchFD != -1) {
    // Add a fake client.
    ++NumClients;
    Reactor.bind_handler(
      pollfd {
        .fd = WatchFD,
        .events = POLLPRI | POLLRDHUP,
        .revents = 0,
      },
      [InternalWatchFD = WatchFD](fasio::error ec) mutable {
        if (ec != fasio::error::success) {
          close(InternalWatchFD);
          --NumClients;
          return fasio::post_callback::drop;
        }
        // Wait for next data
        return fasio::post_callback::repeat;
      });
  }

  Reactor.enable_async_stop();

  while (true) {
    std::optional Timeout = std::chrono::seconds {RequestTimeout};
    if (Foreground || NumClients > 0) {
      Timeout.reset();
    }
    auto Result = Reactor.run_one(Timeout);
    if (Result != fasio::error::success || Reactor.stopped()) {
      Reactor.cleanup();
      break;
    }
  }

  LogMan::Msg::DFmt("[FEXServer] Shutting Down");

  CloseConnections();
}

void SetConfiguration(bool Foreground, uint32_t PersistentTimeout) {
  ProcessPipe::Foreground = Foreground;
  ProcessPipe::RequestTimeout = PersistentTimeout;

  CodeMapDirectory = FEX::Config::GetCacheDirectory() + "codemap";
}

void Shutdown() {
  Reactor.stop_async();
}
} // namespace ProcessPipe
