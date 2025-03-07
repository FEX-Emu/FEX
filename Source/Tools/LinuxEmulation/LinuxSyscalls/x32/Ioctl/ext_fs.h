// SPDX-License-Identifier: MIT
#pragma once

#include "LinuxSyscalls/x32/Types.h"
#include "LinuxSyscalls/x32/Ioctl/HelperDefines.h"

#include <cstdint>
#include <linux/blktrace_api.h>
#include <linux/fs.h>
#include <linux/fiemap.h>
#include <sys/ioctl.h>

namespace FEX::HLE::x32 {

namespace ext_fs {
#include "LinuxSyscalls/x32/Ioctl/ext_fs.inl"
}
} // namespace FEX::HLE::x32
