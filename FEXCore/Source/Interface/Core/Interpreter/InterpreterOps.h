// SPDX-License-Identifier: MIT
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include <FEXCore/Core/CoreState.h>
#include <FEXCore/IR/IR.h>

namespace FEXCore::IR {
class IRListView;
struct IROp_Header;
} // namespace FEXCore::IR

namespace FEXCore::CPU {
enum FallbackABI {
  FABI_UNKNOWN,
  FABI_F80_I16_F32,
  FABI_F80_I16_F64,
  FABI_F80_I16_I16,
  FABI_F80_I16_I32,
  FABI_F32_I16_F80,
  FABI_F64_I16_F80,
  FABI_F64_I16_F64,
  FABI_F64_I16_F64_F64,
  FABI_I16_I16_F80,
  FABI_I32_FCW_F80,
  FABI_I64_I16_F80,
  FABI_I64_I16_F80_F80,
  FABI_F80_I16_F80,
  FABI_F80_I16_F80_F80,
  FABI_I32_I64_I64_I128_I128_I16,
  FABI_I32_I128_I128_I16,
  FABI_I32_I32_F80,
};

struct FallbackInfo {
  FallbackABI ABI;
  void* fn;
  FEXCore::Core::FallbackHandlerIndex HandlerIndex;
  bool SupportsPreserveAllABI;
};

class InterpreterOps {
public:
  static void FillFallbackIndexPointers(uint64_t* Info);
  static bool GetFallbackHandler(bool SupportsPreserveAllABI, const IR::IROp_Header* IROp, FallbackInfo* Info);
};
} // namespace FEXCore::CPU
