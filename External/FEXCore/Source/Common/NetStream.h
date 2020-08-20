#pragma once

#include <array>
#include <iostream>
#include <string.h>

class NetStream : public std::iostream {
public:
    NetStream(int socketfd) : std::iostream(new NetBuf(socketfd)) {}
    virtual ~NetStream();

private:
    class NetBuf : public std::streambuf {

    public:
        NetBuf(int socketfd) {
            socket = socketfd;
            reset_output_buffer();
        }
        virtual ~NetBuf();

    protected:
        virtual std::streamsize xsputn(const char* buffer, std::streamsize size);

        virtual std::streambuf::int_type underflow();
        virtual std::streambuf::int_type overflow(std::streambuf::int_type ch);
        virtual int sync();

    private:
        void reset_output_buffer() {
            // we always leave room for one extra char
            setp(std::begin(output_buffer), std::end(output_buffer) -1);
        }

        int flushBuffer(const char *buffer, size_t size);

        int socket;
        std::array<char, 1400> output_buffer;
        std::array<char, 1500> input_buffer; // enough for a typical packet
    };
};
