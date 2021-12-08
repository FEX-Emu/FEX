#pragma once

#include <FEXCore/Utils/CompilerDefs.h>

#include <array>
#include <iostream>
#include <iterator>
#include <string.h>

namespace FEXCore::Utils {
class FEX_DEFAULT_VISIBILITY NetStream : public std::iostream {
public:
    explicit NetStream(int socketfd) : std::iostream(new NetBuf(socketfd)) {}
    ~NetStream() override;

private:
    class NetBuf : public std::streambuf {
    public:
        explicit NetBuf(int socketfd) : socket{socketfd} {
            reset_output_buffer();
        }
        ~NetBuf() override;

    protected:
        std::streamsize xsputn(const char* buffer, std::streamsize size) override;

        std::streambuf::int_type underflow() override;
        std::streambuf::int_type overflow(std::streambuf::int_type ch) override;
        int sync() override;

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
}
