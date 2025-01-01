// SPDX-License-Identifier: MIT
/*
$info$
tags: LinuxSyscalls|common
desc: Rootfs overlay logic
$end_info$
*/

#include "Common/Config.h"
#include "Common/FDUtils.h"
#include "Common/JSONPool.h"

#include "FEXCore/Config/Config.h"
#include "LinuxSyscalls/FileManagement.h"
#include "LinuxSyscalls/EmulatedFiles/EmulatedFiles.h"
#include "LinuxSyscalls/Syscalls.h"
#include "LinuxSyscalls/x64/Syscalls.h"

#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/FileLoading.h>
#include <FEXCore/fextl/fmt.h>
#include <FEXCore/fextl/list.h>
#include <FEXCore/fextl/string.h>
#include <FEXCore/fextl/vector.h>
#include <FEXHeaderUtils/Filesystem.h>
#include <FEXHeaderUtils/SymlinkChecks.h>
#include <FEXHeaderUtils/Syscalls.h>

#include <algorithm>
#include <errno.h>
#include <cstring>
#include <linux/openat2.h>
#include <fcntl.h>
#include <filesystem>
#include <optional>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/xattr.h>
#include <syscall.h>
#include <system_error>
#include <unistd.h>
#include <utility>

#include <tiny-json.h>

