#pragma once

#include "Tests/LinuxSyscalls/x32/Types.h"
#include "Tests/LinuxSyscalls/x32/Ioctl/HelperDefines.h"

#include <cstdint>
#include <linux/msdos_fs.h>
#include <sys/ioctl.h>

namespace FEX::HLE::x32 {

namespace msdos_fs {
#include "Tests/LinuxSyscalls/x32/Ioctl/msdos_fs.inl"
}
}
