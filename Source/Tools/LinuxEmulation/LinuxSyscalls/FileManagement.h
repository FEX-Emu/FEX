// SPDX-License-Identifier: MIT
/*
$info$
tags: LinuxSyscalls|common
$end_info$
*/

#pragma once
#include <FEXCore/Config/Config.h>
#include <FEXCore/fextl/map.h>
#include <FEXCore/fextl/string.h>
#include <FEXCore/fextl/unordered_set.h>

#include <cstdint>
#include <fcntl.h>
#include <functional>
#include <mutex>
#include <linux/limits.h>
#include <optional>
#include <stddef.h>
#include <sys/stat.h>
#include <unistd.h>

#include "LinuxSyscalls/EmulatedFiles/EmulatedFiles.h"

namespace FEXCore::Context {
class Context;
}

namespace FEX::HLE {
[[maybe_unused]]
static bool IsSymlink(int FD, const char* Filename) {
  // Checks to see if a filepath is a symlink.
  struct stat Buffer {};
  int Result = fstatat(FD, Filename, &Buffer, AT_SYMLINK_NOFOLLOW);
  return Result == 0 && S_ISLNK(Buffer.st_mode);
}

[[maybe_unused]]
static ssize_t GetSymlink(int FD, const char* Filename, char* ResultBuffer, size_t ResultBufferSize) {
  return readlinkat(FD, Filename, ResultBuffer, ResultBufferSize);
}

struct open_how;

class FileManager final {
public:
  FileManager() = delete;
  FileManager(FileManager&&) = delete;

  FileManager(FEXCore::Context::Context* ctx);
  ~FileManager();
  uint64_t Open(const char* pathname, int flags, uint32_t mode);
  uint64_t Close(int fd);
  uint64_t CloseRange(unsigned int first, unsigned int last, unsigned int flags);
  uint64_t Stat(const char* pathname, void* buf);
  uint64_t Lstat(const char* path, void* buf);
  uint64_t Access(const char* pathname, int mode);
  uint64_t FAccessat(int dirfd, const char* pathname, int mode);
  uint64_t FAccessat2(int dirfd, const char* pathname, int mode, int flags);
  uint64_t Readlink(const char* pathname, char* buf, size_t bufsiz);
  uint64_t Chmod(const char* pathname, mode_t mode);
  uint64_t Readlinkat(int dirfd, const char* pathname, char* buf, size_t bufsiz);
  uint64_t Openat(int dirfs, const char* pathname, int flags, uint32_t mode);
  uint64_t Openat2(int dirfs, const char* pathname, FEX::HLE::open_how* how, size_t usize);
  uint64_t Statx(int dirfd, const char* pathname, int flags, uint32_t mask, struct statx* statxbuf);
  uint64_t Mknod(const char* pathname, mode_t mode, dev_t dev);
  uint64_t NewFSStatAt(int dirfd, const char* pathname, struct stat* buf, int flag);
  uint64_t NewFSStatAt64(int dirfd, const char* pathname, struct stat64* buf, int flag);
  uint64_t Setxattr(const char* path, const char* name, const void* value, size_t size, int flags);
  uint64_t LSetxattr(const char* path, const char* name, const void* value, size_t size, int flags);
  uint64_t Getxattr(const char* path, const char* name, void* value, size_t size);
  uint64_t LGetxattr(const char* path, const char* name, void* value, size_t size);
  uint64_t Listxattr(const char* path, char* list, size_t size);
  uint64_t LListxattr(const char* path, char* list, size_t size);
  uint64_t Removexattr(const char* path, const char* name);
  uint64_t LRemovexattr(const char* path, const char* name);
  // vfs
  uint64_t Statfs(const char* path, void* buf);

  std::optional<std::string_view> GetSelf(const char* Pathname);
  bool IsSelfNoFollow(const char* Pathname, int flags) const;

  void UpdatePID(uint32_t PID) {
    CurrentPID = PID;
  }

  fextl::string GetEmulatedPath(const char* pathname, bool FollowSymlink = false);
  using FDPathTmpData = std::array<char[PATH_MAX], 2>;
  std::pair<int, const char*> GetEmulatedFDPath(int dirfd, const char* pathname, bool FollowSymlink, FDPathTmpData& TmpFilename);

  bool SupportsProcFSInterpreterPath() const {
    return SupportsProcFSInterpreter;
  }

#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED
  void TrackFEXFD(int FD) noexcept {
    std::lock_guard lk(FEXTrackingFDMutex);
    FEXTrackingFDs.emplace(FD);
  }

  void RemoveFEXFD(int FD) noexcept {
    std::lock_guard lk(FEXTrackingFDMutex);
    FEXTrackingFDs.erase(FD);
  }

  void RemoveFEXFDRange(int begin, int end) noexcept {
    std::lock_guard lk(FEXTrackingFDMutex);

    std::erase_if(FEXTrackingFDs, [begin, end](int FD) { return FD >= begin && (FD <= end || end == -1); });
  }

  bool CheckIfFDInTrackedSet(int FD) noexcept {
    std::lock_guard lk(FEXTrackingFDMutex);
    return FEXTrackingFDs.contains(FD);
  }

  bool CheckIfFDRangeInTrackedSet(int begin, int end) noexcept {
    std::lock_guard lk(FEXTrackingFDMutex);
    // Just linear scan since the number of tracking FDs is low.
    for (auto it : FEXTrackingFDs) {
      if (it >= begin && (it <= end || end == -1)) {
        return true;
      }
    }
    return false;
  }

#else
  void TrackFEXFD(int FD) const noexcept {}
  bool CheckIfFDInTrackedSet(int FD) const noexcept {
    return false;
  }
  void RemoveFEXFD(int FD) const noexcept {}
  void RemoveFEXFDRange(int begin, int end) const noexcept {}
  bool CheckIfFDRangeInTrackedSet(int begin, int end) const noexcept {
    return false;
  }
#endif

private:
#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED
  std::mutex FEXTrackingFDMutex;
  fextl::set<int> FEXTrackingFDs;
#endif

  bool RootFSPathExists(const char* Filepath);

  struct ThunkDBObject {
    fextl::string LibraryName;
    fextl::unordered_set<fextl::string> Depends;
    fextl::vector<fextl::string> Overlays;
    bool Enabled {};
  };
  void LoadThunkDatabase(fextl::unordered_map<fextl::string, ThunkDBObject>& ThunkDB, bool Global);
  FEX::EmulatedFile::EmulatedFDManager EmuFD;

  fextl::map<fextl::string, fextl::string, std::less<>> ThunkOverlays;

  FEX_CONFIG_OPT(Filename, APP_FILENAME);
  FEX_CONFIG_OPT(LDPath, ROOTFS);
  FEX_CONFIG_OPT(ThunkGuestLibs, THUNKGUESTLIBS);
  FEX_CONFIG_OPT(ThunkGuestLibs32, THUNKGUESTLIBS32);
  FEX_CONFIG_OPT(ThunkConfig, THUNKCONFIG);
  FEX_CONFIG_OPT(AppConfigName, APP_CONFIG_NAME);
  FEX_CONFIG_OPT(Is64BitMode, IS64BIT_MODE);
  uint32_t CurrentPID {};
  int RootFSFD {AT_FDCWD};
  bool SupportsProcFSInterpreter {};
};
} // namespace FEX::HLE
