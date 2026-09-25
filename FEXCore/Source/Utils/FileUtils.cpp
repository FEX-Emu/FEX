// SPDX-License-Identifier: MIT
#include <FEXCore/fextl/fmt.h>
#include <FEXCore/fextl/functional.h>
#include <FEXCore/fextl/string.h>
#include <FEXCore/Utils/CompilerDefs.h>
#include <FEXCore/Utils/LogManager.h>

#ifndef _WIN32
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#else
#include <windows.h>
#include <winnt.h>
#include <winternl.h>
#include <ntstatus.h>

// FEX today doesn't have Windows include in FEXCore.
extern "C" NTSTATUS WINAPI NtQueryDirectoryFile(HANDLE, HANDLE, PIO_APC_ROUTINE, PVOID, PIO_STATUS_BLOCK, PVOID, ULONG,
                                                FILE_INFORMATION_CLASS, BOOLEAN, PUNICODE_STRING, BOOLEAN);

extern "C" NTSTATUS RtlUnicodeToUTF8N(OUT PCHAR UTF8StringDestination, IN ULONG UTF8StringMaxByteCount,
                                      OUT PULONG UTF8StringActualByteCount, IN PCWCH UnicodeStringSource, IN ULONG UnicodeStringByteCount);
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

FEX_DEFAULT_VISIBILITY void WalkDirectory(std::string_view Directory,
                                          fextl::move_only_function<void(std::string_view name, bool is_dir, const void* user_data)> Callback,
                                          const void* user_data) {
  constexpr int DIR_FLAGS = O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC;
  int dir_fd = ::openat(AT_FDCWD, fextl::string(Directory).c_str(), DIR_FLAGS);
  if (dir_fd == -1) {
    if (errno == ENOENT) {
      // Probably raced something. Non-error.
      return;
    }
  }
  // Four pages arbitrary chosen to be a balance between NFS wanting to return data in page-size granules and
  // local filesystems returning arbitrary sizes.
  size_t dirent_size = 4096 * 4;
  uint8_t* dirent_buffer = reinterpret_cast<uint8_t*>(FEXCore::Allocator::malloc(dirent_size));

  if (!dirent_buffer) {
    // Ran out of memory?
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
          goto end;
        }

        continue;
      }

      // Anything else just exit.
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

      Callback(path_name_view, path_dirent->d_type == DT_DIR, user_data);

      // dirent is a VLA so we need to increment by reported size.
      dirent_offset += path_dirent->d_reclen;
    }
  }

end:
  if (dirent_buffer) {
    FEXCore::Allocator::free(dirent_buffer);
  }

  close(dir_fd);
}

#else
static inline std::optional<UNICODE_STRING> PathToNTPath(std::string_view Path) {
  UNICODE_STRING PathW;
  if (!RtlCreateUnicodeStringFromAsciiz(&PathW, fextl::string(Path).c_str())) {
    return std::nullopt;
  }
  UNICODE_STRING NTPath;
  bool Success = RtlDosPathNameToNtPathName_U(PathW.Buffer, &NTPath, nullptr, nullptr);
  RtlFreeUnicodeString(&PathW);
  if (!Success) {
    return std::nullopt;
  }

  return NTPath;
}

static inline void FreeNTPath(UNICODE_STRING Path) {
  RtlFreeUnicodeString(&Path);
}

