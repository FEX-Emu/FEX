// SPDX-License-Identifier: MIT
#pragma once

#include <FEXCore/Utils/CompilerDefs.h>

#include "LinuxSyscalls/x32/Types.h"
#include "LinuxSyscalls/x32/Ioctl/HelperDefines.h"

#include <cstdint>
extern "C" {
// drm headers use a `__user` define that has an address_space attribute. This allows their tooling to see unsafe user-space accesses.
// Define this to nothing so we don't need to modify those headers.
#define __user
#include "fex-drm/drm.h"
#include "fex-drm/drm_mode.h"
#include "fex-drm/i915_drm.h"
#include "fex-drm/amdgpu_drm.h"
#include "fex-drm/lima_drm.h"
#include "fex-drm/panfrost_drm.h"
#include "fex-drm/msm_drm.h"
#include "fex-drm/nouveau_drm.h"
#include "fex-drm/radeon_drm.h"
#include "fex-drm/vc4_drm.h"
#include "fex-drm/v3d_drm.h"
#include "fex-drm/panthor_drm.h"
#include "fex-drm/pvr_drm.h"
#include "fex-drm/virtgpu_drm.h"
#include "fex-drm/xe_drm.h"
}
#include <sys/ioctl.h>

#define CPYT(x) val.x = x
#define CPYF(x) x = val.x
namespace FEX::HLE::x32 {

namespace DRM {
  struct FEX_ANNOTATE("alias-x86_32-drm_version") FEX_ANNOTATE("fex-match") fex_drm_version {
    int version_major;      /**< Major version */
    int version_minor;      /**< Minor version */
    int version_patchlevel; /**< Patch level */
    uint32_t name_len;      /**< Length of name buffer */
    compat_ptr<char> name;  /**< Name of driver */
    uint32_t date_len;      /**< Length of date buffer */
    compat_ptr<char> date;  /**< User-space buffer to hold date */
    uint32_t desc_len;      /**< Length of desc buffer */
    compat_ptr<char> desc;  /**< User-space buffer to hold desc */

    fex_drm_version() = delete;

    operator drm_version() const {
      drm_version val {};
      CPYT(version_major);
      CPYT(version_minor);
      CPYT(version_patchlevel);
      CPYT(name_len);
      CPYT(name);
      CPYT(date_len);
      CPYT(date);
      CPYT(desc_len);
      CPYT(desc);
      return val;
    }

    fex_drm_version(struct drm_version val)
      : name {auto_compat_ptr {val.name}}
      , date {auto_compat_ptr {val.date}}
      , desc {auto_compat_ptr {val.desc}} {
      CPYF(version_major);
      CPYF(version_minor);
      CPYF(version_patchlevel);
      CPYF(name_len);
      CPYF(date_len);
      CPYF(desc_len);
    }
  };

  struct FEX_ANNOTATE("alias-x86_32-drm_unique") FEX_ANNOTATE("fex-match") fex_drm_unique {
    compat_size_t unique_len;
    compat_ptr<char> unique;

    fex_drm_unique() = delete;

    operator drm_unique() const {
      drm_unique val {};
      CPYT(unique_len);
      CPYT(unique);
      return val;
    }

    fex_drm_unique(struct drm_unique val)
      : unique {auto_compat_ptr {val.unique}} {
      CPYF(unique_len);
    }
  };

  struct FEX_ANNOTATE("alias-x86_32-drm_map") FEX_ANNOTATE("fex-match") fex_drm_map {
    uint32_t offset;
    uint32_t size;
    enum drm_map_type type;
    enum drm_map_flags flags;
    compat_ptr<void> handle;
    int32_t mtrr;

    fex_drm_map() = delete;

    operator drm_map() const {
      drm_map val {};
      CPYT(offset);
      CPYT(size);
      CPYT(type);
      CPYT(flags);
      CPYT(handle);
      CPYT(mtrr);
      return val;
    }

    fex_drm_map(struct drm_map val)
      : handle {auto_compat_ptr {val.handle}} {
      CPYF(offset);
      CPYF(size);
      CPYF(type);
      CPYF(flags);
      CPYF(mtrr);
    }
  };

  struct FEX_ANNOTATE("alias-x86_32-drm_client") FEX_ANNOTATE("fex-match") fex_drm_client {
    int32_t idx;
    int32_t auth;
    uint32_t pid;
    uint32_t uid;
    uint32_t magic;
    uint32_t iocs;

    fex_drm_client() = delete;

    operator drm_client() const {
      drm_client val {};
      CPYT(idx);
      CPYT(auth);
      CPYT(pid);
      CPYT(uid);
      CPYT(magic);
      CPYT(iocs);
      return val;
    }

    fex_drm_client(struct drm_client val) {
      CPYF(idx);
      CPYF(auth);
      CPYF(pid);
      CPYF(uid);
      CPYF(magic);
      CPYF(iocs);
    }
  };

  struct FEX_ANNOTATE("alias-x86_32-drm_stats") FEX_ANNOTATE("fex-match") fex_drm_stats {
    uint32_t count;
    struct {
      uint32_t value;
      enum drm_stat_type type;
    } data[15];

    fex_drm_stats() = delete;

    operator drm_stats() const {
      drm_stats val {};
      CPYT(count);
      for (size_t i = 0; i < 15; ++i) {
        CPYT(data[i].value);
        CPYT(data[i].type);
      }
      return val;
    }

