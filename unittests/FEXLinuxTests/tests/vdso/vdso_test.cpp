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
}


int main() {
  LoadVDSO();
  printf("VDSO funcs:\n");
  printf("\ttime: %p\n", time_vdso);
  printf("\tgettimeofday: %p\n", gettimeofday_vdso);
  printf("\tgettime: %p\n", gettime_vdso);
  printf("\tgetres: %p\n", getres_vdso);
  printf("\tgetcpu: %p\n", getcpu_vdso);

  int result{};

  time_t tloc{};
  result = time_vdso(&tloc);
  printf("time\n");
  printf("\tResult: %d\n", result);
  printf("\tTime_t: 0x%lx\n", tloc);

  timeval tv{};
  result = gettimeofday_vdso(&tv, nullptr);
  printf("gettimeofday\n");
  printf("\tResult: %d\n", result);
  printf("\tTime: 0x%lx 0x%lx\n", tv.tv_sec, tv.tv_usec);

  timespec ts{};
  result = gettime_vdso(CLOCK_MONOTONIC, &ts);
  printf("clock_gettime\n");
  printf("\tResult: %d\n", result);
  printf("\tTime: 0x%lx 0x%lx\n", ts.tv_sec, ts.tv_nsec);

  result = getres_vdso(CLOCK_MONOTONIC, &ts);
  printf("clock_getres\n");
  printf("\tResult: %d\n", result);
  printf("\tTime: 0x%lx 0x%lx\n", ts.tv_sec, ts.tv_nsec);

  uint32_t cpu, node;
  result = getcpu_vdso(&cpu, &node);
  printf("getcpu\n");
  printf("\tResult: %d\n", result);
  printf("\tCPU: 0x%x\n", cpu);
  printf("\tNode: 0x%x\n", node);
  return 0;
}
