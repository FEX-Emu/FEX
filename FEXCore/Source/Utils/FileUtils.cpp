// SPDX-License-Identifier: MIT
#include <FEXCore/fextl/fmt.h>
#include <FEXCore/fextl/string.h>
#include <FEXCore/Utils/CompilerDefs.h>
#include <FEXCore/Utils/LogManager.h>

#ifndef _WIN32
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#endif

namespace FEXCore::FileUtils {
#ifndef _WIN32
static inline bool unlinkat(int fd, const char* path, bool dir) {
  if (::unlinkat(fd, path, dir ? AT_REMOVEDIR : 0) == -1) {
    return errno == ENOENT;
  }
  return true;
}

static bool RecursiveRemoveDirectory(int parent_fd, const char* Directory) {
  // Don't follow symlinks and ensure it closes on exec.
  constexpr int DIR_FLAGS = O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC;
  int dir_fd = ::openat(parent_fd, Directory, DIR_FLAGS);

  if (dir_fd == -1) {
    if (errno == ENOENT) {
      // Probably raced something. Non-error.
      return true;
    }
  }

  // Walk the directory listing.
  bool Result = true;
  // Four pages arbitrary chosen to be a balance between NFS wanting to return data in page-size granules and
  // local filesystems returning arbitrary sizes.
  size_t dirent_size = 4096 * 4;
  uint8_t* dirent_buffer = reinterpret_cast<uint8_t*>(FEXCore::Allocator::malloc(dirent_size));

  if (!dirent_buffer) {
    // Ran out of memory?
    Result = false;
    goto end;
  }

  while (true) {
    ssize_t read = getdents64(dir_fd, dirent_buffer, dirent_size);

    if (read == -1) {
      if (errno == EINVAL) {
        // Buffer too small? Scale and try again.
        dirent_size *= 2;
        FEXCore::Allocator::free(dirent_buffer);
        dirent_buffer = reinterpret_cast<uint8_t*>(FEXCore::Allocator::malloc(dirent_size));
        if (!dirent_buffer) {
          // Ran out of memory?
          Result = false;
          goto end;
        }

        continue;
      }

      // Anything else just exit.
      Result = false;
      goto end;
    }

    if (read == 0) {
      // Done.
      break;
    }

    for (size_t dirent_offset = 0; dirent_offset < read;) {
      auto path_dirent = reinterpret_cast<const struct dirent*>(dirent_buffer + dirent_offset);
      std::string_view path_name_view = path_dirent->d_name;

      if (path_name_view == "." || path_name_view == "..") {
        // Skip these two special files.
        dirent_offset += path_dirent->d_reclen;
        continue;
      }

      if (path_dirent->d_type == DT_DIR) {
        // Recurse directories as we find them and remove them.
        if (!RecursiveRemoveDirectory(dir_fd, path_dirent->d_name)) {
          Result = false;
          goto end;
        }
      }

      // Remove anything possible.
      if (!unlinkat(dir_fd, path_dirent->d_name, path_dirent->d_type == DT_DIR)) {
        // Couldn't unlink the file for some reason.
        LogMan::Msg::IFmt("Failed to remove file: {}", path_name_view);
        Result = false;
        goto end;
      }

      // dirent is a VLA so we need to increment by reported size.
      dirent_offset += path_dirent->d_reclen;
    }
  }

end:
  if (dirent_buffer) {
    FEXCore::Allocator::free(dirent_buffer);
  }

  close(dir_fd);
  return Result;
}

FEX_DEFAULT_VISIBILITY bool RecursiveRemoveDirectory(const fextl::string& Directory) {
  return RecursiveRemoveDirectory(AT_FDCWD, Directory.c_str()) && unlinkat(AT_FDCWD, Directory.c_str(), true);
}
#else
FEX_DEFAULT_VISIBILITY bool RecursiveRemoveDirectory(std::string_view Directory) {
  std::error_code ec;
  return std::filesystem::remove_all(Directory, ec) != ~0ULL;
}
#endif
} // namespace FEXCore::FileUtils