    fex_drm_stats(struct drm_stats val) {
      CPYF(count);
      for (size_t i = 0; i < 15; ++i) {
        CPYF(data[i].value);
        CPYF(data[i].type);
      }
    }
  };

  struct FEX_ANNOTATE("alias-x86_32-drm_buf_desc") FEX_ANNOTATE("fex-match") fex_drm_buf_desc {
    int32_t count;
    int32_t size;
    int32_t low_mark;
    int32_t high_mark;
    enum { _DRM_PAGE_ALIGN = 0x01, _DRM_AGP_BUFFER = 0x02, _DRM_SG_BUFFER = 0x04, _DRM_FB_BUFFER = 0x08, _DRM_PCI_BUFFER_RO = 0x10 } flags;
    uint32_t agp_start;

    fex_drm_buf_desc() = delete;

    operator drm_buf_desc() const {
      drm_buf_desc val {};
      CPYT(count);
      CPYT(size);
      CPYT(low_mark);
      CPYT(high_mark);
      memcpy(&val.flags, &flags, sizeof(val.flags));
      CPYT(agp_start);
      return val;
    }

    fex_drm_buf_desc(struct drm_buf_desc val) {
      CPYF(count);
      CPYF(size);
      CPYF(low_mark);
      CPYF(high_mark);
      memcpy(&flags, &val.flags, sizeof(val.flags));
      CPYF(agp_start);
    }
  };

  struct FEX_ANNOTATE("alias-x86_32-drm_buf_info") FEX_ANNOTATE("fex-match") fex_drm_buf_info {
    int32_t count;
    compat_ptr<struct drm_buf_desc> list;

    fex_drm_buf_info() = delete;

    operator drm_buf_info() const {
      drm_buf_info val {};
      CPYT(count);
      CPYT(list);
      return val;
    }

    fex_drm_buf_info(struct drm_buf_info val)
      : list {auto_compat_ptr {val.list}} {
      CPYF(count);
    }
  };

  struct FEX_ANNOTATE("alias-x86_32-drm_buf_pub") FEX_ANNOTATE("fex-match") fex_drm_buf_pub {
    int32_t idx;
    int32_t total;
    int32_t used;
    compat_ptr<void> address;

    fex_drm_buf_pub() = delete;

    operator drm_buf_pub() const {
      drm_buf_pub val {};
      CPYT(idx);
      CPYT(total);
      CPYT(used);
      CPYT(address);
      return val;
    }

    fex_drm_buf_pub(struct drm_buf_pub val)
      : address {auto_compat_ptr {val.address}} {
      CPYF(idx);
      CPYF(total);
      CPYF(used);
    }
  };

  struct FEX_ANNOTATE("alias-x86_32-drm_buf_map") FEX_ANNOTATE("fex-match") fex_drm_buf_map {
    int32_t count;
#ifdef __cplusplus
    compat_ptr<void> virt;
#else
    compat_ptr<void> virtual;
#endif
    compat_ptr<drm_buf_pub> list;

    fex_drm_buf_map() = delete;

    operator drm_buf_map() const {
      drm_buf_map val {};
      CPYT(count);
#ifdef __cplusplus
      CPYT(virt);
#else
      CPYT(virtual);
#endif
      CPYT(list);
      return val;
    }

    fex_drm_buf_map(struct drm_buf_map val)
#ifdef __cplusplus
      : virt {auto_compat_ptr {val.virt}}
#else
      : virtual {auto_compat_ptr {val.virtual}}
#endif
      , list {auto_compat_ptr {val.list}} {
      CPYF(count);
    }
  };

  struct FEX_ANNOTATE("alias-x86_32-drm_buf_free") FEX_ANNOTATE("fex-match") fex_drm_buf_free {
    int32_t count;
    compat_ptr<int> list;

    fex_drm_buf_free() = delete;

    operator drm_buf_free() const {
      drm_buf_free val {};
      CPYT(count);
      CPYT(list);
      return val;
    }

    fex_drm_buf_free(struct drm_buf_free val)
      : list {auto_compat_ptr {val.list}} {
      CPYF(count);
    }
  };

  struct FEX_ANNOTATE("alias-x86_32-drm_ctx_priv_map") FEX_ANNOTATE("fex-match") fex_drm_ctx_priv_map {
    uint32_t ctx_id;
    compat_ptr<void> handle;

    fex_drm_ctx_priv_map() = delete;

    operator drm_ctx_priv_map() const {
      drm_ctx_priv_map val {};
      CPYT(ctx_id);
      CPYT(handle);
      return val;
    }

    fex_drm_ctx_priv_map(struct drm_ctx_priv_map val)
      : handle {auto_compat_ptr {val.handle}} {
      CPYF(ctx_id);
    }
  };

  struct FEX_ANNOTATE("alias-x86_32-drm_ctx_res") FEX_ANNOTATE("fex-match") fex_drm_ctx_res {
    int32_t count;
    compat_ptr<struct drm_ctx> contexts;

    fex_drm_ctx_res() = delete;

    operator drm_ctx_res() const {
      drm_ctx_res val {};
      CPYT(count);
      return val;
    }

    fex_drm_ctx_res(struct drm_ctx_res val)
      : contexts {auto_compat_ptr {val.contexts}} {
      CPYF(count);
    }
  };

