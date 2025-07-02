// SPDX-License-Identifier: MIT
/*
$info$
tags: LinuxSyscalls|common
desc: Emulated /proc/cpuinfo, version, osrelease, etc
$end_info$
*/

#include "CodeLoader.h"

#include "Common/CPUInfo.h"
#include "Common/FDUtils.h"
#include "LinuxSyscalls/Syscalls.h"
#include "LinuxSyscalls/EmulatedFiles/EmulatedFiles.h"

#include <FEXCore/Config/Config.h>
#include <FEXCore/Core/Context.h>
#include <FEXCore/Core/CPUID.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/fextl/fmt.h>
#include <FEXCore/fextl/string.h>
#include <FEXCore/fextl/sstream.h>
#include <FEXHeaderUtils/Filesystem.h>

#include <git_version.h>

#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <ostream>
#include <stdio.h>
#include <system_error>
#include <unistd.h>
#include <utility>

namespace FEX::EmulatedFile {
/**
 * @brief Generates a temporary file using raw FDs
 *
 * Since we are hooking syscalls that are expecting to use raw FDs, we need to make sure to also use raw FDs.
 * The guest application can leave these FDs dangling.
 *
 * Using glibc tmpfile creates a FILE which glibc tracks and will try cleaning up on application exit.
 * If we are running a 32-bit application then this dangling FILE will be allocated using the FEX allcator
 * Which will have already been cleaned up on shutdown.
 *
 * Dangling raw FD is safe since if the guest doesn't close them, then the kernel cleans them up on application close.
 *
 * @return A temporary file that we can use
 */
static int GenTmpFD(const char* pathname, int flags) {
  uint32_t memfd_flags {MFD_ALLOW_SEALING};
  if (flags & O_CLOEXEC) {
    memfd_flags |= MFD_CLOEXEC;
  }

  return memfd_create(pathname, memfd_flags);
}

// Seal the tmpfd features by sealing them all.
// Makes the tmpfd read-only.
static void SealTmpFD(int fd) {
  int ret = fcntl(fd, F_ADD_SEALS, F_SEAL_SEAL | F_SEAL_SHRINK | F_SEAL_GROW | F_SEAL_WRITE | F_SEAL_FUTURE_WRITE);
  if (ret == -1) [[unlikely]] {
    // This shouldn't ever happen, but also isn't fatal.
    LogMan::Msg::EFmt("Couldn't seal tmpfd! {}", errno);
  }
}

fextl::string GenerateCPUInfo(FEXCore::Context::Context* ctx, uint32_t CPUCores) {
  fextl::ostringstream cpu_stream {};
  auto res_0 = ctx->RunCPUIDFunction(0, 0);
  auto res_1 = ctx->RunCPUIDFunction(1, 0);
  auto res_6 = ctx->RunCPUIDFunction(6, 0);
  auto res_7 = ctx->RunCPUIDFunction(7, 0);
  auto res_7_1 = ctx->RunCPUIDFunction(7, 1);
  auto res_d_1 = ctx->RunCPUIDFunction(0xD, 1);
  auto res_10 = ctx->RunCPUIDFunction(0x10, 0);

  auto res_8000_0001 = ctx->RunCPUIDFunction(0x8000'0001, 0);
  auto res_8000_0007 = ctx->RunCPUIDFunction(0x8000'0007, 0);
  auto res_8000_0008 = ctx->RunCPUIDFunction(0x8000'0008, 0);
  auto res_8000_000a = ctx->RunCPUIDFunction(0x8000'000a, 0);
  auto res_8000_001f = ctx->RunCPUIDFunction(0x8000'001f, 0);

  union VendorID {
    struct {
      uint32_t id;
      char Str[13];
    };
    struct {
      FEXCore::CPUID::FunctionResults cpuid;
      uint8_t null;
    };
  };

  union ModelName {
    struct {
      char Str[49];
    };
    struct {
      FEXCore::CPUID::FunctionResults cpuid_2;
      FEXCore::CPUID::FunctionResults cpuid_3;
      FEXCore::CPUID::FunctionResults cpuid_4;
      uint8_t null;
    };
  };

  union Info {
    FEXCore::CPUID::FunctionResults cpuid;
    struct {
      unsigned Stepping   : 4;
      unsigned Model      : 4;
      unsigned FamilyID   : 4;
      unsigned Type       : 4;
      unsigned ExModelID  : 4;
      unsigned ExFamilyID : 8;
      unsigned            : 4;
    };
  };

  VendorID vendorid {};
  vendorid.cpuid = {res_0.eax, res_0.ebx, res_0.edx, res_0.ecx};
  vendorid.null = 0;

  Info info {res_1};

  uint32_t Family = info.FamilyID + (info.FamilyID == 0xF ? info.ExFamilyID : 0);
  fextl::ostringstream flags_data {};
  // Generate the flags data up front
  // This is the same per core
  {
    auto add_flag_if = [&flags_data](bool flag, const char* name) {
      if (flag) {
        flags_data << name << " ";
      }
    };

    add_flag_if(res_1.edx & (1 << 0), "fpu");
    add_flag_if(res_1.edx & (1 << 1), "vme");
    add_flag_if(res_1.edx & (1 << 2), "de");
    add_flag_if(res_1.edx & (1 << 3), "pse");
    add_flag_if(res_1.edx & (1 << 4), "tsc");
    add_flag_if(res_1.edx & (1 << 5), "msr");
    add_flag_if(res_1.edx & (1 << 6), "pae");
    add_flag_if(res_1.edx & (1 << 7), "mce");
    add_flag_if(res_1.edx & (1 << 8), "cx8");
    add_flag_if(res_1.edx & (1 << 9), "apic");
    add_flag_if(res_1.edx & (1 << 11), "sep");
    add_flag_if(res_1.edx & (1 << 12), "mtrr");
    add_flag_if(res_1.edx & (1 << 13), "pge");
    add_flag_if(res_1.edx & (1 << 14), "mca");
    add_flag_if(res_1.edx & (1 << 15), "cmov");
    add_flag_if(res_1.edx & (1 << 16), "pat");
    add_flag_if(res_1.edx & (1 << 17), "pse36");
    add_flag_if(res_1.edx & (1 << 18), "pn");
    add_flag_if(res_1.edx & (1 << 19), "clflush");
    add_flag_if(res_1.edx & (1 << 21), "ds");   // XXX
    add_flag_if(res_1.edx & (1 << 22), "acpi"); // XXX
    add_flag_if(res_1.edx & (1 << 23), "mmx");
    add_flag_if(res_1.edx & (1 << 24), "fxsr");
    add_flag_if(res_1.edx & (1 << 25), "sse");
    add_flag_if(res_1.edx & (1 << 26), "sse2");
    add_flag_if(res_1.edx & (1 << 27), "ss");
    add_flag_if(res_1.edx & (1 << 28), "ht");
    add_flag_if(res_1.edx & (1 << 29), "tm");
    add_flag_if(res_1.edx & (1 << 30), "ia64");
    add_flag_if(res_1.edx & (1 << 31), "pbe");

    add_flag_if(res_8000_0001.edx & (1 << 11), "syscall");
    add_flag_if(res_8000_0001.edx & (1 << 19), "mp");
    add_flag_if(res_8000_0001.edx & (1 << 20), "nx");
    add_flag_if(res_8000_0001.edx & (1 << 22), "mmxext");
    add_flag_if(res_8000_0001.edx & (1 << 25), "fxsr_opt");
    add_flag_if(res_8000_0001.edx & (1 << 26), "pdpe1gb");
    add_flag_if(res_8000_0001.edx & (1 << 27), "rdtscp");
    add_flag_if(res_8000_0001.edx & (1 << 29), "lm");
    add_flag_if(res_8000_0001.edx & (1 << 31), "3dnow");
    add_flag_if(res_8000_0001.edx & (1 << 30), "3dnowext");

    add_flag_if(res_8000_0007.edx & (1 << 8), "constant_tsc");

    // We are not a uniprocessor running in SMP mode
    add_flag_if(false, "up");
    // Timer is always running
    add_flag_if(true, "art");
    // No Intel perfmon
    add_flag_if(false, "arch_perfmon");
    // No precise event based sampling
    add_flag_if(false, "pebs");
    // No branch trace store
    add_flag_if(false, "bts");

    add_flag_if(true, "rep_good");
    add_flag_if(res_8000_0007.edx & (1 << 12), "tm");

    // Always support long nop
    add_flag_if(true, "nopl");

    // Always expose topology information
    add_flag_if(true, "xtoplogy");

    // Atom/geode only?
    add_flag_if(false, "tsc_reliable");
    add_flag_if(res_8000_0007.edx & (1 << 8), "nonstop_tsc");

    // We always support CPUID
    add_flag_if(true, "cpuid");
    add_flag_if(Family > 0x16, "extd_apicid");
    add_flag_if(false, "amd_dcm"); // Never claim to be a multi node processor
    add_flag_if(res_8000_0007.edx & (1 << 11), "aperfmperf");

    // Need to check ARM documentation if we can support this?
    add_flag_if(false, "nonstop_tsc_s3");

    // We can calculate this flag on AArch64
    add_flag_if(true, "tsc_known_freq");

    add_flag_if(res_1.ecx & (1 << 0), "pni");
    add_flag_if(res_1.ecx & (1 << 1), "pclmulqdq");
    add_flag_if(res_1.ecx & (1 << 2), "dtes64");
    add_flag_if(res_1.ecx & (1 << 3), "monitor");
    add_flag_if(res_1.ecx & (1 << 4), "ds_cpl");
    add_flag_if(res_1.ecx & (1 << 5), "vmx");
    add_flag_if(res_1.ecx & (1 << 6), "smx");
    add_flag_if(res_1.ecx & (1 << 7), "est");
    add_flag_if(res_1.ecx & (1 << 8), "tm2");
    add_flag_if(res_1.ecx & (1 << 9), "ssse3");
    add_flag_if(res_1.ecx & (1 << 11), "sdbg");
    add_flag_if(res_1.ecx & (1 << 12), "fma");
    add_flag_if(res_1.ecx & (1 << 13), "cx16");
    add_flag_if(res_1.ecx & (1 << 14), "xptr");
    add_flag_if(res_1.ecx & (1 << 15), "pdcm");
    add_flag_if(res_1.ecx & (1 << 17), "pcid");
    add_flag_if(res_1.ecx & (1 << 18), "dca");
    add_flag_if(res_1.ecx & (1 << 19), "sse4_1");
    add_flag_if(res_1.ecx & (1 << 20), "sse4_2");
    add_flag_if(res_1.ecx & (1 << 21), "x2apic");
    add_flag_if(res_1.ecx & (1 << 22), "movbe");
    add_flag_if(res_1.ecx & (1 << 23), "popcnt");
    add_flag_if(res_1.ecx & (1 << 24), "tsc_deadline_timer");
    add_flag_if(res_1.ecx & (1 << 25), "aes");
    add_flag_if(res_1.ecx & (1 << 26), "xsave");
    add_flag_if(res_1.ecx & (1 << 28), "avx");
    add_flag_if(res_1.ecx & (1 << 29), "f16c");
    add_flag_if(res_1.ecx & (1 << 30), "rdrand");
    add_flag_if(res_1.ecx & (1 << 31), "hypervisor");

    add_flag_if(res_8000_0001.ecx & (1 << 0), "lahf_lm");
    add_flag_if(res_8000_0001.ecx & (1 << 1), "cmp_legacy");
    add_flag_if(res_8000_0001.ecx & (1 << 2), "svm");
    add_flag_if(res_8000_0001.ecx & (1 << 3), "extapic");
    add_flag_if(res_8000_0001.ecx & (1 << 4), "cr8_legacy");
    add_flag_if(res_8000_0001.ecx & (1 << 5), "abm");
    add_flag_if(res_8000_0001.ecx & (1 << 6), "sse4a");
    add_flag_if(res_8000_0001.ecx & (1 << 7), "misalignsse");
    add_flag_if(res_8000_0001.ecx & (1 << 8), "3dnowprefetch");
    add_flag_if(res_8000_0001.ecx & (1 << 9), "osvw");
    add_flag_if(res_8000_0001.ecx & (1 << 10), "ibs");
    add_flag_if(res_8000_0001.ecx & (1 << 11), "xop");
    add_flag_if(res_8000_0001.ecx & (1 << 12), "skinit");
    add_flag_if(res_8000_0001.ecx & (1 << 13), "wdt");
    add_flag_if(res_8000_0001.ecx & (1 << 15), "lwp");
    add_flag_if(res_8000_0001.ecx & (1 << 16), "fma4");
    add_flag_if(res_8000_0001.ecx & (1 << 17), "tce");
    add_flag_if(res_8000_0001.ecx & (1 << 19), "nodeid_msr");
    add_flag_if(res_8000_0001.ecx & (1 << 21), "tbm");
    add_flag_if(res_8000_0001.ecx & (1 << 22), "topoext");
    add_flag_if(res_8000_0001.ecx & (1 << 23), "perfctr_core");
    add_flag_if(res_8000_0001.ecx & (1 << 24), "perfctr_nb");
    add_flag_if(res_8000_0001.ecx & (1 << 26), "bpext");
    add_flag_if(res_8000_0001.ecx & (1 << 27), "ptsc");

    add_flag_if(res_8000_0001.ecx & (1 << 28), "perfctr_llc");
    add_flag_if(res_8000_0001.ecx & (1 << 29), "mwaitx");

    // We don't support ring 3 supporting mwait
    add_flag_if(false, "ring3mwait");
    // We don't support Intel CPUID fault support
    add_flag_if(false, "cpuid_fault");
    add_flag_if(res_8000_0007.edx & (1 << 9), "cpb");
    add_flag_if(res_6.ecx & (1 << 3), "epb");
    add_flag_if(res_10.ebx & (1 << 1), "cat_l3");
    add_flag_if(res_10.ebx & (1 << 2), "cat_l2");
    add_flag_if(false, "invpcid_single");
    add_flag_if(res_8000_0007.edx & (1 << 7), "hw_pstate");
    add_flag_if(res_8000_001f.eax & (1 << 0), "sme");

    // Kernel page table isolation.
    add_flag_if(false, "pti");

    // We don't support Intel's Protected Processor Inventory Number
    add_flag_if(false, "intel_ppin");

    add_flag_if(res_8000_0008.ebx & (1 << 6), "mba");
    add_flag_if(res_8000_001f.eax & (1 << 1), "sev");

    { // Speculative bug workarounds
      // We don't claim to have these bugs, so we don't need to claim these flags
      add_flag_if(res_7.edx & (1 << 31), "ssbd");
      add_flag_if(false, "ibrs");
      add_flag_if(false, "ibpb");

      add_flag_if(res_7.edx & (1 << 27), "stibp");

      add_flag_if(false, "ibrs_enhanced");
    }

    // We don't support Intel's TPR Shadow feature
    add_flag_if(false, "tpr_shadow");
    // Intel virtual NMI
    add_flag_if(false, "vnmi");
    // Intel FlexPriority
    add_flag_if(false, "flexpriority");
    // Intel Extended page table
    add_flag_if(false, "ept");
    // Intel virtual processor ID
    add_flag_if(false, "vpid");

    // Prefer VMMCall to VMCall
    add_flag_if(false, "vmmcall");
    // Intel extended page table access dirty bit
    add_flag_if(false, "ept_ad");
    add_flag_if(res_7.ebx & (1 << 0), "fsgsbase");
    add_flag_if(res_7.ebx & (1 << 1), "tsc_adjust");
    add_flag_if(res_7.ebx & (1 << 3), "bmi1");
    add_flag_if(res_7.ebx & (1 << 4), "hle");
    add_flag_if(res_7.ebx & (1 << 5), "avx2");
    add_flag_if(res_7.ebx & (1 << 7), "smep");
    add_flag_if(res_7.ebx & (1 << 8), "bmi2");
    add_flag_if(res_7.ebx & (1 << 9), "erms");
    add_flag_if(res_7.ebx & (1 << 10), "invpcid");
    add_flag_if(res_7.ebx & (1 << 11), "rtm");
    add_flag_if(res_7.ebx & (1 << 12), "rdt_m");
    add_flag_if(res_7.ebx & (1 << 13), "depc_fpu_cs_ds");
    add_flag_if(res_7.ebx & (1 << 14), "mpx");
    add_flag_if(res_7.ebx & (1 << 15), "rdt_a");
    add_flag_if(res_7.ebx & (1 << 16), "avx512f");
    add_flag_if(res_7.ebx & (1 << 17), "avx512dq");
    add_flag_if(res_7.ebx & (1 << 18), "rdseed");
    add_flag_if(res_7.ebx & (1 << 19), "adx");
    add_flag_if(res_7.ebx & (1 << 20), "smap");
    add_flag_if(res_7.ebx & (1 << 21), "avx512ifma");
    add_flag_if(res_7.ebx & (1 << 23), "clflushopt");
    add_flag_if(res_7.ebx & (1 << 24), "clwb");
    add_flag_if(res_7.ebx & (1 << 25), "intel_pt");
    add_flag_if(res_7.ebx & (1 << 26), "avx512pf");
    add_flag_if(res_7.ebx & (1 << 27), "avx512er");
    add_flag_if(res_7.ebx & (1 << 28), "avx512cd");
    add_flag_if(res_7.ebx & (1 << 29), "sha_ni");
    add_flag_if(res_7.ebx & (1 << 30), "avx512bw");
    add_flag_if(res_7.ebx & (1 << 31), "avx512vl");
    add_flag_if(res_d_1.eax & (1 << 0), "xsaveopt");
    add_flag_if(res_d_1.eax & (1 << 1), "xsavec");
    add_flag_if(res_d_1.eax & (1 << 2), "xgetbv1");
    add_flag_if(res_d_1.eax & (1 << 3), "xsaves");

    add_flag_if(res_7_1.eax & (1 << 5), "avx512_bf16");
    add_flag_if(res_8000_0008.ebx & (1 << 0), "clzero");
    add_flag_if(res_8000_0008.ebx & (1 << 1), "irperf");
    add_flag_if(res_8000_0008.ebx & (1 << 2), "xsaveerptr");

    // Intel digital thermal sensor
    add_flag_if(false, "dtherm");
    // Intel turbo boost
    add_flag_if(false, "ida");
    add_flag_if(res_6.eax & (1 << 2), "arat");
    // Power limit notification controls
    add_flag_if(false, "pln");
    // Intel package thermal status
    add_flag_if(false, "pts");

    // Intel Hardware P-state features
    add_flag_if(false, "hwp");
    add_flag_if(false, "hwp_notify");
    add_flag_if(false, "hwp_act_window");
    add_flag_if(false, "hwp_epp");
    add_flag_if(false, "hwp_pkg_req");

    add_flag_if(res_8000_000a.ebx & (1 << 0), "npt");
    add_flag_if(res_8000_000a.ebx & (1 << 1), "lbrv");
    add_flag_if(res_8000_000a.ebx & (1 << 2), "svm_lock");
    add_flag_if(res_8000_000a.ebx & (1 << 3), "nrip_save");
    add_flag_if(res_8000_000a.ebx & (1 << 4), "tsc_scale");
    add_flag_if(res_8000_000a.ebx & (1 << 5), "vmcb_clean");
    add_flag_if(res_8000_000a.ebx & (1 << 6), "flushbyasid");
    add_flag_if(res_8000_000a.ebx & (1 << 7), "decodeassists");
    add_flag_if(res_8000_000a.ebx & (1 << 10), "pausefilter");
    add_flag_if(res_8000_000a.ebx & (1 << 12), "pfthreshold");
    add_flag_if(res_8000_000a.ebx & (1 << 13), "avic");
    add_flag_if(res_8000_000a.ebx & (1 << 15), "v_vmsave_vmload");
    add_flag_if(res_8000_000a.ebx & (1 << 16), "vgif");

    add_flag_if(res_7.ecx & (1 << 1), "avx512vbmi");
    add_flag_if(res_7.ecx & (1 << 2), "umip");
    add_flag_if(res_7.ecx & (1 << 3), "pku");
    add_flag_if(res_7.ecx & (1 << 4), "ospke");
    add_flag_if(res_7.ecx & (1 << 5), "waitpkg");
    add_flag_if(res_7.ecx & (1 << 6), "avx512_vbmi2");
    add_flag_if(res_7.ecx & (1 << 8), "gfni");
    add_flag_if(res_7.ecx & (1 << 9), "vaes");
    add_flag_if(res_7.ecx & (1 << 10), "vpclmulqdq");
    add_flag_if(res_7.ecx & (1 << 11), "avx512_vnni");
    add_flag_if(res_7.ecx & (1 << 12), "avx512_bitalg");
    add_flag_if(res_7.ecx & (1 << 13), "tme");
    add_flag_if(res_7.ecx & (1 << 14), "avx512_vpopcntdq");
    add_flag_if(res_7.ecx & (1 << 16), "la57");
    add_flag_if(res_7.ecx & (1 << 22), "rdpid");
    add_flag_if(res_7.ecx & (1 << 25), "cldemote");
    add_flag_if(res_7.ecx & (1 << 27), "movdiri");
    add_flag_if(res_7.ecx & (1 << 28), "movdir64b");

    add_flag_if(res_8000_0007.ebx & (1 << 0), "overflow_recov");
    add_flag_if(res_8000_0007.ebx & (1 << 1), "succor");
    add_flag_if(res_8000_0007.ebx & (1 << 3), "smca");

    add_flag_if(res_7.edx & (1 << 2), "avx512_4vnniw");
    add_flag_if(res_7.edx & (1 << 3), "avx512_4fmaps");
    add_flag_if(res_7.edx & (1 << 4), "fsrm");
    add_flag_if(res_7.edx & (1 << 8), "avx512_vp2intersect");
    add_flag_if(res_7.edx & (1 << 10), "md_clear");
    add_flag_if(res_7.edx & (1 << 14), "serialize");
    add_flag_if(res_7.edx & (1 << 18), "pconfig");
    add_flag_if(res_7.edx & (1 << 19), "arch_lbr");
    add_flag_if(res_7.edx & (1 << 28), "flush_l1d");
    add_flag_if(res_7.edx & (1 << 29), "arch_capabilities");
  }

  // Get the cycle counter frequency from CPUID function 15h.
  auto res_15 = ctx->RunCPUIDFunction(0x15, 0);
  // Frequency is calculated in Hz, we need to convert it to megahertz since FEX is guaranteed to return >= 1Ghz.
  // x86 Bogomips is calculated as an equation based on the clock speed of the CPU (Or TSC) divided by 500k jiffies.
  // A `jiffie` is an internal metric for the kernel's `HZ` frequency which is usually between 100 and 1000.
  // Userspace can't query this HZ config option, so assume 1000Hz since that's common.
  // This gives a 1Ghz ARMv9.2 CPU a Bogomips of 2Ghz.
  constexpr double HzInMhz = 1000000.0;
  constexpr double HzInKhz = 1000.0;
  constexpr double BogomipsJiffyPrecision = 1'000.0;
  constexpr double BogoMipsDivisor = 500'000.0 / BogomipsJiffyPrecision;

  const double Frequency = 1.0 / (static_cast<double>(res_15.eax) / (static_cast<double>(res_15.ebx) * static_cast<double>(res_15.ecx)));
  const double FrequencyMhz = Frequency / HzInMhz;
  const double FrequencyKhz = Frequency / HzInKhz;
  const double Bogomips = FrequencyKhz / BogoMipsDivisor;
  // Generate the cycle counter frequency string in the format expected by cpuinfo.
  // ex: `4000.000`
  const auto FrequencyString = fextl::fmt::format("{:.3f}", FrequencyMhz);
  const auto BogomipsString = fextl::fmt::format("{:.2f}", Bogomips);

  for (int i = 0; i < CPUCores; ++i) {
    cpu_stream << "processor\t: " << i << std::endl; // Logical id
    cpu_stream << "vendor_id\t: " << vendorid.Str << std::endl;
    cpu_stream << "cpu family\t: " << Family << std::endl;
    cpu_stream << "model\t\t: " << (info.Model + (info.FamilyID >= 6 ? (info.ExModelID << 4) : 0)) << std::endl;
    ModelName modelname {};
    auto res_8000_0002 = ctx->RunCPUIDFunctionName(0x8000'0002, 0, i);
    auto res_8000_0003 = ctx->RunCPUIDFunctionName(0x8000'0003, 0, i);
    auto res_8000_0004 = ctx->RunCPUIDFunctionName(0x8000'0004, 0, i);
    modelname.cpuid_2 = res_8000_0002;
    modelname.cpuid_3 = res_8000_0003;
    modelname.cpuid_4 = res_8000_0004;
    modelname.null = 0;

    cpu_stream << "model name\t: " << modelname.Str << std::endl;
    cpu_stream << "stepping\t: " << info.Stepping << std::endl;
    cpu_stream << "microcode\t: 0x0" << std::endl;
    cpu_stream << "cpu MHz\t\t: " << FrequencyString << std::endl;
    cpu_stream << "cache size\t: 512 KB" << std::endl;
    cpu_stream << "physical id\t: 0" << std::endl;          // Socket id (always 0 for a single socket system)
    cpu_stream << "siblings\t: " << CPUCores << std::endl;  // Number of logical cores
    cpu_stream << "core id\t\t: " << i << std::endl;        // Physical id
    cpu_stream << "cpu cores\t: " << CPUCores << std::endl; // Number of physical cores
    cpu_stream << "apicid\t\t: " << i << std::endl;
    cpu_stream << "initial apicid\t: " << i << std::endl;
    cpu_stream << "fpu\t\t: " << (res_1.edx & (1 << 0) ? "yes" : "no") << std::endl;
    cpu_stream << "fpu_exception\t: " << (res_1.edx & (1 << 0) ? "yes" : "no") << std::endl;
    cpu_stream << "cpuid level\t: " << vendorid.id << std::endl;
    cpu_stream << "wp\t\t: yes" << std::endl;
    cpu_stream << "flags\t\t: " << flags_data.str() << std::endl;

    // We don't have any bugs, don't question it
    cpu_stream << "bugs\t\t: " << std::endl;
    cpu_stream << "bogomips\t: " << BogomipsString << std::endl;
    // These next four aren't necessarily correct
    cpu_stream << "TLB size\t: 2560 4K pages" << std::endl;
    cpu_stream << "clflush size\t: 64" << std::endl;
    cpu_stream << "cache_alignment\t : 64" << std::endl;

    // Cortex-A is 40 or 44 bits physical, and 48/52 virtual
    // Choose the lesser configuration
    cpu_stream << "address sizes\t: 40 bits physical, 48 bits virtual" << std::endl;

    // No power management but required to report
    cpu_stream << "power management: " << std::endl;

    cpu_stream << std::endl;
  }

  return cpu_stream.str();
}

EmulatedFDManager::EmulatedFDManager(FEXCore::Context::Context* ctx)
  : CTX {ctx}
  , ThreadsConfig {FEX::CPUInfo::CalculateNumberOfCPUs()} {
  FDReadCreators["/proc/cpuinfo"] = [&](FEXCore::Context::Context* ctx, int32_t fd, const char* pathname, int32_t flags, mode_t mode) -> int32_t {
    // Only allow a single thread to initialize the cpu_info.
    // Jit in-case multiple threads try to initialize at once.
    // Check if deferred cpuinfo initialization has occured.
    std::call_once(cpu_info_initialized, [&]() { cpu_info = GenerateCPUInfo(ctx, ThreadsConfig); });

    int FD = GenTmpFD(pathname, flags);
    write(FD, (void*)&cpu_info.at(0), cpu_info.size());
    lseek(FD, 0, SEEK_SET);
    SealTmpFD(FD);
    return FD;
  };

  FDReadCreators["/proc/sys/kernel/osrelease"] = [&](FEXCore::Context::Context* ctx, int32_t fd, const char* pathname, int32_t flags,
                                                     mode_t mode) -> int32_t {
    int FD = GenTmpFD(pathname, flags);
    uint32_t GuestVersion = FEX::HLE::_SyscallHandler->GetGuestKernelVersion();
    char Tmp[64] {};
    snprintf(Tmp, sizeof(Tmp), "%d.%d.%d\n", FEX::HLE::SyscallHandler::KernelMajor(GuestVersion),
             FEX::HLE::SyscallHandler::KernelMinor(GuestVersion), FEX::HLE::SyscallHandler::KernelPatch(GuestVersion));
    // + 1 to ensure null at the end
    write(FD, Tmp, strlen(Tmp) + 1);
    lseek(FD, 0, SEEK_SET);
    SealTmpFD(FD);
    return FD;
  };

  FDReadCreators["/proc/version"] = [&](FEXCore::Context::Context* ctx, int32_t fd, const char* pathname, int32_t flags, mode_t mode) -> int32_t {
    int FD = GenTmpFD(pathname, flags);
    // UTS version NEEDS to be in a format that can pass to `date -d`
    // Format of this is Linux version <Release> (<Compile By>@<Compile Host>) (<Linux Compiler>) #<version> {SMP, PREEMPT, PREEMPT_RT} <UTS version>\n"
    const char kernel_version[] = "Linux version %d.%d.%d (FEX@FEX) (clang) #" GIT_DESCRIBE_STRING " SMP " __DATE__ " " __TIME__ "\n";
    uint32_t GuestVersion = FEX::HLE::_SyscallHandler->GetGuestKernelVersion();
    char Tmp[sizeof(kernel_version) + 64] {};
    snprintf(Tmp, sizeof(Tmp), kernel_version, FEX::HLE::SyscallHandler::KernelMajor(GuestVersion),
             FEX::HLE::SyscallHandler::KernelMinor(GuestVersion), FEX::HLE::SyscallHandler::KernelPatch(GuestVersion));
    // + 1 to ensure null at the end
    write(FD, Tmp, strlen(Tmp) + 1);
    lseek(FD, 0, SEEK_SET);
    SealTmpFD(FD);
    return FD;
  };

  // Wine reads this to ensure TSC is trusted by the kernel. Otherwise it falls back to maximum clock speed of the CPU cores.
  // Without this, games like Horizon Zero Dawn would run their physics in slow-motion.
  FDReadCreators["/sys/devices/system/clocksource/clocksource0/current_clocksource"] =
    [&](FEXCore::Context::Context* ctx, int32_t fd, const char* pathname, int32_t flags, mode_t mode) -> int32_t {
    int FD = GenTmpFD(pathname, flags);
    const char source[] = "tsc\n";
    // + 1 to ensure null at the end
    write(FD, source, strlen(source) + 1);
    lseek(FD, 0, SEEK_SET);
    SealTmpFD(FD);
    return FD;
  };

  auto NumCPUCores = [&](FEXCore::Context::Context* ctx, int32_t fd, const char* pathname, int32_t flags, mode_t mode) -> int32_t {
    int FD = GenTmpFD(pathname, flags);
    write(FD, (void*)&cpus_online.at(0), cpus_online.size());
    lseek(FD, 0, SEEK_SET);
    SealTmpFD(FD);
    return FD;
  };

  FDReadCreators["/sys/devices/system/cpu/online"] = NumCPUCores;
  FDReadCreators["/sys/devices/system/cpu/present"] = NumCPUCores;

  fextl::string procAuxv = fextl::fmt::format("/proc/{}/auxv", getpid());

  FDReadCreators[procAuxv] = &EmulatedFDManager::ProcAuxv;
  FDReadCreators["/proc/self/auxv"] = &EmulatedFDManager::ProcAuxv;

  auto cmdline_handler = [&](FEXCore::Context::Context* ctx, int32_t fd, const char* pathname, int32_t flags, mode_t mode) -> int32_t {
    int FD = GenTmpFD(pathname, flags);
    auto CodeLoader = FEX::HLE::_SyscallHandler->GetCodeLoader();
    auto Args = CodeLoader->GetApplicationArguments();
    char NullChar {};
    // cmdline is an array of null terminated arguments
    for (size_t i = 0; i < Args->size(); ++i) {
      auto& Arg = Args->at(i);
      write(FD, Arg.c_str(), Arg.size());
      // Finish off with a null terminator
      write(FD, &NullChar, sizeof(uint8_t));
    }

    // One additional null terminator to finish the list
    lseek(FD, 0, SEEK_SET);
    SealTmpFD(FD);
    return FD;
  };

  FDReadCreators["/proc/self/cmdline"] = cmdline_handler;
  fextl::string procCmdLine = fextl::fmt::format("/proc/{}/cmdline", getpid());
  FDReadCreators[procCmdLine] = cmdline_handler;

  if (ThreadsConfig > 1) {
    cpus_online = fextl::fmt::format("0-{}", ThreadsConfig - 1);
  } else {
    cpus_online = "0";
  }
}

EmulatedFDManager::~EmulatedFDManager() {}

int32_t EmulatedFDManager::Open(const char* pathname, int flags, uint32_t mode) {
  auto Creator = FDReadCreators.end();
  if (pathname) {
    Creator = FDReadCreators.find(pathname);
  }

  if (Creator == FDReadCreators.end()) {
    return -1;
  }

  return Creator->second(CTX, AT_FDCWD, pathname, flags, mode);
}

int32_t EmulatedFDManager::ProcAuxv(FEXCore::Context::Context* ctx, int32_t fd, const char* pathname, int32_t flags, mode_t mode) {
  uint64_t auxvBase = 0, auxvSize = 0;
  FEX::HLE::_SyscallHandler->GetCodeLoader()->GetAuxv(auxvBase, auxvSize);
  if (!auxvBase) {
    LogMan::Msg::DFmt("Failed to get Auxv stack address");
    return -1;
  }

  int FD = GenTmpFD(pathname, flags);
  write(FD, (void*)auxvBase, auxvSize);
  lseek(FD, 0, SEEK_SET);
  SealTmpFD(FD);
  return FD;
}
} // namespace FEX::EmulatedFile
