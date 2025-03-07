// SPDX-License-Identifier: MIT
#pragma once

#include "LinuxSyscalls/x64/Types.h"
#include "LinuxSyscalls/x64/Ioctl/HelperDefines.h"

#include <cstdint>
#include <linux/blktrace_api.h>
#include <linux/fs.h>
#include <linux/fiemap.h>
#include <sys/ioctl.h>

namespace FEX::HLE::x64 {

namespace ext_fs {
#include "LinuxSyscalls/x64/Ioctl/ext_fs.inl"
}
} // namespace FEX::HLE::x64
