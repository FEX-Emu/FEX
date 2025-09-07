// SPDX-License-Identifier: MIT
/*
$info$
tags: glue|thunks
$end_info$
*/

#pragma once

namespace FEXCore::IR {
struct SHA256Sum;
}

namespace FEXCore {
typedef void ThunkedFunction(void* ArgsRv);

class ThunkHandler {
public:
  virtual ~ThunkHandler() = default;
  virtual ThunkedFunction* LookupThunk(const IR::SHA256Sum& sha256) = 0;
};
} // namespace FEXCore
