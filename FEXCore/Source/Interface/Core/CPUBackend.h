// SPDX-License-Identifier: MIT
/*
$info$
category: backend ~ IR to host code generation
tags: backend|shared
$end_info$
*/

#pragma once

#include <FEXCore/Utils/CompilerDefs.h>
#include <FEXCore/Utils/SignalScopeGuards.h>
#include <FEXCore/fextl/memory.h>
#include <FEXCore/fextl/string.h>
#include <FEXCore/fextl/vector.h>

#include <cstdint>

namespace FEXCore {

namespace IR {
  class IRListView;
} // namespace IR

namespace Core {
  struct DebugData;
  struct ThreadState;
  struct CpuStateFrame;
  struct InternalThreadState;
} // namespace Core

namespace CodeSerialize {
  struct CodeObjectFileSection;
}

struct GuestToHostMap;

namespace CPU {
  struct CodeBuffer {
    uint8_t* Ptr;
    size_t Size;

    fextl::unique_ptr<GuestToHostMap> LookupCache;

    CodeBuffer(size_t Size);
    CodeBuffer(const CodeBuffer&) = delete;
    CodeBuffer& operator=(const CodeBuffer&) = delete;
    CodeBuffer(CodeBuffer&& oth) = delete;
    CodeBuffer& operator=(CodeBuffer&&) = delete;

    ~CodeBuffer();
  };

  /**
   * A manager that coordinates access to the CodeBuffer used for compiling new code across threads.
   *
   * The CodeBuffer is managed as a partially persistent data structure:
   * - Exactly one CodeBuffer is now designated as "active", which means data can be appended to it
   * - Lossy modifications to the active CodeBuffer will not invalidate any data in use by other threads (which is what enables save CodeBuffer sharing across threads)
   * - Instead, such lossy modifications trigger a new "version" of the data in the modifying thread. Old versions of the CodeBuffer persist as read-only data for use by the other threads.
   * - The other threads can update their version of the CodeBuffer. This will decrease the reference count and eventually trigger deallocation of the old version
   */
  class CodeBufferManager {
  public:
    // Get the CodeBuffer that was most recently allocated.
    // This is the only CodeBuffer that data may be written to.
    fextl::shared_ptr<CodeBuffer> GetLatest();

    // Allocate a new CodeBuffer with geometric growth up to an internal maximum.
    // Subsequent calls to GetLatest will point to the returned buffer.
    fextl::shared_ptr<CodeBuffer> StartLargerCodeBuffer();

    // Write offset into the latest CodeBuffer
    std::size_t LatestOffset {};

    // Protects writes to the latest CodeBuffer and changes to LatestOffset
    FEXCore::ForkableUniqueMutex CodeBufferWriteMutex;

    virtual void OnCodeBufferAllocated(CodeBuffer&) {};

  private:
    fextl::shared_ptr<CodeBuffer> Latest;

    fextl::shared_ptr<CodeBuffer> AllocateNew(size_t Size);
  };

  class CPUBackend {
  public:

    CPUBackend(CodeBufferManager&, FEXCore::Core::InternalThreadState*);

    virtual ~CPUBackend();

    struct CompiledCode {
      // Where this code block begins.
      uint8_t* BlockBegin;
      /**
       * The function entrypoint to this codeblock.
       *
       * This may or may not equal `BlockBegin` above. Depending on the CPU backend, it may stick data
       * prior to the BlockEntry.
       *
       * Is actually a function pointer of type `void (FEXCore::Core::ThreadState *Thread)`
       */
      uint8_t* BlockEntry;
      // The total size of the codeblock from [BlockBegin, BlockBegin+Size).
      size_t Size;
    };

    // Header that can live at the start of a JIT block.
    // We want the header to be quite small, with most data living in the tail object.
    struct JITCodeHeader {
      // Offset from the start of this header to where the tail lives.
      // Only 32-bit since the tail block won't ever be more than 4GB away.
      uint32_t OffsetToBlockTail;
    };

    // Header that can live at the end of the JIT block.
    // For any state reconstruction or other data, this is where it should live.
    // Any data that is explicitly tied to the JIT code and needs to be cached with it
    // should end up in this data structure.
    struct JITCodeTail {
      // The total size of the codeblock from [BlockBegin, BlockBegin+Size).
      size_t Size;

      // RIP that the block's entry comes from.
      uint64_t RIP;

      // The length of the guest code for this block.
      size_t GuestSize;

      // Number of RIP entries for this JIT Code section.
      uint32_t NumberOfRIPEntries;

      // Offset after this block to the start of the RIP entries.
      uint32_t OffsetToRIPEntries;

      // Shared-code modification spin-loop futex.
      uint32_t SpinLockFutex;

      // If this block represents a single guest instruction.
      bool SingleInst;

      uint8_t _Pad[3];
    };

    /**
     * @brief Tells this CPUBackend to compile code for the provided IR and DebugData
     *
     * The returned pointer needs to be long lived and be executable in the host environment
     * FEXCore's frontend will store this pointer in to a cache for the current RIP when this was executed
     *
     * This is a thread specific compilation unit since there is one CPUBackend per guest thread
     *
     * @param Size - The byte size of the guest code for this block
     * @param SingleInst - If this block represents a single guest instruction
     * @param IR -  IR that maps to the IR for this RIP
     * @param DebugData - Debug data that is available for this IR indirectly
     * @param CheckTF - If EFLAGS.TF checks should be emitted at the start of the block
     *
     * @return Information about the compiled code block.
     */
    [[nodiscard]]
    virtual CompiledCode CompileCode(uint64_t Entry, uint64_t Size, bool SingleInst, const FEXCore::IR::IRListView* IR,
                                     FEXCore::Core::DebugData* DebugData, bool CheckTF) = 0;

    /**
     * @brief Relocates a block of code from the JIT code object cache
     *
     * @param Entry - RIP of the entry
     * @param SerializationData - Serialization data referring to the object cache for `Entry`
     *
     * @return An executable function pointer relocated from the cache object
     */
    [[nodiscard]]
    virtual void* RelocateJITObjectCode(uint64_t /* Entry */, const CodeSerialize::CodeObjectFileSection* /* SerializationData */) {
      return nullptr;
    }

    virtual void ClearCache() {}

    /**
     * @brief Clear any relocations after JIT compiling
     */
    virtual void ClearRelocations() {}

    bool IsAddressInCodeBuffer(uintptr_t Address) const;

    // Updates the CodeBuffer if needed and returns a reference to the old one.
    // The returned reference should be kept alive carefully to avoid early deletion of resources.
    [[nodiscard]]
    fextl::shared_ptr<CodeBuffer> CheckCodeBufferUpdate();

  protected:
    // Max spill slot size in bytes. We need at most 32 bytes
    // to be able to handle a 256-bit vector store to a slot.
    constexpr static uint32_t MaxSpillSlotSize = 32;

    FEXCore::Core::InternalThreadState* ThreadState;

    size_t MaxCodeSize;
    [[nodiscard]]
    CodeBuffer* GetEmptyCodeBuffer();

    // This is the code buffer containing the main code under execution by this thread.
    // CheckCodeBufferUpdate must be used before compiling new code.
    fextl::shared_ptr<CodeBuffer> CurrentCodeBuffer;

    // Old CodeBuffer generations required to be valid until returning from signal handlers
    fextl::vector<fextl::shared_ptr<CodeBuffer>> SignalHandlerCodeBuffers;

    CodeBufferManager& CodeBuffers;

  private:
    void RegisterForSignalHandler(fextl::shared_ptr<CodeBuffer>);
  };

} // namespace CPU
} // namespace FEXCore
