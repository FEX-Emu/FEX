// SPDX-License-Identifier: MIT
/*
$info$
tags: LinuxSyscalls|common
desc: Emulated /proc/cpuinfo, version, osrelease, etc
$end_info$
*/

#include "CodeLoader.h"

#include "Common/FDUtils.h"
#include "LinuxSyscalls/Syscalls.h"
#include "LinuxSyscalls/EmulatedFiles/EmulatedFiles.h"

#include <FEXCore/Config/Config.h>
#include <FEXCore/Core/Context.h>
#include <FEXCore/Core/CPUID.h>
#include <FEXCore/Utils/CPUInfo.h>
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
  fcntl(fd, F_ADD_SEALS, F_SEAL_SEAL | F_SEAL_SHRINK | F_SEAL_GROW | F_SEAL_WRITE | F_SEAL_FUTURE_WRITE);
}

fextl::string GenerateCPUInfo(FEXCore::Context::Context* ctx, uint32_t CPUCores) {
  fextl::ostringstream cpu_stream {};
  auto res_0 = ctx->RunCPUIDFunction(0, 0);
  auto res_1 = ctx->RunCPUIDFunction(1, 0);
  auto res_6 = ctx->RunCPUIDFunction(6, 0);
  auto res_7 = ctx->RunCPUIDFunction(7, 0);
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
#define FLAG(flag, name)       \
  if (flag) {                  \
    flags_data << name << " "; \
  }
    FLAG(res_1.edx & (1 << 0), "fpu")
    FLAG(res_1.edx & (1 << 1), "vme")
    FLAG(res_1.edx & (1 << 2), "de")
    FLAG(res_1.edx & (1 << 3), "pse")
    FLAG(res_1.edx & (1 << 4), "tsc")
    FLAG(res_1.edx & (1 << 5), "msr")
    FLAG(res_1.edx & (1 << 6), "pae")
    FLAG(res_1.edx & (1 << 7), "mce")
    FLAG(res_1.edx & (1 << 8), "cx8")
    FLAG(res_1.edx & (1 << 9), "apic")
    FLAG(res_1.edx & (1 << 11), "sep")
    FLAG(res_1.edx & (1 << 12), "mtrr")
    FLAG(res_1.edx & (1 << 13), "pge")
    FLAG(res_1.edx & (1 << 14), "mca")
    FLAG(res_1.edx & (1 << 15), "cmov")
    FLAG(res_1.edx & (1 << 16), "pat")
    FLAG(res_1.edx & (1 << 17), "pse36")
    FLAG(res_1.edx & (1 << 18), "pn")
    FLAG(res_1.edx & (1 << 19), "clflush")
    FLAG(res_1.edx & (1 << 21), "ds")   // XXX
    FLAG(res_1.edx & (1 << 22), "acpi") // XXX
    FLAG(res_1.edx & (1 << 23), "mmx")
    FLAG(res_1.edx & (1 << 24), "fxsr")
    FLAG(res_1.edx & (1 << 25), "sse")
    FLAG(res_1.edx & (1 << 26), "sse2")
    FLAG(res_1.edx & (1 << 27), "ss")
    FLAG(res_1.edx & (1 << 28), "ht")
    FLAG(res_1.edx & (1 << 29), "tm")
    FLAG(res_1.edx & (1 << 30), "ia64")
    FLAG(res_1.edx & (1 << 31), "pbe")

    FLAG(res_8000_0001.edx & (1 << 11), "syscall")
    FLAG(res_8000_0001.edx & (1 << 19), "mp")
    FLAG(res_8000_0001.edx & (1 << 20), "nx")
    FLAG(res_8000_0001.edx & (1 << 22), "mmxext")
    FLAG(res_8000_0001.edx & (1 << 25), "fxsr_opt")
    FLAG(res_8000_0001.edx & (1 << 26), "pdpe1gb")
    FLAG(res_8000_0001.edx & (1 << 27), "rdtscp")
    FLAG(res_8000_0001.edx & (1 << 29), "lm")
    FLAG(res_8000_0001.edx & (1 << 31), "3dnow")
    FLAG(res_8000_0001.edx & (1 << 30), "3dnowext")

    FLAG(res_8000_0007.edx & (1 << 8), "constant_tsc")

    // We are not a uniprocessor running in SMP mode
    FLAG(false, "up")
    // Timer is always running
    FLAG(true, "art")
    // No Intel perfmon
    FLAG(false, "arch_perfmon")
    // No precise event based sampling
    FLAG(false, "pebs")
    // No branch trace store
    FLAG(false, "bts")

    FLAG(true, "rep_good")
    FLAG(res_8000_0007.edx & (1 << 12), "tm")

    // Always support long nop
    FLAG(true, "nopl")

    // Always expose topology information
    FLAG(true, "xtoplogy")

    // Atom/geode only?
    FLAG(false, "tsc_reliable")
    FLAG(res_8000_0007.edx & (1 << 8), "nonstop_tsc")

    // We always support CPUID
    FLAG(true, "cpuid")
    FLAG(Family > 0x16, "extd_apicid")
    FLAG(false, "amd_dcm") // Never claim to be a multi node processor
    FLAG(res_8000_0007.edx & (1 << 11), "aperfmperf")

    // Need to check ARM documentation if we can support this?
    FLAG(false, "nonstop_tsc_s3")

    // We can calculate this flag on AArch64
    FLAG(true, "tsc_known_freq")

    FLAG(res_1.ecx & (1 << 0), "pni")
    FLAG(res_1.ecx & (1 << 1), "pclmulqdq")
    FLAG(res_1.ecx & (1 << 2), "dtes64")
    FLAG(res_1.ecx & (1 << 3), "monitor")
    FLAG(res_1.ecx & (1 << 4), "ds_cpl")
    FLAG(res_1.ecx & (1 << 5), "vmx")
    FLAG(res_1.ecx & (1 << 6), "smx")
    FLAG(res_1.ecx & (1 << 7), "est")
    FLAG(res_1.ecx & (1 << 8), "tm2")
    FLAG(res_1.ecx & (1 << 9), "ssse3")
    FLAG(res_1.ecx & (1 << 11), "sdbg")
    FLAG(res_1.ecx & (1 << 12), "fma")
    FLAG(res_1.ecx & (1 << 13), "cx16")
    FLAG(res_1.ecx & (1 << 14), "xptr")
    FLAG(res_1.ecx & (1 << 15), "pdcm")
    FLAG(res_1.ecx & (1 << 17), "pcid")
    FLAG(res_1.ecx & (1 << 18), "dca")
    FLAG(res_1.ecx & (1 << 19), "sse4_1")
    FLAG(res_1.ecx & (1 << 20), "sse4_2")
    FLAG(res_1.ecx & (1 << 21), "x2apic")
    FLAG(res_1.ecx & (1 << 22), "movbe")
    FLAG(res_1.ecx & (1 << 23), "popcnt")
    FLAG(res_1.ecx & (1 << 24), "tsc_deadline_timer")
    FLAG(res_1.ecx & (1 << 25), "aes")
    FLAG(res_1.ecx & (1 << 26), "xsave")
    FLAG(res_1.ecx & (1 << 28), "avx")
    FLAG(res_1.ecx & (1 << 29), "f16c")
    FLAG(res_1.ecx & (1 << 30), "rdrand")
    FLAG(res_1.ecx & (1 << 31), "hypervisor")

    FLAG(res_8000_0001.ecx & (1 << 0), "lahf_lm")
    FLAG(res_8000_0001.ecx & (1 << 1), "cmp_legacy")
    FLAG(res_8000_0001.ecx & (1 << 2), "svm")
    FLAG(res_8000_0001.ecx & (1 << 3), "extapic")
    FLAG(res_8000_0001.ecx & (1 << 4), "cr8_legacy")
    FLAG(res_8000_0001.ecx & (1 << 5), "abm")
    FLAG(res_8000_0001.ecx & (1 << 6), "sse4a")
    FLAG(res_8000_0001.ecx & (1 << 7), "misalignsse")
    FLAG(res_8000_0001.ecx & (1 << 8), "3dnowprefetch")
    FLAG(res_8000_0001.ecx & (1 << 9), "osvw")
    FLAG(res_8000_0001.ecx & (1 << 10), "ibs")
    FLAG(res_8000_0001.ecx & (1 << 11), "xop")
    FLAG(res_8000_0001.ecx & (1 << 12), "skinit")
    FLAG(res_8000_0001.ecx & (1 << 13), "wdt")
    FLAG(res_8000_0001.ecx & (1 << 15), "lwp")
    FLAG(res_8000_0001.ecx & (1 << 16), "fma4")
    FLAG(res_8000_0001.ecx & (1 << 17), "tce")
    FLAG(res_8000_0001.ecx & (1 << 19), "nodeid_msr")
    FLAG(res_8000_0001.ecx & (1 << 21), "tbm")
    FLAG(res_8000_0001.ecx & (1 << 22), "topoext")
    FLAG(res_8000_0001.ecx & (1 << 23), "perfctr_core")
    FLAG(res_8000_0001.ecx & (1 << 24), "perfctr_nb")
    FLAG(res_8000_0001.ecx & (1 << 26), "bpext")
    FLAG(res_8000_0001.ecx & (1 << 27), "ptsc")

    FLAG(res_8000_0001.ecx & (1 << 28), "perfctr_llc")
    FLAG(res_8000_0001.ecx & (1 << 29), "mwaitx")

    // We don't support ring 3 supporting mwait
    FLAG(false, "ring3mwait")
    // We don't support Intel CPUID fault support
    FLAG(false, "cpuid_fault")
    FLAG(res_8000_0007.edx & (1 << 9), "cpb")
    FLAG(res_6.ecx & (1 << 3), "epb")
    FLAG(res_10.ebx & (1 << 1), "cat_l3")
    FLAG(res_10.ebx & (1 << 2), "cat_l2")
    FLAG(false, // Needs leaf support
         "cdp_l3")
    FLAG(false, "invpcid_single")
    FLAG(res_8000_0007.edx & (1 << 7), "hw_pstate")
    FLAG(res_8000_001f.eax & (1 << 0), "sme")

    // Kernel page table isolation.
    FLAG(false, "pti")

    // We don't support Intel's Protected Processor Inventory Number
    FLAG(false, "intel_ppin")
    FLAG(false, // Needs leaf support
         "cdp_l2")

    FLAG(res_8000_0008.ebx & (1 << 6), "mba")
    FLAG(res_8000_001f.eax & (1 << 1), "sev")

      {// Speculative bug workarounds
       // We don't claim to have these bugs, so we don't need to claim these flags
       FLAG(res_7.edx & (1 << 31), "ssbd") FLAG(false, "ibrs") FLAG(false, "ibpb")

         FLAG(res_7.edx & (1 << 27), "stibp")

           FLAG(false, "ibrs_enhanced")}

    // We don't support Intel's TPR Shadow feature
    FLAG(false, "tpr_shadow")
      // Intel virtual NMI
      FLAG(false, "vnmi")
      // Intel FlexPriority
      FLAG(false, "flexpriority")
      // Intel Extended page table
      FLAG(false, "ept")
      // Intel virtual processor ID
      FLAG(false, "vpid")

      // Prefer VMMCall to VMCall
      FLAG(false, "vmmcall")
      // Intel extended page table access dirty bit
      FLAG(false, "ept_ad") FLAG(res_7.ebx & (1 << 0), "fsgsbase") FLAG(res_7.ebx & (1 << 1), "tsc_adjust")
        FLAG(res_7.ebx & (1 << 3), "bmi1") FLAG(res_7.ebx & (1 << 4), "hle") FLAG(res_7.ebx & (1 << 5), "avx2")
          FLAG(res_7.ebx & (1 << 7), "smep") FLAG(res_7.ebx & (1 << 8), "bmi2") FLAG(res_7.ebx & (1 << 9), "erms")
            FLAG(res_7.ebx & (1 << 10), "invpcid") FLAG(res_7.ebx & (1 << 11),
                                                        "rtm") FLAG(false, // Needs leaf support
                                                                    "cqm") FLAG(res_7.ebx & (1 << 14),
                                                                                "mpx") FLAG(false, // Needs leaf support
                                                                                            "rdt_a") FLAG(res_7.ebx & (1 << 16), "avx512f")
              FLAG(res_7.ebx & (1 << 17), "avx512dq") FLAG(res_7.ebx & (1 << 18), "rdseed") FLAG(res_7.ebx & (1 << 19), "adx")
                FLAG(res_7.ebx & (1 << 20), "smap") FLAG(res_7.ebx & (1 << 21), "avx512ifma") FLAG(res_7.ebx & (1 << 23), "clflushopt")
                  FLAG(res_7.ebx & (1 << 24), "clwb") FLAG(res_7.ebx & (1 << 25), "intel_pt") FLAG(res_7.ebx & (1 << 26), "avx512pf")
                    FLAG(res_7.ebx & (1 << 27), "avx512er") FLAG(res_7.ebx & (1 << 28), "avx512cd") FLAG(res_7.ebx & (1 << 29), "sha_ni")
                      FLAG(res_7.ebx & (1 << 30), "avx512bw") FLAG(res_7.ebx & (1 << 31),
                                                                   "avx512vl")
                        FLAG(false,                                                 // Needs leaf support // res_d.eax & (1 << 0) // Leaf 1h
                             "xsaveopt") FLAG(false,                                // Needs leaf support // res_d.eax & (1 << 1) // Leaf 1h
                                              "xsavec") FLAG(false,                 // Needs leaf support // res_d.eax & (1 << 2) // Leaf 1h
                                                             "xgetbv1") FLAG(false, // Needs leaf support // res_d.eax & (1 << 3) // Leaf 1h
                                                                             "xsaves")

                          FLAG(false, // Needs leaf support
                               "avx512_bf16") FLAG(res_8000_0008.ebx & (1 << 0), "clzero") FLAG(res_8000_0008.ebx & (1 << 1), "irperf")
                            FLAG(res_8000_0008.ebx & (1 << 2), "xsaveerptr")

      // Intel digital thermal sensor
      FLAG(false, "dtherm")
      // Intel turbo boost
      FLAG(false, "ida") FLAG(res_6.eax & (1 << 2), "arat")
      // Power limit notification controls
      FLAG(false, "pln")
      // Intel package thermal status
      FLAG(false, "pts")

      // Intel Hardware P-state features
      FLAG(false, "hwp") FLAG(false, "hwp_notify") FLAG(false, "hwp_act_window") FLAG(false, "hwp_epp") FLAG(false, "hwp_pkg_req")

        FLAG(res_8000_000a.ebx & (1 << 0), "npt") FLAG(res_8000_000a.ebx & (1 << 1), "lbrv") FLAG(res_8000_000a.ebx & (1 << 2), "svm_lock")
          FLAG(res_8000_000a.ebx & (1 << 3), "nrip_save") FLAG(res_8000_000a.ebx & (1 << 4), "tsc_scale")
            FLAG(res_8000_000a.ebx & (1 << 5), "vmcb_clean") FLAG(res_8000_000a.ebx & (1 << 6), "flushbyasid")
              FLAG(res_8000_000a.ebx & (1 << 7), "decodeassists") FLAG(res_8000_000a.ebx & (1 << 10), "pausefilter")
                FLAG(res_8000_000a.ebx & (1 << 12), "pfthreshold") FLAG(res_8000_000a.ebx & (1 << 13), "avic")
                  FLAG(res_8000_000a.ebx & (1 << 15), "v_vmsave_vmload") FLAG(res_8000_000a.ebx & (1 << 16), "vgif")

                    FLAG(res_7.ecx & (1 << 1), "avx512vbmi") FLAG(res_7.ecx & (1 << 2), "umip") FLAG(res_7.ecx & (1 << 3), "pku")
                      FLAG(res_7.ecx & (1 << 4), "ospke") FLAG(res_7.ecx & (1 << 5), "waitpkg") FLAG(res_7.ecx & (1 << 6), "avx512_vbmi2")
                        FLAG(res_7.ecx & (1 << 8), "gfni") FLAG(res_7.ecx & (1 << 9), "vaes") FLAG(res_7.ecx & (1 << 10), "vpclmulqdq")
                          FLAG(res_7.ecx & (1 << 11), "avx512_vnni") FLAG(res_7.ecx & (1 << 12), "avx512_bitalg")
                            FLAG(res_7.ecx & (1 << 13), "tme") FLAG(res_7.ecx & (1 << 14), "avx512_vpopcntdq")
                              FLAG(res_7.ecx & (1 << 16), "la57") FLAG(res_7.ecx & (1 << 22), "rdpid") FLAG(res_7.ecx & (1 << 25), "cldemot"
                                                                                                                                   "e")
                                FLAG(res_7.ecx & (1 << 27), "movdiri") FLAG(res_7.ecx & (1 << 28), "movdir64b")

                                  FLAG(res_8000_0007.ebx & (1 << 0), "overflow_recov") FLAG(res_8000_0007.ebx & (1 << 1), "succor")
                                    FLAG(res_8000_0007.ebx & (1 << 3), "smca")

                                      FLAG(res_7.edx & (1 << 2), "avx512_4vnniw") FLAG(res_7.edx & (1 << 3), "avx512_4fmaps")
                                        FLAG(res_7.edx & (1 << 4), "fsrm") FLAG(res_7.edx & (1 << 8), "avx512_vp2intersect")
                                          FLAG(res_7.edx & (1 << 10), "md_clear") FLAG(res_7.edx & (1 << 14), "serialize")
                                            FLAG(res_7.edx & (1 << 18), "pconfig") FLAG(res_7.edx & (1 << 19), "arch_lbr")
                                              FLAG(res_7.edx & (1 << 28), "flush_l1d") FLAG(res_7.edx & (1 << 29), "arch_capabilities")
  }

  // Get the cycle counter frequency from CPUID function 15h.
  auto res_15 = ctx->RunCPUIDFunction(0x15, 0);
  // Frequency is calculated in Hz, we need to convert it to megahertz since FEX is guaranteed to return >= 1Ghz.
  constexpr double HzInMhz = 1000000.0;
  double Frequency = 1.0 / (static_cast<double>(res_15.eax) / (static_cast<double>(res_15.ebx) * static_cast<double>(res_15.ecx)));
  Frequency /= HzInMhz;
  // Generate the cycle counter frequency string in the format expected by cpuinfo.
  // ex: `4000.000`
  const auto FrequencyString = fextl::fmt::format("{:.3f}", Frequency);

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
    cpu_stream << "bogomips\t: 8000.0" << std::endl;
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
  , ThreadsConfig {FEXCore::CPUInfo::CalculateNumberOfCPUs()} {
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

int32_t EmulatedFDManager::OpenAt(int dirfs, const char* pathname, int flags, uint32_t mode) {
  char Tmp[PATH_MAX];
  const char* Path {};

  auto Creator = FDReadCreators.end();
  if (pathname) {
    Creator = FDReadCreators.find(pathname);
    Path = pathname;
  }

  if (Creator == FDReadCreators.end()) {
    if (((pathname && pathname[0] != '/') || // If pathname exists then it must not be absolute
         !pathname) &&
        dirfs != AT_FDCWD) {
      // Passed in a dirfd that isn't magic FDCWD
      // We need to get the path from the fd now
      auto PathLength = FEX::get_fdpath(dirfs, Tmp);
      if (PathLength != -1) {
        if (pathname) {
          Tmp[PathLength] = '/';
          PathLength += 1;
          strncpy(&Tmp[PathLength], pathname, PATH_MAX - PathLength);
        } else {
          Tmp[PathLength] = '\0';
        }
        Path = Tmp;
      } else if (pathname) {
        Path = pathname;
      }
    } else {
      if (!pathname || pathname[0] == 0) {
        return -1;
      }

      Path = pathname;
    }

    bool exists = access(Path, F_OK) == 0;
    bool RealPathExists = false;

    if (exists) {
      // If realpath fails then the temporary buffer is in an undefined state.
      // Need to use another temporary just in-case realpath doesn't succeed.
      char ExistsTempPath[PATH_MAX];
      char* RealPath = realpath(Path, ExistsTempPath);
      if (RealPath) {
        RealPathExists = true;
        Creator = FDReadCreators.find(RealPath);
      }
    }

    if (!RealPathExists) {
      Creator = FDReadCreators.find(FHU::Filesystem::LexicallyNormal(Path));
    }

    if (Creator == FDReadCreators.end()) {
      return -1;
    }
  }

  return Creator->second(CTX, dirfs, Path, flags, mode);
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
