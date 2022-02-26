#pragma once

#include <FEXCore/Utils/CompilerDefs.h>

#include "Tests/LinuxSyscalls/x64/Types.h"
#include "Tests/LinuxSyscalls/x64/Ioctl/HelperDefines.h"

#include <cstdint>
#include <drm/drm.h>
#include <drm/drm_mode.h>
#include <drm/i915_drm.h>
#include <drm/amdgpu_drm.h>
#include <drm/lima_drm.h>
#include <drm/panfrost_drm.h>
#include <drm/msm_drm.h>
#include <drm/nouveau_drm.h>
#include <drm/vc4_drm.h>
#include <drm/v3d_drm.h>
#include <sys/ioctl.h>

#define CPYT(x) val.x = x
#define CPYF(x) x = val.x
namespace FEX::HLE::x64 {

#include "Tests/LinuxSyscalls/x64/Ioctl/drm.inl"
#include "Tests/LinuxSyscalls/x64/Ioctl/amdgpu_drm.inl"
#include "Tests/LinuxSyscalls/x64/Ioctl/msm_drm.inl"

}
#undef CPYT
#undef CPYF
