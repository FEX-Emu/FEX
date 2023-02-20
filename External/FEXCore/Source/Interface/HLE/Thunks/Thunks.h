/*
$info$
tags: glue|thunks
$end_info$
*/

#pragma once

#include <FEXCore/IR/IR.h>

#include <vector>

namespace FEXCore::Context {
  class ContextImpl;
}

namespace FEXCore::Core {
  struct InternalThreadState;
}

namespace FEXCore::IR {
  struct SHA256Sum;
}

namespace FEXCore {
    typedef void ThunkedFunction(void* ArgsRv);

    class ThunkHandler {
    public:
        virtual ThunkedFunction* LookupThunk(const IR::SHA256Sum &sha256) = 0;
        virtual void RegisterTLSState(FEXCore::Core::InternalThreadState *Thread) = 0;
        virtual ~ThunkHandler() { }

        static ThunkHandler* Create();

        virtual void AppendThunkDefinitions(std::vector<FEXCore::IR::ThunkDefinition> const& Definitions) = 0;
    };
};
