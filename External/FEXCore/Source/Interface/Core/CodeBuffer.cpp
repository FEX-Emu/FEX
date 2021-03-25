#include "Interface/Core/CodeBuffer.h"
#include <FEXCore/Utils/LogManager.h>

#include <sys/mman.h>
#include <utility>

namespace FEXCore::CPU {

CodeBuffer::CodeBuffer(size_t Size_) : Size(Size_) {
  Ptr = static_cast<uint8_t*>(
               mmap(nullptr,
                    Size,
                    PROT_READ | PROT_WRITE | PROT_EXEC,
                    MAP_PRIVATE | MAP_ANONYMOUS,
                    -1, 0));
  LogMan::Throw::A(Ptr != reinterpret_cast<uint8_t*>(~0ULL), "Couldn't allocate code buffer");

  Dwarf = DwarfFrame(this);
}

CodeBuffer::~CodeBuffer() {
  if (Ptr) {
    munmap(Ptr, Size);
  }
}

}
