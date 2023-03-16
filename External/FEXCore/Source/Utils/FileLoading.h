#pragma once
#include <FEXCore/fextl/vector.h>

#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <span>

namespace FEXCore::FileLoading {
  /**
   * @brief Loads a filepath in to a vector of data
   *
   * @param Data The vector to load the file data in to
   * @param Filepath The filepath to load
   *
   * @return true on file loaded, false on failure
   */
  // TODO: Delete once all uses of std::vector LoadFile is removed.
  bool LoadFile(std::vector<char> &Data, const std::string &Filepath, size_t FixedSize = 0);
  bool LoadFile(fextl::vector<char> &Data, const std::string &Filepath, size_t FixedSize = 0);

  /**
   * @brief Loads a filepath in to a buffer of data with a fixed size
   *
   * @param Filepath The filepath to load
   * @param Buffer The buffer to load the data in to. Attempting to read the full size of the span
   *
   * @return The amount of data read or -1 on error.
   */
  ssize_t LoadFileToBuffer(const std::string &Filepath, std::span<char> Buffer);
}

