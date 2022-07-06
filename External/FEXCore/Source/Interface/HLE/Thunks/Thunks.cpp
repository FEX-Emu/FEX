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
#include <unordered_set>
#include <memory>
#include <shared_mutex>
#include <stdint.h>
#include <string>
#include <utility>


#define HOST_TRAMPOLINE_ALLOC_STEP (16 * 1024)

struct LoadlibArgs {
    const char *Name;
};

static thread_local FEXCore::Core::InternalThreadState *Thread;

struct TrampolineInstanceInfo {
  uintptr_t HostPacker;
  uintptr_t CallCallback;
  uintptr_t GuestUnpacker;
  uintptr_t GuestTarget;
};

struct GuestcallInfo {
  uintptr_t GuestUnpacker;
  uintptr_t GuestTarget;

  [[nodiscard]] friend constexpr bool operator==(const GuestcallInfo&, const GuestcallInfo&) = default;
};

template <>
struct std::hash<GuestcallInfo> {
  size_t operator()(const GuestcallInfo& x) const noexcept {
    return x.GuestTarget;
  }
};


static __attribute__((aligned(16), naked, section("HostToGuestTrampolineTemplate"))) void HostToGuestTrampolineTemplate() {
#if defined(_M_X86_64)
  asm(
    "lea 0f(%rip), %r11 \n"
    "jmpq *0f(%rip) \n"
    ".align 16 \n"
    "0: \n"
    ".quad 0, 0, 0, 0 \n"
  );
#elif defined(_M_ARM_64)
  asm(
    "adr x11, 0f \n"
    "ldr x16, [x11] \n"
    "br x16 \n"
    ".align 16 \n"
    "0: \n" // CallCallback
    ".quad 0, 0, 0, 0 \n"
  );
#else
#error Unsupported host architecture
#endif
}

extern char __start_HostToGuestTrampolineTemplate[];
extern char __stop_HostToGuestTrampolineTemplate[];

namespace FEXCore {
    struct ExportEntry { uint8_t *sha256; ThunkedFunction* Fn; };

    class ThunkHandler_impl final: public ThunkHandler {
        std::shared_mutex ThunksMutex;

        std::map<IR::SHA256Sum, ThunkedFunction*> Thunks = {
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
                // sha256(fex:link_address_to_function)
                { 0xe6, 0xa8, 0xec, 0x1c, 0x7b, 0x74, 0x35, 0x27, 0xe9, 0x4f, 0x5b, 0x6e, 0x2d, 0xc9, 0xa0, 0x27, 0xd6, 0x1f, 0x2b, 0x87, 0x8f, 0x2d, 0x35, 0x50, 0xea, 0x16, 0xb8, 0xc4, 0x5e, 0x42, 0xfd, 0x77 },
                &LinkAddressToGuestFunction
            },
            {
                // sha256(fex:host_trampoline_for_guestcall)
                { 0xa2, 0xa1, 0x95, 0x64, 0xad, 0x6e, 0xa5, 0x32, 0xc5, 0xb2, 0xcb, 0x5b, 0x5d, 0x85, 0xec, 0x99, 0x46, 0x9d, 0x5a, 0xf4, 0xa5, 0x2f, 0xbe, 0xa3, 0x7b, 0x7d, 0xd1, 0x8e, 0x44, 0xa7, 0x81, 0xe8 },
                &HostTrampolineForGuestcall
            }
        };

        // Can't be a string_view. We need to keep a copy of the library name in-case string_view pointer goes away.
        // Ideally we track when a library has been unloaded and remove it from this set before the memory backing goes away.
        std::set<std::string> Libs;

        std::unordered_map<GuestcallInfo, uintptr_t> GuestcallToHostTrampoline;

        uint8_t *HostTrampolineInstanceDataPtr;
        size_t HostTrampolineInstanceDataAvailable;
          

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

            LOGMAN_THROW_A_FMT(args->original_callee, "Tried to link null pointer address to guest function");
            LOGMAN_THROW_A_FMT(args->target_addr, "Tried to link address to null pointer guest function");
            if (!CTX->Config.Is64BitMode) {
                LOGMAN_THROW_A_FMT((args->original_callee >> 32) == 0, "Tried to link 64-bit address in 32-bit mode");
                LOGMAN_THROW_A_FMT((args->target_addr >> 32) == 0, "Tried to link 64-bit address in 32-bit mode");
            }

            LogMan::Msg::DFmt("Thunks: Adding trampoline from address {:#x} to guest function {:#x}",
                              args->original_callee, args->target_addr);

