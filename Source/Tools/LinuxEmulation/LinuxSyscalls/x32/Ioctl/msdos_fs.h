// SPDX-License-Identifier: MIT
#pragma once

#include "LinuxSyscalls/x32/Types.h"
#include "LinuxSyscalls/x32/Ioctl/HelperDefines.h"

#include <cstdint>
#include <linux/msdos_fs.h>
#include <sys/ioctl.h>

namespace FEX::HLE::x32 {

namespace msdos_fs {
#include "LinuxSyscalls/x32/Ioctl/msdos_fs.inl"
}
} // namespace FEX::HLE::x32
