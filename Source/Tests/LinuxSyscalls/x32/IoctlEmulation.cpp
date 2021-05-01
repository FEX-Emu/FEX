#include "Ioctl/drm.h"
#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/x32/Syscalls.h"
#include "Tests/LinuxSyscalls/x32/Ioctl/asound.h"
#include "Tests/LinuxSyscalls/x32/Ioctl/drm.h"
#include "Tests/LinuxSyscalls/x32/Ioctl/usbdev.h"
#include "Tests/LinuxSyscalls/x32/Ioctl/streams.h"
#include "Tests/LinuxSyscalls/x32/Ioctl/sockios.h"
#include "Tests/LinuxSyscalls/x32/Ioctl/input.h"
#include "Tests/LinuxSyscalls/x32/Ioctl/joystick.h"
#undef _BASIC_META
#undef _BASIC_META_VAR
#undef _CUSTOM_META
#undef _CUSTOM_META_OFFSET

#include <FEXCore/Utils/LogManager.h>

#include <cstdint>
#include <functional>
extern "C" {
#include <drm/drm.h>
#include <drm/msm_drm.h>
}
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace FEX::HLE::x32 {
#ifdef _M_X86_64
  uint32_t ioctl_32(int fd, uint32_t cmd, uint32_t args) {
    uint32_t Result{};
    __asm volatile("int $0x80;"
        : "=a" (Result)
        : "a" (SYSCALL_x86_ioctl)
        , "b" (fd)
        , "c" (cmd)
        , "d" (args)
        : "memory");
    return Result;
  }
#endif

  static void UnhandledIoctl(const char *Type, int fd, uint32_t cmd, uint32_t args) {
    LogMan::Msg::E("@@@@@@@@@@@@@@@@@@@@@@@@@");
    LogMan::Msg::E("Unhandled %s ioctl(%d, 0x%08x, 0x%08x)", Type, fd, cmd, args);
    LogMan::Msg::E("\tDir  : 0x%x", _IOC_DIR(cmd));
    LogMan::Msg::E("\tType : 0x%x", _IOC_TYPE(cmd));
    LogMan::Msg::E("\tNR   : 0x%x", _IOC_NR(cmd));
    LogMan::Msg::E("\tSIZE : 0x%x", _IOC_SIZE(cmd));
    LogMan::Msg::A("@@@@@@@@@@@@@@@@@@@@@@@@@");
  }

  namespace DRM {
    std::map<uint32_t, std::function<uint32_t(int fd, uint32_t cmd, uint32_t args)>> FDToHandler;

    void CheckAndAddFDDuplication(int fd, int NewFD) {
      auto it = FDToHandler.find(fd);
      if (it != FDToHandler.end()) {
        FDToHandler[NewFD] = it->second;
      }
    }

    uint32_t AMDGPU_Handler(int fd, uint32_t cmd, uint32_t args) {
      switch (_IOC_NR(cmd)) {
        case _IOC_NR(FEX_DRM_IOCTL_AMDGPU_GEM_METADATA): {
          AMDGPU::fex_drm_amdgpu_gem_metadata *val = reinterpret_cast<AMDGPU::fex_drm_amdgpu_gem_metadata*>(args);
          drm_amdgpu_gem_metadata Host_val = *val;
          uint64_t Result = ioctl(fd, DRM_IOCTL_AMDGPU_GEM_METADATA, &Host_val);
          *val = Host_val;
          return Result;
          break;
        }
#define _BASIC_META(x) case _IOC_NR(x):
#define _BASIC_META_VAR(x, args...) case _IOC_NR(x):
#define _CUSTOM_META(name, ioctl_num)
#define _CUSTOM_META_OFFSET(name, ioctl_num, offset)
      // DRM
#include "Tests/LinuxSyscalls/x32/Ioctl/amdgpu_drm.inl"
        {
          return ioctl(fd, cmd, args);
          break;
        }
        default:
          UnhandledIoctl("AMDGPU", fd, cmd, args);
          return -EPERM;
          break;
      }
#undef _BASIC_META
#undef _BASIC_META_VAR
#undef _CUSTOM_META
#undef _CUSTOM_META_OFFSET
      return -EPERM;
    }

    uint32_t MSM_Handler(int fd, uint32_t cmd, uint32_t args) {
      switch (_IOC_NR(cmd)) {
        case _IOC_NR(FEX_DRM_IOCTL_MSM_WAIT_FENCE): {
          MSM::fex_drm_msm_wait_fence *val = reinterpret_cast<MSM::fex_drm_msm_wait_fence*>(args);
          drm_msm_wait_fence Host_val = *val;
          uint64_t Result = ioctl(fd, DRM_IOCTL_MSM_WAIT_FENCE, &Host_val);
          *val = Host_val;
          return Result;
          break;
        }

#define _BASIC_META(x) case _IOC_NR(x):
#define _BASIC_META_VAR(x, args...) case _IOC_NR(x):
#define _CUSTOM_META(name, ioctl_num)
#define _CUSTOM_META_OFFSET(name, ioctl_num, offset)
      // DRM
#include "Tests/LinuxSyscalls/x32/Ioctl/msm_drm.inl"
        {
          return ioctl(fd, cmd, args);
          break;
        }
        default:
          UnhandledIoctl("MSM", fd, cmd, args);
          return -EPERM;
          break;
      }
#undef _BASIC_META
#undef _BASIC_META_VAR
#undef _CUSTOM_META
#undef _CUSTOM_META_OFFSET
      return -EPERM;
    }

    void AssignDeviceTypeToFD(int fd, drm_version const &Version) {
      if (Version.name) {
        if (strcmp(Version.name, "amdgpu") == 0) {
          FDToHandler[fd] = AMDGPU_Handler;
        }
        else if (strcmp(Version.name, "msm") == 0) {
          FDToHandler[fd] = MSM_Handler;
        }
        else {
          LogMan::Msg::E("Unknown DRM device: '%s'", Version.name);
        }
      }
    }

    uint32_t Handler(int fd, uint32_t cmd, uint32_t args) {
      switch (_IOC_NR(cmd)) {
        case _IOC_NR(FEX_DRM_IOCTL_VERSION): {
          fex_drm_version *version = reinterpret_cast<fex_drm_version*>(args);
          drm_version Host_Version = *version;
          uint64_t Result = ioctl(fd, DRM_IOCTL_VERSION, &Host_Version);
          *version = Host_Version;
          AssignDeviceTypeToFD(fd, Host_Version);
          return Result;
          break;
        }
        case _IOC_NR(FEX_DRM_IOCTL_GET_UNIQUE): {
          DRM::fex_drm_unique *val = reinterpret_cast<DRM::fex_drm_unique*>(args);
          drm_unique Host_val = *val;
          uint64_t Result = ioctl(fd, DRM_IOCTL_GET_UNIQUE, &Host_val);
          *val = Host_val;
          return Result;
          break;
        }
        // Passthrough
#define _BASIC_META(x) case _IOC_NR(x):
#define _BASIC_META_VAR(x, args...) case _IOC_NR(x):
#define _CUSTOM_META(name, ioctl_num)
#define _CUSTOM_META_OFFSET(name, ioctl_num, offset)
      // DRM
#include "Tests/LinuxSyscalls/x32/Ioctl/drm.inl"
        {
          uint64_t Result = ioctl(fd, cmd, args);
          return Result;
          break;
        }

        case DRM_COMMAND_BASE ... (DRM_COMMAND_END - 1): {
          // This is the space of the DRM device commands
          auto it = FDToHandler.find(fd);
          if (it == FDToHandler.end()) {
            drm_version Host_Version{};
            Host_Version.name = reinterpret_cast<char*>(alloca(128));
            Host_Version.name_len = 128;
            uint64_t Result = ioctl(fd, DRM_IOCTL_VERSION, &Host_Version);

            if (Result != -1) {
              AssignDeviceTypeToFD(fd, Host_Version);
            }

            it = FDToHandler.find(fd);

            if (it == FDToHandler.end()) {
              return -EPERM;
            }
          }
          return it->second(fd, cmd, args);
        break;
        }
        default:
          UnhandledIoctl("DRM", fd, cmd, args);
          return -EPERM;
          break;
      }
#undef _BASIC_META
#undef _BASIC_META_VAR
#undef _CUSTOM_META
#undef _CUSTOM_META_OFFSET

      return -EPERM;
    }
  }

  struct IoctlHandler {
    uint32_t Command;
    std::function<uint32_t(int fd, uint32_t cmd, uint32_t args)> Handler;
  };

  static std::unordered_map<uint32_t, std::function<uint32_t(int fd, uint32_t cmd, uint32_t args)>> Handlers;

  void InitializeStaticIoctlHandlers() {
    using namespace DRM;
    using namespace sockios;

    const std::vector<IoctlHandler> LocalHandlers = {{
#define _BASIC_META(x) IoctlHandler{_IOC_TYPE(x), ::ioctl},
#define _BASIC_META_VAR(x, args...) IoctlHandler{_IOC_TYPE(x(args)), ::ioctl},
#define _CUSTOM_META(name, ioctl_num) IoctlHandler{_IOC_TYPE(FEX_##name), ::ioctl},
#define _CUSTOM_META_OFFSET(name, ioctl_num, offset) IoctlHandler{_IOC_TYPE(FEX_##name), ::ioctl},

      // Asound
#include "Tests/LinuxSyscalls/x32/Ioctl/asound.inl"
      // Streams
#include "Tests/LinuxSyscalls/x32/Ioctl/streams.inl"
      // USB Dev
#include "Tests/LinuxSyscalls/x32/Ioctl/usbdev.inl"
      // Input
#include "Tests/LinuxSyscalls/x32/Ioctl/input.inl"
      // SOCKIOS
#include "Tests/LinuxSyscalls/x32/Ioctl/sockios.inl"
      // Joystick
#include "Tests/LinuxSyscalls/x32/Ioctl/joystick.inl"

#undef _BASIC_META
#undef _BASIC_META_VAR
#undef _CUSTOM_META
#undef _CUSTOM_META_OFFSET

#define _BASIC_META(x) IoctlHandler{_IOC_TYPE(x), FEX::HLE::x32::DRM::Handler},
#define _BASIC_META_VAR(x, args...) IoctlHandler{_IOC_TYPE(x(args)), FEX::HLE::x32::DRM::Handler},
#define _CUSTOM_META(name, ioctl_num) IoctlHandler{_IOC_TYPE(FEX_##name), FEX::HLE::x32::DRM::Handler},
#define _CUSTOM_META_OFFSET(name, ioctl_num, offset) IoctlHandler{_IOC_TYPE(FEX_##name), FEX::HLE::x32::DRM::Handler},
      // DRM
#include "Tests/LinuxSyscalls/x32/Ioctl/drm.inl"

#include "Tests/LinuxSyscalls/x32/Ioctl/amdgpu_drm.inl"
#include "Tests/LinuxSyscalls/x32/Ioctl/msm_drm.inl"
#include "Tests/LinuxSyscalls/x32/Ioctl/i915_drm.inl"
#include "Tests/LinuxSyscalls/x32/Ioctl/panfrost_drm.inl"
#include "Tests/LinuxSyscalls/x32/Ioctl/nouveau_drm.inl"

#undef _BASIC_META
#undef _BASIC_META_VAR
#undef _CUSTOM_META
#undef _CUSTOM_META_OFFSET
    }};

    for (auto &Arg : LocalHandlers) {
      Handlers[Arg.Command] = Arg.Handler;
    }
  }

  uint32_t ioctl32(FEXCore::Core::CpuStateFrame *Frame, int fd, uint32_t request, uint32_t args) {
    //return ioctl_32(fd, request, args);
    auto It = Handlers.find(_IOC_TYPE(request));
    if (It == Handlers.end()) {
      UnhandledIoctl("Base", fd, request, args);
      return -EPERM;
    }

    return It->second(fd, request, args);
  }

  void CheckAndAddFDDuplication(int fd, int NewFD) {
    DRM::CheckAndAddFDDuplication(fd, NewFD);
  }
}

