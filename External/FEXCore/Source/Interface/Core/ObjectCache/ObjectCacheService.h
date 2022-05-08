#pragma once
#include "Interface/Context/Context.h"
#include "Interface/Core/ObjectCache/Relocations.h"

#include <FEXCore/Utils/Event.h>
#include <FEXCore/Utils/Threads.h>

#include <map>
#include <memory>
#include <shared_mutex>
#include <string>
#include <vector>

namespace FEXCore::CodeSerialize {
  struct CodeRegionEntry {
    // XXX: File out code region entry
  };

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

  // Map type must use an interator that isn't invalidation on erase/insert
  using CodeRegionMapType = std::map<uint64_t, std::unique_ptr<CodeRegionEntry>>;
  using CodeRegionPtrMapType = std::map<uint64_t, CodeRegionEntry*>;

  // XXX: Does this need to be signal safe?
  using CodeSerializationMutex = std::shared_mutex;

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
        std::vector<FEXCore::CPU::Relocation> Relocations;

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

    protected:
      friend class CodeObjectSerializeService;
      /**
       * @name Async job submission functions
       * @{ */
        void AsyncAddNamedRegionJob(uintptr_t Base, uintptr_t Size, uintptr_t Offset, const std::string &filename);
        void AsyncRemoveNamedRegionJob(uintptr_t Base, uintptr_t Size);
        void AsyncAddSerializationJob(std::unique_ptr<SerializationJobData> Data);
      /**  @} */

    private:
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
            WorkItemAddNamedRegion(const std::string &base, const std::string &filename, bool executable, CodeRegionMapType::iterator entry)
              : NamedRegionWorkItem {NamedRegionJobType::JOB_ADD_NAMED_REGION}
              , BaseFilename {base}
              , Filename {filename}
              , Executable {executable}
              , Entry {entry}
              {}
            const std::string BaseFilename;
            const std::string Filename;
            bool Executable;
            CodeRegionMapType::iterator Entry;
        };

        class WorkItemRemoveNamedRegion : public NamedRegionWorkItem {
          public:
            WorkItemRemoveNamedRegion(uint64_t base, uint64_t size, CodeRegionMapType::iterator entry)
              : NamedRegionWorkItem {NamedRegionJobType::JOB_REMOVE_NAMED_REGION}
              , Base {base}
              , Size {size}
              , Entry {entry} {}

            uint64_t Base;
            uint64_t Size;
            CodeRegionMapType::iterator Entry;
        };
      /**  @} */
  };

  /**
   * @brief Context specific code object serialization class
   *
   * Contains everything required for FEXCore to serialize code objects
   */
  class CodeObjectSerializeService final {
    public:
      CodeObjectSerializeService(FEXCore::Context::Context *ctx);

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
        void AsyncAddNamedRegionJob(uintptr_t Base, uintptr_t Size, uintptr_t Offset, const std::string &filename) {
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
        void AsyncAddSerializationJob(std::unique_ptr<AsyncJobHandler::SerializationJobData> Data) {
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
      /**
       * @brief Safely closes out code object regions from the map
       *
       * @param it - iterator to do a closure on
       */
      void DoCodeRegionClosure(uint64_t Base, CodeRegionEntry *it);

    private:
      FEXCore::Context::Context *CTX;

      Event WorkAvailable{};
      std::unique_ptr<FEXCore::Threads::Thread> WorkerThread;
      std::atomic_bool WorkerThreadShuttingDown {false};
      AsyncJobHandler AsyncHandler{};

      CodeRegionMapType AddressToEntryMap;
      CodeRegionPtrMapType UnrelocatedAddressToEntryMap;
  };
}
