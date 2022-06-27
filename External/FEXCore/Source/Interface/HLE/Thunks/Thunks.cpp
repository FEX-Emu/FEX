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
    uintptr_t CallbackThunks;
};

static thread_local FEXCore::Core::InternalThreadState *Thread;

struct TrampolineInstanceInfo {
  uintptr_t CallCallback;
  uintptr_t GuestUnpacker;
  uintptr_t GuestTarget;
};

static __attribute__((aligned(16), naked, section("HostToGuestTrampolineTemplate"))) void HostToGuestTrampolineTemplate() {
#if defined(_M_X86_64)
  asm(
    "mov r11, 0f(%RIP) \n"
    "mov rdi, 1f(%RIP) \n"
    "mov rdx, 2f(%RIP) \n"
    "jmp r11 \n"
    ".align 8 \n"
    "0: \n" // CallCallback
    ".q 0 \n"
    "1: \n" // GuestTarget
    ".q 0 \n"
    "2:"
    ".q 0" // GuestUnpacker
  );
#elif defined(_M_ARM64)
  asm(
    "adr r12, 0f \n"
    "ldr r1, 8(r12) \n"
    "mov rdx, 16(%RIP) \n"
    "jmp r11 \n"
    ".align 8 \n"
    "0: \n" // CallCallback
    ".q 0 \n"
    "1: \n" // GuestTarget
    ".q 0 \n"
    "2:"
    ".q 0" // GuestUnpacker
  );
#else
#error Unsupported host architecture
#endif
}

namespace FEXCore {
    struct ExportEntry { uint8_t *sha256; ThunkedFunction* Fn; };

    class ThunkHandler_impl final: public ThunkHandler {
        std::shared_mutex ThunksMutex;

        std::unordered_map<IR::SHA256Sum, ThunkedFunction*> Thunks = {
            {
                // sha256(fex:load_lib)
                { 0xd1, 0x31, 0x17, 0x56, 0xe3, 0x85, 0x25, 0x2a, 0xae, 0x29, 0x5e, 0x3f, 0xc2, 0xe5, 0x15, 0xcb, 0x12, 0x0e, 0x48, 0xda, 0x01, 0x51, 0xf9, 0xfb, 0x81, 0x44, 0x8e, 0x1c, 0x62, 0xa7, 0xb5, 0x69 },
                &LoadLib
            },
            {
                // sha256(fex:is_lib_loaded)
                { 0xe6, 0xa8, 0xec, 0x1c, 0x7b, 0x74, 0x35, 0x27, 0xe9, 0x4f, 0x5b, 0x6e, 0x2d, 0xc9, 0xa0, 0x27, 0xd6, 0x1f, 0x2b, 0x87, 0x8f, 0x2d, 0x35, 0x50, 0xea, 0x16, 0xb8, 0xc4, 0x5e, 0x42, 0xfd, 0x77 },
                &IsLibLoaded
            },
            {
                // sha256(fex:link_host_address_to_guest_function)
                { 0xe6, 0xa8, 0xec, 0x1c, 0x7b, 0x74, 0x35, 0x27, 0xe9, 0x4f, 0x5b, 0x6e, 0x2d, 0xc9, 0xa0, 0x27, 0xd6, 0x1f, 0x2b, 0x87, 0x8f, 0x2d, 0x35, 0x50, 0xea, 0x16, 0xb8, 0xc4, 0x5e, 0x42, 0xfd, 0x77 },
                &LinkAddressToGuestFunction
            },
            {
                // sha256(fex:host_trampoline_for_guestcall)
                { 0xa2, 0xa1, 0x95, 0x64, 0xad, 0x6e, 0xa5, 0x32, 0xc5, 0xb2, 0xcb, 0x5b, 0x5d, 0x85, 0xec, 0x99, 0x46, 0x9d, 0x5a, 0xf4, 0xa5, 0x2f, 0xbe, 0xa3, 0x7b, 0x7d, 0xd1, 0x8e, 0x44, 0xa7, 0x81, 0xe8 },
                &HostTrampolineForGuestcall
            },
            {
                // sha256(fex:guestcall_target_for_host_trampoline)
                { 0x04, 0x15, 0x13, 0xb2, 0x29, 0x32, 0xc8, 0xd8, 0x1a, 0xc1, 0xa7, 0xba, 0x09, 0xaa, 0x9e, 0xb4, 0x91, 0x0c, 0x68, 0x66, 0x62, 0xac, 0xa8, 0x80, 0x6e, 0xbb, 0xbe, 0xda, 0x15, 0xed, 0x55, 0x4a },
                &GuestcallTargetForHostTrampoline
            }
        };

