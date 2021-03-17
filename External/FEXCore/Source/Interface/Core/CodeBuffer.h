#pragma once

#include "Interface/Core/ArchHelpers/Dwarf.h"

#include <stdint.h>


namespace FEXCore::CPU {
class CodeBuffer {
public:
  CodeBuffer() {}
  CodeBuffer(size_t Size);
  ~CodeBuffer();
  uint8_t *Ptr;
  size_t Size;

  CodeBuffer(CodeBuffer&& ) noexcept = default;
  CodeBuffer& operator=(CodeBuffer &&) noexcept = default;

private:
  DwarfFrame Dwarf;
};

static_assert(std::is_nothrow_move_constructible<DwarfFrame>::value, "DwarfFrame should be noexcept MoveConstructible");


}
