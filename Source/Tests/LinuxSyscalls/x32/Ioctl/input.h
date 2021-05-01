#include "Tests/LinuxSyscalls/x32/Types.h"
#include "Tests/LinuxSyscalls/x32/Ioctl/HelperDefines.h"

#include <cstdint>
#include <linux/input.h>
#include <sys/ioctl.h>

namespace FEX::HLE::x32 {
namespace input {
#include "Tests/LinuxSyscalls/x32/Ioctl/input.inl"
}
}