  struct FEX_ANNOTATE("alias-x86_32-drm_dma") FEX_ANNOTATE("fex-match") fex_drm_dma {
    int32_t context;
    int32_t send_count;
    compat_ptr<int32_t> send_indices;
    compat_ptr<int32_t> send_sizes;
    enum drm_dma_flags flags;
    int32_t request_count;
    int32_t request_size;
    compat_ptr<int32_t> request_indices;
    compat_ptr<int32_t> request_sizes;
    int32_t granted_count;

    fex_drm_dma() = delete;

    operator drm_dma() const {
      drm_dma val {};
      CPYT(context);
      CPYT(send_count);
      CPYT(send_indices);
      CPYT(send_sizes);
      CPYT(flags);
      CPYT(request_count);
      CPYT(request_size);
      CPYT(request_indices);
      CPYT(request_sizes);
      CPYT(granted_count);
      return val;
    }

    fex_drm_dma(struct drm_dma val)
      : send_indices {auto_compat_ptr {val.send_indices}}
      , send_sizes {auto_compat_ptr {val.send_sizes}}
      , request_indices {auto_compat_ptr {val.request_indices}}
      , request_sizes {auto_compat_ptr {val.request_sizes}} {
      CPYF(context);
      CPYF(send_count);
      CPYF(flags);
      CPYF(request_count);
      CPYF(request_size);
      CPYF(granted_count);
    }
  };

  struct FEX_ANNOTATE("alias-x86_32-drm_scatter_gather") FEX_ANNOTATE("fex-match") fex_drm_scatter_gather {
    uint32_t size;
    uint32_t handle;

    fex_drm_scatter_gather() = delete;

    operator drm_scatter_gather() const {
      drm_scatter_gather val {};
      CPYT(size);
      CPYT(handle);
      return val;
    }

    fex_drm_scatter_gather(struct drm_scatter_gather val) {
      CPYF(size);
      CPYF(handle);
    }
  };

  struct FEX_ANNOTATE("alias-x86_32-drm_wait_vblank_request") FEX_ANNOTATE("fex-match") fex_drm_wait_vblank_request {
    enum drm_vblank_seq_type type;
    uint32_t sequence;
    uint32_t signal;

    fex_drm_wait_vblank_request() = delete;

    operator drm_wait_vblank_request() const {
      drm_wait_vblank_request val {};
      CPYT(type);
      CPYT(sequence);
      CPYT(signal);
      return val;
    }

    fex_drm_wait_vblank_request(struct drm_wait_vblank_request val) {
      CPYF(type);
      CPYF(sequence);
      CPYF(signal);
    }
  };

  struct FEX_ANNOTATE("alias-x86_32-drm_wait_vblank_reply") FEX_ANNOTATE("fex-match") fex_drm_wait_vblank_reply {
    enum drm_vblank_seq_type type;
    uint32_t sequence;
    int32_t tval_sec;
    int32_t tval_usec;

    fex_drm_wait_vblank_reply() = delete;

    operator drm_wait_vblank_reply() const {
      drm_wait_vblank_reply val {};
      CPYT(type);
      CPYT(sequence);
      CPYT(tval_sec);
      CPYT(tval_usec);
      return val;
    }

    fex_drm_wait_vblank_reply(struct drm_wait_vblank_reply val) {
      CPYF(type);
      CPYF(sequence);
      CPYF(tval_sec);
      CPYF(tval_usec);
    }
  };

  union FEX_ANNOTATE("alias-x86_32-drm_wait_vblank") FEX_ANNOTATE("fex-match") fex_drm_wait_vblank {
    fex_drm_wait_vblank_request request;
    fex_drm_wait_vblank_reply reply;

    fex_drm_wait_vblank() = delete;
  };

  struct FEX_ANNOTATE("alias-x86_32-drm_update_draw") FEX_ANNOTATE("fex-match") FEX_PACKED fex_drm_update_draw {
    drm_drawable_t handle;
    uint32_t type;
    uint32_t num;
    compat_uint64_t data;

    fex_drm_update_draw() = delete;

    operator drm_update_draw() const {
      drm_update_draw val {};
      CPYT(handle);
      CPYT(type);
      CPYT(num);
      CPYT(data);
      return val;
    }

    fex_drm_update_draw(struct drm_update_draw val) {
      CPYF(handle);
      CPYF(type);
      CPYF(num);
      CPYF(data);
    }
  };

  struct FEX_ANNOTATE("alias-x86_32-drm_mode_get_plane_res") FEX_ANNOTATE("fex-match") FEX_PACKED fex_drm_mode_get_plane_res {
    compat_uint64_t plane_id_ptr;
    uint32_t count_planes;
    fex_drm_mode_get_plane_res() = delete;

    operator drm_mode_get_plane_res() const {
      drm_mode_get_plane_res val {};
      CPYT(plane_id_ptr);
      CPYT(count_planes);
      return val;
    }

    fex_drm_mode_get_plane_res(struct drm_mode_get_plane_res val) {
      CPYF(plane_id_ptr);
      CPYF(count_planes);
    }
  };

  struct FEX_ANNOTATE("alias-x86_32-drm_mode_fb_cmd2") FEX_ANNOTATE("fex-match") FEX_PACKED fex_drm_mode_fb_cmd2 {
    uint32_t fb_id;
    uint32_t width;
    uint32_t height;
    uint32_t pixel_format;
    uint32_t flags;

