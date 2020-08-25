#include "Thunks.h"
#include <LogManager.h>

#include "stdio.h"
#include <dlfcn.h>


#include <string>
#include <map>
#include <Interface/Context/Context.h>
#include "Interface/Core/InternalThreadState.h"
#include "FEXCore/Core/X86Enums.h"
#include <LogManager.h>
#include <mutex>
#include <shared_mutex>

struct LoadlibArgs {
    const char *Name;
    uintptr_t CallbackThunks;
};

static thread_local FEXCore::Core::InternalThreadState *Thread;


namespace FEXCore {
    
    struct ExportEntry { const char* Name; ThunkedFunction* Fn; };

    class ThunkHandler_impl final: public ThunkHandler {
        std::shared_mutex ThunksMutex;

        std::map<std::string, ThunkedFunction*> Thunks = {
            { "fex:loadlib", &LoadLib}
        };

        /*
            Set arg0/1 to arg regs, use CTX::HandleCallback to handle the callback
        */
        static void CallCallback(void *callback, void *arg0, void* arg1) {
            
            Thread->State.State.gregs[FEXCore::X86State::REG_RDI] = (uintptr_t)arg0;
            Thread->State.State.gregs[FEXCore::X86State::REG_RSI] = (uintptr_t)arg1;

            Thread->CTX->HandleCallback((uintptr_t)callback);
        }

        static void LoadLib(void *ArgsV) {

            auto Args = reinterpret_cast<LoadlibArgs*>(ArgsV);

            auto CTX = Thread->CTX;

            if (!CTX->Config.UnifiedMemory) {
                Args = CTX->MemoryMapper.GetPointer<LoadlibArgs*>((uintptr_t)Args);
            }

            auto Name = Args->Name;
            auto CallbackThunks = Args->CallbackThunks;

            if (!CTX->Config.UnifiedMemory) {
                Name = CTX->MemoryMapper.GetPointer<const char*>((uintptr_t)Name);
                CallbackThunks = CTX->MemoryMapper.GetPointer<uintptr_t>(CallbackThunks);
            }

            auto SOName = CTX->Config.ThunkLibsPath + "/" + (const char*)Name + "-host.so";

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
                for (i = 0; Exports[i].Name; i++) {
                    That->Thunks[Exports[i].Name] = Exports[i].Fn;
                }

                LogMan::Msg::D("Loaded %d syms", i);
            }
        }

        public:

        ThunkedFunction* LookupThunk(const char *Name) {

            std::shared_lock lk(ThunksMutex);

            auto it = Thunks.find(Name);

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
    };

    ThunkHandler* ThunkHandler::Create() {
        return new ThunkHandler_impl();
    }
}