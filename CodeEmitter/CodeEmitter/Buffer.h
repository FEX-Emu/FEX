// SPDX-License-Identifier: MIT
#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>
#ifdef FEX_HORIZON
#include <FEXCore/Utils/Horizon.h>
#endif

namespace ARMEmitter {
class Buffer {
public:
  Buffer() {
    SetBuffer(nullptr, 0);
  }

  Buffer(uint8_t* Base, uint64_t BaseSize) {
    SetBuffer(Base, BaseSize);
  }

  void SetBuffer(uint8_t* Base, uint64_t BaseSize) {
    BufferBase = Base;
    CurrentOffset = BufferBase;
    Size = BaseSize;
#ifdef FEX_HORIZON
    WritableOffset = reinterpret_cast<uintptr_t>(FEXCore::Allocator::HorizonWritableAddress(Base)) - reinterpret_cast<uintptr_t>(Base);
#endif
  }

  template<typename T>
  requires (std::is_trivially_copyable_v<T>)
  void dcn(const T& Data) {
    std::memcpy(WritableCursor(), &Data, sizeof(Data));
    CurrentOffset += sizeof(Data);
  }
  void dc8(uint8_t Data) {
    dcn(Data);
  }
  void dc16(uint16_t Data) {
    dcn(Data);
  }
  void dc32(uint32_t Data) {
    dcn(Data);
  }
  void dc64(uint64_t Data) {
    dcn(Data);
  }

  void EmitString(const char* String) {
    const auto StringLength = strlen(String);
    memcpy(WritableCursor(), String, StringLength);
    CurrentOffset += StringLength;
  }

  void Align(size_t Size = 4) {
    // Align the buffer to provided size.
    auto CurrentAlignment = reinterpret_cast<uint64_t>(CurrentOffset) & (Size - 1);
    if (!CurrentAlignment) {
      return;
    }
    std::memset(WritableCursor(), 0, Size - CurrentAlignment);
    CurrentOffset += Size - CurrentAlignment;
  }

  template<typename T>
  T GetCursorAddress() const {
    return reinterpret_cast<T>(CurrentOffset);
  }

  static void ClearICache(void* Begin, std::size_t Length) {
#ifdef FEX_HORIZON
    FEXCore::Allocator::HorizonFlushICache(Begin, Length);
#else
    __builtin___clear_cache(static_cast<char*>(Begin), static_cast<char*>(Begin) + Length);
#endif
  }

  template<typename T>
  static T* WritableAddress(T* Pointer) {
#ifdef FEX_HORIZON
    return static_cast<T*>(FEXCore::Allocator::HorizonWritableAddress(Pointer));
#else
    return Pointer;
#endif
  }

  size_t GetCursorOffset() const {
    return static_cast<size_t>(CurrentOffset - BufferBase);
  }

  uint8_t* GetBufferBase() const {
    return BufferBase;
  }

  void CursorIncrement(size_t Size) {
    CurrentOffset += Size;
  }

  void SetCursorOffset(size_t Offset) {
    CurrentOffset = BufferBase + Offset;
  }

  uint64_t GetBufferSize() const {
    return Size;
  }

  template<typename T>
  size_t GetCursorOffsetFromAddress(const T* Address) const {
    return static_cast<size_t>(reinterpret_cast<const uint8_t*>(Address) - BufferBase);
  }

  void* WritableCursor() const {
#ifdef FEX_HORIZON
    return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(CurrentOffset) + WritableOffset);
#else
    return CurrentOffset;
#endif
  }

protected:

  uint8_t* BufferBase;
  uint8_t* CurrentOffset;
  uint64_t Size;
#ifdef FEX_HORIZON
  uintptr_t WritableOffset {};
#endif
};
} // namespace ARMEmitter
