#pragma once
#include "Unwind/Unwind.h"

#include <cstdint>
#include <functional>
#include <string>

namespace CoreDumpService {
class CoreDumpClass;
}

namespace Unwind::x86 {
  Unwinder *Unwind(CoreDumpService::CoreDumpClass *CoreDump, void *Info, void *Context);
}
