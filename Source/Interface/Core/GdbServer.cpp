#include <cstdlib>
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <memory>
#include <optional>
#include "Common/NetStream.h"
    #include "LogManager.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "GdbServer.h"
#include <FEXCore/Core/CodeLoader.h>

namespace FEXCore
{

GdbServer::GdbServer(FEXCore::Context::Context *ctx, FEXCore::CodeLoader *Loader) : CTX(ctx) {
    std::tie(std::ignore, std::ignore, data_offset) = Loader->GetLayout();
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


// Packet parser
// Takes a serial stream and reads a single packet
// Un-escapes chars, checks the checksum and request a retransmit if it fails.
// Once the checksum is validated, it acknowledges and returns the packet in a string
std::string GdbServer::ReadPacket(std::iostream &stream) {
    std::string packet;

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
                LogMan::Msg::E("Received Packet: \"%s\"", packet.c_str());
                stream << "+" << std::flush;
                return packet;
            } else {
                LogMan::Msg::E("Received Invalid Packet: $%s#%02x %c%c", packet.c_str(), expected_checksum);
                stream << "-" << std::flush;
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
    LogMan::Msg::E("GdbServer Reply: %s", escaped.c_str());
    stream << '$' << escaped << '#';
    stream << std::setfill('0') << std::setw(2) << std::hex << (int)calculateChecksum(escaped);
    stream << std::flush;
}

std::string GdbServer::handleXfer(std::string &packet) {

    std::string object;
    std::string rw;
    std::string annex;
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
        ss >> std::hex >> offset;
        ss.get(expectComma);
        ss >> std::hex >> length;

        // Bail on any errors
        if (ss.fail() || !ss.eof() || expectXfer != "qXfer" || rw != "read" || expectComma != ',')
            return "E00";
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
        if (annex == "")
            return encode(CTX->SyscallHandler.GetFilename());
        return "E00";
    }
    return "";
}

std::string GdbServer::handleMemory(std::string &packet) {
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
        return "E00";
    }

    // TODO: check we are in a valid memory range
    //       Also, clamp length
    void* ptr = CTX->MemoryMapper.GetPointer(addr);

    if (write) {
        std::memcpy(ptr, data.data(), data.length());
        // TODO: invalidate any code
        return "OK";
    } else {
        return encodeHex((unsigned char*)ptr, length);
    }
}


std::string GdbServer::handleQuery(std::string &packet) {
    auto match = [&](const char *str) -> bool { return packet.rfind(str, 0) == 0; };

    if (match("qSupported")) {
        return "PacketSize=2000;qXfer:exec-file:read+";
    }
    if (match("qAttached")) {
        return "1"; // We don't currently support launching executables from gdb.
    }
    if (match("qXfer")) {
        return handleXfer(packet);
    }
    if (match("qOffsets")) {
        return "Text=0;Data=0;Bss=0";
    }
    return "";
}

std::string GdbServer::handleV(std::string& packet) {
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

        return F(open(filename.c_str(), flags, mode));
    }
    if((ss = match("vFile:setfs:"))) {
        int pid;
        *ss >> pid;

        F(pid == 0 ? 0 : -1); // Only support the common filesystem
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
            return F_error();
        }
        int ret = read(fd, data.data(), count);
        if (ret < 0) {
            return F_error();
        }
        data.resize(ret);
        return F_data(ret, data);
    }
    return "";
}

std::string readRegs() {
    std::ostringstream ss;

    // The gdb RDP protocol just squirts 27 registers, as 16 digit hex onto the wire.
    ss << std::setfill('0') << std::setw(16) << std::hex;

    for (uint64_t i = 0; i < 70; i++) {
        // I don't know how the regs map... lets just ask
        uint64_t data = (i | 0x80dd000000cc0000 | i << 32 | i << 56);
        if (i == 16) { // RIP
            data = 0x47c990;
        }
        ss << std::setfill('0') << std::setw(16) << std::hex << __builtin_bswap64(data);
    }
    return ss.str();
}


std::string GdbServer::ProcessPacket(std::string &packet) {
    switch (packet[0]) {
    case '?':
        return "S00";
    case 'g':
        return readRegs();
    case 'q':
        return handleQuery(packet);
    case 'v':
        return handleV(packet);
    case 'm': // Memory read
    case 'M': // Memory write
        return handleMemory(packet);
    default:
        return "";
    }
}

void GdbServer::GdbServerLoop(std::unique_ptr<std::iostream> stream) {
    std::string responce;

    // Outer server loop. Handles packet start, ACK/NAK and break

    int c;
    while ((c = stream->get()) >= 0 ) {
        switch (c) {
        case '$': {
            std::string packet = ReadPacket(*stream);
            responce = ProcessPacket(packet);
            LogMan::Msg::E("%s", responce.c_str());
            SendPacket(*stream, responce);
            break;
        }
        case '+':
            // ACK, do nothing.
            break;
        case '-':
            // NAK, Resend requested
            SendPacket(*stream, responce);
            break;
        case '\x03': // ASCII EOT
            LogMan::Msg::E("GdbServer: Break");
            break;
        default:
            LogMan::Msg::E("GdbServer: Unexpected byte %c (%02x)", c, c);
        }
    }
}

void GdbServer::StartThread(std::unique_ptr<std::iostream> stream) {
    //gdbServerThread = std::thread(GdbServerLoop, stream);
    GdbServerLoop(std::move(stream));
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

    LogMan::Msg::E("GdbServer, waiting for connection");
    listen(sockfd, 1);

    new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);
    LogMan::Msg::E("Connected");

    return std::make_unique<NetStream>(new_fd);
}


} // namespace FEXCore