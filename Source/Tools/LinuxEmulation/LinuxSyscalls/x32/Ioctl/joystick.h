// SPDX-License-Identifier: MIT
#pragma once

#include "LinuxSyscalls/x32/Types.h"
#include "LinuxSyscalls/x32/Ioctl/HelperDefines.h"

#include <cstdint>
#include <linux/joystick.h>
#include <sys/ioctl.h>

namespace FEX::HLE::x32 {

namespace joystick {
#include "LinuxSyscalls/x32/Ioctl/joystick.inl"
}
} // namespace FEX::HLE::x32
