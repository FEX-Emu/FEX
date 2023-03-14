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
#include "Tests/LinuxSyscalls/x32/Ioctl/wireless.h"
#undef _BASIC_META
#undef _BASIC_META_VAR
#undef _CUSTOM_META
#undef _CUSTOM_META_OFFSET

#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/fextl/vector.h>

#include <cstdint>
#include <functional>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace FEX::HLE::x32 {
  static void UnhandledIoctl(const char *Type, int fd, uint32_t cmd, uint32_t args) {
    LogMan::Msg::EFmt("@@@@@@@@@@@@@@@@@@@@@@@@@");
    LogMan::Msg::EFmt("Unhandled {} ioctl({}, 0x{:08x}, 0x{:08x})", Type, fd, cmd, args);
    LogMan::Msg::EFmt("\tDir  : 0x{:x}", _IOC_DIR(cmd));
    LogMan::Msg::EFmt("\tType : 0x{:x}", _IOC_TYPE(cmd));
    LogMan::Msg::EFmt("\tNR   : 0x{:x}", _IOC_NR(cmd));
    LogMan::Msg::EFmt("\tSIZE : 0x{:x}", _IOC_SIZE(cmd));
    LogMan::Msg::AFmt("@@@@@@@@@@@@@@@@@@@@@@@@@");
  }

  namespace BasicHandler {
    uint64_t BasicHandler(int fd, uint32_t cmd, uint32_t args) {
      uint64_t Result = ::ioctl(fd, cmd, args);
      SYSCALL_ERRNO();
    }
  }

  namespace DRM {
    uint32_t AddAndRunHandler(int fd, uint32_t cmd, uint32_t args);
    void AssignDeviceTypeToFD(int fd, drm_version const &Version);

    template <size_t LRUSize>
    class LRUCacheFDCache {
    public:
      LRUCacheFDCache() {
        // Set the last element to our handler
        // This element will always be the last one
        LRUCache[LRUSize] = std::make_pair(0, AddAndRunHandler);
      }

      using HandlerType = uint32_t(*)(int fd, uint32_t cmd, uint32_t args);
      void SetFDHandler(uint32_t FD, HandlerType Handler) {
        FDToHandler[FD] = Handler;
      }

      void DuplicateFD(int fd, int NewFD) {
        auto it = FDToHandler.find(fd);
        if (it != FDToHandler.end()) {
          FDToHandler[NewFD] = it->second;
        }
      }

      HandlerType FindHandler(uint32_t FD) {
        HandlerType Handler{};
        for (size_t i = 0; i < LRUSize; ++i) {
          auto &it = LRUCache[i];
          if (it.first == FD) {
            if (i == 0) {
              // If we are the first in the queue then just return it
              return it.second;
            }
            Handler = it.second;
            break;
          }
        }

        if (Handler) {
          AddToFront(FD, Handler);
          return Handler;
        }
        return LRUCache[LRUSize].second;
      }

      uint32_t AddAndRunMapHandler(int fd, uint32_t cmd, uint32_t args) {
        // Couldn't find in cache, check map
        {
          auto it = FDToHandler.find(fd);
          if (it != FDToHandler.end()) {
            // Found, add to the cache
            AddToFront(fd, it->second);
            return it->second(fd, cmd, args);
          }
        }

        // Wasn't found in map, query it
        drm_version Host_Version{};
        Host_Version.name = reinterpret_cast<char*>(alloca(128));
        Host_Version.name_len = 128;
        uint64_t Result = ioctl(fd, DRM_IOCTL_VERSION, &Host_Version);

        // Add it to the map and double check that it was added
        // Next time around when the ioctl is used then it will be added to cache
        if (Result != -1) {
          AssignDeviceTypeToFD(fd, Host_Version);
        }

        auto it = FDToHandler.find(fd);

        if (it == FDToHandler.end()) {
          // We don't understand this DRM ioctl
          return -EPERM;
        }
        Result = it->second(fd, cmd, args);
        SYSCALL_ERRNO();
      }

    private:
      void AddToFront(uint32_t FD, HandlerType Handler) {
        // Push the element to the front if we found one
        // First copy all the other elements back one
        // Ensuring the final element isn't written over
        memmove(&LRUCache[1], &LRUCache[0], (LRUSize - 1) * sizeof(LRUCache[0]));
        // Now set the first element to the one we just found
        LRUCache[0] = std::make_pair(FD, Handler);
      }
      // With four elements total (3 + 1) then this is a single cacheline in size
      std::pair<uint32_t, HandlerType> LRUCache[LRUSize + 1];
      std::map<uint32_t, HandlerType> FDToHandler;
    };

    static LRUCacheFDCache<3> FDToHandler;

    uint32_t AddAndRunHandler(int fd, uint32_t cmd, uint32_t args) {
      return FDToHandler.AddAndRunMapHandler(fd, cmd, args);
    }

    void CheckAndAddFDDuplication(int fd, int NewFD) {
      FDToHandler.DuplicateFD(fd, NewFD);
    }

    uint32_t AMDGPU_Handler(int fd, uint32_t cmd, uint32_t args) {
      switch (_IOC_NR(cmd)) {
        case _IOC_NR(FEX_DRM_IOCTL_AMDGPU_GEM_METADATA): {
          AMDGPU::fex_drm_amdgpu_gem_metadata *val = reinterpret_cast<AMDGPU::fex_drm_amdgpu_gem_metadata*>(args);
          drm_amdgpu_gem_metadata Host_val = *val;
          uint64_t Result = ioctl(fd, DRM_IOCTL_AMDGPU_GEM_METADATA, &Host_val);
          if (Result != -1) {
            *val = Host_val;
          }
          SYSCALL_ERRNO();
          break;
        }
#define _BASIC_META(x) case _IOC_NR(x):
#define _BASIC_META_VAR(x, args...) case _IOC_NR(x):
#define _CUSTOM_META(name, ioctl_num)
#define _CUSTOM_META_OFFSET(name, ioctl_num, offset)
      // DRM
#include "Tests/LinuxSyscalls/x32/Ioctl/amdgpu_drm.inl"
        {
          uint64_t Result = ::ioctl(fd, cmd, args);
          SYSCALL_ERRNO();
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

    uint32_t RADEON_Handler(int fd, uint32_t cmd, uint32_t args) {
      switch (_IOC_NR(cmd)) {
        case _IOC_NR(FEX_DRM_IOCTL_RADEON_CP_INIT): {
          RADEON::fex_drm_radeon_init_t *val = reinterpret_cast<RADEON::fex_drm_radeon_init_t*>(args);
          drm_radeon_init_t Host_val = *val;
          uint64_t Result = ioctl(fd, DRM_IOCTL_RADEON_CP_INIT, &Host_val);
          if (Result != -1) {
            *val = Host_val;
          }
          SYSCALL_ERRNO();
          break;
        }
        case _IOC_NR(FEX_DRM_IOCTL_RADEON_CLEAR): {
          RADEON::fex_drm_radeon_clear_t *val = reinterpret_cast<RADEON::fex_drm_radeon_clear_t*>(args);
          drm_radeon_clear_t Host_val = *val;
          uint64_t Result = ioctl(fd, DRM_IOCTL_RADEON_CLEAR, &Host_val);
          if (Result != -1) {
            *val = Host_val;
          }
          SYSCALL_ERRNO();
          break;
        }
        case _IOC_NR(FEX_DRM_IOCTL_RADEON_STIPPLE): {
          RADEON::fex_drm_radeon_stipple_t *val = reinterpret_cast<RADEON::fex_drm_radeon_stipple_t*>(args);
          drm_radeon_stipple_t Host_val = *val;
          uint64_t Result = ioctl(fd, DRM_IOCTL_RADEON_STIPPLE, &Host_val);
          if (Result != -1) {
            *val = Host_val;
          }
          SYSCALL_ERRNO();
          break;
        }
        case _IOC_NR(FEX_DRM_IOCTL_RADEON_TEXTURE): {
          RADEON::fex_drm_radeon_texture_t *val = reinterpret_cast<RADEON::fex_drm_radeon_texture_t*>(args);
          drm_radeon_texture_t Host_val = *val;
          uint64_t Result = ioctl(fd, DRM_IOCTL_RADEON_TEXTURE, &Host_val);
          if (Result != -1) {
            *val = Host_val;
          }
          SYSCALL_ERRNO();
          break;
        }
        case _IOC_NR(FEX_DRM_IOCTL_RADEON_VERTEX2): {
          RADEON::fex_drm_radeon_vertex2_t *val = reinterpret_cast<RADEON::fex_drm_radeon_vertex2_t*>(args);
          drm_radeon_vertex2_t Host_val = *val;
          uint64_t Result = ioctl(fd, DRM_IOCTL_RADEON_VERTEX2, &Host_val);
          if (Result != -1) {
            *val = Host_val;
          }
          SYSCALL_ERRNO();
          break;
        }
        case _IOC_NR(FEX_DRM_IOCTL_RADEON_CMDBUF): {
          RADEON::fex_drm_radeon_cmd_buffer_t *val = reinterpret_cast<RADEON::fex_drm_radeon_cmd_buffer_t*>(args);
          drm_radeon_cmd_buffer_t Host_val = *val;
          uint64_t Result = ioctl(fd, DRM_IOCTL_RADEON_CMDBUF, &Host_val);
          if (Result != -1) {
            *val = Host_val;
          }
          SYSCALL_ERRNO();
          break;
        }
        case _IOC_NR(FEX_DRM_IOCTL_RADEON_GETPARAM): {
          RADEON::fex_drm_radeon_getparam_t *val = reinterpret_cast<RADEON::fex_drm_radeon_getparam_t*>(args);
          drm_radeon_getparam_t Host_val = *val;
          uint64_t Result = ioctl(fd, DRM_IOCTL_RADEON_GETPARAM, &Host_val);
          if (Result != -1) {
            *val = Host_val;
          }
          SYSCALL_ERRNO();
          break;
        }
        case _IOC_NR(FEX_DRM_IOCTL_RADEON_ALLOC): {
          RADEON::fex_drm_radeon_mem_alloc_t *val = reinterpret_cast<RADEON::fex_drm_radeon_mem_alloc_t*>(args);
          drm_radeon_mem_alloc_t Host_val = *val;
          uint64_t Result = ioctl(fd, DRM_IOCTL_RADEON_ALLOC, &Host_val);
          if (Result != -1) {
            *val = Host_val;
          }
          SYSCALL_ERRNO();
          break;
        }
        case _IOC_NR(FEX_DRM_IOCTL_RADEON_IRQ_EMIT): {
          RADEON::fex_drm_radeon_irq_emit_t *val = reinterpret_cast<RADEON::fex_drm_radeon_irq_emit_t*>(args);
          drm_radeon_irq_emit_t Host_val = *val;
          uint64_t Result = ioctl(fd, DRM_IOCTL_RADEON_IRQ_EMIT, &Host_val);
          if (Result != -1) {
            *val = Host_val;
          }
          SYSCALL_ERRNO();
          break;
        }
        case _IOC_NR(FEX_DRM_IOCTL_RADEON_SETPARAM): {
          RADEON::fex_drm_radeon_setparam_t *val = reinterpret_cast<RADEON::fex_drm_radeon_setparam_t*>(args);
          drm_radeon_setparam_t Host_val = *val;
          uint64_t Result = ioctl(fd, DRM_IOCTL_RADEON_SETPARAM, &Host_val);
          if (Result != -1) {
            *val = Host_val;
          }
          SYSCALL_ERRNO();
          break;
        }
        case _IOC_NR(FEX_DRM_IOCTL_RADEON_GEM_CREATE): {
          RADEON::fex_drm_radeon_gem_create *val = reinterpret_cast<RADEON::fex_drm_radeon_gem_create*>(args);
          drm_radeon_gem_create Host_val = *val;
          uint64_t Result = ioctl(fd, DRM_IOCTL_RADEON_GEM_CREATE, &Host_val);
          if (Result != -1) {
            *val = Host_val;
          }
          SYSCALL_ERRNO();
          break;
        }
#define _BASIC_META(x) case _IOC_NR(x):
#define _BASIC_META_VAR(x, args...) case _IOC_NR(x):
#define _CUSTOM_META(name, ioctl_num)
#define _CUSTOM_META_OFFSET(name, ioctl_num, offset)
      // DRM
#include "Tests/LinuxSyscalls/x32/Ioctl/radeon_drm.inl"
        {
          uint64_t Result = ::ioctl(fd, cmd, args);
          SYSCALL_ERRNO();
          break;
        }
        default:
          UnhandledIoctl("RADEON", fd, cmd, args);
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
          uint64_t Result = ::ioctl(fd, DRM_IOCTL_MSM_WAIT_FENCE, &Host_val);
          if (Result != -1) {
            *val = Host_val;
          }
          SYSCALL_ERRNO();
          break;
        }

#define _BASIC_META(x) case _IOC_NR(x):
#define _BASIC_META_VAR(x, args...) case _IOC_NR(x):
#define _CUSTOM_META(name, ioctl_num)
#define _CUSTOM_META_OFFSET(name, ioctl_num, offset)
      // DRM
#include "Tests/LinuxSyscalls/x32/Ioctl/msm_drm.inl"
        {
          uint64_t Result = ::ioctl(fd, cmd, args);
          SYSCALL_ERRNO();
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

    uint32_t Nouveau_Handler(int fd, uint32_t cmd, uint32_t args) {
      switch (_IOC_NR(cmd)) {
#define _BASIC_META(x) case _IOC_NR(x):
#define _BASIC_META_VAR(x, args...) case _IOC_NR(x):
#define _CUSTOM_META(name, ioctl_num)
#define _CUSTOM_META_OFFSET(name, ioctl_num, offset)
      // DRM
#include "Tests/LinuxSyscalls/x32/Ioctl/nouveau_drm.inl"
        {
          uint64_t Result = ::ioctl(fd, cmd, args);
          SYSCALL_ERRNO();
          break;
        }
        default:
          UnhandledIoctl("Nouveau", fd, cmd, args);
          return -EPERM;
          break;
      }
#undef _BASIC_META
#undef _BASIC_META_VAR
#undef _CUSTOM_META
#undef _CUSTOM_META_OFFSET
      return -EPERM;
    }

    uint32_t I915_Handler(int fd, uint32_t cmd, uint32_t args) {
#define SIMPLE(enum, type) case _IOC_NR(FEX_##enum): { \
          I915::fex_##type *guest = reinterpret_cast<I915::fex_##type*>(args); \
          type host = *guest; \
          uint64_t Result = ::ioctl(fd, enum, &host); \
          if (Result != -1) { \
            *guest = host; \
          } \
          SYSCALL_ERRNO(); \
          break; \
        }


      switch (_IOC_NR(cmd)) {
        SIMPLE(DRM_IOCTL_I915_BATCHBUFFER, drm_i915_batchbuffer_t)
        SIMPLE(DRM_IOCTL_I915_IRQ_EMIT, drm_i915_irq_emit_t)
        SIMPLE(DRM_IOCTL_I915_GETPARAM, drm_i915_getparam_t)
        SIMPLE(DRM_IOCTL_I915_ALLOC, drm_i915_mem_alloc_t)
        SIMPLE(DRM_IOCTL_I915_CMDBUFFER, drm_i915_cmdbuffer_t)

#define _BASIC_META(x) case _IOC_NR(x):
#define _BASIC_META_VAR(x, args...) case _IOC_NR(x):
#define _CUSTOM_META(name, ioctl_num)
#define _CUSTOM_META_OFFSET(name, ioctl_num, offset)
      // DRM
#include "Tests/LinuxSyscalls/x32/Ioctl/i915_drm.inl"
        {
          uint64_t Result = ::ioctl(fd, cmd, args);
          SYSCALL_ERRNO();
          break;
        }
        default:
          UnhandledIoctl("I915", fd, cmd, args);
          return -EPERM;
          break;
      }
#undef SIMPLE
#undef _BASIC_META
#undef _BASIC_META_VAR
#undef _CUSTOM_META
#undef _CUSTOM_META_OFFSET
      return -EPERM;
    }

    uint32_t Panfrost_Handler(int fd, uint32_t cmd, uint32_t args) {
      switch (_IOC_NR(cmd)) {
#define _BASIC_META(x) case _IOC_NR(x):
#define _BASIC_META_VAR(x, args...) case _IOC_NR(x):
#define _CUSTOM_META(name, ioctl_num)
#define _CUSTOM_META_OFFSET(name, ioctl_num, offset)
      // DRM
#include "Tests/LinuxSyscalls/x32/Ioctl/panfrost_drm.inl"
        {
          uint64_t Result = ::ioctl(fd, cmd, args);
          SYSCALL_ERRNO();
          break;
        }
        default:
          UnhandledIoctl("Panfrost", fd, cmd, args);
          return -EPERM;
          break;
      }
#undef _BASIC_META
#undef _BASIC_META_VAR
#undef _CUSTOM_META
#undef _CUSTOM_META_OFFSET
      return -EPERM;
    }

    uint32_t Lima_Handler(int fd, uint32_t cmd, uint32_t args) {
      switch (_IOC_NR(cmd)) {
#define _BASIC_META(x) case _IOC_NR(x):
#define _BASIC_META_VAR(x, args...) case _IOC_NR(x):
#define _CUSTOM_META(name, ioctl_num)
#define _CUSTOM_META_OFFSET(name, ioctl_num, offset)
      // DRM
#include "Tests/LinuxSyscalls/x32/Ioctl/lima_drm.inl"
        {
          uint64_t Result = ::ioctl(fd, cmd, args);
          SYSCALL_ERRNO();
          break;
        }
        default:
          UnhandledIoctl("Lima", fd, cmd, args);
          return -EPERM;
          break;
      }
#undef _BASIC_META
#undef _BASIC_META_VAR
#undef _CUSTOM_META
#undef _CUSTOM_META_OFFSET
      return -EPERM;
    }

    uint32_t VC4_Handler(int fd, uint32_t cmd, uint32_t args) {
      switch (_IOC_NR(cmd)) {
        case _IOC_NR(FEX_DRM_IOCTL_VC4_PERFMON_GET_VALUES): {
          FEX::HLE::x32::VC4::fex_drm_vc4_perfmon_get_values *val = reinterpret_cast<FEX::HLE::x32::VC4::fex_drm_vc4_perfmon_get_values*>(args);
          drm_vc4_perfmon_get_values Host_val = *val;
          uint64_t Result = ::ioctl(fd, DRM_IOCTL_VC4_PERFMON_GET_VALUES, &Host_val);
          if (Result != -1) {
            *val = Host_val;
          }
          SYSCALL_ERRNO();
          break;
        }

#define _BASIC_META(x) case _IOC_NR(x):
#define _BASIC_META_VAR(x, args...) case _IOC_NR(x):
#define _CUSTOM_META(name, ioctl_num)
#define _CUSTOM_META_OFFSET(name, ioctl_num, offset)
      // DRM
#include "Tests/LinuxSyscalls/x32/Ioctl/vc4_drm.inl"
        {
          uint64_t Result = ::ioctl(fd, cmd, args);
          SYSCALL_ERRNO();
          break;
        }
        default:
          UnhandledIoctl("VC4", fd, cmd, args);
          return -EPERM;
          break;
      }
#undef _BASIC_META
#undef _BASIC_META_VAR
#undef _CUSTOM_META
#undef _CUSTOM_META_OFFSET
      return -EPERM;
    }

    uint32_t V3D_Handler(int fd, uint32_t cmd, uint32_t args) {
      switch (_IOC_NR(cmd)) {
        case _IOC_NR(FEX_DRM_IOCTL_V3D_SUBMIT_CSD): {
          FEX::HLE::x32::V3D::fex_drm_v3d_submit_csd *val = reinterpret_cast<FEX::HLE::x32::V3D::fex_drm_v3d_submit_csd*>(args);
          drm_v3d_submit_csd Host_val = FEX::HLE::x32::V3D::fex_drm_v3d_submit_csd::SafeConvertToHost(val, _IOC_SIZE(cmd));
          uint64_t Result = ::ioctl(fd, DRM_IOCTL_V3D_SUBMIT_CSD, &Host_val);
          if (Result != -1) {
            FEX::HLE::x32::V3D::fex_drm_v3d_submit_csd::SafeConvertToGuest(val, Host_val, _IOC_SIZE(cmd));
          }
          SYSCALL_ERRNO();
          break;
        }

#define _BASIC_META(x) case _IOC_NR(x):
#define _BASIC_META_VAR(x, args...) case _IOC_NR(x):
#define _CUSTOM_META(name, ioctl_num)
#define _CUSTOM_META_OFFSET(name, ioctl_num, offset)
      // DRM
#include "Tests/LinuxSyscalls/x32/Ioctl/v3d_drm.inl"
        {
          uint64_t Result = ::ioctl(fd, cmd, args);
          SYSCALL_ERRNO();
          break;
        }
        default:
          UnhandledIoctl("V3D", fd, cmd, args);
          return -EPERM;
          break;
      }
#undef _BASIC_META
#undef _BASIC_META_VAR
#undef _CUSTOM_META
#undef _CUSTOM_META_OFFSET
      return -EPERM;
    }

    uint32_t Virtio_Handler(int fd, uint32_t cmd, uint32_t args) {
      switch (_IOC_NR(cmd)) {
#define _BASIC_META(x) case _IOC_NR(x):
#define _BASIC_META_VAR(x, args...) case _IOC_NR(x):
#define _CUSTOM_META(name, ioctl_num)
#define _CUSTOM_META_OFFSET(name, ioctl_num, offset)
      // DRM
#include "Tests/LinuxSyscalls/x32/Ioctl/virtio_drm.inl"
        {
          uint64_t Result = ::ioctl(fd, cmd, args);
          SYSCALL_ERRNO();
          break;
        }
        default:
          UnhandledIoctl("Virtio", fd, cmd, args);
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
          FDToHandler.SetFDHandler(fd, AMDGPU_Handler);
        }
        else if (strcmp(Version.name, "radeon") == 0) {
          FDToHandler.SetFDHandler(fd, RADEON_Handler);
        }
        else if (strcmp(Version.name, "msm") == 0) {
          FDToHandler.SetFDHandler(fd, MSM_Handler);
        }
        else if (strcmp(Version.name, "nouveau") == 0) {
          FDToHandler.SetFDHandler(fd, Nouveau_Handler);
        }
        else if (strcmp(Version.name, "i915") == 0) {
          FDToHandler.SetFDHandler(fd, I915_Handler);
        }
        else if (strcmp(Version.name, "panfrost") == 0) {
          FDToHandler.SetFDHandler(fd, Panfrost_Handler);
        }
        else if (strcmp(Version.name, "lima") == 0) {
          FDToHandler.SetFDHandler(fd, Lima_Handler);
        }
        else if (strcmp(Version.name, "vc4") == 0) {
          FDToHandler.SetFDHandler(fd, VC4_Handler);
        }
        else if (strcmp(Version.name, "v3d") == 0) {
          FDToHandler.SetFDHandler(fd, V3D_Handler);
        }
        else if (strcmp(Version.name, "virtio_gpu") == 0) {
          FDToHandler.SetFDHandler(fd, Virtio_Handler);
        }
        else {
          LogMan::Msg::EFmt("Unknown DRM device: '{}'", Version.name);
        }
      }
    }

    uint32_t Handler(int fd, uint32_t cmd, uint32_t args) {
#define SIMPLE(enum, type) case _IOC_NR(FEX_##enum): { \
          DRM::fex_##type *guest = reinterpret_cast<DRM::fex_##type*>(args); \
          type host = *guest; \
          uint64_t Result = ::ioctl(fd, enum, &host); \
          if (Result != -1) { \
            *guest = host; \
          } \
          SYSCALL_ERRNO(); \
          break; \
        }

      switch (_IOC_NR(cmd)) {
        case _IOC_NR(FEX_DRM_IOCTL_VERSION): {
          fex_drm_version *version = reinterpret_cast<fex_drm_version*>(args);
          drm_version Host_Version = *version;
          uint64_t Result = ::ioctl(fd, DRM_IOCTL_VERSION, &Host_Version);
          if (Result != -1) {
            *version = Host_Version;
            AssignDeviceTypeToFD(fd, Host_Version);
          }
          SYSCALL_ERRNO();
          break;
        }

        SIMPLE(DRM_IOCTL_GET_UNIQUE, drm_unique)
        SIMPLE(DRM_IOCTL_GET_CLIENT, drm_client)
        SIMPLE(DRM_IOCTL_GET_STATS, drm_stats)
        SIMPLE(DRM_IOCTL_SET_UNIQUE, drm_unique)

        SIMPLE(DRM_IOCTL_ADD_MAP, drm_map)
        SIMPLE(DRM_IOCTL_ADD_BUFS, drm_buf_desc)
        SIMPLE(DRM_IOCTL_MARK_BUFS, drm_buf_desc)
        SIMPLE(DRM_IOCTL_INFO_BUFS, drm_buf_info)
        SIMPLE(DRM_IOCTL_MAP_BUFS, drm_buf_map)
        SIMPLE(DRM_IOCTL_FREE_BUFS, drm_buf_free)
        SIMPLE(DRM_IOCTL_RM_MAP, drm_map)
        SIMPLE(DRM_IOCTL_SET_SAREA_CTX, drm_ctx_priv_map)
        SIMPLE(DRM_IOCTL_GET_SAREA_CTX, drm_ctx_priv_map)

        SIMPLE(DRM_IOCTL_RES_CTX,     drm_ctx_res)
        SIMPLE(DRM_IOCTL_DMA,         drm_dma)
        SIMPLE(DRM_IOCTL_SG_ALLOC,    drm_scatter_gather)
        SIMPLE(DRM_IOCTL_SG_FREE,     drm_scatter_gather)
        SIMPLE(DRM_IOCTL_UPDATE_DRAW, drm_update_draw)
        SIMPLE(DRM_IOCTL_MODE_GETPLANERESOURCES, drm_mode_get_plane_res)
        SIMPLE(DRM_IOCTL_MODE_ADDFB2,            drm_mode_fb_cmd2)
        SIMPLE(DRM_IOCTL_MODE_OBJ_GETPROPERTIES, drm_mode_obj_get_properties)
        SIMPLE(DRM_IOCTL_MODE_OBJ_SETPROPERTY,   drm_mode_obj_set_property)
        SIMPLE(DRM_IOCTL_MODE_GETFB2,            drm_mode_fb_cmd2)

        case _IOC_NR(FEX_DRM_IOCTL_WAIT_VBLANK): {
          fex_drm_wait_vblank *guest = reinterpret_cast<fex_drm_wait_vblank*>(args);
          drm_wait_vblank Host{};
          Host.request = guest->request;
          uint64_t Result = ::ioctl(fd, FEX_DRM_IOCTL_WAIT_VBLANK, &Host);
          if (Result != -1) {
            guest->reply = Host.reply;
          }
          SYSCALL_ERRNO();
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
          uint64_t Result = ::ioctl(fd, cmd, args);
          SYSCALL_ERRNO();
          break;
        }

        case DRM_COMMAND_BASE ... (DRM_COMMAND_END - 1): {
          // This is the space of the DRM device commands
          auto it = FDToHandler.FindHandler(fd);
          return it(fd, cmd, args);
        break;
        }
        default:
          UnhandledIoctl("DRM", fd, cmd, args);
          return -EPERM;
          break;
      }
#undef SIMPLE
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

  static fextl::vector<std::function<uint32_t(int fd, uint32_t cmd, uint32_t args)>> Handlers;

  void InitializeStaticIoctlHandlers() {
    using namespace DRM;
    using namespace sockios;

    const fextl::vector<IoctlHandler> LocalHandlers = {{
#define _BASIC_META(x) IoctlHandler{_IOC_TYPE(x), FEX::HLE::x32::BasicHandler::BasicHandler},
#define _BASIC_META_VAR(x, args...) IoctlHandler{_IOC_TYPE(x(args)), FEX::HLE::x32::BasicHandler::BasicHandler},
#define _CUSTOM_META(name, ioctl_num) IoctlHandler{_IOC_TYPE(FEX_##name), FEX::HLE::x32::BasicHandler::BasicHandler},
#define _CUSTOM_META_OFFSET(name, ioctl_num, offset) IoctlHandler{_IOC_TYPE(FEX_##name), FEX::HLE::x32::BasicHandler::BasicHandler},

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
      // Wireless
#include "Tests/LinuxSyscalls/x32/Ioctl/wireless.inl"

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
#include "Tests/LinuxSyscalls/x32/Ioctl/lima_drm.inl"
#include "Tests/LinuxSyscalls/x32/Ioctl/panfrost_drm.inl"
#include "Tests/LinuxSyscalls/x32/Ioctl/nouveau_drm.inl"
#include "Tests/LinuxSyscalls/x32/Ioctl/radeon_drm.inl"
#include "Tests/LinuxSyscalls/x32/Ioctl/vc4_drm.inl"
#include "Tests/LinuxSyscalls/x32/Ioctl/v3d_drm.inl"
#include "Tests/LinuxSyscalls/x32/Ioctl/virtio_drm.inl"

#undef _BASIC_META
#undef _BASIC_META_VAR
#undef _CUSTOM_META
#undef _CUSTOM_META_OFFSET
    }};

    Handlers.assign(1U << _IOC_TYPEBITS, FEX::HLE::x32::BasicHandler::BasicHandler);

    for (auto &Arg : LocalHandlers) {
      Handlers[Arg.Command] = Arg.Handler;
    }
  }

  uint32_t ioctl32(FEXCore::Core::CpuStateFrame *Frame, int fd, uint32_t request, uint32_t args) {
    return Handlers[_IOC_TYPE(request)](fd, request, args);
  }

  void CheckAndAddFDDuplication(int fd, int NewFD) {
    DRM::CheckAndAddFDDuplication(fd, NewFD);
  }
}

