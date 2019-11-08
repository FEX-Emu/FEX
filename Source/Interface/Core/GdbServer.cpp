#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <memory>
#include "Common/NetStream.h"
#include "LogManager.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>

#include "GdbServer.h"



static int calculateChecksum(std::string &packet) {
    unsigned char checksum = 0;
    for (const char &c : packet) {
        checksum += c;
    }
    return checksum;
}


// Packet parser
// Takes a serial stream and reads a single packet
// Un-escapes chars, checks the checksum and request a retransmit if it fails.
// Once the checksum is validated, it acknowledges and returns the packet in a string
static std::string ReadPacket(std::iostream &stream) {
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

static void SendPacket(std::ostream &stream, std::string packet) {
    LogMan::Msg::E("GdbServer Reply: %s", packet.c_str());
    stream << '$' << packet << '#';
    stream << std::setfill('0') << std::setw(2) << std::hex << (int)calculateChecksum(packet);
    stream << std::setfill('*') << std::setw(0) << std::flush;
}

static std::string handleQuery(std::string &packet) {
    if (packet.rfind("qSupported", 0) == 0) {
        return "PacketSize=2000";
    }
    return "";
}

static std::string ProcessPacket(std::string &packet) {
    switch (packet[0]) {
    case 'q':
        return handleQuery(packet);
    default:
        return "";
    }
}

static void GdbServerLoop(std::unique_ptr<std::iostream> stream) {
    std::string responce;

    // Outer server loop. Handles packet start, ACK/NAK and break

    int c;
    while ((c = stream->get()) > 0 ) {
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

static std::thread gdbServerThread;

static void StartThread(int socket) {
    auto stream = std::make_unique<NetStream>(socket);

    //gdbServerThread = std::thread(GdbServerLoop, stream);
    GdbServerLoop(std::move(stream));
}

void GdbServerInit() {
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

    StartThread(new_fd);


}