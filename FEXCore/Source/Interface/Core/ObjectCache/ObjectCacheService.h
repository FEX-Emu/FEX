#pragma once
#include "Interface/Context/Context.h"
#include "Interface/Core/ObjectCache/Relocations.h"
#include "Interface/Core/ObjectCache/CodeObjectSerializationConfig.h"
#include "Interface/IR/AOTIR.h"

#include <FEXCore/Utils/Event.h>
#include <FEXCore/Utils/Threads.h>
#include <FEXCore/fextl/map.h>
#include <FEXCore/fextl/memory.h>
#include <FEXCore/fextl/queue.h>
#include <FEXCore/fextl/robin_map.h>
#include <FEXCore/fextl/string.h>
#include <FEXCore/fextl/vector.h>

#include <shared_mutex>

namespace FEXCore::CodeSerialize {
  // XXX: Does this need to be signal safe?
  using CodeSerializationMutex = std::shared_mutex;
  struct CodeSerializationData {
  };

  struct CodeObjectFileSection {
    bool Serialized;
    bool Invalid;
    const CodeSerializationData *Data;
    const char *HostCode;
    uint64_t NumRelocations;
    const char *Relocations;
  };

  /**
   * @brief This is the file header that lives at the start of an object cache file
   *
   * This header is updated from multiple processes!
   * Care must be taken to use OS locks when updating the file backing including this header
   */
  struct CodeObjectSerializationHeader {
    // The configuration that this file has
    CodeObjectSerializationConfig Config;
    // The original RIP that this object section was mapped at
    uint64_t OriginalBase{};
    // The original offset in to the file that this object section was loaded from
    uint64_t OriginalOffset{};
    // Total amount of code that should be in this file
    uint64_t TotalCodeSize{};
    // Used to reserve the TSL map
    uint64_t NumCodeEntries{};
    // The number of relocations that point to this section
    uint64_t NumRelocationsTo{};
    // Total relocations in this file
    uint64_t TotalRelocationsCount{};
  };

  struct CodeRegionEntry {
    /**
     * @name Threaded initialization objects for the initial object creation
     * @{ */
      // Base address in memory where the code region is at
      uint64_t Base{};

      // Size of this code entry
      uint64_t Size{};

      // The offset inside the file that is mapped to Base
      uint64_t Offset{};

      // Filename of the object
      fextl::string Filename{};

      CodeObjectSerializationHeader EntryHeader{};
    /**  @} */

    // The filename of the object cache for this entry
    fextl::string ObjectEntrySourceFilename{};

    // In the case of file corruption that we can detect, we can disable serialization early for an entry
    // We should be resiliant to corruption but things happen
    bool StillSerializing {true};

    // Long lived FD for serialization if we have multiple jobs to serialize
    // Bursts of code entries are common and this reduces file lock overhead
    //
    // Especially useful over network mounts where file locks are very slow
    int CurrentSerializedFD {-1};

    /**
     * @name Objects required to sync objects between threads
     * @{ */
      // Refcount for the number of outstanding code entries waiting to be written for this object section
      CodeSerializationMutex ObjectJobRefCountMutex;

      // Refcount for outstanding named object region entry loading itself
      // Will block JIT code cache look up when this has a unique_lock held
      CodeSerializationMutex NamedJobRefCountMutex;
    /**  @} */

    /**
     * @name Object Entry data management
     * @{ */

      /**
       * @name This is the raw file data that we loaded from the code region entry file
       * @{ */
        char *CodeData{};
        size_t FileSize{};

        fextl::vector<CodeObjectFileSection> FileCodeSections;
      /**  @} */

      // This per section map takes the most time to load and needs to be quick
      // This is the map of all code segments for this entry
      fextl::robin_map<uint64_t, CodeObjectFileSection*> SectionLookupMap{};
    /**  @} */

    // Default initialization
    CodeRegionEntry() = default;

    // Initializer specifically for threaded loading
    CodeRegionEntry(uint64_t Base,
      uint64_t Size,
      uint64_t Offset,
      fextl::string const &Filename,
      CodeObjectSerializationHeader const &DefaultHeader)
      : Base {Base}
      , Size {Size}
      , Offset {Offset}
      , Filename {Filename}
      , EntryHeader {DefaultHeader} {
      }
  };

