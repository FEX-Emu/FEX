// SPDX-License-Identifier: MIT
#include "LinuxSyscalls/x32/Types.h"
#include "LinuxSyscalls/x32/Ioctl/HelperDefines.h"

#include <cstdint>
#include <sys/ioctl.h>

namespace FEX::HLE::x32 {
namespace f2fs {
  // There is no userspace definitions for these
  // Must define everything ourselves
  constexpr uint32_t F2FS_IOCTL_MAGIC = 0xf5;
#define F2FS_IOC_START_ATOMIC_WRITE _IO(F2FS_IOCTL_MAGIC, 1)
#define F2FS_IOC_COMMIT_ATOMIC_WRITE _IO(F2FS_IOCTL_MAGIC, 2)
#define F2FS_IOC_START_VOLATILE_WRITE _IO(F2FS_IOCTL_MAGIC, 3)
#define F2FS_IOC_RELEASE_VOLATILE_WRITE _IO(F2FS_IOCTL_MAGIC, 4)
#define F2FS_IOC_ABORT_VOLATILE_WRITE _IO(F2FS_IOCTL_MAGIC, 5)
#define F2FS_IOC_GARBAGE_COLLECT _IOW(F2FS_IOCTL_MAGIC, 6, uint32_t)
#define F2FS_IOC_WRITE_CHECKPOINT _IO(F2FS_IOCTL_MAGIC, 7)
//#define F2FS_IOC_DEFRAGMENT         _IOWR(F2FS_IOCTL_MAGIC, 8,    \
//                                          struct f2fs_defragment)
//#define F2FS_IOC_MOVE_RANGE         _IOWR(F2FS_IOCTL_MAGIC, 9,    \
//                                          struct f2fs_move_range)
//#define F2FS_IOC_FLUSH_DEVICE       _IOW(F2FS_IOCTL_MAGIC, 10,    \
//                                          struct f2fs_flush_device)
//#define F2FS_IOC_GARBAGE_COLLECT_RANGE    _IOW(F2FS_IOCTL_MAGIC, 11,    \
//                                          struct f2fs_gc_range)
#define F2FS_IOC_GET_FEATURES _IOR(F2FS_IOCTL_MAGIC, 12, uint32_t)
#define F2FS_IOC_SET_PIN_FILE _IOW(F2FS_IOCTL_MAGIC, 13, uint32_t)
#define F2FS_IOC_GET_PIN_FILE _IOR(F2FS_IOCTL_MAGIC, 14, uint32_t)
#define F2FS_IOC_PRECACHE_EXTENTS _IO(F2FS_IOCTL_MAGIC, 15)
#define F2FS_IOC_RESIZE_FS _IOW(F2FS_IOCTL_MAGIC, 16, uint64_t)
#define F2FS_IOC_GET_COMPRESS_BLOCKS _IOR(F2FS_IOCTL_MAGIC, 17, uint64_t)
#define F2FS_IOC_RELEASE_COMPRESS_BLOCKS _IOR(F2FS_IOCTL_MAGIC, 18, uint64_t)
#define F2FS_IOC_RESERVE_COMPRESS_BLOCKS _IOR(F2FS_IOCTL_MAGIC, 19, uint64_t)
//#define F2FS_IOC_SEC_TRIM_FILE            _IOW(F2FS_IOCTL_MAGIC, 20,    \
//                                          struct f2fs_sectrim_range)
#include "LinuxSyscalls/x32/Ioctl/f2fs.inl"
} // namespace f2fs
} // namespace FEX::HLE::x32
