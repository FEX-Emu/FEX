#include "LinuxSyscalls/x32/Ioctl/HelperDefines.h"
#include "LinuxSyscalls/x32/Types.h"

#include <cstdint>
#include <linux/videodev2.h>
#include <sys/ioctl.h>

#define CPYT(x) val.x = x
#define CPYF(x) x = val.x

extern "C" {
// Upstream definitions that changed over time.
struct upstream_v4l2_create_buffers {
  uint32_t index;
  uint32_t count;
  uint32_t memory;
  struct v4l2_format format;
  uint32_t capabilities;
  uint32_t flags;
  uint32_t max_num_buffers;
  uint32_t reserved[5];
};

struct upstream_v4l2_remove_buffers {
  uint32_t index;
  uint32_t count;
  uint32_t type;
  uint32_t reserved[13];
};
}

namespace FEX::HLE::x32 {
namespace V4l2 {

  struct FEX_ANNOTATE("alias-x86_32-v4l2_window") FEX_ANNOTATE("fex-match") fex_v4l2_window {
    struct v4l2_rect w;
    uint32_t field;
    uint32_t chromakey;
    compat_uptr_t clips;
    uint32_t clipcount;
    compat_uptr_t bitmap;
    uint8_t global_alpha;

    fex_v4l2_window() = default;
  };

  struct FEX_ANNOTATE("alias-x86_32-v4l2_format") FEX_ANNOTATE("fex-match") fex_v4l2_format {
    uint32_t type;
    union {
      struct v4l2_pix_format pix;
      struct v4l2_pix_format_mplane pix_mp;
      // Just a valid place holder for struct verifier.
      fex_v4l2_window win;
      struct v4l2_vbi_format vbi;
      struct v4l2_sliced_vbi_format sliced;
      struct v4l2_sdr_format sdr;
      struct v4l2_meta_format meta;
      __u8 raw_data[200];
    } fmt;

    fex_v4l2_format() = delete;

    operator v4l2_format() const {
      v4l2_format val {};
      CPYT(type);

      switch (type) {
      case V4L2_BUF_TYPE_VIDEO_CAPTURE: memcpy(&val.fmt, &fmt, sizeof(struct v4l2_pix_format)); break;
      case V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE: memcpy(&val.fmt, &fmt, sizeof(struct v4l2_pix_format_mplane)); break;
      case V4L2_BUF_TYPE_VBI_CAPTURE: memcpy(&val.fmt, &fmt, sizeof(struct v4l2_vbi_format)); break;
      case V4L2_BUF_TYPE_SLICED_VBI_CAPTURE: memcpy(&val.fmt, &fmt, sizeof(struct v4l2_sliced_vbi_format)); break;
      case V4L2_BUF_TYPE_SDR_CAPTURE: memcpy(&val.fmt, &fmt, sizeof(struct v4l2_sdr_format)); break;
      case V4L2_BUF_TYPE_META_CAPTURE: memcpy(&val.fmt, &fmt, sizeof(struct v4l2_meta_format)); break;
      case V4L2_BUF_TYPE_VIDEO_OVERLAY: break;
      default: memcpy(&val.fmt, &fmt, 200); break;
      }

      return val;
    };

    fex_v4l2_format(v4l2_format val) {
      CPYF(type);
      switch (type) {
      case V4L2_BUF_TYPE_VIDEO_CAPTURE: memcpy(&fmt, &val.fmt, sizeof(struct v4l2_pix_format)); break;
      case V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE: memcpy(&fmt, &val.fmt, sizeof(struct v4l2_pix_format_mplane)); break;
      case V4L2_BUF_TYPE_VBI_CAPTURE: memcpy(&fmt, &val.fmt, sizeof(struct v4l2_vbi_format)); break;
      case V4L2_BUF_TYPE_SLICED_VBI_CAPTURE: memcpy(&fmt, &val.fmt, sizeof(struct v4l2_sliced_vbi_format)); break;
      case V4L2_BUF_TYPE_SDR_CAPTURE: memcpy(&fmt, &val.fmt, sizeof(struct v4l2_sdr_format)); break;
      case V4L2_BUF_TYPE_META_CAPTURE: memcpy(&fmt, &val.fmt, sizeof(struct v4l2_meta_format)); break;
      case V4L2_BUF_TYPE_VIDEO_OVERLAY: break;
      default: memcpy(&fmt, &val.fmt, 200); break;
      }
    }
  };

