#pragma once

#include "Tests/LinuxSyscalls/x64/Types.h"
#include "Tests/LinuxSyscalls/x64/Ioctl/HelperDefines.h"

#include <cstdint>
#include <linux/joystick.h>
#include <sys/ioctl.h>

namespace FEX::HLE::x64 {

namespace joystick {
#include "Tests/LinuxSyscalls/x64/Ioctl/joystick.inl"
}
}