        std::unordered_set<std::string_view> Libs;

        std::unordered_map<std::pair<uintptr_t, uintptr_t>, uintptr_t> GuestcallToHostTrampoline;
        std::unordered_map<uintptr_t, std::pair<uintptr_t, uintptr_t>> HostTrampolineToGuestcall;

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

        static void LoadLib(void *ArgsV) {
            auto CTX = Thread->CTX;

            auto Args = reinterpret_cast<LoadlibArgs*>(ArgsV);

            auto Name = Args->Name;
            auto CallbackThunks = Args->CallbackThunks;

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
                bool rv;     // Guest function to call when branching to original_callee
            };

            auto &[Name, rv] = *reinterpret_cast<ArgsRV_t*>(ArgsRV);

            auto CTX = Thread->CTX;
            auto That = reinterpret_cast<ThunkHandler_impl*>(CTX->ThunkHandler.get());

            {
                std::shared_lock lk(That->ThunksMutex);
                rv = That->Libs.contains(Name);
            }
        }

        static void HostTrampolineForGuestcall(void* ArgsRV) {
          struct ArgsRV_t {
              uintptr_t GuestUnpacker;
              uintptr_t GuestTarget;
              uintptr_t rv;
          };

          auto &[GuestTarget, GuestUnpacker, rv] = *reinterpret_cast<ArgsRV_t*>(ArgsRV);

          auto CTX = Thread->CTX;
          auto That = reinterpret_cast<ThunkHandler_impl *>(CTX->ThunkHandler.get());

          // Try first with shared_lock
          {
            std::shared_lock lk(That->ThunksMutex);

            auto found = That->GuestcallToHostTrampoline.find(
                {GuestUnpacker, GuestTarget});
            if (found != That->GuestcallToHostTrampoline.end()) {
              rv = found->second;
              return;
            }
          }

          // retry lookup with full lock before making a new trampoline to avoid double allocations
          std::lock_guard lk(That->ThunksMutex);
          
          auto found = That->GuestcallToHostTrampoline.find({GuestUnpacker, GuestTarget}); \
          if (found != That->GuestcallToHostTrampoline.end()) {
            rv = found->second;
            return;
          }

          extern char __start_HostToGuestTrampolineTemplate_copy_section[];
          extern char __stop_HostToGuestTrampolineTemplate_copy_section[];

          const auto Length = __stop_HostToGuestTrampolineTemplate_copy_section - __start_HostToGuestTrampolineTemplate_copy_section;
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

          InstanceInfo->CallCallback = (uintptr_t)&CallCallback;
          InstanceInfo->GuestUnpacker = GuestUnpacker;
          InstanceInfo->GuestTarget = GuestTarget;

          rv = (uintptr_t)HostTrampoline;

          
          // Still protected by `lk`
          That->HostTrampolineToGuestcall.emplace(rv, GuestUnpacker, GuestTarget);
          That->GuestcallToHostTrampoline.emplace(GuestUnpacker, GuestTarget, rv);
        }

        static void GuestcallTargetForHostTrampoline(void* ArgsRV) {
          struct ArgsRV_t {
              uintptr_t HostTrampoline;
              uintptr_t rv;
          };

          auto CTX = Thread->CTX;
          auto That = reinterpret_cast<ThunkHandler_impl *>(CTX->ThunkHandler.get());

          auto &[HostTrampoline, rv] = *reinterpret_cast<ArgsRV_t*>(ArgsRV);

          {
            std::shared_lock lk(That->ThunksMutex);

            auto found = That->HostTrampolineToGuestcall.find(HostTrampoline); \
            if (found != That->HostTrampolineToGuestcall.end()) {
              rv = found->second.second;
            } else {
              rv = 0;
            }
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

        ~ThunkHandler_impl() {
        }
    };

    ThunkHandler* ThunkHandler::Create() {
        return new ThunkHandler_impl();
    }
}
