#pragma once

#include "Tests/LinuxSyscalls/x32/Types.h"
#include "Tests/LinuxSyscalls/x32/Ioctl/HelperDefines.h"

#include <cstdint>
#include <drm/drm.h>
#include <drm/i915_drm.h>
#include <drm/amdgpu_drm.h>
#include <drm/panfrost_drm.h>
#include <drm/msm_drm.h>
#include <drm/nouveau_drm.h>
#include <sys/ioctl.h>

namespace FEX::HLE::x32 {

namespace DRM {
struct
__attribute__((annotate("alias-x86_32-drm_version")))
__attribute__((annotate("fex-match")))
fex_drm_version {
	int version_major;	  /**< Major version */
	int version_minor;	  /**< Minor version */
	int version_patchlevel;	  /**< Patch level */
	uint32_t name_len;	  /**< Length of name buffer */
	compat_ptr<char> name;	  /**< Name of driver */
	uint32_t date_len;	  /**< Length of date buffer */
	compat_ptr<char> date;	  /**< User-space buffer to hold date */
	uint32_t desc_len;	  /**< Length of desc buffer */
	compat_ptr<char> desc;	  /**< User-space buffer to hold desc */

  fex_drm_version() = delete;

  operator drm_version() const {
    drm_version val{};
    val.version_major = version_major;
    val.version_minor = version_minor;
    val.version_patchlevel = version_patchlevel;
    val.name_len = name_len;
    val.name = name;
    val.date_len = date_len;
    val.date = date;
    val.desc_len = desc_len;
    val.desc = desc;
    return val;
  }

  fex_drm_version(struct drm_version val)
    : name {val.name}
    , date {val.date}
    , desc {val.desc} {
    version_major = val.version_major;
    version_minor = val.version_minor;
    version_patchlevel = val.version_patchlevel;
    name_len = val.name_len;
    name = val.name;
    date_len = val.date_len;
    date = val.date;
    desc_len = val.desc_len;
    desc = val.desc;
  }
};

struct
__attribute__((annotate("alias-x86_32-drm_unique")))
__attribute__((annotate("fex-match")))
fex_drm_unique {
	compat_size_t unique_len;
	compat_ptr<char> unique;

  fex_drm_unique() = delete;

  operator drm_unique() const {
    drm_unique val{};
    val.unique_len = unique_len;
    val.unique = unique;
    return val;
  }

  fex_drm_unique(struct drm_unique val)
    : unique {val.unique} {
    unique_len = val.unique_len;
  }
};
}

namespace AMDGPU {
struct
__attribute__((annotate("alias-x86_32-drm_amdgpu_gem_metadata")))
__attribute__((annotate("fex-match")))
fex_drm_amdgpu_gem_metadata {
	__u32	handle;
	__u32	op;
	struct {
		compat_uint64_t	flags;
		compat_uint64_t	tiling_info;
		__u32	data_size_bytes;
		__u32	data[64];
	} data;

  fex_drm_amdgpu_gem_metadata() = delete;
  operator drm_amdgpu_gem_metadata() const {
    drm_amdgpu_gem_metadata val{};
    val.handle = handle;
    val.op = op;
    val.data.flags = data.flags;
    val.data.tiling_info = data.tiling_info;
    val.data.data_size_bytes = data.data_size_bytes;
    memcpy(val.data.data, data.data, sizeof(data.data));
    return val;
  }

  fex_drm_amdgpu_gem_metadata(struct drm_amdgpu_gem_metadata val) {
    handle = val.handle;
    op = val.op;
    data.flags = val.data.flags;
    data.tiling_info = val.data.tiling_info;
    data.data_size_bytes = val.data.data_size_bytes;
    memcpy(data.data, val.data.data, sizeof(data.data));
  }
};
}

namespace MSM {
struct
__attribute__((annotate("alias-x86_32-drm_msm_timespec")))
__attribute__((annotate("fex-match")))
fex_drm_msm_timespec {
	compat_int64_t tv_sec;
	compat_int64_t tv_nsec;

  fex_drm_msm_timespec() = delete;
  operator drm_msm_timespec() const {
    drm_msm_timespec val{};
    val.tv_sec = tv_sec;
    val.tv_nsec = tv_nsec;
    return val;
  }

  fex_drm_msm_timespec(struct drm_msm_timespec val) {
    tv_sec = val.tv_sec;
    tv_nsec = val.tv_nsec;
  }
};

struct
__attribute__((annotate("alias-x86_32-drm_msm_wait_fence")))
__attribute__((annotate("fex-match")))
__attribute__((packed))
fex_drm_msm_wait_fence {
	uint32_t fence;
	uint32_t pad;
	struct fex_drm_msm_timespec timeout;
	uint32_t queueid;

  fex_drm_msm_wait_fence() = delete;

  operator drm_msm_wait_fence() const {
    drm_msm_wait_fence val{};
    val.fence = fence;
    val.pad = pad;
    val.timeout = timeout;
    val.queueid = queueid;
    return val;
  }

  fex_drm_msm_wait_fence(struct drm_msm_wait_fence val)
    : timeout {val.timeout} {
    fence = val.fence;
    pad = val.pad;
    queueid = val.queueid;
  }
};

}

namespace I915 {

// I915 defines if they don't exist
// Older DRM doesn't have this
#ifndef DRM_IOCTL_I915_GEM_MMAP_OFFSET
struct drm_i915_gem_mmap_offset {
	uint32_t handle;
	uint32_t pad;
	compat_uint64_t offset;
	compat_uint64_t flags;
	compat_uint64_t extensions;
};

#define DRM_IOCTL_I915_GEM_MMAP_OFFSET  DRM_IOWR(DRM_COMMAND_BASE + DRM_I915_GEM_MMAP_GTT, FEX::HLE::x32::I915::drm_i915_gem_mmap_offset)
#endif
}

#include "Tests/LinuxSyscalls/x32/Ioctl/drm.inl"
#include "Tests/LinuxSyscalls/x32/Ioctl/amdgpu_drm.inl"
#include "Tests/LinuxSyscalls/x32/Ioctl/msm_drm.inl"
#include "Tests/LinuxSyscalls/x32/Ioctl/i915_drm.inl"
#include "Tests/LinuxSyscalls/x32/Ioctl/panfrost_drm.inl"
#include "Tests/LinuxSyscalls/x32/Ioctl/nouveau_drm.inl"

}

