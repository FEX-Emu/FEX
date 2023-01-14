#pragma once
#include <fcntl.h>
#include <string>
#include <unistd.h>

namespace FHU::Filesystem {
  /**
   * @brief Check if a filepath exists.
   *
   * @param Path The path to check for.
   *
   * @return True if the file exists, False if it doesn't.
   */
  inline bool Exists(const std::string &Path) {
    return access(Path.c_str(), F_OK) == 0;
  }
}
