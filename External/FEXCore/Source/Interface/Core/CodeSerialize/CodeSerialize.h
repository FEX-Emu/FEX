#pragma once

#include "Interface/Core/ArchHelpers/Relocations.h"
#include "Interface/IR/AOTIR.h"

#include <FEXCore/Debug/InternalThreadState.h>
#include <FEXCore/Utils/Event.h>
#include <FEXCore/Utils/RefCountMutex.h>
#include <FEXCore/Utils/Threads.h>

#include <fcntl.h>
#include <fstream>
#include <map>
#include <unordered_map>
#include <shared_mutex>
#include <sys/uio.h>
#include <sys/mman.h>
#include <tsl/robin_map.h>

#if 0
#define AOTLOG(...) LogMan::Msg::DFmt(__VA_ARGS__)
#else
#define AOTLOG(...) do {} while(0)
#endif

namespace FEXCore::CodeSerialize {
  struct CodeSerializationConfig {
    // If any of these mismatch on load then it will be invalidated
    // Any of these result in codegen changes
    uint64_t Cookie{};

    // Number of instructions per block configuration
    int32_t MaxInstPerBlock{};

    // Follows CPUID 4000_0001_EAX[3:0]
    unsigned Arch : 4;
    // Multiblock enabled
    bool MultiBlock: 1;
    // TSO enabled
    bool TSOEnabled : 1;
    // ABI local flag unsafe optimization
    bool ABILocalFlags : 1;
    // ABI no PF unsafe optimization
    bool ABINoPF : 1;
    // static register allocation enabled
    bool SRA : 1;
    // Paranoid TSO mode enabled
    bool ParanoidTSO : 1;
    // Guest code execution mode (We don't support live mode switch)
    bool Is64BitMode : 1;
    // SMC checks style
    unsigned SMCChecks: 2;
    // Padding to remove uninitialized data warning from asan
    // Shows remaining amount of bits available for config
    unsigned _Pad : 18;

    bool operator==(const CodeSerializationConfig &other) const {
      return Cookie == other.Cookie &&
        MaxInstPerBlock == other.MaxInstPerBlock &&
        Arch == other.Arch &&
        TSOEnabled == other.TSOEnabled &&
        ABILocalFlags == other.ABILocalFlags &&
        ABINoPF == other.ABINoPF &&
        SRA == other.SRA &&
        ParanoidTSO == other.ParanoidTSO &&
        Is64BitMode == other.Is64BitMode &&
        SMCChecks == other.SMCChecks;
    }

    static uint64_t GetHash(const CodeSerializationConfig &other) {
      // For < 64-bits of data just pack directly
      // Skip the cookie
      uint64_t Hash{};
      Hash <<= 32; Hash |= other.MaxInstPerBlock;
      Hash <<= 1;  Hash |= other.Arch;
      Hash <<= 1;  Hash |= other.MultiBlock;
      Hash <<= 1;  Hash |= other.TSOEnabled;
      Hash <<= 1;  Hash |= other.ABILocalFlags;
      Hash <<= 1;  Hash |= other.ABINoPF;
      Hash <<= 1;  Hash |= other.SRA;
      Hash <<= 1;  Hash |= other.ParanoidTSO;
      Hash <<= 1;  Hash |= other.Is64BitMode;
      Hash <<= 2;  Hash |= other.SMCChecks;
      return Hash;
    }
  };

  static_assert(sizeof(CodeSerializationConfig) == 16, "Size changed");
  static_assert((sizeof(CodeSerializationConfig) - sizeof(uint64_t)) == 8, "Config size exceeded 64its. Need to change how the hash is generated!");

  struct CodeSerializationHeader {
    CodeSerializationConfig Config;
    uint64_t OriginalBase{};
    uint64_t OriginalOffset{};
    uint64_t TotalCodeSize{};
    // Used to reserve the TSL map
    uint64_t NumCodeEntries{};
    // The number of relocations that point to this section
    uint64_t NumRelocationsTo;
    uint64_t TotalRelocationCount{};
  };

  struct CodeSerializationData {
    // RIP offset in to the named region from its base
    uint64_t RIPOffset;
    uint64_t HostCodeHash;
    uint64_t HostCodeLength;
    uint64_t NumRelocations;
    uint64_t RelocationSize;
    // XXX: Support multiblock
    uint64_t GuestCodeHash;
    uint64_t GuestCodeLength;
  };

