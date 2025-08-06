#include "LinuxSyscalls/x32/Ioctl/HelperDefines.h"
#include "LinuxSyscalls/x32/Types.h"

#include <cstdint>
#include <linux/videodev2.h>
#include <sys/ioctl.h>

#define CPYT(x) val.x = x
#define CPYF(x) x = val.x

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

#include "LinuxSyscalls/x32/Ioctl/v4l2.inl"
} // namespace V4l2
} // namespace FEX::HLE::x32
#undef CPYT
#undef CPYF
