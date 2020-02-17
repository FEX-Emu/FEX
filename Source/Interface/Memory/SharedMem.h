#pragma once

#include <FEXCore/Memory/SharedMem.h>
#include <stddef.h>
#include <stdint.h>

namespace FEXCore::SHM {
  struct InternalSHMObject {
    SHMObject Object;
    int SHMFD;
    size_t Size;
  };
}
