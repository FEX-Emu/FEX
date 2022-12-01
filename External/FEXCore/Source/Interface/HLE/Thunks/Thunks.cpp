/*
$info$
meta: glue|thunks ~ FEXCore side of thunks: Registration, Lookup
tags: glue|thunks
$end_info$
*/

#include <FEXCore/Config/Config.h>
#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Debug/InternalThreadState.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/IR/IR.h>
#include <FEXCore/IR/IREmitter.h>
#include "FEXCore/Utils/CompilerDefs.h"
#include "Thunks.h"

#include <cstdint>
#include <dlfcn.h>

#include <Interface/Context/Context.h>
#include "FEXCore/Core/X86Enums.h"
#include <malloc.h>
#include <mutex>
#include <unordered_map>
#include <memory>
#include <shared_mutex>
#include <stdint.h>
#include <string>
#include <utility>

#ifdef ENABLE_JEMALLOC
#include "jemalloc/jemalloc.h"
#endif

struct LoadlibArgs {
    const char *Name;
};

static thread_local FEXCore::Core::InternalThreadState *Thread;

static __attribute__((aligned(16), naked, section("HostToGuestTrampolineTemplate"))) void HostToGuestTrampolineTemplate() {
#if defined(_M_X86_64)
  asm(
    "lea 0f(%rip), %r11 \n"
    "jmpq *0f(%rip) \n"
    ".align 8 \n"
    "0: \n"
    ".quad 0, 0, 0, 0 \n" // TrampolineInstanceInfo
  );
#elif defined(_M_ARM_64)
  asm(
    "adr x11, 0f \n"
    "ldr x16, [x11] \n"
    "br x16 \n"
    // Manually align to the next 8-byte boundary
    // NOTE: GCC over-aligns to a full page when using .align directives on ARM (last tested on GCC 11.2)
    "nop \n"
    "0: \n"
    ".quad 0, 0, 0, 0 \n" // TrampolineInstanceInfo
  );
#else
#error Unsupported host architecture
#endif
}

extern char __start_HostToGuestTrampolineTemplate[];
extern char __stop_HostToGuestTrampolineTemplate[];

namespace FEXCore {
    struct ExportEntry { uint8_t *sha256; ThunkedFunction* Fn; };

    struct TrampolineInstanceInfo {
      void* HostPacker;
      uintptr_t CallCallback;
      uintptr_t GuestUnpacker;
      uintptr_t GuestTarget;
    };

    // Opaque type pointing to an instance of HostToGuestTrampolineTemplate and its
    // embedded TrampolineInstanceInfo
    struct HostToGuestTrampolinePtr;
    const auto HostToGuestTrampolineSize = __stop_HostToGuestTrampolineTemplate - __start_HostToGuestTrampolineTemplate;

    static TrampolineInstanceInfo& GetInstanceInfo(HostToGuestTrampolinePtr* Trampoline) {
      const auto Length = __stop_HostToGuestTrampolineTemplate - __start_HostToGuestTrampolineTemplate;
      const auto InstanceInfoOffset = Length - sizeof(TrampolineInstanceInfo);
      return *reinterpret_cast<TrampolineInstanceInfo*>(reinterpret_cast<char*>(Trampoline) + InstanceInfoOffset);
    }

    struct GuestcallInfo {
      uintptr_t GuestUnpacker;
      uintptr_t GuestTarget;

      bool operator==(const GuestcallInfo&) const noexcept = default;
    };

    struct GuestcallInfoHash {
      size_t operator()(const GuestcallInfo& x) const noexcept {
        // Hash only the target address, which is generally unique.
        // For the unlikely case of a hash collision, std::unordered_map still picks the correct bucket entry.
        return std::hash<uintptr_t>{}(x.GuestTarget);
      }
    };

    // Bits in a SHA256 sum are already randomly distributed, so truncation yields a suitable hash function
    struct TruncatingSHA256Hash {
      size_t operator()(const FEXCore::IR::SHA256Sum& SHA256Sum) const noexcept {
        return (const size_t&)SHA256Sum;
      }
    };

    HostToGuestTrampolinePtr* MakeHostTrampolineForGuestFunction(void* HostPacker, uintptr_t GuestTarget, uintptr_t GuestUnpacker);

