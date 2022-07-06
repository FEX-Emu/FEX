#pragma once

#include <cstdint>

namespace FEXCore {
    namespace Core {
        struct NamedRegion;
        struct DebugData;
    }
    void GDBJITRegister(FEXCore::Core::NamedRegion *Entry, uintptr_t VAFileStart, uint64_t GuestRIP, uintptr_t HostEntry, FEXCore::Core::DebugData *DebugData);
}