    uint32_t handles[4];
    uint32_t pitches[4];
    uint32_t offsets[4];
    compat_uint64_t modifier[4];
    fex_drm_mode_fb_cmd2() = delete;

    operator drm_mode_fb_cmd2() const {
      drm_mode_fb_cmd2 val {};
      CPYT(fb_id);
      CPYT(width);
      CPYT(height);
      CPYT(pixel_format);
      CPYT(flags);
      for (int i = 0; i < 4; ++i) {
        CPYT(handles[i]);
        CPYT(pitches[i]);
        CPYT(offsets[i]);
        CPYT(modifier[i]);
      }
      return val;
    }

    fex_drm_mode_fb_cmd2(struct drm_mode_fb_cmd2 val) {
      CPYF(fb_id);
      CPYF(width);
      CPYF(height);
      CPYF(pixel_format);
      CPYF(flags);
      for (int i = 0; i < 4; ++i) {
        CPYF(handles[i]);
        CPYF(pitches[i]);
        CPYF(offsets[i]);
        CPYF(modifier[i]);
      }
    }
  };

  struct FEX_ANNOTATE("alias-x86_32-drm_mode_obj_get_properties") FEX_ANNOTATE("fex-match") FEX_PACKED fex_drm_mode_obj_get_properties {
    compat_uint64_t props_ptr;
    compat_uint64_t prop_values_ptr;
    uint32_t count_props;
    uint32_t obj_id;
    uint32_t obj_type;

    fex_drm_mode_obj_get_properties() = delete;

    operator drm_mode_obj_get_properties() const {
      drm_mode_obj_get_properties val {};
      CPYT(props_ptr);
      CPYT(prop_values_ptr);
      CPYT(count_props);
      CPYT(obj_id);
      CPYT(obj_type);
      return val;
    }

    fex_drm_mode_obj_get_properties(struct drm_mode_obj_get_properties val) {
      CPYF(props_ptr);
      CPYF(prop_values_ptr);
      CPYF(count_props);
      CPYF(obj_id);
      CPYF(obj_type);
    }
  };

  struct FEX_ANNOTATE("alias-x86_32-drm_mode_obj_set_property") FEX_ANNOTATE("fex-match") FEX_PACKED fex_drm_mode_obj_set_property {
    compat_uint64_t value;
    uint32_t prop_id;
    uint32_t obj_id;
    uint32_t obj_type;

    fex_drm_mode_obj_set_property() = delete;

    operator drm_mode_obj_set_property() const {
      drm_mode_obj_set_property val {};
      CPYT(value);
      CPYT(prop_id);
      CPYT(obj_id);
      CPYT(obj_type);
      return val;
    }

    fex_drm_mode_obj_set_property(struct drm_mode_obj_set_property val) {
      CPYF(value);
      CPYF(prop_id);
      CPYF(obj_id);
      CPYF(obj_type);
    }
  };

} // namespace DRM

namespace AMDGPU {
  struct FEX_ANNOTATE("alias-x86_32-drm_amdgpu_gem_metadata") FEX_ANNOTATE("fex-match") fex_drm_amdgpu_gem_metadata {
    __u32 handle;
    __u32 op;
    struct {
      compat_uint64_t flags;
      compat_uint64_t tiling_info;
      __u32 data_size_bytes;
      __u32 data[64];
    } data;

    fex_drm_amdgpu_gem_metadata() = delete;
    operator drm_amdgpu_gem_metadata() const {
      drm_amdgpu_gem_metadata val {};
      CPYT(handle);
      CPYT(op);
      CPYT(data.flags);
      CPYT(data.tiling_info);
      CPYT(data.data_size_bytes);
      memcpy(val.data.data, data.data, sizeof(data.data));
      return val;
    }

    fex_drm_amdgpu_gem_metadata(struct drm_amdgpu_gem_metadata val) {
      CPYF(handle);
      CPYF(op);
      CPYF(data.flags);
      CPYF(data.tiling_info);
      CPYF(data.data_size_bytes);
      memcpy(data.data, val.data.data, sizeof(data.data));
    }
  };
} // namespace AMDGPU

namespace RADEON {
  struct FEX_ANNOTATE("alias-x86_32-drm_radeon_gem_create") FEX_ANNOTATE("fex-match") fex_drm_radeon_gem_create {
    compat_uint64_t size;
    compat_uint64_t alignment;
    __u32 handle;
    __u32 initial_domain;
    __u32 flags;

    fex_drm_radeon_gem_create() = delete;

    operator drm_radeon_gem_create() const {
      drm_radeon_gem_create val {};
      CPYT(size);
      CPYT(alignment);
      CPYT(handle);
      CPYT(initial_domain);
      CPYT(flags);
      return val;
    }

    fex_drm_radeon_gem_create(struct drm_radeon_gem_create val) {
      CPYF(size);
      CPYF(alignment);
      CPYF(handle);
      CPYF(initial_domain);
      CPYF(flags);
    }
  };

  struct FEX_PACKED FEX_ANNOTATE("alias-x86_32-drm_radeon_init") FEX_ANNOTATE("fex-match") fex_drm_radeon_init_t {
    uint32_t func;

