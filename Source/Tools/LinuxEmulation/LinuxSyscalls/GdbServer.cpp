// SPDX-License-Identifier: MIT
/*
$info$
tags: glue|gdbserver
desc: Provides a gdb interface to the guest state
$end_info$
*/

#include "CodeLoader.h"
#include "GdbServer/Info.h"

#include "LinuxSyscalls/NetStream.h"

#include <cstdlib>
#include <cstdio>
#include <iomanip>
#include <memory>
#include <optional>
#include <string_view>

#include <Common/FEXServerClient.h>
#include <FEXCore/Config/Config.h>
#include <FEXCore/Core/Context.h>
#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Core/SignalDelegator.h>
#include <FEXCore/Core/X86Enums.h>
#include <FEXCore/Debug/InternalThreadState.h>
#include <FEXCore/HLE/SyscallHandler.h>
#include <FEXCore/Utils/CompilerDefs.h>
#include <FEXCore/Utils/FileLoading.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/StringUtils.h>
#include <FEXCore/Utils/Threads.h>
#include <FEXCore/fextl/fmt.h>
#include <FEXCore/fextl/map.h>
#include <FEXCore/fextl/sstream.h>
#include <FEXCore/fextl/string.h>
#include <FEXCore/fextl/vector.h>
#include <FEXHeaderUtils/Filesystem.h>

#include <atomic>
#include <cstring>
#ifndef _WIN32
#include <elf.h>
#include <netdb.h>
#include <sys/socket.h>
#endif
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stddef.h>
#include <string_view>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <utility>

#include "LinuxSyscalls/GdbServer.h"