  struct FEX_ANNOTATE("alias-x86_32-v4l2_buffer") FEX_ANNOTATE("fex-match") fex_v4l2_buffer {
    uint32_t index;
    uint32_t type;
    uint32_t bytesused;
    uint32_t flags;
    uint32_t field;
    struct timeval32 timestamp;
    struct v4l2_timecode timecode;
    uint32_t sequence;
    uint32_t memory;

    union {
      uint32_t offset;
      compat_ptr<void> userptr;
      compat_ptr<struct v4l2_plane> planes;
      int32_t fd;
    } m;
    uint32_t length;
    uint32_t reserved2;
    union {
      int32_t request_fd;
      uint32_t reserved;
    };

    fex_v4l2_buffer() = delete;

    operator v4l2_buffer() const {
      v4l2_buffer val {};
      CPYT(index);
      CPYT(type);
      CPYT(bytesused);
      CPYT(flags);
      CPYT(field);
      CPYT(timestamp);
      CPYT(timecode);
      CPYT(sequence);
      CPYT(memory);
      CPYT(length);
      CPYT(reserved2);
      CPYT(m.offset);
      CPYT(request_fd);
      return val;
    }

    fex_v4l2_buffer(v4l2_buffer val)
      : timestamp {val.timestamp}
      , m {.offset = val.m.offset} {
      CPYF(index);
      CPYF(type);
      CPYF(bytesused);
      CPYF(flags);
      CPYF(field);
      CPYF(timecode);
      CPYF(sequence);
      CPYF(memory);
      CPYF(length);
      CPYF(reserved2);
      CPYF(request_fd);
    }
  };

  struct FEX_ANNOTATE("alias-x86_32-v4l2_framebuffer") FEX_ANNOTATE("fex-match") fex_v4l2_framebuffer {
    uint32_t capability;
    uint32_t flags;
    compat_ptr<void> base;
    struct {
      uint32_t width;
      uint32_t height;
      uint32_t pixelformat;
      uint32_t field;
      uint32_t bytesperline;
      uint32_t sizeimage;
      uint32_t colorspace;
      uint32_t priv;
    } fmt;

    fex_v4l2_framebuffer() = delete;

    operator v4l2_framebuffer() const {
      v4l2_framebuffer val {};
      CPYT(capability);
      CPYT(flags);
      CPYT(base);
      memcpy(&val.fmt, &fmt, sizeof(fmt));
      return val;
    }

    fex_v4l2_framebuffer(v4l2_framebuffer val)
      : base {auto_compat_ptr {val.base}} {
      CPYF(capability);
      CPYF(flags);
      memcpy(&fmt, &val.fmt, sizeof(fmt));
    }
  };

  struct FEX_ANNOTATE("alias-x86_32-v4l2_standard") FEX_ANNOTATE("fex-match") fex_v4l2_standard {
    uint32_t index;
    compat_uint64_t id;
    uint8_t name[24];
    struct v4l2_fract frameperiod;
    uint32_t framelines;
    uint32_t reserved[4];

    fex_v4l2_standard() = delete;

    operator v4l2_standard() const {
      v4l2_standard val {};
      CPYT(index);
      CPYT(id);
      memcpy(&val.name, name, sizeof(name));
      CPYT(frameperiod);
      CPYT(framelines);
      memcpy(&val.reserved, reserved, sizeof(uint32_t) * 4);
      return val;
    }

    fex_v4l2_standard(v4l2_standard val) {
      CPYF(index);
      CPYF(id);
      memcpy(&name, val.name, sizeof(name));
      CPYF(frameperiod);
      CPYF(framelines);
      memcpy(&reserved, val.reserved, sizeof(uint32_t) * 4);
    }
  };