    compat_ulong_t sarea_priv_offset;
    int32_t is_pci;
    int32_t cp_mode;
    int32_t gart_size;
    int32_t ring_size;
    int32_t usec_timeout;

    uint32_t fb_bpp;
    uint32_t front_offset, front_pitch;
    uint32_t back_offset, back_pitch;
    uint32_t depth_bpp;
    uint32_t depth_offset, depth_pitch;

    compat_ulong_t fb_offset;
    compat_ulong_t mmio_offset;
    compat_ulong_t ring_offset;
    compat_ulong_t ring_rptr_offset;
    compat_ulong_t buffers_offset;
    compat_ulong_t gart_textures_offset;

    fex_drm_radeon_init_t() = delete;

    operator drm_radeon_init_t() const {
      drm_radeon_init_t val {};
      memcpy(&val.func, &func, sizeof(val.func));
      CPYT(sarea_priv_offset);
      CPYT(is_pci);
      CPYT(cp_mode);
      CPYT(gart_size);
      CPYT(ring_size);
      CPYT(usec_timeout);
      CPYT(fb_bpp);
      CPYT(front_offset);
      CPYT(front_pitch);
      CPYT(back_offset);
      CPYT(back_pitch);
      CPYT(depth_bpp);
      CPYT(depth_offset);
      CPYT(depth_pitch);
      CPYT(fb_offset);
      CPYT(mmio_offset);
      CPYT(ring_offset);
      CPYT(ring_rptr_offset);
      CPYT(buffers_offset);
      CPYT(gart_textures_offset);
      return val;
    }

    fex_drm_radeon_init_t(drm_radeon_init_t val) {
      memcpy(&func, &val.func, sizeof(val.func));
      CPYF(sarea_priv_offset);
      CPYF(is_pci);
      CPYF(cp_mode);
      CPYF(gart_size);
      CPYF(ring_size);
      CPYF(usec_timeout);
      CPYF(fb_bpp);
      CPYF(front_offset);
      CPYF(front_pitch);
      CPYF(back_offset);
      CPYF(back_pitch);
      CPYF(depth_bpp);
      CPYF(depth_offset);
      CPYF(depth_pitch);
      CPYF(fb_offset);
      CPYF(mmio_offset);
      CPYF(ring_offset);
      CPYF(ring_rptr_offset);
      CPYF(buffers_offset);
      CPYF(gart_textures_offset);
    }
  };

  struct FEX_ANNOTATE("alias-x86_32-drm_radeon_clear") FEX_ANNOTATE("fex-match") fex_drm_radeon_clear_t {
    uint32_t flags;
    uint32_t clear_color;
    uint32_t clear_depth;
    uint32_t color_mask;
    uint32_t depth_mask;
    compat_ptr<drm_radeon_clear_rect_t> depth_boxes;

    fex_drm_radeon_clear_t() = delete;

    operator drm_radeon_clear_t() const {
      drm_radeon_clear_t val {};
      CPYT(flags);
      CPYT(clear_color);
      CPYT(clear_depth);
      CPYT(color_mask);
      CPYT(depth_mask);
      CPYT(depth_boxes);
      return val;
    }

    fex_drm_radeon_clear_t(drm_radeon_clear_t val)
      : depth_boxes {auto_compat_ptr {val.depth_boxes}} {
      CPYF(flags);
      CPYF(clear_color);
      CPYF(clear_depth);
      CPYF(color_mask);
      CPYF(depth_mask);
    }
  };

  struct FEX_ANNOTATE("alias-x86_32-drm_radeon_stipple") FEX_ANNOTATE("fex-match") fex_drm_radeon_stipple_t {
    compat_ptr<uint32_t> mask;

    fex_drm_radeon_stipple_t() = delete;

    operator drm_radeon_stipple_t() const {
      drm_radeon_stipple_t val {};
      CPYT(mask);
      return val;
    }

    fex_drm_radeon_stipple_t(drm_radeon_stipple_t val)
      : mask {auto_compat_ptr {val.mask}} {}
  };

  struct FEX_ANNOTATE("alias-x86_32-drm_radeon_texture") FEX_ANNOTATE("fex-match") fex_drm_radeon_texture_t {
    uint32_t offset;
    int32_t pitch;
    int32_t format;
    int32_t width;
    int32_t height;
    compat_ptr<drm_radeon_tex_image_t> image;

    fex_drm_radeon_texture_t() = delete;

    operator drm_radeon_texture_t() const {
      drm_radeon_texture_t val {};
      CPYT(offset);
      CPYT(pitch);
      CPYT(format);
      CPYT(width);
      CPYT(height);
      CPYT(image);
      return val;
    }

    fex_drm_radeon_texture_t(drm_radeon_texture_t val)
      : image {auto_compat_ptr {val.image}} {
      CPYF(offset);
      CPYF(pitch);
      CPYF(format);
      CPYF(width);
      CPYF(height);
    }
  };

  struct FEX_ANNOTATE("alias-x86_32-drm_radeon_vertex2") FEX_ANNOTATE("fex-match") fex_drm_radeon_vertex2_t {
    int32_t idx;
    int32_t discard;
    int32_t nr_states;
    compat_ptr<drm_radeon_state_t> state;
    int32_t nr_prims;
    compat_ptr<drm_radeon_prim_t> prim;

    fex_drm_radeon_vertex2_t() = delete;