namespace FEX {

#ifndef _WIN32
void GdbServer::Break(FEXCore::Core::InternalThreadState* Thread, int signal) {
  std::lock_guard lk(sendMutex);
  if (!CommsStream) {
    return;
  }

  auto ThreadObject = FEX::HLE::ThreadManager::GetStateObjectFromFEXCoreThread(Thread);
  // Current debugging thread switches to the thread that is breaking.
  CurrentDebuggingThread = ThreadObject->ThreadInfo.TID.load();

  const auto str = fextl::fmt::format("T{:02x}thread:{:x};", signal, CurrentDebuggingThread);
  SendPacket(*CommsStream, str);
}

void GdbServer::WaitForThreadWakeup() {
  // Wait for gdbserver to tell us to wake up
  ThreadBreakEvent.Wait();
}

GdbServer::~GdbServer() {
  CloseListenSocket();
  CoreShuttingDown = true;

  if (gdbServerThread->joinable()) {
    gdbServerThread->join(nullptr);
  }
}

GdbServer::GdbServer(FEXCore::Context::Context* ctx, FEX::HLE::SignalDelegator* SignalDelegation, FEX::HLE::SyscallHandler* const SyscallHandler)
  : CTX(ctx)
  , SyscallHandler {SyscallHandler} {
  // Pass all signals by default
  std::fill(PassSignals.begin(), PassSignals.end(), true);

  // This is a total hack as there is currently no way to resume once hitting a segfault
  // But it's semi-useful for debugging.
  for (uint32_t Signal = 0; Signal <= FEX::HLE::SignalDelegator::MAX_SIGNALS; ++Signal) {
    SignalDelegation->RegisterHostSignalHandler(
      Signal,
      [this](FEXCore::Core::InternalThreadState* Thread, int Signal, void* info, void* ucontext) {
      if (PassSignals[Signal]) {
        // Pass signal to the guest
        return false;
      }

      // Let GDB know that we have a signal
      this->Break(Thread, Signal);

      WaitForThreadWakeup();

      return true;
      },
      true);
  }

  StartThread();
}

static int calculateChecksum(const fextl::string& packet) {
  unsigned char checksum = 0;
  for (const char& c : packet) {
    checksum += c;
  }
  return checksum;
}

static fextl::string hexstring(fextl::istringstream& ss, int delm) {
  fextl::string ret;

  char hexString[3] = {0, 0, 0};
  while (ss.peek() != delm) {
    ss.read(hexString, 2);
    int c = std::strtoul(hexString, nullptr, 16);
    ret.push_back((char)c);
  }

  if (delm != -1) {
    ss.get();
  }

  return ret;
}

static fextl::string encodeHex(const unsigned char* data, size_t length) {
  fextl::ostringstream ss;

  for (size_t i = 0; i < length; i++) {
    ss << std::setfill('0') << std::setw(2) << std::hex << int(data[i]);
  }
  return ss.str();
}

static fextl::string encodeHex(std::string_view str) {
  return encodeHex(reinterpret_cast<const unsigned char*>(str.data()), str.size());
}

// Packet parser
// Takes a serial stream and reads a single packet
// Un-escapes chars, checks the checksum and request a retransmit if it fails.
// Once the checksum is validated, it acknowledges and returns the packet in a string
fextl::string GdbServer::ReadPacket(std::iostream& stream) {
  fextl::string packet {};

  // The GDB "Remote Serial Protocal" was originally 7bit clean for use on serial ports.
  // Binary data is useally hex encoded. However some later extentions just put
  // raw 8bit binary data.

  // Packets are in the format
  // $<data>#<checksum>
  // where any $ or # in the packet body are escaped ('}' followed by the char XORed with 0x20)
  // The checksum is a single unsigned byte sum of the data, hex encoded.

  int c;
  while ((c = stream.get()) > 0) {
    switch (c) {
    case '$': // start of packet
      if (packet.size() != 0) {
        LogMan::Msg::EFmt("Dropping unexpected data: \"{}\"", packet);
      }

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
    default: packet.push_back((char)c); break;
    }
  }

  return "";
}

static fextl::string escapePacket(const fextl::string& packet) {
  fextl::ostringstream ss;

  for (const auto& c : packet) {
    switch (c) {
    case '$':
    case '#':
    case '*':
    case '}': {
      char escaped = c ^ 0x20;
      ss << '}' << (escaped);
      break;
    }
    default: ss << c; break;
    }
  }

  return ss.str();
}

void GdbServer::SendPacket(std::ostream& stream, const fextl::string& packet) {
  const auto escaped = escapePacket(packet);
  const auto str = fextl::fmt::format("${}#{:02x}", escaped, calculateChecksum(escaped));

  stream << str << std::flush;
}

void GdbServer::SendACK(std::ostream& stream, bool NACK) {
  if (NoAckMode) {
    return;
  }

  if (NACK) {
    stream << "-" << std::flush;
  } else {
    stream << "+" << std::flush;
  }

  if (SettingNoAckMode) {
    NoAckMode = true;
    SettingNoAckMode = false;
  }
}

const FEX::HLE::ThreadStateObject* GdbServer::FindThreadByTID(uint32_t TID) {
  auto Threads = SyscallHandler->TM.GetThreads();

  for (auto& Thread : *Threads) {
    if (Thread->ThreadInfo.TID != TID) {
      continue;
    }

    return Thread;
  }

  // Return parent thread if TID isn't found.
  return Threads->at(0);
}


GdbServer::GDBContextDefinition GdbServer::GenerateContextDefinition(const FEX::HLE::ThreadStateObject* ThreadObject) {
  GDBContextDefinition GDB {};
  FEXCore::Core::CPUState state {};

  // Copy the thread state.
  memcpy(&state, ThreadObject->Thread->CurrentFrame, sizeof(state));

  // Encode the GDB context definition
  memcpy(&GDB.gregs[0], &state.gregs[0], sizeof(GDB.gregs));
  memcpy(&GDB.rip, &state.rip, sizeof(GDB.rip));

  GDB.eflags = CTX->ReconstructCompactedEFLAGS(ThreadObject->Thread, false, nullptr, 0);

  for (size_t i = 0; i < FEXCore::Core::CPUState::NUM_MMS; ++i) {
    memcpy(&GDB.mm[i], &state.mm[i], sizeof(GDB.mm[i]));
  }

  GDB.fctrl = state.FCW;

  GDB.fstat = static_cast<uint32_t>(state.flags[FEXCore::X86State::X87FLAG_TOP_LOC]) << 11;
  GDB.fstat |= static_cast<uint32_t>(state.flags[FEXCore::X86State::X87FLAG_C0_LOC]) << 8;
  GDB.fstat |= static_cast<uint32_t>(state.flags[FEXCore::X86State::X87FLAG_C1_LOC]) << 9;
  GDB.fstat |= static_cast<uint32_t>(state.flags[FEXCore::X86State::X87FLAG_C2_LOC]) << 10;
  GDB.fstat |= static_cast<uint32_t>(state.flags[FEXCore::X86State::X87FLAG_C3_LOC]) << 14;

  __uint128_t XMM_Low[FEXCore::Core::CPUState::NUM_XMMS];
  __uint128_t YMM_High[FEXCore::Core::CPUState::NUM_XMMS];

  CTX->ReconstructXMMRegisters(ThreadObject->Thread, XMM_Low, YMM_High);
  for (size_t i = 0; i < FEXCore::Core::CPUState::NUM_XMMS; ++i) {
    memcpy(&GDB.xmm[0], &XMM_Low[i], sizeof(__uint128_t));
    memcpy(&GDB.xmm[2], &YMM_High[i], sizeof(__uint128_t));
  }

  return GDB;
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
  const fextl::string& RuntimeExecutable = Filename();
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

    struct stat sb {};
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
  for (auto& Array : SegmentMaps) {
    xml << "\t<library name=\"" << Array.first << "\">\n";
    for (auto& Data : Array.second) {
      xml << "\t\t<segment address=\"0x" << std::hex << Data.Begin << "\"/>\n";
    }
    xml << "\t</library>\n";
  }

  xml << "</library-list>\n";

  LibraryMapString = xml.str();
  LibraryMapChanged = false;
}

// Binary data transfer handlers

GdbServer::HandledPacketType GdbServer::XferCommandExecFile(const fextl::string& annex, int offset, int length) {
  int annex_pid;
  if (annex.empty()) {
    annex_pid = getpid();
  } else {
    auto ss_pid = fextl::istringstream(annex);
    ss_pid >> std::hex >> annex_pid;
  }

  if (annex_pid == getpid()) {
    return {EncodeXferString(Filename(), offset, length), HandledPacketType::TYPE_ACK};
  }

  return {"E00", HandledPacketType::TYPE_ACK};
}

GdbServer::HandledPacketType GdbServer::XferCommandFeatures(const fextl::string& annex, int offset, int length) {
  if (annex == "target.xml") {
    return {EncodeXferString(GDB::Info::BuildTargetXML(), offset, length), HandledPacketType::TYPE_ACK};
  }

  return {"E00", HandledPacketType::TYPE_ACK};
}

GdbServer::HandledPacketType GdbServer::XferCommandThreads(const fextl::string& annex, int offset, int length) {
  if (offset == 0) {
    auto Threads = SyscallHandler->TM.GetThreads();

    ThreadString.clear();
    fextl::ostringstream ss;
    ss << "<threads>\n";
    for (auto& Thread : *Threads) {
      // Thread id is in hex without 0x prefix
      const auto ThreadName = GDB::Info::GetThreadName(::getpid(), Thread->ThreadInfo.TID);
      ss << "<thread id=\"" << std::hex << Thread->ThreadInfo.TID << "\"";
      if (!ThreadName.empty()) {
        ss << " name=\"" << ThreadName << "\"";
      }
      ss << "/>\n";
    }

    ss << "</threads>\n";
    ss << std::flush;
    ThreadString = ss.str();
  }

  return {EncodeXferString(ThreadString, offset, length), HandledPacketType::TYPE_ACK};
}

GdbServer::HandledPacketType GdbServer::XferCommandOSData(const fextl::string& annex, int offset, int length) {
  if (offset == 0) {
    OSDataString = GDB::Info::BuildOSXML();
  }
  return {EncodeXferString(OSDataString, offset, length), HandledPacketType::TYPE_ACK};
}

GdbServer::HandledPacketType GdbServer::XferCommandLibraries(const fextl::string& annex, int offset, int length) {
  if (offset == 0) {
    // Attempt to rebuild when reading from zero
    buildLibraryMap();
  }
  return {EncodeXferString(LibraryMapString, offset, length), HandledPacketType::TYPE_ACK};
}

GdbServer::HandledPacketType GdbServer::XferCommandAuxv(const fextl::string& annex, int offset, int length) {
  auto CodeLoader = SyscallHandler->GetCodeLoader();
  uint64_t auxv_ptr, auxv_size;
  CodeLoader->GetAuxv(auxv_ptr, auxv_size);
  fextl::string data;
  if (Is64BitMode()) {
    data.resize(auxv_size);
    memcpy(data.data(), reinterpret_cast<void*>(auxv_ptr), data.size());
  } else {
    // We need to transcode from 32-bit auxv_t to 64-bit
    data.resize(auxv_size / sizeof(Elf32_auxv_t) * sizeof(Elf64_auxv_t));
    size_t NumAuxv = auxv_size / sizeof(Elf32_auxv_t);
    for (size_t i = 0; i < NumAuxv; ++i) {
      Elf32_auxv_t* auxv = reinterpret_cast<Elf32_auxv_t*>(auxv_ptr + i * sizeof(Elf32_auxv_t));
      Elf64_auxv_t tmp;
      tmp.a_type = auxv->a_type;
      tmp.a_un.a_val = auxv->a_un.a_val;
      memcpy(data.data() + i * sizeof(Elf64_auxv_t), &tmp, sizeof(Elf64_auxv_t));
    }
  }

  return {EncodeXferString(data, offset, length), HandledPacketType::TYPE_ACK};
}

GdbServer::HandledPacketType GdbServer::handleXfer(const fextl::string& packet) {
  fextl::string object;
  fextl::string rw;
  fextl::string annex;
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

    ss >> std::hex >> offset;
    ss.get(expectComma);
    ss >> std::hex >> length;

    // Bail on any errors
    if (ss.fail() || !ss.eof() || expectXfer != "qXfer" || rw != "read" || expectComma != ',') {
      return {"E00", HandledPacketType::TYPE_ACK};
    }
  }

