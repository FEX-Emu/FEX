// SPDX-License-Identifier: MIT
/*
$info$
tags: LinuxSyscalls|syscalls-shared
$end_info$
*/

#include "LinuxSyscalls/Seccomp/SeccompEmulator.h"

#include <linux/bpf_common.h>
#include <linux/filter.h>
#include <linux/seccomp.h>

namespace FEX::HLE {
void SeccompEmulator::DumpProgram(const sock_fprog* prog) {
  auto Parse_Class_LD = [](uint32_t BPFIP, const sock_filter* Inst) {
    auto DestName = [](const sock_filter* Inst) {
      if (BPF_CLASS(Inst->code) == BPF_LD) {
        return "A";
      } else {
        return "X";
      }
    };

    auto AccessSize = [](const sock_filter* Inst) {
      switch (BPF_SIZE(Inst->code)) {
      case BPF_W: return 32;
      case BPF_H: return 16;
      case BPF_B: return 8;
      case 0x18: /* BPF_DW */ return 64;
      }
      return 0;
    };

    auto ModeType = [](const sock_filter* Inst) {
      switch (BPF_MODE(Inst->code)) {
      case BPF_IMM: return "IMM";
      case BPF_ABS: return "ABS";
      case BPF_IND: return "IND";
      case BPF_MEM: return "MEM";
      case BPF_LEN: return "LEN";
      case BPF_MSH: return "MSH";
      }
      return "Unknown";
    };

    auto LoadName = [](const sock_filter* Inst) {
      using namespace std::string_view_literals;
      switch (BPF_MODE(Inst->code)) {
      case BPF_IMM: return fextl::fmt::format("#{}", Inst->k);
      case BPF_ABS: return fextl::fmt::format("seccomp_data + #{}", Inst->k);
      case BPF_IND: return fextl::fmt::format("Ind[X+#{}]", Inst->k);
      case BPF_MEM: return fextl::fmt::format("Mem[#{}]", Inst->k);
      case BPF_LEN: return fextl::fmt::format("len");
      case BPF_MSH: return fextl::fmt::format("msh");
      }
      return fextl::fmt::format("Unknown");
    };

    LogMan::Msg::IFmt("0x{:04x}: {} <- LD.{} {} {}", BPFIP, DestName(Inst), AccessSize(Inst), ModeType(Inst), LoadName(Inst));
  };

  auto Parse_Class_ST = [](uint32_t BPFIP, const sock_filter* Inst) {
    auto DestName = [](const sock_filter* Inst) {
      if (BPF_CLASS(Inst->code) == BPF_ST) {
        return "A";
      } else {
        return "X";
      }
    };

    LogMan::Msg::IFmt("0x{:04x}: Mem[{}] <- ST.{}", BPFIP, Inst->k, DestName(Inst));
  };

  auto Parse_Class_ALU = [](uint32_t BPFIP, const sock_filter* Inst) {
    auto GetOp = [](const sock_filter* Inst) {
      const auto Op = BPF_OP(Inst->code);

      switch (Op) {
      case BPF_ADD: return "ADD";
      case BPF_SUB: return "SUB";
      case BPF_MUL: return "MUL";
      case BPF_DIV: return "DIV";
      case BPF_OR: return "OR";
      case BPF_AND: return "AND";
      case BPF_LSH: return "LSH";
      case BPF_RSH: return "RSH";
      case BPF_MOD: return "MOD";
      case BPF_XOR: return "XOR";
      case BPF_NEG: return "NEG";
      default: return "Unknown";
      }
    };

    auto GetSrc = [](const sock_filter* Inst) {
      switch (BPF_SRC(Inst->code)) {
      case BPF_K: return fextl::fmt::format("0x{:x}", Inst->k);
      case BPF_X: return fextl::fmt::format("<X>");
      }
      return fextl::fmt::format("Unknown");
    };

    LogMan::Msg::IFmt("0x{:04x}: {} <A>, {}", BPFIP, GetOp(Inst), GetSrc(Inst));
  };

  auto Parse_Class_JMP = [](uint32_t BPFIP, const sock_filter* Inst) {
    auto GetOp = [](const sock_filter* Inst) {
      switch (BPF_OP(Inst->code)) {
      case BPF_JA: return "a";
      case BPF_JEQ: return "eq";
      case BPF_JGT: return "gt";
      case BPF_JGE: return "ge";
      case BPF_JSET: return "set";
      }
      return "Unknown";
    };

    auto GetSrc = [](const sock_filter* Inst) {
      switch (BPF_SRC(Inst->code)) {
      case BPF_K: return fextl::fmt::format("0x{:x}", Inst->k);
      case BPF_X: return fextl::fmt::format("<X>");
      }
      return fextl::fmt::format("Unknown");
    };

    LogMan::Msg::IFmt("0x{:04x}: JMP.{} {}, +{} (#0x{:x}), +{} (#0x{:x})", BPFIP, GetOp(Inst), GetSrc(Inst), Inst->jt, BPFIP + Inst->jt + 1,
                      Inst->jf, BPFIP + Inst->jf + 1);
  };

  auto Parse_Class_RET = [](uint32_t BPFIP, const sock_filter* Inst) {
    auto GetRetValue = [](const sock_filter* Inst) {
      switch (BPF_RVAL(Inst->code)) {
      case BPF_K: {
        uint32_t RetData = Inst->k & SECCOMP_RET_DATA;
        switch (Inst->k & SECCOMP_RET_ACTION_FULL) {
        case SECCOMP_RET_KILL_PROCESS: return fextl::fmt::format("KILL_PROCESS.{}", RetData);
        case SECCOMP_RET_KILL_THREAD: return fextl::fmt::format("KILL_THREAD.{}", RetData);
        case SECCOMP_RET_TRAP: return fextl::fmt::format("TRAP.{}", RetData);
        case SECCOMP_RET_ERRNO: return fextl::fmt::format("ERRNO.{}", RetData);
        case SECCOMP_RET_USER_NOTIF: return fextl::fmt::format("USER_NOTIF.{}", RetData);
        case SECCOMP_RET_TRACE: return fextl::fmt::format("TRACE.{}", RetData);
        case SECCOMP_RET_LOG: return fextl::fmt::format("LOG.{}", RetData);
        case SECCOMP_RET_ALLOW: return fextl::fmt::format("ALLOW.{}", RetData);
        default: break;
        }
        return fextl::fmt::format("<Unknown>.{}", RetData);
      }
      case BPF_X: return fextl::fmt::format("<X>");
      case BPF_A: return fextl::fmt::format("<A>");
      }

      return fextl::fmt::format("Unknown");
    };

    LogMan::Msg::IFmt("0x{:04x}: RET {}", BPFIP, GetRetValue(Inst));
  };

  auto Parse_Class_MISC = [](uint32_t BPFIP, const sock_filter* Inst) {
    const auto MiscOp = BPF_MISCOP(Inst->code);
    switch (MiscOp) {
    case BPF_TAX: LogMan::Msg::IFmt("0x{:04x}: TAX", BPFIP); break;
    case BPF_TXA: LogMan::Msg::IFmt("0x{:04x}: TXA", BPFIP); break;
    default: LogMan::Msg::IFmt("0x{:04x}: Misc: Unknown", BPFIP); break;
    };
  };

  LogMan::Msg::IFmt("BPF program: 0x{:x} instructions", prog->len);

  for (size_t i = 0; i < prog->len; ++i) {
    const sock_filter* Inst = &prog->filter[i];
    const uint16_t Code = Inst->code;
    const uint16_t Class = BPF_CLASS(Code);
    switch (Class) {
    case BPF_LD:
    case BPF_LDX: Parse_Class_LD(i, Inst); break;
    case BPF_ST:
    case BPF_STX: Parse_Class_ST(i, Inst); break;
    case BPF_ALU: Parse_Class_ALU(i, Inst); break;
    case BPF_JMP: Parse_Class_JMP(i, Inst); break;
    case BPF_RET: Parse_Class_RET(i, Inst); break;
    case BPF_MISC: Parse_Class_MISC(i, Inst); break;
    }
  }
}
} // namespace FEX::HLE
