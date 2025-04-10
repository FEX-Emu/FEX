// SPDX-License-Identifier: MIT
#include "FEXCore/IR/IR.h"
#include "FEXCore/Utils/AllocatorHooks.h"
#include "Interface/Context/Context.h"
#include "Interface/Core/CPUBackend.h"
#include "Interface/Core/Dispatcher/Dispatcher.h"
#include <cstdint>

#include "LookupCache.h"

#ifndef _WIN32
#include <sys/prctl.h>
#endif

namespace FEXCore {
namespace CPU {

  constexpr static uint64_t NamedVectorConstants[FEXCore::IR::NamedVectorConstant::NAMED_VECTOR_CONST_POOL_MAX][2] = {
    {0x0003'0002'0001'0000ULL, 0x0007'0006'0005'0004ULL}, // NAMED_VECTOR_INCREMENTAL_U16_INDEX
    {0x000B'000A'0009'0008ULL, 0x000F'000E'000D'000CULL}, // NAMED_VECTOR_INCREMENTAL_U16_INDEX_UPPER
    {0x0000'0000'8000'0000ULL, 0x0000'0000'8000'0000ULL}, // NAMED_VECTOR_PADDSUBPS_INVERT
    {0x0000'0000'8000'0000ULL, 0x0000'0000'8000'0000ULL}, // NAMED_VECTOR_PADDSUBPS_INVERT_UPPER
    {0x8000'0000'0000'0000ULL, 0x0000'0000'0000'0000ULL}, // NAMED_VECTOR_PADDSUBPD_INVERT
    {0x8000'0000'0000'0000ULL, 0x0000'0000'0000'0000ULL}, // NAMED_VECTOR_PADDSUBPD_INVERT_UPPER
    {0x8000'0000'0000'0000ULL, 0x8000'0000'0000'0000ULL}, // NAMED_VECTOR_PSUBADDPS_INVERT
    {0x8000'0000'0000'0000ULL, 0x8000'0000'0000'0000ULL}, // NAMED_VECTOR_PSUBADDPS_INVERT_UPPER
    {0x0000'0000'0000'0000ULL, 0x8000'0000'0000'0000ULL}, // NAMED_VECTOR_PSUBADDPD_INVERT
    {0x0000'0000'0000'0000ULL, 0x8000'0000'0000'0000ULL}, // NAMED_VECTOR_PSUBADDPD_INVERT_UPPER
    {0x0000'0001'0000'0000ULL, 0x0000'0003'0000'0002ULL}, // NAMED_VECTOR_MOVMSKPS_SHIFT
    {0x040B'0E01'0B0E'0104ULL, 0x0C03'0609'0306'090CULL}, // NAMED_VECTOR_AESKEYGENASSIST_SWIZZLE
    {0x0706'0504'FFFF'FFFFULL, 0xFFFF'FFFF'0B0A'0908ULL}, // NAMED_VECTOR_BLENDPS_0110B
    {0x0706'0504'0302'0100ULL, 0xFFFF'FFFF'0B0A'0908ULL}, // NAMED_VECTOR_BLENDPS_0111B
    {0xFFFF'FFFF'0302'0100ULL, 0x0F0E'0D0C'FFFF'FFFFULL}, // NAMED_VECTOR_BLENDPS_1001B
    {0x0706'0504'0302'0100ULL, 0x0F0E'0D0C'FFFF'FFFFULL}, // NAMED_VECTOR_BLENDPS_1011B
    {0xFFFF'FFFF'0302'0100ULL, 0x0F0E'0D0C'0B0A'0908ULL}, // NAMED_VECTOR_BLENDPS_1101B
    {0x0706'0504'FFFF'FFFFULL, 0x0F0E'0D0C'0B0A'0908ULL}, // NAMED_VECTOR_BLENDPS_1110B
    {0x8040'2010'0804'0201ULL, 0x8040'2010'0804'0201ULL}, // NAMED_VECTOR_MOVMASKB
    {0x8040'2010'0804'0201ULL, 0x8040'2010'0804'0201ULL}, // NAMED_VECTOR_MOVMASKB_UPPER
    {0x8000'0000'0000'0000ULL, 0x0000'0000'0000'3FFFULL}, // NAMED_VECTOR_X87_ONE
    {0xD49A'784B'CD1B'8AFEULL, 0x0000'0000'0000'4000ULL}, // NAMED_VECTOR_X87_LOG2_10
    {0xB8AA'3B29'5C17'F0BCULL, 0x0000'0000'0000'3FFFULL}, // NAMED_VECTOR_X87_LOG2_E
    {0xC90F'DAA2'2168'C235ULL, 0x0000'0000'0000'4000ULL}, // NAMED_VECTOR_X87_PI
    {0x9A20'9A84'FBCF'F799ULL, 0x0000'0000'0000'3FFDULL}, // NAMED_VECTOR_X87_LOG10_2
    {0xB172'17F7'D1CF'79ACULL, 0x0000'0000'0000'3FFEULL}, // NAMED_VECTOR_X87_LOG_2
    {0x4F00'0000'4F00'0000ULL, 0x4F00'0000'4F00'0000ULL}, // NAMED_VECTOR_CVTMAX_F32_I32
    {0x4F00'0000'4F00'0000ULL, 0x4F00'0000'4F00'0000ULL}, // NAMED_VECTOR_CVTMAX_F32_I32_UPPER
    {0x5F00'0000'5F00'0000ULL, 0x5F00'0000'5F00'0000ULL}, // NAMED_VECTOR_CVTMAX_F32_I64
    {0x41E0'0000'0000'0000ULL, 0x41E0'0000'0000'0000ULL}, // NAMED_VECTOR_CVTMAX_F64_I32
    {0x41E0'0000'0000'0000ULL, 0x41E0'0000'0000'0000ULL}, // NAMED_VECTOR_CVTMAX_F64_I32_UPPER
    {0x43E0'0000'0000'0000ULL, 0x43E0'0000'0000'0000ULL}, // NAMED_VECTOR_CVTMAX_F64_I64
    {0x8000'0000'8000'0000ULL, 0x8000'0000'8000'0000ULL}, // NAMED_VECTOR_CVTMAX_I32
    {0x8000'0000'0000'0000ULL, 0x8000'0000'0000'0000ULL}, // NAMED_VECTOR_CVTMAX_I64
    {0x0000'0000'0000'0000ULL, 0x0000'0000'0000'8000ULL}, // NAMED_VECTOR_F80_SIGN_MASK
    {0x5A82'7999'5A82'7999ULL, 0x5A82'7999'5A82'7999ULL}, // NAMED_VECTOR_SHA1RNDS_K0
    {0x6ED9'EBA1'6ED9'EBA1ULL, 0x6ED9'EBA1'6ED9'EBA1ULL}, // NAMED_VECTOR_SHA1RNDS_K1
    {0x8F1B'BCDC'8F1B'BCDCULL, 0x8F1B'BCDC'8F1B'BCDCULL}, // NAMED_VECTOR_SHA1RNDS_K2
    {0xCA62'C1D6'CA62'C1D6ULL, 0xCA62'C1D6'CA62'C1D6ULL}, // NAMED_VECTOR_SHA1RNDS_K3
  };