    struct ThunkHandler_impl final: public ThunkHandler {
        std::shared_mutex ThunksMutex;

        std::unordered_map<IR::SHA256Sum, ThunkedFunction*, TruncatingSHA256Hash> Thunks = {
            {
                // sha256(fex:loadlib)
                { 0x27, 0x7e, 0xb7, 0x69, 0x5b, 0xe9, 0xab, 0x12, 0x6e, 0xf7, 0x85, 0x9d, 0x4b, 0xc9, 0xa2, 0x44, 0x46, 0xcf, 0xbd, 0xb5, 0x87, 0x43, 0xef, 0x28, 0xa2, 0x65, 0xba, 0xfc, 0x89, 0x0f, 0x77, 0x80 },
                &LoadLib
            },
            {
                // sha256(fex:is_lib_loaded)
                { 0xee, 0x57, 0xba, 0x0c, 0x5f, 0x6e, 0xef, 0x2a, 0x8c, 0xb5, 0x19, 0x81, 0xc9, 0x23, 0xe6, 0x51, 0xae, 0x65, 0x02, 0x8f, 0x2b, 0x5d, 0x59, 0x90, 0x6a, 0x7e, 0xe2, 0xe7, 0x1c, 0x33, 0x8a, 0xff },
                &IsLibLoaded
            },
            {
                // sha256(fex:is_host_heap_allocation)
                { 0xf5, 0x77, 0x68, 0x43, 0xbb, 0x6b, 0x28, 0x18, 0x40, 0xb0, 0xdb, 0x8a, 0x66, 0xfb, 0x0e, 0x2d, 0x98, 0xc2, 0xad, 0xe2, 0x5a, 0x18, 0x5a, 0x37, 0x2e, 0x13, 0xc9, 0xe7, 0xb9, 0x8c, 0xa9, 0x3e },
                &IsHostHeapAllocation
            },
            {
                // sha256(fex:link_address_to_function)
                { 0xe6, 0xa8, 0xec, 0x1c, 0x7b, 0x74, 0x35, 0x27, 0xe9, 0x4f, 0x5b, 0x6e, 0x2d, 0xc9, 0xa0, 0x27, 0xd6, 0x1f, 0x2b, 0x87, 0x8f, 0x2d, 0x35, 0x50, 0xea, 0x16, 0xb8, 0xc4, 0x5e, 0x42, 0xfd, 0x77 },
                &LinkAddressToGuestFunction
            },
            {
                // sha256(fex:allocate_host_trampoline_for_guest_function)
                { 0x9b, 0xb2, 0xf4, 0xb4, 0x83, 0x7d, 0x28, 0x93, 0x40, 0xcb, 0xf4, 0x7a, 0x0b, 0x47, 0x85, 0x87, 0xf9, 0xbc, 0xb5, 0x27, 0xca, 0xa6, 0x93, 0xa5, 0xc0, 0x73, 0x27, 0x24, 0xae, 0xc8, 0xb8, 0x5a },
                &AllocateHostTrampolineForGuestFunction
            },
        };

        // Can't be a string_view. We need to keep a copy of the library name in-case string_view pointer goes away.
        // Ideally we track when a library has been unloaded and remove it from this set before the memory backing goes away.
        std::set<std::string> Libs;

        std::unordered_map<GuestcallInfo, HostToGuestTrampolinePtr*, GuestcallInfoHash> GuestcallToHostTrampoline;

        uint8_t *HostTrampolineInstanceDataPtr;
        size_t HostTrampolineInstanceDataAvailable = 0;


        /*
            Set arg0/1 to arg regs, use CTX::HandleCallback to handle the callback
        */
        static void CallCallback(void *callback, void *arg0, void* arg1) {
          Thread->CurrentFrame->State.gregs[FEXCore::X86State::REG_RDI] = (uintptr_t)arg0;
          Thread->CurrentFrame->State.gregs[FEXCore::X86State::REG_RSI] = (uintptr_t)arg1;

          Thread->CTX->HandleCallback(Thread, (uintptr_t)callback);
        }