    operator drm_radeon_vertex2_t() const {
      drm_radeon_vertex2_t val;
      CPYT(idx);
      CPYT(discard);
      CPYT(nr_states);
      CPYT(state);
      CPYT(nr_prims);
      CPYT(prim);
      return val;
    }

    fex_drm_radeon_vertex2_t(drm_radeon_vertex2_t val)
      : state {auto_compat_ptr {val.state}}
      , prim {auto_compat_ptr {val.prim}} {
      CPYF(idx);
      CPYF(discard);
      CPYF(nr_states);
      CPYF(nr_prims);
    }
  };

  struct FEX_ANNOTATE("alias-x86_32-drm_radeon_cmd_buffer") FEX_ANNOTATE("fex-match") fex_drm_radeon_cmd_buffer_t {
    int32_t bufsz;
    compat_ptr<char> buf;
    int32_t nbox;
    compat_ptr<drm_clip_rect> boxes;

    fex_drm_radeon_cmd_buffer_t() = delete;

    operator drm_radeon_cmd_buffer_t() const {
      drm_radeon_cmd_buffer_t val;
      CPYT(bufsz);
      CPYT(buf);
      CPYT(nbox);
      CPYT(boxes);
      return val;
    }

    fex_drm_radeon_cmd_buffer_t(drm_radeon_cmd_buffer_t val)
      : buf {auto_compat_ptr {val.buf}}
      , boxes {auto_compat_ptr {val.boxes}} {
      CPYF(bufsz);
      CPYF(nbox);
    }
  };

  struct FEX_ANNOTATE("alias-x86_32-drm_radeon_getparam") FEX_ANNOTATE("fex-match") fex_drm_radeon_getparam_t {
    int32_t param;
    compat_ptr<void> value;

    fex_drm_radeon_getparam_t() = delete;

    operator drm_radeon_getparam_t() const {
      drm_radeon_getparam_t val;
      CPYT(param);
      CPYT(value);
      return val;
    }

    fex_drm_radeon_getparam_t(drm_radeon_getparam_t val)
      : value {auto_compat_ptr {val.value}} {
      CPYF(param);
    }
  };

  struct FEX_ANNOTATE("alias-x86_32-drm_radeon_mem_alloc") FEX_ANNOTATE("fex-match") fex_drm_radeon_mem_alloc_t {
    int32_t region;
    int32_t alignment;
    int32_t size;
    compat_ptr<int32_t> region_offset;

    fex_drm_radeon_mem_alloc_t() = delete;

    operator drm_radeon_mem_alloc_t() const {
      drm_radeon_mem_alloc_t val;
      CPYT(region);
      CPYT(alignment);
      CPYT(size);
      CPYT(region_offset);
      return val;
    }

    fex_drm_radeon_mem_alloc_t(drm_radeon_mem_alloc_t val)
      : region_offset {auto_compat_ptr {val.region_offset}} {
      CPYF(region);
      CPYF(alignment);
      CPYF(size);
    }
  };

  struct FEX_ANNOTATE("alias-x86_32-drm_radeon_irq_emit") FEX_ANNOTATE("fex-match") fex_drm_radeon_irq_emit_t {
    compat_ptr<int32_t> irq_seq;

    fex_drm_radeon_irq_emit_t() = delete;

    operator drm_radeon_irq_emit_t() const {
      drm_radeon_irq_emit_t val;
      CPYT(irq_seq);
      return val;
    }

    fex_drm_radeon_irq_emit_t(drm_radeon_irq_emit_t val)
      : irq_seq {auto_compat_ptr {val.irq_seq}} {}
  };

  struct FEX_ANNOTATE("alias-x86_32-drm_radeon_setparam") FEX_ANNOTATE("fex-match") FEX_PACKED fex_drm_radeon_setparam_t {
    uint32_t param;
    compat_int64_t value;

    fex_drm_radeon_setparam_t() = delete;

    operator drm_radeon_setparam_t() const {
      drm_radeon_setparam_t val;
      CPYT(param);
      CPYT(value);
      return val;
    }

    fex_drm_radeon_setparam_t(drm_radeon_setparam_t val) {
      CPYF(param);
      CPYF(value);
    }
  };

} // namespace RADEON

namespace MSM {
  struct FEX_ANNOTATE("alias-x86_32-drm_msm_timespec") FEX_ANNOTATE("fex-match") fex_drm_msm_timespec {
    compat_int64_t tv_sec;
    compat_int64_t tv_nsec;

    operator drm_msm_timespec() const {
      drm_msm_timespec val {};
      CPYT(tv_sec);
      CPYT(tv_nsec);
      return val;
    }

    static fex_drm_msm_timespec FromHost(struct drm_msm_timespec val) {
      fex_drm_msm_timespec ret;
      ret.tv_sec = val.tv_sec;
      ret.tv_nsec = val.tv_nsec;
      return ret;
    }

  private:
    fex_drm_msm_timespec() = default;
  };

  struct FEX_ANNOTATE("alias-x86_32-drm_msm_wait_fence") FEX_ANNOTATE("fex-match") FEX_PACKED fex_drm_msm_wait_fence {
    uint32_t fence;
    uint32_t flags;
    struct fex_drm_msm_timespec timeout;
    uint32_t queueid;

    fex_drm_msm_wait_fence() = delete;

