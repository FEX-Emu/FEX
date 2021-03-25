#pragma once

#include <vector>
#include <stdint.h>

namespace FEXCore::CPU {
class CodeBuffer;

// Holds the data for a Dwarf Frame that has been registered with glibc
// Will automatically unregister when deleted
class DwarfFrame {
public:
  DwarfFrame() {}
  DwarfFrame(CodeBuffer *Buffer);
  ~DwarfFrame();

  DwarfFrame(DwarfFrame &&) noexcept = default;
  DwarfFrame& operator=(DwarfFrame &&) noexcept = default;


private:
  std::vector<uint8_t> DwarfData;
  int FdeOffset;

  void EmitDwarfForCodeBuffer(CodeBuffer *Buffer);
};
static_assert(std::is_nothrow_move_constructible<DwarfFrame>::value, "DwarfFrame should be noexcept MoveConstructible");


void Backtrace();
}