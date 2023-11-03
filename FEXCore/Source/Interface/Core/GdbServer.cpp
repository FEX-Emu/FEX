// SPDX-License-Identifier: MIT
/*
$info$
tags: glue|gdbserver
desc: Provides a gdb interface to the guest state
$end_info$
*/

#include <cstdlib>
#include <cstdio>
#include <iomanip>
#include <memory>
#include <optional>
#include "Common/SoftFloat.h"
#include "Interface/Context/Context.h"

#include <FEXCore/Config/Config.h>
#include <FEXCore/Core/CodeLoader.h>
#include <FEXCore/Core/Context.h>
#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Core/SignalDelegator.h>
#include <FEXCore/Core/X86Enums.h>
#include <FEXCore/Debug/InternalThreadState.h>
#include <FEXCore/HLE/Linux/ThreadManagement.h>
#include <FEXCore/HLE/SyscallHandler.h>
#include <FEXCore/Utils/CompilerDefs.h>
#include <FEXCore/Utils/FileLoading.h>
#include <FEXCore/Utils/NetStream.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/StringUtils.h>
#include <FEXCore/Utils/Threads.h>
#include <FEXCore/fextl/fmt.h>
#include <FEXCore/fextl/map.h>
#include <FEXCore/fextl/sstream.h>
#include <FEXCore/fextl/string.h>
#include <FEXCore/fextl/vector.h>

#include <atomic>
#include <cstring>
#ifndef _WIN32
#include <elf.h>
#include <netdb.h>
#include <sys/socket.h>
#endif
#include <errno.h>
#include <fcntl.h>
#include <fmt/format.h>
#include <signal.h>
#include <stddef.h>
#include <string_view>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>

#include "GdbServer.h"