        /**
         * Instructs the Core to redirect calls to functions at the given
         * address to another function. The original callee address is passed
         * to the target function through an implicit argument stored in r11.
         *
         * The primary use case of this is ensuring that host function pointers
         * returned from thunked APIs can safely be called by the guest.
         */
        static void LinkAddressToGuestFunction(void* argsv) {
            struct args_t {
                uintptr_t original_callee;
                uintptr_t target_addr;     // Guest function to call when branching to original_callee
            };

            auto args = reinterpret_cast<args_t*>(argsv);
            auto CTX = Thread->CTX;

            LOGMAN_THROW_AA_FMT(args->original_callee, "Tried to link null pointer address to guest function");
            LOGMAN_THROW_AA_FMT(args->target_addr, "Tried to link address to null pointer guest function");
            if (!CTX->Config.Is64BitMode) {
                LOGMAN_THROW_AA_FMT((args->original_callee >> 32) == 0, "Tried to link 64-bit address in 32-bit mode");
                LOGMAN_THROW_AA_FMT((args->target_addr >> 32) == 0, "Tried to link 64-bit address in 32-bit mode");
            }

            LogMan::Msg::DFmt("Thunks: Adding guest trampoline from address {:#x} to guest function {:#x}",
                              args->original_callee, args->target_addr);

            auto Result = Thread->CTX->AddCustomIREntrypoint(
                    args->original_callee,
                    [CTX, GuestThunkEntrypoint = args->target_addr](uintptr_t Entrypoint, FEXCore::IR::IREmitter *emit) {
                        auto IRHeader = emit->_IRHeader(emit->Invalid(), 0);
                        auto Block = emit->CreateCodeNode();
                        IRHeader.first->Blocks = emit->WrapNode(Block);
                        emit->SetCurrentCodeBlock(Block);

                        const uint8_t GPRSize = CTX->GetGPRSize();

                        emit->_StoreRegister(emit->_Constant(Entrypoint), false, offsetof(Core::CPUState, gregs[X86State::REG_R11]), IR::GPRClass, IR::GPRFixedClass, GPRSize);
                        emit->_ExitFunction(emit->_Constant(GuestThunkEntrypoint));
                    }, CTX->ThunkHandler.get(), (void*)args->target_addr);

            if (!Result) {
                if (Result.Creator != CTX->ThunkHandler.get()) {
                    ERROR_AND_DIE_FMT("Input address for LinkAddressToGuestFunction is already linked by another module");
                }
                if (Result.Data != (void*)args->target_addr) {
                    // NOTE: This may happen in Vulkan thunks if the Vulkan driver resolves two different symbols
                    //       to the same function (e.g. vkGetPhysicalDeviceFeatures2/vkGetPhysicalDeviceFeatures2KHR)
                    LogMan::Msg::EFmt("Input address for LinkAddressToGuestFunction is already linked elsewhere");
                }
            }
        }

        /**
         * Guest-side helper to initiate creation of a host trampoline for
         * calling guest functions. This must be followed by a host-side call
         * to FinalizeHostTrampolineForGuestFunction to make the trampoline
         * usable.
         *
         * This two-step initialization is equivalent to a host-side call to
         * MakeHostTrampolineForGuestFunction. The split is needed if the
         * host doesn't have all information needed to create the trampoline
         * on its own.
         */
        static void AllocateHostTrampolineForGuestFunction(void* ArgsRV) {
            struct ArgsRV_t {
                uintptr_t GuestUnpacker;
                uintptr_t GuestTarget;
                uintptr_t rv; // Pointer to host trampoline + TrampolineInstanceInfo
            } *args = reinterpret_cast<ArgsRV_t*>(ArgsRV);

            args->rv = (uintptr_t)MakeHostTrampolineForGuestFunction(nullptr, args->GuestTarget, args->GuestUnpacker);
        }

        /**
         * Checks if the given pointer is allocated on the host heap.
         *
         * This is useful for thunking APIs that need to work with both guest
         * and host heap pointers.
         */
        static void IsHostHeapAllocation(void* ArgsRV) {
#ifdef ENABLE_JEMALLOC
            struct ArgsRV_t {
                void* ptr;
                bool rv;
            } *args = reinterpret_cast<ArgsRV_t*>(ArgsRV);

            args->rv = je_is_known_allocation(args->ptr);
#else
            // Thunks usage without jemalloc isn't supported
            ERROR_AND_DIE_FMT("Unsupported: Thunks querying for host heap allocation information");
#endif
        }

