// SPDX-License-Identifier: MIT
#pragma once

#include "LinuxSyscalls/x64/Types.h"
#include "LinuxSyscalls/x64/Ioctl/HelperDefines.h"

#include <cstdint>
#include <linux/msdos_fs.h>
#include <sys/ioctl.h>

namespace FEX::HLE::x64 {

namespace msdos_fs {
#include "LinuxSyscalls/x64/Ioctl/msdos_fs.inl"
}
} // namespace FEX::HLE::x64
