// SPDX-License-Identifier: MIT
#pragma once

#include <Common/Config.h>
#include <Common/FEXServerClient.h>
#include <FEXCore/Core/CodeCache.h>

#include <optional>
#include <sys/file.h>

namespace FEX {

class CodeMapWriterImpl : public FEXCore::CodeMapWriter {
public:
  CodeMapWriterImpl(size_t BufferSize)
    : FEXCore::CodeMapWriter(BufferSize) {}
  ~CodeMapWriterImpl() {
    if (CodeMapFD.value_or(-1) != -1) {
      Flush(BufferOffset);
      close(*CodeMapFD);
    }
  }

private:
  bool IsWriteEnabled(const FEXCore::ExecutableFileSectionInfo& Section) override {
    if (CodeMapFD == -1) {
      return false;
    }

    if (Section.FileStartVA == 0) {
      return false;
    }

    // PV libraries can't yet be read by FEXServer, so skip dumping them
    // TODO: Also disable Wine
    if (Section.FileInfo.Filename.starts_with("/run/pressure-vessel")) {
      return false;
    }

    std::unique_lock CodeBufferLock {CodeMapWriteMutex};
    if (!CodeMapFD) {
      // return false;
      // Query from FEXServer whether this is the first instance of this executable; if it is, also enable code dumping!
      FEX_CONFIG_OPT(RootFSPath, ROOTFS);
      FEX_CONFIG_OPT(Multiblock, MULTIBLOCK);
      auto ProgramName = FEXCore::Config::Get(FEXCore::Config::CONFIG_APP_FILENAME);
      LOGMAN_THROW_A_FMT(ProgramName && ProgramName.value()->c_str()[0] == '/', "");

      // Check RootFS first, then the plain path
      auto ProgramFD = open((RootFSPath() + ProgramName.value()->c_str()).c_str(), O_RDONLY);
      if (ProgramFD == -1) {
        ProgramFD = open(ProgramName.value()->c_str(), O_RDONLY);
      }
      if (ProgramFD == -1) {
        CodeMapFD = -1;
        return false;
      }

      CodeMapFD = FEXServerClient::RequestCodeMapFD(FEXServerClient::GetServerFD(), ProgramFD, Multiblock);
      close(ProgramFD);
      if (CodeMapFD == -1) {
        return false;
      }

      // TODO: Recheck if this is needed if FEXServer already sets that flag.
      auto flags = fcntl(CodeMapFD.value(), F_GETFD);
      fcntl(CodeMapFD.value(), F_SETFD, flags | FD_CLOEXEC);
    }

    return true;
  }

  void CommitData(std::span<const std::byte> Data) override {
    std::unique_lock CodeBufferLock {CodeMapWriteMutex};
    write(*CodeMapFD, Data.data(), Data.size_bytes());
  }

  // std::nullopt: We haven't requested a CodeMapFD yet
  // value is -1:  We requested a CodeMapFD but FEXServer told us not to write any data
  // other values: Code map writing is active
  std::optional<int> CodeMapFD;
  std::mutex CodeMapWriteMutex; // TODO: Err, should probably use the parent Mutex!
};

} // namespace FEX