  struct FileData {
    struct DataSection {
      bool Serialized;
      bool Invalid;
      const CodeSerializationData *Data;
      const char *HostCode;
      uint64_t NumRelocations;
      const char* Relocations;
    };

    char *CodeData{};
    size_t FileSize{};

    std::vector<DataSection> FileCodeSections;
  };

  struct AddrToFileEntry {
    // The base of the File loaded in memory
    uint64_t Base{};
    // The length of this code entry
    uint64_t Len{};

    // The offset inside the file that is mapped to Base
    uint64_t Offset{};

    std::string filename{};
    std::string SourceCodePath{};

    CodeSerializationHeader CodeHeader{};

    // Ref count for how many jobs are waiting to be written to this object
    AOTMutex ObjectJobRefCountMutex;

    // Ref count for number of pending named jobs
    AOTMutex NamedJobRefCountMutex;

    // If the file was deleted or corrupted then stop serializing to it
    bool StillSerializing {true};

    // Long living serialize FD if we have multiple serialize jobs
    int CurrentSerializeFD{-1};

    FileData Data{};
    // This per section map is what takes the most time to load
    tsl::robin_map<uint64_t, FileData::DataSection*> SectionLookupMap{};

    // Loading information
    LatchEvent LoadingComplete;

    // Default initialization
    AddrToFileEntry() = default;

    // Initializer specifically for threaded loading
    AddrToFileEntry(uint64_t _Base,
      uint64_t _Size,
      uint64_t _Offset,
      std::string const &_filename,
      CodeSerializationHeader const &_DefaultHeader)
    : Base {_Base}
    , Len {_Size}
    , Offset {_Offset}
    , filename {_filename}
    , CodeHeader {_DefaultHeader} {

    }
  };

  // Map type must use an interator that isn't invalidation on erase/insert
  using AddrToFileMapType = std::map<uint64_t, std::unique_ptr<AddrToFileEntry>>;
  using AddrToFilePtrMapType = std::map<uint64_t, AddrToFileEntry*>;

  class CodeSerializeService;

  class NamedRegionHandling final {
    public:
      NamedRegionHandling(FEXCore::Context::Context *ctx, CodeSerializeService *service);

      class NamedWorkItem {
        public:
        enum class QueueWorkType {
          TYPE_ADD,
          TYPE_REMOVE,
        };

        NamedWorkItem(QueueWorkType type)
          : Type {type} {}

        QueueWorkType GetType() const { return Type; }

        private:
          QueueWorkType Type;
      };

      // Adding a named region job
      class WorkItemAdd final : public NamedWorkItem {
        public:
          WorkItemAdd(const std::string &base, const std::string &filename, bool executable, AddrToFileMapType::iterator entry)
            : NamedWorkItem {QueueWorkType::TYPE_ADD}
            , BaseFilename {base}
            , Filename {filename}
            , Executable {executable}
            , Entry {entry}
            {}
          const std::string BaseFilename;
          const std::string Filename;
          bool Executable;
          AddrToFileMapType::iterator Entry;
      };

      // Removing a named region job
      class WorkItemRemove final : public NamedWorkItem {
        public:
          WorkItemRemove(uint64_t base, uint64_t size, AddrToFileMapType::iterator entry)
            : NamedWorkItem { QueueWorkType::TYPE_REMOVE}
            , Base {base}
            , Size {size}
            , Entry {entry} {}

          uint64_t Base;
          uint64_t Size;
          AddrToFileMapType::iterator Entry;
      };

      void AddNamedRegionJob(uintptr_t Base, uintptr_t Size, uintptr_t Offset, const std::string &filename, bool Executable);
      void RemoveNamedRegionJob(uintptr_t Base, uintptr_t Size);
      void DrainNameJobQueue();
      void CleanupAfterFork();

      const CodeSerializationConfig &GetDefaultSerializationConfig() const {
        return DefaultSerializationConfig;
      }

    protected:
      friend class CodeSerializeService;
      void HandleNamedRegionJobs();

