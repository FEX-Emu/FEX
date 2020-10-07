#pragma once
#include <stdint.h>

namespace FEXCore::Core {
struct InternalThreadState;
}

namespace FEXCore::HLE::x32 {
  // We must define this ourselves since it doesn't exist on non-x86 platforms
  struct user_desc {
    uint32_t entry_number;
    uint32_t base_addr;
    uint32_t limit;
    uint32_t seg_32bit       : 1;
    uint32_t contents        : 2;
    uint32_t read_exec_only  : 1;
    uint32_t limit_in_pages  : 1;
    uint32_t seg_not_present : 1;
    uint32_t useable         : 1;
  };
}