  struct FEX_ANNOTATE("alias-x86_32-v4l2_input") FEX_ANNOTATE("fex-match") fex_v4l2_input {
    uint32_t index;
    uint8_t name[32];
    uint32_t type;
    uint32_t audioset;
    uint32_t tuner;
    compat_uint64_t std;
    uint32_t status;
    uint32_t capabilities;
    uint32_t reserved[3];

    fex_v4l2_input() = delete;

    operator v4l2_input() const {
      v4l2_input val {};
      CPYT(index);
      memcpy(&val.name, &name, sizeof(name));
      CPYT(type);
      CPYT(audioset);
      CPYT(tuner);
      CPYT(std);
      CPYT(status);
      CPYT(capabilities);
      memcpy(&val.reserved, &reserved, sizeof(uint32_t) * 3);
      return val;
    }

    fex_v4l2_input(v4l2_input val) {
      CPYF(index);
      memcpy(&name, &val.name, sizeof(name));
      CPYF(type);
      CPYF(audioset);
      CPYF(tuner);
      CPYF(std);
      CPYF(status);
      CPYF(capabilities);
      memcpy(&reserved, &val.reserved, sizeof(uint32_t) * 3);
    }
  };

  struct FEX_ANNOTATE("alias-x86_32-v4l2_edid") FEX_ANNOTATE("fex-match") fex_v4l2_edid {
    uint32_t pad;
    uint32_t start_block;
    uint32_t blocks;
    uint32_t reserved[5];
    compat_ptr<uint8_t> edid;

    fex_v4l2_edid() = delete;

    operator v4l2_edid() const {
      v4l2_edid val {};
      CPYT(pad);
      CPYT(start_block);
      CPYT(blocks);
      memcpy(&val.reserved, &reserved, sizeof(uint32_t) * 5);
      CPYT(edid);
      return val;
    }

    fex_v4l2_edid(v4l2_edid val)
      : edid {auto_compat_ptr {val.edid}} {
      CPYF(pad);
      CPYF(start_block);
      CPYF(blocks);
      memcpy(&reserved, &val.reserved, sizeof(uint32_t) * 5);
    }
  };

  struct FEX_ANNOTATE("alias-x86_32-v4l2_ext_controls") FEX_ANNOTATE("fex-match") fex_v4l2_ext_controls {
    union {
      uint32_t ctrl_class;
      uint32_t which;
    };
    uint32_t count;
    uint32_t error_idx;
    int32_t request_fd;
    uint32_t reserved[1];
    compat_ptr<struct v4l2_ext_control> controls;

    fex_v4l2_ext_controls() = delete;

    operator v4l2_ext_controls() const {
      v4l2_ext_controls val {};
      CPYT(which);
      CPYT(count);
      CPYT(error_idx);
      CPYT(request_fd);
      CPYT(reserved[0]);
      CPYT(controls);
      return val;
    }

    fex_v4l2_ext_controls(v4l2_ext_controls val)
      : controls {auto_compat_ptr {val.controls}} {
      CPYF(which);
      CPYF(count);
      CPYF(error_idx);
      CPYF(request_fd);
      CPYF(reserved[0]);
    }
  };

  struct FEX_ANNOTATE("alias-x86_32-v4l2_event_ctrl") FEX_ANNOTATE("fex-match") fex_v4l2_event_ctrl {
    uint32_t changes;
    uint32_t type;
    union {
      int32_t value;
      compat_int64_t value64;
    };
    uint32_t flags;
    int32_t minimum;
    int32_t maximum;
    int32_t step;
    int32_t default_value;

    fex_v4l2_event_ctrl() = default;

    operator v4l2_event_ctrl() const {
      v4l2_event_ctrl val {};
      CPYT(changes);
      CPYT(type);
      CPYT(value64);
      CPYT(flags);
      CPYT(minimum);
      CPYT(maximum);
      CPYT(step);
      CPYT(default_value);
      return val;
    }

