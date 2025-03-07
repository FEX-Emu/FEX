// SPDX-License-Identifier: MIT
#include "LinuxSyscalls/x32/Types.h"
#include "LinuxSyscalls/x32/Ioctl/HelperDefines.h"

#include <cstdint>
#include <linux/usbdevice_fs.h>
#include <sys/ioctl.h>

namespace FEX::HLE::x32 {
namespace usbdev {
#ifndef USBDEVFS_GET_SPEED
#define USBDEVFS_GET_SPEED _IO('U', 31)
#endif
#ifndef USBDEVFS_CONNINFO_EX
#define USBDEVFS_CONNINFO_EX(len) _IOC(_IOC_READ, 'U', 32, len)
#endif
#ifndef USBDEVFS_FORBID_SUSPEND
#define USBDEVFS_FORBID_SUSPEND _IO('U', 33)
#endif
#ifndef USBDEVFS_ALLOW_SUSPEND
#define USBDEVFS_ALLOW_SUSPEND _IO('U', 34)
#endif
#ifndef USBDEVFS_WAIT_FOR_RESUME
#define USBDEVFS_WAIT_FOR_RESUME _IO('U', 35)
#endif
#include "LinuxSyscalls/x32/Ioctl/usbdev.inl"
} // namespace usbdev
} // namespace FEX::HLE::x32