        static void LoadLib(void *ArgsV) {
            auto CTX = Thread->CTX;

            auto Args = reinterpret_cast<LoadlibArgs*>(ArgsV);

            auto Name = Args->Name;

            auto SOName = CTX->Config.ThunkHostLibsPath() + "/" + (const char*)Name + "-host.so";

            LogMan::Msg::DFmt("LoadLib: {} -> {}", Name, SOName);

            auto Handle = dlopen(SOName.c_str(), RTLD_LOCAL | RTLD_NOW);
            if (!Handle) {
                ERROR_AND_DIE_FMT("LoadLib: Failed to dlopen thunk library {}: {}", SOName, dlerror());
            }

            const auto InitSym = std::string("fexthunks_exports_") + Name;

            ExportEntry* (*InitFN)();
            (void*&)InitFN = dlsym(Handle, InitSym.c_str());
            if (!InitFN) {
                ERROR_AND_DIE_FMT("LoadLib: Failed to find export {}", InitSym);
            }

            auto Exports = InitFN();
            if (!Exports) {
                ERROR_AND_DIE_FMT("LoadLib: Failed to initialize thunk library {}. "
                                  "Check if the corresponding host library is installed "
                                  "or disable thunking of this library.", Name);
            }

            auto That = reinterpret_cast<ThunkHandler_impl*>(CTX->ThunkHandler.get());

            {
                std::lock_guard lk(That->ThunksMutex);

                That->Libs.insert(Name);

                int i;
                for (i = 0; Exports[i].sha256; i++) {
                    That->Thunks[*reinterpret_cast<IR::SHA256Sum*>(Exports[i].sha256)] = Exports[i].Fn;
                }

                LogMan::Msg::DFmt("Loaded {} syms", i);
            }
        }

        static void IsLibLoaded(void* ArgsRV) {
            struct ArgsRV_t {
                const char *Name;
                bool rv;
            };

            auto &[Name, rv] = *reinterpret_cast<ArgsRV_t*>(ArgsRV);

            auto CTX = Thread->CTX;
            auto That = reinterpret_cast<ThunkHandler_impl*>(CTX->ThunkHandler.get());

            {
                std::shared_lock lk(That->ThunksMutex);
                rv = That->Libs.contains(Name);
            }
        }

        ThunkedFunction* LookupThunk(const IR::SHA256Sum &sha256) override {

            std::shared_lock lk(ThunksMutex);

            auto it = Thunks.find(sha256);

            if (it != Thunks.end()) {
                return it->second;
            } else {
                return nullptr;
            }
        }

        void RegisterTLSState(FEXCore::Core::InternalThreadState *Thread) override {
            ::Thread = Thread;
        }

        void AppendThunkDefinitions(std::vector<FEXCore::IR::ThunkDefinition> const& Definitions) override {
          for (auto & Definition : Definitions) {
            Thunks.emplace(Definition.Sum, Definition.ThunkFunction);
          }
        }
    };

    ThunkHandler* ThunkHandler::Create() {
      return new ThunkHandler_impl();
    }

