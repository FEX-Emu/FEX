// SPDX-License-Identifier: MIT
#include "LinuxSyscalls/x32/Types.h"
#include "LinuxSyscalls/x32/Ioctl/HelperDefines.h"

#include <cstdint>
#include <linux/wireless.h>
#include <sys/ioctl.h>

namespace FEX::HLE::x32 {
namespace wireless {
#include "LinuxSyscalls/x32/Ioctl/wireless.inl"
}

} // namespace FEX::HLE::x32
