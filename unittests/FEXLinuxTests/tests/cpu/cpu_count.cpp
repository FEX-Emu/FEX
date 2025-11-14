#include <catch2/catch_test_macros.hpp>

#include <cpuid.h>
#include <optional>
#include <unistd.h>
#include <thread>

TEST_CASE("cpu count - libraries") {
  const auto hw_concurrency = std::thread::hardware_concurrency();
  CHECK(hw_concurrency == sysconf(_SC_NPROCESSORS_CONF));
  CHECK(hw_concurrency == sysconf(_SC_NPROCESSORS_ONLN));
}

struct core_info {
  uint32_t local_apicid;
  uint32_t max_addressible_ids;
  uint32_t cores;
  uint32_t threads;
  bool HTT;
};

struct cpuid_fn {
  uint32_t eax, ebx, ecx, edx;
};

cpuid_fn get_cpuid(uint32_t func, uint32_t leaf = 0) {
  cpuid_fn fn {};
  __cpuid_count(func, leaf, fn.eax, fn.ebx, fn.ecx, fn.edx);
  return fn;
}

std::optional<core_info> cpuid_calculate_core_info() {
  core_info info {};

  // Legacy path
  const auto cpuid_fn_0 = get_cpuid(0);
  if (cpuid_fn_0.eax < 1) {
    return std::nullopt;
  }

  const auto cpuid_fn_1 = get_cpuid(1);

  info.local_apicid = cpuid_fn_1.ebx >> 24;
  info.HTT = (cpuid_fn_1.edx >> 28) & 1;

  const auto cpuid_fn_8000_0000 = get_cpuid(0x8000'0000U);

  if (cpuid_fn_8000_0000.eax < 0x8000'0008) {
    return std::nullopt;
  }

  const auto cpuid_fn_8000_0008 = get_cpuid(0x8000'0008U);
  const uint32_t apic_id_size = (cpuid_fn_8000_0008.ecx >> 12) & 0xF;

  // E.5.2: MNLP (Maximum Number of Logical Processors)
  uint32_t MNLP {};

  if (apic_id_size) {
    // Extended topology.
    MNLP = 1 << apic_id_size;
  } else {
    // Legacy path.
    MNLP = (cpuid_fn_8000_0008.ecx & 0xF) + 1;
  }

  info.max_addressible_ids = MNLP;

  const auto cpuid_fn_4 = get_cpuid(4);
  if (cpuid_fn_4.eax & 0xF) {
    // Intel exclusive cpuid function, AMD returns zero as unsupported.
    // `Maximum number of addressable IDs for processor cores in the physical package`
    info.cores = (cpuid_fn_4.eax >> 26) + 1;
    if (info.HTT) {
      // `A value of 1 for HTT indicates the value in CPUID.1.EBX[23:16] (the Maximum number of addressable IDs for logical processors in
      // this package) is valid for the package. `Maximum number of addressable IDs for logical processors in this physical package`
      info.threads = (cpuid_fn_1.ebx >> 16) & 0xFF;
    } else {
      info.threads = info.cores;
    }
  } else if (info.HTT) {
    info.cores = (cpuid_fn_1.ebx >> 16) & 0xFF;
    info.threads = info.cores * 2;
  } else {
    // Legacy path means cores/threads is equal to MNLP.
    info.cores = info.threads = MNLP;
  }

  return info;
}

TEST_CASE("cpu count - cpuid") {
  const auto hw_concurrency = std::thread::hardware_concurrency();
  const auto core_info = cpuid_calculate_core_info();
  REQUIRE(core_info.has_value());
  CHECK(core_info->local_apicid < hw_concurrency);
  CHECK(core_info->local_apicid < core_info->max_addressible_ids);
  CHECK(core_info->max_addressible_ids >= hw_concurrency);
  if (core_info->HTT) {
    // May not be entirely correct on systems that mix HTT and non-HTT cpu cores.
    CHECK((core_info->cores * 2) == core_info->threads);
  } else {
    CHECK(core_info->cores == core_info->threads);
  }
}
