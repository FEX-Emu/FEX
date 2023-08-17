#pragma once

#include <FEXCore/Utils/CompilerDefs.h>

#include <cstdint>

namespace FEXCore::CodeSerialize {
  // If any of the config options mismatch on load then the cache won't be used
  // Any of these will result in codegen changes
  struct
  FEX_PACKED
  CodeObjectSerializationConfig {
    // Cookie in the header of the file, isn't part of the config hash
    uint64_t Cookie{};

    // Instructions per block configuration
    int32_t MaxInstPerBlock{};

    // Follows CPUID 4000_0001_EAX[3:0]
    unsigned Arch : 4;

    // Multiblock enabled
    unsigned MultiBlock : 1;

    // Hardware TSO enabled
    unsigned HardwareTSOEnabled : 1;

    // TSO enabled
    unsigned TSOEnabled : 1;

    // ABI local flag unsafe optimization
    unsigned ABILocalFlags : 1;

    // ABI no PF unsafe optimization
    unsigned ABINoPF : 1;

    // Static register allocation enabled
    unsigned SRA : 1;

    // Paranoid TSO mode enabled
    unsigned ParanoidTSO : 1;

    // Guest code execution mode (We don't support live mode switch)
    unsigned Is64BitMode : 1;

    // SMC checks style
    unsigned SMCChecks : 2;

    // x87 reduced precision
    unsigned x87ReducedPrecision : 1;

    // Padding to remove uninitialized data warning from asan
    // Shows remaining amount of bits available for config
    unsigned _Pad : 17;

    bool operator==(CodeObjectSerializationConfig const &other) const {
      return Cookie == other.Cookie &&
        MaxInstPerBlock == other.MaxInstPerBlock &&
        Arch == other.Arch &&
        MultiBlock == other.MultiBlock &&
        HardwareTSOEnabled == other.HardwareTSOEnabled &&
        TSOEnabled == other.TSOEnabled &&
        ABILocalFlags == other.ABILocalFlags &&
        ABINoPF == other.ABINoPF &&
        SRA == other.SRA &&
        ParanoidTSO == other.ParanoidTSO &&
        Is64BitMode == other.Is64BitMode &&
        SMCChecks == other.SMCChecks &&
        x87ReducedPrecision == other.x87ReducedPrecision;
    }
    static uint64_t GetHash(CodeObjectSerializationConfig const &other) {
      // For < 64-bits of data just pack directly
      // Skip the cookie
      uint64_t Hash{};
      Hash <<= 32; Hash |= other.MaxInstPerBlock;
      Hash <<= 1;  Hash |= other.Arch;
      Hash <<= 1;  Hash |= other.MultiBlock;
      Hash <<= 1;  Hash |= other.HardwareTSOEnabled;
      Hash <<= 1;  Hash |= other.TSOEnabled;
      Hash <<= 1;  Hash |= other.ABILocalFlags;
      Hash <<= 1;  Hash |= other.ABINoPF;
      Hash <<= 1;  Hash |= other.SRA;
      Hash <<= 1;  Hash |= other.ParanoidTSO;
      Hash <<= 1;  Hash |= other.Is64BitMode;
      Hash <<= 2;  Hash |= other.SMCChecks;
      Hash <<= 1;  Hash |= other.x87ReducedPrecision;
      return Hash;
    }
  };

  static_assert(sizeof(CodeObjectSerializationConfig) == 16, "Size changed");
  static_assert((sizeof(CodeObjectSerializationConfig) - sizeof(uint64_t)) == 8, "Config size exceeded 64its. Need to change how the hash is generated!");
}
