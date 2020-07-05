#include "Thunks.h"
#include <LogManager.h>

#include "stdio.h"
#include <dlfcn.h>


#include <string>
#include <map>


namespace FEXCore {

    namespace {
        struct ExportEntry { const char* name; void(*fn)(void*); };

        static void loadlib(void* name);

        static std::map<std::string, ThunkedFunction*> thunks = {
            { "fex:loadlib", &loadlib}
        };

        void loadlib(void* name) {
            //TODO: pass path via env / parameter / relative to bin path
            auto soname = std::string("/home/skmp/projects/FEX/ThunkLibs/") + (const char*)name + "-host.so";

            printf("Load lib: %s -> %s\n", name, soname.c_str());
            auto handle = dlopen(soname.c_str(), RTLD_LOCAL | RTLD_NOW);

            ExportEntry* (*init_fn)();

            auto init_sym = std::string("fexthunks_exports_") + (const char*)name;

            (void*&)init_fn = dlsym(handle, init_sym.c_str());

            auto exports = init_fn();

            // TODO: lock
            int i;
            for (i = 0; exports[i].name; i++) {
                thunks[exports[i].name] = exports[i].fn;
            }

            printf("Loaded %d syms\n", i);
        }
    }

    class ThunkHandler_impl: public ThunkHandler {
        public:

        ThunkedFunction* LookupThunk(const char *name) {
            if (thunks.count(name)) {
                return thunks[name];
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