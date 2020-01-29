#include "LogManager.h"

#include "Interface/Context/Context.h"
#include "Interface/HLE/FileManagement.h"
#include "Interface/HLE/EmulatedFiles/EmulatedFiles.h"

namespace FEXCore::EmulatedFile {
  class SimpleStringFD final : public FEXCore::FD {
    public:
      SimpleStringFD(FEXCore::Context::Context *ctx, int32_t fd, const char *pathname, int32_t flags, mode_t mode, std::string_view Str)
        : FD {ctx, fd, pathname, flags, mode}
        , String {Str} {
      }

      uint64_t read(int fd, void *buf, size_t count) {
        uint64_t SizeToRead = std::min(count, String.size() - ReadOffset);
        if (SizeToRead) {
          memcpy(buf, &String.at(ReadOffset), SizeToRead);
          ReadOffset += SizeToRead;
        }
        return SizeToRead;
      }

      ssize_t writev(int fd, void *iov, int iovcnt) {
        LogMan::Msg::A("Unimplemented");
        return 0;
      }
      uint64_t write(int fd, void *buf, size_t count) {
        LogMan::Msg::A("Unimplemented");
        return 0;
      }

    private:
      std::string_view String;
      uint64_t ReadOffset{};
  };

  static const std::string proc_cpuinfo = R"(
processor       : 0
vendor_id       : AuthenticAMD
cpu family      : 23
model           : 0
model name      : AMD Generic Emulation
stepping        : 0
microcode       : 0x0
cpu MHz         : 3000
cache size      : 512 KB
physical id     : 0
siblings        : 0
core id         : 0
cpu cores       : 1
apicid          : 0
initial apicid  : 0
fpu             : yes
fpu_exception   : yes
cpuid level     : 13
wp              : yes
flags           : fpu vme de pse tsc msr pae mce cx8 apic sep mtrr pge mca cmov pat pse36 clflush mmx fxsr sse sse2 ht syscall nx mmxext fxsr_opt pdpe1gb rdtscp lm constant_tsc rep_good nopl nonstop_tsc cpuid extd_apicid amd_dcm aperfmperf pni pclmulqdq monitor ssse3 fma cx16 sse4_1 sse4_2 movbe popcnt aes xsave avx f16c rdrand lahf_lm cmp_legacy svm extapic cr8_legacy abm sse4a m isalignsse 3dnowprefetch osvw skinit wdt tce topoext perfctr_core perfctr_nb bpext perfctr_llc mwaitx cpb hw_pstate sme ssbd sev ibpb vmmcall fsgsbase bmi1 avx2 smep bmi2 rdseed adx smap clflushopt sha_ni xsaveopt xsavec xgetbv1 xsaves clzero irperf xsaveerptr arat npt lbrv svm_lock nrip_save tsc_scale vmcb_clean flushbyasid decodeassists pausefilter pfthreshold avic v_vmsave_vmlo ad vgif overflow_recov succor smca
bugs            : sysret_ss_attrs null_seg spectre_v1 spectre_v2 spec_store_bypass
bogomips        : 8000.0
TLB size        : 2560 4K pages
clflush size    : 64
cache_alignment : 64
address sizes   : 43 bits physical, 48 bits virtual
)";

  static const std::string cpus_online = R"(
0
)";

  EmulatedFDManager::EmulatedFDManager(FEXCore::Context::Context *ctx)
    : CTX {ctx} {
    EmulatedMap.emplace("/proc/cpuinfo");
    EmulatedMap.emplace("/sys/devices/system/cpu/online");

    FDReadCreators["/proc/cpuinfo"] = [&](FEXCore::Context::Context *ctx, int32_t fd, const char *pathname, int32_t flags, mode_t mode) -> FEXCore::FD* {
      return new SimpleStringFD(ctx, fd, pathname, flags, mode, proc_cpuinfo);
    };
    FDReadCreators["/sys/devices/system/cpu/online"] = [&](FEXCore::Context::Context *ctx, int32_t fd, const char *pathname, int32_t flags, mode_t mode) -> FEXCore::FD* {
      return new SimpleStringFD(ctx, fd, pathname, flags, mode, cpus_online);
    };
  }

  EmulatedFDManager::~EmulatedFDManager() {
  }

  FEXCore::FD *EmulatedFDManager::OpenAt(int dirfs, const char *pathname, int flags, uint32_t mode) {
    if (EmulatedMap.find(pathname) == EmulatedMap.end()) {
      return nullptr;
    }

    return FDReadCreators[pathname](CTX, dirfs, pathname, flags, mode);
  }
}

