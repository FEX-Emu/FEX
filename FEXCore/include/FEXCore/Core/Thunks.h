// SPDX-License-Identifier: MIT
/*
$info$
tags: glue|thunks
$end_info$
*/

#pragma once

#include <FEXCore/fextl/memory.h>
#include <FEXCore/fextl/vector.h>
#include <FEXCore/IR/IR.h>

namespace FEXCore::Context {
class Context;
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
  virtual ThunkedFunction* LookupThunk(const IR::SHA256Sum& sha256) = 0;
  virtual ~ThunkHandler() {}
};
}; // namespace FEXCore
