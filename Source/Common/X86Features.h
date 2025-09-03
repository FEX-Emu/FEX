#pragma once
#include <cstdint>

#ifdef _M_X86_64
#include <cpuid.h>

namespace FEX::X86 {
class Features final {
public:
  Features() {
    cpuid_data data {};
    data = cpuid(0);

    if (data.eax >= 1) {
      auto data_1 = cpuid(0x1);

      Feat_aes = data_1.ecx & (1U << 25);
      Feat_crc = data_1.ecx & (1U << 20);
      Feat_rand = data_1.ecx & (1U << 30);
      Feat_pclmulqdq = data_1.ecx & (1U << 1);
      Feat_avx = data_1.ecx & (1U << 28);
      Feat_ssse3 = data_1.ecx & (1U << 9);
      Feat_sse4_1 = data_1.ecx & (1U << 19);
      Feat_sse4_2 = data_1.ecx & (1U << 20);
      Feat_movbe = data_1.ecx & (1U << 22);
      Feat_xsave = data_1.ecx & (1U << 26);
    }

    if (data.eax >= 7) {
      auto data_7 = cpuid(0x7);
      Feat_fsgsbase = data_7.ebx & (1U << 0);
      Feat_bmi1 = data_7.ebx & (1U << 3);
      Feat_avx &= data_7.ebx & (1U << 5);
      Feat_bmi2 = data_7.ebx & (1U << 8);
      Feat_clwb = data_7.ebx & (1U << 24);
      Feat_rand &= data_7.ebx & (1U << 18);
      Feat_adx = data_7.ebx & (1U << 19);
      Feat_clflopt = data_7.ebx & (1U << 23);
      Feat_sha = data_7.ebx & (1U << 29);
      Feat_vaes = data_7.ecx & (1U << 9);
      Feat_pclmulqdq &= data_7.ecx & (1U << 10);
      Feat_rdpid = data_7.ecx & (1U << 22);
    }

    data = cpuid(0x8000'0000U);
    if (data.eax >= 0x8000'0001U) {
      auto data_8000_0001 = cpuid(0x8000'0001U);
      Feat_3dnow = (data_8000_0001.edx >> 30) == 0b11;
      Feat_sse4a = data_8000_0001.ecx & (1U << 6);
    }

    if (data.eax >= 0x8000'0008U) {
      auto data_8000_0008 = cpuid(0x8000'0008U);

      Feat_clzero = data_8000_0008.ebx & 1;
    }
  }

  // Features.
  bool Feat_3dnow {};
  bool Feat_sse4a {};
  bool Feat_bmi1 {};
  bool Feat_bmi2 {};
  bool Feat_clwb {};
  bool Feat_aes {};
  bool Feat_crc {};
  bool Feat_rand {};
  bool Feat_sha {};
  bool Feat_pclmulqdq {};
  bool Feat_vaes {};
  bool Feat_clzero {};
  bool Feat_avx {};
  bool Feat_ssse3 {};
  bool Feat_sse4_1 {};
  bool Feat_sse4_2 {};
  bool Feat_movbe {};
  bool Feat_adx {};
  bool Feat_xsave {};
  bool Feat_rdpid {};
  bool Feat_clflopt {};
  bool Feat_fsgsbase {};

private:
  struct cpuid_data {
    uint32_t eax, ebx, ecx, edx;
  };

  cpuid_data cpuid(uint32_t Function, uint32_t Leaf = 0) {
    cpuid_data data;
    __cpuid_count(Function, Leaf, data.eax, data.ebx, data.ecx, data.edx);
    return data;
  }
};
} // namespace FEX::X86
#endif
