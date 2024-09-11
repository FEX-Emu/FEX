// SPDX-License-Identifier: MIT
#pragma once

#include <FEXCore/Utils/CompilerDefs.h>
#include <FEXCore/Utils/EnumUtils.h>
#include <FEXCore/Utils/ThreadPoolAllocator.h>
#include <FEXCore/fextl/memory.h>
#include <FEXCore/fextl/sstream.h>
#include <FEXCore/Utils/EnumOperators.h>

#include <cstdint>
#include <cstring>

#include <fmt/format.h>

namespace FEXCore::IR {

class OrderedNode;
class RegisterAllocationPass;
class RegisterAllocationData;

enum class SyscallFlags : uint8_t {
  DEFAULT = 0,
  // Syscalldoesn't care about CPUState being serialized up to the syscall instruction.
  // Means dead code elimination can optimize through a syscall operation.
  OPTIMIZETHROUGH = 1 << 0,
  // Syscall only reads the passed in arguments. Doesn't read CPUState.
  NOSYNCSTATEONENTRY = 1 << 1,
  // Syscall doesn't return. Code generation after syscall return can be removed.
  NORETURN = 1 << 2,
  // Syscall doesn't have any side-effects, so if the result isn't used then it can be removed.
  NOSIDEEFFECTS = 1 << 3,
  // Syscall doesn't return a result.
  // Means the resulting register shouldn't be written (Usually RAX).
  // Usually used with !NOSYNCSTATEONENTRY, so the syscall can modify CPU state entirely.
  // Then on return FEXCore picks up the new state.
  NORETURNEDRESULT = 1 << 4,
};

FEX_DEF_NUM_OPS(SyscallFlags)

// This enum of named vector constants are linked to an array in CPUBackend.cpp.
// This is used with the IROp `LoadNamedVectorConstant` to load a vector constant
// that would otherwise be costly to materialize.
enum NamedVectorConstant : uint8_t {
  NAMED_VECTOR_INCREMENTAL_U16_INDEX = 0,
  NAMED_VECTOR_INCREMENTAL_U16_INDEX_UPPER,
  NAMED_VECTOR_PADDSUBPS_INVERT,
  NAMED_VECTOR_PADDSUBPS_INVERT_UPPER,
  NAMED_VECTOR_PADDSUBPD_INVERT,
  NAMED_VECTOR_PADDSUBPD_INVERT_UPPER,
  NAMED_VECTOR_PSUBADDPS_INVERT,
  NAMED_VECTOR_PSUBADDPS_INVERT_UPPER,
  NAMED_VECTOR_PSUBADDPD_INVERT,
  NAMED_VECTOR_PSUBADDPD_INVERT_UPPER,
  NAMED_VECTOR_MOVMSKPS_SHIFT,
  NAMED_VECTOR_AESKEYGENASSIST_SWIZZLE,
  NAMED_VECTOR_BLENDPS_0110B,
  NAMED_VECTOR_BLENDPS_0111B,
  NAMED_VECTOR_BLENDPS_1001B,
  NAMED_VECTOR_BLENDPS_1011B,
  NAMED_VECTOR_BLENDPS_1101B,
  NAMED_VECTOR_BLENDPS_1110B,
  NAMED_VECTOR_MOVMASKB,
  NAMED_VECTOR_MOVMASKB_UPPER,

  NAMED_VECTOR_X87_ONE,
  NAMED_VECTOR_X87_LOG2_10,
  NAMED_VECTOR_X87_LOG2_E,
  NAMED_VECTOR_X87_PI,
  NAMED_VECTOR_X87_LOG10_2,
  NAMED_VECTOR_X87_LOG_2,

  NAMED_VECTOR_CONST_POOL_MAX,
  // Beginning of named constants that don't have a constant pool backing.
  NAMED_VECTOR_ZERO = NAMED_VECTOR_CONST_POOL_MAX,
  NAMED_VECTOR_MAX,
};

// This enum of named vector constants are linked to an array in CPUBackend.cpp.
// This is used with the IROp `LoadNamedVectorIndexedConstant` to load a vector constant
// that would otherwise be costly to materialize.
enum IndexNamedVectorConstant : uint8_t {
  INDEXED_NAMED_VECTOR_PSHUFLW = 0,
  INDEXED_NAMED_VECTOR_PSHUFHW,
  INDEXED_NAMED_VECTOR_PSHUFD,
  INDEXED_NAMED_VECTOR_SHUFPS,
  INDEXED_NAMED_VECTOR_DPPS_MASK,
  INDEXED_NAMED_VECTOR_DPPD_MASK,
  INDEXED_NAMED_VECTOR_PBLENDW,
  INDEXED_NAMED_VECTOR_MAX,
};

struct SHA256Sum final {
  uint8_t data[32];
  [[nodiscard]]
  bool
  operator<(const SHA256Sum& rhs) const {
    return memcmp(data, rhs.data, sizeof(data)) < 0;
  }

