// SPDX-License-Identifier: MIT
#include <FEXCore/Utils/Allocator.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/NetStream.h>

#include <array>
#include <cstring>
#include <iterator>
#ifndef _WIN32
#include <sys/socket.h>
#endif
#include <unistd.h>

namespace FEXCore::Utils {
namespace {
class NetBuf final : public std::streambuf, public FEXCore::Allocator::FEXAllocOperators {
public:
    explicit NetBuf(int socketfd) : socket{socketfd} {
        reset_output_buffer();
    }
    ~NetBuf() override {
        close(socket);
    }

private:
    std::streamsize xsputn(const char* buffer, std::streamsize size) override;

    std::streambuf::int_type underflow() override;
    std::streambuf::int_type overflow(std::streambuf::int_type ch) override;
    int sync() override;

    void reset_output_buffer() {
        // we always leave room for one extra char
        setp(std::begin(output_buffer), std::end(output_buffer) -1);
    }

    int flushBuffer(const char *buffer, size_t size);

    int socket;
    std::array<char, 1400> output_buffer;
    std::array<char, 1500> input_buffer; // enough for a typical packet
};

int NetBuf::flushBuffer(const char *buffer, size_t size) {
#ifndef _WIN32
    size_t total = 0;

    // Send data
    while (total < size) {
        size_t sent = send(socket, (const void*)(buffer + total), size - total, MSG_NOSIGNAL);
        if (sent == -1) {
            // lets just assume all errors are end of file.
            return -1;
        }
        total += sent;
    }

    return 0;
#else
  ERROR_AND_DIE_FMT("Unsupported");
#endif
}

std::streamsize NetBuf::xsputn(const char* buffer, std::streamsize size) {
    size_t buf_remaining = epptr() - pptr();

    // Check if the string fits neatly in our buffer
    if (size <= buf_remaining) {
        ::memcpy(pptr(), buffer, size);
        pbump(size);
        return size;
    }

    // Otherwise, flush the buffer first
    if (sync() < 0) {
        return traits_type::eof();
    }

    if (size > sizeof(output_buffer) / 2) {
        // If we have a large string, bypass the buffer
        flushBuffer(buffer, size);
        return size;
    } else {
        return xsputn(buffer, size);
    }
}

std::streambuf::int_type NetBuf::overflow(std::streambuf::int_type ch) {
    // we always leave room for one extra char
    *pptr() = (char) ch;
    pbump(1);
    return sync();
}

int NetBuf::sync() {
    // Flush and reset output buffer to zero
    if (flushBuffer(pbase(), pptr() - pbase()) < 0) {
        return -1;
    }
    reset_output_buffer();
    return 0;
}

std::streambuf::int_type NetBuf::underflow() {
#ifndef _WIN32
    ssize_t size = recv(socket, (void *)std::begin(input_buffer), sizeof(input_buffer), 0);

    if (size <= 0) {
        setg(nullptr, nullptr, nullptr);
        return traits_type::eof();
    }

    setg(&input_buffer[0], &input_buffer[0], &input_buffer[size]);

    return traits_type::to_int_type(*gptr());
#else
    ERROR_AND_DIE_FMT("Unsupported");
#endif
}
} // Anonymous namespace

NetStream::NetStream(int socketfd) : std::iostream(new NetBuf(socketfd)) {}

NetStream::~NetStream() {
    delete rdbuf();
}

} // namespace FEXCore::Utils
