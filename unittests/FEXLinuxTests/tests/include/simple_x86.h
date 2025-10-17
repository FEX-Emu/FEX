#pragma once
#include <cstdint>

class SimpleX86Emit final {
public:
  enum Reg {
    RAX = 0,
    RCX = 1,
    RDX = 2,
    RBX = 3,
    RSP = 4,
    RBP = 5,
    RSI = 6,
    RDI = 7,
    // r8 and higher not implemented.
  };
  SimpleX86Emit(void* Ptr, std::size_t size)
    : Ptr {static_cast<uint8_t*>(Ptr)}
    , EndPtr {static_cast<uint8_t*>(Ptr) + size} {}

  void ret() {
    db<uint8_t>(0xc3);
  }

  void mov(Reg reg, uint32_t val) {
    db<uint8_t>(0xB8 + reg);
    db(val);
  }

  void dd(uint32_t val) {
    db(val);
  }

  bool HadError() const {
    return _HadError;
  }

private:
  uint8_t* Ptr;
  uint8_t* EndPtr;
  bool _HadError {};

  template<typename T>
  void db(T v) {
    static_assert(sizeof(uint32_t) == 4);
    std::size_t i {};
    for (i = 0; i < sizeof(T) && Ptr != EndPtr; ++i) {
      *Ptr = v >> (i * 8);
      ++Ptr;
    }

    _HadError = i != sizeof(T);
  }
};