  constexpr static auto PSHUFLW_LUT {[]() consteval {
    struct LUTType {
      uint64_t Val[2];
    };
    // Expectation for this LUT is to simulate PSHUFLW with ARM's TBL (single register) instruction
    // PSHUFLW behaviour:
    // 16-bit words in [63:48], [47:32], [31:16], [15:0] are selected using the 8-bit Index.
    // For 128-bit PSHUFLW, bits [127:64] are identity copied.
    constexpr uint64_t IdentityCopyUpper = 0x0f'0e'0d'0c'0b'0a'09'08;
    std::array<LUTType, 256> TotalLUT {};
    uint64_t WordSelection[4] = {
      0x01'00,
      0x03'02,
      0x05'04,
      0x07'06,
    };
    for (size_t i = 0; i < 256; ++i) {
      auto& LUT = TotalLUT[i];
      const auto Word0 = (i >> 0) & 0b11;
      const auto Word1 = (i >> 2) & 0b11;
      const auto Word2 = (i >> 4) & 0b11;
      const auto Word3 = (i >> 6) & 0b11;

      LUT.Val[0] = (WordSelection[Word0] << 0) | (WordSelection[Word1] << 16) | (WordSelection[Word2] << 32) | (WordSelection[Word3] << 48);

      LUT.Val[1] = IdentityCopyUpper;
    }
    return TotalLUT;
  }()};

