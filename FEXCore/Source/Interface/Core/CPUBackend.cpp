#include "FEXCore/IR/IR.h"
#include "FEXCore/Utils/AllocatorHooks.h"
#include "Interface/Context/Context.h"
#include "Interface/Core/Dispatcher/Dispatcher.h"
#include <FEXCore/Core/CPUBackend.h>

namespace FEXCore {
namespace CPU {

constexpr static uint64_t NamedVectorConstants[FEXCore::IR::NamedVectorConstant::NAMED_VECTOR_MAX][2] = {
  {0x0003'0002'0001'0000, 0x0007'0006'0005'0004}, // NAMED_VECTOR_INCREMENTAL_U16_INDEX
  {0x000B'000A'0009'0008, 0x000F'000E'000D'000C}, // NAMED_VECTOR_INCREMENTAL_U16_INDEX_UPPER
  {0x0000'0000'8000'0000, 0x0000'0000'8000'0000}, // NAMED_VECTOR_PADDSUBPS_INVERT
  {0x0000'0000'8000'0000, 0x0000'0000'8000'0000}, // NAMED_VECTOR_PADDSUBPS_INVERT_UPPER
  {0x8000'0000'0000'0000, 0x0000'0000'0000'0000}, // NAMED_VECTOR_PADDSUBPD_INVERT
  {0x8000'0000'0000'0000, 0x0000'0000'0000'0000}, // NAMED_VECTOR_PADDSUBPD_INVERT_UPPER
  {0x0000'0001'0000'0000, 0x0000'0003'0000'0002}, // NAMED_VECTOR_MOVMSKPS_SHIFT
};

constexpr static auto PSHUFLW_LUT {
[]() consteval {
  struct LUTType {
    uint64_t Val[2];
  };
  // Expectation for this LUT is to simulate PSHUFLW with ARM's TBL (single register) instruction
  // PSHUFLW behaviour:
  // 16-bit words in [63:48], [47:32], [31:16], [15:0] are selected using the 8-bit Index.
  // For 128-bit PSHUFLW, bits [127:64] are identity copied.
  constexpr uint64_t IdentityCopyUpper = 0x0f'0e'0d'0c'0b'0a'09'08;
  std::array<LUTType, 256> TotalLUT{};
  uint64_t WordSelection[4] = {
    0x01'00,
    0x03'02,
    0x05'04,
    0x07'06,
  };
  for (size_t i = 0; i < 256; ++i) {
    auto &LUT = TotalLUT[i];
    const auto Word0 = (i >> 0) & 0b11;
    const auto Word1 = (i >> 2) & 0b11;
    const auto Word2 = (i >> 4) & 0b11;
    const auto Word3 = (i >> 6) & 0b11;

    LUT.Val[0] =
      (WordSelection[Word0] << 0) |
      (WordSelection[Word1] << 16) |
      (WordSelection[Word2] << 32) |
      (WordSelection[Word3] << 48);

    LUT.Val[1] = IdentityCopyUpper;
  }
  return TotalLUT;
}()
};

constexpr static auto PSHUFHW_LUT {
[]() consteval {
  struct LUTType {
    uint64_t Val[2];
  };
  // Expectation for this LUT is to simulate PSHUFHW with ARM's TBL (single register) instruction
  // PSHUFHW behaviour:
  // 16-bit words in [127:112], [111:96], [95:80], [79:64] are selected using the 8-bit Index.
  // Incoming words come from bits [127:64] of the source.
  // Bits [63:0] are identity copied.
  constexpr uint64_t IdentityCopyLower = 0x07'06'05'04'03'02'01'00;
  std::array<LUTType, 256> TotalLUT{};
  uint64_t WordSelection[4] = {
    0x09'08,
    0x0b'0a,
    0x0d'0c,
    0x0f'0e,
  };
  for (size_t i = 0; i < 256; ++i) {
    auto &LUT = TotalLUT[i];
    const auto Word0 = (i >> 0) & 0b11;
    const auto Word1 = (i >> 2) & 0b11;
    const auto Word2 = (i >> 4) & 0b11;
    const auto Word3 = (i >> 6) & 0b11;

    LUT.Val[0] = IdentityCopyLower;

    LUT.Val[1] =
      (WordSelection[Word0] << 0) |
      (WordSelection[Word1] << 16) |
      (WordSelection[Word2] << 32) |
      (WordSelection[Word3] << 48);

  }
  return TotalLUT;
}()
};

constexpr static auto PSHUFD_LUT {
[]() consteval {
  struct LUTType {
    uint64_t Val[2];
  };
  // Expectation for this LUT is to simulate PSHUFD with ARM's TBL (single register) instruction
  // PSHUFD behaviour:
  // 32-bit words in [127:96], [95:64], [63:32], [31:0] are selected using the 8-bit Index.
  std::array<LUTType, 256> TotalLUT{};
  uint64_t WordSelection[4] = {
    0x03'02'01'00,
    0x07'06'05'04,
    0x0b'0a'09'08,
    0x0f'0e'0d'0c,
  };
  for (size_t i = 0; i < 256; ++i) {
    auto &LUT = TotalLUT[i];
    const auto Word0 = (i >> 0) & 0b11;
    const auto Word1 = (i >> 2) & 0b11;
    const auto Word2 = (i >> 4) & 0b11;
    const auto Word3 = (i >> 6) & 0b11;

    LUT.Val[0] =
      (WordSelection[Word0] << 0) |
      (WordSelection[Word1] << 32);

    LUT.Val[1] =
      (WordSelection[Word2] << 0) |
      (WordSelection[Word3] << 32);
  }
  return TotalLUT;
}()
};

CPUBackend::CPUBackend(FEXCore::Core::InternalThreadState *ThreadState, size_t InitialCodeSize, size_t MaxCodeSize)
    : ThreadState(ThreadState), InitialCodeSize(InitialCodeSize), MaxCodeSize(MaxCodeSize) {

  auto &Common = ThreadState->CurrentFrame->Pointers.Common;

  // Initialize named vector constants.
  for (size_t i = 0; i < FEXCore::IR::NamedVectorConstant::NAMED_VECTOR_MAX; ++i) {
    Common.NamedVectorConstantPointers[i] = reinterpret_cast<uint64_t>(NamedVectorConstants[i]);
  }

  // Initialize Indexed named vector constants.
  Common.IndexedNamedVectorConstantPointers[FEXCore::IR::IndexNamedVectorConstant::INDEXED_NAMED_VECTOR_PSHUFLW] = reinterpret_cast<uint64_t>(PSHUFLW_LUT.data());
  Common.IndexedNamedVectorConstantPointers[FEXCore::IR::IndexNamedVectorConstant::INDEXED_NAMED_VECTOR_PSHUFHW] = reinterpret_cast<uint64_t>(PSHUFHW_LUT.data());
  Common.IndexedNamedVectorConstantPointers[FEXCore::IR::IndexNamedVectorConstant::INDEXED_NAMED_VECTOR_PSHUFD] = reinterpret_cast<uint64_t>(PSHUFD_LUT.data());

#ifndef FEX_DISABLE_TELEMETRY
  // Fill in telemetry values
  for (size_t i = 0; i < FEXCore::Telemetry::TYPE_LAST; ++i) {
    auto &Telem = FEXCore::Telemetry::GetTelemetryValue(static_cast<FEXCore::Telemetry::TelemetryType>(i));
    Common.TelemetryValueAddresses[i] = reinterpret_cast<uint64_t>(Telem.GetAddr());
  }
#endif
}

CPUBackend::~CPUBackend() {
  for (auto CodeBuffer : CodeBuffers) {
    FreeCodeBuffer(CodeBuffer);
  }
  CodeBuffers.clear();
}

auto CPUBackend::GetEmptyCodeBuffer() -> CodeBuffer * {
  if (ThreadState->CurrentFrame->SignalHandlerRefCounter == 0) {
    if (CodeBuffers.empty()) {
      auto NewCodeBuffer = AllocateNewCodeBuffer(InitialCodeSize);
      EmplaceNewCodeBuffer(NewCodeBuffer);
    } else {
      if (CodeBuffers.size() > 1) {
        // If we have more than one code buffer we are tracking then walk them and delete
        // This is a cleanup step
        for (size_t i = 1; i < CodeBuffers.size(); i++) {
          FreeCodeBuffer(CodeBuffers[i]);
        }
        CodeBuffers.resize(1);
      }
      // Set the current code buffer to the initial
      CurrentCodeBuffer = &CodeBuffers[0];

      if (CurrentCodeBuffer->Size != MaxCodeSize) {
        FreeCodeBuffer(*CurrentCodeBuffer);

        // Resize the code buffer and reallocate our code size
        CurrentCodeBuffer->Size *= 1.5;
        CurrentCodeBuffer->Size = std::min(CurrentCodeBuffer->Size, MaxCodeSize);

        *CurrentCodeBuffer = AllocateNewCodeBuffer(CurrentCodeBuffer->Size);
      }
    }
  } else {
    // We have signal handlers that have generated code
    // This means that we can not safely clear the code at this point in time
    // Allocate some new code buffers that we can switch over to instead
    auto NewCodeBuffer = AllocateNewCodeBuffer(InitialCodeSize);
    EmplaceNewCodeBuffer(NewCodeBuffer);
  }

  return CurrentCodeBuffer;
}

auto CPUBackend::AllocateNewCodeBuffer(size_t Size) -> CodeBuffer {
  CodeBuffer Buffer;
  Buffer.Size = Size;
  Buffer.Ptr = static_cast<uint8_t *>(
      FEXCore::Allocator::VirtualAlloc(Buffer.Size, true));
  LOGMAN_THROW_AA_FMT(!!Buffer.Ptr, "Couldn't allocate code buffer");

  if (static_cast<Context::ContextImpl*>(ThreadState->CTX)->Config.GlobalJITNaming()) {
    static_cast<Context::ContextImpl*>(ThreadState->CTX)->Symbols.RegisterJITSpace(Buffer.Ptr, Buffer.Size);
  }
  return Buffer;
}

void CPUBackend::FreeCodeBuffer(CodeBuffer Buffer) {
  FEXCore::Allocator::VirtualFree(Buffer.Ptr, Buffer.Size);
}

bool CPUBackend::IsAddressInCodeBuffer(uintptr_t Address) const {
  for (auto &Buffer: CodeBuffers) {
    auto start = (uintptr_t)Buffer.Ptr;
    auto end = start + Buffer.Size;

    if (Address >= start && Address < end) {
      return true;
    }
  }

  return false;
}

}
}