  [[nodiscard]]
  bool
  operator==(const SHA256Sum& rhs) const {
    return memcmp(data, rhs.data, sizeof(data)) == 0;
  }
};

// FEX thunks support two ABIs for passing arguments
// - Default: Build the entire argument and return values on the stack
// - InRegister: Pass all GPR and FPR arguments in registers through AAPCS64 ABI
//   - x86-64 Linux SystemV ABI:
//     - GPR order (Max 6): RDI, RSI, RDX, RCX, R8, R9
//     - FPR order (Max 8): XMM0 - XMM7
//     - GPR return register: RAX (also RDX for 128-bit return values)
//     - FPR return register: XMM0 (also XMM1 for 256-bit return values)
//   - AAPCS64 ABI:
//     - GPR order (Max 8): R0 - R7
//     - FPR order (Max 8): V0 - V7
//     - GPR return register: R0 (also R1 for 128-bit return values)
//     - FPR return register: V0 (Also V1-V3 for up to 512-bit of return data)
//   - Smaller than 32-bit considerations:
//     - SystemV doesn't specify which side of caller-callee does zero/sign extension on arguments.
//       - AAPCS64: The Callee must zero or sign extension arguments, which effectively matches behaviour.
//     - SystemV doesn't specify which side of caller-calle does zero/sign extension on RETURNS.
//       - AAPCS64: Specifies the Caller must zero/sign result, which effectively matches behaviour.
//     - FPR arguments and garbage data in the upper part of the 128-bit vector register
//       - Both x86-64 and AArch64 leave garbage in the upper bits, doesn't matter.
//     - If any of these considerations don't hold then this `InRegister` ABI breaks.
//   - Return values with <= 128-bit size but returns float and integers
//     - Common case is a struct with a float and an integer inside of it.
//     - These get packed in to registers on AArch64 and x86-64 but differently!
//     - AArch64: Packs the float and GPR result in to GPRs entirely.
//       - If <= 64-bit, then only uses X0, if > 64-bit then X0 and X1.
//     - x86-64: Packs the floats and GPR in to RAX and xmm0 if <= 128-bit
//       - If the return is > 128-bit then indirect result is used instead.
enum class ThunkABIFlags : uint32_t {
  // When ABI type is `InRegister`
  GPR_Arg0 = (1U << 0),
  GPR_Arg1 = (1U << 1),
  GPR_Arg2 = (1U << 2),
  GPR_Arg3 = (1U << 3),
  GPR_Arg4 = (1U << 4),
  GPR_Arg5 = (1U << 5),

  FPR_Arg0 = (1U << 6),
  FPR_Arg1 = (1U << 7),
  FPR_Arg2 = (1U << 8),
  FPR_Arg3 = (1U << 9),
  FPR_Arg4 = (1U << 10),
  FPR_Arg5 = (1U << 11),

  ReturnType_Void = (0b00U << 12),
  ReturnType_GPR = (0b01U << 12),
  ReturnType_FPR = (0b10U << 12),

  // Bits [30:14]: Reserved

  // When ABI type is `DefaultOnStack`
  // Bits [30:0]: Reserved

  // Register arguments in use
  // ABI Style
  ABI_DefaultOnStack = (0U << 31),
  ABI_InRegister = (1U << 31),

  Default = 0,
};

FEX_DEF_NUM_OPS(ThunkABIFlags)

typedef void ThunkedFunction(void* ArgsRv);

// The definition of the thunk in the binary.
struct ThunkBinary final {
  SHA256Sum Sum;
  ThunkABIFlags Flags;
};

struct ThunkDefinition final {
  SHA256Sum Sum;
  ThunkedFunction* ThunkFunction;
  ThunkABIFlags Flags;
};

} // namespace FEXCore::IR
