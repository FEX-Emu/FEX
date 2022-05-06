/*
$info$
tags: LinuxSyscalls|common
desc: Rootfs overlay logic
$end_info$
*/

#include "Tests/LinuxSyscalls/FileManagement.h"
#include "Tests/LinuxSyscalls/EmulatedFiles/EmulatedFiles.h"
#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/x64/Syscalls.h"

#include <FEXCore/Utils/LogManager.h>
#include <FEXHeaderUtils/ScopedSignalMask.h>
#include <FEXHeaderUtils/Syscalls.h>

#include <algorithm>
#include <errno.h>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <linux/limits.h>
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

namespace FEXCore::Context {
  struct Context;
}

namespace FEX::HLE {
  struct open_how;

static bool LoadFile(std::vector<char> &Data, const std::string &Filename) {
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

void FileManager::LoadThunkDatabase(bool Global) {
  auto ThunkDBPath = FEXCore::Config::GetConfigDirectory(Global) + "ThunksDB.json";
  std::vector<char> FileData;
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
    for( json_t const* Library = json_getChild( DB ); Library != nullptr; Library = json_getSibling( Library )) {
      // Get the user defined name for the library
      const char* LibraryName = json_getName(Library);
      auto DBObject = ThunkDB.insert_or_assign(LibraryName, ThunkDBObject{}).first;

      // Walk the libraries items to get the data
      for (json_t const* LibraryItem = json_getChild(Library); LibraryItem != nullptr; LibraryItem = json_getSibling(LibraryItem)) {
        const char* ItemName = json_getName(LibraryItem);

        if (strcmp(ItemName, "Library") == 0) {
          // "Library": "libGL-guest.so"
          DBObject->second.LibraryName = json_getValue(LibraryItem);
        }
        else if (strcmp(ItemName, "Depends") == 0) {
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
        else if (strcmp(ItemName, "Overlay") == 0) {
          jsonType_t PropertyType = json_getType(LibraryItem);
          if (PropertyType == JSON_TEXT) {
            DBObject->second.Overlays.emplace_back(json_getValue(LibraryItem));
          }
          else if (PropertyType == JSON_ARRAY) {
            for (json_t const* Overlay = json_getChild(LibraryItem); Overlay != nullptr; Overlay = json_getSibling(Overlay)) {
              DBObject->second.Overlays.emplace_back(json_getValue(Overlay));
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

  if (ThunkConfigFile.size()) {

    auto ThunkGuestPath = std::filesystem::path(ThunkGuestLibs());

    std::vector<char> FileData;
    if (LoadFile(FileData, ThunkConfigFile)) {
      FileData.push_back(0);

      JSON::JsonAllocator Pool {
        .PoolObject = {
          .init = JSON::PoolInit,
          .alloc = JSON::PoolAlloc,
        },
      };

      json_t const *json = json_createWithPool(&FileData.at(0), &Pool.PoolObject);

      json_t const* thunks = json_getProperty( json, "thunks" );
      if (thunks && json_getType(thunks) == JSON_OBJ) {
        json_t const* thunk;
        for( thunk = json_getChild( thunks ); thunk != 0; thunk = json_getSibling( thunk )) {
          char const* GuestThunk = json_getName( thunk );
          jsonType_t propertyType = json_getType( thunk );

          if (propertyType == JSON_TEXT) {
            char const* RootFSLib = json_getValue( thunk );
            auto ThunkPath = ThunkGuestPath / GuestThunk;
            if (std::filesystem::exists(ThunkPath)) {
              ThunkOverlays.emplace(RootFSLib, ThunkPath);
            }
          } else if (propertyType == JSON_ARRAY) {
            json_t const* child;
            for( child = json_getChild( thunk ); child != 0; child = json_getSibling( child ) ) {
              if (json_getType( child ) == JSON_TEXT) {
                char const* RootFSLib = json_getValue( child );
                auto ThunkPath = ThunkGuestPath / GuestThunk;
                if (std::filesystem::exists(ThunkPath)) {
                  ThunkOverlays.emplace(RootFSLib, ThunkPath);
                }
              }
            }
          }
        }
      }

      json_t const* ThunksDB = json_getProperty( json, "ThunksDB" );
      if (ThunksDB) {
        // If a thunks DB property exists then we pull in data from the thunks database
        // Load the initial thunks database
        LoadThunkDatabase(true);
        LoadThunkDatabase(false);

        // Now load this property
        for (json_t const* Item = json_getChild(ThunksDB); Item != nullptr; Item = json_getSibling(Item)) {
          const char *LibraryName = json_getName(Item);
          int64_t LibraryEnabled = json_getInteger(Item);
          if (LibraryEnabled != 0) {
            // If the library is enabled then find it in the DB
            // Enable the overlay and all the dependencies in one go
            auto DBObject = ThunkDB.find(LibraryName);
            if (DBObject != ThunkDB.end() &&
                DBObject->second.Enabled == false) {

              auto ThunkPath = ThunkGuestPath / DBObject->second.LibraryName;
              if (std::filesystem::exists(ThunkPath)) {
                for (auto Overlay : DBObject->second.Overlays) {
                  // Direct full path in guest RootFS to our overlay file
                  ThunkOverlays.emplace(Overlay, ThunkPath);
                }
              }
              DBObject->second.Enabled = true;
              // Now walk the dependencies and set them up as well
              // Make sure to enable each one as we go to remove circular dependencies
              std::function<void(std::unordered_set<std::string> &Depends)> InsertDependencies
                = [this, &ThunkGuestPath, &InsertDependencies](std::unordered_set<std::string> &Depends) -> void {
                for (auto &Depend : Depends) {
                  auto DBDepend = ThunkDB.find(Depend);
                  if (DBDepend != ThunkDB.end() &&
                      DBDepend->second.Enabled == false) {

                    auto ThunkPath = ThunkGuestPath / DBDepend->second.LibraryName;
                    if (std::filesystem::exists(ThunkPath)) {
                      for (auto Overlay : DBDepend->second.Overlays) {
                        // Direct full path in guest RootFS to our overlay file
                        ThunkOverlays.emplace(Overlay, ThunkPath);
                      }
                    }

                    // Enabled, now walk this dependencies
                    DBDepend->second.Enabled = true;
                    InsertDependencies(DBDepend->second.Depends);
                  }
                }
              };
              InsertDependencies(DBObject->second.Depends);
            }
          }
        }

        // Now clear the thunk database since we're loaded
        ThunkDB.clear();
      }
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
  }

  UpdatePID(::getpid());
}

FileManager::~FileManager() {
}

std::string FileManager::GetEmulatedPath(const char *pathname, bool FollowSymlink) {
  auto RootFSPath = LDPath();
  if (!pathname || // If no pathname
      pathname[0] != '/' || // If relative
      strcmp(pathname, "/") == 0) { // If we are getting root
    return {};
  }

  auto thunkOverlay = ThunkOverlays.find(pathname);
  if (thunkOverlay != ThunkOverlays.end()) {
    return thunkOverlay->second;
  }

  if (RootFSPath.empty()) { // If RootFS doesn't exist
    return {};
  }

  std::string Path = RootFSPath + pathname;
  if (FollowSymlink) {
    char ReadlinkPath[PATH_MAX + 1];
    ReadlinkPath[PATH_MAX] = 0;
    ssize_t Result{};
    do {
      Result = readlink(Path.c_str(), ReadlinkPath, PATH_MAX);
      if (Result > 0) {
        // Null terminate
        ReadlinkPath[Result] = 0;

        if (ReadlinkPath[0] == '/') {
          Path = RootFSPath + std::string(ReadlinkPath);
        }
        else {
          // Not absolute, leave now
          break;
        }
      }
    } while (Result > 0);
  }
  return Path;
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

uint64_t FileManager::Open(const char *pathname, [[maybe_unused]] int flags, [[maybe_unused]] uint32_t mode) {
  auto NewPath = GetSelf(pathname);
  const char *SelfPath = NewPath ? NewPath->c_str() : nullptr;
  return ::open(SelfPath, flags, mode);
}

uint64_t FileManager::Close(int fd) {
  {
    FHU::ScopedSignalMaskWithMutex lk(FDLock);
    FDToNameMap.erase(fd);
  }
  return ::close(fd);
}

uint64_t FileManager::CloseRange(unsigned int first, unsigned int last, unsigned int flags) {
#ifndef CLOSE_RANGE_CLOEXEC
#define CLOSE_RANGE_CLOEXEC (1U << 2)
#endif

  if (!(flags & CLOSE_RANGE_CLOEXEC)) {
    // If the flag was set then it doesn't actually close the FDs
    // Just sets the flag on a range
    FHU::ScopedSignalMaskWithMutex lk(FDLock);
    for (unsigned int i = first; i <= last; ++i) {
      // We remove from first to last inclusive
      FDToNameMap.erase(i);
    }
  }
  return ::syscall(SYSCALL_DEF(close_range), first, last, flags);
}

uint64_t FileManager::Stat(const char *pathname, void *buf) {
  auto NewPath = GetSelf(pathname);
  const char *SelfPath = NewPath ? NewPath->c_str() : nullptr;

  // Stat follows symlinks
  auto Path = GetEmulatedPath(SelfPath, true);
  if (!Path.empty()) {
    uint64_t Result = ::stat(Path.c_str(), reinterpret_cast<struct stat*>(buf));
    if (Result != -1)
      return Result;
  }
  return ::stat(SelfPath, reinterpret_cast<struct stat*>(buf));
}

uint64_t FileManager::Lstat(const char *pathname, void *buf) {
  auto NewPath = GetSelf(pathname);
  const char *SelfPath = NewPath ? NewPath->c_str() : nullptr;

  // lstat does not follow symlinks
  auto Path = GetEmulatedPath(SelfPath, false);
  if (!Path.empty()) {
    uint64_t Result = ::lstat(Path.c_str(), reinterpret_cast<struct stat*>(buf));
    if (Result != -1)
      return Result;
  }

  return ::lstat(pathname, reinterpret_cast<struct stat*>(buf));
}

uint64_t FileManager::Access(const char *pathname, [[maybe_unused]] int mode) {
  auto NewPath = GetSelf(pathname);
  const char *SelfPath = NewPath ? NewPath->c_str() : nullptr;

  // Access follows symlinks
  auto Path = GetEmulatedPath(SelfPath, true);
  if (!Path.empty()) {
    uint64_t Result = ::access(Path.c_str(), mode);
    if (Result != -1)
      return Result;
  }

  return ::access(SelfPath, mode);
}

uint64_t FileManager::FAccessat(int dirfd, const char *pathname, int mode) {
  auto NewPath = GetSelf(pathname);
  const char *SelfPath = NewPath ? NewPath->c_str() : nullptr;

  auto Path = GetEmulatedPath(SelfPath);
  if (!Path.empty()) {
    uint64_t Result = ::syscall(SYS_faccessat, dirfd, Path.c_str(), mode);
    if (Result != -1)
      return Result;
  }

  return ::syscall(SYS_faccessat, dirfd, SelfPath, mode);
}

uint64_t FileManager::FAccessat2(int dirfd, const char *pathname, int mode, int flags) {
  auto NewPath = GetSelf(pathname);
  const char *SelfPath = NewPath ? NewPath->c_str() : nullptr;

  auto Path = GetEmulatedPath(SelfPath, (flags & AT_SYMLINK_NOFOLLOW) == 0);
  if (!Path.empty()) {
    uint64_t Result = ::syscall(SYSCALL_DEF(faccessat2), dirfd, Path.c_str(), mode, flags);
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

  auto Path = GetEmulatedPath(pathname);
  if (!Path.empty()) {
    uint64_t Result = ::readlink(Path.c_str(), buf, bufsiz);
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

  auto Path = GetEmulatedPath(SelfPath);
  if (!Path.empty()) {
    uint64_t Result = ::chmod(Path.c_str(), mode);
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
    auto get_fdpath = [](int fd) -> std::string {
      std::error_code ec;
      return std::filesystem::canonical(std::filesystem::path("/proc/self/fd") / std::to_string(fd), ec).string();
    };
    // Passed in a dirfd that isn't magic FDCWD
    // We need to get the path from the fd now
    Path = get_fdpath(dirfd);

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

  Path = GetEmulatedPath(pathname);
  if (!Path.empty()) {
    uint64_t Result = ::readlinkat(dirfd, Path.c_str(), buf, bufsiz);
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

  fd = EmuFD.OpenAt(dirfs, SelfPath, flags, mode);
  if (fd == -1) {
    auto Path = GetEmulatedPath(SelfPath, true);
    if (!Path.empty()) {
      fd = ::openat(dirfs, Path.c_str(), flags, mode);
    }

    if (fd == -1)
      fd = ::openat(dirfs, SelfPath, flags, mode);
  }

  if (fd != -1) {
    FHU::ScopedSignalMaskWithMutex lk(FDLock);
    FDToNameMap.insert_or_assign(fd, SelfPath);
  }

  return fd;
}

uint64_t FileManager::Openat2(int dirfs, const char *pathname, FEX::HLE::open_how *how, size_t usize) {
  auto NewPath = GetSelf(pathname);
  const char *SelfPath = NewPath ? NewPath->c_str() : nullptr;

  int32_t fd = -1;

  fd = EmuFD.OpenAt(dirfs, SelfPath, how->flags, how->mode);
  if (fd == -1) {
    auto Path = GetEmulatedPath(SelfPath, true);
    if (!Path.empty()) {
      fd = ::syscall(SYSCALL_DEF(openat2), dirfs, Path.c_str(), how, usize);
    }

    if (fd == -1)
      fd = ::syscall(SYSCALL_DEF(openat2), dirfs, SelfPath, how, usize);
  }

  if (fd != -1) {
    FHU::ScopedSignalMaskWithMutex lk(FDLock);
    FDToNameMap.insert_or_assign(fd, SelfPath);
  }

  return fd;

}

uint64_t FileManager::Statx(int dirfd, const char *pathname, int flags, uint32_t mask, struct statx *statxbuf) {
  auto NewPath = GetSelf(pathname);
  const char *SelfPath = NewPath ? NewPath->c_str() : nullptr;

  auto Path = GetEmulatedPath(SelfPath, (flags & AT_SYMLINK_NOFOLLOW) == 0);
  if (!Path.empty()) {
    uint64_t Result = FHU::Syscalls::statx(dirfd, Path.c_str(), flags, mask, statxbuf);
    if (Result != -1)
      return Result;
  }
  return FHU::Syscalls::statx(dirfd, SelfPath, flags, mask, statxbuf);
}

uint64_t FileManager::Mknod(const char *pathname, mode_t mode, dev_t dev) {
  auto NewPath = GetSelf(pathname);
  const char *SelfPath = NewPath ? NewPath->c_str() : nullptr;

  auto Path = GetEmulatedPath(SelfPath);
  if (!Path.empty()) {
    uint64_t Result = ::mknod(Path.c_str(), mode, dev);
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

  auto Path = GetEmulatedPath(SelfPath, (flag & AT_SYMLINK_NOFOLLOW) == 0);
  if (!Path.empty()) {
    uint64_t Result = ::fstatat(dirfd, Path.c_str(), buf, flag);
    if (Result != -1) {
      return Result;
    }
  }
  return ::fstatat(dirfd, SelfPath, buf, flag);
}

uint64_t FileManager::NewFSStatAt64(int dirfd, const char *pathname, struct stat64 *buf, int flag) {
  auto NewPath = GetSelf(pathname);
  const char *SelfPath = NewPath ? NewPath->c_str() : nullptr;

  auto Path = GetEmulatedPath(SelfPath, (flag & AT_SYMLINK_NOFOLLOW) == 0);
  if (!Path.empty()) {
    uint64_t Result = ::fstatat64(dirfd, Path.c_str(), buf, flag);
    if (Result != -1) {
      return Result;
    }
  }
  return ::fstatat64(dirfd, SelfPath, buf, flag);
}

std::string *FileManager::FindFDName(int fd) {
  FHU::ScopedSignalMaskWithMutex lk(FDLock);
  auto it = FDToNameMap.find(fd);
  if (it == FDToNameMap.end()) {
    return nullptr;
  }
  return &it->second;
}

}
