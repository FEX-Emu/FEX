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

namespace FEXCore {

namespace IR {
  class IRListView;
  class RegisterAllocationData;
}

namespace Core {
  struct DebugData;
  struct ThreadState;
  struct CpuStateFrame;
}

namespace CPU {
class InterpreterCore;
class JITCore;
class LLVMCore;

  class CPUBackend {
  public:
    virtual ~CPUBackend() = default;
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
                                            FEXCore::IR::IRListView const *IR,
                                            FEXCore::Core::DebugData *DebugData,
                                            FEXCore::IR::RegisterAllocationData *RAData) = 0;


    [[nodiscard]] virtual void *RelocateJITCode(uint64_t Entry, const void *SerializationData) { return nullptr; }

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

    void ExecuteDispatch(FEXCore::Core::CpuStateFrame *Frame) {
      DispatchPtr(Frame);
    }

    virtual void ClearCache() {}
    virtual bool IsAddressInJITCode(uint64_t Address, bool IncludeDispatcher = true) const { return false; }

    /**
     * @brief Does this CPUBackend need its IR to stick around for correct emulation
     *
     * This should only be used on the interpreter, all other backends can clear their IR
     */
    virtual bool NeedsRetainedIRCopy() const { return false; }

    using AsmDispatch = FEX_NAKED void(*)(FEXCore::Core::CpuStateFrame *Frame);
    using JITCallback = FEX_NAKED void(*)(FEXCore::Core::CpuStateFrame *Frame, uint64_t RIP);

    JITCallback CallbackPtr{};
  protected:
    AsmDispatch DispatchPtr{};
  };

}
}
