// SPDX-License-Identifier: MIT
#pragma once

#include <FEXCore/Utils/CompilerDefs.h>
#include <FEXCore/fextl/string.h>
#include <FEXCore/fextl/vector.h>

#include <optional>

namespace FEX::Utils {
class NetStream final {
public:
  NetStream()
    : receive_buffer(1500) {}

  void OpenSocket(int _socketfd) {
    socketfd = _socketfd;
  }

  void InvalidateSocket() {
    socketfd = -1;
  }

  bool HasSocket() const {
    return socketfd != -1;
  }

  struct ReturnGet {
    char data;
    bool Hangup;
  };
  std::optional<ReturnGet> get();
  size_t read(char* buf, size_t size);

  bool SendPacket(const fextl::string& packet);
private:
  int socketfd {-1};
  size_t read_offset {};
  size_t receive_offset {};
  fextl::vector<char> receive_buffer;
};
} // namespace FEX::Utils