            auto Result = Thread->CTX->AddCustomIREntrypoint(
                    args->original_callee,
                    [CTX, GuestThunkEntrypoint = args->target_addr](uintptr_t Entrypoint, FEXCore::IR::IREmitter *emit) {
                        auto IRHeader = emit->_IRHeader(emit->Invalid(), 0);
                        auto Block = emit->CreateCodeNode();
                        IRHeader.first->Blocks = emit->WrapNode(Block);
                        emit->SetCurrentCodeBlock(Block);

                        const uint8_t GPRSize = CTX->GetGPRSize();

                        emit->_StoreContext(GPRSize, IR::GPRClass, emit->_Constant(Entrypoint), offsetof(Core::CPUState, gregs[X86State::REG_R11]));
                        emit->_ExitFunction(emit->_Constant(GuestThunkEntrypoint));
                    }, CTX->ThunkHandler.get(), (void*)args->target_addr);

            if (!Result) {
                if (Result.Creator != CTX->ThunkHandler.get() || Result.Data != (void*)args->target_addr) {
                    ERROR_AND_DIE_FMT("Input address for LinkAddressToGuestFunction is already linked elsewhere");
                }
            }
        }

        static void HostTrampolineForGuestcall(void* ArgsRV) {
          struct ArgsRV_t {
              IR::SHA256Sum *HostPacker;
              uintptr_t GuestUnpacker;
              uintptr_t GuestTarget;
              uintptr_t rv;
          };

          auto &[HostPacker, GuestTarget, GuestUnpacker, rv] = *reinterpret_cast<ArgsRV_t*>(ArgsRV);

          auto const CTX = Thread->CTX;
          auto const That = reinterpret_cast<ThunkHandler_impl *>(CTX->ThunkHandler.get());

          const GuestcallInfo gci = { GuestUnpacker, GuestTarget };

          // Try first with shared_lock
          {
            std::shared_lock lk(That->ThunksMutex);

            auto found = That->GuestcallToHostTrampoline.find(gci);
            if (found != That->GuestcallToHostTrampoline.end()) {
              rv = found->second;
              return;
            }
          }

          std::lock_guard lk(That->ThunksMutex);

          // retry lookup with full lock before making a new trampoline to avoid double trampolines
          {
            auto found = That->GuestcallToHostTrampoline.find(gci);
            if (found != That->GuestcallToHostTrampoline.end()) {
              rv = found->second;
              return;
            }
          }

          auto HostPackerEntry = That->Thunks.find(*HostPacker);
          if (HostPackerEntry == That->Thunks.end()) {
            rv = 0;
            return;
          }

          const auto Length = __stop_HostToGuestTrampolineTemplate - __start_HostToGuestTrampolineTemplate;
          const auto InstanceInfoOffset = Length - sizeof(TrampolineInstanceInfo);

          uint8_t *HostTrampoline;

          // Still protected by `lk`
          if (That->HostTrampolineInstanceDataAvailable < Length) {
            That->HostTrampolineInstanceDataAvailable = HOST_TRAMPOLINE_ALLOC_STEP;
            That->HostTrampolineInstanceDataPtr = (uint8_t *)mmap(
                0, That->HostTrampolineInstanceDataAvailable, PROT_READ | PROT_WRITE | PROT_EXEC,
                MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

            LOGMAN_THROW_A_FMT(That->HostTrampolineInstanceDataPtr != MAP_FAILED, "Failed to mmap HostTrampolineInstanceDataPtr");
          }

          // Still protected by `lk`
          HostTrampoline = That->HostTrampolineInstanceDataPtr;
          That->HostTrampolineInstanceDataAvailable -= Length;
          That->HostTrampolineInstanceDataPtr += Length;

          memcpy(HostTrampoline, (void*)&HostToGuestTrampolineTemplate, Length);

          auto const InstanceInfo = (TrampolineInstanceInfo*)(HostTrampoline + InstanceInfoOffset);

          InstanceInfo->HostPacker = (uintptr_t)HostPackerEntry->second;
          InstanceInfo->CallCallback = (uintptr_t)&CallCallback;
          InstanceInfo->GuestUnpacker = GuestUnpacker;
          InstanceInfo->GuestTarget = GuestTarget;

          rv = (uintptr_t)HostTrampoline;

          // Still protected by `lk`
          That->GuestcallToHostTrampoline[gci] = rv;
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

        public:

        ThunkedFunction* LookupThunk(const IR::SHA256Sum &sha256) {

            std::shared_lock lk(ThunksMutex);

            auto it = Thunks.find(sha256);

            if (it != Thunks.end()) {
                return it->second;
            } else {
                return nullptr;
            }
        }

        void RegisterTLSState(FEXCore::Core::InternalThreadState *Thread) {
            ::Thread = Thread;
        }

        ThunkHandler_impl() {
        }

        ~ThunkHandler_impl() {
        }
    };

    ThunkHandler* ThunkHandler::Create() {
        return new ThunkHandler_impl();
    }
}