  // Map type must use an interator that isn't invalidation on erase/insert
  using CodeRegionMapType = fextl::map<uint64_t, fextl::unique_ptr<CodeRegionEntry>>;
  using CodeRegionPtrMapType = fextl::map<uint64_t, CodeRegionEntry*>;

  class NamedRegionObjectHandler;
  class CodeObjectSerializeService;

  class AsyncJobHandler final {
    public:
      /**
       * @brief Structure containing all the data required to async serialize code objects
       */
      struct SerializationJobData {
        uint64_t GuestRIP;        ///< The RIP for the guest
        // XXX: Support multiblock
        uint64_t GuestCodeLength; ///< The Guest's code length
        uint64_t GuestCodeHash;   ///< Hash of the guest code

        void *HostCodeBegin;      ///< Host JIT code starting memory address
        size_t HostCodeLength;    ///< Host JIT code length
        uint64_t HostCodeHash;    ///< Host JIT code hash before any backpatching

        // This is the thread specific ref counter for outstanding jobs.
        // This shared mutex is incremented when the job is added, then decremented when the job is complete.
        // If a thread is shutting down or clearing code cache then the thread will pull a unique lock on this mutex.
        // This way it will wait until the async job handler is complete with it.
        CodeSerializationMutex *ThreadJobRefCount;

        // These are the reolocations for this serialization job
        // Relatively small number of entries most of the time
        fextl::vector<FEXCore::CPU::Relocation> Relocations;

        /**
         * @name Objects filled in from the Code Object Serialization service when a job is added
         * @{ */
          // This is the code region's ref counter for outstanding jobs.
          // This shared mutex is incremented when the job is added, then decremented when the job is complete.
          // If a named region is being removed then a unique lock will be pulled to wait for all jobs to complete and no new jobs to be added.
          CodeSerializationMutex *ObjectJobRefCountMutexPtr;

          // This is the code region iterator to reduce the number of map lookups
          // This will remain valid while jobs are outstanding for this region
          CodeRegionMapType::iterator CodeRegionIterator;
        /**  @} */
      };

      AsyncJobHandler(NamedRegionObjectHandler *NamedRegionHandler, CodeObjectSerializeService *CodeObjectCacheService)
        : NamedRegionHandler {NamedRegionHandler}
        , CodeObjectCacheService {CodeObjectCacheService} {}

    protected:
      friend class CodeObjectSerializeService;
      friend class NamedRegionObjectHandler;
      /**
       * @name Async job submission functions
       * @{ */
        void AsyncAddNamedRegionJob(uintptr_t Base, uintptr_t Size, uintptr_t Offset, const fextl::string &filename);
        void AsyncRemoveNamedRegionJob(uintptr_t Base, uintptr_t Size);
        void AsyncAddSerializationJob(fextl::unique_ptr<SerializationJobData> Data);
      /**  @} */

      /**
       * @name Async named region handling
       * @{ */
        /**
         * @brief The async named region jobs to handle.
         *
         * Only two, Code serialization goes in to a different queue.
         */
        enum class NamedRegionJobType {
          JOB_ADD_NAMED_REGION,
          JOB_REMOVE_NAMED_REGION,
        };

        class NamedRegionWorkItem {
          public:
          NamedRegionJobType GetType() const { return Type; }

          protected:
            friend class WorkItemAddNamedRegion;
            NamedRegionWorkItem(NamedRegionJobType type)
              : Type {type} {}

          private:
            NamedRegionJobType Type;
        };

        class WorkItemAddNamedRegion : public NamedRegionWorkItem {
          public:
            WorkItemAddNamedRegion(const fextl::string &base, const fextl::string &filename, bool executable, CodeRegionMapType::iterator entry)
              : NamedRegionWorkItem {NamedRegionJobType::JOB_ADD_NAMED_REGION}
              , BaseFilename {base}
              , Filename {filename}
              , Executable {executable}
              , Entry {entry}
              {}
            const fextl::string BaseFilename;
            const fextl::string Filename;
            bool Executable;
            CodeRegionMapType::iterator Entry;
        };