      // Return a default code header based off the default serialization config
      CodeSerializationHeader DefaultCodeHeader(uint64_t Base, uint64_t Offset) const {
        CodeSerializationHeader Header{};
        Header.Config = DefaultSerializationConfig;

        Header.OriginalBase = Base;
        Header.OriginalOffset = Offset;
        Header.NumCodeEntries = 0;
        Header.NumRelocationsTo = 0;
        Header.TotalRelocationCount = 0;
        return Header;
      }

    private:
      // Code version. If the code emission changes then this needs to increment
      constexpr static uint32_t CODE_VERSION = 0x0;

      // Default cookie header for the file header
      constexpr static uint64_t CODE_COOKIE = FEXCore::IR::COOKIE_VERSION("FEXC", CODE_VERSION);

      // Code serialization config for our current process configuration
      CodeSerializationConfig DefaultSerializationConfig{};

      // Mutex for adding new jobs to the work queue
      std::mutex NamedQueueMutex{};

      // Atomic counter for number of jobs in the queue without needing to pull the mutex to check
      std::atomic<uint64_t> NamedWorkQueueJobs{};

      // The job queue
      // Jobs get consumed as FIFO
      // Jobs always get appended to the end
      std::queue<std::unique_ptr<NamedWorkItem>> NamedWorkQueue{};

      CodeSerializeService *CodeService{};
  };

  class CodeSerializeService final {
    public:
      AOTMutex &GetFileMapLock() { return AOTIRCacheLock; }
      AddrToFileMapType &GetFileMap() { return AddrToFile; }

      std::shared_mutex &GetUnrelocatedFileMapLock() { return AOTIRUnrelocatedCacheLock; }
      AddrToFilePtrMapType &GetUnrelocatedFileMap() { return UnrelocatedAddrToFile; }

      void DoClosure(AddrToFileMapType::iterator it);

      void NotifyWork() { StartWork.NotifyAll(); }

      CodeSerializeService(FEXCore::Context::Context *ctx);
      ~CodeSerializeService();
      void Shutdown();

      struct AOTData {
        uint64_t GuestRIP;
        void *HostCodeBegin;
        size_t HostCodeLength;
        uint64_t HostCodeHash;
        // XXX: Support multiblock
        uint64_t GuestCodeLength;
        AOTMutex *ThreadRefCountAOT;
        uint64_t GuestCodeHash;
        std::vector<FEXCore::CPU::Relocation> Relocations;

        // Will remain valid due to ref counting
        AddrToFileMapType::iterator MapIter;

        // Pointer to the named region's reference counter
        AOTMutex *ObjectJobRefCountMutexPtr;
      };

      void AddSerializeJob(std::unique_ptr<AOTData> Data);

      void AddNamedRegionJob(uintptr_t Base, uintptr_t Size, uintptr_t Offset, const std::string &filename, bool Executable) {
        NamedRegionHandling.AddNamedRegionJob(Base, Size, Offset, filename, Executable);
      }

      void RemoveNamedRegionJob(uintptr_t Base, uintptr_t Size) {
        NamedRegionHandling.RemoveNamedRegionJob(Base, Size);
      }

      void PrepareForExecve(FEXCore::Core::InternalThreadState *Thread);
      void CleanupAfterExecve(FEXCore::Core::InternalThreadState *Thread);
      void PrepareForFork(FEXCore::Core::InternalThreadState *Thread);
      void CleanupAfterFork(FEXCore::Core::InternalThreadState *LiveThread, bool Parent);

      const FileData::DataSection* PreGenerateCodeFetch(uint64_t GuestRIP);

      uint64_t FindRelocatedRIP(uint64_t GuestRIP);

      // Public for threading
      void ExecutionThread();

      void Kick() { StartWork.NotifyAll(); }

    protected:
      friend class NamedRegionHandling;
      void AddNamedRegion(AddrToFileMapType::iterator Entry, const std::string &base_filename, const std::string &filename, bool Executable);
      void RemoveNamedRegion(uintptr_t Base, uintptr_t Size);
      void RemoveNamedRegionByIterator(AddrToFileMapType::iterator Entry);

    private:
      FEXCore::Context::Context *CTX;
      NamedRegionHandling NamedRegionHandling;

      Event StartWork{};
      std::shared_mutex WorkingMutex{};

