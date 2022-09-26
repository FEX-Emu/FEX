#include <map>
#include <sys/auxv.h>
#include <string>
#include <unistd.h>
#include <fstream>
#include <elf.h>
#include <dlfcn.h>
#include <cstdint>
#include <sys/time.h>
#include <time.h>
#include <stdio.h>

#include <catch2/catch.hpp>

using time_type = int (*) (time_t* tloc);
time_type time_vdso = (time_type)::time;

using gettimeofday_type = int (*)(struct timeval *tv, struct timezone *tz);
gettimeofday_type gettimeofday_vdso = (gettimeofday_type)::gettimeofday;

using gettime_type = int (*)(clockid_t, struct timespec *);
gettime_type gettime_vdso = (gettime_type)::clock_gettime;

using getres_type = int (*)(clockid_t, struct timespec *);
getres_type getres_vdso = (getres_type)::clock_getres;

using getcpu_type = int (*) (uint32_t *cpu, uint32_t *node);
getcpu_type getcpu_vdso = (getcpu_type)::getcpu;

#if __SIZEOF_POINTER__ == 4
struct timespec64 {
  int64_t tv_sec;
  int64_t tv_nsec;
};

using gettime64_type = int (*)(clockid_t, struct timespec64 *);
gettime64_type gettime64_vdso = nullptr;
#endif

class VDSOParser {
#if __SIZEOF_POINTER__ == 8
  using ELFHeader = Elf64_Ehdr;
  using ELFSectionHeader = Elf64_Shdr;
  using ELFSymbol = Elf64_Sym;
#else
  using ELFHeader = Elf32_Ehdr;
  using ELFSectionHeader = Elf32_Shdr;
  using ELFSymbol = Elf32_Sym;

#endif
  public:
    VDSOParser(uint8_t *Ptr) {
      ELFHeader *Header = (ELFHeader *)Ptr;
      uint64_t SectionHeaderOffset = Header->e_shoff;
      uint16_t SectionHeaderCount = Header->e_shnum;
      ELFSectionHeader *SHdrs = (ELFSectionHeader*)(&Ptr[SectionHeaderOffset]);

      ELFSectionHeader const *DynamicSymHeader{nullptr};
      ELFSectionHeader const *DynamicStringHeader{nullptr};
      for (size_t i = 0; i < SectionHeaderCount; ++i) {
        if (SHdrs[i].sh_type == SHT_STRTAB && SHdrs[i].sh_addr) {
          DynamicStringHeader = &SHdrs[i];
        }
        if (SHdrs[i].sh_type == SHT_DYNSYM) {
          DynamicSymHeader = &SHdrs[i];
        }
      }

      size_t NumDynSymSymbols = DynamicSymHeader->sh_size / DynamicSymHeader->sh_entsize;
      const char *DynStrTab = reinterpret_cast<const char*>(&Ptr[DynamicStringHeader->sh_offset]);

      for (size_t i = 0; i < NumDynSymSymbols; ++i) {
        uint64_t offset = DynamicSymHeader->sh_offset + i * DynamicSymHeader->sh_entsize;
        ELFSymbol const *Symbol =
          reinterpret_cast<ELFSymbol const *>(&Ptr[offset]);

        char const * Name = &DynStrTab[Symbol->st_name];
        if (Symbol->st_info != 0) {
          uint8_t* SymbolPtr = Symbol->st_value + Ptr;
          VDSOSymbols[Name] = SymbolPtr;
          printf("Found VDSO symbol '%s' at %p\n", Name, SymbolPtr);
        }

      }
    }

    uint8_t *GetVDSOSymbol(const char *String) {
      auto it = VDSOSymbols.find(String);
      if (it != VDSOSymbols.end()) {
        return it->second;
      }

      return nullptr;
    }

    std::map<std::string, uint8_t*> VDSOSymbols;
};

static void LoadVDSO() {
  uint64_t Begin = ::getauxval(AT_SYSINFO_EHDR);
  if (!Begin) {
    printf("No VDSO\n");
    return;
  }

  VDSOParser VDSO((uint8_t*)Begin);
  auto it = VDSO.GetVDSOSymbol("__vdso_time");
  if (it) {
    time_vdso = reinterpret_cast<time_type>(it);
  }

  it = VDSO.GetVDSOSymbol("__vdso_gettimeofday");
  if (it) {
    gettimeofday_vdso= reinterpret_cast<gettimeofday_type>(it);
  }

  it = VDSO.GetVDSOSymbol("__vdso_clock_gettime");
  if (it) {
    gettime_vdso = reinterpret_cast<gettime_type>(it);
  }

  it = VDSO.GetVDSOSymbol("__vdso_clock_getres");
  if (it) {
    getres_vdso = reinterpret_cast<getres_type>(it);
  }

  it = VDSO.GetVDSOSymbol("__vdso_getcpu");
  if (it) {
    getcpu_vdso = reinterpret_cast<getcpu_type>(it);
  }

#if __SIZEOF_POINTER__ == 4
  it = VDSO.GetVDSOSymbol("__vdso_clock_gettime64");
  if (it) {
    gettime64_vdso = reinterpret_cast<gettime64_type>(it);
  }
#endif
}


TEST_CASE("VDSO") {
  LoadVDSO();
  REQUIRE(time_vdso != 0);
  REQUIRE(gettimeofday_vdso != 0);
  REQUIRE(gettime_vdso != 0);
  REQUIRE(getres_vdso != 0);
  REQUIRE(getcpu_vdso != 0);

  // There are few strict guarantees on the return values of these functions,
  // so instead we make some educated guesses to check for valid outputs below

  time_t tloc{};
  {
    int result = time_vdso(&tloc);
    printf("time\n");
    CHECK(result != -1);
    printf("\tResult: %d\n", result);
    printf("\tTime_t: 0x%lx\n", tloc);
    CHECK(tloc > 946684800); // Ensure it's later than year 2000
  }

  {
    timeval tv{};
    int result = gettimeofday_vdso(&tv, nullptr);
    printf("gettimeofday\n");
    CHECK(result == 0);
    printf("\tTime: 0x%lx 0x%lx\n", tv.tv_sec, tv.tv_usec);
    // Ensure gettimeofday and time results are consistent
    CHECK(tv.tv_sec >= tloc);
    CHECK(tv.tv_sec <= tloc + 1);
  }

  {
    timespec ts{};
    int result = gettime_vdso(CLOCK_MONOTONIC, &ts);
    printf("clock_gettime\n");
    CHECK(result == 0);
    printf("\tTime: 0x%lx 0x%lx\n", ts.tv_sec, ts.tv_nsec);
  }

  {
    timespec ts{};
    int result = getres_vdso(CLOCK_MONOTONIC, &ts);
    printf("clock_getres\n");
    CHECK(result == 0);
    printf("\tTime: 0x%lx 0x%lx\n", ts.tv_sec, ts.tv_nsec);
    CHECK(ts.tv_sec == 0);
    CHECK(ts.tv_nsec > 0);
  }

  {
    uint32_t cpu, node;
    int result = getcpu_vdso(&cpu, &node);
    printf("getcpu\n");
    CHECK(result == 0);
    printf("\tCPU: 0x%x\n", cpu);
    printf("\tNode: 0x%x\n", node);
  }

#if __SIZEOF_POINTER__ == 4
  if (gettime64_vdso) {
    timespec64 ts{};
    int result = gettime64_vdso(CLOCK_MONOTONIC, &ts);
    printf("clock_gettime64\n");
    CHECK(result == 0);
    printf("\tTime: 0x%llx 0x%llx\n", ts.tv_sec, ts.tv_nsec);
  }
#endif
}
