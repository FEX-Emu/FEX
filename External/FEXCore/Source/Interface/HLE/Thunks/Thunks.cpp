#include "Thunks.h"
#include <LogManager.h>

#include "stdio.h"
#include <dlfcn.h>


#include <string>
#include <map>
#include <Interface/Context/Context.h>
#include <LogManager.h>
#include <mutex>

namespace FEXCore {
    
    struct ExportEntry { const char* Name; ThunkedFunction* Fn; };

    class ThunkHandler_impl: public ThunkHandler {
        std::mutex ThunksMutex;

        std::map<std::string, ThunkedFunction*> Thunks = {
            { "fex:loadlib", &loadlib}
        };

        static void loadlib(FEXCore::Context::Context *CTX, void* Name) {
            //TODO: pass path via env / parameter / relative to bin path
            
            auto SOName = CTX->Config.ThunkLibsPath + "/" + (const char*)Name + "-host.so";

            LogMan::Msg::D("Load lib: %s -> %s", Name, SOName.c_str());
            
            auto Handle = dlopen(SOName.c_str(), RTLD_LOCAL | RTLD_NOW);

            if (!Handle) {
                LogMan::Msg::E("Load lib: failed to dlopen %s", SOName.c_str());
                return;
            }

            ExportEntry* (*InitFN)();

            auto InitSym = std::string("fexthunks_exports_") + (const char*)Name;

            (void*&)InitFN = dlsym(Handle, InitSym.c_str());

            if (!InitFN) {
                LogMan::Msg::E("Load lib: failed to find export %s", InitSym.c_str());
                return;
            }
            
            auto Exports = InitFN();

            auto That = reinterpret_cast<ThunkHandler_impl*>(CTX->ThunkHandler.get());

            {
                std::lock_guard<std::mutex> lk(That->ThunksMutex);

                int i;
                for (i = 0; Exports[i].Name; i++) {
                    That->Thunks[Exports[i].Name] = Exports[i].Fn;
                }

                LogMan::Msg::D("Loaded %d syms", i);
            }
        }

        public:

        ThunkedFunction* LookupThunk(const char *Name) {
            auto it = Thunks.find(Name);

            if (it != Thunks.end()) {
                return it->second;
            } else {
                return nullptr;
            }
        }

        ThunkHandler_impl() {

        }
    };

    ThunkHandler* ThunkHandler::Create() {
        return new ThunkHandler_impl();
    }
}