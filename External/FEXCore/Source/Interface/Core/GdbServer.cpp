#include <cstdlib>
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <memory>
#include <optional>
#include "Common/NetStream.h"
#include "Common/SoftFloat.h"
#include "LogManager.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <fstream>

#include "GdbServer.h"
#include <FEXCore/Core/CodeLoader.h>
#include <FEXCore/Core/X86Enums.h>

namespace FEXCore
{

void GdbServer::Break() {
    std::lock_guard lk(sendMutex);
    if (CommsStream)
        SendPacket(*CommsStream, "S05");
}

GdbServer::GdbServer(FEXCore::Context::Context *ctx) : CTX(ctx) {
    ctx->CustomExitHandler = [&](uint64_t ThreadId, FEXCore::Context::ExitReason ExitReason) {
        if (ExitReason == FEXCore::Context::ExitReason::EXIT_DEBUG) {
            this->Break();
        }
    };

    StartThread();
}

static int calculateChecksum(std::string &packet) {
    unsigned char checksum = 0;
    for (const char &c : packet) {
        checksum += c;
    }
    return checksum;
}

static std::string hexstring(std::istringstream &ss, int delm) {
    std::string ret;

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

static std::string encodeHex(unsigned char *data, size_t length) {
    std::ostringstream ss;

    for (size_t i=0; i < length; i++) {
        ss << std::setfill('0') << std::setw(2) << std::hex << int(data[i]);
    }
    return ss.str();
}

static std::string getThreadName(uint32_t ThreadID) {
  std::fstream fs;
  std::ostringstream ThreadFile;
  ThreadFile << "/proc/" << getpid() << "/task/" << ThreadID << "/comm";

  fs.open(ThreadFile.str(), std::fstream::in | std::fstream::binary);
  if (fs.is_open()) {
    std::string ThreadName;
    fs >> ThreadName;
    fs.close();
    return ThreadName;
  }

  return "<No Name>";
}

// Packet parser
// Takes a serial stream and reads a single packet
// Un-escapes chars, checks the checksum and request a retransmit if it fails.
// Once the checksum is validated, it acknowledges and returns the packet in a string
std::string GdbServer::ReadPacket(std::iostream &stream) {
    std::string packet{};

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
                LogMan::Msg::E("Dropping unexpected data: \"%s\"", packet.c_str());

            // clear any existing data, must have been a mistake.
            packet = std::string();
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
                LogMan::Msg::E("Received Invalid Packet: $%s#%02x %c%c", packet.c_str(), expected_checksum);
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

static std::string escapePacket(std::string packet) {
    std::ostringstream ss;

    for(auto &c : packet) {
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

void GdbServer::SendPacket(std::ostream &stream, std::string packet) {
  auto escaped = escapePacket(packet);
  std::ostringstream ss;

  ss << '$' << escaped << '#';
  ss << std::setfill('0') << std::setw(2) << std::hex << (int)calculateChecksum(escaped);
  stream << ss.str() << std::flush;
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

struct __attribute__((packed)) GDBContextDefinition {
  uint64_t gregs[16];
  uint64_t rip;
  uint32_t eflags;
  uint32_t cs, ss, ds, es, fs, gs;
  X80SoftFloat mm[8];
  uint32_t fctrl;
  uint32_t fstat;
  uint32_t dummies[6];
  uint64_t xmm[16][2];
  uint32_t mxcsr;
};

std::string GdbServer::readRegs() {
  GDBContextDefinition GDB{};
  FEXCore::Core::CPUState state{};

  auto Threads = CTX->GetThreads();
  bool Found = false;

  for (auto &Thread : *Threads) {
    if (Thread->State.ThreadManager.GetTID() != CurrentDebuggingThread) {
      continue;
    }
    state = Thread->State.State;
    Found = true;
    break;
  }

  if (!Found) {
    // If set to an invalid thread then just get the parent thread ID
    state = CTX->GetCPUState();
  }

  // Encode the GDB context definition
  memcpy(&GDB.gregs[0], &state.gregs[0], sizeof(GDB.gregs));
  memcpy(&GDB.rip, &state.rip, sizeof(GDB.rip));

  for (size_t i = 0; i < 32; ++i) {
    uint64_t Flag = state.flags[i];
    GDB.eflags |= (Flag << i);
  }

  for (size_t i = 0; i < 8; ++i) {
    memcpy(&GDB.mm[i], &state.mm[i], sizeof(GDB.mm));
  }

  // Currently unsupported
  GDB.fctrl = 0x37F;

  GDB.fstat  = static_cast<uint32_t>(state.flags[FEXCore::X86State::X87FLAG_TOP_LOC]) << 11;
  GDB.fstat |= static_cast<uint32_t>(state.flags[FEXCore::X86State::X87FLAG_C0_LOC]) << 8;
  GDB.fstat |= static_cast<uint32_t>(state.flags[FEXCore::X86State::X87FLAG_C1_LOC]) << 9;
  GDB.fstat |= static_cast<uint32_t>(state.flags[FEXCore::X86State::X87FLAG_C2_LOC]) << 10;
  GDB.fstat |= static_cast<uint32_t>(state.flags[FEXCore::X86State::X87FLAG_C3_LOC]) << 14;

  memcpy(&GDB.xmm[0], &state.xmm[0], sizeof(GDB.xmm));

  return encodeHex((unsigned char *)&GDB, sizeof(GDBContextDefinition));
}

GdbServer::HandledPacketType GdbServer::readReg(std::string& packet) {
	size_t addr;
	auto ss = std::istringstream(packet);
	ss.get(); // Drop first letter
	ss >> std::hex >> addr;

  FEXCore::Core::CPUState state{};

  auto Threads = CTX->GetThreads();
  bool Found = false;

  for (auto &Thread : *Threads) {
    if (Thread->State.ThreadManager.GetTID() != CurrentDebuggingThread) {
      continue;
    }
    state = Thread->State.State;
    Found = true;
    break;
  }

  if (!Found) {
    // If set to an invalid thread then just get the parent thread ID
    state = CTX->GetCPUState();
  }


  if (addr >= offsetof(GDBContextDefinition, gregs[0]) &&
      addr < offsetof(GDBContextDefinition, gregs[16])) {
    return {encodeHex((unsigned char *)(&state.gregs[addr / sizeof(uint64_t)]), sizeof(uint64_t)), HandledPacketType::TYPE_ACK};
  }
  else if (addr == offsetof(GDBContextDefinition, rip)) {
    return {encodeHex((unsigned char *)(&state.rip), sizeof(uint64_t)), HandledPacketType::TYPE_ACK};
  }
  else if (addr == offsetof(GDBContextDefinition, eflags)) {
    uint32_t eflags{};
    for (size_t i = 0; i < 32; ++i) {
      uint64_t Flag = state.flags[i];
      eflags |= (Flag << i);
    }
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
    return {encodeHex((unsigned char *)(&state.xmm[(addr - offsetof(GDBContextDefinition, xmm[0][0])) / 16][0]), 16), HandledPacketType::TYPE_ACK};
  }
  else if (addr == offsetof(GDBContextDefinition, mxcsr)) {
    uint32_t Empty{};
    return {encodeHex((unsigned char *)(&Empty), sizeof(uint32_t)), HandledPacketType::TYPE_ACK};
  }

  LogMan::Msg::E("Unknown GDB register 0x%lx", addr);
  return {"E00", HandledPacketType::TYPE_ACK};
}

std::string buildTargetXML() {
    std::ostringstream xml;

    xml << "<?xml version='1.0'?>\n";
    xml << "<!DOCTYPE target SYSTEM 'gdb-target.dtd'>\n";\
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
            for (int i=0; i < 16; i++) {
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
                reg("st" + std::to_string(i), "i387_ext", 80);
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
            for (int i=0; i < 16; i++) {
                reg("xmm" + std::to_string(i), "vec128", 128);
            }

            reg("mxcsr", "int", 32);

        xml << "</feature>\n";
    xml << "</target>";
    xml << std::flush;

    return xml.str();
}

GdbServer::HandledPacketType GdbServer::handleXfer(std::string &packet) {
    std::string object;
    std::string rw;
    std::string annex;
    int annex_pid;
    int offset;
    int length;

    // Parse Xfer message
    {
        auto ss = std::istringstream(packet);
        std::string expectXfer;
        char expectComma;

        std::getline(ss, expectXfer, ':');
        std::getline(ss, object, ':');
        std::getline(ss, rw, ':');
        std::getline(ss, annex, ':');
        if (annex == "") {
          annex_pid = getpid();
        }
        else {
          auto ss_pid = std::istringstream(annex);
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
    auto encode = [&](std::string data) -> std::string {
        if (offset == data.size())
            return "l";
        if (offset >= data.size())
            return "E34"; // ERANGE
        if ((data.size() - offset) > length)
            return "m" + data.substr(offset, length);
        return "l" + data.substr(offset);
    };

    if (object == "exec-file") {
      if (annex_pid == getpid())
        return {encode(CTX->SyscallHandler->GetFilename()), HandledPacketType::TYPE_ACK};

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
        std::ostringstream ss;
        ss << "<?xml version=\"1.0\?>\n";
        ss << "<threads>\n";
        for (size_t i = 0; i < Threads->size(); ++i) {
          auto Thread = Threads->at(i);
          ss << "\t<thread id=\"" << std::hex << Thread->State.ThreadManager.GetTID() << "\" core=\"" << i << "\" name=\"" <<  getThreadName(Thread->State.ThreadManager.GetTID()) << "\">\n";
          ss << "\t</thread>\n";
        }

        ss << "</threads>";
        ss << std::flush;
        ThreadString = ss.str();
      }

      return {encode(ThreadString.substr(offset, length)), HandledPacketType::TYPE_ACK};
    }
    return {"", HandledPacketType::TYPE_UNKNOWN};
}

static size_t CheckMemMapping(uint64_t Address, size_t Size) {
  uint64_t AddressEnd = Address + Size;

  std::fstream fs;
  fs.open("/proc/self/maps", std::fstream::in | std::fstream::binary);
  std::string Line;
  while (std::getline(fs, Line)) {
    if (fs.eof()) break;
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

  fs.close();
  return 0;
}

GdbServer::HandledPacketType GdbServer::handleProgramOffsets() {
  std::fstream fs;
  fs.open("/proc/self/maps", std::fstream::in | std::fstream::binary);
  std::string Line;
  std::string const &RuntimeExecutable = CTX->SyscallHandler->GetFilename();
  while (std::getline(fs, Line)) {
    uint64_t Begin, End;
    char Filename[255];
    if (sscanf(Line.c_str(), "%lx-%lx %*c%*c%*c%*c %*x %*x:%*x %*d%s", &Begin, &End, Filename) == 3) {
      if (RuntimeExecutable == Filename) {
        std::ostringstream ss;
        ss << "Text=" << std::hex << Begin << ";Data=" << std::hex << Begin << ";Bss=" << std::hex << Begin;
        ss << std::flush;
        return {ss.str(), HandledPacketType::TYPE_ACK};
      }
    }
  }
  fs.close();
  return {"Text=0;Data=0;Bss=0", HandledPacketType::TYPE_ACK};
}

GdbServer::HandledPacketType GdbServer::handleMemory(std::string &packet) {
    bool write;
    size_t addr;
    size_t length;
    std::string data;

    auto ss = std::istringstream(packet);
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
    if (!CTX->Config.UnifiedMemory) {
      ptr = CTX->MemoryMapper.GetPointer(addr);
    }

    if (write) {
        std::memcpy(ptr, data.data(), data.length());
        // TODO: invalidate any code
        return {"OK", HandledPacketType::TYPE_ACK};
    } else {
        return {encodeHex((unsigned char*)ptr, length), HandledPacketType::TYPE_ACK};
    }
}


GdbServer::HandledPacketType GdbServer::handleQuery(std::string &packet) {
  auto match = [&](const char *str) -> bool { return packet.rfind(str, 0) == 0; };

  if (match("qSupported")) {
    return {"PacketSize=5000;xmlRegisters=i386;qXfer:exec-file:read+;qXfer:features:read+;", HandledPacketType::TYPE_ACK};
  }
  if (match("qAttached")) {
    return {"1", HandledPacketType::TYPE_ACK}; // We don't currently support launching executables from gdb.
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

    std::ostringstream ss;
    ss << "m";
    for (size_t i = 0; i < Threads->size(); ++i) {
      auto Thread = Threads->at(i);
      ss << std::hex << Thread->State.ThreadManager.TID << ",";
    }
    return {ss.str(), HandledPacketType::TYPE_ACK};
  }
  if (match("qsThreadInfo")) {
    return {"l", HandledPacketType::TYPE_ACK};
  }
  if (match("qThreadExtraInfo")) {
    auto ss = std::istringstream(packet);
    ss.seekg(std::string("qThreadExtraInfo").size());
    ss.get(); // discard comma
    uint32_t ThreadID;
    ss >> std::hex >> ThreadID;
    auto ThreadName = getThreadName(ThreadID);
    return {encodeHex((unsigned char*)ThreadName.data(), ThreadName.size()), HandledPacketType::TYPE_ACK};
  }
  if (match("qC")) {
    // Returns the current Thread ID
    std::ostringstream ss;
    ss << "m" <<  std::hex << CTX->ParentThread->State.ThreadManager.TID;
    return {ss.str(), HandledPacketType::TYPE_ACK};
  }
  if (match("QStartNoAckMode")) {
    SettingNoAckMode = true;
    return {"OK", HandledPacketType::TYPE_ACK};
  }
  if (match("qSymbol")) {
    return {"OK", HandledPacketType::TYPE_ACK};
  }

  return {"", HandledPacketType::TYPE_UNKNOWN};
}

GdbServer::HandledPacketType GdbServer::handleV(std::string& packet) {
    auto match = [&](std::string str) -> std::optional<std::istringstream> {
        if (packet.rfind(str, 0) == 0) {
            auto ss = std::istringstream(packet);
            ss.seekg(str.size());
            return ss;
        }
        return std::nullopt;
    };

    auto F = [](int result) {
        std::ostringstream ss;
        ss << "F" << std::hex << result;
        return ss.str(); };
    auto F_error = [&]() {
        std::ostringstream ss;
        ss << "F-1," << std::hex << errno;
        return ss.str(); };
    auto F_data = [&](int result, std::string data) {
        std::ostringstream ss;
        ss << "F" << std::hex << result << ";" << data;
        return ss.str(); };

    std::optional<std::istringstream> ss;
    if((ss = match("vFile:open:"))) {
        std::string filename;
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

        std::string data(count, '\0');
        if (lseek(fd, offset, SEEK_SET) < 0) {
            return {F_error(), HandledPacketType::TYPE_ACK};
        }
        int ret = read(fd, data.data(), count);
        if (ret < 0) {
            return {F_error(), HandledPacketType::TYPE_ACK};
        }
        data.resize(ret);
        return {F_data(ret, data), HandledPacketType::TYPE_ACK};
    }
    if ((ss = match("vCont?"))) {
        return {"vCont;c;C;t;s;S;r", HandledPacketType::TYPE_ACK}; // We support continue, step and terminate
                                // FIXME: We also claim to support continue with signal... because it's compulsory
    }
    if ((ss = match("vCont;"))) {
        char action;
        int thread;

        action = ss->get();

        if (ss->peek() == ':') {
            ss->get();
            *ss >> std::hex >> thread;
        }

        if (ss->fail()) {
 						return {"E00", HandledPacketType::TYPE_ACK};
        }

        switch (action) {
        case 'c': {
            CTX->Run();
            return {"", HandledPacketType::TYPE_ONLYACK};
          }
        case 's': {
            CTX->Step();
						SendPacketPair({"OK", HandledPacketType::TYPE_ACK});
            std::ostringstream ss;
            ss << "T05thread:" << std::setfill('0') << std::setw(2) << std::hex << getpid() << ";core:2c;";

            SendPacketPair({ss.str(), HandledPacketType::TYPE_ACK});
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
		return {"", HandledPacketType::TYPE_ACK};
}

GdbServer::HandledPacketType GdbServer::handleThreadOp(std::string &packet) {
  auto match = [&](const char *str) -> bool { return packet.rfind(str, 0) == 0; };

  if (match("Hc")) {
    // Sets thread to this ID for stepping
    // This is deprecated and vCont should be used instead
    auto ss = std::istringstream(packet);
    ss.seekg(std::string("Hc").size());
    ss >> std::hex >> CurrentDebuggingThread;

    CTX->Pause();
    return {"OK", HandledPacketType::TYPE_ACK};
  }

  if (match("Hg")) {
    // Sets thread for "other" operations
    auto ss = std::istringstream(packet);
    ss.seekg(std::string("Hg").size());
    ss >> std::hex >> CurrentDebuggingThread;

    // This must return quick otherwise IDA complains
    CTX->Pause();
    return {"OK", HandledPacketType::TYPE_ACK};
  }

  return {"", HandledPacketType::TYPE_UNKNOWN};
}

GdbServer::HandledPacketType GdbServer::handleBreakpoint(std::string &packet) {
  auto ss = std::istringstream(packet);

  bool Set{};
  uint64_t Addr;
  uint64_t Type;
  Set = ss.get() == 'Z';

  ss >> std::hex >> Addr;
  ss.get(); // discard comma
  ss >> std::hex >> Type;

  CTX->Pause();
  return {"OK", HandledPacketType::TYPE_ACK};
}

GdbServer::HandledPacketType GdbServer::ProcessPacket(std::string &packet) {
  switch (packet[0]) {
    case '?': {
      // Indicates the reason that the thread has stopped
      // Behaviour changes if the target is in non-stop mode
      // Binja doesn't support S response here
      //return {"S00", HandledPacketType::TYPE_ACK};
      std::ostringstream ss;
      ss << "T00thread:" << std::setfill('0') << std::setw(2) << std::hex << getpid() << ";core:2c;";

      return {ss.str(), HandledPacketType::TYPE_ACK};
    }
    case 'g':
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

void GdbServer::SendPacketPair(HandledPacketType response) {
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
  while (!CTX->CoreShuttingDown.load()) {
    CommsStream = OpenSocket();

    HandledPacketType response{};

    // Outer server loop. Handles packet start, ACK/NAK and break

    int c;
    while ((c = CommsStream->get()) >= 0 ) {
        switch (c) {
        case '$': {
            std::string packet = ReadPacket(*CommsStream);
            response = ProcessPacket(packet);
            SendPacketPair(response);
            if (response.TypeResponse == HandledPacketType::TYPE_UNKNOWN) {
              LogMan::Msg::D("Unknown packet %s", packet.c_str());
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
            std::ostringstream ss;
            ss << "T02thread:" << std::setfill('0') << std::setw(2) << std::hex << getpid() << ";core:2c;";
            SendPacketPair({ss.str(), HandledPacketType::TYPE_ACK});
            break;
          }
        default:
            LogMan::Msg::D("GdbServer: Unexpected byte %c (%02x)", c, c);
        }
    }

    {
        std::lock_guard lk(sendMutex);
        CommsStream.release();
    }
  }
}

void GdbServer::StartThread() {
    gdbServerThread = std::thread(&GdbServer::GdbServerLoop, this);
}

std::unique_ptr<std::iostream> GdbServer::OpenSocket() {
    // open socket
    int sockfd, new_fd;

    struct addrinfo hints, *res;
    struct sockaddr_storage their_addr;
    socklen_t addr_size;


    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if(getaddrinfo(NULL, "8086", &hints, &res) < 0) {
        perror("getaddrinfo");
    }

    int on = 1;

    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd < 0) {
        perror("socket");
    }
    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on)) < 0) {
        perror("setsockopt");
    }

    if (bind(sockfd, res->ai_addr, res->ai_addrlen) < 0) {
        perror("bind");
    }

    // Block until a connection arrives

    LogMan::Msg::I("GdbServer, waiting for connection on localhost:8086");
    listen(sockfd, 1);

    new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);

    return std::make_unique<NetStream>(new_fd);
}


} // namespace FEXCore
