#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>

namespace FEXCore::FileLoading {
  /**
   * @brief Loads a filepath in to a vector of data
   *
   * @param Data The vector to load the file data in to
   * @param Filepath The filepath to load
   *
   * @return true on file loaded, false on failure
   */
  bool LoadFile(std::vector<char> &Data, const std::string &Filepath, size_t FixedSize = 0);
}

