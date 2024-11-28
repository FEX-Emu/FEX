// SPDX-License-Identifier: MIT
#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>

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
  }

  void dc8(uint8_t Data) {
    decltype(Data)* Memory = reinterpret_cast<decltype(Data)*>(CurrentOffset);
    *Memory = Data;
    CurrentOffset += sizeof(Data);
  }

  void dc16(uint16_t Data) {
    decltype(Data)* Memory = reinterpret_cast<decltype(Data)*>(CurrentOffset);
    *Memory = Data;
    CurrentOffset += sizeof(Data);
  }

  void dc32(uint32_t Data) {
    decltype(Data)* Memory = reinterpret_cast<decltype(Data)*>(CurrentOffset);
    *Memory = Data;
    CurrentOffset += sizeof(Data);
  }

  void dc64(uint64_t Data) {
    decltype(Data)* Memory = reinterpret_cast<decltype(Data)*>(CurrentOffset);
    *Memory = Data;
    CurrentOffset += sizeof(Data);
  }
  void EmitString(const char* String) {
    const auto StringLength = strlen(String);
    memcpy(CurrentOffset, String, StringLength);
    CurrentOffset += StringLength;
  }

  void Align() {
    // Align the buffer to instruction size
    auto CurrentAlignment = reinterpret_cast<uint64_t>(CurrentOffset) & 0b11;
    if (!CurrentAlignment) {
      return;
    }
    CurrentOffset += 4 - CurrentAlignment;
  }

  template<typename T>
  T GetCursorAddress() const {
    return reinterpret_cast<T>(CurrentOffset);
  }

  static void ClearICache(void* Begin, std::size_t Length) {
    __builtin___clear_cache(static_cast<char*>(Begin), static_cast<char*>(Begin) + Length);
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

protected:

  uint8_t* BufferBase;
  uint8_t* CurrentOffset;
  uint64_t Size;
};
} // namespace ARMEmitter