namespace FEXCore
{
#ifndef _WIN32
void GdbServer::Break(int signal) {
  std::lock_guard lk(sendMutex);
  if (!CommsStream) {
    return;
  }

  const fextl::string str = fextl::fmt::format("S{:02x}", signal);
  SendPacket(*CommsStream, str);
}

void GdbServer::WaitForThreadWakeup() {
  // Wait for gdbserver to tell us to wake up
  ThreadBreakEvent.Wait();
}

GdbServer::GdbServer(FEXCore::Context::ContextImpl *ctx) : CTX(ctx) {
  // Pass all signals by default
  std::fill(PassSignals.begin(), PassSignals.end(), true);

  ctx->SetExitHandler([this](uint64_t ThreadId, FEXCore::Context::ExitReason ExitReason) {
    if (ExitReason == FEXCore::Context::ExitReason::EXIT_DEBUG) {
      this->Break(SIGTRAP);
    }
  });

  // This is a total hack as there is currently no way to resume once hitting a segfault
  // But it's semi-useful for debugging.
  for (uint32_t Signal = 0; Signal <= SignalDelegator::MAX_SIGNALS; ++Signal) {
    ctx->SignalDelegation->RegisterHostSignalHandler(Signal, [this] (FEXCore::Core::InternalThreadState *Thread, int Signal, void *info, void *ucontext) {
      if (PassSignals[Signal]) {
        // Pass signal to the guest
        return false;
      }

      this->CTX->Config.RunningMode = FEXCore::Context::CoreRunningMode::MODE_SINGLESTEP;

      // Let GDB know that we have a signal
      this->Break(Signal);

      WaitForThreadWakeup();

      return true;
    }, true);
  }

  StartThread();
}

static int calculateChecksum(const fextl::string &packet) {
  unsigned char checksum = 0;
  for (const char &c : packet) {
    checksum += c;
  }
  return checksum;
}

static fextl::string hexstring(fextl::istringstream &ss, int delm) {
  fextl::string ret;

  char hexString[3] = {0, 0, 0};
  while (ss.peek() != delm) {
    ss.read(hexString, 2);
    int c = std::strtoul(hexString, nullptr, 16);
    ret.push_back((char) c);
  }

  if (delm != -1)
    ss.get();

  return ret;
}

static fextl::string encodeHex(const unsigned char *data, size_t length) {
  fextl::ostringstream ss;

  for (size_t i=0; i < length; i++) {
    ss << std::setfill('0') << std::setw(2) << std::hex << int(data[i]);
  }
  return ss.str();
}

static fextl::string getThreadName(uint32_t ThreadID) {
  const auto ThreadFile = fextl::fmt::format("/proc/{}/task/{}/comm", getpid(), ThreadID);
  fextl::string ThreadName;
  FEXCore::FileLoading::LoadFile(ThreadName, ThreadFile);
  return ThreadName;
}

// Packet parser
// Takes a serial stream and reads a single packet
// Un-escapes chars, checks the checksum and request a retransmit if it fails.
// Once the checksum is validated, it acknowledges and returns the packet in a string
fextl::string GdbServer::ReadPacket(std::iostream &stream) {
  fextl::string packet{};

  // The GDB "Remote Serial Protocal" was originally 7bit clean for use on serial ports.
  // Binary data is useally hex encoded. However some later extentions just put
  // raw 8bit binary data.

  // Packets are in the format
  // $<data>#<checksum>
  // where any $ or # in the packet body are escaped ('}' followed by the char XORed with 0x20)
  // The checksum is a single unsigned byte sum of the data, hex encoded.

  int c;
  while ((c = stream.get()) > 0 ) {
    switch(c) {
      case '$': // start of packet
        if (packet.size() != 0)
          LogMan::Msg::EFmt("Dropping unexpected data: \"{}\"", packet);

        // clear any existing data, must have been a mistake.
        packet = fextl::string();
        break;
      case '}': // escape char
      {
        char escaped;
        stream >> escaped;
        packet.push_back(escaped ^ 0x20);
        break;
      }
      case '#': // end of packet
      {
        char hexString[3] = {0, 0, 0};
        stream.read(hexString, 2);
        int expected_checksum = std::strtoul(hexString, nullptr, 16);

        if (calculateChecksum(packet) == expected_checksum) {
          return packet;
        } else {
          LogMan::Msg::EFmt("Received Invalid Packet: ${}#{:02x}", packet, expected_checksum);
        }
        break;
      }
      default:
        packet.push_back((char) c);
        break;
    }
  }

  return "";
}

static fextl::string escapePacket(const fextl::string& packet) {
  fextl::ostringstream ss;

  for(const auto &c : packet) {
    switch (c) {
      case '$':
      case '#':
      case '*':
      case '}': {
        char escaped = c ^ 0x20;
        ss << '}' << (escaped);
        break;
      }
      default:
        ss << c;
        break;
    }
  }

  return ss.str();
}

void GdbServer::SendPacket(std::ostream &stream, const fextl::string& packet) {
  const auto escaped = escapePacket(packet);
  const auto str = fextl::fmt::format("${}#{:02x}", escaped, calculateChecksum(escaped));

  stream << str << std::flush;
}

void GdbServer::SendACK(std::ostream &stream, bool NACK) {
  if (NoAckMode) {
    return;
  }

  if (NACK) {
    stream << "-" << std::flush;
  }
  else {
    stream << "+" << std::flush;
  }

  if (SettingNoAckMode) {
    NoAckMode = true;
    SettingNoAckMode = false;
  }
}

struct FEX_PACKED GDBContextDefinition {
  uint64_t gregs[Core::CPUState::NUM_GPRS];
  uint64_t rip;
  uint32_t eflags;
  uint32_t cs, ss, ds, es, fs, gs;
  X80SoftFloat mm[Core::CPUState::NUM_MMS];
  uint32_t fctrl;
  uint32_t fstat;
  uint32_t dummies[6];
  uint64_t xmm[Core::CPUState::NUM_XMMS][4];
  uint32_t mxcsr;
};

fextl::string GdbServer::readRegs() {
  GDBContextDefinition GDB{};
  FEXCore::Core::CPUState state{};

  auto Threads = CTX->GetThreads();
  FEXCore::Core::InternalThreadState *CurrentThread { CTX->ParentThread };
  bool Found = false;

  for (auto &Thread : *Threads) {
    if (Thread->ThreadManager.GetTID() != CurrentDebuggingThread) {
      continue;
    }
    memcpy(&state, Thread->CurrentFrame, sizeof(state));
    CurrentThread = Thread;
    Found = true;
    break;
  }

  if (!Found) {
    // If set to an invalid thread then just get the parent thread ID
    memcpy(&state, CTX->ParentThread->CurrentFrame, sizeof(state));
  }

  // Encode the GDB context definition
  memcpy(&GDB.gregs[0], &state.gregs[0], sizeof(GDB.gregs));
  memcpy(&GDB.rip, &state.rip, sizeof(GDB.rip));

  GDB.eflags = CTX->ReconstructCompactedEFLAGS(CurrentThread);

  for (size_t i = 0; i < Core::CPUState::NUM_MMS; ++i) {
    memcpy(&GDB.mm[i], &state.mm[i], sizeof(GDB.mm));
  }

  // Currently unsupported
  GDB.fctrl = 0x37F;

  GDB.fstat  = static_cast<uint32_t>(state.flags[FEXCore::X86State::X87FLAG_TOP_LOC]) << 11;
  GDB.fstat |= static_cast<uint32_t>(state.flags[FEXCore::X86State::X87FLAG_C0_LOC]) << 8;
  GDB.fstat |= static_cast<uint32_t>(state.flags[FEXCore::X86State::X87FLAG_C1_LOC]) << 9;
  GDB.fstat |= static_cast<uint32_t>(state.flags[FEXCore::X86State::X87FLAG_C2_LOC]) << 10;
  GDB.fstat |= static_cast<uint32_t>(state.flags[FEXCore::X86State::X87FLAG_C3_LOC]) << 14;

  memcpy(&GDB.xmm[0], &state.xmm.avx.data[0], sizeof(GDB.xmm));

  return encodeHex((unsigned char *)&GDB, sizeof(GDBContextDefinition));
}

GdbServer::HandledPacketType GdbServer::readReg(const fextl::string& packet) {
  size_t addr;
  auto ss = fextl::istringstream(packet);
  ss.get(); // Drop first letter
  ss >> std::hex >> addr;

  FEXCore::Core::CPUState state{};

  auto Threads = CTX->GetThreads();
  FEXCore::Core::InternalThreadState *CurrentThread { CTX->ParentThread };
  bool Found = false;

  for (auto &Thread : *Threads) {
    if (Thread->ThreadManager.GetTID() != CurrentDebuggingThread) {
      continue;
    }
    memcpy(&state, Thread->CurrentFrame, sizeof(state));
    CurrentThread = Thread;
    Found = true;
    break;
  }

  if (!Found) {
    // If set to an invalid thread then just get the parent thread ID
    memcpy(&state, CTX->ParentThread->CurrentFrame, sizeof(state));
  }


  if (addr >= offsetof(GDBContextDefinition, gregs[0]) &&
      addr < offsetof(GDBContextDefinition, gregs[16])) {
    return {encodeHex((unsigned char *)(&state.gregs[addr / sizeof(uint64_t)]), sizeof(uint64_t)), HandledPacketType::TYPE_ACK};
  }
  else if (addr == offsetof(GDBContextDefinition, rip)) {
    return {encodeHex((unsigned char *)(&state.rip), sizeof(uint64_t)), HandledPacketType::TYPE_ACK};
  }
  else if (addr == offsetof(GDBContextDefinition, eflags)) {
    uint32_t eflags = CTX->ReconstructCompactedEFLAGS(CurrentThread);

    return {encodeHex((unsigned char *)(&eflags), sizeof(uint32_t)), HandledPacketType::TYPE_ACK};
  }
  else if (addr >= offsetof(GDBContextDefinition, cs) &&
           addr < offsetof(GDBContextDefinition, mm[0])) {
    uint32_t Empty{};
    return {encodeHex((unsigned char *)(&Empty), sizeof(uint32_t)), HandledPacketType::TYPE_ACK};
  }
  else if (addr >= offsetof(GDBContextDefinition, mm[0]) &&
           addr < offsetof(GDBContextDefinition, mm[8])) {
    return {encodeHex((unsigned char *)(&state.mm[(addr - offsetof(GDBContextDefinition, mm[0])) / sizeof(X80SoftFloat)]), sizeof(X80SoftFloat)), HandledPacketType::TYPE_ACK};
  }
  else if (addr == offsetof(GDBContextDefinition, fctrl)) {
    // XXX: We don't support this yet
    uint32_t FCW = 0x37F;
    return {encodeHex((unsigned char *)(&FCW), sizeof(uint32_t)), HandledPacketType::TYPE_ACK};
  }
  else if (addr == offsetof(GDBContextDefinition, fstat)) {
    uint32_t FSW{};
    FSW = static_cast<uint32_t>(state.flags[FEXCore::X86State::X87FLAG_TOP_LOC]) << 11;
    FSW |= static_cast<uint32_t>(state.flags[FEXCore::X86State::X87FLAG_C0_LOC]) << 8;
    FSW |= static_cast<uint32_t>(state.flags[FEXCore::X86State::X87FLAG_C1_LOC]) << 9;
    FSW |= static_cast<uint32_t>(state.flags[FEXCore::X86State::X87FLAG_C2_LOC]) << 10;
    FSW |= static_cast<uint32_t>(state.flags[FEXCore::X86State::X87FLAG_C3_LOC]) << 14;
    return {encodeHex((unsigned char *)(&FSW), sizeof(uint32_t)), HandledPacketType::TYPE_ACK};
  }
  else if (addr >= offsetof(GDBContextDefinition, dummies[0]) &&
           addr < offsetof(GDBContextDefinition, dummies[6])) {
    uint32_t Empty{};
    return {encodeHex((unsigned char *)(&Empty), sizeof(uint32_t)), HandledPacketType::TYPE_ACK};
  }
  else if (addr >= offsetof(GDBContextDefinition, xmm[0][0]) &&
           addr < offsetof(GDBContextDefinition, xmm[16][0])) {
    const auto XmmIndex = (addr - offsetof(GDBContextDefinition, xmm[0][0])) / Core::CPUState::XMM_AVX_REG_SIZE;
    const auto *Data = (unsigned char *)&state.xmm.avx.data[XmmIndex][0];
    return {encodeHex(Data, Core::CPUState::XMM_AVX_REG_SIZE), HandledPacketType::TYPE_ACK};
  }
  else if (addr == offsetof(GDBContextDefinition, mxcsr)) {
    uint32_t Empty{};
    return {encodeHex((unsigned char *)(&Empty), sizeof(uint32_t)), HandledPacketType::TYPE_ACK};
  }

  LogMan::Msg::EFmt("Unknown GDB register 0x{:x}", addr);
  return {"E00", HandledPacketType::TYPE_ACK};
}

fextl::string buildTargetXML() {
  fextl::ostringstream xml;

  xml << "<?xml version='1.0'?>\n";
  xml << "<!DOCTYPE target SYSTEM 'gdb-target.dtd'>\n";
  xml << "<target>\n";
  xml << "<architecture>i386:x86-64</architecture>\n";
  xml << "<osabi>GNU/Linux</osabi>\n";
    xml << "<feature name='org.gnu.gdb.i386.core'>\n";

      xml << "<flags id='fex_eflags' size='4'>\n";
      // flags register
      for(int i = 0; i < 22; i++) {
          auto name = FEXCore::Core::GetFlagName(i);
          if (name.empty()) {
            continue;
          }
          xml << "\t<field name='" << name << "' start='" << i << "' end='" << i << "' />\n";
      }
      xml << "</flags>\n";

      int32_t TargetSize{};
      auto reg = [&](std::string_view name, std::string_view type, int size) {
        TargetSize += size;
        xml << "<reg name='" << name << "' bitsize='" << size << "' type='" << type << "' />" << std::endl;
      };

      // Register ordering.
      // We want to just memcpy our x86 state to gdb, so we tell it the ordering.

      // GPRs
      for (uint32_t i = 0; i < Core::CPUState::NUM_GPRS; i++) {
        reg(FEXCore::Core::GetGRegName(i), "int64", 64);
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
      for (int i=0; i < 8; i++) {
        reg(fextl::fmt::format("st{}", i), "i387_ext", 80);
      }

      // x87 control
      reg("fctrl", "int32", 32);
      reg("fstat", "int32", 32);
      reg("ftag",  "int32", 32);
      reg("fiseg", "int32", 32);
      reg("fioff", "int32", 32);
      reg("foseg", "int32", 32);
      reg("fooff", "int32", 32);
      reg("fop",   "int32", 32);


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
      for (size_t i = 0; i < Core::CPUState::NUM_XMMS; i++) {
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
    for (size_t i = 0; i < Core::CPUState::NUM_XMMS; i++) {
        reg(fmt::format("ymm{}h", i), "vec128", 128);
    }
    xml << "</feature>\n";

  xml << "</target>";
  xml << std::flush;

  return xml.str();
}

fextl::string buildOSData() {
  fextl::ostringstream xml;

  xml << "<?xml version='1.0'?>\n";

  xml << "<!DOCTYPE target SYSTEM \"osdata.dtd\">\n";
  xml << "<osdata type=\"processes\">";
  // XXX
  xml << "</osdata>";

  xml << std::flush;

  return xml.str();
}

void GdbServer::buildLibraryMap() {
  if (!LibraryMapChanged) {
    // No need to update
    return;
  }

  fextl::ostringstream xml;

  fextl::string MapsFile;
  FEXCore::FileLoading::LoadFile(MapsFile, "/proc/self/maps");
  fextl::istringstream MapsStream(MapsFile);

  fextl::string Line;

  struct FileData {
    uint64_t Begin;
  };

  fextl::map<fextl::string, fextl::vector<FileData>> SegmentMaps;

  // 7ff5dd6d2000-7ff5dd6d3000 rw-p 0000a000 103:0b 1881447                   /usr/lib/x86_64-linux-gnu/libnss_compat.so.2
  fextl::string const &RuntimeExecutable = Filename();
  while (std::getline(MapsStream, Line)) {
    auto ss = fextl::istringstream(Line);
    fextl::string Tmp;
    fextl::string Begin;
    fextl::string Name;
    std::getline(ss, Begin, '-');
    std::getline(ss, Tmp, ' '); // End
    std::getline(ss, Tmp, ' '); // Perm
    std::getline(ss, Tmp, ' '); // Inode
    std::getline(ss, Tmp, ' '); // devid
    std::getline(ss, Tmp, ' '); // Some garbage
    std::getline(ss, Name, '\n');

    if (strstr(Name.c_str(), "aarch64") != nullptr) {
      // If the library comes from aarch64, just skip it
      // Reduces the amount of memory gdb fetches
      continue;
    }

    Name = FEXCore::StringUtils::Trim(Name);

    struct stat sb{};
    if (stat(Name.c_str(), &sb) != -1) {
      if (S_ISCHR(sb.st_mode)) {
        // Skip this special file type
        // Fixes GDB trying to read dri render nodes
        continue;
      }
    }

    // Skip empty entries, the entry from the process, and also anything like [heap]
    if (!Name.empty() && Name != RuntimeExecutable && Name[0] != '[') {
      FileData data {
        .Begin = std::strtoul(Begin.c_str(), nullptr, 16),
      };

      SegmentMaps[Name].emplace_back(data);
    }
  }

  xml << "<library-list>\n";
  for (auto &Array : SegmentMaps) {
    xml << "\t<library name=\"" << Array.first << "\">\n";
    for (auto &Data : Array.second) {
      xml << "\t\t<segment address=\"0x" << std::hex << Data.Begin << "\"/>\n";
    }
    xml << "\t</library>\n";
  }

  xml << "</library-list>\n";

  LibraryMapString = xml.str();
  LibraryMapChanged = false;
}

GdbServer::HandledPacketType GdbServer::handleXfer(const fextl::string &packet) {
  fextl::string object;
  fextl::string rw;
  fextl::string annex;
  int annex_pid;
  int offset;
  int length;

  // Parse Xfer message
  {
    auto ss = fextl::istringstream(packet);
    fextl::string expectXfer;
    char expectComma;

    std::getline(ss, expectXfer, ':');
    std::getline(ss, object, ':');
    std::getline(ss, rw, ':');
    std::getline(ss, annex, ':');
    if (annex == "") {
      annex_pid = getpid();
    }
    else {
      auto ss_pid = fextl::istringstream(annex);
      ss_pid >> std::hex >> annex_pid;
    }
    ss >> std::hex >> offset;
    ss.get(expectComma);
    ss >> std::hex >> length;

    // Bail on any errors
    if (ss.fail() || !ss.eof() || expectXfer != "qXfer" || rw != "read" || expectComma != ',')
      return {"E00", HandledPacketType::TYPE_ACK};
  }

  // Lambda to correctly encode any reply
  auto encode = [&](fextl::string data) -> fextl::string {
    if (offset == data.size())
      return "l";
    if (offset >= data.size())
      return "E34"; // ERANGE
    if ((data.size() - offset) > length)
      return "m" + data.substr(offset, length);
    return "l" + data.substr(offset);
  };

  if (object == "exec-file") {
    if (annex_pid == getpid()) {
      return {encode(Filename()), HandledPacketType::TYPE_ACK};
    }

    return {"E00", HandledPacketType::TYPE_ACK};
  }

  if (object == "features") {
    if (annex == "target.xml")
      return {encode(buildTargetXML()), HandledPacketType::TYPE_ACK};

    return {"E00", HandledPacketType::TYPE_ACK};
  }

  if (object == "threads") {
    if (offset == 0) {
      auto Threads = CTX->GetThreads();

      ThreadString.clear();
      fextl::ostringstream ss;
      ss << "<threads>\n";
      for (auto &Thread : *Threads) {
        // Thread id is in hex without 0x prefix
        const auto ThreadName = getThreadName(Thread->ThreadManager.GetTID());
        ss << "<thread id=\"" << std::hex << Thread->ThreadManager.GetTID() << "\"";
        if (!ThreadName.empty()) {
          ss << " name=\"" << ThreadName << "\"";
        }
        ss << "/>\n";
      }

      ss << "</threads>\n";
      ss << std::flush;
      ThreadString = ss.str();
    }

    return {encode(ThreadString), HandledPacketType::TYPE_ACK};
  }

  if (object == "osdata") {
    if (offset == 0) {
      OSDataString = buildOSData();
    }
    return {encode(OSDataString), HandledPacketType::TYPE_ACK};
  }

  if (object == "libraries") {
    if (offset == 0) {
      // Attempt to rebuild when reading from zero
      buildLibraryMap();
    }
    return {encode(LibraryMapString), HandledPacketType::TYPE_ACK};
  }

  if (object == "auxv") {
    auto CodeLoader = CTX->SyscallHandler->GetCodeLoader();
    uint64_t auxv_ptr, auxv_size;
    CodeLoader->GetAuxv(auxv_ptr, auxv_size);
    fextl::string data;
    if (CTX->Config.Is64BitMode) {
      data.resize(auxv_size);
      memcpy(data.data(), reinterpret_cast<void*>(auxv_ptr), data.size());
    }
    else {
      // We need to transcode from 32-bit auxv_t to 64-bit
      data.resize(auxv_size / sizeof(Elf32_auxv_t) * sizeof(Elf64_auxv_t));
      size_t NumAuxv = auxv_size / sizeof(Elf32_auxv_t);
      for (size_t i = 0; i < NumAuxv; ++i) {
        Elf32_auxv_t *auxv = reinterpret_cast<Elf32_auxv_t*>(auxv_ptr + i * sizeof(Elf32_auxv_t));
        Elf64_auxv_t tmp;
        tmp.a_type = auxv->a_type;
        tmp.a_un.a_val = auxv->a_un.a_val;
        memcpy(data.data() + i * sizeof(Elf64_auxv_t), &tmp, sizeof(Elf64_auxv_t));
      }
    }

    return {encode(data), HandledPacketType::TYPE_ACK};
  }

  return {"", HandledPacketType::TYPE_UNKNOWN};
}

static size_t CheckMemMapping(uint64_t Address, size_t Size) {
  uint64_t AddressEnd = Address + Size;
  fextl::string MapsFile;
  FEXCore::FileLoading::LoadFile(MapsFile, "/proc/self/maps");
  fextl::istringstream MapsStream(MapsFile);

  fextl::string Line;

  while (std::getline(MapsStream, Line)) {
    if (MapsStream.eof()) break;
    uint64_t Begin, End;
    char r,w,x,p;
    if (sscanf(Line.c_str(), "%lx-%lx %c%c%c%c", &Begin, &End, &r, &w, &x, &p) == 6) {
      if (Begin <= Address &&
          End > Address) {
        ssize_t Overrun{};
        if (AddressEnd > End) {
          Overrun = AddressEnd - End;
        }
        return Size - Overrun;
      }
    }
  }

  return 0;
}

GdbServer::HandledPacketType GdbServer::handleProgramOffsets() {
  auto CodeLoader = CTX->SyscallHandler->GetCodeLoader();
  uint64_t BaseOffset = CodeLoader->GetBaseOffset();
  fextl::string str = fextl::fmt::format("Text={:x};Data={:x};Bss={:x}", BaseOffset, BaseOffset, BaseOffset);
  return {std::move(str), HandledPacketType::TYPE_ACK};
}

GdbServer::HandledPacketType GdbServer::handleMemory(const fextl::string &packet) {
  bool write;
  size_t addr;
  size_t length;
  fextl::string data;

  auto ss = fextl::istringstream(packet);
  write = ss.get() == 'M';
  ss >> std::hex >> addr;
  ss.get(); // discard comma
  ss >> std::hex >> length;

  if (write) {
    ss.get(); // discard colon
    data = hexstring(ss, -1); // grab data until end of file.
  }

  // validate packet
  if (ss.fail() || !ss.eof() || (write && (data.length() != length))) {
    return {"E00", HandledPacketType::TYPE_ACK};
  }

  length = CheckMemMapping(addr, length);
  if (length == 0) {
    return {"E00", HandledPacketType::TYPE_ACK};
  }

  // TODO: check we are in a valid memory range
  //       Also, clamp length
  void* ptr = reinterpret_cast<void*>(addr);

  if (write) {
    std::memcpy(ptr, data.data(), data.length());
    // TODO: invalidate any code
    return {"OK", HandledPacketType::TYPE_ACK};
  } else {
    return {encodeHex((unsigned char*)ptr, length), HandledPacketType::TYPE_ACK};
  }
}


GdbServer::HandledPacketType GdbServer::handleQuery(const fextl::string &packet) {
  const auto match = [&](const char *str) -> bool { return packet.rfind(str, 0) == 0; };
  const auto MatchStr = [](const fextl::string &Str, const char *str) -> bool { return Str.rfind(str, 0) == 0; };

  const auto split = [](const fextl::string &Str, char deliminator) -> fextl::vector<fextl::string> {
    fextl::vector<fextl::string> Elements;
    fextl::istringstream Input(Str);
    for (fextl::string line;
         std::getline(Input, line);
         Elements.emplace_back(line));
    return Elements;
  };

  if (match("QNonStop:")) {
    auto ss = fextl::istringstream(packet);
    ss.seekg(fextl::string("QNonStop:").size());
    ss.get(); // discard colon
    ss >> NonStopMode;
    return {"OK", HandledPacketType::TYPE_ACK};
  }
  if (match("qSupported:")) {
    // eg: qSupported:multiprocess+;swbreak+;hwbreak+;qRelocInsn+;fork-events+;vfork-events+;exec-events+;vContSupported+;QThreadEvents+;no-resumed+;memory-tagging+;xmlRegisters=i386
    auto Features = split(packet.substr(strlen("qSupported:")), ';');

    // For feature documentation
    // https://sourceware.org/gdb/current/onlinedocs/gdb/General-Query-Packets.html#qSupported
    fextl::string SupportedFeatures{};

    // Required features
    SupportedFeatures += "PacketSize=32768;";
    SupportedFeatures += "xmlRegisters=i386;";

    SupportedFeatures += "qXfer:auxv:read+;";
    SupportedFeatures += "qXfer:exec-file:read+;";
    SupportedFeatures += "qXfer:features:read+;";
    SupportedFeatures += "qXfer:libraries:read+;";
    // Don't enable this feature. If enabled then gdb doesn't query for
    // memory-map updates post-launch. Resulting in the inability to
    // disassemble code from loaded libraries.
    // gdbserver running on a true host also doesn't use this feature.
    // It is likely used for embedded environments where you have a fixed
    // memory map.
    // SupportedFeatures += "qXfer:memory-map:read+;";
    SupportedFeatures += "qXfer:siginfo:read+;";
    SupportedFeatures += "qXfer:siginfo:write+;";
    SupportedFeatures += "qXfer:threads:read+;";
    SupportedFeatures += "QCatchSignals+;";
    SupportedFeatures += "QPassSignals+;";
    SupportedFeatures += "QNonStop+;";

    SupportedFeatures += "qXfer:osdata:read+;";

    // Causes GDB to crash?
    // SupportedFeatures += "QStartNoAckMode+;";

    for (auto &Feature : Features) {

      if (MatchStr(Feature, "swbreak+")) {
        SupportedFeatures += "swbreak+;";
      }
      if (MatchStr(Feature, "hwbreak+")) {
        SupportedFeatures += "hwbreak+;";
      }
      if (MatchStr(Feature, "vContSupported+")) {
        SupportedFeatures += "vContSupported+;";
      }

      // Unsupported:
      //  multiprocess
      //  qRelocInsn
      //  fork-events
      //  vfork-events
      //  exec-events
      //  QThreadEvents
      //  no-resumed
      //  memory-tagging
    }
    return {SupportedFeatures, HandledPacketType::TYPE_ACK};
  }
  if (match("qAttached")) {
    return {"tnotrun:0", HandledPacketType::TYPE_ACK}; // We don't currently support launching executables from gdb.
  }
  if (match("qXfer")) {
    return handleXfer(packet);
  }
  if (match("qOffsets")) {
    return handleProgramOffsets();
  }
  if (match("qTStatus")) {
    // We don't support trace experiments
    return {"", HandledPacketType::TYPE_ACK};
  }
  if (match("qfThreadInfo")) {
    auto Threads = CTX->GetThreads();

    fextl::ostringstream ss;
    ss << "m";
    for (size_t i = 0; i < Threads->size(); ++i) {
      auto Thread = Threads->at(i);
      ss << std::hex << Thread->ThreadManager.TID;
      if (i != (Threads->size() - 1)) {
        ss << ",";
      }
    }
    return {ss.str(), HandledPacketType::TYPE_ACK};
  }
  if (match("qsThreadInfo")) {
    return {"l", HandledPacketType::TYPE_ACK};
  }
  if (match("qThreadExtraInfo")) {
    auto ss = fextl::istringstream(packet);
    ss.seekg(fextl::string("qThreadExtraInfo").size());
    ss.get(); // discard comma
    uint32_t ThreadID;
    ss >> std::hex >> ThreadID;
    auto ThreadName = getThreadName(ThreadID);
    return {encodeHex((unsigned char*)ThreadName.data(), ThreadName.size()), HandledPacketType::TYPE_ACK};
  }
  if (match("qC")) {
    // Returns the current Thread ID
    fextl::ostringstream ss;
    ss << "m" <<  std::hex << CTX->ParentThread->ThreadManager.TID;
    return {ss.str(), HandledPacketType::TYPE_ACK};
  }
  if (match("QStartNoAckMode")) {
    SettingNoAckMode = true;
    return {"OK", HandledPacketType::TYPE_ACK};
  }
  if (match("qSymbol")) {
    auto ss = fextl::istringstream(packet);
    ss.seekg(fextl::string("qSymbol").size());
    ss.get(); // discard colon
    fextl::string Symbol_Val, Symbol_name;
    std::getline(ss, Symbol_Val, ':');
    std::getline(ss, Symbol_name, ':');

    if (Symbol_Val.empty() && Symbol_name.empty()) {
      return {"OK", HandledPacketType::TYPE_ACK};
    }
    else {
      return {"", HandledPacketType::TYPE_UNKNOWN};
    }
  }

  if (match("QPassSignals")) {
    // First set all signals as unpassed
    std::fill(PassSignals.begin(), PassSignals.end(), false);

    // eg: QPassSignals:e;10;14;17;1a;1b;1c;21;24;25;2c;4c;97;
    auto ss = fextl::istringstream(packet);
    ss.seekg(fextl::string("QPassSignals").size());
    ss.get(); // discard colon

    // We now have a semi-colon deliminated list of signals to pass to the guest process
    for (fextl::string tmp; std::getline(ss, tmp, ';'); ) {
      uint32_t Signal = std::stoi(tmp.c_str(), nullptr, 16);
      if (Signal < SignalDelegator::MAX_SIGNALS) {
        PassSignals[Signal] = true;
      }
    }

    return {"OK", HandledPacketType::TYPE_ACK};
  }
  return {"", HandledPacketType::TYPE_UNKNOWN};
}


GdbServer::HandledPacketType GdbServer::ThreadAction(char action, uint32_t tid) {
  switch (action) {
    case 'c': {
      CTX->Run();
      ThreadBreakEvent.NotifyAll();
      CTX->WaitForThreadsToRun();
      return {"", HandledPacketType::TYPE_ONLYACK};
    }
    case 's': {
      CTX->Step();
      SendPacketPair({"OK", HandledPacketType::TYPE_ACK});
      fextl::string str = fextl::fmt::format("T05thread:{:02x};", getpid());
      if (LibraryMapChanged) {
        // If libraries have changed then let gdb know
        str += "library:1;";
      }

      SendPacketPair({std::move(str), HandledPacketType::TYPE_ACK});
      return {"OK", HandledPacketType::TYPE_ACK};
    }
    case 't':
      // This thread isn't part of the thread pool
      CTX->Stop(false /* Ignore current thread */);
      return {"OK", HandledPacketType::TYPE_ACK};
    default:
      return {"E00", HandledPacketType::TYPE_ACK};
  }
}

GdbServer::HandledPacketType GdbServer::handleV(const fextl::string& packet) {
  const auto match = [&](const fextl::string& str) -> std::optional<fextl::istringstream> {
    if (packet.rfind(str, 0) == 0) {
      auto ss = fextl::istringstream(packet);
      ss.seekg(str.size());
      return ss;
    }
    return std::nullopt;
  };

  const auto F       = [](int result) -> fextl::string { return fextl::fmt::format("F{:x}", result); };
  const auto F_error = []() -> fextl::string { return fextl::fmt::format("F-1,{:x}", errno); };
  const auto F_data  = [](int result, const fextl::string& data) -> fextl::string {
    // Binary encoded data is raw appended to the end
    return fextl::fmt::format("F{:#x};", result) + data;
  };

  std::optional<fextl::istringstream> ss;
  if((ss = match("vFile:open:"))) {
    fextl::string filename;
    int flags;
    int mode;

    filename = hexstring(*ss, ',');
    *ss >> std::hex >> flags;
    ss->get(); // discard comma
    *ss >> std::hex >> mode;

    return {F(open(filename.c_str(), flags, mode)), HandledPacketType::TYPE_ACK};
  }
  if((ss = match("vFile:setfs:"))) {
    int pid;
    *ss >> pid;

    return {F(pid == 0 ? 0 : -1), HandledPacketType::TYPE_ACK}; // Only support the common filesystem
  }
  if((ss = match("vFile:close:"))) {
    int fd;
    *ss >> std::hex >> fd;
    close(fd);
    return {F(0), HandledPacketType::TYPE_ACK};
  }
  if((ss = match("vFile:pread:"))) {
    int fd, count, offset;

    *ss >> std::hex >> fd;
    ss->get(); // discard comma
    *ss >> std::hex >> count;
    ss->get(); // discard comma
    *ss >> std::hex >> offset;

    fextl::string data(count, '\0');
    if (lseek(fd, offset, SEEK_SET) < 0) {
      return {F_error(), HandledPacketType::TYPE_ACK};
    }
    int ret = read(fd, data.data(), count);
    if (ret < 0) {
      return {F_error(), HandledPacketType::TYPE_ACK};
    }

    if (ret == 0) {
      return {F(0), HandledPacketType::TYPE_ACK};
    }

    data.resize(ret);
    return {F_data(ret, data), HandledPacketType::TYPE_ACK};
  }
  if ((ss = match("vCont?"))) {
    return {"vCont;c;t;s;r", HandledPacketType::TYPE_ACK}; // We support continue, step and terminate
    // FIXME: We also claim to support continue with signal... because it's compulsory
  }
  if ((ss = match("vCont;"))) {
    char action{};
    int thread{};

    action = ss->get();

    if (ss->peek() == ':') {
      ss->get();
      *ss >> std::hex >> thread;
    }

    if (ss->fail()) {
      return {"E00", HandledPacketType::TYPE_ACK};
    }

    return ThreadAction(action, thread);
  }
  return {"", HandledPacketType::TYPE_ACK};
}

GdbServer::HandledPacketType GdbServer::handleThreadOp(const fextl::string &packet) {
  const auto match = [&](const char *str) -> bool { return packet.rfind(str, 0) == 0; };

  if (match("Hc")) {
    // Sets thread to this ID for stepping
    // This is deprecated and vCont should be used instead
    auto ss = fextl::istringstream(packet);
    ss.seekg(fextl::string("Hc").size());
    ss >> std::hex >> CurrentDebuggingThread;

    CTX->Pause();
    return {"OK", HandledPacketType::TYPE_ACK};
  }

  if (match("Hg")) {
    // Sets thread for "other" operations
    auto ss = fextl::istringstream(packet);
    ss.seekg(std::string_view("Hg").size());
    ss >> std::hex >> CurrentDebuggingThread;

    // This must return quick otherwise IDA complains
    CTX->Pause();
    return {"OK", HandledPacketType::TYPE_ACK};
  }

  return {"", HandledPacketType::TYPE_UNKNOWN};
}

GdbServer::HandledPacketType GdbServer::handleBreakpoint(const fextl::string &packet) {
  auto ss = fextl::istringstream(packet);

  // Don't do anything with set breakpoints yet
  [[maybe_unused]] bool Set{};
  uint64_t Addr;
  uint64_t Type;
  Set = ss.get() == 'Z';

  ss >> std::hex >> Addr;
  ss.get(); // discard comma
  ss >> std::hex >> Type;

  CTX->Pause();
  return {"OK", HandledPacketType::TYPE_ACK};
}

GdbServer::HandledPacketType GdbServer::ProcessPacket(const fextl::string &packet) {
  switch (packet[0]) {
    case '?': {
      // Indicates the reason that the thread has stopped
      // Behaviour changes if the target is in non-stop mode
      // Binja doesn't support S response here
      fextl::string str = fextl::fmt::format("T00thread:{:x};", getpid());
      return {std::move(str), HandledPacketType::TYPE_ACK};
    }
    case 'c':
      // Continue
      return ThreadAction('c', 0);
    case 'D':
      // Detach
      // Ensure the threads are back in running state on detach
      CTX->Run();
      CTX->WaitForThreadsToRun();
      return {"OK", HandledPacketType::TYPE_ACK};
    case 'g':
      // We might be running while we try reading
      // Pause up front
      CTX->Pause();
      return {readRegs(), HandledPacketType::TYPE_ACK};
    case 'p':
      return readReg(packet);
    case 'q':
    case 'Q':
      return handleQuery(packet);
    case 'v':
      return handleV(packet);
    case 'm': // Memory read
    case 'M': // Memory write
      return handleMemory(packet);
    case 'H': // Sets thread for subsequent operations
      return handleThreadOp(packet);
    case '!': // Enable extended mode
    case 'T': // Is a thread alive?
      return {"OK", HandledPacketType::TYPE_ACK};
    case 's': // Step
      return ThreadAction('s', 0);
    case 'z': // Remove breakpoint or watchpoint
    case 'Z': // Inserts breakpoint or watchpoint
      return handleBreakpoint(packet);
    case 'k': // Kill the process
      CTX->Stop(false /* Ignore current thread */);
      CTX->WaitForIdle(); // Block until exit
      return {"", HandledPacketType::TYPE_NONE};
    default:
      return {"", HandledPacketType::TYPE_UNKNOWN};
  }
}

void GdbServer::SendPacketPair(const HandledPacketType& response) {
  std::lock_guard lk(sendMutex);
  if (response.TypeResponse == HandledPacketType::TYPE_ACK ||
      response.TypeResponse == HandledPacketType::TYPE_ONLYACK) {
    SendACK(*CommsStream, false);
  }
  else if (response.TypeResponse == HandledPacketType::TYPE_NACK ||
           response.TypeResponse == HandledPacketType::TYPE_ONLYNACK) {
    SendACK(*CommsStream, true);
  }

  if (response.TypeResponse == HandledPacketType::TYPE_UNKNOWN) {
    SendPacket(*CommsStream, "");
  }
  else if (response.TypeResponse != HandledPacketType::TYPE_ONLYNACK &&
           response.TypeResponse != HandledPacketType::TYPE_ONLYACK &&
           response.TypeResponse != HandledPacketType::TYPE_NONE) {
    SendPacket(*CommsStream, response.Response);
  }
}

void GdbServer::GdbServerLoop() {
  OpenListenSocket();

  while (!CTX->CoreShuttingDown.load()) {
    CommsStream = OpenSocket();

    HandledPacketType response{};

    // Outer server loop. Handles packet start, ACK/NAK and break

    int c;
    while ((c = CommsStream->get()) >= 0 ) {
        switch (c) {
        case '$': {
            auto packet = ReadPacket(*CommsStream);
            response = ProcessPacket(packet);
            SendPacketPair(response);
            if (response.TypeResponse == HandledPacketType::TYPE_UNKNOWN) {
              LogMan::Msg::DFmt("Unknown packet {}", packet);
            }
            break;
        }
        case '+':
            // ACK, do nothing.
            break;
        case '-':
            // NAK, Resend requested
            {
              std::lock_guard lk(sendMutex);
              SendPacket(*CommsStream, response.Response);
            }
            break;
        case '\x03': { // ASCII EOT
            CTX->Pause();
            fextl::string str = fextl::fmt::format("T02thread:{:02x};", getpid());
            if (LibraryMapChanged) {
              // If libraries have changed then let gdb know
              str += "library:1;";
            }
            SendPacketPair({std::move(str), HandledPacketType::TYPE_ACK});
            break;
          }
        default:
            LogMan::Msg::DFmt("GdbServer: Unexpected byte {} ({:02x})", static_cast<char>(c), c);
        }
    }

    {
        std::lock_guard lk(sendMutex);
        CommsStream.reset();
    }
  }

  close(ListenSocket);
}
static void* ThreadHandler(void *Arg) {
  FEXCore::GdbServer *This = reinterpret_cast<FEXCore::GdbServer*>(Arg);
  This->GdbServerLoop();
  return nullptr;
}

void GdbServer::StartThread() {
  uint64_t OldMask = FEXCore::Threads::SetSignalMask(~0ULL);
  gdbServerThread = FEXCore::Threads::Thread::Create(ThreadHandler, this);
  FEXCore::Threads::SetSignalMask(OldMask);
}

void GdbServer::OpenListenSocket() {
  // getaddrinfo allocates memory that can't be removed.
  FEXCore::Allocator::YesIKnowImNotSupposedToUseTheGlibcAllocator glibc;
  struct addrinfo hints, *res;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  if(getaddrinfo(NULL, "8086", &hints, &res) < 0) {
    perror("getaddrinfo");
  }

  int on = 1;

  ListenSocket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  if (ListenSocket < 0) {
    perror("socket");
  }
  if(setsockopt(ListenSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on)) < 0) {
    perror("setsockopt");
    close(ListenSocket);
  }

  if (bind(ListenSocket, res->ai_addr, res->ai_addrlen) < 0) {
    perror("bind");
    close(ListenSocket);
  }

  listen(ListenSocket, 1);

  freeaddrinfo(res);
}

fextl::unique_ptr<std::iostream> GdbServer::OpenSocket() {
  // Block until a connection arrives
  struct sockaddr_storage their_addr{};
  socklen_t addr_size{};

  LogMan::Msg::IFmt("GdbServer, waiting for connection on localhost:8086");
  int new_fd = accept(ListenSocket, (struct sockaddr *)&their_addr, &addr_size);

  return fextl::make_unique<FEXCore::Utils::NetStream>(new_fd);
}

#endif
} // namespace FEXCore
