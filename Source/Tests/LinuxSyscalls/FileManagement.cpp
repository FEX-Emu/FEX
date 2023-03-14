/*
$info$
tags: LinuxSyscalls|common
desc: Rootfs overlay logic
$end_info$
*/

#include "Common/FDUtils.h"

#include "FEXCore/Config/Config.h"
#include "Tests/LinuxSyscalls/FileManagement.h"
#include "Tests/LinuxSyscalls/EmulatedFiles/EmulatedFiles.h"
#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/x64/Syscalls.h"

#include <FEXCore/Common/Paths.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/fextl/vector.h>
#include <FEXHeaderUtils/ScopedSignalMask.h>
#include <FEXHeaderUtils/Syscalls.h>

#include <algorithm>
#include <errno.h>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <optional>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <syscall.h>
#include <system_error>
#include <unistd.h>
#include <utility>
#include <vector>

#include <tiny-json.h>

namespace JSON {
  struct JsonAllocator {
    jsonPool_t PoolObject;
    std::unique_ptr<std::list<json_t>> json_objects;
  };
  static_assert(offsetof(JsonAllocator, PoolObject) == 0, "This needs to be at offset zero");

  json_t* PoolInit(jsonPool_t* Pool) {
    JsonAllocator* alloc = reinterpret_cast<JsonAllocator*>(Pool);
    alloc->json_objects = std::make_unique<std::list<json_t>>();
    return &*alloc->json_objects->emplace(alloc->json_objects->end());
  }

  json_t* PoolAlloc(jsonPool_t* Pool) {
    JsonAllocator* alloc = reinterpret_cast<JsonAllocator*>(Pool);
    return &*alloc->json_objects->emplace(alloc->json_objects->end());
  }
}

