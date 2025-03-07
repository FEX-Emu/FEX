// SPDX-License-Identifier: MIT
#include "LinuxSyscalls/x64/Types.h"
#include "LinuxSyscalls/x64/Ioctl/HelperDefines.h"

#include <cstdint>
#include <linux/input.h>
#include <sys/ioctl.h>

namespace FEX::HLE::x64 {
namespace input {
#include "LinuxSyscalls/x64/Ioctl/input.inl"
}
} // namespace FEX::HLE::x64
