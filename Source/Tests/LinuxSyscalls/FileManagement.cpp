#include "Tests/LinuxSyscalls/FileManagement.h"

#include <FEXCore/Utils/LogManager.h>
#include <cstring>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/ioctl.h>
#include <sys/timerfd.h>
#include <sys/uio.h>
#include <sys/vfs.h>
#include <unistd.h>
#include <sys/syscall.h>

#include <tiny-json.h>

#include <fstream>
#include <filesystem>

namespace FEX::HLE {

static bool LoadFile(std::vector<char> &Data, const std::string &Filename) {
  std::fstream File;
  File.open(Filename, std::ios::in);

  if (!File.is_open()) {
    return false;
  }

  if (!File.seekg(0, std::fstream::end)) {
    LogMan::Msg::D("Couldn't load configuration file: Seek end");
    return false;
  }

  auto FileSize = File.tellg();
  if (File.fail()) {
    LogMan::Msg::D("Couldn't load configuration file: tellg");
    return false;
  }

  if (!File.seekg(0, std::fstream::beg)) {
    LogMan::Msg::D("Couldn't load configuration file: Seek beginning");
    return false;
  }

  if (FileSize > 0) {
    Data.resize(FileSize);
    if (!File.read(&Data.at(0), FileSize)) {
      // Probably means permissions aren't set. Just early exit
      return false;
    }
    File.close();
  }
  else {
    return false;
  }

  return true;
}

FileManager::FileManager(FEXCore::Context::Context *ctx)
  : EmuFD {ctx} {
  // calculate the non-self link to exe
  // Some executables do getpid, stat("/proc/$pid/exe")
  int pid = getpid();

  char buf[50];
  snprintf(buf, 50, "/proc/%i/exe", pid);

  PidSelfPath = std::string(buf);

  
  auto ThunkConfigFile = ThunkConfig();

  if (ThunkConfigFile.size()) {

    auto ThunkGuestPath = std::filesystem::path(ThunkGuestLibs());

    std::vector<char> FileData;
    if (LoadFile(FileData, ThunkConfigFile)) {
      FileData.push_back(0);

      json_t mem[128];
      json_t const* json = json_create( &FileData.at(0), mem, sizeof mem / sizeof *mem );

      json_t const* thunks = json_getProperty( json, "thunks" );
      if ( !thunks || JSON_OBJ != json_getType( thunks ) ) {
        return;
      }

      json_t const* thunk;
      for( thunk = json_getChild( thunks ); thunk != 0; thunk = json_getSibling( thunk )) {
        char const* GuestThunk = json_getName( thunk );
        jsonType_t propertyType = json_getType( thunk );

        if (propertyType == JSON_TEXT) {
          char const* RootFSLib = json_getValue( thunk );
          ThunkOverlays.emplace(RootFSLib, ThunkGuestPath / GuestThunk);
        } else if (propertyType == JSON_ARRAY) {
          json_t const* child;
          for( child = json_getChild( thunk ); child != 0; child = json_getSibling( child ) ) {
            if (json_getType( child ) == JSON_TEXT) {
              char const* RootFSLib = json_getValue( child );
              ThunkOverlays.emplace(RootFSLib, ThunkGuestPath / GuestThunk);
            }
          }
        }
      }
    }

    if (ThunkOverlays.size()) {
      LogMan::Msg::I("Thunk Overlays:");
      for (auto &Thunk: ThunkOverlays) {
        LogMan::Msg::I("\t%s -> %s", Thunk.first.c_str(), Thunk.second.c_str());
      }
    }
  }
}

FileManager::~FileManager() {
}

std::string FileManager::GetEmulatedPath(const char *pathname) {
  auto RootFSPath = LDPath();
  if (!pathname ||
      pathname[0] != '/' ||
      RootFSPath.empty()) {
    return {};
  }

  auto thunkOverlay = ThunkOverlays.find(pathname);
  if (thunkOverlay != ThunkOverlays.end()) {
    return thunkOverlay->second;
  }

  return RootFSPath + pathname;
}

uint64_t FileManager::Open(const char *pathname, [[maybe_unused]] int flags, [[maybe_unused]] uint32_t mode) {
  return ::open(pathname, flags, mode);
}

uint64_t FileManager::Close(int fd) {
  {
    std::lock_guard<std::mutex> lk(FDLock);
    FDToNameMap.erase(fd);
  }
  return ::close(fd);
}

uint64_t FileManager::Stat(const char *pathname, void *buf) {
  auto Path = GetEmulatedPath(pathname);
  if (!Path.empty()) {
    uint64_t Result = ::stat(Path.c_str(), reinterpret_cast<struct stat*>(buf));
    if (Result != -1)
      return Result;
  }
  return ::stat(pathname, reinterpret_cast<struct stat*>(buf));
}

uint64_t FileManager::Lstat(const char *path, void *buf) {
  auto Path = GetEmulatedPath(path);
  if (!Path.empty()) {
    uint64_t Result = ::lstat(Path.c_str(), reinterpret_cast<struct stat*>(buf));
    if (Result != -1)
      return Result;
  }

  return ::lstat(path, reinterpret_cast<struct stat*>(buf));
}

uint64_t FileManager::Access(const char *pathname, [[maybe_unused]] int mode) {
  auto Path = GetEmulatedPath(pathname);
  if (!Path.empty()) {
    uint64_t Result = ::access(Path.c_str(), mode);
    if (Result != -1)
      return Result;
  }

  return ::access(pathname, mode);
}

uint64_t FileManager::FAccessat(int dirfd, const char *pathname, int mode) {
  auto Path = GetEmulatedPath(pathname);
  if (!Path.empty()) {
    uint64_t Result = ::syscall(SYS_faccessat, dirfd, Path.c_str(), mode);
    if (Result != -1)
      return Result;
  }

  return ::syscall(SYS_faccessat, dirfd, pathname, mode);
}

uint64_t FileManager::FAccessat2(int dirfd, const char *pathname, int mode, int flags) {
#ifndef SYS_faccessat2
  const uint32_t SYS_faccessat2 = 439;
#endif
  auto Path = GetEmulatedPath(pathname);
  if (!Path.empty()) {
    uint64_t Result = ::syscall(SYS_faccessat2, dirfd, Path.c_str(), mode, flags);
    if (Result != -1)
      return Result;
  }

  return ::syscall(SYS_faccessat2, dirfd, pathname, mode, flags);
}

uint64_t FileManager::Readlink(const char *pathname, char *buf, size_t bufsiz) {
  if (strcmp(pathname, "/proc/self/exe") == 0 || strcmp(pathname, PidSelfPath.c_str()) == 0) {
    auto App = Filename();
    strncpy(buf, App.c_str(), bufsiz);
    return std::min(bufsiz, App.size());
  }

  auto Path = GetEmulatedPath(pathname);
  if (!Path.empty()) {
    uint64_t Result = ::readlink(Path.c_str(), buf, bufsiz);
    if (Result != -1)
      return Result;
  }

  return ::readlink(pathname, buf, bufsiz);
}

uint64_t FileManager::Chmod(const char *pathname, mode_t mode) {
  auto Path = GetEmulatedPath(pathname);
  if (!Path.empty()) {
    uint64_t Result = ::chmod(Path.c_str(), mode);
    if (Result != -1)
      return Result;
  }

  return ::chmod(pathname, mode);
}

uint64_t FileManager::Readlinkat(int dirfd, const char *pathname, char *buf, size_t bufsiz) {
  if (strcmp(pathname, "/proc/self/exe") == 0) {
    auto App = Filename();
    strncpy(buf, App.c_str(), bufsiz);
    return std::min(bufsiz, App.size());
  }

  auto Path = GetEmulatedPath(pathname);
  if (!Path.empty()) {
    uint64_t Result = ::readlinkat(dirfd, Path.c_str(), buf, bufsiz);
    if (Result != -1)
      return Result;
  }

  return ::readlinkat(dirfd, pathname, buf, bufsiz);
}

uint64_t FileManager::Openat([[maybe_unused]] int dirfs, const char *pathname, int flags, uint32_t mode) {
  int32_t fd = -1;

  fd = EmuFD.OpenAt(dirfs, pathname, flags, mode);
  if (fd == -1) {
    auto Path = GetEmulatedPath(pathname);
    if (!Path.empty()) {
      fd = ::openat(dirfs, Path.c_str(), flags, mode);
    }

    if (fd == -1)
      fd = ::openat(dirfs, pathname, flags, mode);
  }

  if (fd != -1) {
    std::lock_guard<std::mutex> lk(FDLock);
    FDToNameMap[fd] = pathname;
  }

  return fd;
}

uint64_t FileManager::Statx(int dirfd, const char *pathname, int flags, uint32_t mask, struct statx *statxbuf) {
  auto Path = GetEmulatedPath(pathname);
  if (!Path.empty()) {
    uint64_t Result = ::statx(dirfd, Path.c_str(), flags, mask, statxbuf);
    if (Result != -1)
      return Result;
  }
  return ::statx(dirfd, pathname, flags, mask, statxbuf);
}

uint64_t FileManager::Mknod(const char *pathname, mode_t mode, dev_t dev) {
  auto Path = GetEmulatedPath(pathname);
  if (!Path.empty()) {
    uint64_t Result = ::mknod(Path.c_str(), mode, dev);
    if (Result != -1)
      return Result;
  }
  return ::mknod(pathname, mode, dev);
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
  auto Path = GetEmulatedPath(pathname);
  if (!Path.empty()) {
    uint64_t Result = ::fstatat(dirfd, Path.c_str(), buf, flag);
    if (Result != -1) {
      return Result;
    }
  }
  return ::fstatat(dirfd, pathname, buf, flag);
}

uint64_t FileManager::NewFSStatAt64(int dirfd, const char *pathname, struct stat64 *buf, int flag) {
  auto Path = GetEmulatedPath(pathname);
  if (!Path.empty()) {
    uint64_t Result = ::fstatat64(dirfd, Path.c_str(), buf, flag);
    if (Result != -1) {
      return Result;
    }
  }
  return ::fstatat64(dirfd, pathname, buf, flag);
}

std::string *FileManager::FindFDName(int fd) {
  std::lock_guard<std::mutex> lk(FDLock);
  auto it = FDToNameMap.find(fd);
  if (it == FDToNameMap.end()) {
    return nullptr;
  }
  return &it->second;
}

}
