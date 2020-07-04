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
            thunks["fexthunk:add"] = (ThunkedFunction*)&fexthunks_forward_fexthunk_add;
            thunks["fexthunk:test"] = (ThunkedFunction*)&fexthunks_forward_fexthunk_test;
            thunks["fexthunk:test_void"] = (ThunkedFunction*)&fexthunks_forward_fexthunk_test_void;
        }
    };

    ThunkHandler* ThunkHandler::Create() {
        return new ThunkHandler_impl();
    }
}