    operator drm_msm_wait_fence() const {
      drm_msm_wait_fence val {};
      CPYT(fence);
      CPYT(flags);
      CPYT(timeout);
      CPYT(queueid);
      return val;
    }

    fex_drm_msm_wait_fence(struct drm_msm_wait_fence val)
      : timeout {fex_drm_msm_timespec::FromHost(val.timeout)} {
      CPYF(fence);
      CPYF(flags);
      CPYF(queueid);
    }
  };

} // namespace MSM

namespace I915 {

  struct FEX_ANNOTATE("alias-x86_32-drm_i915_batchbuffer") FEX_ANNOTATE("fex-match") fex_drm_i915_batchbuffer_t {
    int32_t start;
    int32_t used;
    int32_t DR1;
    int32_t DR4;
    int32_t num_cliprects;
    compat_ptr<struct drm_clip_rect> cliprects;

    fex_drm_i915_batchbuffer_t() = delete;

    operator drm_i915_batchbuffer_t() const {
      drm_i915_batchbuffer_t val {};
      CPYT(start);
      CPYT(used);
      CPYT(DR1);
      CPYT(DR4);
      CPYT(num_cliprects);
      CPYT(cliprects);
      return val;
    }

    fex_drm_i915_batchbuffer_t(drm_i915_batchbuffer_t val)
      : cliprects {auto_compat_ptr {val.cliprects}} {
      CPYF(start);
      CPYF(used);
      CPYF(DR1);
      CPYF(DR4);
      CPYF(num_cliprects);
    }
  };

  struct FEX_ANNOTATE("alias-x86_32-drm_i915_irq_emit") FEX_ANNOTATE("fex-match") fex_drm_i915_irq_emit_t {
    compat_ptr<int> irq_seq;

    fex_drm_i915_irq_emit_t() = delete;

    operator drm_i915_irq_emit_t() const {
      drm_i915_irq_emit_t val {};
      CPYT(irq_seq);
      return val;
    }

    fex_drm_i915_irq_emit_t(drm_i915_irq_emit_t val)
      : irq_seq {auto_compat_ptr {val.irq_seq}} {}
  };

  struct FEX_ANNOTATE("alias-x86_32-drm_i915_getparam") FEX_ANNOTATE("fex-match") fex_drm_i915_getparam_t {
    int32_t param;
    compat_ptr<int> value;
    fex_drm_i915_getparam_t() = delete;

    operator drm_i915_getparam_t() const {
      drm_i915_getparam_t val {};
      CPYT(param);
      CPYT(value);
      return val;
    }

    fex_drm_i915_getparam_t(drm_i915_getparam_t val)
      : value {auto_compat_ptr {val.value}} {
      CPYF(param);
    }
  };

  struct FEX_ANNOTATE("alias-x86_32-drm_i915_mem_alloc") FEX_ANNOTATE("fex-match") fex_drm_i915_mem_alloc_t {
    int32_t region;
    int32_t alignment;
    int32_t size;
    compat_ptr<int> region_offset;
    fex_drm_i915_mem_alloc_t() = delete;

    operator drm_i915_mem_alloc_t() const {
      drm_i915_mem_alloc_t val {};
      CPYT(region);
      CPYT(alignment);
      CPYT(size);
      CPYT(region_offset);
      return val;
    }

    fex_drm_i915_mem_alloc_t(drm_i915_mem_alloc_t val)
      : region_offset {auto_compat_ptr {val.region_offset}} {
      CPYF(region);
      CPYF(alignment);
      CPYF(size);
    }
  };

  struct FEX_ANNOTATE("alias-x86_32-_drm_i915_cmdbuffer") FEX_ANNOTATE("fex-match") fex_drm_i915_cmdbuffer_t {
    compat_ptr<char> buf;
    int32_t sz;
    int32_t DR1;
    int32_t DR4;
    int32_t num_cliprects;
    compat_ptr<struct drm_clip_rect> cliprects;

    fex_drm_i915_cmdbuffer_t() = delete;

    operator drm_i915_cmdbuffer_t() const {
      drm_i915_cmdbuffer_t val {};
      CPYT(buf);
      CPYT(sz);
      CPYT(DR1);
      CPYT(DR4);
      CPYT(num_cliprects);
      CPYT(cliprects);
      return val;
    }

    fex_drm_i915_cmdbuffer_t(drm_i915_cmdbuffer_t val)
      : buf {auto_compat_ptr {val.buf}}
      , cliprects {auto_compat_ptr {val.cliprects}} {
      CPYF(sz);
      CPYF(DR1);
      CPYF(DR4);
      CPYF(num_cliprects);
    }
  };

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

#define DRM_IOCTL_I915_GEM_MMAP_OFFSET DRM_IOWR(DRM_COMMAND_BASE + DRM_I915_GEM_MMAP_GTT, FEX::HLE::x32::I915::drm_i915_gem_mmap_offset)
#endif
} // namespace I915

namespace VC4 {
  struct FEX_ANNOTATE("alias-x86_32-drm_vc4_perfmon_get_values") FEX_ANNOTATE("fex-match") fex_drm_vc4_perfmon_get_values {
    uint32_t id;
    compat_uint64_t values_ptr;

    fex_drm_vc4_perfmon_get_values() = delete;

