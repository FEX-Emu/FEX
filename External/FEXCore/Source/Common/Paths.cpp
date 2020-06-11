#include "Common/Paths.h"
#include "LogManager.h"

#include <cstdlib>
#include <filesystem>
#include <sys/stat.h>

namespace FEXCore::Paths {
  std::string CachePath;
  std::string EntryCache;

  void InitializePaths() {
    char const *HomeDir = getenv("HOME");

    if (!HomeDir) {
      HomeDir = getenv("PWD");
    }

    if (!HomeDir) {
      HomeDir = "";
    }

    char *XDGDataDir = getenv("XDG_DATA_DIR");
    if (XDGDataDir) {
      CachePath = XDGDataDir;
    }
    else {
      if (HomeDir) {
        CachePath = HomeDir;
      }
    }

    CachePath += "/.fex-emu/";
    EntryCache = CachePath + "/EntryCache/";

    // Ensure the folder structure is created for our Data
    if (!std::filesystem::exists(EntryCache) &&
        !std::filesystem::create_directories(EntryCache)) {
      LogMan::Msg::D("Couldn't create EntryCache directory: '%s'", EntryCache.c_str());
    }
  }

  std::string GetCachePath() {
    return CachePath;
  }

  std::string GetEntryCachePath() {
    return EntryCache;
  }
}
