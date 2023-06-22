#pragma once

#include "Common/FEXServerClient.h"

#include <FEXCore/Config/Config.h>
#include <FEXCore/fextl/memory.h>
#include <FEXCore/fextl/unordered_set.h>

#include <elf.h>

namespace CoreFileWriter {
  class CoreFileWriter {
    public:
      virtual ~CoreFileWriter() = default;

      virtual void ConsumeGuestContext(FEXServerClient::CoreDump::PacketGuestContext *Req) = 0;
      virtual void ConsumeGuestXState(FEXServerClient::CoreDump::PacketGuestXState *Req) = 0;
      virtual void ConsumeGuestAuxv(const void* auxv, uint64_t AuxvSize) = 0;
      virtual void ConsumeMapsFD(int FD) = 0;
      virtual void Write() = 0;
      void ConsumeCoreDumpFilter(int FD) {
        lseek(FD, 0, SEEK_SET);
        char Line[10];
        read(FD, Line, sizeof(Line));
        CoreDumpFilter = std::strtoull(Line, nullptr, 16);
      }

      using FDFetcher = std::function<int(std::string_view const Filename)>;
      virtual void GetMappedFDs(FDFetcher Fetch) = 0;

      using MemoryFetcher = std::function<bool(void *ResultMemory, uint64_t MemoryBase, size_t MemorySize)>;
      void SetMemoryFetcher(MemoryFetcher Fetch) {
        MemoryFetch = Fetch;
      }

      void SetApplicationName(std::string_view ApplicationName) {
        this->ApplicationName = ApplicationName;
      }

      void SetPID(uint32_t PID) {
        this->PID = PID;
      }
      void SetUID(uint32_t UID) {
        this->UID = UID;
      }

      void SetTimestamp(uint64_t Timestamp) {
        this->Timestamp = Timestamp;
      }

      enum CoreDumpFilterBits {
        // First two bits of the filter are to determine how the coredump is dumped.
        // FEX can't actually use Root dumping but if dumping is set to disabled then we can ignore dumping.
        DUMP_STYLE_MASK        = 0b11,
        DUMP_DISABLE           = 0,
        DUMP_USER              = 1,
        DUMP_ROOT              = 2,

        DUMP_ANONYMOUS_PRIVATE = (1U << 2),
        DUMP_ANONYMOUS_SHARED  = (1U << 3),
        DUMP_FILE_PRIVATE      = (1U << 4),
        DUMP_FILE_SHARED       = (1U << 5),
        // Bit 6: ELF Headers?
        // Bit 7: HUGETLB Private
        // Bit 8: HUGETLB Shared
        // Bit 9: DAX Private
        // Bit 10: DAX Shared
      };

      void CleanupOldCoredumps();
    protected:
      // Get the current offset of the FD.
      off_t GetCurrentOffset(int FD) {
        return lseek(FD, 0, SEEK_CUR);
      }

      // Writes zeroes in to the FD.
      void WriteZero(int FD, size_t Bytes) {
        const auto CurrentOffset = GetCurrentOffset(FD);
        ftruncate(FD, CurrentOffset + Bytes);
        lseek(FD, 0, SEEK_END);
      }

      // Align the FD offset.
      void AlignFD(int FD, size_t Alignment) {
        const auto CurrentOffset = GetCurrentOffset(FD);
        auto CurrentAlignment = CurrentOffset % Alignment;
        if (CurrentAlignment) {
          const auto NeedToWrite = Alignment - CurrentAlignment;
          WriteZero(FD, NeedToWrite);
        }
      }

      std::pair<std::optional<fextl::string>, int> OpenCoreDumpFile();

      MemoryFetcher MemoryFetch{};
      fextl::string ApplicationName{};
      uint32_t PID;
      uint32_t UID;
      uint64_t Timestamp{};
      uint64_t CoreDumpFilter{};


      FEX_CONFIG_OPT(CompressZSTD, COREDUMP_COMPRESS_ZSTD);
      FEX_CONFIG_OPT(CoreDumpConfigFolder, COREDUMP_FOLDER);
      FEX_CONFIG_OPT(CoreDumpMaximumSize, COREDUMP_MAXIMUM_THRESHOLD);
      FEX_CONFIG_OPT(CoreDumpMaximumAge, COREDUMP_MAXIMUM_AGE);

      bool CompressCoredump(const std::string_view File);
  };
  fextl::unique_ptr<CoreFileWriter> CreateWriter64();
  fextl::unique_ptr<CoreFileWriter> CreateWriter32();

}
