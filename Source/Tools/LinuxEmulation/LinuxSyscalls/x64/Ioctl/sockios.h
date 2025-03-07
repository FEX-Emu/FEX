// SPDX-License-Identifier: MIT
#include "LinuxSyscalls/x64/Types.h"
#include "LinuxSyscalls/x64/Ioctl/HelperDefines.h"

#include <cstdint>
#include <linux/if.h>
#include <linux/sockios.h>
#include <sys/ioctl.h>

namespace FEX::HLE::x64 {
namespace sockios {
#include "LinuxSyscalls/x64/Ioctl/sockios.inl"
}
} // namespace FEX::HLE::x64
