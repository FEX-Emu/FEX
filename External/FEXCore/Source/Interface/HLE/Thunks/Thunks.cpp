#include "Thunks.h"

#include "stdio.h"

int fexthunks_impl_fexthunk_add(int a, int b) {
    return a + b;
}

int fexthunks_impl_fexthunk_test(int a) {
    return printf("fexthunk_test: %d\n", a);
}

void fexthunks_impl_fexthunk_test_void(int a) {
    printf("fexthunk_test_void: %d\n", a);
}

#include "FexThunk_forwards.inl"

#include <GL/glx.h>
#include <GL/gl.h>
#include <dlfcn.h>

#include "GLX_forwards.inl"

#include <string>
#include <map>

namespace FEXCore {
    class ThunkHandler_impl: public ThunkHandler {
        public:
        std::map<std::string, ThunkedFunction*> thunks;

        ThunkedFunction* LookupThunk(const char *name) {
            if (thunks.count(name)) {
                return thunks[name];
            } else {
                return nullptr;
            }
        }

        ThunkHandler_impl() {
            thunks["fexthunk:add"] = &fexthunks_forward_fexthunk_add;
            thunks["fexthunk:test"] = &fexthunks_forward_fexthunk_test;
            thunks["fexthunk:test_void"] = &fexthunks_forward_fexthunk_test_void;

            #include "GLX_thunkmap.inl"

            fexthunks_init_libGL();
            fexthunks_init_libX11();
        }
    };

    ThunkHandler* ThunkHandler::Create() {
        return new ThunkHandler_impl();
    }
}