  constexpr static auto PSHUFHW_LUT {[]() consteval {
    struct LUTType {
      uint64_t Val[2];
    };
    // Expectation for this LUT is to simulate PSHUFHW with ARM's TBL (single register) instruction
    // PSHUFHW behaviour:
    // 16-bit words in [127:112], [111:96], [95:80], [79:64] are selected using the 8-bit Index.
    // Incoming words come from bits [127:64] of the source.
    // Bits [63:0] are identity copied.
    constexpr uint64_t IdentityCopyLower = 0x07'06'05'04'03'02'01'00;
    std::array<LUTType, 256> TotalLUT {};
    uint64_t WordSelection[4] = {
      0x09'08,
      0x0b'0a,
      0x0d'0c,
      0x0f'0e,
    };
    for (size_t i = 0; i < 256; ++i) {
      auto& LUT = TotalLUT[i];
      const auto Word0 = (i >> 0) & 0b11;
      const auto Word1 = (i >> 2) & 0b11;
      const auto Word2 = (i >> 4) & 0b11;
      const auto Word3 = (i >> 6) & 0b11;

      LUT.Val[0] = IdentityCopyLower;

      LUT.Val[1] = (WordSelection[Word0] << 0) | (WordSelection[Word1] << 16) | (WordSelection[Word2] << 32) | (WordSelection[Word3] << 48);
    }
    return TotalLUT;
  }()};

  constexpr static auto PSHUFD_LUT {[]() consteval {
    struct LUTType {
      uint64_t Val[2];
    };
    // Expectation for this LUT is to simulate PSHUFD with ARM's TBL (single register) instruction
    // PSHUFD behaviour:
    // 32-bit words in [127:96], [95:64], [63:32], [31:0] are selected using the 8-bit Index.
    std::array<LUTType, 256> TotalLUT {};
    uint64_t WordSelection[4] = {
      0x03'02'01'00,
      0x07'06'05'04,
      0x0b'0a'09'08,
      0x0f'0e'0d'0c,
    };
    for (size_t i = 0; i < 256; ++i) {
      auto& LUT = TotalLUT[i];
      const auto Word0 = (i >> 0) & 0b11;
      const auto Word1 = (i >> 2) & 0b11;
      const auto Word2 = (i >> 4) & 0b11;
      const auto Word3 = (i >> 6) & 0b11;

      LUT.Val[0] = (WordSelection[Word0] << 0) | (WordSelection[Word1] << 32);

      LUT.Val[1] = (WordSelection[Word2] << 0) | (WordSelection[Word3] << 32);
    }
    return TotalLUT;
  }()};

  constexpr static auto SHUFPS_LUT {[]() consteval {
    struct LUTType {
      uint64_t Val[2];
    };
    // 32-bit words in [127:96], [95:64], [63:32], [31:0] are selected using the 8-bit Index.
    // Expectation for this LUT is to simulate SHUFPS with ARM's TBL (two register) instruction.
    // SHUFPS behaviour:
    // Two 32-bits words from each source are selected from each source in the lower and upper halves of the 128-bit destination.
    // Dest[31:0]   = Src1[<Word0>]
    // Dest[63:32]  = Src1[<Word1>]
    // Dest[95:64]  = Src2[<Word2>]
    // Dest[127:96] = Src2[<Word3>]

    std::array<LUTType, 256> TotalLUT {};
    const uint64_t WordSelectionSrc1[4] = {
      0x03'02'01'00,
      0x07'06'05'04,
      0x0b'0a'09'08,
      0x0f'0e'0d'0c,
    };

    // Src2 needs to offset each byte index by 16-bytes to pull from the second source.
    const uint64_t WordSelectionSrc2[4] = {
      0x03'02'01'00 + (0x10101010),
      0x07'06'05'04 + (0x10101010),
      0x0b'0a'09'08 + (0x10101010),
      0x0f'0e'0d'0c + (0x10101010),
    };

    for (size_t i = 0; i < 256; ++i) {
      auto& LUT = TotalLUT[i];
      const auto Word0 = (i >> 0) & 0b11;
      const auto Word1 = (i >> 2) & 0b11;
      const auto Word2 = (i >> 4) & 0b11;
      const auto Word3 = (i >> 6) & 0b11;

      LUT.Val[0] = (WordSelectionSrc1[Word0] << 0) | (WordSelectionSrc1[Word1] << 32);

      LUT.Val[1] = (WordSelectionSrc2[Word2] << 0) | (WordSelectionSrc2[Word3] << 32);
    }
    return TotalLUT;
  }()};

