#include "Tests/LinuxSyscalls/x32/Types.h"
#include "Tests/LinuxSyscalls/x32/Ioctl/HelperDefines.h"

#include <cstdint>
#include <sys/ioctl.h>

namespace FEX::HLE::x32 {
namespace streams {
#include "Tests/LinuxSyscalls/x32/Ioctl/streams.inl"
}

}