  // Specific object documentation: https://sourceware.org/gdb/current/onlinedocs/gdb.html/General-Query-Packets.html#qXfer-read
  if (object == "auxv") {
    return XferCommandAuxv(annex, offset, length);
  }

  // btrace
  // btrace-conf

  if (object == "exec-file") {
    return XferCommandExecFile(annex, offset, length);
  }

  if (object == "features") {
    return XferCommandFeatures(annex, offset, length);
  }

  if (object == "libraries") {
    return XferCommandLibraries(annex, offset, length);
  }

  // libraries-svr4
  // memory-map
  // sdata
  // siginfo:read
  // siginfo:write

  if (object == "threads") {
    return XferCommandThreads(annex, offset, length);
  }

  // traceframe-info
  // uib
  // fdpic

  if (object == "osdata") {
    return XferCommandOSData(annex, offset, length);
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
    if (MapsStream.eof()) {
      break;
    }
    uint64_t Begin, End;
    char r, w, x, p;
    if (sscanf(Line.c_str(), "%lx-%lx %c%c%c%c", &Begin, &End, &r, &w, &x, &p) == 6) {
      if (Begin <= Address && End > Address) {
        ssize_t Overrun {};
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
  auto CodeLoader = SyscallHandler->GetCodeLoader();
  uint64_t BaseOffset = CodeLoader->GetBaseOffset();
  fextl::string str = fextl::fmt::format("Text={:x};Data={:x};Bss={:x}", BaseOffset, BaseOffset, BaseOffset);
  return {std::move(str), HandledPacketType::TYPE_ACK};
}

GdbServer::HandledPacketType GdbServer::ThreadAction(char action, uint32_t tid) {
  switch (action) {
  case 'c': {
    SyscallHandler->TM.Run();
    ThreadBreakEvent.NotifyAll();
    SyscallHandler->TM.WaitForThreadsToRun();
    return {"", HandledPacketType::TYPE_ONLYACK};
  }
  case 's': {
    SyscallHandler->TM.Step();
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
    SyscallHandler->TM.Stop();
    return {"OK", HandledPacketType::TYPE_ACK};
  default: return {"E00", HandledPacketType::TYPE_ACK};
  }
}

// Command handlers
GdbServer::HandledPacketType GdbServer::CommandEnableExtendedMode(const fextl::string& packet) {
  return {"OK", HandledPacketType::TYPE_ACK};
}

GdbServer::HandledPacketType GdbServer::CommandQueryHalted(const fextl::string& packet) {
  // Indicates the reason that the thread has stopped
  // Behaviour changes if the target is in non-stop mode
  // Binja doesn't support S response here
  fextl::string str = fextl::fmt::format("T00thread:{:x};", getpid());
  return {std::move(str), HandledPacketType::TYPE_ACK};
}

GdbServer::HandledPacketType GdbServer::CommandContinue(const fextl::string& packet) {
  // Continue
  return ThreadAction('c', 0);
}

GdbServer::HandledPacketType GdbServer::CommandDetach(const fextl::string& packet) {
  // Detach
  // Ensure the threads are back in running state on detach
  SyscallHandler->TM.Run();
  SyscallHandler->TM.WaitForThreadsToRun();
  return {"OK", HandledPacketType::TYPE_ACK};
}

GdbServer::HandledPacketType GdbServer::CommandReadRegisters(const fextl::string& packet) {
  // We might be running while we try reading
  // Pause up front
  SyscallHandler->TM.Pause();
  const FEX::HLE::ThreadStateObject* CurrentThread = FindThreadByTID(CurrentDebuggingThread);
  auto GDB = GenerateContextDefinition(CurrentThread);
  return {encodeHex((unsigned char*)&GDB, sizeof(GDBContextDefinition)), HandledPacketType::TYPE_ACK};
}

GdbServer::HandledPacketType GdbServer::CommandThreadOp(const fextl::string& packet) {
  const auto match = [&](const char* str) -> bool {
    return packet.rfind(str, 0) == 0;
  };

  if (match("Hc")) {
    // Sets thread to this ID for stepping
    // This is deprecated and vCont should be used instead
    auto ss = fextl::istringstream(packet);
    ss.seekg(strlen("Hc"));
    ss >> std::hex >> CurrentDebuggingThread;

    SyscallHandler->TM.Pause();
    return {"OK", HandledPacketType::TYPE_ACK};
  }

  if (match("Hg")) {
    // Sets thread for "other" operations
    auto ss = fextl::istringstream(packet);
    ss.seekg(strlen("Hg"));
    ss >> std::hex >> CurrentDebuggingThread;

    // This must return quick otherwise IDA complains
    SyscallHandler->TM.Pause();
    return {"OK", HandledPacketType::TYPE_ACK};
  }

  return {"", HandledPacketType::TYPE_UNKNOWN};
}

GdbServer::HandledPacketType GdbServer::CommandKill(const fextl::string& packet) {
  SyscallHandler->TM.Stop();
  SyscallHandler->TM.WaitForIdle(); // Block until exit
  return {"", HandledPacketType::TYPE_NONE};
}

GdbServer::HandledPacketType GdbServer::CommandMemory(const fextl::string& packet) {
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
    ss.get();                 // discard colon
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

GdbServer::HandledPacketType GdbServer::CommandReadReg(const fextl::string& packet) {
  size_t addr;
  auto ss = fextl::istringstream(packet);
  ss.get(); // Drop first letter
  ss >> std::hex >> addr;

  const FEX::HLE::ThreadStateObject* CurrentThread = FindThreadByTID(CurrentDebuggingThread);
  auto GDB = GenerateContextDefinition(CurrentThread);

  if (addr >= offsetof(GDBContextDefinition, gregs[0]) && addr < offsetof(GDBContextDefinition, gregs[16])) {
    return {encodeHex((unsigned char*)(&GDB.gregs[addr / sizeof(uint64_t)]), sizeof(uint64_t)), HandledPacketType::TYPE_ACK};
  } else if (addr == offsetof(GDBContextDefinition, rip)) {
    return {encodeHex((unsigned char*)(&GDB.rip), sizeof(uint64_t)), HandledPacketType::TYPE_ACK};
  } else if (addr == offsetof(GDBContextDefinition, eflags)) {
    return {encodeHex((unsigned char*)(&GDB.eflags), sizeof(uint32_t)), HandledPacketType::TYPE_ACK};
  } else if (addr >= offsetof(GDBContextDefinition, cs) && addr < offsetof(GDBContextDefinition, mm[0])) {
    uint32_t Empty {};
    return {encodeHex((unsigned char*)(&Empty), sizeof(uint32_t)), HandledPacketType::TYPE_ACK};
  } else if (addr >= offsetof(GDBContextDefinition, mm[0]) && addr < offsetof(GDBContextDefinition, mm[8])) {
    return {encodeHex((unsigned char*)(&GDB.mm[(addr - offsetof(GDBContextDefinition, mm[0])) / sizeof(X80Float)]), sizeof(X80Float)),
            HandledPacketType::TYPE_ACK};
  } else if (addr == offsetof(GDBContextDefinition, fctrl)) {
    return {encodeHex((unsigned char*)(&GDB.fctrl), sizeof(uint32_t)), HandledPacketType::TYPE_ACK};
  } else if (addr == offsetof(GDBContextDefinition, fstat)) {
    return {encodeHex((unsigned char*)(&GDB.fstat), sizeof(uint32_t)), HandledPacketType::TYPE_ACK};
  } else if (addr >= offsetof(GDBContextDefinition, dummies[0]) && addr < offsetof(GDBContextDefinition, dummies[6])) {
    return {encodeHex((unsigned char*)(&GDB.dummies[0]), sizeof(uint32_t)), HandledPacketType::TYPE_ACK};
  } else if (addr >= offsetof(GDBContextDefinition, xmm[0][0]) && addr < offsetof(GDBContextDefinition, xmm[16][0])) {
    const auto XmmIndex = (addr - offsetof(GDBContextDefinition, xmm[0][0])) / FEXCore::Core::CPUState::XMM_AVX_REG_SIZE;
    return {encodeHex(reinterpret_cast<const uint8_t*>(&GDB.xmm[XmmIndex]), FEXCore::Core::CPUState::XMM_AVX_REG_SIZE), HandledPacketType::TYPE_ACK};
  } else if (addr == offsetof(GDBContextDefinition, mxcsr)) {
    return {encodeHex((unsigned char*)(&GDB.mxcsr), sizeof(uint32_t)), HandledPacketType::TYPE_ACK};
  }

  LogMan::Msg::EFmt("Unknown GDB register 0x{:x}", addr);
  return {"E00", HandledPacketType::TYPE_ACK};
}

GdbServer::HandledPacketType GdbServer::CommandQuery(const fextl::string& packet) {
  const auto match = [&](const char* str) -> bool {
    return packet.rfind(str, 0) == 0;
  };
  const auto MatchStr = [](const fextl::string& Str, const char* str) -> bool {
    return Str.rfind(str, 0) == 0;
  };

  const auto split = [](const fextl::string& Str, char deliminator) -> fextl::vector<fextl::string> {
    fextl::vector<fextl::string> Elements;
    fextl::istringstream Input(Str);
    for (fextl::string line; std::getline(Input, line); Elements.emplace_back(line))
      ;
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
    fextl::string SupportedFeatures {};

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
    SupportedFeatures += "QStartNoAckMode+;";

    // TODO: Support breakpoints
    // SupportedFeatures += "swbreak+;";
    // SupportedFeatures += "hwbreak+;";
    // SupportedFeatures += "BreakpointCommands+;";

    // TODO: If we want to support conditional breakpoints then we need to support single stepping.
    // SupportedFeatures += "ConditionalBreakpoints+;";

    for (auto& Feature : Features) {
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
    return {std::move(SupportedFeatures), HandledPacketType::TYPE_ACK};
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
    auto Threads = SyscallHandler->TM.GetThreads();

    fextl::ostringstream ss;
    ss << "m";
    for (size_t i = 0; i < Threads->size(); ++i) {
      auto Thread = Threads->at(i);
      ss << std::hex << Thread->ThreadInfo.TID;
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
    auto ThreadName = GDB::Info::GetThreadName(::getpid(), ThreadID);
    return {encodeHex((unsigned char*)ThreadName.data(), ThreadName.size()), HandledPacketType::TYPE_ACK};
  }
  if (match("qC")) {
    // Returns the current Thread ID
    auto Threads = SyscallHandler->TM.GetThreads();
    fextl::ostringstream ss;
    ss << "m" << std::hex << Threads->at(0)->ThreadInfo.TID;
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
    } else {
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
    for (fextl::string tmp; std::getline(ss, tmp, ';');) {
      uint32_t Signal = std::stoi(tmp.c_str(), nullptr, 16);
      if (Signal < FEX::HLE::SignalDelegator::MAX_SIGNALS) {
        PassSignals[Signal] = true;
      }
    }

    return {"OK", HandledPacketType::TYPE_ACK};
  }

  // lldb specific queries
  if (match("qHostInfo")) {
    // Returns Key:Value pairs separated by ;
    // eg:
    // triple:7838365f36342d70632d6c696e75782d676e75;
    // ptrsize:8;
    // distribution_id:7562756e7475;
    // watchpoint_exceptions_received:after;
    // endian:little;
    // os_version:6.3.3;
    // os_build:362e332e332d3036303330332d67656e65726963;
    // os_kernel:2332303233303531373133333620534d5020505245454d50545f44594e414d494320576564204d61792031372031333a34353a3139205554432032303233;
    // hostname:7279616e682d545235303030;
    fextl::string HostFeatures {};

    // 64-bit always returned for the host environment.
    // qProcessInfo will return i386 or not.
    HostFeatures += fextl::fmt::format("triple:{};", encodeHex("x86_64-pc-linux-gnu"));
    HostFeatures += "ptrsize:8;";

    // Always little-endian.
    HostFeatures += "endian:little;";

    struct utsname buf {};
    if (uname(&buf) != -1) {
      uint32_t Major {};
      uint32_t Minor {};
      uint32_t Patch {};

      // Parse kernel version in the form of `<Major>.<Minor>.<Patch>[Optional Data]`
      const auto End = buf.release + sizeof(buf.release);
      auto Results = std::from_chars(buf.release, End, Major, 10);
      Results = std::from_chars(Results.ptr + 1, End, Minor, 10);
      Results = std::from_chars(Results.ptr + 1, End, Patch, 10);

      HostFeatures += fextl::fmt::format("os_version:{}.{}.{};", Major, Minor, Patch);

      // os_build returns the release untouched.
      HostFeatures += fextl::fmt::format("os_build:{};", encodeHex(buf.release));
      HostFeatures += fextl::fmt::format("os_kernel:{};", encodeHex(buf.version));
      HostFeatures += fextl::fmt::format("hostname:{};", encodeHex(buf.nodename));
    }

    // TODO: distribution_id should be fetched with `lsb_release -i`
    // TODO: watchpoint_exceptions_received is unsupported
    return {std::move(HostFeatures), HandledPacketType::TYPE_ACK};
  }
  if (match("qGetWorkingDir")) {
    char Tmp[PATH_MAX];
    if (getcwd(Tmp, PATH_MAX)) {
      return {encodeHex(Tmp), HandledPacketType::TYPE_ACK};
    }
    return {"E00", HandledPacketType::TYPE_ACK};
  }
  return {"", HandledPacketType::TYPE_UNKNOWN};
}

GdbServer::HandledPacketType GdbServer::CommandSingleStep(const fextl::string& packet) {
  return ThreadAction('s', 0);
}

GdbServer::HandledPacketType GdbServer::CommandQueryThreadAlive(const fextl::string& packet) {
  return {"OK", HandledPacketType::TYPE_ACK};
}

GdbServer::HandledPacketType GdbServer::HandlevFile(const fextl::string& packet) {
  const auto match = [&](const fextl::string& str) -> std::optional<fextl::istringstream> {
    if (packet.rfind(str, 0) == 0) {
      auto ss = fextl::istringstream(packet);
      ss.seekg(str.size());
      return ss;
    }
    return std::nullopt;
  };

  const auto F = [](int result) -> fextl::string {
    return fextl::fmt::format("F{:x}", result);
  };
  const auto F_error = []() -> fextl::string {
    return fextl::fmt::format("F-1,{:x}", errno);
  };
  const auto F_data = [](int result, const fextl::string& data) -> fextl::string {
    // Binary encoded data is raw appended to the end
    return fextl::fmt::format("F{:#x};", result) + data;
  };

  std::optional<fextl::istringstream> ss;
  if ((ss = match("vFile:open:"))) {
    fextl::string filename;
    int flags;
    int mode;

    filename = hexstring(*ss, ',');
    *ss >> std::hex >> flags;
    ss->get(); // discard comma
    *ss >> std::hex >> mode;

    return {F(open(filename.c_str(), flags, mode)), HandledPacketType::TYPE_ACK};
  }
  if ((ss = match("vFile:setfs:"))) {
    int pid;
    *ss >> pid;

    return {F(pid == 0 ? 0 : -1), HandledPacketType::TYPE_ACK}; // Only support the common filesystem
  }
  if ((ss = match("vFile:close:"))) {
    int fd;
    *ss >> std::hex >> fd;
    close(fd);
    return {F(0), HandledPacketType::TYPE_ACK};
  }
  if ((ss = match("vFile:pread:"))) {
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

  return {"", HandledPacketType::TYPE_ACK};
}

GdbServer::HandledPacketType GdbServer::HandlevCont(const fextl::string& packet) {
  const auto match = [&](const fextl::string& str) -> std::optional<fextl::istringstream> {
    if (packet.rfind(str, 0) == 0) {
      auto ss = fextl::istringstream(packet);
      ss.seekg(str.size());
      return ss;
    }
    return std::nullopt;
  };

  std::optional<fextl::istringstream> ss;
  if ((ss = match("vCont?"))) {
    return {"vCont;c;t;s;r", HandledPacketType::TYPE_ACK}; // We support continue, step and terminate
    // FIXME: We also claim to support continue with signal... because it's compulsory
  }

  if ((ss = match("vCont;"))) {
    char action {};
    int thread {};

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

GdbServer::HandledPacketType GdbServer::CommandMultiLetterV(const fextl::string& packet) {
  // TODO: vAttach
  if (packet.starts_with("vCont")) {
    return HandlevCont(packet);
  }

  // TODO: vCtrlC

  if (packet.starts_with("vFile")) {
    return HandlevFile(packet);
  }

  if (packet.starts_with("vKill")) {
    tgkill(::getpid(), ::getpid(), SIGKILL);
  }

  // TODO: vRun
  // TODO: vStopped

  return {"", HandledPacketType::TYPE_ACK};
}

GdbServer::HandledPacketType GdbServer::CommandBreakpoint(const fextl::string& packet) {
  auto ss = fextl::istringstream(packet);

  // Don't do anything with set breakpoints yet
  [[maybe_unused]] bool Set {};
  uint64_t Addr;
  uint64_t Type;
  Set = ss.get() == 'Z';

  ss >> std::hex >> Addr;
  ss.get(); // discard comma
  ss >> std::hex >> Type;

  SyscallHandler->TM.Pause();
  return {"OK", HandledPacketType::TYPE_ACK};
}

GdbServer::HandledPacketType GdbServer::CommandUnknown(const fextl::string& packet) {
  return {"", HandledPacketType::TYPE_UNKNOWN};
}

GdbServer::HandledPacketType GdbServer::ProcessPacket(const fextl::string& packet) {
  // Packet commands list: https://sourceware.org/gdb/current/onlinedocs/gdb.html/Packets.html#Packets

  switch (packet[0]) {
  // Command: $!
  // - Desc: Enable extended mode
  // - Args: <None>
  case '!': return CommandEnableExtendedMode(packet);
  // Command: $?
  // - Desc: Sent on connection first established to query the reason the target halted.
  case '?': return CommandQueryHalted(packet);
  // Command: $A
  // - Desc: Initialized argv[] array passed in to the program.
  // - Args: arglen,argnum,arg,...
  case 'A': return CommandUnknown(packet);
  // Command: $b
  // - Desc: Change the serial line speed to baud
  // - Args: baud
  // - Deprecated: Behaviour isn't well-defined.
  case 'b': return CommandUnknown(packet);
  // Command: $B
  // - Desc: Set or clear a breadpoint at address
  // - Args: addr,mode
  // - Deprecated: Use $Z and $z instead.
  case 'B': return CommandUnknown(packet);
  // Command: $c
  // - Desc: Continue execution of process
  // - Args: [addr]
  // - Deprecated: See $vCont for multi-threaded support.
  case 'c': return CommandContinue(packet);
  // Command: $C
  // - Desc: Continue execution of process with signal
  // - Args: sig[;addr]
  // - Deprecated: See $vCont for multi-threaded support.
  case 'C': return CommandUnknown(packet);
  // Command: $d
  // - Desc: Toggle debug flag
  // - Args: <None>
  // - Deprecated: Use $q or $Q instead.
  case 'd': return CommandUnknown(packet);
  // Command: $D
  // - Desc: Detach GDB from the remote system
  // - Args: [;pid]
  case 'D': return CommandDetach(packet);
  // Command: $F
  // - Desc: A reply from GDB to the `F` packet sent by the target. Part of the File-I/O protocol.
  // - Args: RC,EE,CF;XX
  case 'F': return CommandUnknown(packet);
  // Command: $g
  // - Desc: Read general registers
  // - Args: <None>
  case 'g': return CommandReadRegisters(packet);
  // Command: $G
  // - Desc: Write general registers
  // - Args: XX...
  case 'G': return CommandUnknown(packet);
  // Command: $H
  // - Desc: Sets thread for subsequent operations
  // - Args: op thread-id
  case 'H': return CommandThreadOp(packet);
  // Command: $i
  // - Desc: Step the remote target by a single clock cycle
  // - Args: [addr[,nnn]]
  case 'i': return CommandUnknown(packet);
  // Command: $I
  // - Desc: Signal, then cycle step
  // - Args: <None>
  case 'I': return CommandUnknown(packet);
  // Command: $k
  // - Desc: kill process
  case 'k': return CommandKill(packet);
  // Command: $m
  // - Desc: Read addressable memory
  // - Args: addr length
  case 'm':
  // Command: $M
  // - Desc: Write addressable memory
  // - Args: addr length
  case 'M': return CommandMemory(packet);
  // Command: $p
  // - Desc: Read the value of a register
  // - Args: index
  case 'p': return CommandReadReg(packet);
  // Command: $q
  // - Desc: General query fetching
  // - Args: Name params...
  case 'q':
  // Command: $Q
  // - Desc: General query setting
  // - Args: Name params...
  case 'Q': return CommandQuery(packet);
  // Command: $r
  // - Desc: Reset the entire system
  // - Args: <None>
  // - Deprecated: Use $R instead.
  case 'r': return CommandUnknown(packet);
  // Command: $R
  // - Desc: Restart the program beging debugged
  // - Args: XX
  case 'R': return CommandUnknown(packet);
  // Command: $s
  // - Desc: Single step
  // - Args: [addr]
  case 's': return CommandSingleStep(packet);
  // Command: $S
  // - Desc: Step with Signal
  // - Args: sig[;addr]
  // - Deprecated: See $vCont for multi-threaded support.
  case 'S': return CommandUnknown(packet);
  // Command: $t
  // - Desc: Search backwards started at address with pattern and mask.
  // - Args: addr:PP,MM
  case 't': return CommandUnknown(packet);
  // Command: $T
  // - Desc: Find out if the thread is alive
  // - Args: thread-id
  case 'T': return CommandQueryThreadAlive(packet);
  // Command: $v<Operation>
  // - Desc: Multi-letter command
  case 'v': return CommandMultiLetterV(packet);
  // Command: $X
  // - Desc: Write data to memory
  // - Args: addr,length:XX...
  case 'X': return CommandUnknown(packet);
  // Command: $z
  // - Desc: Insert a type of breakpoint or watchpoint
  // - Args: type,addr,kind
  case 'z':
  // Command: $Z
  // - Desc: Remove a type of breakpoint or watchpoint
  // - Args: type,addr,kind
  case 'Z': return CommandBreakpoint(packet);
  default: return {"", HandledPacketType::TYPE_UNKNOWN};
  }
}

void GdbServer::SendPacketPair(const HandledPacketType& response) {
  std::lock_guard lk(sendMutex);
  if (response.TypeResponse == HandledPacketType::TYPE_ACK || response.TypeResponse == HandledPacketType::TYPE_ONLYACK) {
    SendACK(*CommsStream, false);
  } else if (response.TypeResponse == HandledPacketType::TYPE_NACK || response.TypeResponse == HandledPacketType::TYPE_ONLYNACK) {
    SendACK(*CommsStream, true);
  }

  if (response.TypeResponse == HandledPacketType::TYPE_UNKNOWN) {
    SendPacket(*CommsStream, "");
  } else if (response.TypeResponse != HandledPacketType::TYPE_ONLYNACK && response.TypeResponse != HandledPacketType::TYPE_ONLYACK &&
             response.TypeResponse != HandledPacketType::TYPE_NONE) {
    SendPacket(*CommsStream, response.Response);
  }
}

GdbServer::WaitForConnectionResult GdbServer::WaitForConnection() {
  while (!CoreShuttingDown.load()) {
    struct pollfd PollFD {
      .fd = ListenSocket, .events = POLLIN | POLLPRI | POLLRDHUP, .revents = 0,
    };
    int Result = ppoll(&PollFD, 1, nullptr, nullptr);
    if (Result > 0) {
      if (PollFD.revents & POLLIN) {
        CommsStream = OpenSocket();
        return WaitForConnectionResult::CONNECTION;
      } else if (PollFD.revents & (POLLHUP | POLLERR | POLLNVAL)) {
        // Listen socket error or shutting down
        LogMan::Msg::EFmt("[GdbServer] gdbserver shutting down: {}");
        return WaitForConnectionResult::ERROR;
      }
    } else if (Result == -1) {
      LogMan::Msg::EFmt("[GdbServer] poll failure: {}", errno);
    }
  }

  LogMan::Msg::EFmt("[GdbServer] Shutting Down");
  return WaitForConnectionResult::ERROR;
}

void GdbServer::GdbServerLoop() {
  OpenListenSocket();
  if (ListenSocket == -1) {
    // Couldn't open socket, just exit.
    return;
  }

  while (!CoreShuttingDown.load()) {
    if (WaitForConnection() == WaitForConnectionResult::ERROR) {
      break;
    }

    HandledPacketType response {};

    // Outer server loop. Handles packet start, ACK/NAK and break

    int c;
    while ((c = CommsStream->get()) >= 0) {
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
        SyscallHandler->TM.Pause();
        fextl::string str = fextl::fmt::format("T02thread:{:02x};", getpid());
        if (LibraryMapChanged) {
          // If libraries have changed then let gdb know
          str += "library:1;";
        }
        SendPacketPair({std::move(str), HandledPacketType::TYPE_ACK});
        break;
      }
      default: LogMan::Msg::DFmt("GdbServer: Unexpected byte {} ({:02x})", static_cast<char>(c), c);
      }
    }

    {
      std::lock_guard lk(sendMutex);
      CommsStream.reset();
    }
  }

  CloseListenSocket();
}
static void* ThreadHandler(void* Arg) {
  FEXCore::Threads::SetThreadName("FEX:gdbserver");
  auto This = reinterpret_cast<FEX::GdbServer*>(Arg);
  This->GdbServerLoop();
  return nullptr;
}

void GdbServer::StartThread() {
  uint64_t OldMask = FEXCore::Threads::SetSignalMask(~0ULL);
  gdbServerThread = FEXCore::Threads::Thread::Create(ThreadHandler, this);
  FEXCore::Threads::SetSignalMask(OldMask);
}

void GdbServer::OpenListenSocket() {
  const auto GdbUnixPath = fextl::fmt::format("{}/FEX_gdbserver/", FEXServerClient::GetTempFolder());
  if (FHU::Filesystem::CreateDirectory(GdbUnixPath) == FHU::Filesystem::CreateDirectoryResult::ERROR) {
    LogMan::Msg::EFmt("[GdbServer] Couldn't create gdbserver folder {}", GdbUnixPath);
    return;
  }

  GdbUnixSocketPath = fextl::fmt::format("{}{}-gdb", GdbUnixPath, ::getpid());

  ListenSocket = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
  if (ListenSocket == -1) {
    LogMan::Msg::EFmt("[GdbServer] Couldn't open AF_UNIX socket {} {}", errno, strerror(errno));
    return;
  }

  struct sockaddr_un addr {};
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, GdbUnixSocketPath.data(), sizeof(addr.sun_path));
  size_t SizeOfAddr = offsetof(sockaddr_un, sun_path) + GdbUnixSocketPath.size();

  // Bind the socket to the path
  int Result {};
  for (int attempt = 0; attempt < 2; ++attempt) {
    Result = bind(ListenSocket, reinterpret_cast<struct sockaddr*>(&addr), SizeOfAddr);
    if (Result == 0) {
      break;
    }

    // This can happen periodically with execve. unlink the path and try again.
    // The PID is reused but FEX likely started a gdbserver thread for the PID before execve.
    unlink(GdbUnixSocketPath.c_str());
  }

  if (Result != 0) {
    LogMan::Msg::EFmt("[GdbServer] Couldn't bind AF_UNIX socket '{}': {} {}\n", addr.sun_path, errno, strerror(errno));
    close(ListenSocket);
    ListenSocket = -1;
    return;
  }

  listen(ListenSocket, 1);
  LogMan::Msg::IFmt("[GdbServer] Waiting for connection on {}", GdbUnixSocketPath);
  LogMan::Msg::IFmt("[GdbServer] gdb-multiarch -ex \"set debug remote 1\" -ex \"target extended-remote {}\"", GdbUnixSocketPath);
}

void GdbServer::CloseListenSocket() {
  if (ListenSocket != -1) {
    close(ListenSocket);
    ListenSocket = -1;
  }
  unlink(GdbUnixSocketPath.c_str());
}

fextl::unique_ptr<std::iostream> GdbServer::OpenSocket() {
  // Block until a connection arrives
  struct sockaddr_storage their_addr {};
  socklen_t addr_size {};

  int new_fd = accept(ListenSocket, (struct sockaddr*)&their_addr, &addr_size);

  return fextl::make_unique<FEXCore::Utils::NetStream>(new_fd);
}

#endif
} // namespace FEX