  constexpr static auto DPPS_MASK {[]() consteval {
    struct LUTType {
      uint32_t Val[4];
    };

    std::array<LUTType, 16> TotalLUT {};
    for (size_t i = 0; i < TotalLUT.size(); ++i) {
      auto& LUT = TotalLUT[i];
      constexpr auto GetLUT = [](size_t i, size_t Index) {
        if (i & (1U << Index)) {
          return -1U;
        }
        return 0U;
      };

      LUT.Val[0] = GetLUT(i, 0);
      LUT.Val[1] = GetLUT(i, 1);
      LUT.Val[2] = GetLUT(i, 2);
      LUT.Val[3] = GetLUT(i, 3);
    }
    return TotalLUT;
  }()};

  constexpr static auto DPPD_MASK {[]() consteval {
    struct LUTType {
      uint64_t Val[2];
    };

    std::array<LUTType, 4> TotalLUT {};
    for (size_t i = 0; i < TotalLUT.size(); ++i) {
      auto& LUT = TotalLUT[i];
      constexpr auto GetLUT = [](size_t i, size_t Index) {
        if (i & (1U << Index)) {
          return -1ULL;
        }
        return 0ULL;
      };

      LUT.Val[0] = GetLUT(i, 0);
      LUT.Val[1] = GetLUT(i, 1);
    }
    return TotalLUT;
  }()};

  constexpr static auto PBLENDW_LUT {[]() consteval {
    struct LUTType {
      uint16_t Val[8];
    };
    // 16-bit words in [127:112], [111:96], [95:80], [79:64], [63:48], [47:32], [31:16], [15:0] are selected using 8-bit swizzle.
    // Expectation for this LUT is to simulate PBLENDW with ARM's TBX (one register) instruction.
    // PBLENDW behaviour:
    // 16-bit words from the source is moved in to the destination based on the bit in the swizzle.
    // Dest[15:0]    = Swizzle[0] ? Src[15:0] : Dest[15:0]
    // Dest[31:16]   = Swizzle[1] ? Src[31:16] : Dest[31:16]
    // Dest[47:32]   = Swizzle[2] ? Src[47:32] : Dest[47:32]
    // Dest[63:48]   = Swizzle[3] ? Src[63:48] : Dest[63:48]
    // Dest[79:64]   = Swizzle[4] ? Src[79:64] : Dest[79:64]
    // Dest[95:80]   = Swizzle[5] ? Src[95:80] : Dest[95:80]
    // Dest[111:96]  = Swizzle[6] ? Src[111:96] : Dest[111:96]
    // Dest[127:112] = Swizzle[7] ? Src[127:112] : Dest[127:112]

    std::array<LUTType, 256> TotalLUT {};
    const uint16_t WordSelectionSrc[8] = {
      0x01'00, 0x03'02, 0x05'04, 0x07'06, 0x09'08, 0x0B'0A, 0x0D'0C, 0x0F'0E,
    };

    constexpr uint16_t OriginalDest = 0xFF'FF;

    for (size_t i = 0; i < 256; ++i) {
      auto& LUT = TotalLUT[i];
      for (size_t j = 0; j < 8; ++j) {
        LUT.Val[j] = ((i >> j) & 1) ? WordSelectionSrc[j] : OriginalDest;
      }
    }
    return TotalLUT;
  }()};

