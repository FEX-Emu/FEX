#pragma once

#include "Tests/LinuxSyscalls/x32/Types.h"
#include "Tests/LinuxSyscalls/x32/Ioctl/HelperDefines.h"

#include <cstdint>
#include <linux/blktrace_api.h>
#include <linux/fs.h>
#include <linux/fiemap.h>
#include <sys/ioctl.h>

namespace FEX::HLE::x32 {

namespace ext_fs {
#include "Tests/LinuxSyscalls/x32/Ioctl/ext_fs.inl"
}
}
