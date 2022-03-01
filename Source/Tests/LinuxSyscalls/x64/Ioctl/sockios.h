#include "Tests/LinuxSyscalls/x64/Types.h"
#include "Tests/LinuxSyscalls/x64/Ioctl/HelperDefines.h"

#include <cstdint>
#include <linux/if.h>
#include <linux/sockios.h>
#include <sys/ioctl.h>

namespace FEX::HLE::x64 {
namespace sockios {
#include "Tests/LinuxSyscalls/x64/Ioctl/sockios.inl"
}
}

