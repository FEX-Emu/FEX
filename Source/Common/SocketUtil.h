#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

namespace SocketUtil {
  size_t ReadDataFromSocket(int Socket, std::vector<uint8_t> &Data);
}
