// SPDX-License-Identifier: MIT
#include "LinuxSyscalls/x32/Types.h"
#include "LinuxSyscalls/x32/Ioctl/HelperDefines.h"

#include <cstdint>
#include <sys/ioctl.h>

namespace FEX::HLE::x32 {
namespace streams {
#ifndef TIOCGPTPEER
#define TIOCGPTPEER _IO('T', 0x41)
#endif
#include "LinuxSyscalls/x32/Ioctl/streams.inl"
} // namespace streams

} // namespace FEX::HLE::x32
