// SPDX-License-Identifier: MIT
#pragma once

#include <FEXCore/Utils/CompilerDefs.h>
#include <FEXCore/fextl/string.h>
#include <FEXCore/fextl/vector.h>

#include <chrono>
#include <optional>

namespace FEX::Utils {
class NetStream final {
public:
  NetStream(int socketfd)
    : socketfd {socketfd}
    , receive_buffer(1500) {}

  struct ReturnGet {
    char data;
    bool Hangup;
  };
  std::optional<ReturnGet> get(std::optional<std::chrono::seconds> timeout = std::nullopt);
  size_t read(char* buf, size_t size);

  bool SendPacket(const fextl::string& packet);
private:
  int socketfd;
  size_t read_offset {};
  size_t receive_offset {};
  fextl::vector<char> receive_buffer;
};
} // namespace FEX::Utils