      std::shared_mutex SerializeLock;
      AOTMutex AOTIRCacheLock;
      std::shared_mutex AOTIRUnrelocatedCacheLock;

      // Mutex to block removing entries
      // Allows inserting without without blocking while removing will block
      std::shared_mutex AOTIRCacheAddLock;
      std::shared_mutex AOTIRCacheRemoveLock;

      std::shared_mutex NamedRegionModifying;

      std::unique_ptr<FEXCore::Threads::Thread> WorkerThread;
      std::atomic_bool ShuttingDown{false};

      std::atomic<size_t> RemainingFileSerializations{};

      std::mutex QueueMutex{};
      std::queue<std::unique_ptr<AOTData>> WorkQueue{};

      AddrToFileMapType AddrToFile{};
      AddrToFilePtrMapType UnrelocatedAddrToFile;

      AddrToFileMapType::iterator FindAddrForFile(uint64_t Entry);
      AddrToFilePtrMapType::iterator FindUnrelocatedAddrForFile(uint64_t Entry);

      bool AppendSerializeDataFD(int fd, AddrToFileMapType::iterator it, CodeSerializationData *CodeData, AOTData *Data);
      void AppendSerializeData(AddrToFileMapType::iterator it, CodeSerializationData *CodeData, AOTData *Data);
      void Serialize(AddrToFileMapType::iterator it, AOTData *Data);

      void SerializeHandler(std::unique_ptr<AOTData> Data);
      void DrainAOTJobQueue();

    private:
      void Initialize();
  };

  [[maybe_unused]]
  static int OpenFDForAppend(const char *Filename) {
    static int DefaultFlags = O_RDWR | O_CLOEXEC | O_NOATIME;
    auto fd = open(Filename, DefaultFlags, (S_IRUSR | S_IWUSR) | (S_IRGRP | S_IWGRP) | (S_IROTH | S_IWOTH));
    if (fd == -1 && errno == EPERM) {
      // Try again without NOATIME
      // Some filesystems don't allow NOATIME, like sshfs
      DefaultFlags &= ~O_NOATIME;
      fd = open(Filename, DefaultFlags, (S_IRUSR | S_IWUSR) | (S_IRGRP | S_IWGRP) | (S_IROTH | S_IWOTH));
    }

    return fd;
  }

  [[maybe_unused]]
  static void GetFDLock(int fd, short type) {
    // Advisory locks (F_SETLKW)
    // - Not inherited by fork
    // - preserved across execve unless O_CLOEXEC is set
    // OFD locks (F_OFD_SETLKW)
    // - associated with the FD, so inherited by anything that inherits FD, So both fork and execve unless O_CLOEXEC set.
#if 1
    flock lk {
      .l_type = type,
      .l_whence = SEEK_CUR,
      .l_start = 0, // Everything
      .l_len = 0, // Everything
    };
    while (fcntl(fd, F_OFD_SETLKW, &lk) == -1);
#else
    flock lk {
      .l_type = type,
      .l_whence = SEEK_CUR,
      .l_start = 0, // Everything
      .l_len = 0, // Everything
    };
    while (fcntl(fd, F_SETLKW, &lk) == -1);
#endif
  }

  [[maybe_unused]]
  static void ReadHeader(int fd, FEXCore::CodeSerialize::CodeSerializationHeader *CodeHeader) {
    [[maybe_unused]] ssize_t Result = ::pread(fd, CodeHeader, sizeof(*CodeHeader), 0);
    LOGMAN_THROW_A_FMT(Result != -1, "Failed to read header");
  }

  [[maybe_unused]]
  static void WriteHeader(int fd, FEXCore::CodeSerialize::CodeSerializationHeader *CodeHeader) {
    [[maybe_unused]] int Result = ::pwrite(fd, CodeHeader, sizeof(*CodeHeader), 0);
    LOGMAN_THROW_A_FMT(Result != -1, "Had an error serializing Code Header. {} {}", errno, strerror(errno));
  }

  [[maybe_unused]]
  static void* ThreadHandler(void *Arg) {
    FEXCore::CodeSerialize::CodeSerializeService *This = reinterpret_cast<FEXCore::CodeSerialize::CodeSerializeService*>(Arg);
    This->ExecutionThread();
    return nullptr;
  }
}
