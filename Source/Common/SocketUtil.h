#pragma once
#include <FEXCore/fextl/vector.h>

#include <cstddef>
#include <cstdint>

namespace SocketUtil {
  size_t ReadDataFromSocket(int Socket, fextl::vector<uint8_t> &Data);
}
