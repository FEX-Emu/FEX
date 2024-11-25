// SPDX-License-Identifier: MIT
/*
$info$
tags: glue|gdbserver
desc: Provides a gdb interface to the guest state
$end_info$
*/

#include "GdbServer/Info.h"

#include <Common/StringUtil.h>

#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Core/X86Enums.h>
#include <FEXCore/fextl/fmt.h>
#include <FEXCore/fextl/sstream.h>
#include <FEXCore/Utils/CompilerDefs.h>
#include <FEXCore/Utils/FileLoading.h>
#include <FEXCore/Utils/LogManager.h>

#include <array>
#include <string_view>

namespace FEX::GDB::Info {
constexpr std::array<std::string_view, 22> FlagNames = {
  "CF", "", "PF", "", "AF", "", "ZF", "SF", "TF", "IF", "DF", "OF", "IOPL", "", "NT", "", "RF", "VM", "AC", "VIF", "VIP", "ID",
};

const std::string_view& GetFlagName(unsigned Bit) {
  LOGMAN_THROW_A_FMT(Bit < 22, "Bit position too large");
  return FlagNames[Bit];
}

std::string_view GetGRegName(unsigned Reg) {
  switch (Reg) {
  case FEXCore::X86State::REG_RAX: return "rax";
  case FEXCore::X86State::REG_RBX: return "rbx";
  case FEXCore::X86State::REG_RCX: return "rcx";
  case FEXCore::X86State::REG_RDX: return "rdx";
  case FEXCore::X86State::REG_RSP: return "rsp";
  case FEXCore::X86State::REG_RBP: return "rbp";
  case FEXCore::X86State::REG_RSI: return "rsi";
  case FEXCore::X86State::REG_RDI: return "rdi";
  case FEXCore::X86State::REG_R8: return "r8";
  case FEXCore::X86State::REG_R9: return "r9";
  case FEXCore::X86State::REG_R10: return "r10";
  case FEXCore::X86State::REG_R11: return "r11";
  case FEXCore::X86State::REG_R12: return "r12";
  case FEXCore::X86State::REG_R13: return "r13";
  case FEXCore::X86State::REG_R14: return "r14";
  case FEXCore::X86State::REG_R15: return "r15";
  default: FEX_UNREACHABLE;
  }
}

fextl::string GetThreadName(uint32_t PID, uint32_t ThreadID) {
  const auto ThreadFile = fextl::fmt::format("/proc/{}/task/{}/comm", PID, ThreadID);
  fextl::string ThreadName;
  FEXCore::FileLoading::LoadFile(ThreadName, ThreadFile);
  // Trim out the potential newline, breaks GDB if it exists.
  FEX::StringUtil::trim(ThreadName);
  return ThreadName;
}

fextl::string BuildOSXML() {
  fextl::ostringstream xml;

  xml << "<?xml version='1.0'?>\n";

  xml << "<!DOCTYPE target SYSTEM \"osdata.dtd\">\n";
  xml << "<osdata type=\"processes\">";
  // XXX
  xml << "</osdata>";

  xml << std::flush;

  return xml.str();
}

fextl::string BuildTargetXML() {
  fextl::ostringstream xml;

  xml << "<?xml version='1.0'?>\n";
  xml << "<!DOCTYPE target SYSTEM 'gdb-target.dtd'>\n";
  xml << "<target>\n";
  xml << "<architecture>i386:x86-64</architecture>\n";
  xml << "<osabi>GNU/Linux</osabi>\n";
  xml << "<feature name='org.gnu.gdb.i386.core'>\n";

  xml << "<flags id='fex_eflags' size='4'>\n";
  // flags register
  for (int i = 0; i < 22; i++) {
    auto name = GDB::Info::GetFlagName(i);
    if (name.empty()) {
      continue;
    }
    xml << "\t<field name='" << name << "' start='" << i << "' end='" << i << "' />\n";
  }
  xml << "</flags>\n";

  int32_t TargetSize {};
  auto reg = [&](std::string_view name, std::string_view type, int size) {
    TargetSize += size;
    xml << "<reg name='" << name << "' bitsize='" << size << "' type='" << type << "' />" << std::endl;
  };

  // Register ordering.
  // We want to just memcpy our x86 state to gdb, so we tell it the ordering.

  // GPRs
  for (uint32_t i = 0; i < FEXCore::Core::CPUState::NUM_GPRS; i++) {
    reg(GDB::Info::GetGRegName(i), "int64", 64);
  }

  reg("rip", "code_ptr", 64);

  reg("eflags", "fex_eflags", 32);

  // Fake registers which GDB requires, but we don't support;
  // We stick them past the end of our cpu state.

  // non-userspace segment registers
  reg("cs", "int32", 32);
  reg("ss", "int32", 32);
  reg("ds", "int32", 32);
  reg("es", "int32", 32);

  reg("fs", "int32", 32);
  reg("gs", "int32", 32);

  // x87 stack
  for (int i = 0; i < 8; i++) {
    reg(fextl::fmt::format("st{}", i), "i387_ext", 80);
  }

  // x87 control
  reg("fctrl", "int32", 32);
  reg("fstat", "int32", 32);
  reg("ftag", "int32", 32);
  reg("fiseg", "int32", 32);
  reg("fioff", "int32", 32);
  reg("foseg", "int32", 32);
  reg("fooff", "int32", 32);
  reg("fop", "int32", 32);


  xml << "</feature>\n";
  xml << "<feature name='org.gnu.gdb.i386.sse'>\n";
  xml <<
    R"(<vector id="v4f" type="ieee_single" count="4"/>
        <vector id="v2d" type="ieee_double" count="2"/>
        <vector id="v16i8" type="int8" count="16"/>
        <vector id="v8i16" type="int16" count="8"/>
        <vector id="v4i32" type="int32" count="4"/>
        <vector id="v2i64" type="int64" count="2"/>
        <union id="vec128">
          <field name="v4_float" type="v4f"/>
          <field name="v2_double" type="v2d"/>
          <field name="v16_int8" type="v16i8"/>
          <field name="v8_int16" type="v8i16"/>
          <field name="v4_int32" type="v4i32"/>
          <field name="v2_int64" type="v2i64"/>
          <field name="uint128" type="uint128"/>
        </union>
        )";

