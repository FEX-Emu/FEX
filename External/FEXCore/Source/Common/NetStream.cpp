#include "NetStream.h"

#include <cstring>

#include <sys/types.h>
#include <sys/socket.h>

#include <stdio.h>
#include <unistd.h>

int NetStream::NetBuf::flushBuffer(const char *buffer, size_t size) {
    size_t total = 0;

    // Send data
    while (total < size) {
        size_t sent = send(socket, (const void*)(buffer + total), size - total, 0);
        if (sent == -1) {
            // lets just assume all errors are end of file.
            return -1;
        }
        total += sent;
    }

    return 0;
}

std::streamsize NetStream::NetBuf::xsputn(const char* buffer, std::streamsize size) {
    size_t buf_remaining = epptr() - pptr();

    // Check if the string fits neatly in our buffer
    if (size <= buf_remaining) {
        std::memcpy(pptr(), buffer, size);
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

std::streambuf::int_type NetStream::NetBuf::overflow(std::streambuf::int_type ch) {
    // we always leave room for one extra char
    *pptr() = (char) ch;
    pbump(1);
    return sync();
}

int NetStream::NetBuf::sync() {
    // Flush and reset output buffer to zero
    if(flushBuffer(pbase(), pptr() - pbase()) < 0) {
        return -1;
    }
    reset_output_buffer();
    return 0;
}

std::streambuf::int_type NetStream::NetBuf::underflow() {
    ssize_t size = recv(socket, (void *)std::begin(input_buffer), sizeof(input_buffer), 0);

    if (size <= 0) {
        setg(nullptr, nullptr, nullptr);
        return traits_type::eof();
    }

    setg(&input_buffer[0], &input_buffer[0], &input_buffer[size]);

    return traits_type::to_int_type(*gptr());
}

NetStream::~NetStream() {
    delete rdbuf();
}

NetStream::NetBuf::~NetBuf() {
  close(socket);
}
