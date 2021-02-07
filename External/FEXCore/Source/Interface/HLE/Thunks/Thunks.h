#pragma once
#include <FEXCore/IR/IR.h>

namespace FEXCore::Core {
  struct InternalThreadState;
}

namespace FEXCore {
    typedef void ThunkedFunction(void* ArgsRv);

    class ThunkHandler {
    public:
        virtual ThunkedFunction* LookupThunk(const IR::SHA256Sum &sha256) = 0;
        virtual void RegisterTLSState(FEXCore::Core::InternalThreadState *Thread) = 0;
        virtual ~ThunkHandler() { }

        static ThunkHandler* Create();
    };
};