  CPUBackend::CPUBackend(CodeBufferManager& CodeBuffers, FEXCore::Core::InternalThreadState* ThreadState, size_t MaxCodeSize)
    : ThreadState(ThreadState)
    , MaxCodeSize(MaxCodeSize)
    , CodeBuffers(CodeBuffers) {

    auto& Common = ThreadState->CurrentFrame->Pointers.Common;

    // Initialize named vector constants.
    for (size_t i = 0; i < FEXCore::IR::NamedVectorConstant::NAMED_VECTOR_CONST_POOL_MAX; ++i) {
      Common.NamedVectorConstantPointers[i] = reinterpret_cast<uint64_t>(NamedVectorConstants[i]);
    }

    // Copy named vector constants.
    memcpy(Common.NamedVectorConstants, NamedVectorConstants, sizeof(NamedVectorConstants));

    // Initialize Indexed named vector constants.
    Common.IndexedNamedVectorConstantPointers[FEXCore::IR::IndexNamedVectorConstant::INDEXED_NAMED_VECTOR_PSHUFLW] =
      reinterpret_cast<uint64_t>(PSHUFLW_LUT.data());
    Common.IndexedNamedVectorConstantPointers[FEXCore::IR::IndexNamedVectorConstant::INDEXED_NAMED_VECTOR_PSHUFHW] =
      reinterpret_cast<uint64_t>(PSHUFHW_LUT.data());
    Common.IndexedNamedVectorConstantPointers[FEXCore::IR::IndexNamedVectorConstant::INDEXED_NAMED_VECTOR_PSHUFD] =
      reinterpret_cast<uint64_t>(PSHUFD_LUT.data());
    Common.IndexedNamedVectorConstantPointers[FEXCore::IR::IndexNamedVectorConstant::INDEXED_NAMED_VECTOR_SHUFPS] =
      reinterpret_cast<uint64_t>(SHUFPS_LUT.data());
    Common.IndexedNamedVectorConstantPointers[FEXCore::IR::IndexNamedVectorConstant::INDEXED_NAMED_VECTOR_DPPS_MASK] =
      reinterpret_cast<uint64_t>(DPPS_MASK.data());
    Common.IndexedNamedVectorConstantPointers[FEXCore::IR::IndexNamedVectorConstant::INDEXED_NAMED_VECTOR_DPPD_MASK] =
      reinterpret_cast<uint64_t>(DPPD_MASK.data());
    Common.IndexedNamedVectorConstantPointers[FEXCore::IR::IndexNamedVectorConstant::INDEXED_NAMED_VECTOR_PBLENDW] =
      reinterpret_cast<uint64_t>(PBLENDW_LUT.data());

#ifndef FEX_DISABLE_TELEMETRY
    // Fill in telemetry values
    for (size_t i = 0; i < FEXCore::Telemetry::TYPE_LAST; ++i) {
      auto& Telem = FEXCore::Telemetry::GetTelemetryValue(static_cast<FEXCore::Telemetry::TelemetryType>(i));
      Common.TelemetryValueAddresses[i] = reinterpret_cast<uint64_t>(&Telem);
    }
#endif
  }

  CPUBackend::~CPUBackend() = default;

  auto CPUBackend::GetEmptyCodeBuffer() -> CodeBuffer* {
    auto PrevCodeBuffer = CurrentCodeBuffer;

    // Resize the code buffer and reallocate our code size
    if (!CodeBuffers.Latest) {
      // Allocate initial CodeBuffer and return it
      CurrentCodeBuffer = CodeBuffers.GetCurrentCodeBuffer();
    } else {
      auto NewCodeBufferSize = CodeBuffers.GetCurrentCodeBufferSize();
      NewCodeBufferSize = std::min<size_t>(NewCodeBufferSize * 2.0, MaxCodeSize);
      CurrentCodeBuffer = CodeBuffers.AllocateNewCodeBuffer(NewCodeBufferSize);
    }

    RegisterForSignalHandler(PrevCodeBuffer);
    return CurrentCodeBuffer.get();
  }

  void CPUBackend::RegisterForSignalHandler(fextl::shared_ptr<CodeBuffer> CodeBuffer) {
    if (ThreadState->CurrentFrame->SignalHandlerRefCounter != 0) {
      // We have signal handlers that have generated code
      // This means that we can not safely clear the code at this point in time
      // Keep a reference to the old code buffer to delay deallocation
      SignalHandlerCodeBuffers.push_back(CodeBuffer);
    } else {
      SignalHandlerCodeBuffers.clear();
    }
  }

  fextl::shared_ptr<CodeBuffer> CPUBackend::GetThreadLocalCodeBuffer() const {
    return CurrentCodeBuffer;
  }

  bool CPUBackend::UsesSharedCodeBuffer() const {
    return CurrentCodeBuffer.use_count() > 1;
  }

  fextl::shared_ptr<CodeBuffer> CPUBackend::CheckCodeBufferUpdate() {
    fextl::shared_ptr<CodeBuffer> OldCodeBuffer;
    auto NewCodeBuffer = CodeBuffers.GetCurrentCodeBuffer();
    if (CurrentCodeBuffer != NewCodeBuffer) {
      RegisterForSignalHandler(CurrentCodeBuffer);
      return std::exchange(CurrentCodeBuffer, NewCodeBuffer);
    }
    return nullptr;
  }

