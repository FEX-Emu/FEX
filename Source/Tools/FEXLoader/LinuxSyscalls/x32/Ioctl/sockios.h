#include "Tests/LinuxSyscalls/x32/Types.h"
#include "Tests/LinuxSyscalls/x32/Ioctl/HelperDefines.h"

#include <cstdint>
#include <linux/sockios.h>
#include <sys/ioctl.h>

namespace FEX::HLE::x32 {
namespace sockios {
#ifndef SIOCGSKNS
#define SIOCGSKNS	0x894C
#endif
#include "Tests/LinuxSyscalls/x32/Ioctl/sockios.inl"
}
}