    operator drm_vc4_perfmon_get_values() const {
      drm_vc4_perfmon_get_values val {};
      CPYT(id);
      CPYT(values_ptr);
      return val;
    }
    fex_drm_vc4_perfmon_get_values(drm_vc4_perfmon_get_values val) {
      CPYF(id);
      CPYF(values_ptr);
    }
  };

} // namespace VC4

namespace V3D {
  struct FEX_ANNOTATE("alias-x86_32-drm_v3d_submit_csd") FEX_ANNOTATE("fex-match") fex_drm_v3d_submit_csd {
    uint32_t cfg[7];
    uint32_t coef[4];

    compat_uint64_t bo_handles;

    uint32_t bo_handle_count;

    uint32_t in_sync;

    uint32_t out_sync;

    /**
     * @name This member were added in Linux 5.15
     * Commit: 26a4dc29b74a137f45665089f6d3d633fcc9b662
     *
     * As far as I can tell this is an ABI break, Probably safe since this likely would have been padded to 8 bytes.
     * Still pretty sketchy.
     * @{ */

    uint32_t perfmon_id;
    /**  @} */

    /**
     * @name These members were added in Linux 5.17
     * Commit: bb3425efdcd99f2b4e608e850226f7107b2f993e
     * This added additional members to `drm_v3d_submit_cl` and `drm_v3d_submit_tfu` as well.
     *
     * As far as I can tell this is an ABI break for the `submit_tfu` and `submit_csd` structs.
     * `submit_cl` is safe because it it already had a flags member.
     *
     * We just need to eat the fact that if the userspace isn't compiled against Linux 5.17 headers
     * that copying this member may cause faults that we can't capture currently.
     * @{ */

    compat_uint64_t extensions;

    uint32_t flags;

    uint32_t pad;
    /**  @} */

    fex_drm_v3d_submit_csd() = default;

    operator drm_v3d_submit_csd() const {
      drm_v3d_submit_csd val {};
      memcpy(val.cfg, cfg, sizeof(cfg));
      memcpy(val.coef, coef, sizeof(coef));
      CPYT(bo_handles);
      CPYT(bo_handle_count);
      CPYT(in_sync);
      CPYT(out_sync);
      CPYT(perfmon_id);
      CPYT(extensions);
      CPYT(flags);
      CPYT(pad);
      return val;
    }

    static void SafeConvertToGuest(fex_drm_v3d_submit_csd* Result, drm_v3d_submit_csd Src, size_t IoctlSize) {
      // We need to be more careful since this API changes over time
      fex_drm_v3d_submit_csd Tmp = Src;
      memcpy(Result, &Tmp, IoctlSize);
    }

    static drm_v3d_submit_csd SafeConvertToHost(fex_drm_v3d_submit_csd* Src, size_t IoctlSize) {
      // We need to be more careful since this API changes over time
      drm_v3d_submit_csd Result {};

      // Copy the incoming variable over with memcpy
      // This way if it is smaller than expected we will zero the remaining struct
      fex_drm_v3d_submit_csd Tmp {};
      memcpy(&Tmp, Src, std::min(IoctlSize, sizeof(fex_drm_v3d_submit_csd)));

      memcpy(Result.cfg, Tmp.cfg, sizeof(cfg));
      memcpy(Result.coef, Tmp.coef, sizeof(coef));
      Result.bo_handles = Tmp.bo_handles;
      Result.bo_handle_count = Tmp.bo_handle_count;
      Result.in_sync = Tmp.in_sync;
      Result.out_sync = Tmp.out_sync;
      Result.perfmon_id = Tmp.perfmon_id;
      Result.extensions = Tmp.extensions;
      Result.flags = Tmp.flags;
      Result.pad = Tmp.pad;

      return Result;
    }

    fex_drm_v3d_submit_csd(drm_v3d_submit_csd val) {
      memcpy(cfg, val.cfg, sizeof(cfg));
      memcpy(coef, val.coef, sizeof(coef));
      CPYF(bo_handles);
      CPYF(bo_handle_count);
      CPYF(in_sync);
      CPYF(out_sync);
      CPYF(perfmon_id);
      CPYF(extensions);
      CPYF(flags);
      CPYF(pad);
    }
  };

} // namespace V3D

#include "LinuxSyscalls/x32/Ioctl/drm.inl"
#include "LinuxSyscalls/x32/Ioctl/amdgpu_drm.inl"
#include "LinuxSyscalls/x32/Ioctl/msm_drm.inl"
#include "LinuxSyscalls/x32/Ioctl/i915_drm.inl"
#include "LinuxSyscalls/x32/Ioctl/lima_drm.inl"
#include "LinuxSyscalls/x32/Ioctl/panfrost_drm.inl"
#include "LinuxSyscalls/x32/Ioctl/nouveau_drm.inl"
#include "LinuxSyscalls/x32/Ioctl/radeon_drm.inl"
#include "LinuxSyscalls/x32/Ioctl/vc4_drm.inl"
#include "LinuxSyscalls/x32/Ioctl/v3d_drm.inl"
#include "LinuxSyscalls/x32/Ioctl/panthor_drm.inl"
#include "LinuxSyscalls/x32/Ioctl/pvr_drm.inl"
#include "LinuxSyscalls/x32/Ioctl/xe_drm.inl"
} // namespace FEX::HLE::x32
#undef CPYT
#undef CPYF
