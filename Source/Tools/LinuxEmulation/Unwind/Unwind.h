// SPDX-License-Identifier: MIT
#pragma once

#include <FEXCore/fextl/memory.h>

namespace FEX::HLE {
struct ThreadStateObject;
}

namespace CoreDumpService {
class CoreDumpClass;
}

namespace Unwind {
class Unwinder {
public:
  virtual void Backtrace(FEX::HLE::ThreadStateObject* Thread, void* Info, void* Context) = 0;
  virtual ~Unwinder() {}
};
} // namespace Unwind

namespace Unwind::x86 {
fextl::unique_ptr<Unwinder> Unwind();
}

namespace Unwind::x86_64 {
fextl::unique_ptr<Unwinder> Unwind();
}