  // SSE regs
  for (size_t i = 0; i < FEXCore::Core::CPUState::NUM_XMMS; i++) {
    reg(fextl::fmt::format("xmm{}", i), "vec128", 128);
  }

  reg("mxcsr", "int", 32);

  xml << "</feature>\n";

  xml << "<feature name='org.gnu.gdb.i386.avx'>";
  xml <<
    R"(<vector id="v4f" type="ieee_single" count="4"/>
        <vector id="v2d" type="ieee_double" count="2"/>
        <vector id="v16i8" type="int8" count="16"/>
        <vector id="v8i16" type="int16" count="8"/>
        <vector id="v4i32" type="int32" count="4"/>
        <vector id="v2i64" type="int64" count="2"/>
        <union id="vec128">
          <field name="v4_float" type="v4f"/>
          <field name="v2_double" type="v2d"/>
          <field name="v16_int8" type="v16i8"/>
          <field name="v8_int16" type="v8i16"/>
          <field name="v4_int32" type="v4i32"/>
          <field name="v2_int64" type="v2i64"/>
          <field name="uint128" type="uint128"/>
        </union>
        )";
  for (size_t i = 0; i < FEXCore::Core::CPUState::NUM_XMMS; i++) {
    reg(fmt::format("ymm{}h", i), "vec128", 128);
  }
  xml << "</feature>\n";

  xml << "</target>";
  xml << std::flush;

  return xml.str();
}

} // namespace FEX::GDB::Info