namespace FEX::HLE {
bool FileManager::RootFSPathExists(const char* Filepath) {
  LOGMAN_THROW_A_FMT(Filepath && Filepath[0] == '/', "Filepath needs to be absolute");
  return FHU::Filesystem::ExistsAt(RootFSFD, Filepath + 1);
}

void FileManager::LoadThunkDatabase(fextl::unordered_map<fextl::string, ThunkDBObject>& ThunkDB, bool Global) {
  auto ThunkDBPath = FEXCore::Config::GetConfigDirectory(Global) + "ThunksDB.json";
  fextl::vector<char> FileData;
  if (FEXCore::FileLoading::LoadFile(FileData, ThunkDBPath)) {

    // If the thunksDB file exists then we need to check if the rootfs supports multi-arch or not.
    const bool RootFSIsMultiarch = RootFSPathExists("/usr/lib/x86_64-linux-gnu/") || RootFSPathExists("/usr/lib/i386-linux-gnu/");

    fextl::vector<fextl::string> PathPrefixes {};
    if (RootFSIsMultiarch) {
      // Multi-arch debian distros have a fairly complex arrangement of filepaths.
      // These fractal out to the combination of library prefixes with arch suffixes.
      constexpr static std::array<std::string_view, 4> LibPrefixes = {
        "/usr/lib",
        "/usr/local/lib",
        "/lib",
        "/usr/lib/pressure-vessel/overrides/lib",
      };

      // We only need to generate 32-bit or 64-bit depending on the operating mode.
      const auto ArchPrefix = Is64BitMode() ? "x86_64-linux-gnu" : "i386-linux-gnu";

      for (auto Prefix : LibPrefixes) {
        PathPrefixes.emplace_back(fextl::fmt::format("{}/{}", Prefix, ArchPrefix));
      }
    } else {
      // Non multi-arch supporting distros like Fedora and Debian have a much more simple layout.
      // lib/ folders refer to 32-bit library folders.
      // li64/ folders refer to 64-bit library folders.
      constexpr static std::array<std::string_view, 4> LibPrefixes = {
        "/usr",
        "/usr/local",
        "", // root, the '/' will be appended in the next step.
        "/usr/lib/pressure-vessel/overrides",
      };

      // We only need to generate 32-bit or 64-bit depending on the operating mode.
      const auto ArchPrefix = Is64BitMode() ? "lib64" : "lib";

      for (auto Prefix : LibPrefixes) {
        PathPrefixes.emplace_back(fextl::fmt::format("{}/{}", Prefix, ArchPrefix));
      }
    }

    FEX::JSON::JsonAllocator Pool {};
    const json_t* json = FEX::JSON::CreateJSON(FileData, Pool);

    if (!json) {
      return;
    }

    const json_t* DB = json_getProperty(json, "DB");
    if (!DB || JSON_OBJ != json_getType(DB)) {
      return;
    }

    std::string_view HomeDirectory = FEX::Config::GetHomeDirectory();

    for (const json_t* Library = json_getChild(DB); Library != nullptr; Library = json_getSibling(Library)) {
      // Get the user defined name for the library
      const char* LibraryName = json_getName(Library);
      auto DBObject = ThunkDB.insert_or_assign(LibraryName, ThunkDBObject {}).first;

      // Walk the libraries items to get the data
      for (const json_t* LibraryItem = json_getChild(Library); LibraryItem != nullptr; LibraryItem = json_getSibling(LibraryItem)) {
        std::string_view ItemName = json_getName(LibraryItem);

        if (ItemName == "Library") {
          // "Library": "libGL-guest.so"
          DBObject->second.LibraryName = json_getValue(LibraryItem);
        } else if (ItemName == "Depends") {
          jsonType_t PropertyType = json_getType(LibraryItem);
          if (PropertyType == JSON_TEXT) {
            DBObject->second.Depends.insert(json_getValue(LibraryItem));
          } else if (PropertyType == JSON_ARRAY) {
            for (const json_t* Depend = json_getChild(LibraryItem); Depend != nullptr; Depend = json_getSibling(Depend)) {
              DBObject->second.Depends.insert(json_getValue(Depend));
            }
          }
        } else if (ItemName == "Overlay") {
          auto AddWithReplacement = [HomeDirectory, &PathPrefixes](ThunkDBObject& DBObject, fextl::string LibraryItem) {
            // Walk through template string and fill in prefixes from right to left

            using namespace std::string_view_literals;
            const std::pair PrefixHome {"@HOME@"sv, LibraryItem.find("@HOME@")};
            const std::pair PrefixLib {"@PREFIX_LIB@"sv, LibraryItem.find("@PREFIX_LIB@")};

            fextl::string::size_type PrefixPositions[] = {
              PrefixHome.second,
              PrefixLib.second,
            };
            // Sort offsets in descending order to enable safe in-place replacement
            std::sort(std::begin(PrefixPositions), std::end(PrefixPositions), std::greater<> {});

            for (auto& LibPrefix : PathPrefixes) {
              fextl::string Replacement = LibraryItem;
              for (auto PrefixPos : PrefixPositions) {
                if (PrefixPos == fextl::string::npos) {
                  continue;
                } else if (PrefixPos == PrefixHome.second) {
                  Replacement.replace(PrefixPos, PrefixHome.first.size(), HomeDirectory);
                } else if (PrefixPos == PrefixLib.second) {
                  Replacement.replace(PrefixPos, PrefixLib.first.size(), LibPrefix);
                }
              }
              DBObject.Overlays.emplace_back(std::move(Replacement));

              if (PrefixLib.second == fextl::string::npos) {
                // Don't repeat for other LibPrefixes entries if the prefix wasn't used
                break;
              }
            }
          };

          jsonType_t PropertyType = json_getType(LibraryItem);
          if (PropertyType == JSON_TEXT) {
            AddWithReplacement(DBObject->second, json_getValue(LibraryItem));
          } else if (PropertyType == JSON_ARRAY) {
            for (const json_t* Overlay = json_getChild(LibraryItem); Overlay != nullptr; Overlay = json_getSibling(Overlay)) {
              AddWithReplacement(DBObject->second, json_getValue(Overlay));
            }
          }
        }
      }
    }
  }
}

FileManager::FileManager(FEXCore::Context::Context* ctx)
  : EmuFD {ctx} {
  const auto& ThunkConfigFile = ThunkConfig();

  // We try to load ThunksDB from:
  // - FEX global config
  // - FEX user config
  // - Defined ThunksConfig option
  // - Steam AppConfig Global
  // - AppConfig Global
  // - Steam AppConfig Local
  // - AppConfig Local
  // - AppConfig override
  // This doesn't support the classic thunks interface.

  const auto& AppName = AppConfigName();
  fextl::vector<fextl::string> ConfigPaths {
    FEXCore::Config::GetConfigFileLocation(true),
    FEXCore::Config::GetConfigFileLocation(false),
    ThunkConfigFile,
  };

  auto SteamID = getenv("SteamAppId");
  if (SteamID) {
    // If a SteamID exists then let's search for Steam application configs as well.
    // We want to key off both the SteamAppId number /and/ the executable since we may not want to thunk all binaries.
    fextl::string SteamAppName = fextl::fmt::format("Steam_{}_{}", SteamID, AppName);

    // Steam application configs interleaved with non-steam for priority sorting.
    ConfigPaths.emplace_back(FEXCore::Config::GetApplicationConfig(SteamAppName, true));
    ConfigPaths.emplace_back(FEXCore::Config::GetApplicationConfig(AppName, true));
    ConfigPaths.emplace_back(FEXCore::Config::GetApplicationConfig(SteamAppName, false));
    ConfigPaths.emplace_back(FEXCore::Config::GetApplicationConfig(AppName, false));
  } else {
    ConfigPaths.emplace_back(FEXCore::Config::GetApplicationConfig(AppName, true));
    ConfigPaths.emplace_back(FEXCore::Config::GetApplicationConfig(AppName, false));
  }

  const char* AppConfig = getenv("FEX_APP_CONFIG");
  if (AppConfig) {
    ConfigPaths.emplace_back(AppConfig);
  }

  if (!LDPath().empty()) {
    RootFSFD = open(LDPath().c_str(), O_DIRECTORY | O_PATH | O_CLOEXEC);
    if (RootFSFD == -1) {
      RootFSFD = AT_FDCWD;
    } else {
      TrackFEXFD(RootFSFD);
    }
  }

  fextl::unordered_map<fextl::string, ThunkDBObject> ThunkDB;
  LoadThunkDatabase(ThunkDB, true);
  LoadThunkDatabase(ThunkDB, false);

  for (const auto& Path : ConfigPaths) {
    fextl::vector<char> FileData;
    if (FEXCore::FileLoading::LoadFile(FileData, Path)) {
      FEX::JSON::JsonAllocator Pool {};

      // If a thunks DB property exists then we pull in data from the thunks database
      const json_t* json = FEX::JSON::CreateJSON(FileData, Pool);
      if (!json) {
        continue;
      }

      const json_t* ThunksDB = json_getProperty(json, "ThunksDB");
      if (!ThunksDB) {
        continue;
      }

      for (const json_t* Item = json_getChild(ThunksDB); Item != nullptr; Item = json_getSibling(Item)) {
        const char* LibraryName = json_getName(Item);
        bool LibraryEnabled = json_getInteger(Item) != 0;
        // If the library is enabled then find it in the DB
        auto DBObject = ThunkDB.find(LibraryName);
        if (DBObject != ThunkDB.end()) {
          DBObject->second.Enabled = LibraryEnabled;
        }
      }
    }
  }

  // Now that we loaded the thunks object, walk through and ensure dependencies are enabled as well
  auto ThunkGuestPath = Is64BitMode() ? ThunkGuestLibs() : ThunkGuestLibs32();
  for (const auto& DBObject : ThunkDB) {
    if (!DBObject.second.Enabled) {
      continue;
    }

    // Recursively add paths for this thunk library and its dependencies to ThunkOverlays.
    // Using a local struct for this is slightly less ugly than using self-capturing lambdas
    struct {
      decltype(FileManager::ThunkOverlays)& ThunkOverlays;
      decltype(ThunkDB)& ThunkDB;
      const fextl::string& ThunkGuestPath;
      bool Is64BitMode;

      void SetupOverlay(const ThunkDBObject& DBDepend) {
        auto ThunkPath = fextl::fmt::format("{}/{}", ThunkGuestPath, DBDepend.LibraryName);
        if (!FHU::Filesystem::Exists(ThunkPath)) {
          if (!Is64BitMode) {
            // Guest libraries not existing is expected since not all libraries are thunked on 32-bit
            return;
          }
          ERROR_AND_DIE_FMT("Requested thunking via guest library \"{}\" that does not exist", ThunkPath);
        }

        for (const auto& Overlay : DBDepend.Overlays) {
          // Direct full path in guest RootFS to our overlay file
          ThunkOverlays.emplace(Overlay, ThunkPath);
        }
      };

      void InsertDependencies(const fextl::unordered_set<fextl::string>& Depends) {
        for (const auto& Depend : Depends) {
          auto& DBDepend = ThunkDB.at(Depend);
          if (DBDepend.Enabled) {
            continue;
          }

          SetupOverlay(DBDepend);

          // Mark enabled and recurse into dependencies
          DBDepend.Enabled = true;
          InsertDependencies(DBDepend.Depends);
        }
      };
    } DBObjectHandler {ThunkOverlays, ThunkDB, ThunkGuestPath, Is64BitMode()};

    DBObjectHandler.SetupOverlay(DBObject.second);
    DBObjectHandler.InsertDependencies(DBObject.second.Depends);
  }

  if (false) {
    // Useful for debugging
    if (ThunkOverlays.size()) {
      LogMan::Msg::IFmt("Thunk Overlays:");
      for (const auto& [Overlay, ThunkPath] : ThunkOverlays) {
        LogMan::Msg::IFmt("\t{} -> {}", Overlay, ThunkPath);
      }
    }
  }

  // Keep an fd open for /proc, to bypass chroot-style sandboxes
  ProcFD = open("/proc", O_RDONLY | O_CLOEXEC);

  // Track the st_dev of /proc, to check for inode equality
  struct stat Buffer;
  auto Result = fstat(ProcFD, &Buffer);
  if (Result >= 0) {
    ProcFSDev = Buffer.st_dev;
  }

  UpdatePID(::getpid());
}

FileManager::~FileManager() {
  close(RootFSFD);
}

size_t FileManager::GetRootFSPrefixLen(const char* pathname, size_t len, bool AliasedOnly) {
  if (len < 2 ||            // If no pathname or root
      pathname[0] != '/') { // If we are getting root
    return 0;
  }

  const auto& RootFSPath = LDPath();
  if (RootFSPath.empty()) { // If RootFS doesn't exist
    return 0;
  }

  auto RootFSLen = RootFSPath.length();
  if (RootFSPath.ends_with("/")) {
    RootFSLen -= 1;
  }

  if (RootFSLen > len) {
    return 0;
  }

  if (memcmp(pathname, RootFSPath.c_str(), RootFSLen) || (len > RootFSLen && pathname[RootFSLen] != '/')) {
    return 0; // If the path is not within the RootFS
  }

  if (AliasedOnly) {
    fextl::string Path(pathname, len); // Need to nul-terminate so copy

    struct stat HostStat {};
    struct stat RootFSStat {};
    if (lstat(Path.c_str(), &RootFSStat)) {
      LogMan::Msg::DFmt("GetRootFSPrefixLen: lstat on RootFS path failed: {}", std::string_view(pathname, len));
      return 0; // RootFS path does not exist?
    }
    if (lstat(Path.c_str() + RootFSLen, &HostStat)) {
      return 0; // Host path does not exist or not accessible
    }
    // Note: We do not check st_dev, since the RootFS might be
    // an overlayfs mount that changes it. This means there could
    // be false positives. However, since we check the size too,
    // this is highly unlikely (an overlaid file would need to
    // have the same exact size and coincidentally the same
    // inode number as on the host, which is implausible for things
    // like binaries and libraries).
    if (RootFSStat.st_size != HostStat.st_size || RootFSStat.st_ino != HostStat.st_ino || RootFSStat.st_mode != HostStat.st_mode) {
      return 0; // Host path is a different file
    }
  }

  return RootFSLen;
}

ssize_t FileManager::StripRootFSPrefix(char* pathname, ssize_t len, bool leaky) {
  if (len < 0) {
    return len;
  }

  auto Prefix = GetRootFSPrefixLen(pathname, len, false);
  if (Prefix == 0) {
    return len;
  }

  if (Prefix == len) {
    if (leaky) {
      // Getting the root, without a trailing /. This is a hack pressure-vessel uses to get the FEX RootFS,
      // so we have to leak it here...
      LogMan::Msg::DFmt("Leaking RootFS path for pressure-vessel");
      return len;
    } else {
      ::strcpy(pathname, "/");
      return 1;
    }
  }

  ::memmove(pathname, pathname + Prefix, len - Prefix);
  pathname[len - Prefix] = '\0';

  return len - Prefix;
}

fextl::string FileManager::GetHostPath(fextl::string& Path, bool AliasedOnly) {
  auto Prefix = GetRootFSPrefixLen(Path.c_str(), Path.length(), AliasedOnly);

  if (Prefix == 0) {
    return {};
  }

  auto ret = Path.substr(Prefix);
  if (ret.empty()) { // Getting the root
    ret = "/";
  }

  return ret;
}

fextl::string FileManager::GetEmulatedPath(const char* pathname, bool FollowSymlink) {
  if (!pathname ||                  // If no pathname
      pathname[0] != '/' ||         // If relative
      strcmp(pathname, "/") == 0) { // If we are getting root
    return {};
  }

  auto thunkOverlay = ThunkOverlays.find(pathname);
  if (thunkOverlay != ThunkOverlays.end()) {
    return thunkOverlay->second;
  }

  const auto& RootFSPath = LDPath();
  if (RootFSPath.empty()) { // If RootFS doesn't exist
    return {};
  }

  fextl::string Path = RootFSPath + pathname;
  if (FollowSymlink) {
    char Filename[PATH_MAX];
    while (FEX::HLE::IsSymlink(AT_FDCWD, Path.c_str())) {
      auto SymlinkSize = FEX::HLE::GetSymlink(AT_FDCWD, Path.c_str(), Filename, PATH_MAX - 1);
      if (SymlinkSize > 0 && Filename[0] == '/') {
        Path = RootFSPath;
        Path += std::string_view(Filename, SymlinkSize);
      } else {
        break;
      }
    }
  }
  return Path;
}

std::pair<int, const char*> FileManager::GetEmulatedFDPath(int dirfd, const char* pathname, bool FollowSymlink, FDPathTmpData& TmpFilename) {
  constexpr auto NoEntry = std::make_pair(-1, nullptr);

  if (!pathname) {
    // No pathname.
    return NoEntry;
  }

  if (pathname[0] == '/') {
    // If the path is absolute then dirfd is ignored.
    dirfd = AT_FDCWD;
  }

  if (pathname[0] != '/' || // If relative
      pathname[1] == 0 ||   // If we are getting root
      dirfd != AT_FDCWD) {  // If dirfd isn't special FDCWD
    return NoEntry;
  }

  auto thunkOverlay = ThunkOverlays.find(pathname);
  if (thunkOverlay != ThunkOverlays.end()) {
    return std::make_pair(AT_FDCWD, thunkOverlay->second.c_str());
  }

  if (RootFSFD == AT_FDCWD) {
    // If RootFS doesn't exist
    return NoEntry;
  }

  // Starting subpath is the pathname passed in.
  const char* SubPath = pathname;

  // Current index for the temporary path to use.
  uint32_t CurrentIndex {};

  // The two temporary paths.
  const std::array<char*, 2> TmpPaths = {
    TmpFilename[0],
    TmpFilename[1],
  };

  if (FollowSymlink) {
    // Check if the combination of RootFS FD and subpath with the front '/' stripped off is a symlink.
    bool HadAtLeastOne {};
    struct stat Buffer {};
    for (;;) {
      // We need to check if the filepath exists and is a symlink.
      // If the initial filepath doesn't exist then early exit.
      // If it did exist at some state then trace it all all the way to the final link.
      int Result = fstatat(RootFSFD, &SubPath[1], &Buffer, AT_SYMLINK_NOFOLLOW);
      if (Result != 0 && errno == ENOENT && !HadAtLeastOne) {
        // Initial file didn't exist at all
        return NoEntry;
      }

      const bool IsLink = Result == 0 && S_ISLNK(Buffer.st_mode);

      HadAtLeastOne = true;

      if (IsLink) {
        // Choose the current temporary working path.
        auto CurrentTmp = TmpPaths[CurrentIndex];

        // Get the symlink of RootFS FD + stripped subpath.
        auto SymlinkSize = FEX::HLE::GetSymlink(RootFSFD, &SubPath[1], CurrentTmp, PATH_MAX - 1);

        // This might be a /proc symlink into the RootFS, so strip it in that case.
        SymlinkSize = StripRootFSPrefix(CurrentTmp, SymlinkSize, false);

        if (SymlinkSize > 1 && CurrentTmp[0] == '/') {
          // If the symlink is absolute and not the root:
          // 1) Zero terminate it.
          // 2) Set the path as our current subpath.
          // 3) Switch to the next temporary index. (We don't want to overwrite the current one on the next loop iteration).
          // 4) Run the loop again.
          CurrentTmp[SymlinkSize] = 0;
          SubPath = CurrentTmp;
          CurrentIndex ^= 1;
        } else {
          // If the path wasn't a symlink or wasn't absolute.
          // 1) Break early, returning the previous found result.
          // 2) If first iteration then we return `pathname`.
          break;
        }
      } else {
        break;
      }
    }
  }

  // Return the pair of rootfs FD plus relative subpath by stripping off the front '/'
  return std::make_pair(RootFSFD, &SubPath[1]);
}

///< Returns true if the pathname is self and symlink flags are set NOFOLLOW.
bool FileManager::IsSelfNoFollow(const char* Pathname, int flags) const {
  const bool Follow = (flags & AT_SYMLINK_NOFOLLOW) == 0;
  if (Follow) {
    // If we are following the self symlink then we don't care about this.
    return false;
  }

  if (!Pathname) {
    return false;
  }

  char PidSelfPath[50];
  snprintf(PidSelfPath, sizeof(PidSelfPath), "/proc/%i/exe", CurrentPID);

  return strcmp(Pathname, "/proc/self/exe") == 0 || strcmp(Pathname, "/proc/thread-self/exe") == 0 || strcmp(Pathname, PidSelfPath) == 0;
}

std::optional<std::string_view> FileManager::GetSelf(const char* Pathname) {
  if (!Pathname) {
    return std::nullopt;
  }

  char PidSelfPath[50];
  snprintf(PidSelfPath, sizeof(PidSelfPath), "/proc/%i/exe", CurrentPID);

  if (strcmp(Pathname, "/proc/self/exe") == 0 || strcmp(Pathname, "/proc/thread-self/exe") == 0 || strcmp(Pathname, PidSelfPath) == 0) {
    return Filename();
  }

  return Pathname;
}

static bool ShouldSkipOpenInEmu(int flags) {
  if (flags & O_CREAT) {
    // If trying to create a file then skip checking in emufd
    return true;
  }

  if (flags & O_WRONLY) {
    // If the file is trying to be open with write permissions then skip.
    return true;
  }

  if (flags & O_APPEND) {
    // If the file is trying to be open with append options then skip.
    return true;
  }

  return false;
}

bool FileManager::ReplaceEmuFd(int fd, int flags, uint32_t mode) {
  char Tmp[PATH_MAX + 1];

  if (fd < 0) {
    return false;
  }

  // Get the path of the file we just opened
  auto PathLength = FEX::get_fdpath(fd, Tmp);
  if (PathLength == -1) {
    return false;
  }
  Tmp[PathLength] = '\0';

  // And try to open via EmuFD
  auto EmuFd = EmuFD.Open(Tmp, flags, mode);
  if (EmuFd == -1) {
    return false;
  }

  // If we succeeded, swap out the fd
  ::dup2(EmuFd, fd);
  ::close(EmuFd);
  return true;
}

uint64_t FileManager::Open(const char* pathname, int flags, uint32_t mode) {
  auto NewPath = GetSelf(pathname);
  const char* SelfPath = NewPath ? NewPath->data() : nullptr;
  int fd = -1;

  if (!ShouldSkipOpenInEmu(flags)) {
    FDPathTmpData TmpFilename;
    auto Path = GetEmulatedFDPath(AT_FDCWD, SelfPath, false, TmpFilename);
    if (Path.first != -1) {
      FEX::HLE::open_how how = {
        .flags = (uint64_t)flags,
        .mode = (flags & (O_CREAT | O_TMPFILE)) ? mode & 07777 : 0, // openat2() is stricter about this
        .resolve = (Path.first == AT_FDCWD) ? 0u : RESOLVE_IN_ROOT, // AT_FDCWD means it's a thunk and not via RootFS
      };
      fd = ::syscall(SYSCALL_DEF(openat2), Path.first, Path.second, &how, sizeof(how));

      if (fd == -1 && errno == EXDEV) {
        // This means a magic symlink (/proc/foo) was involved. In this case we
        // just punt and do the access without RESOLVE_IN_ROOT.
        fd = ::syscall(SYSCALL_DEF(openat), Path.first, Path.second, flags, mode);
      }
    }

    // Open through RootFS failed (probably nonexistent), so open directly.
    if (fd == -1) {
      fd = ::open(SelfPath, flags, mode);
    }

    ReplaceEmuFd(fd, flags, mode);
  } else {
    fd = ::open(SelfPath, flags, mode);
  }

  return fd;
}

uint64_t FileManager::Close(int fd) {
#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED
  if (CheckIfFDInTrackedSet(fd)) {
    LogMan::Msg::EFmt("{} closing FEX FD {}", __func__, fd);
    RemoveFEXFD(fd);
  }
#endif

  return ::close(fd);
}

uint64_t FileManager::CloseRange(unsigned int first, unsigned int last, unsigned int flags) {
#ifndef CLOSE_RANGE_CLOEXEC
#define CLOSE_RANGE_CLOEXEC (1U << 2)
#endif
#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED
  if (!(flags & CLOSE_RANGE_CLOEXEC) && CheckIfFDRangeInTrackedSet(first, last)) {
    LogMan::Msg::EFmt("{} closing FEX FDs in range ({}, {})", __func__, first, last);
    RemoveFEXFDRange(first, last);
  }
#endif

  return ::syscall(SYSCALL_DEF(close_range), first, last, flags);
}

uint64_t FileManager::Stat(const char* pathname, void* buf) {
  auto NewPath = GetSelf(pathname);
  const char* SelfPath = NewPath ? NewPath->data() : nullptr;

  // Stat follows symlinks
  FDPathTmpData TmpFilename;
  auto Path = GetEmulatedFDPath(AT_FDCWD, SelfPath, true, TmpFilename);
  if (Path.first != -1) {
    uint64_t Result = ::fstatat(Path.first, Path.second, reinterpret_cast<struct stat*>(buf), 0);
    if (Result != -1) {
      return Result;
    }
  }
  return ::stat(SelfPath, reinterpret_cast<struct stat*>(buf));
}

uint64_t FileManager::Lstat(const char* pathname, void* buf) {
  auto NewPath = GetSelf(pathname);
  const char* SelfPath = NewPath ? NewPath->data() : nullptr;

  // lstat does not follow symlinks
  FDPathTmpData TmpFilename;
  auto Path = GetEmulatedFDPath(AT_FDCWD, SelfPath, false, TmpFilename);
  if (Path.first != -1) {
    uint64_t Result = ::fstatat(Path.first, Path.second, reinterpret_cast<struct stat*>(buf), AT_SYMLINK_NOFOLLOW);
    if (Result != -1) {
      return Result;
    }
  }

  return ::lstat(pathname, reinterpret_cast<struct stat*>(buf));
}

uint64_t FileManager::Access(const char* pathname, [[maybe_unused]] int mode) {
  auto NewPath = GetSelf(pathname);
  const char* SelfPath = NewPath ? NewPath->data() : nullptr;

  // Access follows symlinks
  FDPathTmpData TmpFilename;
  auto Path = GetEmulatedFDPath(AT_FDCWD, SelfPath, true, TmpFilename);
  if (Path.first != -1) {
    uint64_t Result = ::faccessat(Path.first, Path.second, mode, 0);
    if (Result != -1) {
      return Result;
    }
  }
  return ::access(SelfPath, mode);
}

uint64_t FileManager::FAccessat(int dirfd, const char* pathname, int mode) {
  auto NewPath = GetSelf(pathname);
  const char* SelfPath = NewPath ? NewPath->data() : nullptr;

  FDPathTmpData TmpFilename;
  auto Path = GetEmulatedFDPath(dirfd, SelfPath, true, TmpFilename);
  if (Path.first != -1) {
    uint64_t Result = ::syscall(SYSCALL_DEF(faccessat), Path.first, Path.second, mode);
    if (Result != -1) {
      return Result;
    }
  }

  return ::syscall(SYS_faccessat, dirfd, SelfPath, mode);
}

uint64_t FileManager::FAccessat2(int dirfd, const char* pathname, int mode, int flags) {
  auto NewPath = GetSelf(pathname);
  const char* SelfPath = NewPath ? NewPath->data() : nullptr;

  FDPathTmpData TmpFilename;
  auto Path = GetEmulatedFDPath(dirfd, SelfPath, (flags & AT_SYMLINK_NOFOLLOW) == 0, TmpFilename);
  if (Path.first != -1) {
    uint64_t Result = ::syscall(SYSCALL_DEF(faccessat2), Path.first, Path.second, mode, flags);
    if (Result != -1) {
      return Result;
    }
  }

  return ::syscall(SYSCALL_DEF(faccessat2), dirfd, SelfPath, mode, flags);
}

uint64_t FileManager::Readlink(const char* pathname, char* buf, size_t bufsiz) {
  // calculate the non-self link to exe
  // Some executables do getpid, stat("/proc/$pid/exe")
  char PidSelfPath[50];
  snprintf(PidSelfPath, 50, "/proc/%i/exe", CurrentPID);

  if (strcmp(pathname, "/proc/self/exe") == 0 || strcmp(pathname, "/proc/thread-self/exe") == 0 || strcmp(pathname, PidSelfPath) == 0) {
    const auto& App = Filename();
    strncpy(buf, App.c_str(), bufsiz);
    return std::min(bufsiz, App.size());
  }

  FDPathTmpData TmpFilename;
  auto Path = GetEmulatedFDPath(AT_FDCWD, pathname, false, TmpFilename);
  uint64_t Result = -1;
  if (Path.first != -1) {
    Result = ::readlinkat(Path.first, Path.second, buf, bufsiz);

    if (Result == -1 && errno == EINVAL) {
      // This means that the file wasn't a symlink
      // This is expected behaviour
      return -1;
    }
  }
  if (Result == -1) {
    Result = ::readlink(pathname, buf, bufsiz);
  }

  // We might have read a /proc/self/fd/* link. If so, strip the RootFS prefix from it.
  return StripRootFSPrefix(buf, Result, true);
}

uint64_t FileManager::Chmod(const char* pathname, mode_t mode) {
  auto NewPath = GetSelf(pathname);
  const char* SelfPath = NewPath ? NewPath->data() : nullptr;

  FDPathTmpData TmpFilename;
  auto Path = GetEmulatedFDPath(AT_FDCWD, SelfPath, false, TmpFilename);
  if (Path.first != -1) {
    uint64_t Result = ::fchmodat(Path.first, Path.second, mode, 0);
    if (Result != -1) {
      return Result;
    }
  }
  return ::chmod(SelfPath, mode);
}

uint64_t FileManager::Readlinkat(int dirfd, const char* pathname, char* buf, size_t bufsiz) {
  // calculate the non-self link to exe
  // Some executables do getpid, stat("/proc/$pid/exe")
  // Can't use `GetSelf` directly here since readlink{at,} returns EINVAL if it isn't a symlink
  // Self is always a symlink and isn't expected to fail

  fextl::string Path {};
  if (((pathname && pathname[0] != '/') || // If pathname exists then it must not be absolute
       !pathname) &&
      dirfd != AT_FDCWD) {
    // Passed in a dirfd that isn't magic FDCWD
    // We need to get the path from the fd now
    char Tmp[PATH_MAX] = "";
    auto PathLength = FEX::get_fdpath(dirfd, Tmp);
    if (PathLength != -1) {
      Path = fextl::string(Tmp, PathLength);
    }

    if (pathname) {
      if (!Path.empty()) {
        // If the path returned empty then we don't need a separator
        Path += "/";
      }
      Path += pathname;
    }
  } else {
    if (!pathname || strlen(pathname) == 0) {
      return -1;
    } else if (pathname) {
      Path = pathname;
    }
  }

  char PidSelfPath[50];
  snprintf(PidSelfPath, 50, "/proc/%i/exe", CurrentPID);

  if (Path == "/proc/self/exe" || Path == "/proc/thread-self/exe" || Path == PidSelfPath) {
    const auto& App = Filename();
    strncpy(buf, App.c_str(), bufsiz);
    return std::min(bufsiz, App.size());
  }

  FDPathTmpData TmpFilename;
  auto NewPath = GetEmulatedFDPath(dirfd, pathname, false, TmpFilename);
  uint64_t Result = -1;

  if (NewPath.first != -1) {
    Result = ::readlinkat(NewPath.first, NewPath.second, buf, bufsiz);

    if (Result == -1 && errno == EINVAL) {
      // This means that the file wasn't a symlink
      // This is expected behaviour
      return -1;
    }
  }

  if (Result == -1) {
    Result = ::readlinkat(dirfd, pathname, buf, bufsiz);
  }

  // We might have read a /proc/self/fd/* link. If so, strip the RootFS prefix from it.
  return StripRootFSPrefix(buf, Result, true);
}

uint64_t FileManager::Openat([[maybe_unused]] int dirfs, const char* pathname, int flags, uint32_t mode) {
  auto NewPath = GetSelf(pathname);
  const char* SelfPath = NewPath ? NewPath->data() : nullptr;

  int32_t fd = -1;

  if (!ShouldSkipOpenInEmu(flags)) {
    FDPathTmpData TmpFilename;
    auto Path = GetEmulatedFDPath(dirfs, SelfPath, false, TmpFilename);
    if (Path.first != -1) {
      FEX::HLE::open_how how = {
        .flags = (uint64_t)flags,
        .mode = (flags & (O_CREAT | O_TMPFILE)) ? mode & 07777 : 0, // openat2() is stricter about this,
        .resolve = (Path.first == AT_FDCWD) ? 0u : RESOLVE_IN_ROOT, // AT_FDCWD means it's a thunk and not via RootFS
      };
      fd = ::syscall(SYSCALL_DEF(openat2), Path.first, Path.second, &how, sizeof(how));
      if (fd == -1 && errno == EXDEV) {
        // This means a magic symlink (/proc/foo) was involved. In this case we
        // just punt and do the access without RESOLVE_IN_ROOT.
        fd = ::syscall(SYSCALL_DEF(openat), Path.first, Path.second, flags, mode);
      }
    }

    // Open through RootFS failed (probably nonexistent), so open directly.
    if (fd == -1) {
      fd = ::syscall(SYSCALL_DEF(openat), dirfs, SelfPath, flags, mode);
    }

    ReplaceEmuFd(fd, flags, mode);
  } else {
    fd = ::syscall(SYSCALL_DEF(openat), dirfs, SelfPath, flags, mode);
  }

  return fd;
}

uint64_t FileManager::Openat2(int dirfs, const char* pathname, FEX::HLE::open_how* how, size_t usize) {
  auto NewPath = GetSelf(pathname);
  const char* SelfPath = NewPath ? NewPath->data() : nullptr;

  int32_t fd = -1;

  if (!ShouldSkipOpenInEmu(how->flags)) {
    FDPathTmpData TmpFilename;
    auto Path = GetEmulatedFDPath(dirfs, SelfPath, false, TmpFilename);
    if (Path.first != -1 && !(how->resolve & RESOLVE_IN_ROOT)) {
      // AT_FDCWD means it's a thunk and not via RootFS
      if (Path.first != AT_FDCWD) {
        how->resolve |= RESOLVE_IN_ROOT;
      }
      fd = ::syscall(SYSCALL_DEF(openat2), Path.first, Path.second, how, usize);
      how->resolve &= RESOLVE_IN_ROOT;
      if (fd == -1 && errno == EXDEV) {
        // This means a magic symlink (/proc/foo) was involved. In this case we
        // just punt and do the access without RESOLVE_IN_ROOT.
        fd = ::syscall(SYSCALL_DEF(openat2), Path.first, Path.second, how, usize);
      }
    }

    // Open through RootFS failed (probably nonexistent), so open directly.
    if (fd == -1) {
      fd = ::syscall(SYSCALL_DEF(openat2), dirfs, SelfPath, how, usize);
    }

    ReplaceEmuFd(fd, how->flags, how->mode);
  } else {
    fd = ::syscall(SYSCALL_DEF(openat2), dirfs, SelfPath, how, usize);
  }

  return fd;
}

uint64_t FileManager::Statx(int dirfd, const char* pathname, int flags, uint32_t mask, struct statx* statxbuf) {
  if (IsSelfNoFollow(pathname, flags)) {
    // If we aren't following the symlink for self then we need to return data about the symlink itself.
    // Let's just /actually/ return FEXInterpreter symlink information in this case.
    return FHU::Syscalls::statx(dirfd, pathname, flags, mask, statxbuf);
  }

  auto NewPath = GetSelf(pathname);
  const char* SelfPath = NewPath ? NewPath->data() : nullptr;

  FDPathTmpData TmpFilename;
  auto Path = GetEmulatedFDPath(dirfd, SelfPath, (flags & AT_SYMLINK_NOFOLLOW) == 0, TmpFilename);
  if (Path.first != -1) {
    uint64_t Result = FHU::Syscalls::statx(Path.first, Path.second, flags, mask, statxbuf);
    if (Result != -1) {
      return Result;
    }
  }
  return FHU::Syscalls::statx(dirfd, SelfPath, flags, mask, statxbuf);
}

uint64_t FileManager::Mknod(const char* pathname, mode_t mode, dev_t dev) {
  auto NewPath = GetSelf(pathname);
  const char* SelfPath = NewPath ? NewPath->data() : nullptr;

  FDPathTmpData TmpFilename;
  auto Path = GetEmulatedFDPath(AT_FDCWD, SelfPath, false, TmpFilename);
  if (Path.first != -1) {
    uint64_t Result = ::mknodat(Path.first, Path.second, mode, dev);
    if (Result != -1) {
      return Result;
    }
  }
  return ::mknod(SelfPath, mode, dev);
}

uint64_t FileManager::Statfs(const char* path, void* buf) {
  auto Path = GetEmulatedPath(path);
  if (!Path.empty()) {
    uint64_t Result = ::statfs(Path.c_str(), reinterpret_cast<struct statfs*>(buf));
    if (Result != -1) {
      return Result;
    }
  }
  return ::statfs(path, reinterpret_cast<struct statfs*>(buf));
}

uint64_t FileManager::NewFSStatAt(int dirfd, const char* pathname, struct stat* buf, int flag) {
  if (IsSelfNoFollow(pathname, flag)) {
    // See Statx
    return ::fstatat(dirfd, pathname, buf, flag);
  }

  auto NewPath = GetSelf(pathname);
  const char* SelfPath = NewPath ? NewPath->data() : nullptr;

  FDPathTmpData TmpFilename;
  auto Path = GetEmulatedFDPath(dirfd, SelfPath, (flag & AT_SYMLINK_NOFOLLOW) == 0, TmpFilename);
  if (Path.first != -1) {
    uint64_t Result = ::fstatat(Path.first, Path.second, buf, flag);
    if (Result != -1) {
      return Result;
    }
  }
  return ::fstatat(dirfd, SelfPath, buf, flag);
}

uint64_t FileManager::NewFSStatAt64(int dirfd, const char* pathname, struct stat64* buf, int flag) {
  if (IsSelfNoFollow(pathname, flag)) {
    // See Statx
    return ::fstatat64(dirfd, pathname, buf, flag);
  }

  auto NewPath = GetSelf(pathname);
  const char* SelfPath = NewPath ? NewPath->data() : nullptr;

  FDPathTmpData TmpFilename;
  auto Path = GetEmulatedFDPath(dirfd, SelfPath, (flag & AT_SYMLINK_NOFOLLOW) == 0, TmpFilename);
  if (Path.first != -1) {
    uint64_t Result = ::fstatat64(Path.first, Path.second, buf, flag);
    if (Result != -1) {
      return Result;
    }
  }
  return ::fstatat64(dirfd, SelfPath, buf, flag);
}

uint64_t FileManager::Setxattr(const char* path, const char* name, const void* value, size_t size, int flags) {
  auto NewPath = GetSelf(path);
  const char* SelfPath = NewPath ? NewPath->data() : nullptr;

  auto Path = GetEmulatedPath(SelfPath, true);
  if (!Path.empty()) {
    uint64_t Result = ::setxattr(Path.c_str(), name, value, size, flags);
    if (Result != -1 || errno != ENOENT) {
      return Result;
    }
  }

  return ::setxattr(SelfPath, name, value, size, flags);
}

uint64_t FileManager::LSetxattr(const char* path, const char* name, const void* value, size_t size, int flags) {
  auto NewPath = GetSelf(path);
  const char* SelfPath = NewPath ? NewPath->data() : nullptr;

  auto Path = GetEmulatedPath(SelfPath, false);
  if (!Path.empty()) {
    uint64_t Result = ::lsetxattr(Path.c_str(), name, value, size, flags);
    if (Result != -1 || errno != ENOENT) {
      return Result;
    }
  }

  return ::lsetxattr(SelfPath, name, value, size, flags);
}

uint64_t FileManager::Getxattr(const char* path, const char* name, void* value, size_t size) {
  auto NewPath = GetSelf(path);
  const char* SelfPath = NewPath ? NewPath->data() : nullptr;

  auto Path = GetEmulatedPath(SelfPath, true);
  if (!Path.empty()) {
    uint64_t Result = ::getxattr(Path.c_str(), name, value, size);
    if (Result != -1 || errno != ENOENT) {
      return Result;
    }
  }

  return ::getxattr(SelfPath, name, value, size);
}

uint64_t FileManager::LGetxattr(const char* path, const char* name, void* value, size_t size) {
  auto NewPath = GetSelf(path);
  const char* SelfPath = NewPath ? NewPath->data() : nullptr;

  auto Path = GetEmulatedPath(SelfPath, false);
  if (!Path.empty()) {
    uint64_t Result = ::lgetxattr(Path.c_str(), name, value, size);
    if (Result != -1 || errno != ENOENT) {
      return Result;
    }
  }

  return ::lgetxattr(SelfPath, name, value, size);
}

uint64_t FileManager::Listxattr(const char* path, char* list, size_t size) {
  auto NewPath = GetSelf(path);
  const char* SelfPath = NewPath ? NewPath->data() : nullptr;

  auto Path = GetEmulatedPath(SelfPath, true);
  if (!Path.empty()) {
    uint64_t Result = ::listxattr(Path.c_str(), list, size);
    if (Result != -1 || errno != ENOENT) {
      return Result;
    }
  }

  return ::listxattr(SelfPath, list, size);
}

uint64_t FileManager::LListxattr(const char* path, char* list, size_t size) {
  auto NewPath = GetSelf(path);
  const char* SelfPath = NewPath ? NewPath->data() : nullptr;

  auto Path = GetEmulatedPath(SelfPath, false);
  if (!Path.empty()) {
    uint64_t Result = ::llistxattr(Path.c_str(), list, size);
    if (Result != -1 || errno != ENOENT) {
      return Result;
    }
  }

  return ::llistxattr(SelfPath, list, size);
}

uint64_t FileManager::Removexattr(const char* path, const char* name) {
  auto NewPath = GetSelf(path);
  const char* SelfPath = NewPath ? NewPath->data() : nullptr;

  auto Path = GetEmulatedPath(SelfPath, true);
  if (!Path.empty()) {
    uint64_t Result = ::removexattr(Path.c_str(), name);
    if (Result != -1 || errno != ENOENT) {
      return Result;
    }
  }

  return ::removexattr(SelfPath, name);
}

uint64_t FileManager::LRemovexattr(const char* path, const char* name) {
  auto NewPath = GetSelf(path);
  const char* SelfPath = NewPath ? NewPath->data() : nullptr;

  auto Path = GetEmulatedPath(SelfPath, false);
  if (!Path.empty()) {
    uint64_t Result = ::lremovexattr(Path.c_str(), name);
    if (Result != -1 || errno != ENOENT) {
      return Result;
    }
  }

  return ::lremovexattr(SelfPath, name);
}

void FileManager::UpdatePID(uint32_t PID) {
  CurrentPID = PID;

  // Track the inode of /proc/self/fd/<RootFSFD>, to be able to hide it
  auto FDpath = fextl::fmt::format("self/fd/{}", RootFSFD);
  struct stat Buffer {};
  int Result = fstatat(ProcFD, FDpath.c_str(), &Buffer, AT_SYMLINK_NOFOLLOW);
  if (Result >= 0) {
    RootFSFDInode = Buffer.st_ino;
  } else {
    // Probably in a strict sandbox
    RootFSFDInode = 0;
    ProcFDInode = 0;
    return;
  }

  // And track the ProcFSFD itself
  FDpath = fextl::fmt::format("self/fd/{}", ProcFD);
  Result = fstatat(ProcFD, FDpath.c_str(), &Buffer, AT_SYMLINK_NOFOLLOW);
  if (Result >= 0) {
    ProcFDInode = Buffer.st_ino;
  } else {
    // ??
    ProcFDInode = 0;
    return;
  }
}

bool FileManager::IsRootFSFD(int dirfd, uint64_t inode) {

  // Check if we have to hide this entry
  if (inode == RootFSFDInode || inode == ProcFDInode) {
    struct stat Buffer;
    if (fstat(dirfd, &Buffer) >= 0) {
      if (Buffer.st_dev == ProcFSDev) {
        LogMan::Msg::DFmt("Hiding directory entry for RootFSFD");
        return true;
      }
    }
  }
  return false;
}

} // namespace FEX::HLE
