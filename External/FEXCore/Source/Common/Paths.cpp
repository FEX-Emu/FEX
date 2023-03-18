#include "Common/Paths.h"
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/fextl/string.h>

#include <cstdlib>
#include <filesystem>
#include <memory>
#include <pwd.h>
#include <system_error>
#include <unistd.h>

namespace FEXCore::Paths {
  std::unique_ptr<fextl::string> CachePath;
  std::unique_ptr<fextl::string> EntryCache;

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
    CachePath = std::make_unique<fextl::string>();
    EntryCache = std::make_unique<fextl::string>();

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

    std::error_code ec{};
    FEXCore::Allocator::YesIKnowImNotSupposedToUseTheGlibcAllocator glibc;
    // Ensure the folder structure is created for our Data
    if (!std::filesystem::exists(*EntryCache, ec) &&
        !std::filesystem::create_directories(*EntryCache, ec)) {
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
