// SPDX-License-Identifier: MIT
#pragma once

#include "LinuxSyscalls/x64/Types.h"
#include "LinuxSyscalls/x64/Ioctl/HelperDefines.h"

#include <cstdint>
#include <linux/joystick.h>
#include <sys/ioctl.h>

namespace FEX::HLE::x64 {

namespace joystick {
#include "LinuxSyscalls/x64/Ioctl/joystick.inl"
}
} // namespace FEX::HLE::x64
