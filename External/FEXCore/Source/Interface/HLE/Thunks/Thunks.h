#pragma once

namespace FEXCore::Core {
  struct InternalThreadState;
}

namespace FEXCore {
    typedef void ThunkedFunction(void* ArgsRv);

    class ThunkHandler {
    public:
        virtual ThunkedFunction* LookupThunk(const char *name) = 0;
        virtual void RegisterTLSState(FEXCore::Core::InternalThreadState *Thread) = 0;
        virtual ~ThunkHandler() { }

        static ThunkHandler* Create();
    };
};