        class WorkItemRemoveNamedRegion : public NamedRegionWorkItem {
          public:
            WorkItemRemoveNamedRegion(uint64_t base, uint64_t size, fextl::unique_ptr<CodeRegionEntry> entry)
              : NamedRegionWorkItem {NamedRegionJobType::JOB_REMOVE_NAMED_REGION}
              , Base {base}
              , Size {size}
              , Entry {std::move(entry)} {}

            uint64_t Base;
            uint64_t Size;
            fextl::unique_ptr<CodeRegionEntry> Entry;
        };
      /**  @} */

    private:
      NamedRegionObjectHandler *NamedRegionHandler;
      CodeObjectSerializeService *CodeObjectCacheService;
  };

  class NamedRegionObjectHandler final {
    public:
      NamedRegionObjectHandler(FEXCore::Context::ContextImpl *ctx);

      void HandleNamedRegionObjectJobs();

      CodeObjectSerializationConfig const &GetDefaultSerializationConfig() const {
        return DefaultSerializationConfig;
      }

    protected:
      friend class AsyncJobHandler;

      // Return a default code header based off the default serialization config
      CodeObjectSerializationHeader DefaultCodeHeader(uint64_t Base, uint64_t Offset) const {
        return CodeObjectSerializationHeader {
          .Config = DefaultSerializationConfig,
          .OriginalBase = Base,
          .OriginalOffset = Offset,
          .NumCodeEntries = 0,
          .NumRelocationsTo = 0,
          .TotalRelocationsCount = 0,
        };
      }

      /**
       * @brief Adds an asynchronous add named region work item to the object queue
       *
       * This adds the job that will do the loading of file resources and data tracking.
       */
      void AsyncAddNamedRegionWorkItem(const fextl::string &base, const fextl::string &filename, bool executable, CodeRegionMapType::iterator entry) {
        std::unique_lock lk {NamedWorkQueueMutex};
        WorkQueue.emplace(fextl::make_unique<AsyncJobHandler::WorkItemAddNamedRegion> (
          base,
          filename,
          executable,
          entry
        ));
        ++NamedWorkQueueJobs;
      }

      void AsyncRemoveNamedRegionWorkItem(uint64_t Base, uint64_t Size, fextl::unique_ptr<CodeRegionEntry> Entry) {
        std::unique_lock lk {NamedWorkQueueMutex};
        WorkQueue.emplace(fextl::make_unique<AsyncJobHandler::WorkItemRemoveNamedRegion> (
          Base,
          Size,
          std::move(Entry)
        ));
        ++NamedWorkQueueJobs;
      }

    private:
      // Code version. If the code emission changes then this needs to increment
      constexpr static uint32_t CODE_VERSION = 0x0;

      // Default cookie header for the file header
      constexpr static uint64_t CODE_COOKIE = FEXCore::IR::COOKIE_VERSION("FEXC", CODE_VERSION);

      // Code serialization config for our current process configuration
      CodeObjectSerializationConfig DefaultSerializationConfig;

      // Atomic counter for number of jobs in the queue without needing to pull the mutex to check
      std::atomic<uint64_t> NamedWorkQueueJobs{};

      // Mutex for ading new jobs to the work queue
      std::mutex NamedWorkQueueMutex{};

      // The job queue itself
      // Jobs get consumed as a FIFO
      // Jobs always get appended to the end
      fextl::queue<fextl::unique_ptr<AsyncJobHandler::NamedRegionWorkItem>> WorkQueue{};

      /**
       * @name Named Region object handling
       * @{ */
        void AddNamedRegionObject(CodeRegionMapType::iterator Entry, const fextl::string &base_filename, const fextl::string &filename, bool Executable);
        void RemoveNamedRegionObject(uintptr_t Base, uintptr_t Size, fextl::unique_ptr<CodeRegionEntry> Entry);
      /**  @} */
  };

  /**
   * @brief Context specific code object serialization class
   *
   * Contains everything required for FEXCore to serialize code objects
   */
  class CodeObjectSerializeService final {
    public:
      CodeObjectSerializeService(FEXCore::Context::ContextImpl *ctx);