    fex_v4l2_event_ctrl(v4l2_event_ctrl val) {
      CPYF(changes);
      CPYF(type);
      CPYF(value64);
      CPYF(flags);
      CPYF(minimum);
      CPYF(maximum);
      CPYF(step);
      CPYF(default_value);
    }
  };

  struct FEX_ANNOTATE("alias-x86_32-v4l2_event") FEX_ANNOTATE("fex-match") fex_v4l2_event {
    uint32_t type;
    union {
      struct v4l2_event_vsync vsync;
      fex_v4l2_event_ctrl ctrl;
      struct v4l2_event_frame_sync frame_sync;
      struct v4l2_event_src_change src_change;
      struct v4l2_event_motion_det motion_det;
      uint8_t data[64];
    } u;
    uint32_t pending;
    uint32_t sequence;
    timespec32 timestamp;
    uint32_t id;
    uint32_t reserved[8];

    fex_v4l2_event() = delete;

    operator v4l2_event() const {
      v4l2_event val {};
      CPYT(type);
      switch (type) {
      case V4L2_EVENT_VSYNC: CPYT(u.vsync); break;
      case V4L2_EVENT_CTRL: CPYT(u.ctrl); break;
      case V4L2_EVENT_FRAME_SYNC: CPYT(u.frame_sync); break;
      case V4L2_EVENT_SOURCE_CHANGE: CPYT(u.src_change); break;
      case V4L2_EVENT_MOTION_DET: CPYT(u.motion_det); break;
      default: memcpy(&val.u.data, &u.data, 64); break;
      }
      CPYT(pending);
      CPYT(sequence);
      CPYT(timestamp);
      CPYT(id);
      memcpy(&val.reserved, &reserved, sizeof(uint32_t) * 8);
      return val;
    }

    fex_v4l2_event(v4l2_event val) {
      CPYF(type);
      switch (type) {
      case V4L2_EVENT_VSYNC: CPYF(u.vsync); break;
      case V4L2_EVENT_CTRL: CPYF(u.ctrl); break;
      case V4L2_EVENT_FRAME_SYNC: CPYF(u.frame_sync); break;
      case V4L2_EVENT_SOURCE_CHANGE: CPYF(u.src_change); break;
      case V4L2_EVENT_MOTION_DET: CPYF(u.motion_det); break;
      default: memcpy(&u.data, &val.u.data, 64); break;
      }
      CPYF(pending);
      CPYF(sequence);
      CPYF(timestamp);
      CPYF(id);
      memcpy(&reserved, &val.reserved, sizeof(uint32_t) * 8);
    }
  };

  struct FEX_ANNOTATE("alias-x86_32-upstream_v4l2_create_buffers") FEX_ANNOTATE("fex-match") fex_v4l2_create_buffers {
    uint32_t index;
    uint32_t count;
    uint32_t memory;
    fex_v4l2_format format;
    uint32_t capabilities;
    uint32_t flags;
    uint32_t max_num_buffers;
    uint32_t reserved[5];

    fex_v4l2_create_buffers() = delete;
    operator upstream_v4l2_create_buffers() const {
      upstream_v4l2_create_buffers val {};
      CPYT(index);
      CPYT(count);
      CPYT(memory);
      CPYT(format);
      CPYT(capabilities);
      CPYT(flags);
      CPYT(max_num_buffers);
      memcpy(&val.reserved, &reserved, sizeof(uint32_t) * 5);
      return val;
    }

    fex_v4l2_create_buffers(upstream_v4l2_create_buffers val)
      : format {val.format} {
      CPYF(index);
      CPYF(count);
      CPYF(memory);
      CPYF(capabilities);
      CPYF(flags);
      CPYF(max_num_buffers);
      memcpy(&reserved, &val.reserved, sizeof(uint32_t) * 5);
    }
  };

#include "LinuxSyscalls/x32/Ioctl/v4l2.inl"
} // namespace V4l2
} // namespace FEX::HLE::x32
#undef CPYT
#undef CPYF
