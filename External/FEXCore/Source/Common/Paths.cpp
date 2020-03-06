#include "Common/Paths.h"

#include <cstdlib>
#include <sys/stat.h>

namespace FEXCore::Paths {
  std::string DataPath;
  std::string EntryCache;

  void InitializePaths() {
    char *HomeDir = getenv("HOME");
    char *XDGDataDir = getenv("XDG_DATA_DIR");
    if (XDGDataDir) {
      DataPath = XDGDataDir;
    }
    else {
      if (HomeDir) {
        DataPath = HomeDir;
      }
    }
    DataPath += "/.fexcore/";
    EntryCache = DataPath + "/EntryCache/";
    mkdir(DataPath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    mkdir(EntryCache.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
  }

  std::string GetDataPath() {
    return DataPath;
  }
}
