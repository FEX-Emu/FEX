/*
$info$
category: backend ~ IR to host code generation
tags: backend|shared
$end_info$
*/

#pragma once

#include <FEXCore/Utils/CompilerDefs.h>

#include <cstdint>
#include <string>
#include <memory>
#include <vector>

namespace FEXCore {

namespace IR {
  class IRListView;
  class RegisterAllocationData;
}

namespace Core {
  struct DebugData;
  struct ThreadState;
  struct CpuStateFrame;
  struct InternalThreadState;
}

struct ObjCacheFragment;
struct ObjCacheRelocations;


namespace CPU {
  struct CPUBackendFeatures {
    bool SupportsStaticRegisterAllocation = false;
  };
  
  class CPUBackend {
  public:
    struct CodeBuffer {
      uint8_t *Ptr;
      size_t Size;
    };

    /**
     * @param InitialCodeSize - Initial size for the code buffers
     * @param MaxCodeSize - Max size for the code buffers
    */
    CPUBackend(FEXCore::Core::InternalThreadState *ThreadState, size_t InitialCodeSize, size_t MaxCodeSize);

    virtual ~CPUBackend();
    /**
     * @return The name of this backend
     */
    [[nodiscard]] virtual std::string GetName() = 0;
    /**
     * @brief Tells this CPUBackend to compile code for the provided IR and DebugData
     *
     * The returned pointer needs to be long lived and be executable in the host environment
     * FEXCore's frontend will store this pointer in to a cache for the current RIP when this was executed
     *
     * This is a thread specific compilation unit since there is one CPUBackend per guest thread
     *
     * If NeedsOpDispatch is returning false then IR and DebugData may be null and the expectation is that the code will still compile
     * FEXCore::Core::ThreadState* is valid at the time of compilation.
     *
     * @param IR -  IR that maps to the IR for this RIP
     * @param DebugData - Debug data that is available for this IR indirectly
     *
     * @return An executable function pointer that is theoretically compiled from this point.
     * Is actually a function pointer of type `void (FEXCore::Core::ThreadState *Thread)
     */
    [[nodiscard]] virtual void *CompileCode(uint64_t Entry,
                                            const FEXCore::IR::IRListView *const IR,
                                            FEXCore::Core::DebugData *const DebugData,
                                            const FEXCore::IR::RegisterAllocationData *const RAData,
                                            bool GDBEnabled,
                                            bool DebugHelpersEnabled) = 0;

    /**
     * @brief Relocates a block of code from the JIT code object cache
     *
     * @param Entry - RIP of the entry
     * @param SerializationData - Serialization data referring to the object cache for `Entry`
     *
     * @return An executable function pointer relocated from the cache object
     */
    [[nodiscard]] virtual void *RelocateJITObjectCode(uint64_t Entry, const ObjCacheFragment *const HostCode, const ObjCacheRelocations *const Relocations) = 0;

    /**
     * @brief Function for mapping memory in to the CPUBackend's visible space. Allows setting up virtual mappings if required
     *
     * @return Currently unused
     */
    [[nodiscard]] virtual void *MapRegion(void *HostPtr, uint64_t GuestPtr, uint64_t Size) = 0;

    /**
     * @brief This is post-setup initialization that is called just before code executino
     *
     * Guest memory is available at this point and ThreadState is valid
     */
    virtual void Initialize() {}

    /**
     * @brief Lets FEXCore know if this CPUBackend needs IR and DebugData for CompileCode
     *
     * This is useful if the FEXCore Frontend hits an x86-64 instruction that isn't understood but can continue regardless
     *
     * This is useful for example, a VM based CPUbackend
     *
     * @return true if it needs the IR
     */
    [[nodiscard]] virtual bool NeedsOpDispatch() = 0;

    virtual void ClearCache() {}

    bool IsAddressInCodeBuffer(uintptr_t Address) const;

  protected:
    FEXCore::Core::InternalThreadState *ThreadState;

    size_t InitialCodeSize, MaxCodeSize;
    [[nodiscard]] CodeBuffer *GetEmptyCodeBuffer();

    // This is the current code buffer that we are tracking
    CodeBuffer *CurrentCodeBuffer{};

  private:
    CodeBuffer AllocateNewCodeBuffer(size_t Size);
    void FreeCodeBuffer(CodeBuffer Buffer);

    void EmplaceNewCodeBuffer(CodeBuffer Buffer) {
      CurrentCodeBuffer = &CodeBuffers.emplace_back(Buffer);
    }

    // This is the array of code buffers. Unless signals force us to keep more than
    // buffer, there will be only one entry here
    std::vector<CodeBuffer> CodeBuffers{};
  };

}
}
