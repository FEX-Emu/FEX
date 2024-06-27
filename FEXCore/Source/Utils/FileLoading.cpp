// SPDX-License-Identifier: MIT
#include <FEXCore/fextl/string.h>
#include <FEXCore/fextl/vector.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <span>
#include <unistd.h>
#ifdef _WIN32
#include <fstream>
#endif

namespace FEXCore::FileLoading {

#ifndef _WIN32
template<typename T>
static bool LoadFileImpl(T& Data, const fextl::string& Filepath, size_t FixedSize) {
  int FD = open(Filepath.c_str(), O_RDONLY);

  if (FD == -1) {
    return false;
  }

  size_t FileSize {};
  if (FixedSize == 0) {
    struct stat buf;
    if (fstat(FD, &buf) == 0) {
      FileSize = buf.st_size;
    }
  } else {
    FileSize = FixedSize;
  }

  ssize_t Read = -1;
  bool LoadedFile {};
  if (FileSize) {
    // File size is known upfront
    Data.resize(FileSize);
    Read = pread(FD, &Data.at(0), FileSize, 0);

    LoadedFile = Read == FileSize;
  } else {
    // The file is either empty or its size is unknown (e.g. procfs data).
    // Try reading in chunks instead
    ssize_t CurrentOffset = 0;
    constexpr size_t READ_SIZE = 4096;
    Data.resize(READ_SIZE);

    while ((Read = pread(FD, &Data.at(CurrentOffset), READ_SIZE, CurrentOffset)) == READ_SIZE) {
      CurrentOffset += Read;
      Data.resize(CurrentOffset + Read);
    }

    if (Read == -1) {
      Data.clear();
      close(FD);
      return false;
    }

    // Final resize to ensure there is no garbage data past the end.
    Data.resize(CurrentOffset + Read);

    LoadedFile = true;
  }
  close(FD);
  return LoadedFile;
}

ssize_t LoadFileToBuffer(const fextl::string& Filepath, std::span<char> Buffer) {
  int FD = open(Filepath.c_str(), O_RDONLY);

  if (FD == -1) {
    return -1;
  }

  ssize_t Read = pread(FD, Buffer.data(), Buffer.size(), 0);
  close(FD);
  return Read;
}

#else
template<typename T>
static bool LoadFileImpl(T& Data, const fextl::string& Filepath, size_t FixedSize) {
  std::ifstream f(Filepath.c_str(), std::ios::binary | std::ios::ate);
  if (f.fail()) {
    return false;
  }
  auto Size = f.tellg();
  f.seekg(0, std::ios::beg);
  Data.resize(Size);
  f.read(Data.data(), Size);
  return !f.fail();
}

ssize_t LoadFileToBuffer(const fextl::string& Filepath, std::span<char> Buffer) {
  std::ifstream f(Filepath.c_str(), std::ios::binary | std::ios::ate);
  return f.readsome(Buffer.data(), Buffer.size());
}

#endif

bool LoadFile(fextl::vector<char>& Data, const fextl::string& Filepath, size_t FixedSize) {
  return LoadFileImpl(Data, Filepath, FixedSize);
}

bool LoadFile(fextl::string& Data, const fextl::string& Filepath, size_t FixedSize) {
  return LoadFileImpl(Data, Filepath, FixedSize);
}

} // namespace FEXCore::FileLoading
