// SPDX-License-Identifier: MIT
#pragma once

#include <FEXCore/Utils/CompilerDefs.h>

#include "LinuxSyscalls/x64/Types.h"
#include "LinuxSyscalls/x64/Ioctl/HelperDefines.h"

#include <cstdint>
extern "C" {
#include "fex-drm/drm.h"
#include "fex-drm/drm_mode.h"
#include "fex-drm/i915_drm.h"
#include "fex-drm/amdgpu_drm.h"
#include "fex-drm/lima_drm.h"
#include "fex-drm/panfrost_drm.h"
#include "fex-drm/msm_drm.h"
#include "fex-drm/nouveau_drm.h"
#include "fex-drm/vc4_drm.h"
#include "fex-drm/v3d_drm.h"
#include "fex-drm/virtgpu_drm.h"
}
#include <sys/ioctl.h>

#define CPYT(x) val.x = x
#define CPYF(x) x = val.x
namespace FEX::HLE::x64 {

#include "LinuxSyscalls/x64/Ioctl/drm.inl"
#include "LinuxSyscalls/x64/Ioctl/amdgpu_drm.inl"
#include "LinuxSyscalls/x64/Ioctl/msm_drm.inl"

} // namespace FEX::HLE::x64
#undef CPYT
#undef CPYF