namespace FEX::HLE {
  struct open_how;

static bool LoadFile(fextl::vector<char> &Data, const std::string &Filename) {
  std::fstream File(Filename, std::ios::in);

  if (!File.is_open()) {
    return false;
  }

  if (!File.seekg(0, std::fstream::end)) {
    LogMan::Msg::DFmt("Couldn't load configuration file: Seek end");
    return false;
  }

  auto FileSize = File.tellg();
  if (File.fail()) {
    LogMan::Msg::DFmt("Couldn't load configuration file: tellg");
    return false;
  }

  if (!File.seekg(0, std::fstream::beg)) {
    LogMan::Msg::DFmt("Couldn't load configuration file: Seek beginning");
    return false;
  }

  if (FileSize <= 0) {
    LogMan::Msg::DFmt("FileSize less than or equal to zero specified");
    return false;
  }

  Data.resize(FileSize);
  if (!File.read(Data.data(), FileSize)) {
    // Probably means permissions aren't set. Just early exit
    return false;
  }
  return true;
}

struct ThunkDBObject {
  std::string LibraryName;
  std::unordered_set<std::string> Depends;
  fextl::vector<std::string> Overlays;
  bool Enabled{};
};

static void LoadThunkDatabase(std::unordered_map<std::string, ThunkDBObject>& ThunkDB, bool Is64BitMode, bool Global) {
  auto ThunkDBPath = FEXCore::Config::GetConfigDirectory(Global) + "ThunksDB.json";
  fextl::vector<char> FileData;
  if (LoadFile(FileData, ThunkDBPath)) {
    FileData.push_back(0);

    JSON::JsonAllocator Pool {
      .PoolObject = {
        .init = JSON::PoolInit,
        .alloc = JSON::PoolAlloc,
      },
    };

    json_t const *json = json_createWithPool(&FileData.at(0), &Pool.PoolObject);

    json_t const* DB = json_getProperty( json, "DB" );
    if ( !DB || JSON_OBJ != json_getType( DB ) ) {
      return;
    }

    std::string_view HomeDirectory = FEXCore::Paths::GetHomeDirectory();

    for( json_t const* Library = json_getChild( DB ); Library != nullptr; Library = json_getSibling( Library )) {
      // Get the user defined name for the library
      const char* LibraryName = json_getName(Library);
      auto DBObject = ThunkDB.insert_or_assign(LibraryName, ThunkDBObject{}).first;

      // Walk the libraries items to get the data
      for (json_t const* LibraryItem = json_getChild(Library); LibraryItem != nullptr; LibraryItem = json_getSibling(LibraryItem)) {
        std::string_view ItemName = json_getName(LibraryItem);

        if (ItemName == "Library") {
          // "Library": "libGL-guest.so"
          DBObject->second.LibraryName = json_getValue(LibraryItem);
        }
        else if (ItemName == "Depends") {
          jsonType_t PropertyType = json_getType(LibraryItem);
          if (PropertyType == JSON_TEXT) {
            DBObject->second.Depends.insert(json_getValue(LibraryItem));
          }
          else if (PropertyType == JSON_ARRAY) {
            for (json_t const* Depend = json_getChild(LibraryItem); Depend != nullptr; Depend = json_getSibling(Depend)) {
              DBObject->second.Depends.insert(json_getValue(Depend));
            }
          }
        }
        else if (ItemName == "Overlay") {
          auto AddWithReplacement = [Is64BitMode, HomeDirectory](ThunkDBObject& DBObject, std::string LibraryItem) {
            constexpr static std::array<std::string_view, 4> LibPrefixes = {
              "/usr/lib",
              "/usr/local/lib",
              "/lib",
              "/usr/lib/pressure-vessel/overrides/lib",
            };

            constexpr static std::array<std::string_view, 2> ArchPrefixes = {
              "i386",
              "x86_64",
            };

            // Walk through template string and fill in prefixes from right to left

            using namespace std::string_view_literals;
            const std::pair PrefixArch { "@PREFIX_ARCH@"sv, LibraryItem.find("@PREFIX_ARCH@") };
            const std::pair PrefixHome { "@HOME@"sv, LibraryItem.find("@HOME@") };
            const std::pair PrefixLib { "@PREFIX_LIB@"sv, LibraryItem.find("@PREFIX_LIB@") };

            std::string::size_type PrefixPositions[] = {
              PrefixArch.second, PrefixHome.second, PrefixLib.second,
            };
            // Sort offsets in descending order to enable safe in-place replacement
            std::sort(std::begin(PrefixPositions), std::end(PrefixPositions), std::greater<>{});

            for (auto& LibPrefix : LibPrefixes) {
              std::string Replacement = LibraryItem;
              for (auto PrefixPos : PrefixPositions) {
                if (PrefixPos == std::string::npos) {
                  continue;
                } else if (PrefixPos == PrefixArch.second) {
                  Replacement.replace(PrefixPos, PrefixArch.first.size(), ArchPrefixes[Is64BitMode]);
                } else if (PrefixPos == PrefixHome.second) {
                  Replacement.replace(PrefixPos, PrefixHome.first.size(), HomeDirectory);
                } else if (PrefixPos == PrefixLib.second) {
                  Replacement.replace(PrefixPos, PrefixLib.first.size(), LibPrefix);
                }
              }
              DBObject.Overlays.emplace_back(std::move(Replacement));

              if (PrefixLib.second == std::string::npos) {
                // Don't repeat for other LibPrefixes entries if the prefix wasn't used
                break;
              }
            }
          };

          jsonType_t PropertyType = json_getType(LibraryItem);
          if (PropertyType == JSON_TEXT) {
            AddWithReplacement(DBObject->second, json_getValue(LibraryItem));
          }
          else if (PropertyType == JSON_ARRAY) {
            for (json_t const* Overlay = json_getChild(LibraryItem); Overlay != nullptr; Overlay = json_getSibling(Overlay)) {
              AddWithReplacement(DBObject->second, json_getValue(Overlay));
            }
          }
        }
      }
    }
  }
}

FileManager::FileManager(FEXCore::Context::Context *ctx)
  : EmuFD {ctx} {

  auto ThunkConfigFile = ThunkConfig();

  // We try to load ThunksDB from:
  // - FEX global config
  // - FEX user config
  // - Defined ThunksConfig option
  // - Steam AppConfig Global
  // - AppConfig Global
  // - Steam AppConfig Local
  // - AppConfig Local
  // This doesn't support the classic thunks interface.

  auto AppName = AppConfigName();
  fextl::vector<std::string> ConfigPaths {
    FEXCore::Config::GetConfigFileLocation(true),
    FEXCore::Config::GetConfigFileLocation(false),
    ThunkConfigFile,
  };

  auto SteamID = getenv("SteamAppId");
  if (SteamID) {
    // If a SteamID exists then let's search for Steam application configs as well.
    // We want to key off both the SteamAppId number /and/ the executable since we may not want to thunk all binaries.
    auto SteamAppName = fmt::format("Steam_{}_{}", SteamID, AppName);

    // Steam application configs interleaved with non-steam for priority sorting.
    ConfigPaths.emplace_back(FEXCore::Config::GetApplicationConfig(SteamAppName, true));
    ConfigPaths.emplace_back(FEXCore::Config::GetApplicationConfig(AppName, true));
    ConfigPaths.emplace_back(FEXCore::Config::GetApplicationConfig(SteamAppName, false));
    ConfigPaths.emplace_back(FEXCore::Config::GetApplicationConfig(AppName, false));
  }
  else {
    ConfigPaths.emplace_back(FEXCore::Config::GetApplicationConfig(AppName, true));
    ConfigPaths.emplace_back(FEXCore::Config::GetApplicationConfig(AppName, false));
  }

  std::unordered_map<std::string, ThunkDBObject> ThunkDB;
  LoadThunkDatabase(ThunkDB, Is64BitMode(), true);
  LoadThunkDatabase(ThunkDB, Is64BitMode(), false);

  for (const auto &Path : ConfigPaths) {
    fextl::vector<char> FileData;
    if (LoadFile(FileData, Path)) {
      JSON::JsonAllocator Pool {
        .PoolObject = {
          .init = JSON::PoolInit,
          .alloc = JSON::PoolAlloc,
        },
      };

      // If a thunks DB property exists then we pull in data from the thunks database
      json_t const *json = json_createWithPool(&FileData.at(0), &Pool.PoolObject);
      json_t const* ThunksDB = json_getProperty( json, "ThunksDB" );
      if (!ThunksDB) {
        continue;
      }

      for (json_t const* Item = json_getChild(ThunksDB); Item != nullptr; Item = json_getSibling(Item)) {
        const char *LibraryName = json_getName(Item);
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
  auto ThunkGuestPath = std::filesystem::path { Is64BitMode() ? ThunkGuestLibs() : ThunkGuestLibs32() };
  for (auto const &DBObject : ThunkDB) {
    if (!DBObject.second.Enabled) {
      continue;
    }

    // Recursively add paths for this thunk library and its dependencies to ThunkOverlays.
    // Using a local struct for this is slightly less ugly than using self-capturing lambdas
    struct {
      decltype(FileManager::ThunkOverlays)& ThunkOverlays;
      decltype(ThunkDB)& ThunkDB;
      const std::filesystem::path& ThunkGuestPath;
      bool Is64BitMode;

      void SetupOverlay(const ThunkDBObject& DBDepend) {
          auto ThunkPath = ThunkGuestPath / DBDepend.LibraryName;
          if (!std::filesystem::exists(ThunkPath)) {
            if (!Is64BitMode) {
              // Guest libraries not existing is expected since not all libraries are thunked on 32-bit
              return;
            }
            ERROR_AND_DIE_FMT("Requested thunking via guest library \"{}\" that does not exist", ThunkPath.string());
          }

          for (const auto& Overlay : DBDepend.Overlays) {
            // Direct full path in guest RootFS to our overlay file
            ThunkOverlays.emplace(Overlay, ThunkPath);
          }
      };

      void InsertDependencies(const std::unordered_set<std::string> &Depends) {
        for (auto const &Depend : Depends) {
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
    } DBObjectHandler { ThunkOverlays, ThunkDB, ThunkGuestPath, Is64BitMode() };

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

  UpdatePID(::getpid());

  if (!LDPath().empty()) {
    RootFSFD = open(LDPath().c_str(), O_DIRECTORY | O_PATH | O_CLOEXEC);
    if (RootFSFD == -1) {
      RootFSFD = AT_FDCWD;
    }
  }
}

FileManager::~FileManager() {
  close(RootFSFD);
}

std::string FileManager::GetEmulatedPath(const char *pathname, bool FollowSymlink) {
  if (!pathname || // If no pathname
      pathname[0] != '/' || // If relative
      strcmp(pathname, "/") == 0) { // If we are getting root
    return {};
  }

  auto thunkOverlay = ThunkOverlays.find(pathname);
  if (thunkOverlay != ThunkOverlays.end()) {
    return thunkOverlay->second;
  }

  auto RootFSPath = LDPath();
  if (RootFSPath.empty()) { // If RootFS doesn't exist
    return {};
  }

  std::string Path = RootFSPath + pathname;
  if (FollowSymlink) {
    char Filename[PATH_MAX];
    while(FEX::HLE::IsSymlink(AT_FDCWD, Path.c_str())) {
      auto SymlinkSize = FEX::HLE::GetSymlink(AT_FDCWD, Path.c_str(), Filename, PATH_MAX - 1);
      if (SymlinkSize > 0 && Filename[0] == '/') {
        Path = RootFSPath;
        Path += std::string_view(Filename, SymlinkSize);
      }
      else {
        break;
      }
    }
  }
  return Path;
}

std::pair<int, const char*> FileManager::GetEmulatedFDPath(int dirfd, const char *pathname, bool FollowSymlink, FDPathTmpData &TmpFilename) {
  constexpr auto NoEntry = std::make_pair(-1, nullptr);

  if (!pathname || // If no pathname
      pathname[0] != '/' || // If relative
      pathname[1] == 0 || // If we are getting root
      dirfd != AT_FDCWD) { // If dirfd isn't special FDCWD
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
  const char *SubPath = pathname;

  // Current index for the temporary path to use.
  uint32_t CurrentIndex{};

  // The two temporary paths.
  const std::array<char*, 2> TmpPaths ={
    TmpFilename[0],
    TmpFilename[1],
  };

  if (FollowSymlink) {
    // Check if the combination of RootFS FD and subpath with the front '/' stripped off is a symlink.
    while(FEX::HLE::IsSymlink(RootFSFD, &SubPath[1])) {
      // Choose the current temporary working path.
      auto CurrentTmp = TmpPaths[CurrentIndex];

      // Get the symlink of RootFS FD + stripped subpath.
      auto SymlinkSize = FEX::HLE::GetSymlink(RootFSFD, &SubPath[1], CurrentTmp, PATH_MAX - 1);

      if (SymlinkSize > 0 && CurrentTmp[0] == '/') {
        // If the symlink is absolute:
        // 1) Zero terminate it.
        // 2) Set the path as our current subpath.
        // 3) Switch to the next temporary index. (We don't want to overwrite the current one on the next loop iteration).
        // 4) Run the loop again.
        CurrentTmp[SymlinkSize] = 0;
        SubPath = CurrentTmp;
        CurrentIndex ^= 1;
      }
      else {
        // If the path wasn't a symlink or wasn't absolute.
        // 1) Break early, returning the previous found result.
        // 2) If first iteration then we return `pathname`.
        break;
      }
    }
  }

  // Return the pair of rootfs FD plus relative subpath by stripping off the front '/'
  return std::make_pair(RootFSFD, &SubPath[1]);
}

std::optional<std::string> FileManager::GetSelf(const char *Pathname) {
  if (!Pathname) {
    return std::nullopt;
  }

  char PidSelfPath[50];
  snprintf(PidSelfPath, 50, "/proc/%i/exe", CurrentPID);

  if (strcmp(Pathname, "/proc/self/exe") == 0 ||
      strcmp(Pathname, "/proc/thread-self/exe") == 0 ||
      strcmp(Pathname, PidSelfPath) == 0) {
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

uint64_t FileManager::Open(const char *pathname, int flags, uint32_t mode) {
  auto NewPath = GetSelf(pathname);
  const char *SelfPath = NewPath ? NewPath->c_str() : nullptr;
  int fd = -1;

  if (!ShouldSkipOpenInEmu(flags)) {
    fd = EmuFD.OpenAt(AT_FDCWD, SelfPath, flags, mode);
    if (fd == -1) {
      FDPathTmpData TmpFilename;
      auto Path = GetEmulatedFDPath(AT_FDCWD, SelfPath, true, TmpFilename);
      if (Path.first != -1) {
        fd = ::openat(Path.first, Path.second, flags, mode);
      }
    }
  }

  if (fd == -1) {
    fd = ::open(SelfPath, flags, mode);
  }

  return fd;
}

uint64_t FileManager::Close(int fd) {
  return ::close(fd);
}

uint64_t FileManager::CloseRange(unsigned int first, unsigned int last, unsigned int flags) {
#ifndef CLOSE_RANGE_CLOEXEC
#define CLOSE_RANGE_CLOEXEC (1U << 2)
#endif
  return ::syscall(SYSCALL_DEF(close_range), first, last, flags);
}

uint64_t FileManager::Stat(const char *pathname, void *buf) {
  auto NewPath = GetSelf(pathname);
  const char *SelfPath = NewPath ? NewPath->c_str() : nullptr;

  // Stat follows symlinks
  FDPathTmpData TmpFilename;
  auto Path = GetEmulatedFDPath(AT_FDCWD, SelfPath, true, TmpFilename);
  if (Path.first != -1) {
    uint64_t Result = ::fstatat(Path.first, Path.second, reinterpret_cast<struct stat*>(buf), 0);
    if (Result != -1)
      return Result;
  }
  return ::stat(SelfPath, reinterpret_cast<struct stat*>(buf));
}

uint64_t FileManager::Lstat(const char *pathname, void *buf) {
  auto NewPath = GetSelf(pathname);
  const char *SelfPath = NewPath ? NewPath->c_str() : nullptr;

  // lstat does not follow symlinks
  FDPathTmpData TmpFilename;
  auto Path = GetEmulatedFDPath(AT_FDCWD, SelfPath, false, TmpFilename);
  if (Path.first != -1) {
    uint64_t Result = ::fstatat(Path.first, Path.second, reinterpret_cast<struct stat*>(buf), AT_SYMLINK_NOFOLLOW);
    if (Result != -1)
      return Result;
  }

  return ::lstat(pathname, reinterpret_cast<struct stat*>(buf));
}

uint64_t FileManager::Access(const char *pathname, [[maybe_unused]] int mode) {
  auto NewPath = GetSelf(pathname);
  const char *SelfPath = NewPath ? NewPath->c_str() : nullptr;

  // Access follows symlinks
  FDPathTmpData TmpFilename;
  auto Path = GetEmulatedFDPath(AT_FDCWD, SelfPath, true, TmpFilename);
  if (Path.first != -1) {
    uint64_t Result = ::faccessat(Path.first, Path.second, mode, 0);
    if (Result != -1)
      return Result;
  }
  return ::access(SelfPath, mode);
}

uint64_t FileManager::FAccessat(int dirfd, const char *pathname, int mode) {
  auto NewPath = GetSelf(pathname);
  const char *SelfPath = NewPath ? NewPath->c_str() : nullptr;

  FDPathTmpData TmpFilename;
  auto Path = GetEmulatedFDPath(dirfd, SelfPath, true, TmpFilename);
  if (Path.first != -1) {
    uint64_t Result = ::syscall(SYSCALL_DEF(faccessat2), Path.first, Path.second, mode, 0);
    if (Result != -1)
      return Result;
  }

  return ::syscall(SYS_faccessat, dirfd, SelfPath, mode);
}

uint64_t FileManager::FAccessat2(int dirfd, const char *pathname, int mode, int flags) {
  auto NewPath = GetSelf(pathname);
  const char *SelfPath = NewPath ? NewPath->c_str() : nullptr;

  FDPathTmpData TmpFilename;
  auto Path = GetEmulatedFDPath(dirfd, SelfPath, (flags & AT_SYMLINK_NOFOLLOW) == 0, TmpFilename);
  if (Path.first != -1) {
    uint64_t Result = ::syscall(SYSCALL_DEF(faccessat2), Path.first, Path.second, mode, flags);
    if (Result != -1)
      return Result;
  }

  return ::syscall(SYSCALL_DEF(faccessat2), dirfd, SelfPath, mode, flags);
}

uint64_t FileManager::Readlink(const char *pathname, char *buf, size_t bufsiz) {
  // calculate the non-self link to exe
  // Some executables do getpid, stat("/proc/$pid/exe")
  char PidSelfPath[50];
  snprintf(PidSelfPath, 50, "/proc/%i/exe", CurrentPID);

  if (strcmp(pathname, "/proc/self/exe") == 0 ||
      strcmp(pathname, "/proc/thread-self/exe") == 0 ||
      strcmp(pathname, PidSelfPath) == 0) {
    auto App = Filename();
    strncpy(buf, App.c_str(), bufsiz);
    return std::min(bufsiz, App.size());
  }

  FDPathTmpData TmpFilename;
  auto Path = GetEmulatedFDPath(AT_FDCWD, pathname, false, TmpFilename);
  if (Path.first != -1) {
    uint64_t Result = ::readlinkat(Path.first, Path.second, buf, bufsiz);
    if (Result != -1)
      return Result;

    if (Result == -1 &&
        errno == EINVAL) {
      // This means that the file wasn't a symlink
      // This is expected behaviour
      return -errno;
    }
  }

  return ::readlink(pathname, buf, bufsiz);
}

uint64_t FileManager::Chmod(const char *pathname, mode_t mode) {
  auto NewPath = GetSelf(pathname);
  const char *SelfPath = NewPath ? NewPath->c_str() : nullptr;

  FDPathTmpData TmpFilename;
  auto Path = GetEmulatedFDPath(AT_FDCWD, SelfPath, false, TmpFilename);
  if (Path.first != -1) {
    uint64_t Result = ::fchmodat(Path.first, Path.second, mode, 0);
    if (Result != -1)
      return Result;
  }
  return ::chmod(SelfPath, mode);
}

uint64_t FileManager::Readlinkat(int dirfd, const char *pathname, char *buf, size_t bufsiz) {
  // calculate the non-self link to exe
  // Some executables do getpid, stat("/proc/$pid/exe")
  // Can't use `GetSelf` directly here since readlink{at,} returns EINVAL if it isn't a symlink
  // Self is always a symlink and isn't expected to fail

  std::string Path{};
  if (((pathname && pathname[0] != '/') || // If pathname exists then it must not be absolute
        !pathname) &&
        dirfd != AT_FDCWD) {
    // Passed in a dirfd that isn't magic FDCWD
    // We need to get the path from the fd now
    Path = FEX::get_fdpath(dirfd).value_or("");

    if (pathname) {
      if (!Path.empty()) {
        // If the path returned empty then we don't need a separator
        Path += "/";
      }
      Path += pathname;
    }
  }
  else {
    if (!pathname || strlen(pathname) == 0) {
      return -1;
    }
    else if (pathname) {
      Path = pathname;
    }
  }

  char PidSelfPath[50];
  snprintf(PidSelfPath, 50, "/proc/%i/exe", CurrentPID);

  if (Path == "/proc/self/exe" ||
      Path == "/proc/thread-self/exe" ||
      Path == PidSelfPath) {
    auto App = Filename();
    strncpy(buf, App.c_str(), bufsiz);
    return std::min(bufsiz, App.size());
  }

  FDPathTmpData TmpFilename;
  auto NewPath = GetEmulatedFDPath(dirfd, pathname, false, TmpFilename);
  if (NewPath.first != -1) {
    uint64_t Result = ::readlinkat(NewPath.first, NewPath.second, buf, bufsiz);
    if (Result != -1)
      return Result;

    if (Result == -1 &&
        errno == EINVAL) {
      // This means that the file wasn't a symlink
      // This is expected behaviour
      return -errno;
    }
  }

  return ::readlinkat(dirfd, pathname, buf, bufsiz);
}

uint64_t FileManager::Openat([[maybe_unused]] int dirfs, const char *pathname, int flags, uint32_t mode) {
  auto NewPath = GetSelf(pathname);
  const char *SelfPath = NewPath ? NewPath->c_str() : nullptr;

  int32_t fd = -1;

  if (!ShouldSkipOpenInEmu(flags)) {
    fd = EmuFD.OpenAt(dirfs, SelfPath, flags, mode);
    if (fd == -1) {
      FDPathTmpData TmpFilename;
      auto Path = GetEmulatedFDPath(dirfs, SelfPath, true, TmpFilename);
      if (Path.first != -1) {
        fd = ::syscall(SYSCALL_DEF(openat), Path.first, Path.second, flags, mode);
      }
    }
  }

  if (fd == -1) {
    fd = ::syscall(SYSCALL_DEF(openat), dirfs, SelfPath, flags, mode);
  }

  return fd;
}

uint64_t FileManager::Openat2(int dirfs, const char *pathname, FEX::HLE::open_how *how, size_t usize) {
  auto NewPath = GetSelf(pathname);
  const char *SelfPath = NewPath ? NewPath->c_str() : nullptr;

  int32_t fd = -1;

  if (!ShouldSkipOpenInEmu(how->flags)) {
    fd = EmuFD.OpenAt(dirfs, SelfPath, how->flags, how->mode);
    if (fd == -1) {
      FDPathTmpData TmpFilename;
      auto Path = GetEmulatedFDPath(dirfs, SelfPath, true, TmpFilename);
      if (Path.first != -1) {
        fd = ::syscall(SYSCALL_DEF(openat2), Path.first, Path.second, how, usize);
      }
    }
  }

  if (fd == -1) {
    fd = ::syscall(SYSCALL_DEF(openat2), dirfs, SelfPath, how, usize);
  }

  return fd;

}

uint64_t FileManager::Statx(int dirfd, const char *pathname, int flags, uint32_t mask, struct statx *statxbuf) {
  auto NewPath = GetSelf(pathname);
  const char *SelfPath = NewPath ? NewPath->c_str() : nullptr;

  FDPathTmpData TmpFilename;
  auto Path = GetEmulatedFDPath(dirfd, SelfPath, (flags & AT_SYMLINK_NOFOLLOW) == 0, TmpFilename);
  if (Path.first != -1) {
    uint64_t Result = FHU::Syscalls::statx(Path.first, Path.second, flags, mask, statxbuf);
    if (Result != -1)
      return Result;
  }
  return FHU::Syscalls::statx(dirfd, SelfPath, flags, mask, statxbuf);
}

uint64_t FileManager::Mknod(const char *pathname, mode_t mode, dev_t dev) {
  auto NewPath = GetSelf(pathname);
  const char *SelfPath = NewPath ? NewPath->c_str() : nullptr;

  FDPathTmpData TmpFilename;
  auto Path = GetEmulatedFDPath(AT_FDCWD, SelfPath, false, TmpFilename);
  if (Path.first != -1) {
    uint64_t Result = ::mknodat(Path.first, Path.second, mode, dev);
    if (Result != -1)
      return Result;
  }
  return ::mknod(SelfPath, mode, dev);
}

uint64_t FileManager::Statfs(const char *path, void *buf) {
  auto Path = GetEmulatedPath(path);
  if (!Path.empty()) {
    uint64_t Result = ::statfs(Path.c_str(), reinterpret_cast<struct statfs*>(buf));
    if (Result != -1)
      return Result;
  }
  return ::statfs(path, reinterpret_cast<struct statfs*>(buf));
}

uint64_t FileManager::NewFSStatAt(int dirfd, const char *pathname, struct stat *buf, int flag) {
  auto NewPath = GetSelf(pathname);
  const char *SelfPath = NewPath ? NewPath->c_str() : nullptr;

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

uint64_t FileManager::NewFSStatAt64(int dirfd, const char *pathname, struct stat64 *buf, int flag) {
  auto NewPath = GetSelf(pathname);
  const char *SelfPath = NewPath ? NewPath->c_str() : nullptr;

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

}
