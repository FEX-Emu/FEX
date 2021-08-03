#include "Common/Paths.h"
#include <FEXCore/Utils/LogManager.h>

#include <cstdlib>
#include <filesystem>
#include <sys/stat.h>

namespace FEXCore::Paths {
  std::unique_ptr<std::string> CachePath;
  std::unique_ptr<std::string> EntryCache;

  void InitializePaths() {
    CachePath = std::make_unique<std::string>();
    EntryCache = std::make_unique<std::string>();

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
    // Ensure the folder structure is created for our Data
    if (!std::filesystem::exists(*EntryCache, ec) &&
        !std::filesystem::create_directories(*EntryCache, ec)) {
      LogMan::Msg::D("Couldn't create EntryCache directory: '%s'", EntryCache->c_str());
    }
  }

  void ShutdownPaths() {
    CachePath.reset();
    EntryCache.reset();
  }

  std::string GetCachePath() {
    return *CachePath;
  }

  std::string GetEntryCachePath() {
    return *EntryCache;
  }
}
