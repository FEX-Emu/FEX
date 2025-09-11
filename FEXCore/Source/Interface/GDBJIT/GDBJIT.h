// SPDX-License-Identifier: MIT

#include <FEXCore/Core/CodeCache.h>
#include <Interface/Core/JIT/DebugData.h>

namespace FEXCore {
void GDBJITRegister(FEXCore::ExecutableFileInfo&, uintptr_t VAFileStart, uint64_t GuestRIP, uintptr_t HostEntry, FEXCore::Core::DebugData&);
}
