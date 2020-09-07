#pragma once
#include <stdint.h>

namespace FEXCore {
class X86GeneratedCode final {
public:
  X86GeneratedCode();
  ~X86GeneratedCode();

  uint64_t SignalReturn{};
  uint64_t CallbackReturn{};

private:
  void *CodePtr{};
};
}