      /**
       * @brief Initialize the internal interface
       *
       * Is a public interface to allow the service to reinitialize after forking
       */
      void Initialize();

      /**
       * @brief Safely shut down the Code Object serialization service.
       *
       * This service needs to be resiliant to application crashes, but shutting down safely is still preferred.
       */
      void Shutdown();

      /**
       * @name Async interface
       * @{ */
        /**
         * @brief Loads a named region in to the code serialization service. As async as possible.
         *
         * @param Base - Virtual address that this named region is loaded
         * @param Size - The size of the region
         * @param Offset - The offset from the file
         * @param filename - The filename itself
         */
        void AsyncAddNamedRegionJob(uintptr_t Base, uintptr_t Size, uintptr_t Offset, const fextl::string &filename) {
          AsyncHandler.AsyncAddNamedRegionJob(Base, Size, Offset, filename);
        }

        /**
         * @brief Unloads a named region from the code serialization service. As async as possible.
         *
         * @param Base - Virtual address of the named region
         * @param Size - The size of the region
         */
        void AsyncRemoveNamedRegionJob(uintptr_t Base, uintptr_t Size) {
          AsyncHandler.AsyncRemoveNamedRegionJob(Base, Size);
        }

        /**
         * @brief Adds a code object serialization job. As async as possible.
         * Code hashing happens prior to async job serialization to catch invalidations due to backpatching.
         *
         * @param Data - A fully filled out struct containing all the code serialization
         */
        void AsyncAddSerializationJob(fextl::unique_ptr<AsyncJobHandler::SerializationJobData> Data) {
          AsyncHandler.AsyncAddSerializationJob(std::move(Data));
        }
      /**  @} */

      /**
       * @name Synchronous interface
       * @{ */
        /**
         * @brief Synchronously waits for this thread's job queue to become empty.
         *
         * This is necessary for when a thread is shutting down
         *
         * @param ThreadJobRefCount - The shared mutex to wait on until to be empty
         */
        static void WaitForEmptyJobQueue(CodeSerializationMutex *ThreadJobRefCount) {
          // Once the shared mutex is empty this unique lock will be gained
          std::unique_lock lk {*ThreadJobRefCount};
        }

        /**
         * @brief Fetches object code from the Code Object Cache for JIT.
         *
         * @param GuestRIP - Which GuestRIP to search the cache for
         *
         * @return Data required for the JIT to relocate the Object code.
         */
        CodeObjectFileSection const *FetchCodeObjectFromCache(uint64_t GuestRIP);
      /**  @} */

      // Public for threading
      void ExecutionThread();

    protected:
      friend class AsyncJobHandler;

      /**
       * @brief Safely closes out code object regions from the map
       *
       * @param it - iterator to do a closure on
       */
      void DoCodeRegionClosure(uint64_t Base, CodeRegionEntry *it);

      CodeSerializationMutex &GetEntryMapMutex() { return EntryMapMutex; }
      CodeSerializationMutex &GetUnrelocatedEntryMapMutex() { return EntryMapMutex; }

      CodeRegionMapType &GetEntryMap() { return AddressToEntryMap; }
      CodeRegionPtrMapType &GetUnrelocatedEntryMap() { return UnrelocatedAddressToEntryMap; }

      /**
       * @brief Notify the async thread that it has work to do
       */
      void NotifyWork() { WorkAvailable.NotifyOne(); }

    private:
      FEXCore::Context::ContextImpl *CTX;

      Event WorkAvailable{};
      fextl::unique_ptr<FEXCore::Threads::Thread> WorkerThread;
      std::atomic_bool WorkerThreadShuttingDown {false};
      AsyncJobHandler AsyncHandler;
      NamedRegionObjectHandler NamedRegionHandler;

      // Mutex to hold when modifying the entry maps
      CodeSerializationMutex EntryMapMutex;
      CodeSerializationMutex UnrelocatedEntryMapMutex;

      // Entry maps
      CodeRegionMapType AddressToEntryMap;
      CodeRegionPtrMapType UnrelocatedAddressToEntryMap;
  };
}
