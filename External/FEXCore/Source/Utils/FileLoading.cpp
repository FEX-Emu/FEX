#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <fcntl.h>
#include <unistd.h>
#include <span>

namespace FEXCore::FileLoading {
bool LoadFile(std::vector<char> &Data, const std::string &Filepath, size_t FixedSize) {
  std::fstream ConfigFile;
  ConfigFile.open(Filepath, std::ios::in);

  if (!ConfigFile.is_open()) {
    return false;
  }

  size_t FileSize{};

  if (FixedSize == 0) {
    if (!ConfigFile.seekg(0, std::fstream::end)) {
      return false;
    }

    FileSize = ConfigFile.tellg();
    if (ConfigFile.fail()) {
      return false;
    }

    if (!ConfigFile.seekg(0, std::fstream::beg)) {
      return false;
    }
  }
  else {
    FileSize = FixedSize;
  }

  if (FileSize > 0) {
    Data.resize(FileSize);
    if (!ConfigFile.read(&Data.at(0), FileSize)) {
      // Probably means permissions aren't set. Just early exit
      return false;
    }
    ConfigFile.close();
  }
  else {
    return false;
  }

  return true;
}

ssize_t LoadFileToBuffer(const std::string &Filepath, std::span<char> Buffer) {
  int FD = open(Filepath.c_str(), O_RDONLY);

  if (FD == -1) {
    return -1;
  }

  ssize_t Read = pread(FD, Buffer.data(), Buffer.size(), 0);
  close(FD);
  return Read;
}

}