    /**
     * Generates a host-callable trampoline to call guest functions via the host ABI.
     *
     * This trampoline uses the same calling convention as the given HostPacker. Trampolines
     * are cached, so it's safe to call this function repeatedly on the same arguments without
     * leaking memory.
     *
     * Invoking the returned trampoline has the effect of:
     * - packing the arguments (using the HostPacker identified by its SHA256)
     * - performing a host->guest transition
     * - unpacking the arguments via GuestUnpacker
     * - calling the function at GuestTarget
     *
     * The primary use case of this is ensuring that guest function pointers ("callbacks")
     * passed to thunked APIs can safely be called by the native host library.
     *
     * Returns a pointer to the generated host trampoline and its TrampolineInstanceInfo.
     *
     * If HostPacker is zero, the trampoline will be partially initialized and needs to be
     * finalized with a call to FinalizeHostTrampolineForGuestFunction. A typical use case
     * is to allocate the trampoline for a given GuestTarget/GuestUnpacker on the guest-side,
     * and provide the HostPacker host-side.
     */
    FEX_DEFAULT_VISIBILITY
    HostToGuestTrampolinePtr* MakeHostTrampolineForGuestFunction(void* HostPacker, uintptr_t GuestTarget, uintptr_t GuestUnpacker) {
      LOGMAN_THROW_AA_FMT(GuestTarget, "Tried to create host-trampoline to null pointer guest function");

      const auto CTX = Thread->CTX;
      const auto ThunkHandler = reinterpret_cast<ThunkHandler_impl *>(CTX->ThunkHandler.get());

      const GuestcallInfo gci = { GuestUnpacker, GuestTarget };

      // Try first with shared_lock
      {
        std::shared_lock lk(ThunkHandler->ThunksMutex);

        auto found = ThunkHandler->GuestcallToHostTrampoline.find(gci);
        if (found != ThunkHandler->GuestcallToHostTrampoline.end()) {
          return found->second;
        }
      }

      std::lock_guard lk(ThunkHandler->ThunksMutex);

      // Retry lookup with full lock before making a new trampoline to avoid double trampolines
      {
        auto found = ThunkHandler->GuestcallToHostTrampoline.find(gci);
        if (found != ThunkHandler->GuestcallToHostTrampoline.end()) {
          return found->second;
        }
      }

      LogMan::Msg::DFmt("Thunks: Adding host trampoline for guest function {:#x} via unpacker {:#x}",
                        GuestTarget, GuestUnpacker);

      if (ThunkHandler->HostTrampolineInstanceDataAvailable < HostToGuestTrampolineSize) {
        const auto allocation_step = 16 * 1024;
        ThunkHandler->HostTrampolineInstanceDataAvailable = allocation_step;
        ThunkHandler->HostTrampolineInstanceDataPtr = (uint8_t *)mmap(
            0, ThunkHandler->HostTrampolineInstanceDataAvailable,
            PROT_READ | PROT_WRITE | PROT_EXEC,
            MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

        LOGMAN_THROW_AA_FMT(ThunkHandler->HostTrampolineInstanceDataPtr != MAP_FAILED, "Failed to mmap HostTrampolineInstanceDataPtr");
      }

      auto HostTrampoline = reinterpret_cast<HostToGuestTrampolinePtr* const>(ThunkHandler->HostTrampolineInstanceDataPtr);
      ThunkHandler->HostTrampolineInstanceDataAvailable -= HostToGuestTrampolineSize;
      ThunkHandler->HostTrampolineInstanceDataPtr += HostToGuestTrampolineSize;
      memcpy(HostTrampoline, (void*)&HostToGuestTrampolineTemplate, HostToGuestTrampolineSize);
      GetInstanceInfo(HostTrampoline) = TrampolineInstanceInfo {
          .HostPacker = HostPacker,
          .CallCallback = (uintptr_t)&ThunkHandler_impl::CallCallback,
          .GuestUnpacker = GuestUnpacker,
          .GuestTarget = GuestTarget
      };

      ThunkHandler->GuestcallToHostTrampoline[gci] = HostTrampoline;
      return HostTrampoline;
    }

    FEX_DEFAULT_VISIBILITY
    void FinalizeHostTrampolineForGuestFunction(HostToGuestTrampolinePtr* TrampolineAddress, void* HostPacker) {

      if (TrampolineAddress == nullptr) return;

      auto& Trampoline = GetInstanceInfo(TrampolineAddress);

      LOGMAN_THROW_A_FMT(Trampoline.CallCallback == (uintptr_t)&ThunkHandler_impl::CallCallback,
                        "Invalid trampoline at {} passed to {}", fmt::ptr(TrampolineAddress), __FUNCTION__);

      if (!Trampoline.HostPacker) {
        LogMan::Msg::DFmt("Thunks: Finalizing trampoline at {} with host packer {}", fmt::ptr(TrampolineAddress), fmt::ptr(HostPacker));
        Trampoline.HostPacker = HostPacker;
      }
    }
}
