// SPDX-License-Identifier: MIT
/*
$info$
category: backend ~ IR to host code generation
tags: backend|shared
$end_info$
*/

#pragma once

#include <FEXCore/Utils/CompilerDefs.h>
#include <FEXCore/fextl/string.h>
#include <FEXCore/fextl/vector.h>

#include <cstdint>
#include <memory>

namespace FEXCore {

namespace IR {
  class IRListView;
  class RegisterAllocationData;
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

namespace CPU {
  class CPUBackend {
  public:
    struct CodeBuffer {
      uint8_t* Ptr;
      size_t Size;
    };

    /**
     * @param InitialCodeSize - Initial size for the code buffers
     * @param MaxCodeSize - Max size for the code buffers
     */
    CPUBackend(FEXCore::Core::InternalThreadState* ThreadState, size_t InitialCodeSize, size_t MaxCodeSize);

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

      // If this block represents a single guest instruction.
      bool SingleInst;

      // Number of RIP entries for this JIT Code section.
      uint32_t NumberOfRIPEntries;

      // Offset after this block to the start of the RIP entries.
      uint32_t OffsetToRIPEntries;

      // Shared-code modification spin-loop futex.
      uint32_t SpinLockFutex;

      uint32_t _Pad;
    };

    // Entries that live after the JITCodeTail.
    // These entries correlate JIT code regions with guest RIP regions.
    // Using these entries FEX is able to reconstruct the guest RIP accurately when an instruction cause a signal fault.
    // Packed using 16-bit entries to ensure the size isn't too large.
    // These smaller sizes means that each entry is relative to each other instead of absolute offset from the start of the JIT block.
    // When reconstructing the RIP, each entry must be walked linearly and accumulated with the previous entries.
    // This is a trade-off between compression inside the JIT code space and execution time when reconstruction the RIP.
    // RIP reconstruction when faulting is less likely so we are requiring the accumulation.
    struct JITRIPReconstructEntries {
      // The Host PC offset from the previous entry.
      uint16_t HostPCOffset;

      // How much to offset the RIP from the previous entry.
      int16_t GuestRIPOffset;
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
                                     FEXCore::Core::DebugData* DebugData, const FEXCore::IR::RegisterAllocationData* RAData, bool CheckTF) = 0;

    /**
     * @brief Relocates a block of code from the JIT code object cache
     *
     * @param Entry - RIP of the entry
     * @param SerializationData - Serialization data referring to the object cache for `Entry`
     *
     * @return An executable function pointer relocated from the cache object
     */
    [[nodiscard]]
    virtual void* RelocateJITObjectCode(uint64_t Entry, const CodeSerialize::CodeObjectFileSection* SerializationData) {
      return nullptr;
    }

    virtual void ClearCache() {}

    /**
     * @brief Clear any relocations after JIT compiling
     */
    virtual void ClearRelocations() {}

    bool IsAddressInCodeBuffer(uintptr_t Address) const;

  protected:
    // Max spill slot size in bytes. We need at most 32 bytes
    // to be able to handle a 256-bit vector store to a slot.
    constexpr static uint32_t MaxSpillSlotSize = 32;

    FEXCore::Core::InternalThreadState* ThreadState;

    size_t InitialCodeSize, MaxCodeSize;
    [[nodiscard]]
    CodeBuffer* GetEmptyCodeBuffer();

    // This is the current code buffer that we are tracking
    CodeBuffer* CurrentCodeBuffer {};

  private:
    CodeBuffer AllocateNewCodeBuffer(size_t Size);
    void FreeCodeBuffer(CodeBuffer Buffer);

    void EmplaceNewCodeBuffer(CodeBuffer Buffer) {
      CurrentCodeBuffer = &CodeBuffers.emplace_back(Buffer);
    }

    // This is the array of code buffers. Unless signals force us to keep more than
    // buffer, there will be only one entry here
    fextl::vector<CodeBuffer> CodeBuffers {};
  };

} // namespace CPU
} // namespace FEXCore