  GuestToHostMap& GetLookupCache(const CodeBuffer& Buffer) {
    return *Buffer.LookupCache;
  }

  CodeBuffer::CodeBuffer(size_t Size)
    : Size(Size) {
    Ptr = static_cast<uint8_t*>(FEXCore::Allocator::VirtualAlloc(Size, true));
    LOGMAN_THROW_A_FMT(!!Ptr, "Couldn't allocate code buffer");

    // Protect the last page of the allocated buffer to trigger SIGSEGV on write access
    uintptr_t LastPageAddr = AlignDown(reinterpret_cast<uintptr_t>(Ptr) + Size - 1, FEXCore::Utils::FEX_PAGE_SIZE);
    if (!FEXCore::Allocator::VirtualProtect(reinterpret_cast<void*>(LastPageAddr), FEXCore::Utils::FEX_PAGE_SIZE,
                                            FEXCore::Allocator::ProtectOptions::None)) {
      LogMan::Msg::EFmt("Failed to mprotect last page of code buffer.");
    }

    LookupCache = fextl::make_unique<GuestToHostMap>();
  }

  CodeBuffer::~CodeBuffer() {
    FEXCore::Allocator::VirtualFree(Ptr, Size);
  }

  auto CodeBufferManager::AllocateNewCodeBuffer(size_t Size) -> fextl::shared_ptr<CodeBuffer> {
#ifndef _WIN32
// MDWE (Memory-Deny-Write-Execute) is a new Linux 6.3 feature.
// It's equivalent to systemd's `MemoryDenyWriteExecute` but implemented entirely in the kernel.
//
// MDWE prevents applications from creating RWX memory mappings.
// This prevents FEX from doing anything JIT related, as FEX uses RWX for JIT memory mappings.
//
// A potential workaround to make FEX work with MDWE is to call mprotect every time we need to write or modify code.
// Alternatively, FEX could use a memory mirror where one half is mapped as RW and the other is RX.
//
// Once MDWE is enabled with the prctl, the feature is sealed and it can /NOT/ be turned off.
//
// Status of MDWE is queried through prctl using `PR_GET_MDWE`:
// -1: The kernel doesn't support MDWE
// 0: MDWE is supported but disabled
// >0: MDWE is enabled, hence prohibiting RWX mappings
#ifndef PR_GET_MDWE
#define PR_GET_MDWE 66
#endif
    int MDWE = ::prctl(PR_GET_MDWE, 0, 0, 0, 0);
    if (MDWE != -1 && MDWE != 0) {
      LogMan::Msg::EFmt("MDWE was set to 0x{:x} which means FEX can't allocate executable memory", MDWE);
    }
#endif

    auto Buffer = fextl::make_shared<CodeBuffer>(Size);

    Latest = Buffer;
    LatestOffset = 0;

    OnCodeBufferAllocated(*Buffer);

    return Buffer;
  }

  fextl::shared_ptr<CodeBuffer> CodeBufferManager::GetCurrentCodeBuffer() {
    if (!Latest) {
      static constexpr size_t INITIAL_CODE_SIZE = 1024 * 1024 * 16;
      AllocateNewCodeBuffer(INITIAL_CODE_SIZE);
    }
    return Latest;
  }

  bool CPUBackend::IsAddressInCodeBuffer(uintptr_t Address) const {
    auto CheckCodeBuffer = [](CodeBuffer& Buffer, uintptr_t Address) {
      // The last page of the code buffer is protected, so we need to exclude it from the valid range
      // when checking if the address is in the code buffer.
      uintptr_t LastPageAddr = AlignDown(reinterpret_cast<uintptr_t>(Buffer.Ptr) + Buffer.Size - 1, FEXCore::Utils::FEX_PAGE_SIZE);
      return (Address >= reinterpret_cast<uintptr_t>(Buffer.Ptr) && Address < LastPageAddr);
    };

    if (CheckCodeBuffer(*CurrentCodeBuffer, Address)) {
      return true;
    }
    for (auto& Buffer : SignalHandlerCodeBuffers) {
      if (CheckCodeBuffer(*Buffer, Address)) {
        return true;
      }
    }
    return false;
  }

} // namespace CPU
} // namespace FEXCore
