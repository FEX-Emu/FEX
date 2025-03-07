// SPDX-License-Identifier: MIT
#include "LinuxSyscalls/x64/Types.h"
#include "LinuxSyscalls/x64/Ioctl/HelperDefines.h"

#include <cstdint>
#include <linux/wireless.h>
#include <sys/ioctl.h>

namespace FEX::HLE::x64 {
namespace wireless {
#include "LinuxSyscalls/x64/Ioctl/wireless.inl"
}

} // namespace FEX::HLE::x64
