#include <FEXCore/Utils/Allocator.h>
#include <sys/mman.h>

namespace FEXCore::Allocator {
  MMAP_Hook mmap {::mmap};
  MUNMAP_Hook munmap {::munmap};
}
