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
#include "Thunks.h"

#include <dlfcn.h>

#include <Interface/Context/Context.h>
#include "FEXCore/Core/X86Enums.h"
#include <malloc.h>
#include <map>
#include <memory>
#include <shared_mutex>
#include <stdint.h>
#include <string>
#include <utility>

struct LoadlibArgs {
    const char *Name;
    uintptr_t CallbackThunks;
};

static thread_local FEXCore::Core::InternalThreadState *Thread;


namespace FEXCore {
    struct ExportEntry { uint8_t *sha256; ThunkedFunction* Fn; };

    class ThunkHandler_impl final: public ThunkHandler {
        std::shared_mutex ThunksMutex;

        std::map<IR::SHA256Sum, ThunkedFunction*> Thunks = {
            {
                // sha256(fex:loadlib)
                { 0x27, 0x7e, 0xb7, 0x69, 0x5b, 0xe9, 0xab, 0x12, 0x6e, 0xf7, 0x85, 0x9d, 0x4b, 0xc9, 0xa2, 0x44, 0x46, 0xcf, 0xbd, 0xb5, 0x87, 0x43, 0xef, 0x28, 0xa2, 0x65, 0xba, 0xfc, 0x89, 0x0f, 0x77, 0x80},
                &LoadLib
            }
        };

        /*
            Set arg0/1 to arg regs, use CTX::HandleCallback to handle the callback
        */
        static void CallCallback(void *callback, void *arg0, void* arg1) {
          Thread->CurrentFrame->State.gregs[FEXCore::X86State::REG_RDI] = (uintptr_t)arg0;
          Thread->CurrentFrame->State.gregs[FEXCore::X86State::REG_RSI] = (uintptr_t)arg1;

          Thread->CTX->HandleCallback(Thread, (uintptr_t)callback);
        }

        static void LoadLib(void *ArgsV) {
            auto CTX = Thread->CTX;

            auto Args = reinterpret_cast<LoadlibArgs*>(ArgsV);

            auto Name = Args->Name;
            auto CallbackThunks = Args->CallbackThunks;

            auto SOName = CTX->Config.ThunkHostLibsPath() + "/" + (const char*)Name + "-host.so";

            LogMan::Msg::D("Load lib: %s -> %s", Name, SOName.c_str());

            auto Handle = dlopen(SOName.c_str(), RTLD_LOCAL | RTLD_NOW);

            if (!Handle) {
                LogMan::Msg::E("Load lib: failed to dlopen %s: %s", SOName.c_str(), dlerror());
                return;
            }

            ExportEntry* (*InitFN)(void *, uintptr_t);

            auto InitSym = std::string("fexthunks_exports_") + (const char*)Name;

            (void*&)InitFN = dlsym(Handle, InitSym.c_str());

            if (!InitFN) {
                LogMan::Msg::E("Load lib: failed to find export %s", InitSym.c_str());
                return;
            }

            auto Exports = InitFN((void*)&CallCallback, CallbackThunks);

            auto That = reinterpret_cast<ThunkHandler_impl*>(CTX->ThunkHandler.get());

            {
                std::unique_lock lk(That->ThunksMutex);

                int i;
                for (i = 0; Exports[i].sha256; i++) {
                    That->Thunks[*reinterpret_cast<IR::SHA256Sum*>(Exports[i].sha256)] = Exports[i].Fn;
                }

                LogMan::Msg::D("Loaded %d syms", i);
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