FEX_DEFAULT_VISIBILITY void WalkDirectory(std::string_view Directory,
                                          fextl::move_only_function<void(std::string_view name, bool is_dir, const void* user_data)> Callback,
                                          const void* user_data) {
  auto NTPath = PathToNTPath(Directory);
  if (!NTPath) {
    return;
  }

  NTSTATUS Status {};

  HANDLE dir_fd {};
  OBJECT_ATTRIBUTES attr {};
  IO_STATUS_BLOCK io {};

  InitializeObjectAttributes(&attr, &*NTPath, OBJ_CASE_INSENSITIVE, nullptr, nullptr);
  Status = NtOpenFile(&dir_fd, FILE_LIST_DIRECTORY | SYNCHRONIZE, &attr, &io, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                      FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);

  FreeNTPath(*NTPath);

  if (!NT_SUCCESS(Status)) {
    return;
  }

  BOOLEAN FirstQuery = TRUE;
  size_t dirent_size = 4096 * 4;
  uint8_t* dirent_buffer = reinterpret_cast<uint8_t*>(FEXCore::Allocator::malloc(dirent_size));

  if (!dirent_buffer) {
    // Ran out of memory?
    goto end;
  }

  while (true) {
    Status = NtQueryDirectoryFile(dir_fd,
                                  nullptr, // Event
                                  nullptr, // ApcRoutine
                                  nullptr, // ApcContext
                                  &io, dirent_buffer, dirent_size, FileDirectoryInformation,
                                  FALSE,   // ReturnSingleEntry
                                  nullptr, // FileName
                                  FirstQuery);
    FirstQuery = FALSE;

    if (Status == STATUS_NO_MORE_FILES) {
      // No more files
      break;
    }

    if (!NT_SUCCESS(Status)) {
      if (Status == STATUS_BUFFER_TOO_SMALL || Status == STATUS_INFO_LENGTH_MISMATCH) {
        // Buffer too small? Scale and try again.
        dirent_size *= 2;
        FEXCore::Allocator::free(dirent_buffer);
        uint8_t* dirent_buffer = reinterpret_cast<uint8_t*>(FEXCore::Allocator::malloc(dirent_size));

        if (!dirent_buffer) {
          // Ran out of memory?
          goto end;
        }

        continue;
      }

      // Any other failure, exit loop.
      break;
    }

    // Iterate the entries returned.
    for (size_t dirent_offset = 0;;) {
      auto Info = reinterpret_cast<FILE_DIRECTORY_INFORMATION*>(dirent_buffer + dirent_offset);

      std::wstring_view EntryName(Info->FileName, Info->FileNameLength / sizeof(wchar_t));

      if (EntryName != L"." && EntryName != L"..") {
        ULONG utf8_bytes_needed = 0;
        ULONG actual_utf8_bytes = 0;
        RtlUnicodeToUTF8N(nullptr, 0, &utf8_bytes_needed, Info->FileName, Info->FileNameLength);
        char* dynamic_buf = reinterpret_cast<char*>(FEXCore::Allocator::malloc(utf8_bytes_needed + 1));
        if (dynamic_buf) {
          RtlUnicodeToUTF8N(dynamic_buf, utf8_bytes_needed + 1, &actual_utf8_bytes, Info->FileName, Info->FileNameLength);
          std::string_view name_view(dynamic_buf, actual_utf8_bytes);
          bool is_dir = (Info->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
          Callback(name_view, is_dir, user_data);
          FEXCore::Allocator::free(dynamic_buf);
        }
      }

      if (Info->NextEntryOffset == 0) {
        break;
      }

      dirent_offset += Info->NextEntryOffset;
    }
  }

end:
  if (dirent_buffer) {
    FEXCore::Allocator::free(dirent_buffer);
  }

  NtClose(dir_fd);
}

FEX_DEFAULT_VISIBILITY bool RecursiveRemoveDirectory(const fextl::string& Directory) {
  using CallbackType = void (*)(std::string_view name, bool is_dir, const void* user_data);
  struct UserData {
    CallbackType RemoveFile {};
    std::string_view base_path;
  };

  CallbackType RemoveFile = [](std::string_view name, bool is_dir, const void* user_data) {
    auto Data = reinterpret_cast<const UserData*>(user_data);

    auto full_path = std::format("{}/{}", Data->base_path, name);
    if (is_dir) {
      UserData NewData {
        .RemoveFile = Data->RemoveFile,
        .base_path = full_path,
      };

      WalkDirectory(full_path, Data->RemoveFile, &NewData);
    }

    if (is_dir) {
      RemoveDirectoryA(full_path.c_str());
    } else {
      DeleteFileA(full_path.c_str());
    }
  };

  UserData Data {
    .RemoveFile = RemoveFile,
    .base_path = Directory,
  };

  WalkDirectory(Directory, RemoveFile, &Data);

  return RemoveDirectoryA(Directory.c_str()) != 0;
}

#endif
} // namespace FEXCore::FileUtils
