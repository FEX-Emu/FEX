#include "Common/Paths.h"
#include <FEXCore/Utils/Allocator.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/fextl/memory.h>
#include <FEXCore/fextl/string.h>
#include <FEXHeaderUtils/Filesystem.h>

#include <cstdlib>
#include <filesystem>
#include <memory>
#include <pwd.h>
#include <system_error>
#include <sys/stat.h>
#include <unistd.h>

namespace FEXCore::Paths {
  fextl::unique_ptr<fextl::string> CachePath;
  fextl::unique_ptr<fextl::string> EntryCache;

  char const* FindUserHomeThroughUID() {
    auto passwd = getpwuid(geteuid());
    if (passwd) {
      return passwd->pw_dir;
    }
    return nullptr;
  }

  const char *GetHomeDirectory() {
    char const *HomeDir = getenv("HOME");

    // Try to get home directory from uid
    if (!HomeDir) {
      HomeDir = FindUserHomeThroughUID();
    }

    // try the PWD
    if (!HomeDir) {
      HomeDir = getenv("PWD");
    }

    // Still doesn't exit? You get local
    if (!HomeDir) {
      HomeDir = ".";
    }

    return HomeDir;
  }

  void InitializePaths() {
    CachePath = fextl::make_unique<fextl::string>();
    EntryCache = fextl::make_unique<fextl::string>();

    char const *HomeDir = getenv("HOME");

    if (!HomeDir) {
      HomeDir = getenv("PWD");
    }

    if (!HomeDir) {
      HomeDir = ".";
    }

    char *XDGDataDir = getenv("XDG_DATA_DIR");
    if (XDGDataDir) {
      *CachePath = XDGDataDir;
    }
    else {
      if (HomeDir) {
        *CachePath = HomeDir;
      }
    }

    *CachePath += "/.fex-emu/";
    *EntryCache = *CachePath + "/EntryCache/";

    // Ensure the folder structure is created for our Data
    if (!FHU::Filesystem::Exists(EntryCache->c_str()) &&
        !FHU::Filesystem::CreateDirectories(*EntryCache)) {
      LogMan::Msg::DFmt("Couldn't create EntryCache directory: '{}'", *EntryCache);
    }
  }

  void ShutdownPaths() {
    CachePath.reset();
    EntryCache.reset();
  }

  fextl::string GetCachePath() {
    return *CachePath;
  }

  fextl::string GetEntryCachePath() {
    return *EntryCache;
  }
}
